/*
 * app_uart.c
 *
 *  Created on: 2017年10月23日
 *      Author: 1
 */


/*********************************************************************
 * CONSTANTS
 */
#include <jendefs.h>
#include <AppApi.h>
#include <pwrm.h>
#include <os.h>
#include "os_gen.h"
#include <dbg.h>
#include <dbg_uart.h>
#include <app_exceptions.h>
#include <app_pdm.h>
#include <zps_nwk_pub.h>
#include <zps_apl_af.h>
#include <zps_apl_aib.h>
#include "zps_apl_zdp.h"
#include "zps_nwk_nib.h"
#include "zps_gen.h"
#include <app_timer_driver.h>
#include "pdm.h"
#include "pdum_gen.h"
#include "pdum_apl.h"
#include "app_CIE_uart.h"
#include "AppHardwareApi.h"
#include "zcl.h"
#include "zcl_common.h"
#include "app_events.h"
#include "PDM_IDs.h"
#include "IASZone.h"
#include "ZQueue.h"
#include "CRC_Table.h"
#include "app_cmd_handle.h"
#include "app_data_handle.h"
#include "Array_list.h"


#define TRACE_APP_UART    TRUE
#define UART_APP_PORT  E_AHI_UART_1
#if !defined( UART_APP_BAUD )
#define UART_APP_BAUD  E_AHI_UART_RATE_9600
#endif


#define UART_RX_NULL               0
#define UART_RX_HEAD               1
#define UART_RX_DATA               2
#define UART_RX_CRC                3
#define UART_RX_BUSY               4






#define UART_TIMEOUT_VALUE         100

#define UART_DMA_RXBUF_LEN          255
#define UART_DMA_TXBUF_LEN          255

/**/
uint16 Frame_Seq=0xFFFF;
uint8  Uart_STxBuf[UART_TX_MAX_NUM+1];
uint16 Frame_SeqQueue[FRAME_SEQ_MAX_NUM+1];
uSoft_Ver CIE_soft_ver;
uYcl      CIE_Ycl;

sEnddev_BasicInf   Enddev_BasicInf[MAX_DEV_MANAGE_NUM];
sAttr_Model_Array  Attr_Model_Array[MAX_DEV_MODEL_NUM];
sCoor_Dev_manage   Coor_Dev_manage;
static tszQueue    APP_msgframe;
static tszQueue    APP_msgSerialRx;
/**/
static uint8  Uart_Analysis_Step = UART_RX_NULL;
static uint8  Uart_DMA_RxBuf[UART_DMA_RXBUF_LEN];
static uint8  Uart_DMA_TxBuf[UART_DMA_TXBUF_LEN];
//串口队列
static uint8  Uart_SRxQueue[UART_RX_MAX_NUM+1];//


static uint8  Uart_SRxBuf[UART_RX_MAX_NUM+1];//

static uint16 Uart_SRxLen = 0;
static uint8 s_eUart_state=0;



/*
 * 内部函数
 */
static void CJP_Analysis(void);
static uint16 CRC_Calculate(uint8 *pBuf, uint8 len);
static void  CJP_TxCMDData(uint8 cmd_type, uint8 value );
static CJP_Status CJP_RxCMDDeal(uint8 *rx_buf);
static uint16 HalUARTWrite(uint8 u8Uart,uint8 *pu8Data,uint16 u16DataLength);
static uint16 HalUARTRead( uint8 u8Uart,uint8 *pu8DataBuffer,uint16 u16DataBufferLength);
static void CIE_data_Init(void);

PUBLIC void Uart_Task_Init(void)
{
  User_Uart_Init();
  ZQ_vQueueCreate ( &APP_msgSerialRx, UART_RX_MAX_NUM  , sizeof ( uint8 ), (uint8*)Uart_SRxQueue );//初始化串口接收队列
  ZQ_vQueueCreate ( &APP_msgframe, FRAME_SEQ_MAX_NUM  , sizeof  ( uint16 ), (uint8*)Frame_SeqQueue );//初始化CJP 协议的帧序列号
  CIE_data_Init();
  Array_init(&alist,Enddev_BasicInf , MAX_DEV_MANAGE_NUM ,Coor_Dev_manage.dev_num);//初始化设备列表
}
/*
 * 串口初始化函数
 */
