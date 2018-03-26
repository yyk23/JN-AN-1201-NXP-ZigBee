/*****************************************************************************
 *
 * MODULE:             JN-AN-1201
 *
 * COMPONENT:          zha_ACE_node.c
 *
 * DESCRIPTION:        ZHA Demo : Stack <-> Remote Control App Interaction for ACE
 *                     (Implementation)
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
#include <string.h>
#include <appapi.h>
#include "os.h"
#include "os_gen.h"
#include "pdum_apl.h"
#include "pdum_gen.h"
#include "pdm.h"
#include "dbg.h"
#include "dbg_uart.h"
#include "pwrm.h"
#include "zps_gen.h"
#include "zps_apl_af.h"
#include "zps_apl_zdo.h"
#include "zps_apl_aib.h"
#include "zps_apl_zdp.h"
#include "rnd_pub.h"

#include "app_common.h"
#include "groups.h"

#include "PDM_IDs.h"

#include "app_timer_driver.h"
#include "app_captouch_buttons.h"
#include "app_led_control.h"

#include "zha_ACE_node.h"

#include "app_zcl_ACE_task.h"
#include "app_zbp_utilities.h"

#include "app_events.h"
#include "zcl_customcommand.h"
#include "ha.h"

#include "haEzJoin.h"

#include "zcl_common.h"
#include "IASACE.h"
#include "IASZone.h"
#include "app_ias_indicator.h"
#include "app_ias_save.h"
#include "PingParent.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifdef DEBUG_ACE_NODE
    #define TRACE_ACE_NODE   TRUE
#else
    #define TRACE_ACE_NODE   FALSE
#endif

#ifndef DEBUG_SLEEP
    #define TRACE_SLEEP     FALSE
#else
    #define TRACE_SLEEP     TRUE
#endif

#ifdef DEEP_SLEEP_ENABLE
    #define POWER_BTN (1)
    #define DEEP_SLEEP_TIME 155 /* drop in the deep sleep after 155((DEEP_SLEEPTIME - KEEPALIVE) / SLEEP_TIME) secs = 15 minutes */
#endif

#define NUMBER_DEVICE_TO_BE_DISCOVERED 5

#define MAX_SERVICE_DISCOVERY   3
#define SLEEP_DURATION_MS (800)
#define SLEEP_TIMER_TICKS_PER_MS (32)

#define APP_MATCH_DESCRIPTOR_RESPONSE   0x8006

#define SIX_SECONDS                     6

const uint8 u8MyEndpoint = 1;
static uint8 s_au8LnkKeyArray[16] = {0x5a, 0x69, 0x67, 0x42, 0x65, 0x65, 0x41, 0x6c,
                                     0x6c, 0x69, 0x61, 0x6e, 0x63, 0x65, 0x30, 0x39};
static uint8 u8PingCount = 0;
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
#ifdef SLEEP_ENABLE
    PRIVATE void vLoadKeepAliveTime(uint8 u8TimeInSec);
#endif
PRIVATE void APP_vHandleKeyPress(teUserKeyCodes eKeyCode);
PRIVATE void vHandleAppEvent( APP_tsEvent sAppEvent );
PUBLIC void vStopAllTimers(void);
PUBLIC void vStopTimer(OS_thSWTimer hSwTimer);

#ifdef PDM_EEPROM
PUBLIC uint8 u8PDM_CalculateFileSystemCapacity(void);
PUBLIC uint8 u8PDM_GetFileSystemOccupancy(void);
#endif

PRIVATE void vHandleJoinAndRejoinNWK( ZPS_tsAfEvent *pZPSevent,teEZ_JoinAction eJoinAction  );
PRIVATE void app_vRestartNode (void);
PRIVATE void app_vStartNodeFactoryNew(void);
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
PUBLIC  tsDeviceDesc           sDeviceDesc;
uint16 u16GroupId;
teACECommandState eCommandState;
teShiftLevel eShiftLevel = E_SHIFT_0;
uint8 u8ArmMode = 0;
uint8 u8SelZoneID = 0;
uint8 au8ZoneIDList[CLD_IASACE_ZONE_TABLE_SIZE] = {0};
extern bool bPollFailed;
extern bool_t bDeepSleep;
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
/*Primary channel Set */
PRIVATE uint16 u16FastPoll;
PRIVATE uint8 u8RunningWithJoinFailedTime = 10;
static uint8 u8PollCount = 0;
#ifdef SLEEP_ENABLE
    #ifdef DEEP_SLEEP_ENABLE
        PRIVATE uint8 u8DeepSleepTime= DEEP_SLEEP_TIME;
    #endif
    PRIVATE uint8 u8KeepAliveTime = KEEP_ALIVETIME;
    PRIVATE pwrm_tsWakeTimerEvent    sWake;
