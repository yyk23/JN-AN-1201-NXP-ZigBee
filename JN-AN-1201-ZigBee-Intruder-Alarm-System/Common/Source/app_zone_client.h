/*****************************************************************************
 *
 * MODULE:             JN-AN-1201
 *
 * COMPONENT:          app_zone_client.h
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

#ifndef APP_ZONE_CLIENT_H
#define APP_ZONE_CLIENT_H
#include "zps_apl_af.h"
#include "IASZone.h"
#include "IASACE.h"
#include "appZdpExtraction.h"
#include "PDM_IDs.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define ZONE_QUERY_TIME_IN_SEC 60/*once a day 24(H)*60(M)*60(S)= 86400*/ /*To find the server*/
#define ZONE_DISCOVERY_TIMEOUT_IN_SEC 15
#define ZONE_DL_IN_PROGRESS_TIME_IN_SEC  120

#define RAND_TIMEOUT_MIN_IN_SEC 60
#define RAND_TIMEOUT_MAX_IN_SEC 120

#define MAX_ZONE_SERVER_EPs 1
#define MAX_ZONE_SERVER_NODES 10

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef struct {
    bool bValid;
    bool bCIEAddressWritten;

    uint64 u64IeeeAddrOfServer;
    uint16 u16NwkAddrOfServer;

    uint8 u8ZoneState;
    uint16 u16ZoneType;
    uint16 u16ZoneStatus;
    uint8 u8ZoneId;

    uint8 u8ArmBypass;
    uint8 u8Config;
    uint8 u8PermitEnrol;
    uint8 u8MatchLength;
    uint8 u8MatchList[MAX_ZONE_SERVER_EPs];
}tsDiscovedZoneServers;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
PUBLIC void vHandleZDPReqResForZone(ZPS_tsAfZdpEvent  *psAfZdpEvent);
PUBLIC void vHandleAppZoneClient(tsCLD_IASZoneCallBackMessage *psCallBackMessage,uint16 u16ShortAddress);
PUBLIC void vHandleDeviceAnnce(ZPS_tsAfZdpEvent *psAfZdpEvent);

PUBLIC void vSetPanelParamter(teCLD_IASACE_PanelStatus ePanelStatus,
                              teCLD_IASACE_AlarmStatus eAlarmStatus,
                              teCLD_IASACE_AudibleNotification eSound);
PUBLIC void vStartWarning(uint8 eStrobeLevel,
                                  uint16 u16WarningDuration,
                                  uint8 u8u8StrobeDutyCycle,
                                  uint8 u8WarningModeStrobeAndSirenLevel);
PUBLIC void vStartSquawk(uint8 u8SquawkModeStrobeAndLevel);
PUBLIC void vSendUnenrollReq(uint8 u8ZoneId);
PUBLIC void vSendAutoEnroll(uint8 u8DeviceIndex);
PUBLIC void vAppUpdateZoneTable(tsZCL_CallBackEvent *psEvent);
PUBLIC void vReadAttributes(tsZCL_CallBackEvent *psEvent);
PUBLIC void vDisplayPanel(void);
PUBLIC uint8 u8GetTableIndex( uint16 u16ShortAddress);
/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/
extern tsDiscovedZoneServers sDiscovedZoneServers[MAX_ZONE_SERVER_NODES];
extern uint8 u8Discovered;
extern bool bStayInExitDelay;
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#endif /*APP_OTA_CLIENT_H*/
