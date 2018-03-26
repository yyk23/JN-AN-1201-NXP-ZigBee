/*****************************************************************************
 *
 * MODULE:             JN-AN-1201
 *
 * COMPONENT:          app_zone_client.c
 *
 * DESCRIPTION:        Application Zone Client Behavior(Implementation)
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
 * Copyright NXP B.V. 2013. All rights reserved
 *
 ***************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>

#include "dbg.h"
#include "pdm.h"

#include "os_gen.h"
#include "pdum_gen.h"

#include "IASZONE.h"
#include "ha.h"

#include "app_timer_driver.h"
#include "app_zone_client.h"
#include "zha_CIE_node.h"
#include "app_common.h"
#include "Utilities.h"
#include "rnd_pub.h"
#include "IASACE.h"
#include "IASWD.h"

#if(TARGET == CIE)

#include "app_zcl_CIE_task.h"
#include "app_CIE_save.h"
#endif


/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifndef DEBUG_APP_ZONE
    #define TRACE_APP_ZONE               FALSE
#else
    #define TRACE_APP_ZONE               TRUE
#endif

#define ZONE_STARTUP_DELAY_IN_SEC      5

#define APP_IEEE_ADDR_RESPONSE         0x8001
#define APP_MATCH_DESCRIPTOR_RESPONSE  0x8006
#define APP_ZDP_DEVICE_ANNCE           0x0013

#define NUMBER_OF_ATTRIBUTES_IN_ZONE_TO_READ   0x05
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vGetIEEEAddress(uint8 u8Index);
PRIVATE void vCheckForZoneMatch(ZPS_tsAfZdpEvent *psAfZdpEvent);
PRIVATE void vCheckForZoneServerIeeeAddress(ZPS_tsAfZdpEvent *psAfZdpEvent);

PRIVATE uint64 u64GetIEEEAddress(uint16 u16ShortAddress);
PRIVATE uint16 u16GetShortAddress(uint64 *pu64IEEEAddress);
PRIVATE bool_t bZonePermited(tsCLD_IASZone_EnrollRequestPayload *psEnrollRequestPayload,uint64 *pu64IeeeServerAddress);
PRIVATE bool_t bDupilcate(tsCLD_IASZone_EnrollRequestPayload *psEnrollRequestPayload,uint64 *pu64IeeeServerAddress, uint8 *pu8ZoneID);

PRIVATE void vEnrollZoneWithCIE( tsCLD_IASZone_EnrollRequestPayload *psEnrollRequestPayload,
        uint64 *pu64IeeeServerAddress,
        tsCLD_IASZone_EnrollResponsePayload *pEnrollResponsePayload);

PRIVATE void vBindIfWD(tsCLD_IASZone_EnrollRequestPayload *psEnrollRequestPayload,
                  uint64 *pu64IeeeServerAddress,
                  tsCLD_IASZone_EnrollResponsePayload *pEnrollResponsePayload);

PRIVATE void vBindIfACEClientDevice(tsCLD_IASZone_EnrollRequestPayload *psEnrollRequestPayload,
                  uint64 *pu64IeeeServerAddress,
                  tsCLD_IASZone_EnrollResponsePayload *pEnrollResponsePayload);

PRIVATE void vProcessAlarm(uint8 u8ZoneId,uint16 u16Status);

PRIVATE void vUpdateDiscoverytable( uint16 u16ShortAddress,
                                   tsCLD_IASZone_EnrollResponsePayload *pEnrollResponsePayload);
PRIVATE void vCheckIfAlarmExistOnZones(void);
PRIVATE bool_t bCheckEntryExist( uint64 u64IEEEAddress);
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
PUBLIC tsDiscovedZoneServers sDiscovedZoneServers[MAX_ZONE_SERVER_NODES];
PUBLIC uint8 u8Discovered;
static bool bIsDiscovery = FALSE;
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
bool bStayInExitDelay = FALSE;
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: eSendZoneMatchDescriptorToJoinedNode
 *
 * DESCRIPTION:
 * Sends the Zone match descriptor for Zone client discovery as a broadcast.
 *
 *
 * INPUT:
 *  uint16 u16ProfileId Profile Identifier, destination address
 *
 * RETURNS:
 * ZPS status of the call
 *
 ****************************************************************************/
PUBLIC ZPS_teStatus eSendZoneMatchDescriptorToJoinedNode(uint16 u16ProfileId,uint16 u16DestinationAddress)
{
    uint16 au16InClusters[]={SECURITY_AND_SAFETY_CLUSTER_ID_IASZONE};
    uint8 u8TransactionSequenceNumber;
    ZPS_tsAplZdpMatchDescReq sMatch;
    ZPS_tuAddress uAddress;

    sMatch.u16ProfileId = u16ProfileId;
    sMatch.u8NumInClusters=sizeof(au16InClusters)/sizeof(uint16);
    sMatch.pu16InClusterList=au16InClusters;
    sMatch.pu16OutClusterList=NULL;
    sMatch.u8NumOutClusters=0;
    sMatch.u16NwkAddrOfInterest=u16DestinationAddress;

    uAddress.u16Addr=u16DestinationAddress;

    PDUM_thAPduInstance hAPduInst = PDUM_hAPduAllocateAPduInstance(apduZDP);

    if (hAPduInst == PDUM_INVALID_HANDLE)
    {
        DBG_vPrintf(TRACE_APP_ZONE, "Allocate PDU ERR:\n");
        return (ZPS_teStatus)PDUM_E_INVALID_HANDLE;
    }

    ZPS_teStatus eStatus = ZPS_eAplZdpMatchDescRequest(
                            hAPduInst,
                            uAddress,
                            FALSE,
                            &u8TransactionSequenceNumber,
                            &sMatch);

    if (eStatus)
    {
        PDUM_eAPduFreeAPduInstance(hAPduInst);
        DBG_vPrintf(TRACE_APP_ZONE, "Match ERR: 0x%x", eStatus);
    }

    return eStatus;
}