#endif

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: APP_vInitialiseNode
 *
 * DESCRIPTION:
 * Initialises the application related functions
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_vInitialiseNode(void)
{
    PDM_teStatus eStatusReload;
    uint16 u16ByteRead;
    DBG_vPrintf(TRACE_ACE_NODE, "\nAPP_vInitialiseNode*");

    vInitIndicationLEDs();

    /* Initialise buttons;
     */
    APP_bButtonInitialise();

    /* Restore any application data previously saved to flash */
    PDM_eReadDataFromRecord(PDM_ID_APP_ACE, &sDeviceDesc,
            sizeof(tsDeviceDesc), &u16ByteRead);

    /*Load the IAS Zone Server attributes from EEPROM */
    eStatusReload = eLoadIASZoneServerAttributesFromEEPROM();
    /* Initialise ZBPro stack */
    ZPS_vAplSecSetInitialSecurityState(ZPS_ZDO_NO_NETWORK_KEY, (uint8 *)&s_au8LnkKeyArray, 0x00, ZPS_APS_GLOBAL_LINK_KEY);
    DBG_vPrintf(TRACE_ACE_NODE, "Set Sec state\n");

    /* Store channel mask */
    vEZ_RestoreDefaultAIBChMask();

    /* Initialize ZBPro stack */
    ZPS_eAplAfInit();
    DBG_vPrintf(TRACE_ACE_NODE, "ZPS_eAplAfInit\n");

    /*Set Save default channel mask as it is going to be manipulated */
    vEZ_SetDefaultAIBChMask();

    /*Fix the channel for testing purpose*/
    #if (defined FIX_CHANNEL)
        DBG_vPrintf(TRACE_ACE_NODE,"\nCurrent Channel = 0x%08x\n",ZPS_psAplAibGetAib()->apsChannelMask);
        ZPS_eAplAibSetApsChannelMask(1<<FIX_CHANNEL);
        DBG_vPrintf(TRACE_ACE_NODE,"\nCurrent Channel = 0x%08x\n",ZPS_psAplAibGetAib()->apsChannelMask);
    #endif

    APP_ZCL_vInitialise();
    /*Copy the EEPROM stuff on the shared structure */
    if (eStatusReload != PDM_E_STATUS_OK)
    {
        vSaveIASZoneAttributes(ACE_ENDPOINT_ID);
    }
    else
    {
        vLoadIASZoneAttributes(ACE_ENDPOINT_ID);
    }

    /* If the device state has been restored from flash, re-start the stack
     * and set the application running again.
     */
    if (sDeviceDesc.eNodeState == E_RUNNING)
    {
        app_vRestartNode();
        sDeviceDesc.eNodeState = E_NFN_START;
    }
    else
    {
        app_vStartNodeFactoryNew();
    }

    #ifdef PDM_EEPROM
    /*
     * The functions u8PDM_CalculateFileSystemCapacity and u8PDM_GetFileSystemOccupancy
     * may be called at any time to monitor space available in  the eeprom
     */
        DBG_vPrintf(TRACE_ACE_NODE, "\r\nPDM: Capacity %d\n", u8PDM_CalculateFileSystemCapacity() );
        DBG_vPrintf(TRACE_ACE_NODE, "\r\nPDM: Occupancy %d\n", u8PDM_GetFileSystemOccupancy() );
    #endif

    OS_eStartSWTimer(APP_IndicatorTimer, APP_TIME_MS(250), NULL);
    OS_eActivateTask(APP_ZHA_ACE_Task);

}

/****************************************************************************
 *
 * NAME: vStartFastPolling
 *
 * DESCRIPTION:
 * Set fast poll time
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/

PUBLIC void vStartFastPolling(uint8 u8Seconds)
{
    /* Fast poll is every 100ms, so times by 10 */
    u16FastPoll = 10*u8Seconds;
}

