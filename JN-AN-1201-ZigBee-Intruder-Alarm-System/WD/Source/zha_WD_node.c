/*****************************************************************************
 *
 * MODULE:             JN-AN-1201
 *
 * COMPONENT:          zha_WD_node.c
 *
 * DESCRIPTION:        ZHA Demo : Stack <-> App Interaction (Implementation)
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
 * Copyright NXP B.V. 2014. All rights reserved
 *
 ***************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include "dbg.h"
#include "os.h"
#include "os_gen.h"
#include "pdum_gen.h"
#include "pdm.h"
#include "pdum_gen.h"
#include "zps_gen.h"
#include "zps_apl.h"
#include "zps_apl_aib.h"
#include "zps_nwk_sap.h"
#include "appapi.h"
#include "AppHardwareApi.h"
#include "app_common.h"
#include "app_buttons.h"

#include "app_events.h"
#include <rnd_pub.h>
#include "app_zcl_WD_task.h"
#include "zha_WD_node.h"
#include "ha.h"
#include "haEzJoin.h"


#include "app_pdm.h"
#include "app_ias_indicator.h"
#include "PDM_IDs.h"
#include "zcl_options.h"
#include "LightingBoard.h"
#include "DriverPiezo.h"
#include "app_zbp_utilities.h"
#include "zcl_common.h"

#ifdef CLD_GROUPS
#include "groups.h"
#include "Groups_internal.h"
#endif

#include "app_ias_save.h"
#include "app_ias_enroll_req.h"
#include "zha_WD_generation.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifndef DEBUG_WD_NODE
    #define TRACE_WD_NODE   FALSE
#else
    #define TRACE_WD_NODE   TRUE
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void app_vRestartNode (void);
PRIVATE void app_vStartNodeFactoryNew(void);
PRIVATE void vHandleStartUp( ZPS_tsAfEvent *pZPSevent );
PRIVATE void vHandleRunningEvent(ZPS_tsAfEvent *sStackEvent);

PRIVATE void vDeletePDMOnButtonPress(uint8 u8ButtonDIO);
PRIVATE void vInitLEDs(void);
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
PUBLIC    uint8 s_au8LnkKeyArray[16] = {0x5a, 0x69, 0x67, 0x42, 0x65, 0x65, 0x41, 0x6c,
        0x6c, 0x69, 0x61, 0x6e, 0x63, 0x65, 0x30, 0x39};

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
tsDeviceDesc sDeviceDesc;

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
    PDM_teStatus eStatus;
    PDM_teStatus eStatusReload;

    DBG_vPrintf(TRACE_WD_NODE, "\nAPP_vInitialiseNode*");

   /* If required, at this point delete the network context from flash, perhaps upon some condition
    * For example, check if a button is being held down at reset, and if so request the Persistent
    * Data Manager to delete all its records:
    * e.g. bDeleteRecords = vCheckButtons();
    * Alternatively, always call PDM_vDeleteAllDataRecords() if context saving is not required.
    */
   APP_bButtonInitialise();
   vDeletePDMOnButtonPress(APP_BUTTONS_BUTTON_1);

    /* Restore device state previously saved to flash */
    eStatus = eRestoreDeviceState();
    DBG_vPrintf(TRACE_WD_NODE, "\neRestoreDeviceState %d",eStatus);
    /*Load the IAS Zone Server attributes from EEPROM */
    eStatusReload = eLoadIASZoneServerAttributesFromEEPROM();

    ZPS_vAplSecSetInitialSecurityState(ZPS_ZDO_NO_NETWORK_KEY, (uint8 *)&s_au8LnkKeyArray, 0x00, ZPS_APS_GLOBAL_LINK_KEY);

    /* Store channel mask */
    vEZ_RestoreDefaultAIBChMask();

    /* Initialise ZBPro stack */
    ZPS_eAplAfInit();

    /*Set Save default channel mask as it is going to be manipulated */
    vEZ_SetDefaultAIBChMask();

    /*Fix the channel for testing purpose*/
    #if (defined FIX_CHANNEL)
       DBG_vPrintf(TRACE_WD_NODE,"\nCurrent Channel = 0x%08x\n",ZPS_psAplAibGetAib()->apsChannelMask);
       ZPS_eAplAibSetApsChannelMask(1<<FIX_CHANNEL);
       DBG_vPrintf(TRACE_WD_NODE,"\nCurrent Channel = 0x%08x\n",ZPS_psAplAibGetAib()->apsChannelMask);
    #endif

    APP_ZCL_vInitialise();
    /*Copy the EEPROM stuff on the shared structure */
    if (eStatusReload != PDM_E_STATUS_OK)
    {
        vSaveIASZoneAttributes(1);
    }
    else
    {
        vLoadIASZoneAttributes(1);
    }

    if (E_RUNNING == eGetNodeState())
    {
        app_vRestartNode();
    }
    else
    {
        app_vStartNodeFactoryNew();
    }

    /*Initialize LEDs*/
    vInitLEDs();
    vInitIndicationLEDs();
    /* Initialise piezo sounder */
    DriverPiezo_vInit(E_AHI_TIMER_0);

    OS_eStartSWTimer(APP_IndicatorTimer, APP_TIME_MS(250), NULL);
}