/****************************************************************************
 *
 * NAME: vHandleZDPReqResForZone
 *
 * DESCRIPTION:
 * Handles the stack event for Zone discovery. Called from the OS Task
 * upon a stack event.
 *
 * INPUT:
 * ZPS_tsAfEvent  * psStackEvent Pointer to stack event
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vHandleZDPReqResForZone(ZPS_tsAfZdpEvent  * psAfZdpEvent)
{
    vCheckForZoneMatch(psAfZdpEvent);
    vCheckForZoneServerIeeeAddress(psAfZdpEvent);
}

/****************************************************************************
 *
 * NAME: vHandleAppZoneClient
 *
 * DESCRIPTION:
 * Handles the Zone Cluster Client events.
 * This is called from the EndPoint call back in the application
 * when an Zone event occurs.
 *
 * INPUT:
 * tsCLD_IASZoneCallBackMessage *psCallBackMessage Pointer to cluster callback message
 * uint16 u16ShortAddress address of the msg.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vHandleAppZoneClient(tsCLD_IASZoneCallBackMessage *psCallBackMessage,uint16 u16ShortAddress)
{
      DBG_vPrintf(TRACE_APP_ZONE,"\nIAS Zone Command Id = %d\n",psCallBackMessage->u8CommandId);

      switch (psCallBackMessage->u8CommandId)
      {
        case E_CLD_IASZONE_CMD_ZONE_ENROLL_REQUEST:
        {
            tsCLD_IASZone_EnrollRequestPayload *psEnrollRequestPayload;
            tsCLD_IASZone_EnrollResponsePayload *pEnrollResponsePayload;
            uint64 u64IeeeServerAddress;

            DBG_vPrintf(TRACE_APP_ZONE,"\nIAS Zone Enroll Request from Node 0x%04x \n", u16ShortAddress);

            psEnrollRequestPayload = psCallBackMessage->uMessage.sZoneEnrollRequestCallbackPayload.psZoneEnrollRequestPayload;
            pEnrollResponsePayload = &psCallBackMessage->uMessage.sZoneEnrollRequestCallbackPayload.sZoneEnrollResponsePayload;
            u64IeeeServerAddress   = u64GetIEEEAddress(u16ShortAddress);
            DBG_vPrintf(TRACE_APP_ZONE," Long Address = 0x%016llx \n", u64IeeeServerAddress);
            vEnrollZoneWithCIE(psEnrollRequestPayload,&u64IeeeServerAddress,pEnrollResponsePayload);
            vUpdateDiscoverytable(u16ShortAddress,pEnrollResponsePayload);

            break;
        }
        case E_CLD_IASZONE_CMD_ZONE_STATUS_CHANGE_NOTIFICATION:
        {
            tsCLD_IASZone_StatusChangeNotificationPayload *psStatusChangeNotificationPayload;

            DBG_vPrintf(TRACE_APP_ZONE,"\nIAS Zone Status Change Notification from Node 0x%04x ", u16ShortAddress);

            psStatusChangeNotificationPayload = psCallBackMessage->uMessage.psZoneStatusNotificationPayload;
            DBG_vPrintf(TRACE_APP_ZONE,"\n ZoneId =%d",psStatusChangeNotificationPayload->u8ZoneId);
            DBG_vPrintf(TRACE_APP_ZONE,"\n ExtStatus=%d",psStatusChangeNotificationPayload->b8ExtendedStatus);
            DBG_vPrintf(TRACE_APP_ZONE,"\n Status = 0x%04x",psStatusChangeNotificationPayload->b16ZoneStatus);

            eCLD_IASACESetZoneParameterValue(
                    CIE_EP,
                    E_CLD_IASACE_ZONE_PARAMETER_ZONE_STATUS,
                    psStatusChangeNotificationPayload->u8ZoneId,
                    psStatusChangeNotificationPayload->b16ZoneStatus
                    );

            vProcessAlarm(psStatusChangeNotificationPayload->u8ZoneId,psStatusChangeNotificationPayload->b16ZoneStatus);
            break;
         }
      }

}

/****************************************************************************
 *
 * NAME: vSendUnenrollReq
 *
 * DESCRIPTION:
 * Sends a write attribute to un-enroll the zone with CIE
 *
 * INPUT:
 * uint8 u8ZoneID                               Test Zone ID
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vSendUnenrollReq(uint8 u8ZoneId)
{
    tsZCL_Address              sDestinationAddress;
    uint8                      u8TransactionSequenceNumber;
    tsZCL_WriteAttributeRecord sZCL_WriteAttributeRecord;
    uint64                     u64IEEEAddress=0;


    teZCL_CommandStatus eStatus = eCLD_IASACERemoveZoneEntry (
                    CIE_EP,                           /*uint8                                       u8SourceEndPointId,*/
                    u8ZoneId,                         /*uint8                                       u8ZoneID,*/
                    &u64IEEEAddress);                 /*uint64                                      *pu64IeeeAddress);*/

    if(!eStatus)
    {
        uint64 u64UnEnrolledIEEEAddr=0;

        sDestinationAddress.eAddressMode = E_ZCL_AM_SHORT_NO_ACK;
        sDestinationAddress.uAddress.u16DestinationAddress = u16GetShortAddress(&u64IEEEAddress);

        sZCL_WriteAttributeRecord.eAttributeDataType = E_ZCL_IEEE_ADDR;
        sZCL_WriteAttributeRecord.u16AttributeEnum = E_CLD_IASZONE_ATTR_ID_IAS_CIE_ADDRESS;
        sZCL_WriteAttributeRecord.pu8AttributeData = (uint8 *)&u64UnEnrolledIEEEAddr;

        uint8 u8Index = u8GetTableIndex(sDestinationAddress.uAddress.u16DestinationAddress);
        eStatus = eZCL_SendWriteAttributesRequest(
                                 CIE_EP,                                      /*uint8                       u8SourceEndPointId,*/
                                 sDiscovedZoneServers[u8Index].u8MatchList[0],/*uint8                       u8DestinationEndPointId,*/
                                 SECURITY_AND_SAFETY_CLUSTER_ID_IASZONE,      /*uint16                      u16ClusterId,*/
                                 FALSE,                                       /*bool_t                      bDirectionIsServerToClient,*/
                                 &sDestinationAddress,                        /*tsZCL_Address              *psDestinationAddress,*/
                                 &u8TransactionSequenceNumber,                /*uint8                      *pu8TransactionSequenceNumber,*/
                                 1,                                           /*uint8                       u8NumberOfAttributesInRequest,*/
                                 FALSE,                                       /*bool_t                      bIsManufacturerSpecific,*/
                                 ZCL_MANUFACTURER_CODE,                       /*uint16                      u16ManufacturerCode,*/
                                 &sZCL_WriteAttributeRecord);                 /*tsZCL_WriteAttributeRecord *pu16AttributeRequestList)*/

        DBG_vPrintf(TRACE_APP_ZONE,"\nWrite Attribute Error = %d\n",eStatus);

        if(!eStatus)
        {

            sDiscovedZoneServers[u8Index].bCIEAddressWritten=FALSE;
            sDiscovedZoneServers[u8Index].u8ZoneId=0xff;
            sDiscovedZoneServers[u8Index].u8ZoneState=0;

        }
        else
        {
            /*The write attribute failed - re enroll it back */
            sDiscovedZoneServers[u8Index].u8PermitEnrol = 1;
            vSendAutoEnroll(u8Index);
        }
    }
}