/****************************************************************************
 *
 * NAME: APP_ZHA_ACE_Task
 *
 * DESCRIPTION:
 * Task that handles the application related functionality
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_ZHA_ACE_Task)
{
    APP_tsEvent sAppEvent;
    ZPS_tsAfEvent sStackEvent;
    sStackEvent.eType = ZPS_EVENT_NONE;
    sAppEvent.eType = APP_E_EVENT_NONE;
    /*Collect the application events*/
    if (OS_eCollectMessage(APP_msgEvents, &sAppEvent) == OS_E_OK)
    {

    }
    /*Collect stack Events */
    else if ( OS_eCollectMessage(APP_msgZpsEvents, &sStackEvent) == OS_E_OK)
    {
        #ifdef DEBUG_ACE_NODE
            vDisplayStackEvent(sStackEvent);
        #endif


        /*******************************************************************************/

        /* Mgmt Leave Received */
        if( ZPS_EVENT_NWK_LEAVE_INDICATION == sStackEvent.eType )
        {
            if( sStackEvent.uEvent.sNwkLeaveIndicationEvent.u64ExtAddr == 0 )
            {
                DBG_vPrintf(TRACE_ACE_NODE, "ZDO Leave\n" );
                PDM_vDeleteAllDataRecords();
                vAHI_SwReset();
            }
        }
        /*******************************************************************************/
    }

    /* Handle events depending on node state */
    switch (sDeviceDesc.eNodeState)
    {
        case E_STARTUP:
            vHandleJoinAndRejoinNWK(&sStackEvent,E_EZ_JOIN);
            break;

        case E_NFN_START:
            sDeviceDesc.eNodeState = E_RUNNING;
            if (sAppEvent.eType == APP_E_EVENT_BUTTON_DOWN)
            {
                /* Dummy Button read to avoid reading KEY_16 on button intialize on power on reset */
                switch (sAppEvent.uEvent.sButton.u8Button)
                {
                    default:
                        break;
                }
            }
            break;

        case E_REJOINING:
            vHandleJoinAndRejoinNWK(&sStackEvent,E_EZ_REJOIN);
            DBG_vPrintf(TRACE_ACE_NODE, "In E_REJOIN - Kick off Tick Timer \n");
            OS_eStartSWTimer(APP_TickTimer, ZCL_TICK_TIME, NULL);
            vHandleAppEvent( sAppEvent );
            break;

        case E_RUNNING:
            DBG_vPrintf(TRACE_ACE_NODE, "E_RUNNING\r\n");
            if (sStackEvent.eType == ZPS_EVENT_NWK_FAILED_TO_JOIN)
            {
                DBG_vPrintf(TRACE_ACE_NODE, "Start join failed tmr 1000\n");
                vStopAllTimers();
                OS_eStartSWTimer(APP_TickTimer, ZCL_TICK_TIME, NULL);
                vStartStopTimer( APP_JoinTimer, APP_TIME_MS(1000),(uint8*)&(sDeviceDesc.eNodeState),E_REJOINING );
                DBG_vPrintf(TRACE_ACE_NODE, "failed join running %02x\n",sStackEvent.uEvent.sNwkJoinFailedEvent.u8Status );
            }
            /* Handle joined as end device event */
            if(ZPS_EVENT_NWK_JOINED_AS_ENDDEVICE  == sStackEvent.eType)
            {
                DBG_vPrintf(TRACE_ACE_NODE, "Joined as ED\r\n");
                /* As just rejoined so start ping time from here again */
                bPingSent = FALSE;
                vResetPingTime();
            }
            if (sStackEvent.eType == ZPS_EVENT_NWK_LEAVE_CONFIRM) {
                if (sStackEvent.uEvent.sNwkLeaveConfirmEvent.u64ExtAddr == 0) {
                    /* we left let commissioning task know */

                    /* reset app data and restart */
                    PDM_vDeleteAllDataRecords();
                    /* force a restart */
                    vAHI_SwReset();
                }
            }

            if((ZPS_EVENT_APS_DATA_INDICATION == sStackEvent.eType) &&
                (0 == sStackEvent.uEvent.sApsDataIndEvent.u8DstEndpoint))
            {

            }
            #ifdef SLEEP_ENABLE
                if (ZPS_EVENT_NWK_POLL_CONFIRM == sStackEvent.eType)
                {
                    if (MAC_ENUM_SUCCESS == sStackEvent.uEvent.sNwkPollConfirmEvent.u8Status)
                    {
                        /* If data pending continue poll */
                        u8PollCount = POLL_MINIMUM_RATE_IN_SECS;
                    }
                    else if (MAC_ENUM_NO_DATA == sStackEvent.uEvent.sNwkPollConfirmEvent.u8Status)
                    {
                        u8PollCount = 0;
                    }
                }
            #endif
            vHandleAppEvent( sAppEvent );
            break;
        default:
            break;
    }

    /*
     * Global clean up to make sure any PDUs have been freed
     */
    if (sStackEvent.eType == ZPS_EVENT_APS_DATA_INDICATION)
    {
        PDUM_eAPduFreeAPduInstance(sStackEvent.uEvent.sApsDataIndEvent.hAPduInst);
    }
}
/****************************************************************************
 *
 * NAME: vHandleJoinAndRejoinNWK
 *
 * DESCRIPTION:
 * Handles the Start UP events
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleJoinAndRejoinNWK( ZPS_tsAfEvent *pZPSevent,teEZ_JoinAction eJoinAction  )
{
    teEZ_State ezState;
    teCLD_IASZone_State   eZoneState;
    vSetIASDeviceState(E_IAS_DEV_STATE_NOT_JOINED);

    /* Start Indication Timer */
    if (OS_eGetSWTimerStatus(APP_IndicatorTimer) != OS_E_SWTIMER_RUNNING)
    {
        OS_eStartSWTimer(APP_IndicatorTimer, APP_TIME_MS(250), NULL);
    }

    /*Call The EZ mode Handler passing the events*/
    vEZ_EZModeNWKJoinHandler(pZPSevent,eJoinAction);
    ezState = eEZ_GetJoinState();
    DBG_vPrintf(TRACE_ACE_NODE, "EZ_STATE\%x r\n", ezState);

    if(ezState == E_EZ_DEVICE_IN_NETWORK)
    {
        if(eJoinAction == E_EZ_REJOIN)
        {
            /* Go to the state before rejoining was triggered */
            eZCL_ReadLocalAttributeValue(
                                     ACE_ENDPOINT_ID,                           /*uint8                      u8SrcEndpoint,             */
                                     SECURITY_AND_SAFETY_CLUSTER_ID_IASZONE,    /*uint16                     u16ClusterId,              */
                                     TRUE,                                      /*bool                       bIsServerClusterInstance,  */
                                     FALSE,                                     /*bool                       bManufacturerSpecific,     */
                                     FALSE,                                     /*bool_t                     bIsClientAttribute,        */
                                     E_CLD_IASZONE_ATTR_ID_ZONE_STATE,          /*uint16                     u16AttributeId,            */
                                     &eZoneState);

            if(eZoneState == E_CLD_IASZONE_STATE_ENROLLED)
            {
                vSetIASDeviceState(E_IAS_DEV_STATE_ENROLLED);
            }
            else
            {
                vSetIASDeviceState(E_IAS_DEV_STATE_JOINED);
            }
        }
        else
        {
            vSetIASDeviceState(E_IAS_DEV_STATE_JOINED);
        }
        DBG_vPrintf(TRACE_ACE_NODE, "HA EZMode EVT: E_EZ_SETUP_DEVICE_IN_NETWORK \n");
        vStartStopTimer( APP_JoinTimer, APP_TIME_MS(500),(uint8*)&(sDeviceDesc.eNodeState),E_RUNNING );

        u16GroupId=ZPS_u16AplZdoGetNwkAddr();
        PDM_eSaveRecordData( PDM_ID_APP_ACE, &sDeviceDesc,sizeof(tsDeviceDesc));
        ZPS_vSaveAllZpsRecords();
        /* Start 1 seconds polling */
        OS_eStartSWTimer(APP_PollTimer, POLL_TIME, NULL);
        OS_eStartSWTimer(APP_TickTimer, ZCL_TICK_TIME, NULL);
    }

}
#ifdef SLEEP_ENABLE
#ifdef DEEP_SLEEP_ENABLE
/****************************************************************************
 *
 * NAME: vLoadDeepSleepTimer
 *
 * DESCRIPTION:
 * Loads the deep sleep time
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vLoadDeepSleepTimer(uint8 u8SleepTime)
{
    u8DeepSleepTime = u8SleepTime;
}

/****************************************************************************
 *
 * NAME: vSetUpWakeUpConditions
 *
 * DESCRIPTION:
 *
 * Set up the wake up inputs while going to deep sleep
 *
 *
 ****************************************************************************/