PUBLIC void User_Uart_Init(void)
{

  //是能串口

  bAHI_UartEnable(UART_APP_PORT,
  Uart_DMA_TxBuf,
  UART_DMA_TXBUF_LEN,
  Uart_DMA_RxBuf,
  UART_DMA_RXBUF_LEN);
  //设置波特率
  vAHI_UartSetBaudRate(UART_APP_PORT,UART_APP_BAUD);

  //设置校验位，数据位  停止位等
  vAHI_UartSetControl(UART_APP_PORT,
		  	  	  	  E_AHI_UART_ODD_PARITY,
		  	  	  	  E_AHI_UART_PARITY_DISABLE,
		  	  	  	  E_AHI_UART_WORD_LEN_8,
		  	  	  	  E_AHI_UART_1_STOP_BIT,
		  	  	  	  E_AHI_UART_RTS_LOW);


  //设置终端终端模式
  vAHI_UartSetInterrupt(UART_APP_PORT,
		  	  	  	  	FALSE,
		  	  	  	  	FALSE,
		  	  	  	  	FALSE,
		  	  	  	  	TRUE,
		  	  	  	  	E_AHI_UART_FIFO_LEVEL_1);
  OS_eStartSWTimer (APP_uart_timeout, APP_TIME_MS(10), NULL);
  OS_eStartSWTimer (APP_uart_cycle, APP_TIME_MS(1), NULL);

}

static void CIE_data_Init(void)
{
	CIE_Ycl.Num = 13;
	CIE_Ycl.Mac = ZPS_u64AplZdoGetIeeeAddr();
	CIE_Ycl.YCL_ID.YCL_ID = COOR_YCL_ID;
	CIE_soft_ver.Sv_YCL_ID = COOR_YCL_ID;
	CIE_soft_ver.Sv_Num=11;
	CIE_soft_ver.Sv_Mainv_Num = 0;
	CIE_soft_ver.Sv_Modv_Num = 0;
	CIE_soft_ver.Sv_Secv_Num =0;
	CIE_soft_ver.Sv_Dev_Date[0] =0;
}

/*
 * 串口任务
 */
OS_TASK(APP_CIE_taskuart)
{
	APP_uartEvent sUartEvent;
    /* check if any messages to collect */
	uData_test data_save;
	sCJP_Head  head_test;
	if (OS_E_OK != OS_eCollectMessage(APP_CIE_msgUartEvents, &sUartEvent))
	{
    	switch (sUartEvent.eType)
    	{
    	    /*发送数据*/

    	 	 default:
    	 		 break;

    	}

    }

    if (OS_eGetSWTimerStatus(APP_uart_timeout) == OS_E_SWTIMER_EXPIRED) //
    {
    	Uart_Analysis_Step = UART_RX_NULL;
    	DBG_vPrintf(TRACE_APP_UART, "uart test");
    	DBG_vPrintf(TRACE_APP_UART, "uart test %d",CJP_HEAD_LEN);
    	DBG_vPrintf(TRACE_APP_UART, "uart test %d",100);
    	DBG_vPrintf(TRACE_APP_UART, "segerm num %d",PDM_u8GetSegmentCapacity());
    	OS_eStopSWTimer (APP_uart_timeout);
    	OS_eStartSWTimer (APP_uart_timeout, APP_TIME_MS(1000), NULL);

    }

    if(OS_eGetSWTimerStatus(APP_uart_cycle) == OS_E_SWTIMER_EXPIRED)
    {
    	CJP_Analysis();
    	OS_eStopSWTimer (APP_uart_cycle);
    	OS_eStartSWTimer (APP_uart_cycle, APP_TIME_MS(1), NULL);

     }


    switch (s_eUart_state)
    {
       case 0:
    	  OS_eStartSWTimer (APP_uart_timeout, APP_TIME_MS(10), NULL);
    	  s_eUart_state =1;
    	  break;
       case 1 :
    	   break;
       default :
    	   break;

    }

}


PUBLIC void app_UartSendMesg(APP_uartEventType  type)
{
	APP_uartEvent sUartEvent;
    sUartEvent.eType = type;
    OS_ePostMessage(APP_CIE_msgUartEvents, &sUartEvent);//发送串口消息
    DBG_vPrintf(TRACE_APP_UART, "uart mesg %d ",type);
}





/*
 *读串口函数
 */
static uint16 HalUARTRead(uint8 u8Uart,uint8 *pu8Data,uint16 u16DataLength)
{
	uint16 i=0;
	if((u16DataLength>UART_RX_MAX_NUM)||(u16DataLength==0))
	{
		return 0;
	}
	for(u16DataLength;u16DataLength>0;u16DataLength--)
	{

		if(ZQ_bQueueReceive(&APP_msgSerialRx, pu8Data++ ))//出队
		{
			//只要是能够读到数值就重新计时。
			OS_eStopSWTimer (APP_uart_timeout);
			OS_eStartSWTimer (APP_uart_timeout, APP_TIME_MS(10), NULL);
			i++;
		}
		else
		{
			break;
		}
	}
	return i;
}

/*
 * 写串口函数
 */
