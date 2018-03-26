/****************************************************************************/
/*
 * MODULE              JN-AN-1162 JenNet-IP Smart Home
 *
 * DESCRIPTION         Piezo driver implementation
 *
 * RTTTL ringtones can be found at:
 *
 * http://www.vex.net/~lawrence/ringtones.html
 * "Urgent:d=8,o=6,b=500:c,e,d7,c,e,a#,c,e,a,c,e,g,c,e,a,c,e,a#,c,e,d7"
 * "Triple:d=8,o=5,b=635:c,e,g,c,e,g,c,e,g,c6,e6,g6,c6,e6,g6,c6,e6,g6,c7,e7,g7,c7,e7,g7,c7,e7,g7"
 * http://arcadetones.emuunlim.com/arcade.htm
 */
/****************************************************************************/
/*
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5168, JN5164].
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
 * Copyright NXP B.V. 2015. All rights reserved
 */
/****************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
/* Standard library include files */
#include <stdlib.h>
#include <string.h>
/* Jennic include files */
#include <jendefs.h>
#include <AppHardwareApi.h>
#include <LedControl.h>
#include <PeripheralRegs.h>
#include <dbg.h>
#ifdef RTOS
#include <os.h>
#endif
/* Local include files */
//#include "DeviceDefs.h"
#include "DriverPiezo.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define DEBUG_DRIVER FALSE

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Prototypes                                              ***/
/****************************************************************************/
#ifndef RTOS
PRIVATE void DriverPiezo_vTimerIsr(uint32 u32DeviceId, uint32 u32ItemBitmap);
#endif

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PRIVATE uint8			   u8PiezoTimer = 0xff;		/**< Timer */

PRIVATE uint8 			   u8PiezoMode;				/**< Piezo mode */
PRIVATE uint16			  u16PiezoPeriodHi;			/**< Piezo period */
PRIVATE uint16			  u16PiezoPeriodLo;			/**< Piezo period */
PRIVATE uint16			  u16PiezoMinPeriod;		/**< Piezo period */
PRIVATE uint16			  u16PiezoMaxPeriod;		/**< Piezo period */
PRIVATE uint8 			   u8PiezoPulse;			/**< Piezo pulse mask */
PRIVATE uint8			   u8PiezoStep;				/**< Piezo step */
PRIVATE uint16			 au16PiezoPeriod[DRIVER_PIEZO_OCTAVES][DRIVER_PIEZO_NOTES] = 		/**< Piezo notes by octave then note for 2Mhz timer */
{
/*	     C     C#      D     D#      E      F     F#     G      G#      A     A#      B     RtttlOct IntOct */
	{30578, 28862, 27242, 25713, 24270, 22908, 21622, 20408, 19263, 18182, 17161, 16198}, /* 1       0 */
	{15289, 14431, 13621, 12856, 12135, 11454, 10811, 10204,  9631,  9091,  8581,  8099}, /* 2       1 */
	{ 7645,  7215,  6810,  6428,  6067,  5727,  5405,  5102,  4816,  4545,  4290,  4050}, /* 3       2 */
	{ 3822,  3608,  3405,  3214,  3034,  2863,  2703,  2551,  2408,  2273,  2145,  2025}, /* 4       3 */
	{ 1911,  1804,  1703,  1607,  1517,  1432,  1351,  1276,  1204,  1136,  1073,  1012}, /* 5       4 */
	{  956,   902,   851,   804,   758,   716,   676,   638,   602,   568,   536,   506}, /* 6       5 */
	{  478,   451,   426,   402,   379,   358,   338,   319,   301,   284,   268,   253}  /* 7       6 */
};
PRIVATE uint8  au8PiezoIndex[7] = {9, 11, 0, 2, 4, 5, 7};
PRIVATE uint32 u32PiezoBeat64;
PRIVATE uint16 u16PiezoNotes;
PRIVATE uint16  u16PiezoNote;
PRIVATE uint8   au8PiezoNote[256];
PRIVATE uint8   au8PiezoOctave[256];
PRIVATE uint8   au8PiezoDuration[256];
PRIVATE uint32  u32PiezoDuration;
PRIVATE char     acPiezoName[64];
PRIVATE uint8    u8PiezoName;
PRIVATE uint32	u32PiezoTimer500ns;
PRIVATE uint32	u32PiezoTimer100ms;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************/
/**
 * <b>DriverPiezo_vInit</b> &mdash; Initialise the network.
 *
 * Called after the network stack is initialised and provides the opportunity
 * to perform additional initialisation tasks.
 *
 * This function is called on both cold and warm starts.
 */