PUBLIC void vSetUpWakeUpConditions(void)
{
    if (bDeepSleep)
    {
        APP_vSetLeds(0);                    /* ensure leds are off */
        u32AHI_DioWakeStatus();             /* clear interrupts */

        vAHI_DioSetDirection(POWER_BTN,0); /* Set Power Button(DIO0) as Input  */
        vAHI_DioWakeEdge(0,POWER_BTN);     /* Set wake up as DIO Falling Edge  */
        vAHI_DioWakeEnable(POWER_BTN,0);   /* Enable Wake up DIO Power Button  */
    }
}
#endif

/****************************************************************************
 *
 * NAME: vLoadKeepAliveTime
 *
 * DESCRIPTION:
 * Loads the keep alive timer based on the right conditions.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vLoadKeepAliveTime(uint8 u8TimeInSec)
{
    u8KeepAliveTime=u8TimeInSec;
    OS_eStopSWTimer(APP_PollTimer);
    OS_eStartSWTimer(APP_PollTimer,POLL_TIME, NULL);
    OS_eStopSWTimer(APP_TickTimer);
    OS_eStartSWTimer(APP_TickTimer, ZCL_TICK_TIME, NULL);
    if(sDeviceDesc.eNodeState == E_REJOINING)
        vStartStopTimer( APP_JoinTimer, APP_TIME_MS(1000),(uint8*)&(sDeviceDesc.eNodeState),E_REJOINING );
}

/****************************************************************************
 *
 * NAME: bWatingToSleep
 *
 * DESCRIPTION:
 * Gets the status if the module is waiting for sleep.
 *
 * RETURNS:
 * bool
 *
 ****************************************************************************/
PUBLIC bool bWatingToSleep(void)
{
    if (0 == u8KeepAliveTime)
        return TRUE;
    else
        return FALSE;
}

