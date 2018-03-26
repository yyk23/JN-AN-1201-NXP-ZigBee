/*****************************************************************************
 *
 * MODULE:             JN-AN-
 *
 * COMPONENT:          app_sleep_functions.c
 *
 * AUTHOR:             jpenn
 *
 * DESCRIPTION:        Application Sleep Handler Functions
 *
 * $HeadURL $
 *
 * $Revision: 9281 $
 *
 * $LastChangedBy: nxp33194 $
 *
 * $LastChangedDate: 2012-06-08 15:13:02 +0100 (Fri, 08 Jun 2012) $
 *
 * $Id: app_sleep_functions.c 9281 2012-06-08 14:13:02Z nxp33194 $
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

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <jendefs.h>
#include "os.h"
#include "os_gen.h"
#include "pdum_apl.h"
#include "pdum_gen.h"
#include "pdm.h"
#include "pwrm.h"
#include "dbg.h"
#include "app_sleep_functions.h"
#include "zha_ZONE_node.h"
#include "app_common.h"
#include "app_zcl_ZONE_task.h"
#include "PingParent.h"
#include "zha_ZONE_node.h"
#include "app_uart.h"
#include "app_buttons.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

PRIVATE void vStopSWTimers (void);
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
static uint8 u8PingCount = 0;
static bool app_busy_status=TRUE;
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

PUBLIC pwrm_tsWakeTimerEvent   sWake;

/****************************************************************************
 *
 * NAME: vStopSWTimers
 *
 * DESCRIPTION:
 * Stops any timers ready for sleep
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vStopSWTimers (void)
{

#ifdef KEY_CON_ENABLE

#else
    if (OS_eGetSWTimerStatus(APP_ButtonsScanTimer) != OS_E_SWTIMER_STOPPED)
    {
        OS_eStopSWTimer(APP_ButtonsScanTimer);
    }
#endif

    if (OS_eGetSWTimerStatus(APP_PollTimer) != OS_E_SWTIMER_STOPPED)
    {
        OS_eStopSWTimer(APP_PollTimer);
    }

    if (OS_eGetSWTimerStatus(APP_JoinTimer) != OS_E_SWTIMER_STOPPED)
    {
        OS_eStopSWTimer(APP_JoinTimer);
    }

    if (OS_eGetSWTimerStatus(APP_TickTimer) != OS_E_SWTIMER_STOPPED)
    {
        OS_eStopSWTimer(APP_TickTimer);
    }

    if (OS_eGetSWTimerStatus(APP_BackOffTimer) != OS_E_SWTIMER_STOPPED)
    {
        OS_eStopSWTimer(APP_BackOffTimer);
    }
    if (OS_eGetSWTimerStatus(APP_IndicatorTimer) != OS_E_SWTIMER_STOPPED)
    {
        OS_eStopSWTimer(APP_IndicatorTimer);
    }
	if (OS_eGetSWTimerStatus(APP_uart_timeout) != OS_E_SWTIMER_STOPPED)
	{
	    OS_eStopSWTimer(APP_uart_timeout);
	}
}

/****************************************************************************
 *
 * NAME: vScheduleSleep
 *
 * DESCRIPTION:
 * Schedule a wake event
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vScheduleSleep(void)
{
	ucs cs;
		cs.YCL_ID=0x12345678;
		DBG_vPrintf(1,"a[0] =%x",cs.YCL_Array[0]);
		DBG_vPrintf(1,"a[1] =%x",cs.YCL_Array[1]);
		DBG_vPrintf(1,"a[2] =%x",cs.YCL_Array[2]);
		DBG_vPrintf(1,"a[3] =%x",cs.YCL_Array[3]);
	//进入低功耗前
	if(sDeviceDesc.eNodeState==E_RUNNING)//设备在正在运行
	{
		if( !App_BusyRead() )
		{

		vStopSWTimers();

		#ifdef PERIODIC_WAKE
		PWRM_teStatus teStatus = PWRM_eScheduleActivity(&sWake, SLEEP_PERIOD*sEP_Dev_Inf.sEP_Dev_HeartBeat.heartbeat_value , vAppWakeCallBack  );     /* Schedule the next sleep point */
		if(!teStatus)
		{
			DBG_vPrintf(SLEEP_INFO, "\r\n Scheduling sleep point in %d Min, status = %d\n",sEP_Dev_Inf.sEP_Dev_HeartBeat.heartbeat_value,teStatus);
		}
		else
		{
			DBG_vPrintf(SLEEP_INFO, "\r\n Timer already scheduled");
		}

		#endif
		}
	}
	else if(sDeviceDesc.eNodeState==E_STARTUP)//设备在入网
	{
		DBG_vPrintf(SLEEP_INFO, "\r\n E_startup0");
		vStopSWTimers();
		OS_eStopSWTimer(APP_ActiveTime);//强制进入低功耗模式
		PWRM_teStatus teStatus = PWRM_eScheduleActivity(&sWake, SLEEP_PERIOD*sEP_Dev_Inf.sEP_Dev_HeartBeat.heartbeat_value , vAppWakeCallBack  );//设备在入网失败，进入低功耗，只有外部中断才可以唤醒 。

	}
	else if(sDeviceDesc.eNodeState==E_REJOINING)
	{
		vStopSWTimers();
		OS_eStopSWTimer(APP_ActiveTime);//强制进入低功耗模式
		PWRM_teStatus teStatus = PWRM_eScheduleActivity(&sWake, SLEEP_PERIOD*sEP_Dev_Inf.sEP_Dev_HeartBeat.heartbeat_value , vAppWakeCallBack  );
	}
	else
	{

	}


}

