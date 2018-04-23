/*****************************************************************************
 *
 * MODULE:             JN-AN-1201
 *
 * COMPONENT:          zha_CIE_node.c
 *
 * DESCRIPTION:        CIE node (Implementation)
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
#include "stdlib.h"
#include "stdio.h"
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
#include "app_common.h"
#include "app_events.h"
#include "app_zcl_CIE_task.h"
#include "zha_CIE_node.h"
#include "ha.h"
#include "haEzJoin.h"
#include "PDM_IDs.h"
#include "zcl_options.h"
#include "app_zbp_utilities.h"
#include "zcl_common.h"
#include "app_buttons.h"
#include "app_pdm.h"
#include "AppHardwareApi.h"
#include "LcdDriver.h"
#include "LcdFont.h"
#include "IASWD.h"
#include "IASACE.h"
#include "app_zone_client.h"
//#include "app_CIE_display.h"
#include "GenericBoard.h"
#include "app_CIE_save.h"
#include "app_CIE_uart.h"
#include "app_cmd_handle.h"
#include "app_data_handle.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifndef DEBUG_CIE_NODE
    #define TRACE_CIE_NODE   FALSE
#else
    #define TRACE_CIE_NODE   TRUE
#endif

/*DIO2 LED indication for Permit join True */
#define LED_PERMIT_JOIN 0x00000008UL   /* using DIO3 */

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

//PRIVATE void app_vStartNodeFactoryNew(void);
PRIVATE void vHandleStartUp( ZPS_tsAfEvent *pZPSevent );
PRIVATE void vHandleRunningEvent(ZPS_tsAfEvent *sStackEvent);
PRIVATE void vDeletePDMOnButtonPress(uint8 u8ButtonDIO);
PRIVATE void vInitDR1174LED(void);
PRIVATE void vHandleStackEvent( ZPS_tsAfEvent sStackEvent );

PRIVATE bool_t bTransportKeyDecider(uint16 u16ShortAddr, uint64 u64DeviceAddress,
  uint64 u64ParentAddress, uint8 u8Status);
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern uint8 u8Flash;

uint8 join_way=0 ;

PUBLIC  uint8 s_au8LnkKeyArray[16] = {0x5a, 0x69, 0x67, 0x42, 0x65, 0x65, 0x41, 0x6c,
        0x6c, 0x69, 0x61, 0x6e, 0x63, 0x65, 0x30, 0x39};
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
tsDeviceDesc sDeviceDesc;
PDM_tsRecordDescriptor sDevicePDDesc;
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: vPermitJoinIndication
 *
 * DESCRIPTION:
 * LED Indication
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vPermitJoinIndication(void)
{
    if(ZPS_bGetPermitJoiningStatus())
    {
        vAHI_DioSetOutput(0,LED_PERMIT_JOIN);
    }
    else
    {
        vAHI_DioSetOutput(LED_PERMIT_JOIN,0);

    }
}

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
    /* If required, at this point delete the network context from flash, perhaps upon some condition
     * For example, check if a button is being held down at reset, and if so request the Persistent
     * Data Manager to delete all its records:
     * e.g. bDeleteRecords = vCheckButtons();
     * Alternatively, always call PDM_vDelete() if context saving is not required.
     */
	uint8 tchannel=0 ;
	uint16	bytenum=0;
	PDM_teStatus tchannel_pdm=0;
    // APP_bButtonInitialise();
     vDeletePDMOnButtonPress(APP_BUTTONS_BUTTON_1);

    /* Restore any application data previously saved to flash */
    /* Restore device state previously saved to flash */
    eRestoreDeviceState();
  // ZPS_vTCSetCallback(bTransportKeyDecider);//设备入网回调函数，主要是避免
    /*Load the IAS ACE Cluster from EEPROM */
    vLoadIASCIEFromEEPROM(CIE_EP);

    /* Initialise ZBPro stack */
    ZPS_vAplSecSetInitialSecurityState(ZPS_ZDO_PRECONFIGURED_LINK_KEY, (uint8 *)&s_au8LnkKeyArray, 0x00, ZPS_APS_GLOBAL_LINK_KEY);

	/*Fix the channel for testing purpose*/
	tchannel_pdm = PDM_eReadDataFromRecord(   PDM_ID_CIE_CHANNEL, &tchannel,sizeof(uint8), &bytenum);

	DBG_vPrintf(TRACE_APP_UART,"\ntchannel_pdm= 0x%02x\n",tchannel_pdm);
	DBG_vPrintf(TRACE_APP_UART,"\nset channel = 0x%02x\n",tchannel);
	ZPS_eAplAibSetApsChannelMask(1<<18);
	if((tchannel>=0x0B) &&(tchannel<=0x1A))
	{
	  DBG_vPrintf(TRACE_APP_UART,"\nCurrent Channel = 0x%08x\n",ZPS_psAplAibGetAib()->apsChannelMask);
	  ZPS_eAplAibSetApsChannelMask(1<<tchannel);
	  DBG_vPrintf(TRACE_APP_UART,"\nCurrent Channel = 0x%08x\n",ZPS_psAplAibGetAib()->apsChannelMask);
	}

    /* Store channel mask */
    vEZ_RestoreDefaultAIBChMask();

    /* Initialise ZBPro stack */
    ZPS_eAplAfInit();

    /*Set Save default channel mask as it is going to be manipulated */
    vEZ_SetDefaultAIBChMask();

    /*Fix the channel for testing purpose*/
    tchannel_pdm = PDM_eReadDataFromRecord(   PDM_ID_CIE_CHANNEL, &tchannel,sizeof(uint8), &bytenum);

    DBG_vPrintf(TRACE_APP_UART,"\ntchannel_pdm= 0x%02x\n",tchannel_pdm);
    DBG_vPrintf(TRACE_APP_UART,"\nset channel = 0x%02x\n",tchannel);
    ZPS_eAplAibSetApsChannelMask(1<<18);
    if((tchannel>=0x0B) &&(tchannel<=0x1A))
    {
       DBG_vPrintf(TRACE_APP_UART,"\nCurrent Channel = 0x%08x\n",ZPS_psAplAibGetAib()->apsChannelMask);
       ZPS_eAplAibSetApsChannelMask(1<<tchannel);
       DBG_vPrintf(TRACE_APP_UART,"\nCurrent Channel = 0x%08x\n",ZPS_psAplAibGetAib()->apsChannelMask);
    }

    /* Initialise ZCL */
    APP_ZCL_vInitialise();

    /*Verify Load for the EEPROM */
    vVerifyIASCIELoad(CIE_EP);

    if (E_RUNNING == eGetNodeState())
    {
        app_vRestartNode();

    }
    else
    {
        app_vStartNodeFactoryNew();
    }
    ZPS_eAplZdoPermitJoining(0xff);
    //初始化串口
    Uart_Task_Init();
}

