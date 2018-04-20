/*****************************************************************************
 *
 * MODULE:             JN-AN-1201
 *
 * COMPONENT:          app_zcl_remote_task.c
 *
 * DESCRIPTION:        ZHA Remote Control Behavior (Implementation)
 *
 *****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5168, JN5164,
 * JN5161, JN5148, JN5142, JN5139].
 * You, and any third parties must reproduce the copyright and warranty notice
 * and any other legend of ownership on each copy or partial copy of the
 * software.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright NXP B.V. 2013. All rights reserved
 *
 ****************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <jendefs.h>
#include <appapi.h>
#include <string.h>
#include "os.h"
#include "os_gen.h"
#include "zps_gen.h"
#include "pdum_apl.h"
#include "pdum_gen.h"
#include "pdm.h"
#include "dbg.h"
#include "pwrm.h"

#include "zps_apl_af.h"
#include "zps_apl_zdo.h"
#include "zps_apl_aib.h"
#include "zps_apl_zdp.h"
#include "rnd_pub.h"
#include "mac_pib.h"

#include "app_timer_driver.h"

#include "zcl_options.h"
#include "zcl.h"
#include "ha.h"
#include "zone.h"
#include "app_common.h"
#include "zha_ZONE_node.h"
#include "ahi_aes.h"
#include "app_events.h"
#ifdef CSW
#include "GenericBoard.h"
#endif
#include "ha.h"

#include "app_sleep_functions.h"


#include "app_zcl_ZONE_task.h"
#include "alsdriver.h"
#include "app_ias_indicator.h"
#include "app_ias_unenroll_req.h"
#include "app_ias_save.h"
#include "PingParent.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifdef DEBUG_ZCL
    #define TRACE_ZCL   TRUE
#else
    #define TRACE_ZCL   TRUE
#endif

#ifdef DEBUG_REMOTE_TASK
    #define TRACE_ZONE_TASK   TRUE
#else
    #define TRACE_ZONE_TASK   FALSE
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

PRIVATE void APP_ZCL_cbGeneralCallback(tsZCL_CallBackEvent *psEvent);
PRIVATE void APP_ZCL_cbEndpointCallback(tsZCL_CallBackEvent *psEvent);
PRIVATE void vAPP_ZCL_DeviceSpecific_Init(void);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern tsDeviceDesc sDeviceDesc;
extern uint16 u16GroupId;
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

tsHA_IASZoneDevice sDevice;  //定义IASZONE的所有ClusterID的变量

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: APP_ZCL_vInitialise
 *
 * DESCRIPTION:
 * Initialises ZCL related functions
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_ZCL_vInitialise(void)
{
    teZCL_Status eZCL_Status;

    /* Initialise ZHA,ZCL共同的东西 */
    eZCL_Status = eHA_Initialise(&APP_ZCL_cbGeneralCallback, apduZCL);//共同的回调函数,定义ZCL的回调函数、PDUM
    if (eZCL_Status != E_ZCL_SUCCESS)
    {
        DBG_vPrintf(TRACE_ZCL, "Error: eHA_Initialise returned %d\r\n", eZCL_Status);
    }
    //在这个地方设定各个想设定的ClusterID的默认的属性

    /* Register ZHA EndPoint */
   //定义与ZONE有关的变量ClusterID属性命令等，ZONE特有的东西，ZONE_ZONE_ENDPOINT端点的回调函数
    eZCL_Status = eHA_RegisterIASZoneEndPoint(ZONE_ZONE_ENDPOINT,APP_ZCL_cbEndpointCallback,&sDevice);

    if (eZCL_Status != E_ZCL_SUCCESS)
    {
            DBG_vPrintf(TRACE_ZONE_TASK, "Error: eApp_HA_RegisterEndpoint:%d\r\n", eZCL_Status);
    }
    else
    {
    	//定义本设备特有的东西
        vAPP_ZCL_DeviceSpecific_Init();
    }

    DBG_vPrintf(TRACE_ZONE_TASK, "Chan Mask %08x\n", ZPS_psAplAibGetAib()->apsChannelMask);
    DBG_vPrintf(TRACE_ZONE_TASK, "\nRxIdle TRUE");
    OS_eStartSWTimer(APP_TickTimer, ZCL_TICK_TIME, NULL);
}

