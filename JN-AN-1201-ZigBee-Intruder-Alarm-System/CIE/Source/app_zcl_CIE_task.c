/*****************************************************************************
 *
 * MODULE:             JN-AN-1201 ZHA Demo
 *
 * COMPONENT:          app_zcl_CIE_task.c
 *
 * DESCRIPTION:        ZHA CIE Application Behavior - Implementation
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
#include "string.h"
#include "app_zcl_CIE_task.h"
#include "zha_CIE_node.h"
#include "app_common.h"
#include "app_events.h"
#include "app_zone_client.h"

#include "app_CIE_save.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifdef DEBUG_ZCL
    #define TRACE_ZCL   TRUE
#else
    #define TRACE_ZCL   TRUE
#endif

#ifdef DEBUG_CIE_TASK
    #define TRACE_CIE_TASK  TRUE
#else
    #define TRACE_CIE_TASK FALSE
#endif

#ifdef DEBUG_PATH
    #define TRACE_PATH  TRUE
#else
    #define TRACE_PATH  FALSE
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void APP_ZCL_cbGeneralCallback(tsZCL_CallBackEvent *psEvent);
PRIVATE void APP_ZCL_cbEndpointCallback(tsZCL_CallBackEvent *psEvent);
PRIVATE void vHandleAppACEServer(tsCLD_IASACECallBackMessage *psCallBackMessage);
PRIVATE void vAPP_ZCL_DeviceSpecific_Init(void);
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
uint8 u8LastPanelStatus = E_CLD_IASACE_PANEL_STATUS_PANEL_DISARMED;
uint8 u8PanelStatusB4NotReadyToArm = E_CLD_IASACE_PANEL_STATUS_PANEL_DISARMED;
uint8 u8ConfigFlag = 0;
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PUBLIC tsHA_IASCIE sDevice;
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
    OS_eStartSWTimer(APP_TickTimer, APP_TIME_MS(1000), NULL);

    /* Register EndPoint */
    eZCL_Status = eHA_RegisterIASCIEEndPoint(1,APP_ZCL_cbEndpointCallback,&sDevice);//给端点1注册回调函数

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
 * ZCL Task for the Light
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
        vZCL_EventHandler(&sCallBackEvent);  //处理ZCL的事情
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
    tsZCL_CallBackEvent sCallBackEvent;

    OS_eContinueSWTimer(APP_TickTimer, APP_TIME_MS(1000), NULL);
    sCallBackEvent.pZPSevent = NULL;
    sCallBackEvent.eEventType = E_ZCL_CBET_TIMER;
    vZCL_EventHandler(&sCallBackEvent);
    vPermitJoinIndication();
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
 *处理其他端点的ZCL事项，基本用不上
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
 *处理端点1的关于ZCL的所有事项，
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

        //收到上报attribute，对每一个attribute产生一个此消息，
    case E_ZCL_CBET_REPORT_INDIVIDUAL_ATTRIBUTE:
    {
               uint16    u16SizeOfAttribute = 0;
               uint8     u16Elements =  0;
               uint16    i =  0;
               DBG_vPrintf(TRACE_ZCL,"E_ZCL_CBET_REPORT_INDIVIDUAL_ATTRIBUTE shoudao");
               //属性的数据格式
               switch ( psEvent->uMessage.sIndividualAttributeResponse.eAttributeDataType )
               {
               case(E_ZCL_OSTRING):
               case(E_ZCL_CSTRING):
                  if ( psEvent->uMessage.sIndividualAttributeResponse.pvAttributeData != NULL )
                  {
                      u16Elements =  ( (uint8*)psEvent->uMessage.sIndividualAttributeResponse.pvAttributeData ) [ 0 ];
                  }
                  else
                  {
                      u16Elements   =  0 ;
                  }
               break;
               case(E_ZCL_LOSTRING):
               case(E_ZCL_LCSTRING):
                   if ( psEvent->uMessage.sIndividualAttributeResponse.pvAttributeData != NULL )
                   {
                       u16Elements =  ( (uint16*)psEvent->uMessage.sIndividualAttributeResponse.pvAttributeData ) [ 0 ];
                   }
                   else
                   {
                       u16Elements   =  0 ;
                   }
               break;
               default:
                   u16Elements   =  1;
                   break;
               }



               /* Send event upwards */


               if((psEvent->eEventType == E_ZCL_CBET_READ_INDIVIDUAL_ATTRIBUTE_RESPONSE))
               {

               }
               else if((psEvent->eEventType == E_ZCL_CBET_REPORT_INDIVIDUAL_ATTRIBUTE))
               {

               }
               else if((psEvent->eEventType == E_ZCL_CBET_WRITE_ATTRIBUTES_RESPONSE))
               {

               }

           }
           break;
           //收到已经对单个节点的所有attributes解析完成，生成此消息
    case   E_ZCL_CBET_REPORT_ATTRIBUTES:
    	 DBG_vPrintf(TRACE_ZCL,"E_ZCL_CBET_REPORT_INDIVIDUAL_ATTRIBUTE");
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
        }
        break;

    case E_ZCL_CBET_READ_INDIVIDUAL_ATTRIBUTE_RESPONSE:
        DBG_vPrintf(TRACE_ZCL,"Individual Attribute Read Response for Cluster = %d\n",psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum);
        if(psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum == SECURITY_AND_SAFETY_CLUSTER_ID_IASZONE)
        {
            DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Address = %04x", psEvent->pZPSevent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr);
            DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Attr = %04x", psEvent->uMessage.sIndividualAttributeResponse.u16AttributeEnum);

            vAppUpdateZoneTable(psEvent);
            PDM_eSaveRecordData( PDM_ID_APP_IASCIE_STRUCT,
                                 &sDiscovedZoneServers[0],
                                 sizeof(tsDiscovedZoneServers) * MAX_ZONE_SERVER_NODES);
        }
        break;

    case E_ZCL_CBET_READ_ATTRIBUTES_RESPONSE:
        /* DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Read attributes response"); */
        break;

    case E_ZCL_CBET_WRITE_ATTRIBUTES_RESPONSE:
        vReadAttributes(psEvent);
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

        //处理某个Cluster的特定命令，如ON/OFF cluster 中的ON命令，OFF命令
    case E_ZCL_CBET_CLUSTER_CUSTOM:
        DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Custom Cl %04x\n", psEvent->uMessage.sClusterCustomMessage.u16ClusterId);

        switch(psEvent->uMessage.sClusterCustomMessage.u16ClusterId)
        {
            case SECURITY_AND_SAFETY_CLUSTER_ID_IASZONE:
            {
                tsCLD_IASZoneCallBackMessage *psCallBackMessage = (tsCLD_IASZoneCallBackMessage *)psEvent->uMessage.sClusterCustomMessage.pvCustomData;
                uint16 u16ShortAddress = psEvent->pZPSevent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr;
                vHandleAppZoneClient(psCallBackMessage,u16ShortAddress);

            }
            break;

            case SECURITY_AND_SAFETY_CLUSTER_ID_IASACE:
            {
                uint8 Count = 0;
                bool bIsEnrolledACE = TRUE;
                tsCLD_IASACECallBackMessage *psCallBackMessage = (tsCLD_IASACECallBackMessage *)psEvent->uMessage.sClusterCustomMessage.pvCustomData;
                /* Only take request from an enrolled ACE device */
                Count = u8GetTableIndex(psEvent->pZPSevent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr);
                /* Check has device ever joined and enrolled */
                if((Count == 0xFF) ||
                        (sDiscovedZoneServers[Count].u8ZoneId == 0xFF))
                {
                    bIsEnrolledACE = FALSE;
                }
                /* For bypass if asked from un-enrolled device do not take request */
                if(psCallBackMessage->u8CommandId == E_CLD_IASACE_CMD_BYPASS)
                {
                    for(Count=0; Count < u8Discovered; Count++)
                    {
                        if((sDiscovedZoneServers[Count].bValid == TRUE) &&
                                (sDiscovedZoneServers[Count].u8ZoneId != 0xFF))
                        {
                            if(bIsEnrolledACE)
                                ZCL_BIT_CLEAR(uint8,sDiscovedZoneServers[Count].u8Config,CLD_IASACE_ZONE_CONFIG_FLAG_NOT_BYPASSED);
                            else
                                ZCL_BIT_SET(uint8,sDiscovedZoneServers[Count].u8Config,CLD_IASACE_ZONE_CONFIG_FLAG_NOT_BYPASSED);
                            eCLD_IASACESetZoneParameterValue (
                                        CIE_EP,
                                        E_CLD_IASACE_ZONE_PARAMETER_ZONE_CONFIG_FLAG,
                                        sDiscovedZoneServers[Count].u8ZoneId,
                                        sDiscovedZoneServers[Count].u8Config);
                        }
                    }
                }
                if(bIsEnrolledACE)
                {
                    vHandleAppACEServer(psCallBackMessage);
                }
            }
            break;

            case GENERAL_CLUSTER_ID_GROUPS:
            break;
            case GENERAL_CLUSTER_ID_IDENTIFY:
            break;
        }
        break;

    case E_ZCL_CBET_WRITE_INDIVIDUAL_ATTRIBUTE:
        DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Write Individual Attribute");
        break;

    case E_ZCL_CBET_CLUSTER_UPDATE:
        if (psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum == SECURITY_AND_SAFETY_CLUSTER_ID_IASACE)
        {
            /* Update local structure to save in PDM for any changes in zone parameter*/
            vDisplayPanel();

           /* PDM_eSaveRecordData( PDM_ID_APP_IASACE_ZONE_PARAM,
                                 (tsCLD_IASACE_ZoneParameter *)&sDevice.sIASACEServerCustomDataStructure.asCLD_IASACE_ZoneParameter[0],
                                 sizeof(tsCLD_IASACE_ZoneParameter) * CLD_IASACE_ZONE_TABLE_SIZE);*/
            PDM_eSaveRecordData( PDM_ID_APP_IASCIE_STRUCT,
                                 &sDiscovedZoneServers[0],
                                 sizeof(tsDiscovedZoneServers) * MAX_ZONE_SERVER_NODES);
        }
        DBG_vPrintf(TRACE_CIE_TASK, "Update Id %04x\n", psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum);
        break;

    default:
        DBG_vPrintf(TRACE_ZCL, "\nEP EVT: Invalid evt type 0x%x", (uint8)psEvent->eEventType);
        break;
    }

}