/****************************************************************************
 *
 * NAME: APP_taskCIE
 *
 * DESCRIPTION:
 * Main state machine
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_taskCIE)
{
    ZPS_tsAfEvent sStackEvent = {0};
    APP_tsEvent sAppButtonEvent = {0};

    /* The button event for the SSL bulbs */
    if (OS_eCollectMessage(APP_msgEvents, &sAppButtonEvent) == OS_E_OK)
    {

        DBG_vPrintf(TRACE_CIE_NODE,"\n\nButton Q\n EventType = %d\n", sAppButtonEvent.eType);
        DBG_vPrintf(TRACE_CIE_NODE," Button = %d\n", sAppButtonEvent.uEvent.sButton.u8Button);
        //vHandleButtonPress(sAppButtonEvent.eType,sAppButtonEvent.uEvent.sButton.u8Button);
    }

    /*Collect stack Events */
    if ( OS_eCollectMessage(APP_msgZpsEvents, &sStackEvent) == OS_E_OK)
    {
    	//处理通过ZigBee网络接收到的所有数据
    	 vHandleStackEvent( sStackEvent );

    }


    /* Handle events depending on node state */
    switch (sDeviceDesc.eNodeState)
    {
        case E_STARTUP:
            DBG_vPrintf(TRACE_CIE_NODE, "\nE_STARTUP" );
            vHandleStartUp(&sStackEvent);
            break;

        case E_RUNNING:
            DBG_vPrintf(TRACE_CIE_NODE, "E_RUNNING\r\n");
            vHandleRunningEvent(&sStackEvent);
            break;

        default:
            DBG_vPrintf(TRACE_CIE_NODE, "ERR: Unknown State %d\n", sDeviceDesc.eNodeState );
            break;
    }

    /* Global clean up to make sure any APDUs have been freed   */
    if (sStackEvent.eType == ZPS_EVENT_APS_DATA_INDICATION)
    {
        PDUM_eAPduFreeAPduInstance(sStackEvent.uEvent.sApsDataIndEvent.hAPduInst);//释放PDUM
    }
}

