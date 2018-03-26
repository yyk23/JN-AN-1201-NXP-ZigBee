/****************************************************************************
 *
 * MODULE:             JN-AN-1201
 *
 * COMPONENT:          app_start_zone.c
 *
 * DESCRIPTION:        ZHA IAS Zone Application Initialisation and Startup
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
#include "pwrm.h"
#include "pdum_nwk.h"
#include "pdum_apl.h"
#include "pdm.h"
#include "dbg.h"
#include "dbg_uart.h"
#include "pdum_gen.h"
#include "os_gen.h"
#include "zps_gen.h"
#include "zps_apl_af.h"
#include "appapi.h"
#include "zha_ZONE_node.h"
#include "app_timer_driver.h"
#ifndef DBG_ENABLE
#include "app_uart.h"
#endif
#include "app_buttons.h"
#include "app_exceptions.h"
#include "app_pdm.h"
#include "app_zcl_ZONE_task.h"
#include "app_sleep_functions.h"
#ifdef VMS
#include "TSL2550.h"
#include "LightingBoard.h"
#include "app_ias_indicator.h"
#endif
#ifdef CSW
#include "GenericBoard.h"
#endif
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#ifndef DEBUG_START_UP
    #define TRACE_START TRUE
#else
    #define TRACE_START TRUE
#endif

#define HALT_ON_EXCEPTION   FALSE
#define RAM_HELD 2

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

PRIVATE void vInitialiseApp(void);
PRIVATE void vSetUpWakeUpConditions(void);
PRIVATE void vExtendedStatusCallBack (ZPS_teExtendedStatus eExtendedStatus);


/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern void *_stack_low_water_mark;

uint8 u8PowerMode=RAM_HELD;
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
/**
 * Power manager Callback.
 * Called just before the device is put to sleep
 */

static PWRM_DECLARE_CALLBACK_DESCRIPTOR(PreSleep);
/**
 * Power manager Callback.
 * Called just after the device wakes up from sleep
 */
static PWRM_DECLARE_CALLBACK_DESCRIPTOR(Wakeup);
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/


/****************************************************************************
 *
 * NAME: vAppMain
 *
 * DESCRIPTION:
 * Entry point for application from a cold start.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppMain(void)
{
    #if JENNIC_CHIP_FAMILY == JN516x
        /* Wait until FALSE i.e. on XTAL  - otherwise uart data will be at wrong speed */
         while (bAHI_GetClkSource() == TRUE);
         /* Now we are running on the XTAL, optimise the flash memory wait states */
         vAHI_OptimiseWaitStates();
    #endif

    /*
     * Don't use RTS/CTS pins on UART0 as they are used for buttons
     * */
    vAHI_UartSetRTSCTS(E_AHI_UART_0, FALSE);

    /*
     * Initialize the debug diagnostics module to use UART0 at 115K Baud;
     * Do not use UART 1 if LEDs are used, as it shares DIO with the LEDS
     * */


    DBG_vUartInit(DBG_E_UART_0, DBG_E_UART_BAUD_RATE_115200);


    DBG_vPrintf(TRACE_START, "\n\nAPP: Switch Power Up");


    /*
     * Initialise the stack overflow exception to trigger if the end of the
     * stack is reached. See the linker command file to adjust the allocated
     * stack size.
     */
    vAHI_SetStackOverflow(TRUE, (uint32)&_stack_low_water_mark);


    /*
     * Catch resets due to watchdog timer expiry. Comment out to harden code.
     */
    if (bAHI_WatchdogResetEvent())
    {
        DBG_vPrintf(TRACE_START, "APP: Watchdog timer has reset device!\n");
        DBG_vDumpStack();
        #if HALT_ON_EXCEPTION
            vAHI_WatchdogStop();
            while (1);
        #endif
    }

    /* initialise ROM based software modules */
    #ifndef JENNIC_MAC_MiniMacShim
        u32AppApiInit(NULL, NULL, NULL, NULL, NULL, NULL);
    #endif

    /* start the RTOS */
    OS_vStart(vInitialiseApp, vUnclaimedInterrupt, vOSError);
    DBG_vPrintf(TRACE_START, "OS started\n");
    eAppApiPlmeSet(PHY_PIB_ATTR_TX_POWER, 10);
    /* idle task commences here */
    while (TRUE)
    {
        /* Re-load the watch-dog timer. Execution must return through the idle
         * task before the CPU is suspended by the power manager. This ensures
         * that at least one task / ISR has executed with in the watchdog period
         * otherwise the system will be reset.
         */
        vAHI_WatchdogRestart();

        /*
         * suspends CPU operation when the system is idle or puts the device to
         * sleep if there are no activities in progress
         */
        PWRM_vManagePower();

    }
}


/****************************************************************************
 *
 * NAME: vAppRegisterPWRMCallbacks
 *
 * DESCRIPTION:
 * Power manager callback.
 * Called to allow the application to register  sleep and wake callbacks.
 *
 * PARAMETERS:      Name            RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
void vAppRegisterPWRMCallbacks(void)
{
    PWRM_vRegisterPreSleepCallback(PreSleep);
    PWRM_vRegisterWakeupCallback(Wakeup);
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: vInitialiseApp
 *
 * DESCRIPTION:
 * Initialises Zigbee stack, hardware and application.
 *
 *
 * RETURNS:
 * void
 ****************************************************************************/
