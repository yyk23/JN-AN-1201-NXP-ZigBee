/*****************************************************************************
 *
 * MODULE:             JN-AN-1201 ZHA Demo
 *
 * COMPONENT:          app_zcl_WD_task.c
 *
 * DESCRIPTION:        ZHA Light Application Behavior - Implementation
 *
 ****************************************************************************
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
 * Copyright NXP B.V. 2012. All rights reserved
 *
 ***************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <jendefs.h>
#include <string.h>
#include "os.h"
#include "os_gen.h"
#include "pdum_gen.h"
#include "pdm.h"
#include "dbg.h"
#include "zps_gen.h"
#include "PDM_IDs.h"
#include "zcl_options.h"
#include "zps_apl_af.h"
#include "ha.h"

#include "app_zcl_WD_task.h"
#include "zha_WD_node.h"
#include "app_common.h"
#include "app_events.h"
#include "app_ias_indicator.h"
#include "app_ias_unenroll_req.h"
#include "app_ias_save.h"

#include "IASWD.h"
#include "IASZONE.h"

#include "DriverPiezo.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifdef DEBUG_ZCL
    #define TRACE_ZCL   TRUE
#else
    #define TRACE_ZCL   FALSE
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
tsHA_IASWarningDevice sDevice;
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

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
        DBG_vPrintf(TRACE_ZCL, "\nErr: eHA_Initialise:%d", eZCL_Status);
    }

    /* Start the tick timer */
    OS_eStartSWTimer(APP_TickTimer, TEN_HZ_TICK_TIME, NULL);

    /* Register EndPoint */
    eZCL_Status = eHA_RegisterIASWarningDeviceEndPoint(WD_WD_ENDPOINT, APP_ZCL_cbEndpointCallback,&sDevice);

    if (eZCL_Status != E_ZCL_SUCCESS)
    {
            DBG_vPrintf(TRACE_ZCL, "Error: eApp_HA_RegisterEndpoint:%d\r\n", eZCL_Status);
    }

    vAPP_ZCL_DeviceSpecific_Init();
}
/****************************************************************************
 *
 * NAME: ZCL_Task
 *
 * DESCRIPTION:
 * ZCL Task for the Warning Device
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

    /* If there is a stack event to process, pass it on to ZCL */
    sStackEvent.eType = ZPS_EVENT_NONE;
    if ( OS_eCollectMessage(APP_msgZpsEvents_ZCL, &sStackEvent) == OS_E_OK)
    {
        DBG_vPrintf(TRACE_ZCL, "\nZCL_Task event:%d",sStackEvent.eType);
        sCallBackEvent.eEventType = E_ZCL_CBET_ZIGBEE_EVENT;
        vZCL_EventHandler(&sCallBackEvent);
    }

}

