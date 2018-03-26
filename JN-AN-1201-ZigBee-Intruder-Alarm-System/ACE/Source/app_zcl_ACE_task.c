/*****************************************************************************
 *
 * MODULE:             JN-AN-1201
 *
 * COMPONENT:          app_zcl_ACE_task.c
 *
 * DESCRIPTION:        ZHA ACE controller Behavior (Implementation)
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
#include "os.h"
#include "os_gen.h"
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
#include "Utilities.h"
#include "zcl_options.h"
#include "zcl.h"
#include "ha.h"
#include "app_common.h"
#include "zha_ACE_node.h"
#include "ahi_aes.h"
#include "app_events.h"
#include "ha.h"
#include "app_led_control.h"
#include "app_zcl_ACE_task.h"
#include "app_ias_unenroll_req.h"
#include "app_ias_save.h"
#include "app_ias_indicator.h"
#include "PingParent.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifdef DEBUG_ZCL
    #define TRACE_ZCL   TRUE
#else
    #define TRACE_ZCL   FALSE
#endif

#ifdef DEBUG_ACE_TASK
    #define TRACE_ACE_TASK   TRUE
#else
    #define TRACE_ACE_TASK   FALSE
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
tsHA_IASACE sDevice;
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
bool bPollFailed;
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

    /* Initialise ZHA */
    eZCL_Status = eHA_Initialise(&APP_ZCL_cbGeneralCallback, apduZCL);
    if (eZCL_Status != E_ZCL_SUCCESS)
    {
        DBG_vPrintf(TRACE_ZCL, "Error: eHA_Initialise returned %d\r\n", eZCL_Status);
    }

    /* Register ZHA EndPoint */
    eZCL_Status = eHA_RegisterIASACEEndPoint(ACE_ENDPOINT_ID,APP_ZCL_cbEndpointCallback,&sDevice);
    if (eZCL_Status != E_ZCL_SUCCESS)
    {
            DBG_vPrintf(TRACE_ACE_TASK, "Error: eApp_HA_RegisterEndpoint:%d\r\n", eZCL_Status);
    }

    vAPP_ZCL_DeviceSpecific_Init();

    DBG_vPrintf(TRACE_ACE_TASK, "Chan Mask %08x\n", ZPS_psAplAibGetAib()->apsChannelMask);
    DBG_vPrintf(TRACE_ACE_TASK, "\nRxIdle TRUE");

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
        OS_eContinueSWTimer(APP_TickTimer, ZCL_TICK_TIME, NULL);
        vZCL_EventHandler(&sCallBackEvent);

        #ifdef SLEEP_ENABLE
        if( (sDeviceDesc.eNodeState == E_RUNNING) ||
                (sDeviceDesc.eNodeState == E_REJOINING))
                vUpdateKeepAliveTimer();
        #endif
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
    uint8 u8i = 0;
    switch (psEvent->eEventType)
    {
        case E_ZCL_CBET_LOCK_MUTEX:
            break;
        case E_ZCL_CBET_UNLOCK_MUTEX:
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
                                                psEvent->pZPSevent->uEvent.sApsDataIndEvent.u16ClusterId,            /*uint16 u16ClusterId,   */
                                                psEvent->pZPSevent->uEvent.sApsDataIndEvent.u8DstEndpoint,           /*uint8 u8SrcEndpoint,   */
                                                psEvent->pZPSevent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr,     /*uint16 u16DstAddr,     */
                                                (uint64)(*((uint64*)psIndividualAttributeResponse->pvAttributeData)),/*uint64 u64DstIeeeAddr, */
                                                psEvent->pZPSevent->uEvent.sApsDataIndEvent.u8SrcEndpoint);          /*uint8 u8DstEndpoint);  */

                        /*Make Binding for the ACE server as all the communication will go from the ACE client*/
                        eStatus|=ZPS_eAplZdoBind(
                                                SECURITY_AND_SAFETY_CLUSTER_ID_IASACE,                               /*uint16 u16ClusterId,   */
                                                psEvent->pZPSevent->uEvent.sApsDataIndEvent.u8DstEndpoint,           /*uint8 u8SrcEndpoint,   */
                                                psEvent->pZPSevent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr,     /*uint16 u16DstAddr,     */
                                                (uint64)(*((uint64*)psIndividualAttributeResponse->pvAttributeData)),/*uint64 u64DstIeeeAddr, */
                                                psEvent->pZPSevent->uEvent.sApsDataIndEvent.u8SrcEndpoint);
                        vSetIASDeviceState(E_IAS_DEV_STATE_READY_TO_ENROLL);
                        vSaveIASZoneAttributes(psEvent->pZPSevent->uEvent.sApsDataIndEvent.u8DstEndpoint);
                        DBG_vPrintf(TRACE_ZCL,"Binding status =%d",eStatus);
                        vSetPingAddress(psEvent->pZPSevent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr);

                    }
                }
            }
            break;
        case E_ZCL_CBET_UNHANDLED_EVENT:
        case E_ZCL_CBET_READ_ATTRIBUTES_RESPONSE:
        case E_ZCL_CBET_READ_REQUEST:
        case E_ZCL_CBET_DEFAULT_RESPONSE:
        case E_ZCL_CBET_ERROR:
        case E_ZCL_CBET_TIMER:
        case E_ZCL_CBET_ZIGBEE_EVENT:
            DBG_vPrintf(TRACE_ZCL, "EP EVT:No action\r\n");
            break;

        case E_ZCL_CBET_READ_INDIVIDUAL_ATTRIBUTE_RESPONSE:
            DBG_vPrintf(TRACE_ACE_TASK, " Read Attrib Rsp %d %02x\n", psEvent->uMessage.sIndividualAttributeResponse.eAttributeStatus,
                *((uint8*)psEvent->uMessage.sIndividualAttributeResponse.pvAttributeData));
            vPingRecv(psEvent);
            break;

        case E_ZCL_CBET_CLUSTER_CUSTOM:
            DBG_vPrintf(TRACE_ZCL, "EP EVT: Custom %04x\r\n", psEvent->uMessage.sClusterCustomMessage.u16ClusterId);

            switch (psEvent->uMessage.sClusterCustomMessage.u16ClusterId)
            {

                case GENERAL_CLUSTER_ID_IDENTIFY:
                    DBG_vPrintf(TRACE_ZCL, "- for identify cluster\r\n");
                    break;

                case GENERAL_CLUSTER_ID_GROUPS:
                    DBG_vPrintf(TRACE_ZCL, "- for groups cluster\r\n");
                    break;

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
                            }
                        }
                        break;
                    }
                }
                break;

                case SECURITY_AND_SAFETY_CLUSTER_ID_IASACE:
                    DBG_vPrintf(TRACE_ZCL, "- for IAS ACE cluster\r\n");
                    tsCLD_IASACECallBackMessage *psACE = ((tsCLD_IASACECallBackMessage *)psEvent->uMessage.sClusterCustomMessage.pvCustomData);
                    switch(psACE->u8CommandId)
                    {
                        case E_CLD_IASACE_CMD_ARM_RESP:
                        {
                            tsCLD_IASACE_ArmRespPayload *psArmRespPayload = psACE->uMessage.psArmRespPayload;
                            DBG_vPrintf(TRACE_ZCL, "Armresp %d\r\n",psArmRespPayload->eArmNotification);
                            break;
                        }
                        case E_CLD_IASACE_CMD_BYPASS_RESP:
                        {
                            tsCLD_IASACE_BypassRespPayload *psBypassRespPayload = psACE->uMessage.psBypassRespPayload;
                            DBG_vPrintf(TRACE_ZCL, "Bypass Response %d\r\n",psBypassRespPayload->u8NumofZones);
                            #if TRACE_ZCL
                                for( u8i = 0 ; u8i < psBypassRespPayload->u8NumofZones; u8i++)
                                {
                                    DBG_vPrintf(TRACE_ZCL, "%x:",psBypassRespPayload->pu8BypassResult[u8i]);
                                }
                                DBG_vPrintf(TRACE_ZCL, "%\r%\n");
                            #endif
                            break;
                        }
                        case E_CLD_IASACE_CMD_GET_ZONE_ID_MAP_RESP:
                        {
                            tsCLD_IASACE_GetZoneIDMapRespPayload *psGetZoneIDMapRespPayload = psACE->uMessage.psGetZoneIDMapRespPayload;
                            DBG_vPrintf(TRACE_ZCL, "ZoneID MapResponse\r\n");
                            for(u8i=0;u8i<CLD_IASACE_MAX_BYTES_FOR_NUM_OF_ZONES;u8i++)
                            {
                                DBG_vPrintf(TRACE_ZCL, "%04x:",psGetZoneIDMapRespPayload->au16ZoneIDMap[u8i]);
                            }
                            DBG_vPrintf(TRACE_ZCL, "%\r%\n");

                            for(u8i=0;u8i<CLD_IASACE_MAX_BYTES_FOR_NUM_OF_ZONES;u8i++)
                            {
                                if(((psGetZoneIDMapRespPayload->au16ZoneIDMap[0] >> u8i) & 0x01))
                                {
                                    if(eCommandState == E_CMD_STATE_SEND_GET_ZONE_INFO)
                                    {
                                        APP_ZCL_vSendHAGetZoneInfo(u8i);
                                    }
                                    if(eCommandState == E_CMD_STATE_SEND_GET_ZONE_STATUS_SEND)
                                    {
                                        APP_ZCL_vSendHAGetZoneStatus(FALSE);
                                    }
                                }
                            }
                            eCommandState = E_CMD_STATE_IDLE;
                            break;
                        }
                        case E_CLD_IASACE_CMD_GET_ZONE_INFO_RESP:
                        {
                            tsCLD_IASACE_GetZoneInfoRespPayload *psGetZoneInfoPayload = psACE->uMessage.psGetZoneInfoRespPayload;
                            DBG_vPrintf(TRACE_ZCL, "Zone Info Response %x:%04x:%016llx\r\n",psGetZoneInfoPayload->u8ZoneID,
                                                                                            psGetZoneInfoPayload->u16ZoneType,
                                                                                            psGetZoneInfoPayload->u64IeeeAddress);
                            break;
                        }
                        case E_CLD_IASACE_CMD_ZONE_STATUS_CHANGED:
                        {
                            tsCLD_IASACE_ZoneStatusChangedPayload *psZoneStatusChangedPayload = psACE->uMessage.psZoneStatusChangedPayload;
                            DBG_vPrintf(TRACE_ZCL, "Zone Status Changed %x:%04x:%x\r\n",psZoneStatusChangedPayload->u8ZoneID,
                                                                                            psZoneStatusChangedPayload->eZoneStatus,
                                                                                            psZoneStatusChangedPayload->eAudibleNotification);
                            break;
                        }
                        case E_CLD_IASACE_CMD_PANEL_STATUS_CHANGED:
                        case E_CLD_IASACE_CMD_GET_PANEL_STATUS_RESP:
                        {
                            tsCLD_IASACE_PanelStatusChangedOrGetPanelStatusRespPayload *psPanelStatusChangedOrGetPanelStatusRespPayload = psACE->uMessage.psPanelStatusChangedOrGetPanelStatusRespPayload;
                            if((psPanelStatusChangedOrGetPanelStatusRespPayload->ePanelStatus == E_CLD_IASACE_PANEL_STATUS_PANEL_NOT_READY_TO_ARM) &&
                                    (psACE->u8CommandId == E_CLD_IASACE_CMD_PANEL_STATUS_CHANGED))
                                APP_ZCL_vSendHAGetZoneStatus(TRUE);
                            DBG_vPrintf(TRACE_ZCL, "Panel Status Changed %x:%x:%x:%x\r\n",psPanelStatusChangedOrGetPanelStatusRespPayload->ePanelStatus,
                                                                                         psPanelStatusChangedOrGetPanelStatusRespPayload->u8SecondsRemaining,
                                                                                         psPanelStatusChangedOrGetPanelStatusRespPayload->eAudibleNotification,
                                                                                         psPanelStatusChangedOrGetPanelStatusRespPayload->eAlarmStatus);
                            break;
                        }
                        case E_CLD_IASACE_CMD_SET_BYPASSED_ZONE_LIST:
                        {
                            tsCLD_IASACE_SetBypassedZoneListPayload *psSetBypassedZoneListPayload = psACE->uMessage.psSetBypassedZoneListPayload;
                            DBG_vPrintf(TRACE_ZCL, "Set Bypassed Zone List %x\r\n",psSetBypassedZoneListPayload->u8NumofZones);
                            #if TRACE_ZCL
                                for(u8i = 0 ;u8i < psSetBypassedZoneListPayload->u8NumofZones ;u8i++)
                                {
                                    DBG_vPrintf(TRACE_ZCL, "%x:",psSetBypassedZoneListPayload->pu8ZoneID[u8i]);
                                }
                                DBG_vPrintf(TRACE_ZCL, "%\r%\n");
                            #endif
                            break;
                        }
                        case E_CLD_IASACE_CMD_GET_ZONE_STATUS_RESP :
                        {
                            tsCLD_IASACE_GetZoneStatusRespPayload *psGetZoneStatusRespPayload = psACE->uMessage.psGetZoneStatusRespPayload;
                            uint16 u16ZoneStatus = 0;
                            DBG_vPrintf(TRACE_ZCL, "Get Zone Status Response %x\r\n",psGetZoneStatusRespPayload->u8NumofZones);
                            while(u8i < (psGetZoneStatusRespPayload->u8NumofZones * 3))
                            {
                                DBG_vPrintf(TRACE_ZCL, "%x:",psGetZoneStatusRespPayload->pu8ZoneStatus[u8i++]);
                                memcpy(&u16ZoneStatus,&psGetZoneStatusRespPayload->pu8ZoneStatus[u8i],2);
                                DBG_vPrintf(TRACE_ZCL, "%04x\r\n",u16ZoneStatus);
                                u8i+=2;
                            }
                            break;
                        }
                        default:
                            break;

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
                if (psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum == GENERAL_CLUSTER_ID_IDENTIFY)
                {
                    #ifdef SLEEP_ENABLE
                        vReloadSleepTimers();
                    #endif
                }
                break;

        default:
            DBG_vPrintf(TRACE_ZCL, "EP EVT: Invalid event type\r\n");
            break;
    }
}


