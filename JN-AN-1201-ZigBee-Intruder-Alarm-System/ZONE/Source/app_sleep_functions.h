/*****************************************************************************
 *
 * MODULE:             JN-AN-1135 (IPD)
 *
 * COMPONENT:          app_sleep_functions.h
 *
 * AUTHOR:             jpenn
 *
 * DESCRIPTION:        Application Sleep Handler Functions Header
 *
 * $HeadURL: https://www.collabnet.nxp.com/svn/lprf_apps/Application_Notes/JN-AN-1135-Smart-Energy-HAN-Solutions/Branches/MergeExercise/Tom/IPD_NODE/Source/app_sleep_functions.h $
 *
 * $Revision: 9271 $
 *
 * $LastChangedBy: nxp33194 $
 *
 * $LastChangedDate: 2012-06-01 16:40:44 +0100 (Fri, 01 Jun 2012) $
 *
 * $Id: app_sleep_functions.h 9271 2012-06-01 15:40:44Z nxp33194 $
 *
 ****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5148, JN5142,
 * JN5139]. You, and any third parties must reproduce the copyright and
 * warranty notice and any other legend of ownership on each copy or partial
 * copy of the software.
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

#ifndef APP_SLEEP_FUNCTIONS_H
#define APP_SLEEP_FUNCTIONS_H

#include <jendefs.h>


/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define MAX_POLLS_NO_DATA   1

#define PERIODIC_WAKE

#ifdef PERIODIC_WAKE
#define SLEEP_TIME_IN_SECS          60    //设置睡眠的单位时间为1分钟，即最少1分钟
#define SLEEP_PERIOD                (32768 * SLEEP_TIME_IN_SECS)
#endif

#ifndef DEBUG_SLEEP_INFO
    #define SLEEP_INFO TRUE
#else
    #define SLEEP_INFO TRUE
#endif
#define ACTIVE_TIME          400  //唤醒以后的最少活跃时间400ms
PUBLIC pwrm_tsWakeTimerEvent   sWake;
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
PUBLIC void vScheduleSleep(void);
PUBLIC void vAppWakeCallBack(void);
PUBLIC void vCheckForSleepPostTx( void );
PUBLIC void vCheckForSleep( uint8 * u8NoDataCount );
/*设定APP的忙状态，用于判断设备是否符合进入睡眠状态*/
PUBLIC  void App_BusyConfig(bool appstatus);
PUBLIC bool App_BusyRead(void);
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#endif /* APP_SLEEP_FUNCTIONS_H */