/****************************************************************************
 *
 * NAME: Tick_Task
 *
 * DESCRIPTION:
 * Task kicked by the tick timer
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(Tick_Task)
{
    static uint32 u32Tick10ms = 9;
    static uint32 u32Tick1Sec = 99;

    tsZCL_CallBackEvent sCallBackEvent;

    OS_eContinueSWTimer(APP_TickTimer, APP_TIME_MS(10), NULL);

    u32Tick10ms++;
    u32Tick1Sec++;

    /* Wrap the Tick10ms counter and provide 100ms ticks to cluster */
    if (u32Tick10ms > 9)
    {
        eHA_Update100mS();
        u32Tick10ms = 0;
    }

    /* Wrap the Tick counter and provide 1Hz ticks to cluster */
    if(u32Tick1Sec > 99)
    {
        u32Tick1Sec = 0;
        sCallBackEvent.pZPSevent = NULL;
        sCallBackEvent.eEventType = E_ZCL_CBET_TIMER;
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
        DBG_vPrintf(TRACE_ZCL, "\nEVT: Unhandled Event\r\n");
        break;

    case E_ZCL_CBET_READ_ATTRIBUTES_RESPONSE:
        DBG_vPrintf(TRACE_ZCL, "\nEVT: Read attributes response");
        break;

    case E_ZCL_CBET_READ_REQUEST:
        DBG_vPrintf(TRACE_ZCL, "\nEVT: Read request");
        break;

    case E_ZCL_CBET_DEFAULT_RESPONSE:
        DBG_vPrintf(TRACE_ZCL, "\nEVT: Default response");
        break;

    case E_ZCL_CBET_ERROR:
        DBG_vPrintf(TRACE_ZCL, "\nEVT: Error");
        break;

    case E_ZCL_CBET_TIMER:
        DBG_vPrintf(TRACE_ZCL, "\nEVT: Timer");
        break;

    case E_ZCL_CBET_ZIGBEE_EVENT:
        DBG_vPrintf(TRACE_ZCL, "\nEVT: ZigBee");
        break;

    case E_ZCL_CBET_CLUSTER_CUSTOM:
        DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Custom");
        break;

    default:
        DBG_vPrintf(TRACE_ZCL, "\nInvalid event type");
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

    DBG_vPrintf(TRACE_ZCL, "\nEntering cbZCL_EndpointCallback");

    switch (psEvent->eEventType)
    {

    case E_ZCL_CBET_LOCK_MUTEX:
        break;

    case E_ZCL_CBET_UNLOCK_MUTEX:
        break;

    case E_ZCL_CBET_UNHANDLED_EVENT:
        /* DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Unhandled event");*/
        break;
    case E_ZCL_CBET_REPORT_INDIVIDUAL_ATTRIBUTES_CONFIGURE:
        {
            tsZCL_AttributeReportingConfigurationRecord    *psAttributeReportingRecord= &psEvent->uMessage.sAttributeReportingConfigurationRecord;
            DBG_vPrintf(TRACE_ZCL,"Individual Configure attribute for Cluster = %d\n",psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum);
            DBG_vPrintf(TRACE_ZCL,"eAttributeDataType = %d\n",psAttributeReportingRecord->eAttributeDataType);
            DBG_vPrintf(TRACE_ZCL,"u16AttributeEnum = %d\n",psAttributeReportingRecord->u16AttributeEnum );
            DBG_vPrintf(TRACE_ZCL,"u16MaximumReportingInterval = %d\n",psAttributeReportingRecord->u16MaximumReportingInterval );
            DBG_vPrintf(TRACE_ZCL,"u16MinimumReportingInterval = %d\n",psAttributeReportingRecord->u16MinimumReportingInterval );
            DBG_vPrintf(TRACE_ZCL,"u16TimeoutPeriodField = %d\n",psAttributeReportingRecord->u16TimeoutPeriodField );
            DBG_vPrintf(TRACE_ZCL,"u8DirectionIsReceived = %d\n",psAttributeReportingRecord->u8DirectionIsReceived );
            DBG_vPrintf(TRACE_ZCL,"uAttributeReportableChange = %d\n",psAttributeReportingRecord->uAttributeReportableChange );
            break;
        }
    case E_ZCL_CBET_READ_INDIVIDUAL_ATTRIBUTE_RESPONSE:
        /*DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Rd Attr %04x RS %d AS %d", psEvent->uMessage.sIndividualAttributeResponse.u16AttributeEnum, psEvent->uMessage.sIndividualAttributeResponse.psAttributeStatus->eRequestStatus, psEvent->uMessage.sIndividualAttributeResponse.psAttributeStatus->eAttributeStatus); */
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
                if(   (uint64)(*((uint64*)(psIndividualAttributeResponse->pvAttributeData))) == 0      )
                {
                    /*Handled in Range check if there is 0 written to the attribute*/

                }
                else
                {
                    /*Make Binding for the Client as all the communication will go to the client*/
                    ZPS_teStatus eStatus=ZPS_eAplZdoBind(
                                            psEvent->pZPSevent->uEvent.sApsDataIndEvent.u16ClusterId,              /*uint16 u16ClusterId,*/
                                            psEvent->pZPSevent->uEvent.sApsDataIndEvent.u8DstEndpoint,             /*uint8 u8SrcEndpoint,*/
                                            psEvent->pZPSevent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr,       /*uint16 u16DstAddr,*/
                                            (uint64)(*((uint64*)(psIndividualAttributeResponse->pvAttributeData))),/*uint64 u64DstIeeeAddr,*/
                                            psEvent->pZPSevent->uEvent.sApsDataIndEvent.u8SrcEndpoint);            /*uint8 u8DstEndpoint);*/
                    vSetIASDeviceState(E_IAS_DEV_STATE_READY_TO_ENROLL);
                    vSaveIASZoneAttributes(psEvent->pZPSevent->uEvent.sApsDataIndEvent.u8DstEndpoint);
                    DBG_vPrintf(TRACE_ZCL,"Binding status =%d",eStatus);
                }
            }
        }
        break;
    case E_ZCL_CBET_READ_ATTRIBUTES_RESPONSE:
        /* DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Read attributes response"); */
        break;

    case E_ZCL_CBET_READ_REQUEST:
        /* DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Read request"); */
        break;

    case E_ZCL_CBET_DEFAULT_RESPONSE:
        /* DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Default response"); */
        break;

    case E_ZCL_CBET_ERROR:
        /* DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Error"); */
        break;

    case E_ZCL_CBET_TIMER:
        /* DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Timer"); */
        break;

    case E_ZCL_CBET_ZIGBEE_EVENT:
        /* DBG_vPrintf(TRACE_ZCL, "\nEP EVT: ZigBee"); */
        break;

    case E_ZCL_CBET_CLUSTER_CUSTOM:
        DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Custom Cl %04x\n", psEvent->uMessage.sClusterCustomMessage.u16ClusterId);

        switch(psEvent->uMessage.sClusterCustomMessage.u16ClusterId)
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
                            /*Save State*/
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

            case SECURITY_AND_SAFETY_CLUSTER_ID_IASWD:
            {
                tsCLD_IASWDCallBackMessage *psCallBackMessage = (tsCLD_IASWDCallBackMessage *)psEvent->uMessage.sClusterCustomMessage.pvCustomData;

                DBG_vPrintf(TRACE_ZCL,"\nIAS WD Command Id = %d\n",psCallBackMessage->u8CommandId);

                switch (psCallBackMessage->u8CommandId)
                {
                    case E_CLD_IASWD_CMD_START_WARNING:
                    {
                        uint8 u8Mode;
                        bool_t u8Strobe;
                        tsCLD_IASWD_StartWarningReqPayload *psWDStartWarningReqPayload = (tsCLD_IASWD_StartWarningReqPayload *)psCallBackMessage->uMessage.psWDStartWarningReqPayload;
                        DBG_vPrintf(TRACE_ZCL, "Warning..\n ");
                        DBG_vPrintf(TRACE_ZCL, "u8WarningModeStrobeAndSirenLevel %d\n",psWDStartWarningReqPayload->u8WarningModeStrobeAndSirenLevel);
                        DBG_vPrintf(TRACE_ZCL, "u16WarningDuration %d\n",psWDStartWarningReqPayload->u16WarningDuration);
                        DBG_vPrintf(TRACE_ZCL, "u8StrobeDutyCycle %d\n",psWDStartWarningReqPayload->u8StrobeDutyCycle);
                        DBG_vPrintf(TRACE_ZCL, "eStrobeLevel %d\n\n",psWDStartWarningReqPayload->eStrobeLevel);

                        u8Mode = psWDStartWarningReqPayload->u8WarningModeStrobeAndSirenLevel >> 4;
                        u8Strobe = (psWDStartWarningReqPayload->u8WarningModeStrobeAndSirenLevel & 0xF0) >> 2;

                        vWarning(
                                u8Mode,
                                psWDStartWarningReqPayload->u16WarningDuration*1000,
                                u8Strobe,
                                psWDStartWarningReqPayload->eStrobeLevel,
                                psWDStartWarningReqPayload->u8StrobeDutyCycle);
                    }
                    break;

                    case E_CLD_IASWD_CMD_SQUAWK:
                    {
                        uint8 u8Mode;
                        uint8 u8Strobe;
                        uint8 u8SquawkLevel;
                        tsCLD_IASWD_SquawkReqPayload *psWDSquawkReqPayload = (tsCLD_IASWD_SquawkReqPayload *)psCallBackMessage->uMessage.psWDSquawkReqPayload;
                        DBG_vPrintf(TRACE_ZCL, "Squawk..\n ");
                        DBG_vPrintf(TRACE_ZCL, "u8WarningModeStrobeAndSirenLevel %d\n",psWDSquawkReqPayload->u8SquawkModeStrobeAndLevel);

                        u8Mode        = (psWDSquawkReqPayload->u8SquawkModeStrobeAndLevel >> 4) & 0x0F;
                        u8Strobe      = (psWDSquawkReqPayload->u8SquawkModeStrobeAndLevel >> 3) & 0x01;
                        u8SquawkLevel = (psWDSquawkReqPayload->u8SquawkModeStrobeAndLevel     ) & 0x03;

                        vSquawk(u8Mode,
                                u8Strobe,
                                u8SquawkLevel);
                    }
                    break;

                }
            }
            break;
        }
        break;

        default:
            DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Invalid evt type 0x%x", (uint8)psEvent->eEventType);
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
    /* Initialize the strings in Basic */
    memcpy(sDevice.sBasicServerCluster.au8ManufacturerName, "NXP", CLD_BAS_MANUF_NAME_SIZE);
    memcpy(sDevice.sBasicServerCluster.au8ModelIdentifier, "ZHA-WD", CLD_BAS_MODEL_ID_SIZE);
    memcpy(sDevice.sBasicServerCluster.au8DateCode, "20150112", CLD_BAS_DATE_SIZE);
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