/****************************************************************************/
PUBLIC void DriverPiezo_vInit (uint8 u8Timer)
{
	/* Valid timer ? */
	if (u8Timer <= E_AHI_TIMER_4)
	{
		/* Note timer */
		u8PiezoTimer = u8Timer;

		/* Set up timer for 2MHz base */
		vAHI_TimerEnable(u8PiezoTimer,	/*uint8 u8Timer*/
						 3,				/*uint8 u8Prescale*/
						 FALSE,			/*bool_t bIntRiseEnable*/
						 TRUE,			/*bool_t bIntPeriodEnable*/
						 TRUE);			/*bool_t bOutputEnable*/
		vAHI_TimerConfigure(u8PiezoTimer,
							FALSE, /*bool_t bInvertPwmOutput*/
							TRUE); /*bool_t bGateDisable*/

		/* No RTOS ? */
		#ifndef RTOS
		{
			/* Which timer ? */
			switch (u8PiezoTimer)
			{
				/* Specify interrupt callback */
				case E_AHI_TIMER_0: vAHI_Timer0RegisterCallback(DriverPiezo_vTimerIsr); break;
				case E_AHI_TIMER_1: vAHI_Timer1RegisterCallback(DriverPiezo_vTimerIsr); break;
				case E_AHI_TIMER_2: vAHI_Timer2RegisterCallback(DriverPiezo_vTimerIsr); break;
				case E_AHI_TIMER_3: vAHI_Timer3RegisterCallback(DriverPiezo_vTimerIsr); break;
				case E_AHI_TIMER_4: vAHI_Timer4RegisterCallback(DriverPiezo_vTimerIsr); break;
			}
		}
		#endif

		acPiezoName[0] = '\0';
	}
}

/****************************************************************************/
/**
 * <b>DriverPiezo_vOff</b> &mdash; Turn off piezo
 */
/****************************************************************************/
PUBLIC void DriverPiezo_vOff(void)
{
	/* Valid timer and parameters ? */
	if (u8PiezoTimer <= E_AHI_TIMER_4)
	{
		/* Go into off mode */
		u8PiezoMode = DRIVER_PIEZO_MODE_OFF;
	}

    /* Debug */
    DBG_vPrintf(DEBUG_DRIVER, "\nDriverPiezo_vOff()");
}


/****************************************************************************/
/**
 * <b>DriverPiezo_vTone</b> &mdash; Play tone in different styles
 *
 * Tone is packed as follows (least to highest significance)
 *
 * MinNote   - 4 bits
 * MinOctave - 4 bits
 * MaxNote   - 4 bits
 * MaxOctave - 4 bits
 * Pulse     - 8 bits
 * Step      - 4 bits
 * Mode      - 4 bits
 */