/****************************************************************************
 *
 * NAME: vReadAttributes
 *
 * DESCRIPTION:
 * Reads all attribute from the Zone server on an incoming stack msg
 *
 * INPUT:
 * tsZCL_CallBackEvent *psEvent
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vReadAttributes(tsZCL_CallBackEvent *psEvent)
{

        tsZCL_Address              sDestinationAddress;
        uint8                      u8TransactionSequenceNumber;
        uint16                     au16IASZoneAttributeList[NUMBER_OF_ATTRIBUTES_IN_ZONE_TO_READ] = {0x0000,0x0001,0x0002,0x0010,0x0011};


        if(bIsDiscovery)
        {
            sDestinationAddress.eAddressMode = E_ZCL_AM_SHORT;
            bIsDiscovery = FALSE;
        }
        else
        {
            sDestinationAddress.eAddressMode = E_ZCL_AM_SHORT_NO_ACK;
        }
        sDestinationAddress.uAddress.u16DestinationAddress = psEvent->pZPSevent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr;


        eZCL_SendReadAttributesRequest(
                    CIE_EP,                                /*uint8                        u8SourceEndPointId,*/
                    psEvent->pZPSevent->uEvent.sApsDataIndEvent.u8SrcEndpoint,/*uint8                        u8DestinationEndPointId,*/
                    SECURITY_AND_SAFETY_CLUSTER_ID_IASZONE, /*uint16                       u16ClusterId,*/
                    FALSE,                                 /*bool                         bDirectionIsServerToClient,*/
                    &sDestinationAddress,                  /*tsZCL_Address               *psAddress,*/
                    &u8TransactionSequenceNumber,          /*uint8                       *pu8TransactionSequenceNumber,*/
                    NUMBER_OF_ATTRIBUTES_IN_ZONE_TO_READ,                                      /*uint8                       u8NumberOfAttributesInRequest,*/
                    FALSE,                                 /*bool                         bIsManufacturerSpecific,*/
                    ZCL_MANUFACTURER_CODE,                 /*uint16                       u16ManufacturerCode);*/
                    &au16IASZoneAttributeList[0]);          /*uint16                     *pu16AttributeRequestList)*/

}
/****************************************************************************
 *
 * NAME: vSendAutoEnroll
 *
 * DESCRIPTION:
 * Sends out auto enroll response from CIE to a zone from the discovery table
 *
 * INPUT:
 * uint8 u8DeviceIndex  Index to the device in the Discovery table
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vSendAutoEnroll(uint8 u8DeviceIndex)
{

    /* Send CIE Address write to the target*/
    tsZCL_Address              sDestinationAddress;
    uint8                      u8TransactionSequenceNumber;
    tsZCL_WriteAttributeRecord sZCL_WriteAttributeRecord;
    uint64                     u64AttributeValue = ZPS_u64AplZdoGetIeeeAddr();
    teZCL_CommandStatus        eStatus;
    sDestinationAddress.eAddressMode = E_ZCL_AM_SHORT_NO_ACK;
    sDestinationAddress.uAddress.u16DestinationAddress = sDiscovedZoneServers[u8DeviceIndex].u16NwkAddrOfServer;

    sZCL_WriteAttributeRecord.eAttributeDataType = E_ZCL_IEEE_ADDR;
    sZCL_WriteAttributeRecord.u16AttributeEnum = E_CLD_IASZONE_ATTR_ID_IAS_CIE_ADDRESS;
    sZCL_WriteAttributeRecord.pu8AttributeData = (uint8 *)&u64AttributeValue;


    eStatus = eZCL_SendWriteAttributesRequest(
                                 CIE_EP,                                /*uint8                       u8SourceEndPointId,*/
                                 sDiscovedZoneServers[u8DeviceIndex].u8MatchList[0],/*uint8                       u8DestinationEndPointId,*/
                                 SECURITY_AND_SAFETY_CLUSTER_ID_IASZONE, /*uint16                      u16ClusterId,*/
                                 FALSE,                                 /*bool_t                      bDirectionIsServerToClient,*/
                                 &sDestinationAddress,                  /*tsZCL_Address              *psDestinationAddress,*/
                                 &u8TransactionSequenceNumber,          /*uint8                      *pu8TransactionSequenceNumber,*/
                                 1,                                     /*uint8                       u8NumberOfAttributesInRequest,*/
                                 FALSE,                                 /*bool_t                      bIsManufacturerSpecific,*/
                                 ZCL_MANUFACTURER_CODE,                 /*uint16                      u16ManufacturerCode,*/
                                 &sZCL_WriteAttributeRecord);            /*tsZCL_WriteAttributeRecord *pu16AttributeRequestList)*/

    DBG_vPrintf(TRACE_APP_ZONE,"\nWrite Attribute Error = %d\n",eStatus);

    if(!eStatus)
    {
        sDiscovedZoneServers[u8DeviceIndex].bCIEAddressWritten=TRUE;


        tsCLD_IASZone_EnrollRequestPayload sEnrollRequestPayload ;
        tsCLD_IASZone_EnrollResponsePayload sEnrollResponsePayload;

        sEnrollRequestPayload.e16ZoneType = sDiscovedZoneServers[u8DeviceIndex].u16ZoneType;
        sEnrollRequestPayload.u16ManufacturerCode = ZCL_MANUFACTURER_CODE;


        vEnrollZoneWithCIE( &sEnrollRequestPayload,
                            &sDiscovedZoneServers[u8DeviceIndex].u64IeeeAddrOfServer,
                            &sEnrollResponsePayload);

        vUpdateDiscoverytable(sDiscovedZoneServers[u8DeviceIndex].u16NwkAddrOfServer,&sEnrollResponsePayload);

        eCLD_IASZoneEnrollRespSend (
                CIE_EP,                                              /*uint8                             u8SourceEndPointId,*/
                sDiscovedZoneServers[u8DeviceIndex].u8MatchList[0],  /*uint8                             u8DestinationEndPointId,*/
                &sDestinationAddress,                                /*tsZCL_Address                     *psDestinationAddress,*/
                &u8TransactionSequenceNumber,                        /*uint8                             *pu8TransactionSequenceNumber,*/
                &sEnrollResponsePayload);                            /*tsCLD_IASZone_EnrollResponsePayload   *psPayload);*/
    }

}

/****************************************************************************
 *
 * NAME: vHandleDeviceAnnce
 *
 * DESCRIPTION:
 * Handles Device Anncement and updates the discovery table for further processing
 *
 * INPUT:
 * ZPS_tsAfEvent *psStackEvent  Stack event
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vHandleDeviceAnnce(ZPS_tsAfZdpEvent *psAfZdpEvent)
{
    if (APP_ZDP_DEVICE_ANNCE == psAfZdpEvent->u16ClusterId)
    {
        DBG_vPrintf(TRACE_APP_ZONE,"APP_ZDP_DEVICE_ANNCE..");

        if(bCheckEntryExist(psAfZdpEvent->uZdpData.sDeviceAnnce.u64IeeeAddr))
        {
            /*Duplicate , so change network address cause of network update*/
            uint16 u16ShortAddress = u16GetShortAddress(&psAfZdpEvent->uZdpData.sDeviceAnnce.u64IeeeAddr);
            uint8 u8Index = u8GetTableIndex(u16ShortAddress);
            sDiscovedZoneServers[u8Index].u16NwkAddrOfServer = psAfZdpEvent->uZdpData.sDeviceAnnce.u16NwkAddr;
            return;
        }
        eSendZoneMatchDescriptorToJoinedNode(HA_PROFILE_ID,psAfZdpEvent->uZdpData.sDeviceAnnce.u16NwkAddr );
    }
}
/****************************************************************************
 *
 * NAME: vSetPanelParamter
 *
 * DESCRIPTION:
 * Sets the Panel Parameters
 *
 * INPUT:
 * teCLD_IASACE_PanelStatus ePanelStatus Pannel Status to be updated
 * teCLD_IASACE_AlarmStatus eAlarmStatus Alarm Status to be updated
 * teCLD_IASACE_AudibleNotification eSound Sound/Mute to be updated
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vSetPanelParamter(teCLD_IASACE_PanelStatus ePanelStatus,
                              teCLD_IASACE_AlarmStatus eAlarmStatus,
                              teCLD_IASACE_AudibleNotification eSound)
{

    eCLD_IASACESetPanelParameter (
            CIE_EP,                                   /*uint8                                       u8SourceEndPointId,*/
            E_CLD_IASACE_PANEL_PARAMETER_PANEL_STATUS,/*teCLD_IASACE_PanelParameterID               eParameterId,*/
            ePanelStatus);                            /*        uint8                                       u8ParameterValue);*/


    eCLD_IASACESetPanelParameter (
            CIE_EP,                                   /*uint8                                       u8SourceEndPointId,*/
            E_CLD_IASACE_PANEL_PARAMETER_ALARM_STATUS,/*teCLD_IASACE_PanelParameterID               eParameterId,*/
            eAlarmStatus);                            /*        uint8                                       u8ParameterValue);*/

    eCLD_IASACESetPanelParameter (
            CIE_EP,                                           /*uint8                                       u8SourceEndPointId,*/
            E_CLD_IASACE_PANEL_PARAMETER_AUDIBLE_NOTIFICATION,/*teCLD_IASACE_PanelParameterID               eParameterId,*/
            eSound);                                          /*        uint8                                       u8ParameterValue);*/


}

/****************************************************************************
 *
 * NAME: vStartWarning
 *
 * DESCRIPTION:
 * Sends out the Start warning to a bound sounder
 *
 * INPUT:
 * uint8 eStrobeLevel Strobe Level
 * uint16 u16WarningDuration Warning duration in seconds
 * uint8 u8StrobeDutyCycle  Strobe duty cycle in 10 increments
 * uint8 u8WarningModeStrobeAndSirenLevel Bit map of Warning Mode, Strobe and Siren Level
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vStartWarning(uint8 eStrobeLevel,
                                  uint16 u16WarningDuration,
                                  uint8 u8StrobeDutyCycle,
                                  uint8 u8WarningModeStrobeAndSirenLevel)
{
    int Count;

    tsZCL_Address sDestinationAddress;
    uint8  u8TransactionSequenceNumber;
    tsCLD_IASWD_StartWarningReqPayload sPayload;
    teZCL_Status eStatus;

    sPayload.eStrobeLevel=eStrobeLevel;
    sPayload.u16WarningDuration=u16WarningDuration;
    sPayload.u8StrobeDutyCycle=u8StrobeDutyCycle;
    sPayload.u8WarningModeStrobeAndSirenLevel=u8WarningModeStrobeAndSirenLevel;

    sDestinationAddress.eAddressMode = E_ZCL_AM_SHORT_NO_ACK;

    /* Check for all the warning devices in the list and send start warning request */
    for(Count=0; Count < u8Discovered; Count++)
    {
        if((sDiscovedZoneServers[Count].bValid == TRUE) &&
           (sDiscovedZoneServers[Count].u8ZoneId != 0xFF) &&
           (sDiscovedZoneServers[Count].u16ZoneType == 0x0225))
        {
			sDestinationAddress.uAddress.u16DestinationAddress = sDiscovedZoneServers[Count].u16NwkAddrOfServer;

			eStatus = eCLD_IASWDStartWarningReqSend (
									CIE_EP,                      /*uint8                              u8SourceEndPointId,*/
									1,                           /*uint8                              u8DestinationEndPointId,*/
									&sDestinationAddress,        /*tsZCL_Address                      *psDestinationAddress,*/
									&u8TransactionSequenceNumber,/*uint8                              *pu8TransactionSequenceNumber,*/
									&sPayload);                  /*tsCLD_IASWD_StartWarningReqPayload *psPayload)*/

			DBG_vPrintf(TRACE_APP_ZONE,"\nWD Warning Status = %d \n",eStatus);
     	}
    }
}

