/*****************************************************************************
 *
 * MODULE:             JN-AN-1201
 *
 * COMPONENT:          zha_WD_generation.c
 *
 * DESCRIPTION:        ZHA Demo : Warning implementation
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
#include "zha_WD_generation.h"
#include "app_timer_driver.h"
#include "LightingBoard.h"
#include "DriverPiezo.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifndef DEBUG_WD_GEN
    #define TRACE_WD_GEN   FALSE
#else
    #define TRACE_WD_GEN   TRUE
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PRIVATE APP_tsStartWarning sWarning;
PRIVATE bool_t bON;
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: vStartWarn
 *
 * DESCRIPTION:
 * Starts the warning with given parameters
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vStartWarn(APP_tsStartWarning sStartWarning)
{
	if(OS_E_SWTIMER_STOPPED != OS_eGetSWTimerStatus(APP_WD_GenTimer))
		OS_eStopSWTimer(APP_WD_GenTimer);
	sWarning = sStartWarning;
	DBG_vPrintf(TRACE_WD_GEN,"vStartWarn\n");
	DBG_vPrintf(TRACE_WD_GEN,"vStartWarn sWarning.u16OnTime   = %d\n",   sWarning.u16OnTime);
	DBG_vPrintf(TRACE_WD_GEN,"vStartWarn sWarning.u16OffTime  = %d\n",   sWarning.u16OffTime);
	DBG_vPrintf(TRACE_WD_GEN,"vStartWarn sWarning.u16Duration = %d \n",  sWarning.u16Duration);
	DBG_vPrintf(TRACE_WD_GEN,"vStartWarn sWarning.u8Level     = %d \n",  sWarning.u8Level);
	DBG_vPrintf(TRACE_WD_GEN,"vStartWarn sWarning.u32Tone     = 0x%x\n", sWarning.u32Tone);
	OS_eStartSWTimer(APP_WD_GenTimer, APP_TIME_MS(100),NULL );
	bWhite_LED_SetLevel(sWarning.u8Level);
	bWhite_LED_On();
	bON = TRUE;
	/* Play tone on piezo */
	DriverPiezo_bTone(sWarning.u32Tone);
}

/****************************************************************************
 *
 * NAME: APP_RunWarning
 *
 * DESCRIPTION:
 * Callback to start the timer
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_SWTIMER_CALLBACK(APP_RunWarning,ptr)
{
    uint16 u16Time;
	if(OS_E_SWTIMER_STOPPED != OS_eGetSWTimerStatus(APP_WD_GenTimer))
		OS_eStopSWTimer(APP_WD_GenTimer);

    DBG_vPrintf(TRACE_WD_GEN,"APP_RunWarning bON %d\n",bON);
    if(bON)
    {
        u16Time = sWarning.u16OnTime;
        bWhite_LED_On();
        bON = FALSE;
    }
    else
    {
        u16Time = sWarning.u16OffTime;
        bWhite_LED_Off();
        bON = TRUE;
    }

    if(sWarning.u16Duration >= u16Time)
    {
        sWarning.u16Duration -= u16Time;
    }
    else
    {
        sWarning.u16Duration =0;
    }

    if(sWarning.u16Duration)
    {
        OS_eStartSWTimer(APP_WD_GenTimer, APP_TIME_MS(u16Time),NULL );
    }
    else
    {
    	bWhite_LED_SetLevel(0);
    	bWhite_LED_Off();
		/* Silence tone on piezo */
		DriverPiezo_vOff();
    }
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