/****************************************************************************
 *
 * NAME: vUpdateKeepAliveTimer
 *
 * DESCRIPTION:
 * Updates the Keep Alive time at 1 sec call from the tick timer that served ZCL as well.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vUpdateKeepAliveTimer(void)
{
    if( u8KeepAliveTime > 0 )
    {
        u8KeepAliveTime--;
        DBG_vPrintf(TRACE_ACE_NODE,"\n KeepAliveTime = %d \n",u8KeepAliveTime);
    }
    else
    {

        vStopAllTimers();

        DBG_vPrintf(TRACE_ACE_NODE,"\n Activity %d, KeepAliveTime = %d \n",PWRM_u16GetActivityCount(),u8KeepAliveTime);
        #ifdef DEEP_SLEEP_ENABLE
            if(u8DeepSleepTime > 0 )
            {
                u8DeepSleepTime--;
                PWRM_eScheduleActivity(&sWake, SLEEP_DURATION_MS*SLEEP_TIMER_TICKS_PER_MS , vWakeCallBack);
            }
            else
            {
                PWRM_vInit(E_AHI_SLEEP_DEEP);
                bDeepSleep=TRUE;
            }
        #else
            PWRM_teStatus eStatus = PWRM_eScheduleActivity(&sWake, SLEEP_DURATION_MS*SLEEP_TIMER_TICKS_PER_MS , vWakeCallBack);
            DBG_vPrintf(TRACE_ACE_NODE,"\nSleep Status = %d\n",eStatus);
            APP_vSetLeds(E_SHIFT_0);
        #endif
    }
}
#endif

/****************************************************************************
 *
 * NAME: vHandleAppEvent
 *
 * DESCRIPTION:
 * Function to handle the app event - buttons
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleAppEvent( APP_tsEvent sAppEvent )
{
    switch(sAppEvent.eType)
    {
        case APP_E_EVENT_BUTTON_DOWN:

            OS_eStartSWTimer(APP_IndicatorTimer, APP_TIME_MS(100), NULL);
            APP_vHandleKeyPress(sAppEvent.uEvent.sButton.u8Button);

        #ifdef SLEEP_ENABLE
            vReloadSleepTimers();
        #endif

        break;

        case APP_E_EVENT_BUTTON_UP:
        break;

        default:
        break;
    }
}
/****************************************************************************
 *
 * NAME: APP_vHandleKeyPress
 *
 * Button map is as follow, please update as changes are made:
 *
 * +--------------------+ +--------------------+ +--------------------+ +--------------------+
 * |0 Emergency         | |0 Zone ID 0         | |0 Zone ID 1         | |0 Select Arm Mode   |
 * |1                   | |1                   | |1                   | |1                   |
 * |2                   | |2                   | |2                   | |2                   |
 * |3                   | |3                   | |3                   | |3                   |
 * |                    | |                    | |                    | |                    |
 * |        1(+)        | |       2(1)         | |       3(2)         | |       4(|)         |
 * +--------------------+ +--------------------+ +--------------------+ +--------------------+
 * +--------------------+ +--------------------+ +--------------------+ +--------------------+
 * |0 Fire              | |0 Zone ID 2         | |0 Zone ID 3         | |0 Disarm            |
 * |1                   | |1                   | |1                   | |1                   |
 * |2                   | |2                   | |2                   | |2                   |
 * |3                   | |3                   | |3                   | |3                   |
 * |                    | |                    | |                    | |                    |
 * |       5(-)         | |       6(3)         | |        7(4)        | |        8(O)        |
 * +--------------------+ +--------------------+ +--------------------+ +--------------------+
 * +--------------------+ +--------------------+ +--------------------+ +--------------------+
 * |0 Zone ID 4         | |0 Zone ID 5         | |0 Zone ID 6         | |0 Zone ID 7         |
 * |1 Arm all           | |1 Arm Day           | |1 Arm Night         | |1                   |
 * |2                   | |2                   | |2                   | |2                   |
 * |3                   | |3                   | |3                   | |3                   |
 * |                    | |                    | |                    | |                    |
 * |        9(A)        | |       10(B)        | |       11(C)        | |       12(D)        |
 * +--------------------+ +--------------------+ +--------------------+ +--------------------+
 * +--------------------+ +--------------------+ +--------------------+ +--------------------+
 * |0                   | |0 Zone ID map       | |0 Panic             | |0 Get Bypassed list |
 * |1   Send zone Info  | |1                   | |1 Get Panel Status  | |1 Get Zone status   |
 * |2                   | |2                   | |2                   | |2 Bypass            |
 * |3                   | |3                   | |3                   | |3                   |
 * |                    | |                    | |                    | |                    |
 * |       13(*)        | |       14(?)        | |       15(>)        | |      16(#)         |
 * +--------------------+ +--------------------+ +--------------------+ +--------------------+
 * Note1: FactoryNew(erases persistent data from the self(remote control unit))
 *        KeySequence: '*' '+' '-'
 *
 * DESCRIPTION:
 * Handles key presses
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void APP_vHandleKeyPress(teUserKeyCodes eKeyCode)
{
    switch (eKeyCode)
    {
        case KEY_1: if(eCommandState == E_CMD_STATE_START_ERASE_PDM)
                        eCommandState = E_CMD_STATE_ERASE_PDM;
                    else
                        APP_ZCL_vSendEmergency();
                    break;
        case KEY_2: /* Zone ID 0 selected */
                    if(eCommandState == E_CMD_BYPASS_STATE_SELECT_ZONES){
                        au8ZoneIDList[u8SelZoneID] = 0;
                        u8SelZoneID++;
                    }
                    break;
        case KEY_3: /* Zone ID 1 selected */
                    if(eCommandState == E_CMD_BYPASS_STATE_SELECT_ZONES){
                        au8ZoneIDList[u8SelZoneID] = 1;
                        u8SelZoneID++;
                    }
                    break;
        case KEY_4: /* Arming the system */
                    eCommandState = E_CMD_ARM_STATE_SELECT_MODE;
                    break;
        case KEY_5: if(eCommandState == E_CMD_STATE_ERASE_PDM)
                    {
                         DBG_vPrintf(TRACE_ACE_NODE,"Deleting the PDM\n");
                         PDM_vDeleteAllDataRecords();
                         vAHI_SwReset();
                         eCommandState = E_CMD_STATE_IDLE;
                    }else{
                        APP_ZCL_vSendFire();
                    }
                    break;
        case KEY_6: /* Zone ID 2 selected */
                    if(eCommandState == E_CMD_BYPASS_STATE_SELECT_ZONES){
                        au8ZoneIDList[u8SelZoneID] = 2;
                        u8SelZoneID++;
                    }
                    break;
        case KEY_7: /* Zone ID 3 selected */
                    if(eCommandState == E_CMD_BYPASS_STATE_SELECT_ZONES){
                        au8ZoneIDList[u8SelZoneID] = 3;
                        u8SelZoneID++;
                    }
                    break;
        case KEY_8: /* Disarm Mode */
                    u8ArmMode = E_CLD_IASACE_ARM_MODE_DISARM;
                    APP_ZCL_vSendHAArm();
                    break;
        case KEY_9:
                    if(eCommandState == E_CMD_ARM_STATE_SELECT_MODE)
                    {
                        /* Arm All Zones */
                        u8ArmMode = E_CLD_IASACE_ARM_MODE_ARM_ALL_ZONES;
                        APP_ZCL_vSendHAArm();
                    }
                    else if(eCommandState == E_CMD_BYPASS_STATE_SELECT_ZONES)
                    {
                        au8ZoneIDList[u8SelZoneID] = 4;
                        u8SelZoneID++;
                    }
                    break;
       case KEY_10:
                    if(eCommandState == E_CMD_ARM_STATE_SELECT_MODE)
                    {
                        /* Arm Home Zones */
                        u8ArmMode = E_CLD_IASACE_ARM_MODE_ARM_DAY_HOME_ZONES_ONLY;
                        APP_ZCL_vSendHAArm();
                    }
                    else if(eCommandState == E_CMD_BYPASS_STATE_SELECT_ZONES)
                    {
                        au8ZoneIDList[u8SelZoneID] = 5;
                        u8SelZoneID++;
                    }
                    break;
        case KEY_11:
                    if(eCommandState == E_CMD_ARM_STATE_SELECT_MODE)
                    {
                        /* Arm Night Zones */
                        u8ArmMode = E_CLD_IASACE_ARM_MODE_ARM_NIGHT_SLEEP_ZONES_ONLY;
                        APP_ZCL_vSendHAArm();
                    }
                    else if(eCommandState == E_CMD_BYPASS_STATE_SELECT_ZONES)
                    {
                        au8ZoneIDList[u8SelZoneID] = 6;
                        u8SelZoneID++;
                    }
                    break;
        case KEY_12:
                    if(eCommandState == E_CMD_BYPASS_STATE_SELECT_ZONES)
                    {
                        au8ZoneIDList[u8SelZoneID] = 7;
                        u8SelZoneID++;
                    }
                     break;
        case KEY_13: if(eCommandState == E_CMD_STATE_GET_ZONE_RELATED_INFO)
                     {
                         eCommandState = E_CMD_STATE_SEND_GET_ZONE_INFO;
                         APP_ZCL_vSendHAGetZoneIDMap();
                     }else{
                         eCommandState = E_CMD_STATE_START_ERASE_PDM;
                     }
                     break;
        case KEY_14: eCommandState = E_CMD_STATE_GET_ZONE_RELATED_INFO;
                     break;
        case KEY_15: if(eCommandState == E_CMD_STATE_GET_ZONE_RELATED_INFO)
                     {
                        APP_ZCL_vSendHAGetPanelStatus();
                     }else{
                        APP_ZCL_vSendPanic();
                     }
                     break;
        case KEY_16: /* Bypass */
                    if(eCommandState == E_CMD_STATE_GET_ZONE_RELATED_INFO)
                    {
                        eCommandState = E_CMD_STATE_SEND_GET_ZONE_STATUS_SEND;
                        APP_ZCL_vSendHAGetZoneIDMap();
                    }else if(eCommandState == E_CMD_STATE_IDLE)
                    {
                        u8SelZoneID = 0;
                        memset(au8ZoneIDList,0,8);
                        eCommandState = E_CMD_BYPASS_STATE_SELECT_ZONES;
                    }else{
                        if(u8SelZoneID == 0)
                            APP_ZCL_vSendHAGetBypassedList();
                        else
                            APP_ZCL_vSendHABypass();
                    }
                     break;
        default : /* Default */
                  break;
    }

     if(u8SelZoneID > CLD_IASACE_ZONE_TABLE_SIZE)
         u8SelZoneID = 0;

     /* Update LED's to reflect key press */
     APP_vSetLeds(E_SHIFT_1);
     APP_vBlinkLeds(E_SHIFT_1);

}