/****************************************************************************
 *
 * NAME: APP_ZPR_WD_Task
 *
 * DESCRIPTION:
 * Task that handles application related functions
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_ZPR_WD_Task)
{
    ZPS_tsAfEvent sStackEvent = {0};
    APP_tsEvent sAppButtonEvent = {0};

    /* The button event for the SSL bulbs */
    if (OS_eCollectMessage(APP_msgEvents, &sAppButtonEvent) == OS_E_OK)
    {
        DBG_vPrintf(TRACE_WD_NODE,"\n\nButton Q\n EventType = %d\n", sAppButtonEvent.eType);
        DBG_vPrintf(TRACE_WD_NODE," Button = %d\n", sAppButtonEvent.uEvent.sButton.u8Button);
        if(sAppButtonEvent.eType == APP_E_EVENT_BUTTON_DOWN)
            vSendEnrollReq(1);
    }
    /*Collect stack Events */
    else if ( OS_eCollectMessage(APP_msgZpsEvents, &sStackEvent) == OS_E_OK)
    {

        #ifdef DEBUG_WD_NODE
            vDisplayStackEvent( sStackEvent );
        #endif
    }


    /* Handle events depending on node state */
    switch (sDeviceDesc.eNodeState)
    {
        case E_STARTUP:
            DBG_vPrintf(TRACE_WD_NODE, "\nE_STARTUP" );
            vHandleStartUp(&sStackEvent);
            break;

        case E_RUNNING:
            DBG_vPrintf(TRACE_WD_NODE, "E_RUNNING\r\n");
            vHandleRunningEvent(&sStackEvent);
            break;

        case E_LEAVE_WAIT:
            DBG_vPrintf(TRACE_WD_NODE, "E_LEAVE_WAIT\r\n");
            vStartStopTimer( APP_StartUPTimer, APP_TIME_MS(500),(uint8*)&(sDeviceDesc.eNodeState),E_LEAVE_RESET );
            break;

        case E_LEAVE_RESET:
            DBG_vPrintf(TRACE_WD_NODE, "E_LEAVE_RESET\r\n");
            PDM_vDeleteAllDataRecords();
            vAHI_SwReset();
            break;

        default:
            DBG_vPrintf(TRACE_WD_NODE, "ERR: Unknown State %d\n", sDeviceDesc.eNodeState );
            break;
    }

    /* Global clean up to make sure any APDUs have been freed   */
    if (sStackEvent.eType == ZPS_EVENT_APS_DATA_INDICATION)
    {
        PDUM_eAPduFreeAPduInstance(sStackEvent.uEvent.sApsDataIndEvent.hAPduInst);
    }
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
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
    /* The node is in running state indicates that
     * the EZ Mode state is as E_EZ_SETUP_DEVICE_IN_NETWORK*/
    eEZ_UpdateEZState(E_EZ_DEVICE_IN_NETWORK);

    sDeviceDesc.eNodeState = E_RUNNING;

    /* Store the NWK frame counter increment */
    ZPS_vSaveAllZpsRecords();

    DBG_vPrintf(TRACE_WD_NODE, "Restart Running\n");
    OS_eActivateTask(APP_ZPR_WD_Task);

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
    /* The node is in running state indicates that
     * the EZ Mode state is as E_EZ_SETUP_START*/
    eEZ_UpdateEZState(E_EZ_START);

    DBG_vPrintf(TRACE_WD_NODE, "\nRun and activate\n");
    vStartStopTimer( APP_StartUPTimer, APP_TIME_MS(500),(uint8*)&(sDeviceDesc.eNodeState),E_STARTUP );
    DBG_vPrintf(TRACE_WD_NODE, "Start Factory New\n");
}



