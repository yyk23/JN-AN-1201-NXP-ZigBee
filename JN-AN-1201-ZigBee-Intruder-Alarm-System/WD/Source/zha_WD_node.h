/*****************************************************************************
 *
 * MODULE:             JN-AN-1201
 *
 * COMPONENT:          zha_WD_node.h
 *
 * DESCRIPTION:        ZHA Demo : Stack <-> Light-App Interaction (Interface)
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

#ifndef APP_WD_NODE_H_
#define APP_WD_NODE_H_

#include "app_common.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef enum{
    E_WARNING_STOP,
    E_WARNING_BURGLAR,
    E_WARNING_FIRE,
    E_WARNING_EMERGENCY,
    E_WARNING_ENTRY_EXIT_DELAY
}teWarningMode;

#define WARNING_BURGLAR_PERIOD          1000 /* 50 % DC */
#define WARNING_FIRE_PERIOD             800  /* 20 % DC */
#define WARNING_EMERGENCY_PERIOD        200  /* 50 % DC */
#define WARNING_ENTRY_EXIT_DELAY_PERIOD 500  /* 50 % DC */

typedef enum{
    E_SQUAWK_ARMED,
    E_SQUAWK_DISARMED
}teSquawkMode;

#define SQUAWK_PERIOD   				500  /* 50 % DC */

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
PUBLIC void APP_vInitialiseNode(void);
PUBLIC teNODE_STATES eGetNodeState(void);
PUBLIC uint8 u8GetIASDeviceState(void);
PUBLIC void vStobeIndication(bool_t bStrobe,uint8 u8Level);
PUBLIC void vWarning(uint8 u8Mode, uint16 u16Duration, uint8 u8Strobe, uint8 u8StrobeLevel, uint8 u8StobeDutyCycle);
PUBLIC void vSquawk(uint8 u8Mode, uint8 u8Strobe, uint8 u8SquawkLevel);

/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/

/****************************************************************************/

/****************************************************************************/
/****************************************************************************/

#endif /*APP_WD_NODE_H_*/
