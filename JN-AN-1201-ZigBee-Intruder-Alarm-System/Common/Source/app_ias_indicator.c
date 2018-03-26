/*****************************************************************************
 *
 * MODULE:             JN-AN-1201
 *
 * COMPONENT:          app_ias_indicator.c
 *
 * DESCRIPTION:        ZHA Demo : LED Indications
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
#include "os.h"
#include "os_gen.h"
#include "dbg.h"
#include "app_timer_driver.h"

#include "app_ias_indicator.h"


/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifndef DEBUG_INDICATOR
#define TRACE_INDICATOR TRUE
#else
#define TRACE_INDICATOR TRUE
#endif
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vToggleLED(void);
PRIVATE void vLED_Off(void);
PRIVATE void vLED_On(void);
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PRIVATE teIASDeviceState eIASDeviceState;
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/


/****************************************************************************
 *
 * NAME: vInitIndicationLEDs
 *
 * DESCRIPTION:
 * Initialize the LEDs both for strobe indication as well as the joining
 * indicators
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vInitIndicationLEDs(void)
{
	 vAHI_DioSetDirection(0, APP_LED_DIO_MASK);
	 vAHI_DioSetOutput(0,APP_LED_DIO_MASK);
}
/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: vDeviceStateIndicator
 *
 * DESCRIPTION:
 * Generates the strobe while the device is in warning
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(vDeviceStateIndicatorTask)
{

    OS_eStopSWTimer(APP_IndicatorTimer);
    vToggleLED();
    DBG_vPrintf(TRACE_INDICATOR,"\nThe IAS Device State = %d",eGetIASDeviceState());
    switch(eGetIASDeviceState())
    {
    case E_IAS_DEV_STATE_NOT_JOINED:
        /*50% dutycycle , 0.5Hz  => OnTime = 1 Sec and Offtime = 1 sec*/
        OS_eStartSWTimer(APP_IndicatorTimer,APP_TIME_MS(1000) , NULL);

        break;
    case E_IAS_DEV_STATE_JOINED:
        /*50% dutycycle , 1Hz  => OnTime = 0.5 Sec and Offtime = 0.5 sec*/
        OS_eStartSWTimer(APP_IndicatorTimer,APP_TIME_MS(500) , NULL);
        /**/
        break;
    case E_IAS_DEV_STATE_READY_TO_ENROLL:
        /*50% dutycycle , 2Hz  => OnTime = 0.25 Sec and Offtime = 0.25 sec*/
        OS_eStartSWTimer(APP_IndicatorTimer,APP_TIME_MS(250) , NULL);
        break;
    case E_IAS_DEV_STATE_ENROLLED:
        vLED_On();
        break;
    default:
    	OS_eStartSWTimer(APP_IndicatorTimer,APP_TIME_MS(2000) , NULL);
    	break;

    }

}
/****************************************************************************
 *
 * NAME: eGetIASDeviceState
 *
 * DESCRIPTION:
 * Gets the device status for the LED indication
 *
 * RETURNS:
 * Device State teIASDeviceState
 *
 ****************************************************************************/
teIASDeviceState eGetIASDeviceState(void)
{
    return eIASDeviceState;
}
/****************************************************************************
 *
 * NAME: vSetIASDeviceState
 *
 * DESCRIPTION:
 * Sets the device status for the LED indication
 *
 * PARAMETERS :
 * Device state to be set teIASDeviceState
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
void vSetIASDeviceState(teIASDeviceState eDeviceState)
{
    eIASDeviceState=eDeviceState;
}
/****************************************************************************
 *
 * NAME: vToggleLED
 *
 * DESCRIPTION:
 * Sets the toggle bit for the LED blink
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vToggleLED(void)
{
    static bool_t bToggle;
    bToggle? vLED_Off(): vLED_On();
    bToggle=~bToggle;
}
/****************************************************************************
 *
 * NAME: vLED_Off
 *
 * DESCRIPTION:
 * Sets the LED off
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vLED_Off(void)
{
	 vAHI_DioSetOutput(0,APP_LED_DIO_MASK);
}
/****************************************************************************
 *
 * NAME: vLED_On
 *
 * DESCRIPTION:
 * Sets the LED On
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vLED_On(void)
{
	 vAHI_DioSetOutput(APP_LED_DIO_MASK,0);
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