/****************************************************************************
 *
 * NAME: vStopAllTimers
 *
 * DESCRIPTION:
 * Stops all the timers
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vStopAllTimers(void)
{
    vStopTimer(APP_LedBlinkTimer);
    vStopTimer(APP_JoinFailedTimer);
    vStopTimer(APP_PollTimer);
    vStopTimer(APP_TickTimer);
    vStopTimer(APP_JoinTimer);
    vStopTimer(APP_BackOffTimer);
    vStopTimer(App_PingTimer);
    vStopTimer(APP_IndicatorTimer);
}

/****************************************************************************
 *
 * NAME: vStopTimer
 *
 * DESCRIPTION:
 * Stops the timer
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vStopTimer(OS_thSWTimer hSwTimer)
{
    if(OS_eGetSWTimerStatus(hSwTimer) != OS_E_SWTIMER_STOPPED)
        OS_eStopSWTimer(hSwTimer);
}

OS_TASK(APP_SleepJoinFailed)
{
#ifdef SLEEP_ENABLE
    {
        OS_eStopSWTimer(APP_JoinFailedTimer);
        DBG_vPrintf(TRACE_ACE_NODE,"SleepJoinFailed u8RunningWithJoinFailedTime - %d \n", u8RunningWithJoinFailedTime);

        if(u8RunningWithJoinFailedTime==0)
        {
            #ifdef DEEP_SLEEP_ENABLE
            if (u8DeepSleepTime) {
                DBG_vPrintf(TRACE_ACE_NODE, "SleepJoinFailed u8DeepSleepTime %d \n", u8RunningWithJoinFailedTime, u8DeepSleepTime);
                OS_eStartSWTimer(APP_JoinFailedTimer, APP_TIME_MS(1030), NULL);
                u8DeepSleepTime--;
            }
            else
            {

                vStopAllTimers();
                DBG_vPrintf(TRACE_ACE_NODE, "join failed: go deep... %d\n", PWRM_u16GetActivityCount());
                PWRM_vInit(E_AHI_SLEEP_DEEP);
                bDeepSleep = TRUE;
            }
            #else
            vUpdateKeepAliveTimer();
            #endif

        }
        else
        {
            u8RunningWithJoinFailedTime--;
            DBG_vPrintf(TRACE_ACE_NODE, "join failed: try a rejoin\n");
            OS_eStartSWTimer(APP_JoinFailedTimer, APP_TIME_MS(1000), NULL);
        }
    }
#endif
}
/****************************************************************************
 *
 * NAME: APP_SleepTask
 *
 * DESCRIPTION:
 * Os Task activated by the wake up event to manage sleep
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/

OS_TASK(APP_SleepTask)
{
#ifdef SLEEP_ENABLE
    /* Restart the keyboard scanning timer as we've come up through */
    /* warm start via the Power Manager if we get here              */

    vConfigureScanTimer();

    DBG_vPrintf(TRACE_SLEEP, "SleepTask: start poll timer\n");
    vIncrementPingTime(1);
    if(!bPingParent())
    {
        u8PingCount = 0;
        OS_eStartSWTimer(APP_TickTimer, APP_TIME_MS(200), NULL);
    }
    else
    {
        /* start ping timer and keep alive for atleast 3 seconds*/
        OS_eStartSWTimer(App_PingTimer,APP_TIME_MS(1000),NULL);
        vLoadKeepAliveTime(3);
        u8PollCount = POLL_MINIMUM_RATE_IN_SECS;
    }
    if(u8PollCount >= (POLL_MINIMUM_RATE_IN_SECS - 1))
    {
        ZPS_eAplZdoPoll();
        u8PollCount = 0;
    }
    else
    {
        u8PollCount++;
    }