PUBLIC void App_BusyConfig(bool appstatus)
{
	app_busy_status=appstatus;
}

PUBLIC bool App_BusyRead(void)
{
	return(app_busy_status);
}


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
PUBLIC void vAppWakeCallBack(void)
{
    DBG_vPrintf(SLEEP_INFO, "vAppWakeCallBack & Poll\n");

    /*Start the APP_TickTimer to continue the ZCL tasks */
    if (OS_eGetSWTimerStatus(APP_TickTimer) != OS_E_SWTIMER_RUNNING)
    {
        OS_eStartSWTimer(APP_TickTimer, ZCL_TICK_TIME, NULL);
    }

    if(sDeviceDesc.eNodeState==E_RUNNING)
    {


    	//if(ZPS_eAplZdoPoll()!=ZPS_E_SUCCESS)
    	//{
    	//	OS_eStartSWTimer(APP_PollTimer, 500, NULL);
    	//}
    	//else
    	//{
    		if(wakeup_way==WEAK_TIME)
    		{
    			if(sEP_Dev_Inf.M_ClusterID==0xFFFF)
    			{
    				app_UartSendMesg(APP_U_EVENT_READ_DEV_INFO);
    			}
    			else
    			{
#ifdef KEY_CON_ENABLE
    				dev_cluster_t.dev_state=0xAA;
    				dev_cluster_t.dev_power_state=0x45;
    				app_UartSendMesg(APP_U_EVENT_ESEND_DATA);
#else
    				app_UartSendMesg(APP_U_EVENT_READ_DEV_STATUS);
#endif
    			}
    			//如果是从定时醒来，则发送读取设备状态的消息
    		}
    		else
    		{
#ifdef KEY_CON_ENABLE
    			dev_cluster_t.dev_state=0x55;
    			dev_cluster_t.dev_power_state=0x45;
    			app_UartSendMesg(APP_U_EVENT_ESEND_DATA);
    			//正常消息
#else
#endif
    			wakeup_way=WEAK_TIME;
    		}
    		OS_eActivateTask(APP_taskuart);
    	//}
    	OS_eStartSWTimer (APP_PollTimer, APP_TIME_MS(300), NULL);//开启数据轮训
    }
    else if(sDeviceDesc.eNodeState==E_STARTUP)
    {
    	app_StartJoinConfig(FALSE);
    	//
    }
    else if(sDeviceDesc.eNodeState==E_REJOINING)
    {
    	vStartRejoinProcess();
    	//重新连接
    }

    App_BusyConfig(TRUE); //设定APP的忙状态
    OS_eStartSWTimer (APP_ActiveTime, APP_TIME_MS(ACTIVE_TIME), NULL);

#ifdef VMS
    OS_eActivateTask(APP_PIRTask);
#endif
}