/****************************************************************************
 *
 * NAME: vDisplayStackEvent
 *
 * DESCRIPTION:
 * Display function only, display the current stack event before each state
 * consumes  处理协议栈的消息，如设备入网退网建网等，还有ZDO 层的Cluster处理。
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE  void vHandleStackEvent( ZPS_tsAfEvent sStackEvent )
{
    DBG_vPrintf(TRACE_CIE_NODE, "\nAPP_ZPR_Light_Task event:%d",sStackEvent.eType);

    switch (sStackEvent.eType)
    {
        case ZPS_EVENT_APS_DATA_INDICATION:
            DBG_vPrintf(TRACE_CIE_NODE, "\nData Ind: Profile :%x Cluster :%x EP:%x",
                                          sStackEvent.uEvent.sApsDataIndEvent.u16ProfileId,
                                          sStackEvent.uEvent.sApsDataIndEvent.u16ClusterId,
                                          sStackEvent.uEvent.sApsDataIndEvent.u8DstEndpoint);//目的地址

        break;

        case ZPS_EVENT_NWK_STATUS_INDICATION:
            DBG_vPrintf(TRACE_APP_UART, "\nNwkStat: Addr:%x Status:%x",
                sStackEvent.uEvent.sNwkStatusIndicationEvent.u16NwkAddr,
                sStackEvent.uEvent.sNwkStatusIndicationEvent.u8Status);
        break;

        case ZPS_EVENT_NWK_STARTED:
        	fBuildNet_Notice(CJP_SUCCESS) ;
            DBG_vPrintf(TRACE_APP_UART, "\nZPS_EVENT_NWK_STARTED\n");
            ZPS_eAplZdoPermitJoining(0xff);
        break;

        case ZPS_EVENT_NWK_FAILED_TO_START:
        	//建网失败
        	fBuildNet_Notice(CJP_ERROR) ;
            DBG_vPrintf(TRACE_APP_UART, "\nZPS_EVENT_NWK_FAILED_TO_START\n");
        break;

        case ZPS_EVENT_NWK_NEW_NODE_HAS_JOINED:
        	//新设备入网
        	ZPS_eAplZdoPermitJoining(0x00);//关闭入网许可
        	//入网成功 后删除当前的链接密钥
        //	ZPS_eAplZdoRemoveLinkKey(sStackEvent.uEvent.sNwkJoinIndicationEvent.u64ExtAddr);
        	join_way = sStackEvent.uEvent.sNwkJoinIndicationEvent.u8Rejoin;//设备的入网方式 ：Join(0)  or rejoin(1)

            DBG_vPrintf(TRACE_APP_UART, "\nZPS_EVENT_NWK_NEW_NODE_HAS_JOINED\n");
        break;
        case ZPS_EVENT_NWK_LEAVE_INDICATION:
        	DBG_vPrintf(TRACE_APP_UART, "\ ZPS_EVENT_NWK_LEAVE_INDICATION\n");//
        	if( fDev_Leave_Notice(sStackEvent.uEvent.sNwkLeaveIndicationEvent.u64ExtAddr)==CJP_SUCCESS)
        	{
        		DBG_vPrintf(TRACE_APP_UART, "A device leave net success\n");//
        	}
        	else
        	{
        		DBG_vPrintf(TRACE_APP_UART, "A device leave net error\n");//
        	}

        	break;
        case ZPS_EVENT_NWK_FAILED_TO_JOIN:
        	//入网失败
            DBG_vPrintf(TRACE_CIE_NODE, "\nZPS_EVENT_NWK_FAILED_TO_JOIN - %x \n",sStackEvent.uEvent.sNwkJoinFailedEvent.u8Status);
        break;

        case ZPS_EVENT_ERROR:
            DBG_vPrintf(TRACE_CIE_NODE, "\nZPS_EVENT_ERROR\n");
            ZPS_tsAfErrorEvent *psErrEvt = &sStackEvent.uEvent.sAfErrorEvent;
            DBG_vPrintf(TRACE_CIE_NODE, "\nStack Err: %d", psErrEvt->eError);
        break;

        default:
            DBG_vPrintf(TRACE_CIE_NODE, "\nUnhandled Stack Event\n");
        break;
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
PUBLIC void app_vRestartNode (void)
{
    /* The node is in running state indicates that
     * the EZ Mode state is as E_EZ_SETUP_DEVICE_IN_NETWORK*/
    eEZ_UpdateEZState(E_EZ_DEVICE_IN_NETWORK);

    sDeviceDesc.eNodeState = E_RUNNING;

    /* Store the NWK frame counter increment */
    ZPS_vSaveAllZpsRecords();

    DBG_vPrintf(TRACE_APP_UART, "Restart Running\n");
    OS_eActivateTask(APP_taskCIE);

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
PUBLIC void app_vStartNodeFactoryNew(void)
{
    /* The node is in running state indicates that
     * the EZ Mode state is as E_EZ_SETUP_START*/
    vEZ_FormNWK();
    vEZ_SetUpPolicy(E_EZ_JOIN_ELSE_FORM_IF_NO_NETWORK);

    eEZ_UpdateEZState(E_EZ_START);

    DBG_vPrintf(TRACE_CIE_NODE, "\nRun and activate\n");
    vStartStopTimer( APP_StartUPTimer, APP_TIME_MS(500),(uint8*)&(sDeviceDesc.eNodeState),E_STARTUP );
    DBG_vPrintf(TRACE_CIE_NODE, "Start Factory New\n");
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
    /*Call The EZ mode Handler passing the events*/
    vEZ_EZModeNWKJoinHandler(pZPSevent,E_EZ_JOIN);
    ezState = eEZ_GetJoinState();
    DBG_vPrintf(TRACE_CIE_NODE, "EZ_STATE\%x r\n", ezState);
    if(ezState == E_EZ_DEVICE_IN_NETWORK)
    {
        DBG_vPrintf(TRACE_CIE_NODE, "HA EZMode E_EZ_SETUP_DEVICE_IN_NETWORK \n");
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
    /*Request response event, call the child age logic */
    if((ZPS_EVENT_APS_DATA_INDICATION == psStackEvent->eType) &&
        (0 == psStackEvent->uEvent.sApsDataIndEvent.u8DstEndpoint))
    {
    	//处理目的端点为0的消息
        ZPS_tsAfZdpEvent sAfZdpEvent;
        zps_bAplZdpUnpackResponse(psStackEvent,
                                  &sAfZdpEvent);

        DBG_vPrintf(TRACE_CIE_NODE, "APP: vCheckStackEvent: ZPS_EVENT_APS_ZDP_REQUEST_RESPONSE, Cluster = %x\n",    sAfZdpEvent.u16ClusterId );

        vHandleZDPReqResForZone(&sAfZdpEvent);
        if(ZPS_u16AplZdoGetNwkAddr() != psStackEvent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr)
        {
            vHandleDeviceAnnce(&sAfZdpEvent);
        }
    }

    if((ZPS_EVENT_APS_DATA_INDICATION == psStackEvent->eType) &&
           (1 == psStackEvent->uEvent.sApsDataIndEvent.u8DstEndpoint))
    {
    	//发送到端点1的数据处理函数


    }

    /* Mgmt Leave Received */
    if( ZPS_EVENT_NWK_LEAVE_INDICATION == psStackEvent->eType )
    {
        DBG_vPrintf(TRACE_CIE_NODE, "MgmtLeave\n" );
        /* add leave handling here */
    }

    if(ZPS_EVENT_NWK_NEW_NODE_HAS_JOINED == psStackEvent->eType )
    {
        DBG_vPrintf(TRACE_CIE_NODE, "New Node Joined\n" );
    }

    if (ZPS_EVENT_ERROR == psStackEvent->eType)
    {
        DBG_vPrintf(TRACE_CIE_NODE, "Error Type %d\n" , psStackEvent->uEvent.sAfErrorEvent.eError);
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

    /* If required, at this point delete the network context from flash, perhaps upon some condition
     * For example, check if a button is being held down at reset, and if so request the Persistent
     * Data Manager to delete all its records:
     * e.g. bDeleteRecords = vCheckButtons();
     * Alternatively, always call PDM_vDelete() if context saving is not required.
     */
    if(bDeleteRecords)
    {
        DBG_vPrintf(TRACE_CIE_NODE,"Deleting the PDM\n");
        PDM_vDeleteAllDataRecords();
    }
}


PRIVATE bool_t bTransportKeyDecider(uint16 u16ShortAddr, uint64 u64DeviceAddress,
 uint64 u64ParentAddress, uint8 u8Status)
{

}

/****************************************************************************
 *
 * NAME: vInitDR1174LED
 *
 * DESCRIPTION:
 * LED indication
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vInitDR1174LED(void)
{

    /*Enable Pull Ups : Not really required for the outputs */
    vAHI_DioSetPullup(0,LED_PERMIT_JOIN);

    /*Make the DIo as output*/
    vAHI_DioSetDirection(0,LED_PERMIT_JOIN);
}





/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