/****************************************************************************
 *
 * NAME: vHandleStartUp
 *
 * DESCRIPTION:
 * Handles the Start UP events
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleStartUp( ZPS_tsAfEvent *pZPSevent )
{
    teEZ_State ezState;
    vSetIASDeviceState(E_IAS_DEV_STATE_NOT_JOINED);

    /*Call The EZ mode Handler passing the events*/
    vEZ_EZModeNWKJoinHandler(pZPSevent,E_EZ_JOIN);
    ezState = eEZ_GetJoinState();
    DBG_vPrintf(TRACE_WD_NODE, "EZ_STATE\%x r\n", ezState);
    if(ezState == E_EZ_DEVICE_IN_NETWORK)
    {
        vSetIASDeviceState(E_IAS_DEV_STATE_JOINED);

        DBG_vPrintf(TRACE_WD_NODE, "HA EZMode E_EZ_SETUP_DEVICE_IN_NETWORK \n");
        vStartStopTimer( APP_StartUPTimer, APP_TIME_MS(500),(uint8*)&(sDeviceDesc.eNodeState),E_RUNNING );

        vEnablePermitJoin(EZ_MODE_TIME * 60);
        PDM_eSaveRecordData( PDM_ID_APP_APP_ROUTER,
                            &sDeviceDesc,
                            sizeof(tsDeviceDesc));
        ZPS_vSaveAllZpsRecords();
    }
}


/****************************************************************************
 *
 * NAME: vHandleRunningEvent
 *
 * DESCRIPTION:
 * Handles the running events
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleRunningEvent(ZPS_tsAfEvent *psStackEvent)
{
    /*Request response event */
    if((ZPS_EVENT_APS_DATA_INDICATION == psStackEvent->eType) &&
        (0 == psStackEvent->uEvent.sApsDataIndEvent.u8DstEndpoint))
    {
    }

    /* Mgmt Leave Received */
    if( ZPS_EVENT_NWK_LEAVE_INDICATION == psStackEvent->eType )
    {
        DBG_vPrintf(TRACE_WD_NODE, "MgmtLeave\n" );
        /* add leave handling here */
    }
}
/****************************************************************************
 *
 * NAME: eGetNodeState
 *
 * DESCRIPTION:
 * returns the device state
 *
 * RETURNS:
 * teNODE_STATES
 *
 ****************************************************************************/
PUBLIC teNODE_STATES eGetNodeState(void)
{
    return sDeviceDesc.eNodeState;
}

/****************************************************************************
 *
 * NAME: vDeletePDMOnButtonPress
 *
 * DESCRIPTION:
 * PDM context clearing on button press
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vDeletePDMOnButtonPress(uint8 u8ButtonDIO)
{
    bool_t bDeleteRecords = FALSE;
    uint32 u32Buttons = u32AHI_DioReadInput() & (1 << u8ButtonDIO);
    if (u32Buttons == 0)
    {
        bDeleteRecords = TRUE;
    }
    else
    {
        bDeleteRecords = FALSE;
    }
    /* If required, at this point delete the network context from flash, perhaps upon some condition
     * For example, check if a button is being held down at reset, and if so request the Persistent
     * Data Manager to delete all its records:
     * e.g. bDeleteRecords = vCheckButtons();
     * Alternatively, always call PDM_vDeleteAllDataRecords() if context saving is not required.
     */
    if(bDeleteRecords)
    {
        DBG_vPrintf(TRACE_WD_NODE,"Deleting the PDM\n");
        PDM_vDeleteAllDataRecords();
    }
}