/****************************************************************************
 *
 * NAME: vCheckForSleep
 *
 * DESCRIPTION:
 * Checks the number of polls without data, then schedules sleep if
 * conditions met
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vCheckForSleep( uint8 * u8NoDataCount )
{
    teCLD_IASZone_State   eZoneState;
    /* If we are enrolled, we can start checking for sleep */
    eZCL_ReadLocalAttributeValue(
                             1,                                     /*uint8                      u8SrcEndpoint,*/
                             SECURITY_AND_SAFETY_CLUSTER_ID_IASZONE,/*uint16                     u16ClusterId,*/
                             TRUE,                                  /*bool                       bIsServerClusterInstance,*/
                             FALSE,                                 /*bool                       bManufacturerSpecific,*/
                             FALSE,                                 /*bool_t                     bIsClientAttribute,*/
                             E_CLD_IASZONE_ATTR_ID_ZONE_STATE,      /*uint16                     u16AttributeId,*/
                             &eZoneState);
    DBG_vPrintf(SLEEP_INFO, "Check for sleep Enrrolled %d ", eZoneState);
    if(1)//eZoneState == E_CLD_IASZONE_STATE_ENROLLED
    {
        *u8NoDataCount += 1;
        DBG_vPrintf(SLEEP_INFO, "poll count %d\n", *u8NoDataCount);

        /* Do not sleep if Pinging Parent wait for atleast 3 polls */
        if( (TRUE == bPingSent) && (FALSE == bPingRespRcvd))
        {
            DBG_vPrintf(SLEEP_INFO, "\r\n%d Ping sent",u8PingCount);
            if(u8PingCount >= MAX_PINGS_NO_RSP)  //5次认证不成功，就进行重新连接
            {
                DBG_vPrintf(SLEEP_INFO, "\r\nRejoin");
                ZPS_eAplZdoRejoinNetwork(FALSE);
                bPingSent = FALSE;
            }else
            {
                if(u8PingCount < MAX_PINGS_NO_RSP)
                {
                    if(u8PingCount != 0)
                    {
                        /* Increment ping time so that we definitely ping the parent */
                        vIncrementPingTime(PING_PARENT_TIME);
                        /* Ping Parent for rejoin in case of Child ageing */
                        bPingParent();
                    }
                    else
                    {
                        OS_eStartSWTimer(APP_PollTimer, APP_TIME_MS(1000), NULL);
                    }
                }
                u8PingCount += 1;
            }
        }else
        {
            if( *u8NoDataCount >= MAX_POLLS_NO_DATA )
            {
                DBG_vPrintf(SLEEP_INFO, "\r\nNo Data Polls Exceeded");
                *u8NoDataCount = 0;
                vScheduleSleep();
            }
        }
    }
    DBG_vPrintf(SLEEP_INFO, "\n");
}

/****************************************************************************
 *
 * NAME: vCheckForSleepPostTx
 *
 * DESCRIPTION:
 * Checks for sleep after transmission
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vCheckForSleepPostTx( void )
{
    teCLD_IASZone_State   eZoneState;
    /* If we are enrolled, we can start checking for sleep */
    eZCL_ReadLocalAttributeValue(
                             1,                                     /*uint8                      u8SrcEndpoint,*/
                             SECURITY_AND_SAFETY_CLUSTER_ID_IASZONE,/*uint16                     u16ClusterId,*/
                             TRUE,                                  /*bool                       bIsServerClusterInstance,*/
                             FALSE,                                 /*bool                       bManufacturerSpecific,*/
                             FALSE,                                 /*bool_t                     bIsClientAttribute,*/
                             E_CLD_IASZONE_ATTR_ID_ZONE_STATE,      /*uint16                     u16AttributeId,*/
                             &eZoneState);
    DBG_vPrintf(SLEEP_INFO, "Check for sleep Enrrolled Post TX %d ", eZoneState);

    if(eZoneState == E_CLD_IASZONE_STATE_ENROLLED)
    {
        ZPS_eAplZdoPoll();
    }
}

/****************************************************************************
 *
 */
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