/****************************************************************************
 *
 * NAME: vHandleAppACEServer
 *
 * DESCRIPTION:
 * Handles all the ACE sever received commands
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
void vHandleAppACEServer(tsCLD_IASACECallBackMessage *psCallBackMessage)
{
    uint8 u8Alarmstatus=0,u8LastAlarmstatus = 0;
    uint8 u8PanelStatus=0;
    uint8 u8WarningMode=0,u8LastAudibleNotification = 0;
    uint8 u8StrobeDutyCycle=0;


    eCLD_IASACEGetPanelParameter (
        CIE_EP,                                             /*uint8                                       u8SourceEndPointId, */
        E_CLD_IASACE_PANEL_PARAMETER_PANEL_STATUS,          /*teCLD_IASACE_PanelParameterID               eParameterId, */
        &u8PanelStatus);

    eCLD_IASACEGetPanelParameter (
        CIE_EP,                                              /*uint8                                       u8SourceEndPointId, */
        E_CLD_IASACE_PANEL_PARAMETER_ALARM_STATUS,           /*teCLD_IASACE_PanelParameterID               eParameterId,*/
        &u8LastAlarmstatus);

    eCLD_IASACEGetPanelParameter (
        CIE_EP,                                              /*uint8                                       u8SourceEndPointId, */
        E_CLD_IASACE_PANEL_PARAMETER_AUDIBLE_NOTIFICATION,   /*teCLD_IASACE_PanelParameterID               eParameterId, */
        &u8LastAudibleNotification);

    switch (psCallBackMessage->u8CommandId)
    {
    case E_CLD_IASACE_CMD_ARM:
    {
        tsCLD_IASACE_ArmPayload  *psArmPayload=psCallBackMessage->uMessage.psArmPayload;
        if (psArmPayload->eArmMode == E_CLD_IASACE_ARM_MODE_DISARM)
        {
            if(u8PanelStatus != E_CLD_IASACE_PANEL_STATUS_PANEL_DISARMED){
                /* Make sure to stop exit/entry delay timer & accept the request */
                eCLD_IASACESetPanelParameter (
                        CIE_EP,
                        E_CLD_IASACE_PANEL_PARAMETER_SECONDS_REMAINING,
                        0);

                OS_eStopSWTimer(APP_EntryExitDelayTmr);

                /* if disarming the system stop all the warnings & change ePanelStatus to E_CLD_IASACE_PANEL_STATUS_PANEL_DISARMED
                 *  for ACE server Panel parameter */
                bStayInExitDelay = FALSE;
                vStartWarning(DISABLE_WARNING,DISABLE_WARNING,DISABLE_WARNING,DISABLE_WARNING);
                vSetPanelParamter(E_CLD_IASACE_PANEL_STATUS_PANEL_DISARMED,E_CLD_IASACE_ALARM_STATUS_NO_ALARM,E_CLD_IASACE_AUDIBLE_NOTIF_MUTE);
                vStartSquawk(SQUAWK_MODE_STROBE_AND_LEVEL_DISARMED);
            }
        }
        else
        {
            /* Move to arm state only if you are disarmed && no alarm on any zones*/
            if(u8PanelStatus == E_CLD_IASACE_PANEL_STATUS_PANEL_DISARMED)
            {
                switch (psArmPayload->eArmMode)
                {
                case E_CLD_IASACE_ARM_MODE_ARM_DAY_HOME_ZONES_ONLY:
                    u8PanelStatus = E_CLD_IASACE_PANEL_STATUS_PANEL_ARMING_STAY;
                    u8ConfigFlag = CLD_IASACE_ZONE_CONFIG_FLAG_DAY_HOME;
                    break;
                case E_CLD_IASACE_ARM_MODE_ARM_NIGHT_SLEEP_ZONES_ONLY:
                    u8PanelStatus = E_CLD_IASACE_PANEL_STATUS_PANEL_ARMING_NIGHT;
                    u8ConfigFlag = CLD_IASACE_ZONE_CONFIG_FLAG_NIGHT_SLEEP;
                    break;
                case E_CLD_IASACE_ARM_MODE_ARM_ALL_ZONES:
                    u8ConfigFlag = 0xFF;
                    u8PanelStatus = E_CLD_IASACE_PANEL_STATUS_PANEL_ARMING_AWAY;
                    break;
                default:
                    break;
                }
                if(bCheckNotReadyToArm(u8ConfigFlag))
                {
                    u8PanelStatusB4NotReadyToArm = E_CLD_IASACE_PANEL_STATUS_PANEL_DISARMED;
                    DBG_vPrintf(TRACE_CIE_TASK,"\nChanging state to not ready when check zones\n");
                    vSetPanelParamter(E_CLD_IASACE_PANEL_STATUS_PANEL_NOT_READY_TO_ARM,u8LastAlarmstatus,u8LastAudibleNotification);
                }
                else
                {
                    vSetPanelParamter(u8PanelStatus,E_CLD_IASACE_ALARM_STATUS_NO_ALARM,E_CLD_IASACE_AUDIBLE_NOTIF_MUTE);
                    u8LastPanelStatus = u8PanelStatus;
                    vStartArmingSystem();
                }
            }else
            {
                /* Change status to not ready to arm to send arm response back and not allowing ZCL to take action
                 * and then revert back to last panel state
                 */
                DBG_vPrintf(TRACE_CIE_TASK,"\nChanging state to not ready\n");
                u8PanelStatusB4NotReadyToArm = u8PanelStatus;
                vSetPanelParamter(E_CLD_IASACE_PANEL_STATUS_PANEL_NOT_READY_TO_ARM,u8LastAlarmstatus,u8LastAudibleNotification);
            }
        }
        break;
    }
    case E_CLD_IASACE_CMD_BYPASS:
    {
        tsCLD_IASACE_BypassPayload *psBypassPayload = psCallBackMessage->uMessage.psBypassPayload;
        /* Call Ace display for filling up the ACE string*/

        break;
    }
    case E_CLD_IASACE_CMD_EMERGENCY:
        u8Alarmstatus = E_CLD_IASACE_ALARM_STATUS_EMERGENCY;
        u8WarningMode  = WARNING_MODE_STROBE_AND_SIREN_LEVEL_EMERGENCY;
        u8StrobeDutyCycle = 50;
        break;
    case E_CLD_IASACE_CMD_FIRE:
        u8Alarmstatus = E_CLD_IASACE_ALARM_STATUS_FIRE;
        u8WarningMode  = WARNING_MODE_STROBE_AND_SIREN_LEVEL_FIRE;
        u8StrobeDutyCycle = 20;
        break;
    case E_CLD_IASACE_CMD_PANIC:
        u8Alarmstatus = E_CLD_IASACE_ALARM_STATUS_POLICE_PANIC;
        u8WarningMode  = WARNING_MODE_STROBE_AND_SIREN_LEVEL_BURGLAR;
        u8StrobeDutyCycle = 50;
        break;
    case E_CLD_IASACE_CMD_GET_ZONE_ID_MAP:
        break;
    case E_CLD_IASACE_CMD_GET_ZONE_INFO:
    {
        tsCLD_IASACE_GetZoneInfoPayload  *psGetZoneInfoPayload=psCallBackMessage->uMessage.psGetZoneInfoPayload;
        /* Call Ace display for filling up the ACE string*/

        break;
    }
    case E_CLD_IASACE_CMD_GET_PANEL_STATUS:
    {

        break;
    }
    case E_CLD_IASACE_CMD_GET_BYPASSED_ZONE_LIST:
    {

    }
    break;
    case E_CLD_IASACE_CMD_GET_ZONE_STATUS:
    {
        tsCLD_IASACE_GetZoneStatusPayload  *psGetZoneStatusPayload = psCallBackMessage->uMessage.psGetZoneStatusPayload;
        /* Call Ace display for filling up the ACE string*/

        /* If panel status was Not ready to arm , go back to original state */
        if(u8PanelStatus == E_CLD_IASACE_PANEL_STATUS_PANEL_NOT_READY_TO_ARM)
        {
            DBG_vPrintf(TRACE_CIE_TASK,"\nChanging state back to %d\n",u8PanelStatusB4NotReadyToArm);
            vSetPanelParamter(u8PanelStatusB4NotReadyToArm,u8LastAlarmstatus,u8LastAudibleNotification);
        }
        break;
    }

    default :
        break;

    }

    if(u8Alarmstatus)
    {
        /* If the Emergency, fire , and panic is received set the ePanelStatus to E_CLD_IASACE_PANEL_STATUS_PANEL_IN_ALARM
         * and issue warning to the warning device
         */
        eCLD_IASACESetPanelParameter (
                CIE_EP,                                        /*uint8                                       u8SourceEndPointId,*/
                E_CLD_IASACE_PANEL_PARAMETER_SECONDS_REMAINING,/*teCLD_IASACE_PanelParameterID               eParameterId,*/
                0);
        vSetPanelParamter(E_CLD_IASACE_PANEL_STATUS_PANEL_IN_ALARM,u8Alarmstatus,E_CLD_IASACE_AUDIBLE_NOTIF_DEFAULT_SOUND);
        /*Alarm condition for the Zone-Start WD*/
        vStartWarning(STROBE_LEVEL,ALARM_WARNING_DURATION,u8StrobeDutyCycle,u8WarningMode);
    }

    /* Update local structure and refresh display */

}