/****************************************************************************
 *
 * NAME: vInitLEDs
 *
 * DESCRIPTION:
 * Initialize the LEDs both for strobe indication as well as the joining
 * indicators
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vInitLEDs(void)
{
    /*Stobe Indicators*/
    bWhite_LED_Enable();
    bWhite_LED_SetLevel(0);
    bWhite_LED_Off();
}
/****************************************************************************
 *
 * NAME: vStobeIndication
 *
 * DESCRIPTION:
 * Generates the strobe while the device is in warning
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vStobeIndication(bool_t bStrobe,uint8 u8Level)
{
    bWhite_LED_SetLevel(u8Level*50);
    if(bStrobe)
    {
        bWhite_LED_On();
    }
    else
    {
        bWhite_LED_Off();
    }
}
/****************************************************************************
 *
 * NAME: vWarning
 *
 * DESCRIPTION:
 * Generates the strobe warning with different modes maps to frequency
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/

PUBLIC void vWarning(uint8 u8Mode, uint16 u16Duration,uint8 u8Strobe,uint8 u8StrobeLevel, uint8 u8StobeDutyCycle )
{
    APP_tsStartWarning sWarning ={0};
    uint16 T =0;

    if(u8Strobe)
    {
        sWarning.u16Duration = u16Duration;
        sWarning.u8Level     = u8StrobeLevel*50;
        switch (u8Mode)
        {
        case E_WARNING_BURGLAR:
            T =WARNING_BURGLAR_PERIOD;
            sWarning.u16OnTime   = ((uint16)T*u8StobeDutyCycle)/100;
            sWarning.u16OffTime  = T-sWarning.u16OnTime;
            sWarning.u32Tone     = 0x24FF3020;
            break;
        case E_WARNING_FIRE:
            T =WARNING_FIRE_PERIOD;
            sWarning.u16OnTime   = ((uint16)T*u8StobeDutyCycle)/100;
            sWarning.u16OffTime  = T-sWarning.u16OnTime;
            sWarning.u32Tone     = 0x34FF3020;
            break;
        case E_WARNING_EMERGENCY:
            T =WARNING_EMERGENCY_PERIOD;
            sWarning.u16OnTime   = ((uint16)T*u8StobeDutyCycle)/100;
            sWarning.u16OffTime  = T-sWarning.u16OnTime;
            sWarning.u32Tone     = 0x44FF3020;
            break;
        case E_WARNING_ENTRY_EXIT_DELAY:
            T =WARNING_ENTRY_EXIT_DELAY_PERIOD;
            sWarning.u16OnTime   = ((uint16)T*u8StobeDutyCycle)/100;
            sWarning.u16OffTime  = T-sWarning.u16OnTime;
            sWarning.u32Tone     = 0x10013030;
            break;
        default :
            break;
        }
    }

    vStartWarn(sWarning);

}

/****************************************************************************
 *
 * NAME: vSquawk
 *
 * DESCRIPTION:
 * Generates the squawk
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vSquawk(uint8 u8Mode, uint8 u8Strobe, uint8 u8SquawkLevel)
{
    APP_tsStartWarning sWarning ={0};

    if(u8Strobe)
    {
        sWarning.u16Duration = 500;
        sWarning.u8Level     = 50;
        switch (u8Mode)
        {
        case E_SQUAWK_ARMED:
            sWarning.u16OnTime   = 75;
            sWarning.u16OffTime  = 25;
            sWarning.u32Tone     = 0x34FF3060;
            break;
        case E_SQUAWK_DISARMED:
            sWarning.u16OnTime   = 25;
            sWarning.u16OffTime  = 75;
            sWarning.u32Tone     = 0x24FF3000;
            break;
        default :
            break;
        }
    }

    vStartWarn(sWarning);
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