#endif

}

#ifdef SLEEP_ENABLE
/****************************************************************************
 *
 * NAME: vWakeCallBack
 *
 * DESCRIPTION:
 * Wake up call back called upon wake up by the schedule activity event.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vWakeCallBack(void)
{
    DBG_vPrintf(TRACE_ACE_NODE, "vWakeCallBack\n");
}
#endif

/****************************************************************************
 *
 * NAME: APP_PollTask
 *
 * DESCRIPTION:
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_PollTask)
{
    uint32 u32PollPeriod = POLL_TIME;


#ifdef SLEEP_ENABLE
    if(!bWatingToSleep())
#endif
    {
        if( u16FastPoll )
        {
            u16FastPoll--;
            u32PollPeriod = POLL_TIME_FAST;
            #ifdef SLEEP_ENABLE
                vReloadSleepTimers();
            #endif
        }
        OS_eStopSWTimer(APP_PollTimer);
        OS_eStartSWTimer(APP_PollTimer, u32PollPeriod, NULL);

        ZPS_teStatus u8PStatus;

        u8PStatus = ZPS_eAplZdoPoll();
        if( u8PStatus )
        {
            DBG_vPrintf(TRACE_ACE_NODE, "\nPoll Failed \n", u8PStatus );
        }
    }
}

#ifdef SLEEP_ENABLE
/****************************************************************************
*
* NAME: vReloadSleepTimers
*
* DESCRIPTION:
* reloads boththe timers on identify
*
* RETURNS:
* void
*
****************************************************************************/
PUBLIC void vReloadSleepTimers(void)
{

    vLoadKeepAliveTime(KEEP_ALIVETIME);
    #ifdef DEEP_SLEEP_ENABLE
        vLoadDeepSleepTimer(DEEP_SLEEP_TIME);
    #endif
}
#endif
/****************************************************************************
 *
 * NAME: app_vRestartNode
 *
 * DESCRIPTION:
 * Start the Restart the ZigBee Stack after a context restore from
 * the EEPROM/Flash
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void app_vRestartNode (void)
{
    ZPS_tsNwkNib * thisNib;

    /* Get the NWK Handle to have the NWK address from local node and use it as GroupId*/
    thisNib = ZPS_psNwkNibGetHandle(ZPS_pvAplZdoGetNwkHandle());

    /* The node is in running state indicates that
     * the EZ Mode state is as E_EZ_SETUP_COMPLETED*/
    eEZ_UpdateEZState(E_EZ_DEVICE_IN_NETWORK);

    DBG_vPrintf(TRACE_ACE_NODE, "\nNon Factory New Start");

    ZPS_vSaveAllZpsRecords();
    u16GroupId = thisNib->sPersist.u16NwkAddr;
    /* Start 1 seconds polling */
    OS_eStartSWTimer(APP_PollTimer,POLL_TIME, NULL);

    /*Rejoin NWK when coming from reset.*/
    ZPS_eAplZdoRejoinNetwork(FALSE);
}