/****************************************************************************
 *
 * NAME: ZCL_Task
 *
 * DESCRIPTION:
 * ZCL Task for the Switch
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(ZCL_Task)
{
    ZPS_tsAfEvent sStackEvent;
    tsZCL_CallBackEvent sCallBackEvent;
    sCallBackEvent.pZPSevent = &sStackEvent;

    /*
     * If the 1 second tick timer has expired, restart it and pass
     * the event on to ZCL
     */
    if (OS_eGetSWTimerStatus(APP_TickTimer) == OS_E_SWTIMER_EXPIRED)
    {
        sCallBackEvent.eEventType = E_ZCL_CBET_TIMER;
        OS_eContinueSWTimer(APP_TickTimer, ZCL_TICK_TIME, NULL);//1秒钟
        vZCL_EventHandler(&sCallBackEvent);


    }
    /* If there is a stack event to process, pass it on to ZCL */
    sStackEvent.eType = ZPS_EVENT_NONE;
    if (OS_eCollectMessage(APP_msgZpsEvents_ZCL, &sStackEvent) == OS_E_OK)
    {
        DBG_vPrintf(TRACE_ZCL, "\nZCL_Task event:%d",sStackEvent.eType);
        sCallBackEvent.eEventType = E_ZCL_CBET_ZIGBEE_EVENT;
        vZCL_EventHandler(&sCallBackEvent);
    }

}
/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: APP_ZCL_cbGeneralCallback
 *
 * DESCRIPTION:
 * General callback for ZCL events
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void APP_ZCL_cbGeneralCallback(tsZCL_CallBackEvent *psEvent)
{

    switch (psEvent->eEventType)
    {
        case E_ZCL_CBET_LOCK_MUTEX:
            break;

        case E_ZCL_CBET_UNLOCK_MUTEX:
            break;

        case E_ZCL_CBET_UNHANDLED_EVENT:
            DBG_vPrintf(TRACE_ZCL, "EVT: Unhandled Event\r\n");
            if (psEvent->pZPSevent != NULL) {
                DBG_vPrintf(SLEEP_INFO, "of type %02x\n", psEvent->pZPSevent->eType);
                if (psEvent->pZPSevent->eType == 20) {
                    DBG_vPrintf(SLEEP_INFO, "Bind Evt Status %02x, Errors %d\n",
                            psEvent->pZPSevent->uEvent.sBindRequestServerEvent.u8Status,
                            psEvent->pZPSevent->uEvent.sBindRequestServerEvent.u32FailureCount);
                    vCheckForSleepPostTx();
                }
            }
            break;

        case E_ZCL_CBET_READ_ATTRIBUTES_RESPONSE:
            DBG_vPrintf(TRACE_ZCL, "EVT: Read attributes response\r\n");
            break;

        case E_ZCL_CBET_READ_REQUEST:
            DBG_vPrintf(TRACE_ZCL, "EVT: Read request\r\n");
            break;

        case E_ZCL_CBET_DEFAULT_RESPONSE:
            DBG_vPrintf(TRACE_ZCL, "EVT: Default response\r\n");
            break;

        case E_ZCL_CBET_ERROR:
            DBG_vPrintf(TRACE_ZCL, "EVT: Error\r\n");
            break;

        case E_ZCL_CBET_TIMER:
            break;

        case E_ZCL_CBET_ZIGBEE_EVENT:
            DBG_vPrintf(TRACE_ZCL, "EVT: ZigBee\r\n");
            break;

        case E_ZCL_CBET_CLUSTER_CUSTOM:
            DBG_vPrintf(TRACE_ZCL, "EP EVT: Custom\r\n");
            break;

        default:
            DBG_vPrintf(TRACE_ZCL, "Invalid event type\r\n");
            break;
    }
}