/****************************************************************************
 *
 * NAME: vStartSquawk
 *
 * DESCRIPTION:
 * Sends out the Squawk to a bound sounder
 *
 * INPUT:
 * uint8 u8SquawkModeStrobeAndLevel Bitmap
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vStartSquawk(uint8 u8SquawkModeStrobeAndLevel)
{
    int Count;

    tsZCL_Address sDestinationAddress;
    uint8  u8TransactionSequenceNumber;
    tsCLD_IASWD_SquawkReqPayload sPayload;
    teZCL_Status eStatus;

    sPayload.u8SquawkModeStrobeAndLevel = u8SquawkModeStrobeAndLevel;

    sDestinationAddress.eAddressMode = E_ZCL_AM_SHORT_NO_ACK;

    /* Check for all the warning devices in the list and send start warning request */
    for(Count=0; Count < u8Discovered; Count++)
    {
        if((sDiscovedZoneServers[Count].bValid == TRUE) &&
           (sDiscovedZoneServers[Count].u8ZoneId != 0xFF) &&
           (sDiscovedZoneServers[Count].u16ZoneType == 0x0225))
        {
			sDestinationAddress.uAddress.u16DestinationAddress = sDiscovedZoneServers[Count].u16NwkAddrOfServer;

			eStatus = eCLD_IASWDSquawkReqSend (
									CIE_EP,                      /*uint8                              u8SourceEndPointId,*/
									1,                           /*uint8                              u8DestinationEndPointId,*/
									&sDestinationAddress,        /*tsZCL_Address                      *psDestinationAddress,*/
									&u8TransactionSequenceNumber,/*uint8                              *pu8TransactionSequenceNumber,*/
									&sPayload);                  /*tsCLD_IASWD_SquawkReqPayload		  *psPayload)*/

			DBG_vPrintf(TRACE_APP_ZONE,"\nWD Squawk Status = %d \n",eStatus);
     	}
    }
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: vUpdateDiscoverytable
 *
 * DESCRIPTION:
 * Updates the discovery table so that the display can fetch the values.
 *
 * INPUT:
 * uint16 u16ShortAddress : Used to get the index to the table
 * tsCLD_IASZone_EnrollResponsePayload *pEnrollResponsePayload Enroll payload
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vUpdateDiscoverytable( uint16 u16ShortAddress,
                                   tsCLD_IASZone_EnrollResponsePayload *pEnrollResponsePayload)
{

    uint8 u8Index;

    u8Index = u8GetTableIndex(u16ShortAddress);
    if(u8Index != 0xff)
    {
        sDiscovedZoneServers[u8Index].u8ZoneState = E_CLD_IASZONE_STATE_ENROLLED;
        sDiscovedZoneServers[u8Index].u8ZoneId=pEnrollResponsePayload->u8ZoneID;
    }

}

/****************************************************************************
 *
 * NAME: vEnrollZoneWithCIE
 *
 * DESCRIPTION:
 * Handles the Zone Cluster Client events.
 * This is called from the EndPoint call back in the application
 * when an Zone event occurs.
 *
 * INPUT:
 * tsCLD_IASZone_EnrollRequestPayload *psEnrollRequestPayload Enroll req Payload
 * uint64 *pu64IeeeServerAddress                              IEEE address of the zone
 * tsCLD_IASZone_EnrollResponsePayload *pEnrollResponsePayload Enroll rsp payload
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vEnrollZoneWithCIE( tsCLD_IASZone_EnrollRequestPayload *psEnrollRequestPayload,
                                uint64 *pu64IeeeServerAddress,
                                tsCLD_IASZone_EnrollResponsePayload *pEnrollResponsePayload)
{

    DBG_vPrintf(TRACE_APP_ZONE," Payload Zone Type  = 0x%04x\n",psEnrollRequestPayload->e16ZoneType);
    DBG_vPrintf(TRACE_APP_ZONE," Payload Manufacturer Code  = 0x%04x\n",psEnrollRequestPayload->u16ManufacturerCode);

    /*Set the Response as Default initially*/
    pEnrollResponsePayload->u8ZoneID = 0xFF;
    pEnrollResponsePayload->e8EnrollResponseCode=E_CLD_IASZONE_ENROLL_RESP_NO_ENROLL_PERMIT;
    /* Validate here to decide which are permitted or not - TBD, */
    if(bZonePermited(psEnrollRequestPayload, pu64IeeeServerAddress))
    {
        teZCL_CommandStatus eStatus=E_ZCL_CMDS_SUCCESS;
        if(!bDupilcate(psEnrollRequestPayload,pu64IeeeServerAddress,&(pEnrollResponsePayload->u8ZoneID)))
        {

            eStatus = eCLD_IASACEAddZoneEntry (
                        1,                                   /*uint8             u8SourceEndPointId,*/
                        psEnrollRequestPayload->e16ZoneType, /*uint16            u16ZoneType,*/
                        *pu64IeeeServerAddress,              /*uint64            u64IeeeAddress,*/
                        &(pEnrollResponsePayload->u8ZoneID));/*uint8             *pu8ZoneID);*/
        }

        /*Map the status code */
        switch (eStatus)
        {

            case E_ZCL_CMDS_SUCCESS:
            {
                pEnrollResponsePayload->e8EnrollResponseCode=E_CLD_IASZONE_ENROLL_RESP_SUCCESS;
                vBindIfWD(psEnrollRequestPayload,pu64IeeeServerAddress,pEnrollResponsePayload);
                vBindIfACEClientDevice(psEnrollRequestPayload,pu64IeeeServerAddress,pEnrollResponsePayload);
            }
                break;
            case E_ZCL_CMDS_INSUFFICIENT_SPACE:
                pEnrollResponsePayload->e8EnrollResponseCode = E_CLD_IASZONE_ENROLL_RESP_TOO_MANY_ZONES;
                break;
            default:
                pEnrollResponsePayload->e8EnrollResponseCode = E_CLD_IASZONE_ENROLL_RESP_NOT_SUPPORTED;
                break;
        }

    }
}

/****************************************************************************
 *
 * NAME: bZonePermited
 *
 * DESCRIPTION:
 * Checks if the zone is permitted to be enrolled.
 *
 * INPUT:
 * tsCLD_IASZone_EnrollRequestPayload *psEnrollRequestPayload Enrol Payload
 * uint64 *pu64IeeeServerAddress                              IEEE addr of zone
 *
 * RETURNS:
 * True is permitted else false
 *
 ****************************************************************************/
PRIVATE bool_t bZonePermited(tsCLD_IASZone_EnrollRequestPayload *psEnrollRequestPayload,uint64 *pu64IeeeServerAddress)
{
    uint16 u16ShortAddress  = u16GetShortAddress(pu64IeeeServerAddress);
    uint8 u8Index = u8GetTableIndex(u16ShortAddress);

    if (sDiscovedZoneServers[u8Index].u8PermitEnrol)
        return TRUE;

    return FALSE;
}
/****************************************************************************
 *
 * NAME: bDupilcate
 *
 * DESCRIPTION:
 * Checks if the zone is already in the CIE enrolled list.
 *
 * INPUT:
 * tsCLD_IASZone_EnrollRequestPayload *psEnrollRequestPayload Enroll Payload
 * uint64 *pu64IeeeServerAddress                   Test Zone IEEE address
 * uint8 * pu8ZoneID                               Test Zone ID
 *
 * RETURNS:
 * True if duplicate else false
 *
 ****************************************************************************/