/****************************************************************************
 *
 * NAME: APP_EntryExitDelay
 *
 * DESCRIPTION:
 * Task kicked by Exit/Entry Delay timer
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_EntryExitDelay)
{
    uint8 u8SecondsRemaining = 0,u8PanelStatus = 0;

    /* If stuck in exit delay cause of some alarm on some zone, stay at exit delay */
    eCLD_IASACEGetPanelParameter(CIE_EP,E_CLD_IASACE_PANEL_PARAMETER_SECONDS_REMAINING,&u8SecondsRemaining);
    /* Find the panel status to check to keep sending warning only for exit delay*/
    eCLD_IASACEGetPanelParameter(CIE_EP,E_CLD_IASACE_PANEL_PARAMETER_PANEL_STATUS,&u8PanelStatus);

    if(bStayInExitDelay == FALSE)
    {
        /* decrement the seconds remaining every second to update */
        if(u8SecondsRemaining > 0)
        {
            u8SecondsRemaining--;
            eCLD_IASACESetPanelParameter (
                    CIE_EP,
                    E_CLD_IASACE_PANEL_PARAMETER_SECONDS_REMAINING,
                    u8SecondsRemaining);
           /* PDM_eSaveRecordData( PDM_ID_APP_IASACE_PANEL_PARAM,
                                 (tsCLD_IASACE_PanelParameter *)&sDevice.sIASACEServerCustomDataStructure.sCLD_IASACE_PanelParameter,
                                 sizeof(tsCLD_IASACE_PanelParameter));*/
        }
        OS_eContinueSWTimer(APP_EntryExitDelayTmr, APP_TIME_MS(1000), NULL );
    }else if((u8SecondsRemaining > 0) &&
            (u8PanelStatus == E_CLD_IASACE_PANEL_STATUS_PANEL_EXIT_DELAY))
    {
        /* Keep raising the alarm to WD for every one sec so that user can see the zone under alarm */
        vStartWarning(STROBE_LEVEL,1,50/*STROBE_DUTY_CYCLE*/,WARNING_MODE_STROBE_AND_SIREN_LEVEL_ENTRY_EXIT_DELAY);
        OS_eContinueSWTimer(APP_EntryExitDelayTmr, APP_TIME_MS(1000), NULL );
    }

    /* If in exit/entry delay timer has expired move to appropriate state
     *  with appropriate actions taken
     */
    if(u8SecondsRemaining == 0)
    {
        OS_eStopSWTimer(APP_EntryExitDelayTmr);

        /* If in exit delay timer has expired move ePanelStatus from arming to armed
         */
        if(u8PanelStatus == E_CLD_IASACE_PANEL_STATUS_PANEL_EXIT_DELAY)
        {
            switch(u8LastPanelStatus)
            {
                case E_CLD_IASACE_PANEL_STATUS_PANEL_ARMING_STAY:
                    u8PanelStatus = E_CLD_IASACE_PANEL_STATUS_PANEL_ARMED_DAY;
		            vStartSquawk(SQUAWK_MODE_STROBE_AND_LEVEL_ARMED);
                break;
                case E_CLD_IASACE_PANEL_STATUS_PANEL_ARMING_NIGHT:
                    u8PanelStatus = E_CLD_IASACE_PANEL_STATUS_PANEL_ARMED_NIGHT;
		            vStartSquawk(SQUAWK_MODE_STROBE_AND_LEVEL_ARMED);
                break;
                case E_CLD_IASACE_PANEL_STATUS_PANEL_ARMING_AWAY:
                    u8PanelStatus = E_CLD_IASACE_PANEL_STATUS_PANEL_ARMED_AWAY;
		            vStartSquawk(SQUAWK_MODE_STROBE_AND_LEVEL_ARMED);
                break;
                default:
                    break;
            }
        }else if(u8PanelStatus == E_CLD_IASACE_PANEL_STATUS_PANEL_ENTRY_DELAY)
        {
            /* If in exit delay timer has expired move ePanelStatus to E_CLD_IASACE_PANEL_STATUS_PANEL_IN_ALARM
             * and start issue to warning device
             */
            u8PanelStatus = E_CLD_IASACE_PANEL_STATUS_PANEL_IN_ALARM;
            vStartWarning(STROBE_LEVEL,ALARM_WARNING_DURATION,STROBE_DUTY_CYCLE,WARNING_MODE_STROBE_AND_SIREN_LEVEL_BURGLAR);
        }

        eCLD_IASACESetPanelParameter (
                CIE_EP,                                   /*uint8                                       u8SourceEndPointId,*/
                E_CLD_IASACE_PANEL_PARAMETER_PANEL_STATUS,/*teCLD_IASACE_PanelParameterID               eParameterId,*/
                u8PanelStatus);


       /* PDM_eSaveRecordData( PDM_ID_APP_IASACE_PANEL_PARAM,
                             (tsCLD_IASACE_PanelParameter *)&sDevice.sIASACEServerCustomDataStructure.sCLD_IASACE_PanelParameter,
                             sizeof(tsCLD_IASACE_PanelParameter));*/
    }

}