/****************************************************************************
 *
 * NAME: APP_ZCL_cbEndpointCallback
 *
 * DESCRIPTION:
 * Endpoint specific callback for ZCL events
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void APP_ZCL_cbEndpointCallback(tsZCL_CallBackEvent *psEvent)
{
    switch (psEvent->eEventType)
    {
        case E_ZCL_CBET_LOCK_MUTEX:
            break;
        case E_ZCL_CBET_UNLOCK_MUTEX:
            break;
        case E_ZCL_CBET_UNHANDLED_EVENT:
        case E_ZCL_CBET_READ_ATTRIBUTES_RESPONSE:
        case E_ZCL_CBET_READ_REQUEST:
        case E_ZCL_CBET_DEFAULT_RESPONSE:
        case E_ZCL_CBET_ERROR:
        	/*发送失败*/

        //	break;
        case E_ZCL_CBET_TIMER:
        case E_ZCL_CBET_ZIGBEE_EVENT:
        	DBG_vPrintf(TRACE_ZCL, "eEventType is %02x" , psEvent->eEventType);
            DBG_vPrintf(TRACE_ZCL, "EP EVT:No action\r\n");
            break;

        case E_ZCL_CBET_READ_INDIVIDUAL_ATTRIBUTE_RESPONSE:
            DBG_vPrintf(TRACE_ZONE_TASK, " Read Attrib Rsp %d %02x\n", psEvent->uMessage.sIndividualAttributeResponse.eAttributeStatus,
                *((uint8*)psEvent->uMessage.sIndividualAttributeResponse.pvAttributeData));
            vPingRecv(psEvent);
            break;

        case E_ZCL_CBET_CHECK_ATTRIBUTE_RANGE:
            vProcessWriteAttributeRangeCheck(psEvent);
        break;
        case E_ZCL_CBET_WRITE_INDIVIDUAL_ATTRIBUTE:
            {
                tsZCL_IndividualAttributesResponse   *psIndividualAttributeResponse = &psEvent->uMessage.sIndividualAttributeResponse;
                DBG_vPrintf(TRACE_ZCL,"Cluster Id = %04x\n",psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum);
                DBG_vPrintf(TRACE_ZCL,"Attribute Id = %04x\n",psIndividualAttributeResponse->u16AttributeEnum);
                DBG_vPrintf(TRACE_ZCL,"Attribute Type = %d\n",psIndividualAttributeResponse->eAttributeDataType);
                DBG_vPrintf(TRACE_ZCL,"Attribute Status = %d\n",psIndividualAttributeResponse->eAttributeStatus);
                DBG_vPrintf(TRACE_ZCL,"Attribute Value = %16llx\n",(uint64)(*((uint64*)psIndividualAttributeResponse->pvAttributeData)));

                if(
                        ( SECURITY_AND_SAFETY_CLUSTER_ID_IASZONE == psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum ) &&
                        ( E_CLD_IASZONE_ATTR_ID_IAS_CIE_ADDRESS == psIndividualAttributeResponse->u16AttributeEnum ) &&
                        ( E_ZCL_CMDS_SUCCESS == psIndividualAttributeResponse->eAttributeStatus)
                    )
                {

                    if(   (uint64)(*((uint64*)psIndividualAttributeResponse->pvAttributeData)) == 0      )
                    {
                    }
                    else
                    {

                        /*Make Binding for the Client as all the communication will go to the client*/
                        ZPS_teStatus eStatus=ZPS_eAplZdoBind(
                                                psEvent->pZPSevent->uEvent.sApsDataIndEvent.u16ClusterId,              /*uint16 u16ClusterId,*/
                                                psEvent->pZPSevent->uEvent.sApsDataIndEvent.u8DstEndpoint,             /*uint8 u8SrcEndpoint,*/
                                                psEvent->pZPSevent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr,       /*uint16 u16DstAddr,*/
                                                (uint64)(*((uint64*)psIndividualAttributeResponse->pvAttributeData)),  /*uint64 u64DstIeeeAddr,*/
                                                psEvent->pZPSevent->uEvent.sApsDataIndEvent.u8SrcEndpoint);           /*uint8 u8DstEndpoint);*/
                        vSetIASDeviceState(E_IAS_DEV_STATE_READY_TO_ENROLL);
                        vSaveIASZoneAttributes(psEvent->pZPSevent->uEvent.sApsDataIndEvent.u8DstEndpoint);
                        DBG_vPrintf(TRACE_ZCL,"Binding status =%d",eStatus);
                        vSetPingAddress(psEvent->pZPSevent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr);
                    }
                }
            }
            break;



        case E_ZCL_CBET_CLUSTER_CUSTOM:
            DBG_vPrintf(TRACE_ZCL, "EP EVT: Custom %04x\r\n", psEvent->uMessage.sClusterCustomMessage.u16ClusterId);

            switch (psEvent->uMessage.sClusterCustomMessage.u16ClusterId)
            {

                case SECURITY_AND_SAFETY_CLUSTER_ID_IASZONE:
                {
                    tsCLD_IASZoneCallBackMessage *psCallBackMessage = (tsCLD_IASZoneCallBackMessage *)psEvent->uMessage.sClusterCustomMessage.pvCustomData;

                    DBG_vPrintf(TRACE_ZCL,"\nIAS Zone Command Id = %d\n",psCallBackMessage->u8CommandId);

                    switch (psCallBackMessage->u8CommandId)
                    {
                        case E_CLD_IASZONE_CMD_ZONE_ENROLL_RESP:
                        {
                            tsCLD_IASZone_EnrollResponsePayload *psZoneEnrollResponsePayload = (tsCLD_IASZone_EnrollResponsePayload *)psCallBackMessage->uMessage.psZoneEnrollResponsePayload;
                            DBG_vPrintf(TRACE_ZCL, "Zone Enroll Response\n ");
                            DBG_vPrintf(TRACE_ZCL, " Status = %d\n",psZoneEnrollResponsePayload->e8EnrollResponseCode);
                            DBG_vPrintf(TRACE_ZCL, " ZoneId = %d\n",psZoneEnrollResponsePayload->u8ZoneID);
                            if(E_CLD_IASZONE_ENROLL_RESP_SUCCESS==psZoneEnrollResponsePayload->e8EnrollResponseCode)
                            {
                                vSetIASDeviceState(E_IAS_DEV_STATE_ENROLLED);
                                eCLD_IASZoneUpdateZoneState(psEvent->u8EndPoint,E_CLD_IASZONE_STATE_ENROLLED);
                                eCLD_IASZoneUpdateZoneID(psEvent->u8EndPoint,psZoneEnrollResponsePayload->u8ZoneID);
                                vSaveIASZoneAttributes(psEvent->u8EndPoint);
                                vScheduleSleep();
                            }

                        }
                        break;
                    }
                }
                break;

                case 0x1000:
                    DBG_vPrintf(TRACE_ZCL, "\n    - for 0x1000");
                    break;

                default:
                    DBG_vPrintf(TRACE_ZCL, "- for unknown cluster %d\r\n", psEvent->uMessage.sClusterCustomMessage.u16ClusterId);
                    break;
            }
            break;

            case E_ZCL_CBET_CLUSTER_UPDATE:

                DBG_vPrintf(TRACE_ZCL, "Update Id %04x\n", psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum);
                break;

        default:
            DBG_vPrintf(TRACE_ZCL, "EP EVT: Invalid event type\r\n");
            break;
    }
}

/****************************************************************************
 *
 * NAME: vAPP_ZCL_DeviceSpecific_Init
 *
 * DESCRIPTION:
 * ZCL Device Specific initialization
 *
 * PARAMETER: void
 *
 * RETURNS: void
 *
 ****************************************************************************/
PRIVATE void vAPP_ZCL_DeviceSpecific_Init(void)
{
    teZCL_Status eZCL_Status;

    /* Initialize the strings in Basic */
    memcpy(sDevice.sBasicServerCluster.au8ManufacturerName, "XTHN", CLD_BAS_MANUF_NAME_SIZE);
    memcpy(sDevice.sBasicServerCluster.au8ModelIdentifier, "HN-DWWW", CLD_BAS_MODEL_ID_SIZE);
    memcpy(sDevice.sBasicServerCluster.au8DateCode, "20171111", CLD_BAS_DATE_SIZE);

    /* Initialize the Zone Type attribute */
    eZCL_Status = eCLD_IASZoneUpdateZoneType(ZONE_ZONE_ENDPOINT,E_CLD_IASZONE_TYPE_STANDARD_WARNING_DEVICE);//水浸

    DBG_vPrintf(TRACE_ZCL, "eZCL_Status %d\n", eZCL_Status);
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