/****************************************************************************/
PUBLIC bool_t DriverPiezo_bTone(uint32 u32Tone)
{
	bool_t    bReturn = FALSE;
	uint8 	 u8Mode;
	uint8 	 u8MinOctave;
	uint8 	 u8MinNote;
	uint8 	 u8MaxOctave;
	uint8 	 u8MaxNote;
	uint8 	 u8Step;
	uint8 	 u8Pulse;
	uint16  u16MinPeriod;
	uint16  u16MaxPeriod;
	uint16 u16Temp;

    /* Debug */
    DBG_vPrintf(DEBUG_DRIVER, "\nDriverPiezo_bTone(0x%x)",
    	u32Tone);

	/* Unpack data */
	u8MinNote     = (uint8) (u32Tone & 0xF);
	u32Tone     >>= 4;
	u8MinOctave   = (uint8) (u32Tone & 0xF);
	u32Tone     >>= 4;
	u8MaxNote     = (uint8) (u32Tone & 0xF);
	u32Tone     >>= 4;
	u8MaxOctave   = (uint8) (u32Tone & 0xF);
	u32Tone     >>= 4;
	u8Pulse       = (uint8) (u32Tone & 0xFF);
	u32Tone     >>= 8;
	u8Step        = (uint8) (u32Tone & 0xF);
	u32Tone     >>= 4;
	u8Mode        = (uint8) (u32Tone & 0xF);

    /* Debug */
    DBG_vPrintf(DEBUG_DRIVER, "{%d, %d, %d, %d, %d, %d, 0x%x}",
	   u8Mode,
	   u8MinOctave,
	   u8MinNote,
	   u8MaxOctave,
	   u8MaxNote,
	   u8Step,
	   u8Pulse);

	/* Valid timer and parameters ? */
	if (u8PiezoTimer <= E_AHI_TIMER_4 &&
		u8Mode 		 < DRIVER_PIEZO_MODE_TUNE &&
	    u8MinOctave  < DRIVER_PIEZO_OCTAVES &&
	    u8MaxOctave  < DRIVER_PIEZO_OCTAVES &&
	    u8MinNote    < DRIVER_PIEZO_NOTES &&
	    u8MaxNote    < DRIVER_PIEZO_NOTES)
	{
		/* Convert notes to periods */
		u16MinPeriod = au16PiezoPeriod[u8MinOctave][u8MinNote];
		u16MaxPeriod = au16PiezoPeriod[u8MaxOctave][u8MaxNote];

		/* Periods wrong way around ? */
		if (u16MaxPeriod < u16MinPeriod)
		{
			/* Swap them */
			u16Temp = u16MaxPeriod;
			u16MaxPeriod = u16MinPeriod;
			u16MinPeriod = u16Temp;
		}

		/* Setting has changed ? */
		if (u8Mode 		 != u8PiezoMode ||
			u16MinPeriod != u16PiezoMinPeriod ||
			u16MaxPeriod != u16PiezoMaxPeriod ||
			u8Step       != u8PiezoStep ||
			u8Pulse      != u8PiezoPulse)
		{
			/* What is the new mode ? */
			switch (u8Mode)
			{
				/* Constant or up or up down ? */
				case DRIVER_PIEZO_MODE_CONSTANT:
				case DRIVER_PIEZO_MODE_UP:
				case DRIVER_PIEZO_MODE_UP_DOWN:
				{
					/* Set current period to new minimum */
					u16PiezoPeriodHi = u16MinPeriod;
					u16PiezoPeriodLo = u16PiezoPeriodHi/2;
				}
				break;

				/* Down or down up ? */
				case DRIVER_PIEZO_MODE_DOWN:
				case DRIVER_PIEZO_MODE_DOWN_UP:
				{
					/* Set current period to new maximum */
					u16PiezoPeriodHi = u16MinPeriod;
					u16PiezoPeriodLo = u16PiezoPeriodHi/2;
				}
				break;

				/* Default (off) */
				default:
				{
					/* Set current period to new minimum */
					u16PiezoPeriodHi = u16MinPeriod;
					/* Set low period so no sound is made */
					u16PiezoPeriodLo = u16PiezoPeriodHi;
				}
				break;
			}

			/* Pulsing and we should make no sound - adjust low period */
			if ((u8Pulse & u32PiezoTimer100ms) == 0) u16PiezoPeriodLo = u16PiezoPeriodHi;
			/* Note new settings */
			u16PiezoMinPeriod = u16MinPeriod;
			u16PiezoMaxPeriod = u16MaxPeriod;
			u8PiezoStep = u8Step;
			u8PiezoPulse = u8Pulse;
			/* Finally note new mode */
			u8PiezoMode = u8Mode;
			/* Piezo is not currently off - start timer running */
			if (u8PiezoMode != DRIVER_PIEZO_MODE_OFF) vAHI_TimerStartSingleShot(u8PiezoTimer, u16PiezoPeriodLo, u16PiezoPeriodHi);
		}

		/* Everything is awesome */
		bReturn = TRUE;
	}

    /* Debug */
    DBG_vPrintf(DEBUG_DRIVER, " = %d {0x%x}",
		bReturn, u32PiezoTimer100ms);

	return bReturn;
}