/****************************************************************************
 *
 * NAME: APP_ZCL_vSendHAArm
 *
 * DESCRIPTION:
 *    Send out Arm Command, the address mode
 *    is bound
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_ZCL_vSendHAArm(void) {

    uint8 u8Seq;
    tsZCL_Address sAddress = {0};
    teZCL_Status eZCL_Status;
    tsCLD_IASACE_ArmPayload sPayload;

    eCommandState = E_CMD_STATE_IDLE;

    sAddress.eAddressMode = E_ZCL_AM_BOUND_NO_ACK;
    sPayload.eArmMode = u8ArmMode;
    sPayload.sArmDisarmCode.u8Length = 0;
    sPayload.u8ZoneID = sDevice.sIASZoneServerCluster.u8ZoneId;

    eZCL_Status =  eCLD_IASACE_ArmSend(
            u8MyEndpoint,
            0,
            &sAddress,
            &u8Seq,
            &sPayload);

    DBG_vPrintf(TRACE_ZCL, "eZCL_Status %d\n", eZCL_Status);
}
/****************************************************************************
 *
 * NAME: APP_ZCL_vSendHABypass
 *
 * DESCRIPTION:
 *    Send out Bypass Command, the address mode
 *    is bound
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_ZCL_vSendHABypass(void) {

    uint8 u8Seq;
    tsZCL_Address sAddress = {0};
    teZCL_Status eZCL_Status;
    tsCLD_IASACE_BypassPayload sPayload;

    eCommandState = E_CMD_STATE_IDLE;

    sAddress.eAddressMode = E_ZCL_AM_BOUND_NO_ACK;
    sPayload.u8NumOfZones = u8SelZoneID;
    sPayload.sArmDisarmCode.u8Length = 0;
    sPayload.pu8ZoneID = &au8ZoneIDList[0];

    eZCL_Status =  eCLD_IASACE_BypassSend(
            u8MyEndpoint,
            0,
            &sAddress,
            &u8Seq,
            &sPayload);

    DBG_vPrintf(TRACE_ZCL, "eZCL_Status %d\n", eZCL_Status);
}

/****************************************************************************
 *
 * NAME: APP_ZCL_vSendHAGetBypassedList
 *
 * DESCRIPTION:
 *    Send out Get Bypassed List Command, the address mode
 *    is bound
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_ZCL_vSendHAGetBypassedList(void) {

    uint8 u8Seq;
    tsZCL_Address sAddress = {0};
    teZCL_Status eZCL_Status;

    eCommandState = E_CMD_STATE_IDLE;

    sAddress.eAddressMode = E_ZCL_AM_BOUND_NO_ACK;

    eZCL_Status =  eCLD_IASACE_GetBypassedZoneListSend(
            u8MyEndpoint,
            0,
            &sAddress,
            &u8Seq);

    DBG_vPrintf(TRACE_ZCL, "eZCL_Status %d\n", eZCL_Status);
}

/****************************************************************************
 *
 * NAME: APP_ZCL_vSendHAGetZoneIDMap
 *
 * DESCRIPTION:
 *    Send out get zone ID map Command, the address mode
 *    is bound
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_ZCL_vSendHAGetZoneIDMap(void) {

    uint8 u8Seq;
    tsZCL_Address sAddress = {0};
    teZCL_Status eZCL_Status;

    sAddress.eAddressMode = E_ZCL_AM_BOUND_NO_ACK;

    eZCL_Status =  eCLD_IASACE_GetZoneIDMapSend(
            u8MyEndpoint,
            0,
            &sAddress,
            &u8Seq);

    DBG_vPrintf(TRACE_ZCL, "eZCL_Status %d\n", eZCL_Status);
}

/****************************************************************************
 *
 * NAME: APP_ZCL_vSendHAGetZoneInfo
 *
 * DESCRIPTION:
 *    Send out get zone info Command, the address mode
 *    is bound
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_ZCL_vSendHAGetZoneInfo(uint8 u8ZoneID) {

    uint8 u8Seq;
    tsZCL_Address sAddress = {0};
    teZCL_Status eZCL_Status;
    tsCLD_IASACE_GetZoneInfoPayload sPayload;

    sAddress.eAddressMode = E_ZCL_AM_BOUND_NO_ACK;

    sPayload.u8ZoneID = u8ZoneID;

    eZCL_Status =  eCLD_IASACE_GetZoneInfoSend(
            u8MyEndpoint,
            0,
            &sAddress,
            &u8Seq,
            &sPayload);

    DBG_vPrintf(TRACE_ZCL, "eZCL_Status %d\n", eZCL_Status);
}

/****************************************************************************
 *
 * NAME: APP_ZCL_vSendEmergency
 *
 * DESCRIPTION:
 *    Send out emergency Command, the address mode
 *    is bound
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_ZCL_vSendEmergency(void) {

    uint8 u8Seq;
    tsZCL_Address sAddress = {0};
    teZCL_Status eZCL_Status;

    eCommandState = E_CMD_STATE_IDLE;

    sAddress.eAddressMode = E_ZCL_AM_BOUND_NO_ACK;

    eZCL_Status =  eCLD_IASACE_EmergencySend(
            u8MyEndpoint,
            0,
            &sAddress,
            &u8Seq);

    DBG_vPrintf(TRACE_ZCL, "eZCL_Status %d\n", eZCL_Status);
}

/****************************************************************************
 *
 * NAME: APP_ZCL_vSendPanic
 *
 * DESCRIPTION:
 *    Send out panic Command, the address mode
 *    is bound
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_ZCL_vSendPanic(void) {

    uint8 u8Seq;
    tsZCL_Address sAddress = {0};
    teZCL_Status eZCL_Status;

    eCommandState = E_CMD_STATE_IDLE;

    sAddress.eAddressMode = E_ZCL_AM_BOUND_NO_ACK;

    eZCL_Status =  eCLD_IASACE_PanicSend(
            u8MyEndpoint,
            0,
            &sAddress,
            &u8Seq);

    DBG_vPrintf(TRACE_ZCL, "eZCL_Status %d\n", eZCL_Status);
}

/****************************************************************************
 *
 * NAME: APP_ZCL_vSendFire
 *
 * DESCRIPTION:
 *    Send out fire Command, the address mode
 *    is bound
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_ZCL_vSendFire(void) {

    uint8 u8Seq;
    tsZCL_Address sAddress = {0};
    teZCL_Status eZCL_Status;

    eCommandState = E_CMD_STATE_IDLE;

    sAddress.eAddressMode = E_ZCL_AM_BOUND_NO_ACK;

    eZCL_Status =  eCLD_IASACE_FireSend(
            u8MyEndpoint,
            0,
            &sAddress,
            &u8Seq);

    DBG_vPrintf(TRACE_ZCL, "eZCL_Status %d\n", eZCL_Status);
}

/****************************************************************************
 *
 * NAME: APP_ZCL_vSendHAGetPanelStatus
 *
 * DESCRIPTION:
 *    Send out Get Panel status Command, the address mode
 *    is bound
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_ZCL_vSendHAGetPanelStatus(void) {

    uint8 u8Seq;
    tsZCL_Address sAddress = {0};
    teZCL_Status eZCL_Status;

    sAddress.eAddressMode = E_ZCL_AM_BOUND_NO_ACK;
    eCommandState = E_CMD_STATE_IDLE;

    eZCL_Status =  eCLD_IASACE_GetPanelStatusSend(
            u8MyEndpoint,
            0,
            &sAddress,
            &u8Seq);

    DBG_vPrintf(TRACE_ZCL, "eZCL_Status %d\n", eZCL_Status);
}

/****************************************************************************
 *
 * NAME: APP_ZCL_vSendHAGetZoneStatus
 *
 * DESCRIPTION:
 *    Send out get zone status Command, the address mode
 *    is bound
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_ZCL_vSendHAGetZoneStatus(bool bZoneStatusMaskFlag) {

    uint8 u8Seq;
    tsZCL_Address sAddress = {0};
    teZCL_Status eZCL_Status;
    tsCLD_IASACE_GetZoneStatusPayload sPayload;

    eCommandState = E_CMD_STATE_IDLE;
    sAddress.eAddressMode = E_ZCL_AM_BOUND_NO_ACK;

    sPayload.u8StartingZoneID = 0;
    sPayload.u8MaxNumOfZoneID = 8;
    sPayload.bZoneStatusMaskFlag = bZoneStatusMaskFlag;
    if(bZoneStatusMaskFlag)
    {
        sPayload.u16ZoneStatusMask = (CLD_IASZONE_STATUS_MASK_ALARM1 | \
                                      CLD_IASZONE_STATUS_MASK_ALARM2 | \
                                      CLD_IASZONE_STATUS_MASK_TAMPER | \
                                      CLD_IASZONE_STATUS_MASK_TROUBLE | \
                                      CLD_IASZONE_STATUS_MASK_AC_MAINS);
    }else{
        sPayload.u16ZoneStatusMask = 0;
    }

    eZCL_Status =  eCLD_IASACE_GetZoneStatusSend(
            u8MyEndpoint,
            0,
            &sAddress,
            &u8Seq,
            &sPayload);

    DBG_vPrintf(TRACE_ZCL, "eZCL_Status %d\n", eZCL_Status);
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
    memcpy(sDevice.sBasicServerCluster.au8ManufacturerName, "NXP", CLD_BAS_MANUF_NAME_SIZE);
    memcpy(sDevice.sBasicServerCluster.au8ModelIdentifier, "ZHA-ACE", CLD_BAS_MODEL_ID_SIZE);
    memcpy(sDevice.sBasicServerCluster.au8DateCode, "20150112", CLD_BAS_DATE_SIZE);

    /* Initialize the Zone Type attribute */
    eZCL_Status = eCLD_IASZoneUpdateZoneType(ACE_ENDPOINT_ID,E_CLD_IASZONE_TYPE_KEYPAD);
    DBG_vPrintf(TRACE_ZCL, "eZCL_Status %d\n", eZCL_Status);
}


/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