PRIVATE bool_t bDupilcate(tsCLD_IASZone_EnrollRequestPayload *psEnrollRequestPayload,uint64 *pu64IeeeServerAddress,uint8 * pu8ZoneID)
{
    tsCLD_IASACE_ZoneTable  *psZoneTable;
    uint8 u8NumOfEnrolledZones = CLD_IASACE_ZONE_TABLE_SIZE;
    uint8 au8ZoneID[CLD_IASACE_ZONE_TABLE_SIZE];

    eCLD_IASACEGetEnrolledZones (
                    CIE_EP,                       /*uint8                                       u8SourceEndPointId,*/
                    (uint8*)&au8ZoneID[0],        /*uint8                                       *pu8ZoneID,*/
                    &u8NumOfEnrolledZones);       /*uint8                                       *pu8NumOfEnrolledZones);*/

    DBG_vPrintf(TRACE_APP_ZONE," No of Enrolled Zones = %d\n",u8NumOfEnrolledZones);

    for (;u8NumOfEnrolledZones > 0;u8NumOfEnrolledZones--)
    {
        eCLD_IASACEGetZoneTableEntry (
                CIE_EP,
                au8ZoneID[u8NumOfEnrolledZones-1],
                &psZoneTable);

        DBG_vPrintf(TRACE_APP_ZONE," ZoneId = %d\n",psZoneTable->u8ZoneID);
        DBG_vPrintf(TRACE_APP_ZONE," ZoneType = 0x%04x\n",psZoneTable->u16ZoneType);
        DBG_vPrintf(TRACE_APP_ZONE," 64Address = 0x%016llx\n\n\n",psZoneTable->u64IeeeAddress);

        if (
                (psEnrollRequestPayload->e16ZoneType == psZoneTable->u16ZoneType) &&
                (*pu64IeeeServerAddress == psZoneTable->u64IeeeAddress)
            )
        {
            *pu8ZoneID=psZoneTable->u8ZoneID;
            return TRUE;

        }
    }
    return FALSE;
}

/****************************************************************************
 *
 * NAME: vCheckForZoneMatch
 *
 * DESCRIPTION:
 * Checks for the Zone cluster match during Zone server discovery, if a match
 * found it will make an entry in the local discovery table.
 *
 *
 * INPUT:
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vCheckForZoneMatch( ZPS_tsAfZdpEvent  * psAfZdpEvent)
{
    if (APP_MATCH_DESCRIPTOR_RESPONSE == psAfZdpEvent->u16ClusterId)
    {
        if((psAfZdpEvent->uZdpData.sMatchDescRsp.u8Status == ZPS_E_SUCCESS)&&
                (psAfZdpEvent->uZdpData.sMatchDescRsp.u8MatchLength != 0))
        {
            uint8 i;
            sDiscovedZoneServers[u8Discovered].u16NwkAddrOfServer= psAfZdpEvent->uZdpData.sMatchDescRsp.u16NwkAddrOfInterest;
            DBG_vPrintf(TRACE_APP_ZONE,"\n\nNwk Address oF server = %04x\n",sDiscovedZoneServers[u8Discovered].u16NwkAddrOfServer);

            sDiscovedZoneServers[u8Discovered].u8MatchLength = psAfZdpEvent->uZdpData.sMatchDescRsp.u8MatchLength;
            DBG_vPrintf(TRACE_APP_ZONE,"Number of Zone Server EPs = %d\n",sDiscovedZoneServers[u8Discovered].u8MatchLength);

            for( i=0; i<sDiscovedZoneServers[u8Discovered].u8MatchLength && i<MAX_ZONE_SERVER_EPs ;i++)
            {
                /*sDiscovedZoneServers[u8Discovered].u8MatchList[i] = psAfZdpEvent->uZdpData.sMatchDescRsp.pu8MatchList[i];*/
                sDiscovedZoneServers[u8Discovered].u8MatchList[i] = psAfZdpEvent->uLists.au8Data[i];
                DBG_vPrintf(TRACE_APP_ZONE,"Zone Server EP# = %d\n",sDiscovedZoneServers[u8Discovered].u8MatchList[i]);
            }
            vGetIEEEAddress(u8Discovered);
        }
        u8Discovered++;
        PDM_eSaveRecordData( PDM_ID_APP_IASCIE_NODE,
                             &u8Discovered,
                             sizeof(uint8));
        PDM_eSaveRecordData( PDM_ID_APP_IASCIE_STRUCT,
                             &sDiscovedZoneServers[0],
                             sizeof(tsDiscovedZoneServers) * MAX_ZONE_SERVER_NODES);
    }
}
/****************************************************************************
 *
 * NAME: vCheckForZoneServerIeeeAddress
 *
 * DESCRIPTION:
 * Handles IEEE address look up query query
 * Makes an entry in the application Zone discovery table. Later this is used
 * for by the Zone enroll requests.
 *
 * This function is called from the application Zone handler with stack event
 * as input.
 *
 *
 * INPUT:
 * ZPS_tsAfEvent  * psStackEvent   Pointer to the stack event
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vCheckForZoneServerIeeeAddress(ZPS_tsAfZdpEvent *psAfZdpEvent)
{
    if (APP_IEEE_ADDR_RESPONSE == psAfZdpEvent->u16ClusterId)
    {
        if(!psAfZdpEvent->uZdpData.sIeeeAddrRsp.u8Status)
        {
            uint8 i;
            for( i=0; i<u8Discovered ;i++)
            {
                if( sDiscovedZoneServers[i].u16NwkAddrOfServer ==
                        psAfZdpEvent->uZdpData.sIeeeAddrRsp.u16NwkAddrRemoteDev)
                {
                    /*Make an entry in the Zone server tables*/
                    sDiscovedZoneServers[i].u64IeeeAddrOfServer = psAfZdpEvent->uZdpData.sIeeeAddrRsp.u64IeeeAddrRemoteDev;
                    DBG_vPrintf(TRACE_APP_ZONE,"Entry Added NWK Addr 0x%04x IEEE Addr 0x%016llx",
                            sDiscovedZoneServers[i].u16NwkAddrOfServer,sDiscovedZoneServers[i].u64IeeeAddrOfServer);
                    sDiscovedZoneServers[i].bValid = TRUE;

                    /* Send CIE Address write to the target*/
                    tsZCL_Address              sDestinationAddress;
                    uint8                      u8TransactionSequenceNumber;
                    tsZCL_WriteAttributeRecord sZCL_WriteAttributeRecord;
                    uint64                     u64AttributeValue = ZPS_u64AplZdoGetIeeeAddr();
                    teZCL_CommandStatus        eStatus;

                    sDestinationAddress.eAddressMode = E_ZCL_AM_SHORT;
                    sDestinationAddress.uAddress.u16DestinationAddress = sDiscovedZoneServers[i].u16NwkAddrOfServer;


                    sZCL_WriteAttributeRecord.eAttributeDataType = E_ZCL_IEEE_ADDR;
                    sZCL_WriteAttributeRecord.u16AttributeEnum = E_CLD_IASZONE_ATTR_ID_IAS_CIE_ADDRESS;
                    sZCL_WriteAttributeRecord.pu8AttributeData = (uint8 *)&u64AttributeValue;


                    //eStatus = eZCL_SendWriteAttributesRequest(
                                                 //CIE_EP,                                            /*uint8                       u8SourceEndPointId,*/
                                                 //sDiscovedZoneServers[u8DeviceIndex].u8MatchList[0],/*uint8                       u8DestinationEndPointId,*/
                                                // SECURITY_AND_SAFETY_CLUSTER_ID_IASZONE,            /*uint16                      u16ClusterId,*/
                                                // FALSE,                                             /*bool_t                      bDirectionIsServerToClient,*/
                                                // &sDestinationAddress,                              /*tsZCL_Address              *psDestinationAddress,*/
                                                // &u8TransactionSequenceNumber,                      /*uint8                      *pu8TransactionSequenceNumber,*/
                                                // 1,                                                 /*uint8                       u8NumberOfAttributesInRequest,*/
                                                // FALSE,                                             /*bool_t                      bIsManufacturerSpecific,*/
                                                // ZCL_MANUFACTURER_CODE,                             /*uint16                      u16ManufacturerCode,*/
                                                // &sZCL_WriteAttributeRecord);                       /*tsZCL_WriteAttributeRecord *pu16AttributeRequestList)*/


                    if(!eStatus)
                    {
                        sDiscovedZoneServers[i].bCIEAddressWritten=TRUE;
                        bIsDiscovery  = TRUE;
                    }
                }
            }
        }
     }
}