static uint16 HalUARTWrite(uint8 u8Uart,
		uint8 *pu8Data,
		uint16 u16DataLength)
{
	  return u16AHI_UartBlockWriteData(u8Uart,
			  	  	  	  	  	  	   pu8Data,
			  	  	  	  	  	  	   u16DataLength);
}


/*
 * 串口中断函数
 */
OS_ISR(vISR_CIE_Uart1)
{
	uint32    u32ItemBitmap = ( ( *( (volatile uint32 *)( 0x02004000 + 0x08 ) ) ) >> 1 ) & 0x0007;
	uint8     u8Byte;
	if ( u32ItemBitmap & E_AHI_UART_INT_RXDATA )
	{
	      u8Byte = u8AHI_UartReadData ( UART_APP_PORT );
	      ZQ_bQueueSend ( &APP_msgSerialRx, &u8Byte );//入队
	}

}


//Uart回调解析函数
static void CJP_Analysis(void)
{
  uint8 rx_buf_num=0;
  uint8 *p_rx_buf=Uart_SRxBuf;
  sCJP_Head *   CJP_Head;
  switch( Uart_Analysis_Step)
  {
  	  case UART_RX_NULL:
  		  Uart_SRxLen = 0;
  		  Uart_Analysis_Step = UART_RX_HEAD;
  	  case UART_RX_HEAD:
  		  rx_buf_num = HalUARTRead(UART_APP_PORT, p_rx_buf+Uart_SRxLen, CJP_HEAD_LEN-Uart_SRxLen);
  		  Uart_SRxLen+=rx_buf_num;
  		  if(Uart_SRxLen>=CJP_HEAD_LEN)
  		  {
  			  vAHI_UartWriteData(UART_APP_PORT,0x02);
  			  CJP_Head=(sCJP_Head *)Uart_SRxBuf;
  			  if((CJP_Head->u8CType !=C_DEV_TYPE )||(CJP_Head->u8FrType > 1)||(CJP_Head->u8EPAddr !=PORT_NUM)||(CJP_Head->u8DataLen>UART_RX_DATA_MAX_NUM))
  			  {
  				  vAHI_UartWriteData(UART_APP_PORT,0x03);
  				  Uart_Analysis_Step = UART_RX_NULL;
  				  break;
  			  }
  			  Uart_Analysis_Step = UART_RX_DATA;
  			  vAHI_UartWriteData(UART_APP_PORT,0x04);

  		  }
  		  else
  		  {
  			  break;
  		  }
  	  case UART_RX_DATA:
  		  rx_buf_num = HalUARTRead(UART_APP_PORT, p_rx_buf+Uart_SRxLen, CJP_Head->u8DataLen + CJP_HEAD_LEN+2-Uart_SRxLen);
  		  Uart_SRxLen+=rx_buf_num;
  		  if(Uart_SRxLen >= (CJP_Head->u8DataLen + CJP_HEAD_LEN+2))
  		  {
  			  uint16 crcnum=0;
  			  crcnum=CRC_Calculate(p_rx_buf, CJP_Head->u8DataLen + CJP_HEAD_LEN);
  			  vAHI_UartWriteData(UART_APP_PORT,(uint8)(crcnum&0x00FF));
  			  vAHI_UartWriteData(UART_APP_PORT,(uint8)((crcnum&0xFF00)>>8));
  			  vAHI_UartWriteData(UART_APP_PORT,0x05);
  			  if(BUILD_UINT16(*(p_rx_buf+CJP_HEAD_LEN+CJP_Head->u8DataLen),*(p_rx_buf+CJP_HEAD_LEN+CJP_Head->u8DataLen+1))==CRC_Calculate(p_rx_buf, CJP_Head->u8DataLen + CJP_HEAD_LEN))
  			  {
  				  vAHI_UartWriteData(UART_APP_PORT,0x06);
  				  CJP_RxCMDDeal(p_rx_buf);
  				  CJP_TxCMDData(1, 2 );
  			  }
  			  vAHI_UartWriteData(UART_APP_PORT,0x07);
  			  Uart_Analysis_Step = UART_RX_NULL;
  			  break;

  		  }
  		  else
  		  {
  			  break;
  		  }
  	  case UART_RX_BUSY:
  		  break;
  	  default:
  		  break;
  }

}


/*
 * 按照CJP协议，处理JNI层的串口数据
 * rx_buf :有效帧的存储地址
 */
