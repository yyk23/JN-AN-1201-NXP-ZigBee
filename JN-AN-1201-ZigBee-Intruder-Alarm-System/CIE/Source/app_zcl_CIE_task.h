/*****************************************************************************
 *
 * MODULE:             JN-AN-1201 ZHA Demo
 *
 * COMPONENT:          app_zcl_CIE_task.h
 *
 * DESCRIPTION:        ZHA CIE Application Behavior - Interface
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
 * Copyright NXP B.V. 2012. All rights reserved
 *
 ***************************************************************************/

/****************************************************************************/
/* Description.                                                             */
/* If you do not need this file to be parsed by doxygen then delete @file   */
/****************************************************************************/

/** @file
 * Add brief description here.
 * Add more detailed description here
 */

/****************************************************************************/
/* Description End                                                          */
/****************************************************************************/

#ifndef APP_ZCL_TASK_H_
#define APP_ZCL_TASK_H_

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/

#include <jendefs.h>
#include "control_and_indicating_equipment.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define CLD_IASACE_PANEL_PARAMTER_SECONDS_REMAINING     10
#define DISABLE_WARNING                                 0
#define ENTRY_EXIT_DELAY_WARNING_DURATION               10
#define ALARM_WARNING_DURATION                          30
#define STROBE_LEVEL                                    1
#define STROBE_DUTY_CYCLE                               20
#define WARNING_MODE_STROBE_AND_SIREN_LEVEL_BURGLAR     0x17
#define WARNING_MODE_STROBE_AND_SIREN_LEVEL_FIRE        0x27
#define WARNING_MODE_STROBE_AND_SIREN_LEVEL_EMERGENCY   0x37
#define WARNING_MODE_STROBE_AND_SIREN_LEVEL_ENTRY_EXIT_DELAY         0x47
#define SQUAWK_MODE_STROBE_AND_LEVEL_ARMED				0x0B
#define SQUAWK_MODE_STROBE_AND_LEVEL_DISARMED			0x1B

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
PUBLIC void APP_ZCL_vInitialise(void);
PUBLIC void vPermitJoinIndication(void);
PUBLIC bool bCheckNotReadyToArm(uint8 u8ConfigFlag);
PUBLIC void vStartArmingSystem(void);
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern uint8 u8LastPanelStatus;
extern uint8 u8PanelStatusB4NotReadyToArm;
extern uint8 u8ConfigFlag;
extern tsHA_IASCIE sDevice;
#endif /* APP_ZCL_TASK_H_ */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