PRIVATE void vInitialiseApp(void)
{
    /*
     * Initialise JenOS modules. Initialise Power Manager even on non-sleeping nodes
     * as it allows the device to doze when in the idle task.
     * Parameter options: E_AHI_SLEEP_OSCON_RAMON or E_AHI_SLEEP_DEEP or ...
     */

#ifdef PERIODIC_WAKE
    PWRM_vInit(E_AHI_SLEEP_OSCON_RAMON);
#else
    PWRM_vInit(E_AHI_SLEEP_OSCOFF_RAMON);
#endif

    /* Initialise the Persistent Data Manager */
    #if JENNIC_CHIP == JN5169
         PDM_eInitialise(63, NULL);//63个段，每个段64字节，可用的数量为56个字节
    #else
         PDM_eInitialise(0, NULL);
    #endif


    /* Initialise Protocol Data Unit Manager */
    PDUM_vInit();

    ZPS_vExtendedStatusSetCallback(vExtendedStatusCallBack);

    /* Initialise application */
    APP_vInitialiseNode();
    Uart_Task_Init();

}

/****************************************************************************
 *
 * NAME: vSetUpWakeUpConditions
 *
 * DESCRIPTION:
 *
 * Set up the wake up inputs while going to sleep.
 *
 * PARAMETERS:      Name            RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSetUpWakeUpConditions(void)
{

    vAHI_DioSetDirection(APP_BUTTONS_DIO_MASK,0);    /* Set as Power Button(DIO0) as Input */
    vAHI_DioWakeEdge(0,APP_BUTTONS_DIO_MASK);       /* Set the wake up DIO Edge - Falling Edge */
    vAHI_DioWakeEnable(APP_BUTTONS_DIO_MASK,0);     /* Set the Wake up DIO Power Button */
}


/****************************************************************************
 *
 * NAME: PreSleep
 *
 * DESCRIPTION:
 *
 * PreSleep call back by the power manager before the controller put into sleep.
 *
 * PARAMETERS:      Name            RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PWRM_CALLBACK(PreSleep)
{
    DBG_vPrintf(SLEEP_INFO, "\r\n PWRM_CALLBACK(PreSleep)");
    DBG_vPrintf(SLEEP_INFO,"\r\n Sleeping...\n");
    /* If the power mode is with RAM held do the following
     * else not required as the entry point will init everything*/
    if(u8PowerMode == RAM_HELD)
    {
       vAppApiSaveMacSettings();
    }
    /* Disable UART */
    vAHI_UartDisable(E_AHI_UART_0);
    vAHI_UartDisable(E_AHI_UART_1);
    /* Set up wake up input */
    vSetUpWakeUpConditions();  //睡眠之前

}
/****************************************************************************
 *
 * NAME: Wakeup
 *
 * DESCRIPTION:
 *
 * Wakeup call back by  power manager after the controller wakes up from sleep.
 *
 ****************************************************************************/
PWRM_CALLBACK(Wakeup)
{
    /*Stabilise the oscillator*/
    #if JENNIC_CHIP_FAMILY == JN516x
        /* Wait until FALSE i.e. on XTAL  - otherwise uart data will be at wrong speed */
        while (bAHI_GetClkSource() == TRUE);
        /* Now we are running on the XTAL, optimise the flash memory wait states */
        vAHI_OptimiseWaitStates();
        #ifndef PDM_EEPROM
            PDM_vWarmInitHW();
        #endif
    #endif
    /* Don't use RTS/CTS pins on UART0 as they are used for buttons */
    vAHI_UartSetRTSCTS(E_AHI_UART_0, FALSE);
    User_Uart_Init();
    DBG_vUartInit(DBG_E_UART_0, DBG_E_UART_BAUD_RATE_115200);


#ifdef VMS
    bool_t bStatus= bTSL2550_Init();
    DBG_vPrintf(TRACE_START,"bTSL2550_Init = %d\n",bStatus);
    vInitIndicationLEDs();
#endif

    DBG_vPrintf(TRACE_START, "\n PWRM_CALLBACK(Wakeup)");
    DBG_vPrintf(TRACE_START, "\nAPP: Woken up (CB)");
    DBG_vPrintf(TRACE_START, "\nAPP: Warm Waking powerStatus = 0x%x", u8AHI_PowerStatus());

    /* If the power status is OK and RAM held while sleeping
     * restore the MAC settings
     * */
    if( (u8AHI_PowerStatus()) && (u8PowerMode == RAM_HELD) )
    {
        /* Restore Mac settings (turns radio on) */
        vMAC_RestoreSettings();
        DBG_vPrintf(TRACE_START, "\nAPP: MAC settings restored");
    }

    /* Restart the OS */
    DBG_vPrintf(TRACE_START, "\nAPP: Restarting OS \n");
    OS_vRestart();  //重新启动系统，内存和EEPROM都会不变，相当于继续执行系统
}

/****************************************************************************
 *
 * NAME: vExtendedStatusCallBack
 *
 * DESCRIPTION:
 * Handles an extended error status callback
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vExtendedStatusCallBack (ZPS_teExtendedStatus eExtendedStatus)
{
    DBG_vPrintf(TRACE_START,"Extended status %x\n", eExtendedStatus);
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