/****************************************************************************
 *
 * NAME: bCheckNotReadyToArm
 *
 * DESCRIPTION:
 * Check whether system is not ready to arm
 *
 * RETURNS:
 * bool
 *
 ****************************************************************************/
PUBLIC bool bCheckNotReadyToArm(uint8 u8ConfigFlag)
{
    int Count;
     /* Check for any pending warnings before arming a system,
      * if any of zone is in alarm change the ePanelStatus to E_CLD_IASACE_PANEL_STATUS_PANEL_NOT_READY_TO_ARM
      *  for ACE server Panel parameter */
     for(Count=0; Count < u8Discovered; Count++)
     {
         if(sDiscovedZoneServers[Count].bValid == TRUE)
         {
             if(sDiscovedZoneServers[Count].u16ZoneStatus != 0 &&
                     (sDiscovedZoneServers[Count].u8Config & u8ConfigFlag || u8ConfigFlag == 0xFF) &&
                     (!(sDiscovedZoneServers[Count].u8ArmBypass & CLD_IASACE_ZONE_STATUS_FLAG_BYPASS))){
                 return TRUE;
             }
         }
     }
     return FALSE;
}

/****************************************************************************
 *
 * NAME: vStartArmingSystem
 *
 * DESCRIPTION:
 * Starts to arm the sytem
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vStartArmingSystem(void)
{
    /* Enter into the exit delay mode before arming the system
     * by changing the ePanelStatus to E_CLD_IASACE_PANEL_STATUS_PANEL_EXIT_DELAY
     *  for ACE server Panel parameter */
    eCLD_IASACESetPanelParameter (
            CIE_EP,
            E_CLD_IASACE_PANEL_PARAMETER_SECONDS_REMAINING,
            CLD_IASACE_PANEL_PARAMTER_SECONDS_REMAINING);
    vSetPanelParamter(E_CLD_IASACE_PANEL_STATUS_PANEL_EXIT_DELAY,E_CLD_IASACE_ALARM_STATUS_NO_ALARM,E_CLD_IASACE_AUDIBLE_NOTIF_DEFAULT_SOUND);
    /*Resoultion of this timer goes only till 1 mins */
    OS_eStopSWTimer(APP_EntryExitDelayTmr);
    OS_eStartSWTimer(APP_EntryExitDelayTmr, APP_TIME_MS(1000), NULL );
    vStartWarning(STROBE_LEVEL,ENTRY_EXIT_DELAY_WARNING_DURATION,50/*STROBE_DUTY_CYCLE*/,WARNING_MODE_STROBE_AND_SIREN_LEVEL_ENTRY_EXIT_DELAY);
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
    memcpy(sDevice.sBasicServerCluster.au8ModelIdentifier, "ZHA-CIE", CLD_BAS_MODEL_ID_SIZE);
    memcpy(sDevice.sBasicServerCluster.au8DateCode, "20150112", CLD_BAS_DATE_SIZE);
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
