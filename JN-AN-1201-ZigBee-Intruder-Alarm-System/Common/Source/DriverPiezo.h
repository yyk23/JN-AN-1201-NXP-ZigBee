/****************************************************************************/
/*
 * MODULE              JN-AN-1162 JenNet-IP Smart Home
 *
 * DESCRIPTION         Piezo driver public interface
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
#ifndef  DRIVERPIEZO_H_INCLUDED
#define  DRIVERPIEZO_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/
/* Jennic include files */
#include <jendefs.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define DRIVER_PIEZO_MODE_OFF		 	 	 0
#define DRIVER_PIEZO_MODE_CONSTANT  		 1
#define DRIVER_PIEZO_MODE_DOWN	  			 2
#define DRIVER_PIEZO_MODE_UP		  		 3
#define DRIVER_PIEZO_MODE_UP_DOWN   		 4
#define DRIVER_PIEZO_MODE_DOWN_UP   		 5
#define DRIVER_PIEZO_MODE_TUNE  	  		 6

#define DRIVER_PIEZO_OCTAVES				 7
#define DRIVER_PIEZO_NOTES			   		12
#define DRIVER_PIEZO_RTTTL_OCTAVE_OFFSET  	 2

#define DRIVER_PIEZO_NOTE_C				 	 0
#define DRIVER_PIEZO_NOTE_CS				 1
#define DRIVER_PIEZO_NOTE_D					 2
#define DRIVER_PIEZO_NOTE_DS				 3
#define DRIVER_PIEZO_NOTE_E					 4
#define DRIVER_PIEZO_NOTE_F					 5
#define DRIVER_PIEZO_NOTE_FS				 6
#define DRIVER_PIEZO_NOTE_G					 7
#define DRIVER_PIEZO_NOTE_GS				 8
#define DRIVER_PIEZO_NOTE_A					 9
#define DRIVER_PIEZO_NOTE_AS				10
#define DRIVER_PIEZO_NOTE_B					11

#define DRIVER_PIEZO_TUNES					 2

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
PUBLIC void   DriverPiezo_vInit(uint8 u8Timer);
PUBLIC void   DriverPiezo_vOff(void);
PUBLIC bool_t DriverPiezo_bTone(uint32 u32Tone);
PUBLIC bool_t DriverPiezo_bTune(char *pcTune);
PUBLIC uint8  DriverPiezo_u8Mode(void);
PUBLIC char  *DriverPiezo_pcTune(void);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern char acDriverPiezoTune[DRIVER_PIEZO_TUNES][300];

#if defined __cplusplus
}
#endif

#endif  /* DRIVERPIEZO_H_INCLUDED*/

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/