/****************************************************************************
 *
 * NAME: vGetIEEEAddress
 *
 * DESCRIPTION:
 * Finds an IEEE address on the local node by calling Stack API, if no entries
 * found it request the IEEE look up on air.
 *
 *
 * INPUT:
 * uint8 u8Index   Index to the discovery table point to the NWK address of
 *                 the discovered server
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vGetIEEEAddress(uint8 u8Index)
{
    /* Always query for address Over the air - Uncomment if required for local look up first*/
#ifdef LOCAL_ADDRESS_LOOK_UP
    /* See if there is a local address map exists  */
    uint64 u64IeeeAdress = ZPS_u64AplZdoLookupIeeeAddr(sDiscovedZoneServers[u8Index].u16NwkAddrOfServer);
    if( u64IeeeAdress != 0x0000000000000000 || u64IeeeAdress != 0xFFFFFFFFFFFFFFFF )
    {
        /*Valid address found, setting up the Zone server address */
        sDiscovedZoneServers[u8Index].u64IeeeAddrOfServer = u64IeeeAdress;
    }
    else
#endif
    {
        /* If there is no address map existing, then do a look up */
        PDUM_thAPduInstance hAPduInst;
        hAPduInst = PDUM_hAPduAllocateAPduInstance(apduZDP);

        if (hAPduInst == PDUM_INVALID_HANDLE)
        {
            DBG_vPrintf(TRACE_APP_ZONE, "IEEE Address Request - PDUM_INVALID_HANDLE\n");
        }
        else
        {
            ZPS_tuAddress uDstAddr;
            bool bExtAddr;
            uint8 u8SeqNumber;
            ZPS_teStatus eStatus;
            ZPS_tsAplZdpIeeeAddrReq sZdpIeeeAddrReq;

            uDstAddr.u16Addr = sDiscovedZoneServers[u8Index].u16NwkAddrOfServer;
            bExtAddr = FALSE;
            sZdpIeeeAddrReq.u16NwkAddrOfInterest=sDiscovedZoneServers[u8Index].u16NwkAddrOfServer;
            sZdpIeeeAddrReq.u8RequestType =0;
            sZdpIeeeAddrReq.u8StartIndex =0;

            eStatus= ZPS_eAplZdpIeeeAddrRequest(
                                            hAPduInst,
                                            uDstAddr,
                                            bExtAddr,
                                            &u8SeqNumber,
                                            &sZdpIeeeAddrReq);
            if (eStatus)
            {
                PDUM_eAPduFreeAPduInstance(hAPduInst);
                DBG_vPrintf(TRACE_APP_ZONE, "Address Request failed: 0x%02x\n", eStatus);
            }
        }
    }
}
/****************************************************************************
 *
 * NAME: u64GetIEEEAddress
 *
 * DESCRIPTION:
 * Finds an IEEE address on the discovery table given a short address
 *
 *
 * INPUT:
 * uint16 u16ShortAddress   Short address in the discovery table
 *
 * RETURNS:
 * IEEE address if found else 0
 *
 ****************************************************************************/
PRIVATE uint64 u64GetIEEEAddress(uint16 u16ShortAddress)
{
    uint8 i;
    for( i=0; i<u8Discovered ;i++)
    {
        if(
              (sDiscovedZoneServers[i].u16NwkAddrOfServer == u16ShortAddress)&&
              (sDiscovedZoneServers[i].bValid)
           )
        {
            /*Make an entry in the Zone server tables*/
            return sDiscovedZoneServers[i].u64IeeeAddrOfServer;

        }
    }
    return 0;

}
/****************************************************************************
 *
 * NAME: u16GetShortAddress
 *
 * DESCRIPTION:
 * Finds a short address on the discovery table given a IEEE address
 *
 *
 * INPUT:
 * uint64 *pu64IEEEAddress   Pointer to IEEE address in the discovery table
 *
 * RETURNS:
 * IEEE address if found else 0
 *
 ****************************************************************************/
PRIVATE uint16 u16GetShortAddress(uint64 *pu64IEEEAddress)
{
    uint8 i;
    for( i=0; i<u8Discovered ;i++)
    {
        if(
              (sDiscovedZoneServers[i].u64IeeeAddrOfServer == *pu64IEEEAddress)&&
              (sDiscovedZoneServers[i].bValid)
           )
        {
            /*Make an entry in the Zone server tables*/
            return sDiscovedZoneServers[i].u16NwkAddrOfServer;

        }
    }
    return 0;

}
/****************************************************************************
 *
 * NAME: u8GetTableIndex
 *
 * DESCRIPTION:
 * Finds a the discovery table index given a short address
 *
 *
 * INPUT:
 * uint16 u16ShortAddress   Short address in the discovery table
 *
 * RETURNS:
 * Index else 0xFF if not found
 *
 ****************************************************************************/
PUBLIC uint8 u8GetTableIndex( uint16 u16ShortAddress)
{
    uint8 i;
    for( i=0; i<u8Discovered ;i++)
    {
        if(
              (sDiscovedZoneServers[i].u16NwkAddrOfServer == u16ShortAddress)&&
              (sDiscovedZoneServers[i].bValid)
           )
        {
            return i;

        }
    }
    return 0xff;

}

/****************************************************************************
 *
 * NAME: bCheckEntryExist
 *
 * DESCRIPTION:
 * Finds whether in the discovery table entry exist with given IEEE Address address
 *
 *
 * INPUT:
 * uint64 u64IEEEAddress   IEEE address in the discovery table
 *
 * RETURNS:
 * TRUE if entry found else FALSE if not found
 *
 ****************************************************************************/