/****************************************************************************/
/**
 * <b>DriverPiezo_vTune</b> &mdash; Play tune on piezo
 */
/****************************************************************************/
PUBLIC bool_t DriverPiezo_bTune(char *pcTune)
{
	bool_t        bReturn          = FALSE;
	uint16  	u16DefaultOctave   = 4;
	uint16  	u16DefaultDuration = 4;
	uint16    	u16Bpm             = 120;
	uint8        u8TuneState       = 0;
	uint16 	  *pu16TuneParam       = NULL;
	uint8   	 u8Note            = 0xff;
	uint8   	 u8NoteDuration    = 0xff;
	uint8   	 u8NoteOctave      = 0xff;
	bool_t  	  bNoteDot         = FALSE;

	/* Valid timer and parameters ? */
	if (u8PiezoTimer <= E_AHI_TIMER_4 &&
		pcTune  	 != NULL)
	{
		/* Initialise position in tune */
		u16PiezoNotes = 0;
		u16PiezoNote  = 0;
		u8PiezoName  = 0;

		/* Work through the name */
		while(u8TuneState == 0)
		{
			/* End of string ? */
			if (*pcTune == '\0')
			{
				/* Error */
				u8TuneState = 0xff;
			}
			/* Reached : marking the end of the name */
			else if (*pcTune == ':')
			{
				/* Terminate name */
				acPiezoName[u8PiezoName++] = '\0';
				/* Looking for a parameter */
				u8TuneState = 1;
			}
			else
			{
				/* Transfer character */
				acPiezoName[u8PiezoName++] = *pcTune;
			}
			/* Go to next character */
			pcTune++;
		}

		/* Work through the parameters */
		while(u8TuneState == 1)
		{
			/* End of string ? */
			if (*pcTune == '\0')
			{
				/* Error */
				u8TuneState = 0xff;
			}
			/* Reached : marking the end of the parameters */
			else if (*pcTune == ':')
			{
				/* Null parameter */
				pu16TuneParam = NULL;
				/* Sanity check default parameters */
				if (u16DefaultDuration == 0) u16DefaultDuration =   4;
				if (u16DefaultOctave   == 0) u16DefaultOctave   =   4;
				if (u16Bpm            == 0)  u16Bpm             = 120;
				/* Calculate how many periods per 64th note (bpm is quarter notes per minute) */
				u32PiezoBeat64 = ((uint32) 2000000 * (uint32) 60) / ((uint32) u16Bpm * (uint32) 16);
				/* Ready to read notes */
				u8TuneState = 2;
			}
			/* Default duration ? */
			else if (*pcTune == 'd')
			{
				/* Note the paramter we are expecting to process */
				pu16TuneParam = &u16DefaultDuration;
			}
			/* Default octave ? */
			else if (*pcTune == 'o')
			{
				/* Note the paramter we are expecting to process */
				pu16TuneParam = &u16DefaultOctave;
			}
			/* Default BPM ? */
			else if (*pcTune == 'b')
			{
				/* Note the paramter we are expecting to process */
				pu16TuneParam = &u16Bpm;
			}
			/* Equals ? */
			else if (*pcTune == '=')
			{
				/* Zero value */
				if (pu16TuneParam != NULL) *pu16TuneParam = 0;
			}
			/* Comma ? */
			else if (*pcTune == ',')
			{
				/* Null parameter */
				pu16TuneParam = NULL;
			}
			/* Numeric ? */
			else if (*pcTune >= '0' && *pcTune <= '9')
			{
				/* Update parameter value */
				if (pu16TuneParam != NULL)
				{
					/* Multiply current value by 10 */
					*pu16TuneParam *= 10;
					/* Add new digit */
					*pu16TuneParam += (*pcTune - '0');
				}
			}

			/* Go to next character */
			pcTune++;
		}

		/* Parsing notes ? */
		while (u8TuneState == 2)
		{
			/* Comma or terminator ? */
			if (*pcTune == ',' || *pcTune == '\0')
			{
				/* Valid note ? */
				if (u8Note <= 12)
				{
					/* Transfer note to array */
					au8PiezoNote[u16PiezoNotes] = u8Note;
					/* Not got an octave ? */
					if (u8NoteOctave == 0xff)
					{
						/* Use the default octave */
						u8NoteOctave = u16DefaultOctave;
					}
					/* Use specified octave */
					au8PiezoOctave[u16PiezoNotes] = u8NoteOctave;
					/* Not got a duration ? */
					if (u8NoteDuration == 0xff)
					{
						/* Use the default duration */
						u8NoteDuration = u16DefaultDuration;
					}
					/* Convert duration to 64ths of a beat */
					if      (u8NoteDuration <=  1) au8PiezoDuration[u16PiezoNotes] = 64;
					else if (u8NoteDuration <=  2) au8PiezoDuration[u16PiezoNotes] = 32;
					else if (u8NoteDuration <=  4) au8PiezoDuration[u16PiezoNotes] = 16;
					else if (u8NoteDuration <=  8) au8PiezoDuration[u16PiezoNotes] =  8;
					else if (u8NoteDuration <= 16) au8PiezoDuration[u16PiezoNotes] =  4;
					else                           au8PiezoDuration[u16PiezoNotes] =  2;
					/* Dotted note - increase duration by half */
					if (bNoteDot) au8PiezoDuration[u16PiezoNotes] += (au8PiezoDuration[u16PiezoNotes] / 2);
					/* Increment number of notes */
					u16PiezoNotes++;
					/* Reset values for next note */
					u8Note            = 0xff;
					u8NoteDuration    = 0xff;
					u8NoteOctave      = 0xff;
					bNoteDot          = FALSE;
				}
				/* End of string - end parsing */
				if (*pcTune == '\0') u8TuneState = 3;
			}
			/* Note ? */
			else if (*pcTune >= 'A' && *pcTune <= 'G')
			{
				/* Set note */
				u8Note = au8PiezoIndex[*pcTune-'A'];
			}
			/* Note ? */
			else if (*pcTune >= 'a' && *pcTune <= 'g')
			{
				/* Set note */
				u8Note = au8PiezoIndex[*pcTune-'a'];
			}
			/* Sharp Note ? */
			else if (*pcTune == '#')
			{
				/* Valid note - increment */
				if (u8Note < 12) u8Note++;
			}
			/* Dotted note ? */
			else if (*pcTune == '.')
			{
				/* Valid note - increment */
				bNoteDot = TRUE;
			}
			/* Pause note ? */
			else if (*pcTune == 'P' || *pcTune == 'p')
			{
				/* Use 12 for note (out of range) */
				u8Note = 12;
			}
			/* Numeric ? */
			else if (*pcTune >= '0' && *pcTune <= '9')
			{
				/* Not got note yet (numeric is duration) ? */
				if (u8Note == 0xff)
				{
					/* Numeric is a duration not yet got one ? */
					if (u8NoteDuration == 0xff)
					{
						/* Note duration */
						u8NoteDuration = *pcTune - '0';
					}
					else
					{
						/* Multiply current duration by 10 */
						u8NoteDuration *= 10;
						/* Add this digit */
						u8NoteDuration += *pcTune - '0';
					}
				}
				/* Got note (numeric is octave) ? */
				else
				{
					/* Numeric is a octave not yet got one ? */
					if (u8NoteOctave == 0xff)
					{
						/* Note octave */
						u8NoteOctave = *pcTune - '0';
					}
					else
					{
						/* Multiply current octave by 10 */
						u8NoteOctave *= 10;
						/* Add this digit */
						u8NoteOctave += *pcTune - '0';
					}
				}
			}

			/* Go to next character */
			pcTune++;
		}

		/* Ready to play and have some notes ? */
		if (u8TuneState == 3 && u16PiezoNotes > 0)
		{
			/* Make sure we don't pulse */
			u8PiezoPulse = 0xff;
			/* Note we are in piezo tune mode */
			u8PiezoMode = DRIVER_PIEZO_MODE_TUNE;
			/* Zero duration */
			u32PiezoDuration = 0;
			/* Valid note ? */
			if (au8PiezoNote[u16PiezoNote] < 12)
			{
				/* Set period for note */
				u16PiezoPeriodHi = au16PiezoPeriod[au8PiezoOctave[u16PiezoNote]-DRIVER_PIEZO_RTTTL_OCTAVE_OFFSET][au8PiezoNote[u16PiezoNote]];
				/* Make sure we set a lo period so the piezo sounds */
				u16PiezoPeriodLo = u16PiezoPeriodHi / 2;
			}
			/* Pause note */
			else
			{
				/* Set period for pause */
				u16PiezoPeriodHi = au16PiezoPeriod[4][0];
				/* No low period so the piezo does not sound */
				u16PiezoPeriodLo = u16PiezoPeriodHi;
			}
			/* Run timer */
			vAHI_TimerStartSingleShot(u8PiezoTimer, u16PiezoPeriodLo, u16PiezoPeriodHi);
			/* Everything is awesome */
			bReturn = TRUE;
		}
	}

	return bReturn;
}