static CJP_Status CJP_RxCMDDeal(uint8 *rx_buf)
{
  uint8 *p_deal_buf=rx_buf;
  sCJP_Head *   CJP_Head;
  uint8 *p_data_head=rx_buf + CJP_HEAD_LEN;
  CJP_Head=(sCJP_Head *)p_deal_buf;

  if(Frame_Seq==CJP_Head->u16FSeq)
  {
	  return CJP_ERROR;
  }

  /*
   * 和终端的通信数据
   */
  if(CJP_Head->u8FrType==F_JNI_END)
  {
	  switch(CJP_Head->u8CmdID)
	  {
	  	  case CJP_END_NORMAL_DATA_REPORT_RESP:
	  		 //暂时没用
	  		  break;
	  	  case CJP_END_HEART_DATA_REPORT_RESP:
	  		  //暂时没用
	  		  break;
	  	  case CJP_END_READ_ATTR_REQ:
	  		return( fEnd_Read_AttrsReq( CJP_Head->u64Mac , CJP_Head->u16ClusterID , CJP_Head->u8DataLen , p_data_head ) );
	  		  break;
	  	  case CJP_END_WRITE_ATTR_REQ:
	  		return (fEnd_Write_AttrsReq( CJP_Head->u64Mac , CJP_Head->u16ClusterID , CJP_Head->u8DataLen , p_data_head ) );
	  		  break;
	  	  case CJP_END_ALARM_DATA_REPORT_RESP:
	  		  //查看报警回执
	  		return (fEnd_Alarm_ReportResp( CJP_Head->u64Mac , CJP_Head->u16ClusterID ,CJP_Head->u8DataLen , p_data_head ) );
	  		  break;

	  	  default :
	  		  break;

	  }

	  return CJP_SUCCESS;
  }


 /*
* 和协调器之间的通信
 */
  else if(CJP_Head->u8FrType==F_JNI_COOR)
  {
	  switch(CJP_Head->u8CmdID)
	  {

	  	  case CJP_BUILD_NET_REQ:
	  		  fBuildNet(*(uint8 *)(p_data_head+2),*(uint16 *)(p_data_head+4));
	  		  break;
	  	  case CJP_RESET_DEV_REQ:/*复位协调器*/
	  	  {
	  		vAHI_SwReset();
	  		break;
	  	  }
	  	  case CJP_JOIN_NET_SET_REQ:
	  		  fJoinNet_Set(*(uint8 *)(p_data_head+2));
	  		  break;
	  	  case CJP_DEL_DEV_REQ:
	  		  fDel_Dev(*(uint64 *)(p_data_head+2));
	  		//ZPS_eAplZdoLeaveNetwork(0,FALSE,FALSE);//地址
	  		break;
	  	  case CJP_DEV_JOIN_REQ:
	  		  fDev_Join(*(uYcl *)(p_data_head+2));
	  		  break;
	  	  case CJP_RESET_DEF_SET_REQ:
	  		  fReset_Def_Set();
	  		  break;
	  	  case CJP_READ_COOR_DEV_INF_REQ:
	  		 fRead_Coor_inf();
	  		  break;
	  	  case CJP_READ_END_DEV_INF_REQ:
	  		  fRead_End_inf(CJP_Head->u64Mac);
	  		  break;
	  	  case CJP_COOR_SELF_TEST_REQ:
	  		  fCoor_Self_Test();
	  		  break;
	  	  default:
	  		  break;
	  }

	  return CJP_SUCCESS;
  }
  return CJP_FTYPE_ERROR;

}


/*
 * 串口发送函数，
 * len:数据域长度，不包括帧头部分
 */
PUBLIC CJP_Status CJP_TxData(uint8 len)
{
	uint8 *tx_buf=Uart_STxBuf;
	if(len>UART_TX_MAX_NUM){
		return CJP_ERROR;
	}
	*(uint16 *)(tx_buf+len+CJP_HEAD_LEN)= CRC_Calculate(tx_buf, len+CJP_HEAD_LEN);
	HalUARTWrite(UART_APP_PORT,tx_buf, len+CJP_HEAD_LEN+2);
	return CJP_SUCCESS ;

}

/*
//函数功能：向串口发送命令数据
//cmd_type:命令类型
//value：数值
//波特率为9600，1ms传送不到1个字节
*/
static void  CJP_TxCMDData(uint8 cmd_type, uint8 value )
{
     uint8 *tx_buf=Uart_STxBuf;
     *(tx_buf+3)=cmd_type;
     *(tx_buf+(*(tx_buf+2)+2))= CRC_Calculate(tx_buf, (*(tx_buf+2))+2);
     HalUARTWrite(UART_APP_PORT,tx_buf, 10);
}


/*
//函数功能：CRC校验
//pBuf:数据指针
//len:数据长度
*/
static uint16 CRC_Calculate(uint8 *pBuf, uint8 len)
{
	uint16 crc = 0;
	uint8  da;
	while (len--)
	{
		da = crc >> 8;  // CRC(h)
	    crc <<= 8;
	    crc ^= crc_tab[da ^ *pBuf++];
	 }
	 return crc;
}