PRIVATE bool_t bCheckEntryExist( uint64 u64IEEEAddress)
{
    uint8 i;

    for( i=0; i<u8Discovered ;i++)
    {
        if(sDiscovedZoneServers[i].u64IeeeAddrOfServer == u64IEEEAddress)
        {
            return TRUE;
        }
    }
    return FALSE;

}
/****************************************************************************
 *
 * NAME: vAppUpdateZoneTable
 *
 * DESCRIPTION:
 * Updates the Zone tabled upon a read response attribute transaction
 *
 *
 * INPUT:
 * tsZCL_CallBackEvent *psEvent   Stack event
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppUpdateZoneTable(tsZCL_CallBackEvent *psEvent)
{
    DBG_vPrintf(TRACE_APP_ZONE,"\n\nAddress = 0x%04x ",psEvent->pZPSevent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr);
    uint8 u8Index=u8GetTableIndex(psEvent->pZPSevent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr);
    if(0xff != u8Index)
    {

        switch (psEvent->uMessage.sIndividualAttributeResponse.u16AttributeEnum)
        {
            case E_CLD_IASZONE_ATTR_ID_ZONE_STATE:
                sDiscovedZoneServers[u8Index].u8ZoneState=(uint8)(*((uint8*)psEvent->uMessage.sIndividualAttributeResponse.pvAttributeData));
                DBG_vPrintf(TRACE_APP_ZONE,"\nState = %d",sDiscovedZoneServers[u8Index].u8ZoneState);
                break;
            case E_CLD_IASZONE_ATTR_ID_ZONE_TYPE:
                sDiscovedZoneServers[u8Index].u16ZoneType=(uint16)(*((uint16*)psEvent->uMessage.sIndividualAttributeResponse.pvAttributeData));
                DBG_vPrintf(TRACE_APP_ZONE,"\nType = %d",sDiscovedZoneServers[u8Index].u16ZoneType);
                break;
            case E_CLD_IASZONE_ATTR_ID_ZONE_STATUS:
                sDiscovedZoneServers[u8Index].u16ZoneStatus=(uint16)(*((uint16*)psEvent->uMessage.sIndividualAttributeResponse.pvAttributeData));
                DBG_vPrintf(TRACE_APP_ZONE,"\nStatus = %d",sDiscovedZoneServers[u8Index].u16ZoneStatus);
                break;
            case E_CLD_IASZONE_ATTR_ID_IAS_CIE_ADDRESS:

                break;
            case E_CLD_IASZONE_ATTR_ID_ZONE_ID:
            {
                tsCLD_IASACE_ZoneTable  *psZoneTable;
                sDiscovedZoneServers[u8Index].u8ZoneId=(uint8)(*((uint8*)psEvent->uMessage.sIndividualAttributeResponse.pvAttributeData));
                /* As entry does not exist please remove from application as well */
                if((sDiscovedZoneServers[u8Index].u8ZoneId != 0xFF) &&
                    (eCLD_IASACEGetZoneTableEntry (CIE_EP,sDiscovedZoneServers[u8Index].u8ZoneId,&psZoneTable) == E_ZCL_CMDS_NOT_FOUND))
                {
                    sDiscovedZoneServers[u8Index].bCIEAddressWritten=FALSE;
                    sDiscovedZoneServers[u8Index].u8ZoneId=0xFF;
                    sDiscovedZoneServers[u8Index].u8ZoneState=0;
                }
                DBG_vPrintf(TRACE_APP_ZONE,"\nZone Id = %d\n\n\n",sDiscovedZoneServers[u8Index].u8ZoneId);
            }
                break;

            default:
                break;
        }
    }
}
/****************************************************************************
 *
 * NAME: vDisplayPanel
 *
 * DESCRIPTION:
 * Prints the discovey Table out for debug purpose
 *
 *
 * INPUT:
 * void
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vDisplayPanel(void)
{
    uint8 i;
    uint8 u8Len;
    DBG_vPrintf(TRACE_APP_ZONE,"\n\nu64Addr\t\t\tu16Add\tZoneId\tState\tType\tStatus\tArmBypass\tConfig");

    for( i=0; i<u8Discovered ;i++)
    {
        eCLD_IASACEGetZoneParameter (
                        CIE_EP,                                         /*uint8                                       u8SourceEndPointId,*/
                        E_CLD_IASACE_ZONE_PARAMETER_ZONE_STATUS,        /*teCLD_IASACE_ZoneParameterID                eParameterId,*/
                        sDiscovedZoneServers[i].u8ZoneId,               /*uint8                                       u8ZoneID,*/
                        &u8Len,                                         /*uint8                                       *pu8ParameterLength,*/
                        (uint8*)&sDiscovedZoneServers[i].u16ZoneStatus);/*uint8                                       *pu8ParameterValue);*/


        eCLD_IASACEGetZoneParameter (
                        CIE_EP,                                           /*uint8                                       u8SourceEndPointId,*/
                        E_CLD_IASACE_ZONE_PARAMETER_ZONE_STATUS_FLAG,     /*teCLD_IASACE_ZoneParameterID                eParameterId,*/
                        sDiscovedZoneServers[i].u8ZoneId,                 /*uint8                                       u8ZoneID,*/
                        &u8Len,                                           /*uint8                                       *pu8ParameterLength,*/
                        (uint8*)&sDiscovedZoneServers[i].u8ArmBypass);    /*uint8                                       *pu8ParameterValue);*/


        eCLD_IASACEGetZoneParameter (
                        CIE_EP,                                        /*uint8                                       u8SourceEndPointId,*/
                        E_CLD_IASACE_ZONE_PARAMETER_ZONE_CONFIG_FLAG,  /*teCLD_IASACE_ZoneParameterID                eParameterId,*/
                        sDiscovedZoneServers[i].u8ZoneId,              /*uint8                                       u8ZoneID,*/
                        &u8Len,                                        /*uint8                                       *pu8ParameterLength,*/
                        (uint8*)&sDiscovedZoneServers[i].u8Config);    /*uint8                                       *pu8ParameterValue);*/


    }

#if TRACE_APP_ZONE
    for( i=0; i<u8Discovered ;i++)
        DBG_vPrintf(TRACE_APP_ZONE,"\n0x%016llx \t0x%04x \t0x%02x \t0x%02x \t0x%04x \t0x%04x \t0x%02x \t\t0x%02x",
                sDiscovedZoneServers[i].u64IeeeAddrOfServer,
                sDiscovedZoneServers[i].u16NwkAddrOfServer,
                sDiscovedZoneServers[i].u8ZoneId,
                sDiscovedZoneServers[i].u8ZoneState,
                sDiscovedZoneServers[i].u16ZoneType,
                sDiscovedZoneServers[i].u16ZoneStatus,
                sDiscovedZoneServers[i].u8ArmBypass,
                sDiscovedZoneServers[i].u8Config
        );
