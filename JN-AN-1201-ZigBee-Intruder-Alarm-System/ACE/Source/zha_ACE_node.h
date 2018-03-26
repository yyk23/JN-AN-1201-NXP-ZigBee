/*****************************************************************************
 *
 * MODULE:             JN-AN-1201
 *
 * COMPONENT:          zha_remote_node.c
 *
 * DESCRIPTION:        ZHA Demo : Stack <-> Remote Control App Interaction
 *                     (Interface)
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

#ifndef APP_SENSOR_NODE_H_
#define APP_SENSOR_NODE_H_

#include "zps_nwk_sap.h"
#include "zps_nwk_nib.h"
#include "zps_nwk_pub.h"
#include "zps_apl.h"
#include "zps_apl_zdo.h"

#include "app_common.h"

#include "zcl_customcommand.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define SCAN_AND_JOIN_TIME 5
#define POLL_MINIMUM_RATE_IN_SECS    7
#define ACE_ENDPOINT_ID         1
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef enum 
{
    E_CMD_STATE_IDLE,
    E_CMD_ARM_STATE_SELECT_MODE,
    E_CMD_BYPASS_STATE_SELECT_ZONES,
    E_CMD_STATE_GET_ZONE_RELATED_INFO,
    E_CMD_STATE_SEND_GET_ZONE_INFO,
    E_CMD_STATE_SEND_GET_ZONE_STATUS_SEND,
    E_CMD_STATE_START_ERASE_PDM,
    E_CMD_STATE_ERASE_PDM
}teACECommandState;
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

PUBLIC void APP_vInitialiseNode(void);
PUBLIC void vStartFastPolling(uint8 u8Seconds);
#ifdef SLEEP_ENABLE
    PUBLIC void vReloadSleepTimers(void);
    PUBLIC void vUpdateKeepAliveTimer(void);
    PUBLIC bool bWatingToSleep(void);
    PUBLIC void vWakeCallBack(void);
    #ifdef DEEP_SLEEP_ENABLE
        PUBLIC void vLoadDeepSleepTimer(uint8 u8SleepTime);
        PUBLIC void vSetUpWakeUpConditions(void);
    #endif
#endif

/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/
extern teACECommandState eCommandState;
extern const uint8 u8MyEndpoint;
extern uint8 u8ArmMode;
extern uint8 u8SelZoneID;
extern uint8 au8ZoneIDList[];
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#endif /*APP_SENSOR_NDOE_H_*/