/****************************************************************************/
/**
 * <b>pcPiezo_Tune</b> &mdash; Return tune name currently playing
 */
/****************************************************************************/
PUBLIC char *DriverPiezo_pcTune(void)
{
	return acPiezoName;
}

/****************************************************************************/
/**
 * <b>pcPiezo_Tune</b> &mdash; Return tune name currently playing
 */
/****************************************************************************/
PUBLIC uint8 DriverPiezo_u8Mode(void)
{
	return u8PiezoMode;
}

/****************************************************************************/
/**
 * <b>DriverPiezo_vTimerIsr</b> &mdash; Handle timer 1 interrupt
 */
/****************************************************************************/
#ifdef RTOS
OS_ISR(DriverPiezo_vTimerIsr)
#else
PRIVATE void DriverPiezo_vTimerIsr(uint32 u32DeviceId, uint32 u32ItemBitmap)
#endif
{
    /* Debug */
    //DBG_vPrintf(DEBUG_DRIVER, ".");

    /* RTOS ? */
    #if RTOS
    {
		/* Clear interrupt */
		(void) u8AHI_TimerFired(u8PiezoTimer);
	}
	#endif

	/* Increment 500ns timer by period */
	u32PiezoTimer500ns += (uint32) u16PiezoPeriodHi;
	/* Has the 500ns timer gone over 100ms ? */
	if (u32PiezoTimer500ns >= 200000)
	{
	    /* Debug */
	    //DBG_vPrintf(DEBUG_DRIVER, "#");
		/* Increment 100ms timer */
		u32PiezoTimer100ms++;
		/* Roll back 500ns timer */
		u32PiezoTimer500ns -= 200000;
	}

	/* What is the piezo mode ? */
	switch (u8PiezoMode)
	{
		case DRIVER_PIEZO_MODE_DOWN:
		{
			/* Period less than max ? */
			if (u16PiezoPeriodHi < u16PiezoMaxPeriod)
			{
				/* Increment */
				u16PiezoPeriodHi+=u8PiezoStep;
			}
			else
			{
				/* Restart from minimum */
				u16PiezoPeriodHi = u16PiezoMinPeriod;
			}
			/* Set low period so we sound */
			u16PiezoPeriodLo = u16PiezoPeriodHi/2;
		}
		break;

		case DRIVER_PIEZO_MODE_DOWN_UP:
		{
			/* Period less than max ? */
			if (u16PiezoPeriodHi < u16PiezoMaxPeriod)
			{
				/* Increment */
				u16PiezoPeriodHi+=u8PiezoStep;
			}
			else
			{
				/* Start going down */
				u16PiezoPeriodHi = u16PiezoMaxPeriod;
				u8PiezoMode    = DRIVER_PIEZO_MODE_UP_DOWN;
			}
			/* Set low period so we sound */
			u16PiezoPeriodLo = u16PiezoPeriodHi/2;
		}
		break;

		case DRIVER_PIEZO_MODE_UP:
		{
			/* Period greater than min ? */
			if (u16PiezoPeriodHi > u16PiezoMinPeriod)
			{
				/* Decrement */
				u16PiezoPeriodHi-=u8PiezoStep;
			}
			else
			{
				/* Restart from maximum */
				u16PiezoPeriodHi = u16PiezoMaxPeriod;
			}
			/* Set low period so we sound */
			u16PiezoPeriodLo = u16PiezoPeriodHi/2;
		}
		break;

		case DRIVER_PIEZO_MODE_UP_DOWN:
		{
			/* Period greater than min ? */
			if (u16PiezoPeriodHi > u16PiezoMinPeriod)
			{
				/* Decrement */
				u16PiezoPeriodHi-=u8PiezoStep;
			}
			else
			{
				/* Start going up */
				u16PiezoPeriodHi = u16PiezoMinPeriod;
				u8PiezoMode    = DRIVER_PIEZO_MODE_DOWN_UP;
			}
			/* Set low period so we sound */
			u16PiezoPeriodLo = u16PiezoPeriodHi/2;
		}
		break;

		case DRIVER_PIEZO_MODE_TUNE:
		{
			/* Adjust the duration of this note by the period */
			u32PiezoDuration += (uint32) u16PiezoPeriodHi;
			/* Have we played this note for long enough ? */
			if (u32PiezoDuration >= (uint32) au8PiezoDuration[u16PiezoNote] * u32PiezoBeat64)
			{
				/* Go to next note */
				u16PiezoNote++;
				/* Reached end of tune - stop */
				if (u16PiezoNote >= u16PiezoNotes) u8PiezoMode = DRIVER_PIEZO_MODE_OFF;
				/* Got a new note to play ? */
				if (u16PiezoNote < u16PiezoNotes && u8PiezoMode == DRIVER_PIEZO_MODE_TUNE)
				{
					/* Zero duration */
					u32PiezoDuration = 0;
					/* Valid note ? */
					if (au8PiezoNote[u16PiezoNote] < 12)
					{
						/* Set period for note */
						u16PiezoPeriodHi = au16PiezoPeriod[au8PiezoOctave[u16PiezoNote]-DRIVER_PIEZO_RTTTL_OCTAVE_OFFSET][au8PiezoNote[u16PiezoNote]];
						/* Make sure we set a lo period so the piezo sounds */
						u16PiezoPeriodLo = u16PiezoPeriodHi / 2;
					}
					/* Pause note */
					else
					{
						/* Set period for pause */
						u16PiezoPeriodHi = au16PiezoPeriod[4-DRIVER_PIEZO_RTTTL_OCTAVE_OFFSET][au8PiezoNote[0]];
						/* No low period so the piezo does not sound */
						u16PiezoPeriodLo = u16PiezoPeriodHi;
					}
				}
			}
		}
		break;

		/* Default (off or constant) */
		default:
		{
			/* Set low period so we sound */
			u16PiezoPeriodLo = u16PiezoPeriodHi/2;
		}
		break;
	}

	/* Pulsing and we should make no sound - adjust low period */
	if ((u8PiezoPulse & u32PiezoTimer100ms) == 0) u16PiezoPeriodLo = u16PiezoPeriodHi;
	/* Piezo is not currently off - start timer running */
	if (u8PiezoMode != DRIVER_PIEZO_MODE_OFF) vAHI_TimerStartSingleShot(u8PiezoTimer, u16PiezoPeriodLo, u16PiezoPeriodHi);
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