#endif

}
/****************************************************************************
 *
 * NAME: vBindIfWD
 *
 * DESCRIPTION:
 * Binds the WD cluster if found when the CIE enrollment is successful for Wd device
 *
 *
 * INPUT:
 * tsCLD_IASZone_EnrollRequestPayload *psEnrollRequestPayload Enroll Req
 * uint64 *pu64IeeeServerAddress                              IEEE addr
 * tsCLD_IASZone_EnrollResponsePayload *pEnrollResponsePayload Enroll Resp
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vBindIfWD(tsCLD_IASZone_EnrollRequestPayload *psEnrollRequestPayload,
                  uint64 *pu64IeeeServerAddress,
                  tsCLD_IASZone_EnrollResponsePayload *pEnrollResponsePayload)
{
    uint16 u16ShortAddress;
    uint8 u8Index;
    if(psEnrollRequestPayload->e16ZoneType == E_CLD_IASZONE_TYPE_STANDARD_WARNING_DEVICE)
    {
        u16ShortAddress = u16GetShortAddress(pu64IeeeServerAddress);
        u8Index = u8GetTableIndex(u16ShortAddress);
        if(u8Index != 0xff)
        {
            /*Make Binding for the Client as all the communication will go to the client*/
            ZPS_teStatus eStatus=ZPS_eAplZdoBind(
                    SECURITY_AND_SAFETY_CLUSTER_ID_IASWD,                     /*uint16 u16ClusterId,*/
                    sDiscovedZoneServers[u8Index].u8MatchList[0],             /*uint8 u8SrcEndpoint,*/
                    u16ShortAddress,                                          /*uint16 u16DstAddr,*/
                    *pu64IeeeServerAddress,                                   /*uint64 u64DstIeeeAddr,*/
                    CIE_EP);
            DBG_vPrintf(TRACE_APP_ZONE,"\nWD Bind Status = %d\n",eStatus);
        }
    }
}
/****************************************************************************
 *
 * NAME: vBindIfACEClientDevice
 *
 * DESCRIPTION:
 * Binds the ACE cluster if found when the CIE enrollment is successful for Keypad
 *
 *
 * INPUT:
 * tsCLD_IASZone_EnrollRequestPayload *psEnrollRequestPayload Enroll Req
 * uint64 *pu64IeeeServerAddress                              IEEE addr
 * tsCLD_IASZone_EnrollResponsePayload *pEnrollResponsePayload Enroll Resp
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vBindIfACEClientDevice(tsCLD_IASZone_EnrollRequestPayload *psEnrollRequestPayload,
                  uint64 *pu64IeeeServerAddress,
                  tsCLD_IASZone_EnrollResponsePayload *pEnrollResponsePayload)
{
    uint16 u16ShortAddress;
    uint8 u8Index;
    if(psEnrollRequestPayload->e16ZoneType == E_CLD_IASZONE_TYPE_KEYPAD)
    {
        u16ShortAddress = u16GetShortAddress(pu64IeeeServerAddress);
        u8Index = u8GetTableIndex(u16ShortAddress);
        if(u8Index != 0xff)
        {
            /*Make Binding for the Client as all the communication will go to the client*/
            ZPS_teStatus eStatus=ZPS_eAplZdoBind(
                    SECURITY_AND_SAFETY_CLUSTER_ID_IASACE,             /*uint16 u16ClusterId,*/
                    sDiscovedZoneServers[u8Index].u8MatchList[0],      /*uint8 u8SrcEndpoint,*/
                    u16ShortAddress,                                   /*uint16 u16DstAddr,*/
                    *pu64IeeeServerAddress,                            /*uint64 u64DstIeeeAddr,*/
                    CIE_EP);
            DBG_vPrintf(TRACE_APP_ZONE,"\nACE Bind Status = %d\n",eStatus);
        }
    }
}
/****************************************************************************
 *
 * NAME: vProcessAlarm
 *
 * DESCRIPTION:
 * Process Alarms on the CIE for a given zone and start warning if required.
 *
 *
 * INPUT:
 * uint8 u8ZoneId ZOne ID
 * uint16 u16Status Status mask of the zone
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vProcessAlarm(uint8 u8ZoneId,uint16 u16Status)
{
    uint8 u8ParamLen;
    uint8 u8ParamValue[1];
    uint8 u8PanelStatus = 0;

    eCLD_IASACEGetZoneParameter (
                    CIE_EP,                                      /*uint8                                       u8SourceEndPointId,*/
                    E_CLD_IASACE_ZONE_PARAMETER_ZONE_STATUS_FLAG,/*teCLD_IASACE_ZoneParameterID                eParameterId,*/
                    u8ZoneId,                                    /*uint8                                       u8ZoneID,*/
                    &u8ParamLen,                                 /*uint8                                       *pu8ParameterLength,*/
                    &u8ParamValue[0]);                           /*uint8                                       *pu8ParameterValue);*/

    eCLD_IASACEGetPanelParameter (
        CIE_EP,                                        /*uint8                                       u8SourceEndPointId,*/
        E_CLD_IASACE_PANEL_PARAMETER_PANEL_STATUS,     /*teCLD_IASACE_PanelParameterID               eParameterId,*/
        &u8PanelStatus);

    DBG_vPrintf(TRACE_APP_ZONE,"\nWDZone Status = %d u8PanelStatus = %d \n",u8ParamValue[0],u8PanelStatus);

    if(  ZCL_IS_BIT_SET(uint8,u8ParamValue[0],CLD_IASACE_ZONE_STATUS_FLAG_ARM) &&
         ZCL_IS_BIT_CLEAR(uint8,u8ParamValue[0],CLD_IASACE_ZONE_STATUS_FLAG_BYPASS)    )
    {

        if(  ZCL_IS_BIT_SET(uint16,u16Status,CLD_IASZONE_STATUS_MASK_ALARM1) ||
             ZCL_IS_BIT_SET(uint16,u16Status,CLD_IASZONE_STATUS_MASK_ALARM2) ||
             ZCL_IS_BIT_SET(uint16,u16Status,CLD_IASZONE_STATUS_MASK_TAMPER) ||
             ZCL_IS_BIT_SET(uint16,u16Status,CLD_IASZONE_STATUS_MASK_TROUBLE) ||
             ZCL_IS_BIT_SET(uint16,u16Status,CLD_IASZONE_STATUS_MASK_AC_MAINS)
             )
        {
            if(u8PanelStatus == E_CLD_IASACE_PANEL_STATUS_PANEL_EXIT_DELAY)
            {
                /* If in exit delay and alarm goes off stay in that state till alarm gets clear
                 * and store seconds remaining iff its first time received during exit */
                bStayInExitDelay = TRUE;
            }
            else if(u8PanelStatus != E_CLD_IASACE_PANEL_STATUS_PANEL_ENTRY_DELAY)
            {
                eCLD_IASACESetPanelParameter (
                        CIE_EP,                                         /*uint8                                       u8SourceEndPointId,*/
                        E_CLD_IASACE_PANEL_PARAMETER_SECONDS_REMAINING, /*teCLD_IASACE_PanelParameterID               eParameterId,*/
                        CLD_IASACE_PANEL_PARAMTER_SECONDS_REMAINING);
                DBG_vPrintf(TRACE_APP_ZONE,"\nWDaLarmStatus = %d \n",u16Status);
                vSetPanelParamter(E_CLD_IASACE_PANEL_STATUS_PANEL_ENTRY_DELAY,E_CLD_IASACE_ALARM_STATUS_BURGLAR,E_CLD_IASACE_AUDIBLE_NOTIF_DEFAULT_SOUND);
                OS_eStopSWTimer(APP_EntryExitDelayTmr);
                OS_eStartSWTimer(APP_EntryExitDelayTmr,APP_TIME_MS(1000), NULL );
                vStartWarning(STROBE_LEVEL,ENTRY_EXIT_DELAY_WARNING_DURATION,50/*STROBE_DUTY_CYCLE*/,WARNING_MODE_STROBE_AND_SIREN_LEVEL_ENTRY_EXIT_DELAY);
            }
        }else if(u8PanelStatus == E_CLD_IASACE_PANEL_STATUS_PANEL_EXIT_DELAY)
        {
            vCheckIfAlarmExistOnZones();
        }
    }
}

PRIVATE void vCheckIfAlarmExistOnZones(void)
{
    uint8 u8ParamLen;
    uint8 u8ParamValue[2];
    uint8 u8NumOfZonesInAlarm = 0;
    int i = 0;
    uint16  u16ZoneStatus = 0;
    uint8 u8SecondsRemaining;

    /* Check if alarm still exist on any of armed zones continue in exit delay state else start exit delay timer again */
    for(i=0;i<u8Discovered;i++)
    {
        if((sDiscovedZoneServers[i].bValid == TRUE) &&
                sDiscovedZoneServers[i].u8ZoneId != 0xFF )
        {
            eCLD_IASACEGetZoneParameter (
                            CIE_EP,                                      /*uint8                                       u8SourceEndPointId,*/
                            E_CLD_IASACE_ZONE_PARAMETER_ZONE_STATUS_FLAG,/*teCLD_IASACE_ZoneParameterID                eParameterId,*/
                            sDiscovedZoneServers[i].u8ZoneId,            /*uint8                                       u8ZoneID,*/
                            &u8ParamLen,                                 /*uint8                                       *pu8ParameterLength,*/
                            &u8ParamValue[0]);

            if( ZCL_IS_BIT_SET(uint8,u8ParamValue[0],CLD_IASACE_ZONE_STATUS_FLAG_ARM) &&
                             ZCL_IS_BIT_CLEAR(uint8,u8ParamValue[0],CLD_IASACE_ZONE_STATUS_FLAG_BYPASS))
            {
                eCLD_IASACEGetZoneParameter (
                            CIE_EP,                                  /*uint8                                       u8SourceEndPointId,*/
                            E_CLD_IASACE_ZONE_PARAMETER_ZONE_STATUS, /*teCLD_IASACE_ZoneParameterID                eParameterId,*/
                            sDiscovedZoneServers[i].u8ZoneId,        /*uint8                                       u8ZoneID,*/
                            &u8ParamLen,                             /*uint8                                       *pu8ParameterLength,*/
                            &u8ParamValue[0]);                       /*uint8                                       *pu8ParameterValue); */

                memcpy((uint8 *)&u16ZoneStatus,u8ParamValue,2);

                if( u16ZoneStatus != 0)
                {
                    u8NumOfZonesInAlarm++;
                }

            }
        }
    }
    if(u8NumOfZonesInAlarm == 0)
    {
        DBG_vPrintf(TRACE_APP_ZONE,"\n u8NumOfZonesInAlarm = %d \n",u8NumOfZonesInAlarm);
        /*Restart Exit delay to reach the end Resoultion of this timer goes only till 1 mins */
        eCLD_IASACEGetPanelParameter (
                CIE_EP,                                        /*uint8                                       u8SourceEndPointId,*/
                E_CLD_IASACE_PANEL_PARAMETER_SECONDS_REMAINING,/*teCLD_IASACE_PanelParameterID               eParameterId,*/
                &u8SecondsRemaining);
        OS_eStopSWTimer(APP_EntryExitDelayTmr);
        OS_eStartSWTimer(APP_EntryExitDelayTmr, APP_TIME_MS(1000), NULL );
        vStartWarning(STROBE_LEVEL,u8SecondsRemaining,50/*STROBE_DUTY_CYCLE*/,WARNING_MODE_STROBE_AND_SIREN_LEVEL_ENTRY_EXIT_DELAY);
        bStayInExitDelay = FALSE;
    }
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