/****************************************************************************
 *
 * NAME: app_vStartNodeFactoryNew
 *
 * DESCRIPTION:
 * Start the ZigBee Stack for the first ever Time.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void app_vStartNodeFactoryNew(void)
{
    eEZ_UpdateEZState(E_EZ_START);

    /* Stay awake for joining */
    DBG_vPrintf(TRACE_ACE_NODE, "\nFactory New Start");
    vStartStopTimer( APP_JoinTimer, APP_TIME_MS(1000),(uint8*)&(sDeviceDesc.eNodeState),E_STARTUP );
}

/****************************************************************************
 *
 * NAME: APP_PingTimerTask
 *
 * DESCRIPTION:
 * Ping Timer task.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_PingTimerTask)
{
    OS_eStopSWTimer(App_PingTimer);
    if( (TRUE == bPingSent) && (FALSE == bPingRespRcvd))
    {
        if(u8PingCount >= MAX_PINGS_NO_RSP)
        {
            ZPS_eAplZdoRejoinNetwork(FALSE);
            bPingSent = FALSE;
        }
        else
        {
            if(u8PingCount < (MAX_PINGS_NO_RSP - 2))
            {
                vIncrementPingTime(PING_PARENT_TIME);
                bPingParent();
                vLoadKeepAliveTime(2);
                OS_eStartSWTimer(App_PingTimer,APP_TIME_MS(1000),NULL);
            }
            else
            {
                OS_eStartSWTimer(App_PingTimer,APP_TIME_MS(5000),NULL);
                vLoadKeepAliveTime(MAX_PINGS_NO_RSP + 3);
            }
           u8PingCount += 1;
        }
    }
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
