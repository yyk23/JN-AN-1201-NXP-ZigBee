/******************************************************************
 * app_cmd_handle.c
 *
 * 此文件主要对CJP协议的相关命令进行处理函数的集合
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
#include "app_CIE_uart.h"
#include "app_data_handle.h"
#include "app_cmd_handle.h"


PUBLIC CJP_Status fCJP_Tx_Coor(CJP_CmdID cmd_coor , uint8 *pdata , uint8 len)
{
	sCJP_Head *   CJP_Head;
	CJP_Status    status;
	CJP_Head=(sCJP_Head *)Uart_STxBuf ;
	CJP_Head->u16FSeq = Frame_Seq ;
	CJP_Head->u8CType = C_ZIGBEE_DEV ;
	CJP_Head->u8FrType = F_JNI_COOR ;
	CJP_Head->u64Mac = 0 ;
	CJP_Head->u8EPAddr = PORT_NUM ;
	CJP_Head->u16ProfileID =  PROFILE_ID ;
	CJP_Head->u16ClusterID = 0 ;
	CJP_Head->u8CmdID = cmd_coor;
	CJP_Head->u8DataLen = len ;

	memcpy(&(Uart_STxBuf[CJP_HEAD_LEN]), pdata , len);
	if(len>UART_TX_MAX_NUM)
		return CJP_ERROR;
	else
		status=CJP_TxData(CJP_HEAD_LEN+len);

	return status ;

}


PUBLIC CJP_Status fBuildNet(uint8 channel,uint16 panid)
{
	uint8 sdata[3];
	CJP_Status status=CJP_SUCCESS;
	sdata[0] = 0x01;
	sdata[1] = E_ZCL_UINT8;

	if((channel>26)||(channel<11))
		sdata[2] = CJP_ERROR;
	else
		sdata[2] = CJP_SUCCESS;

	status=fCJP_Tx_Coor( CJP_BUILD_NET_RESP, sdata , 3);
	//开始建网
	ZPS_eAplZdoStartStack();
	return status;

}


PUBLIC void fBuildNet_Notice(CJP_Status status)
{
	sBuildNet_Notice  BuildNetNotice;

	BuildNetNotice.u8Status = status;
	BuildNetNotice.u16PanID = ZPS_u16AplZdoGetNetworkPanId(); //获取PANID
	BuildNetNotice.u8Channel = ZPS_u8AplZdoGetRadioChannel(); //获取CHannelID
	fCJP_Tx_Coor( CJP_BUILD_NET_NOTICE, (uint8 *)&BuildNetNotice , sizeof(sBuildNet_Notice));

}


PUBLIC CJP_Status fJoinNet_Set(uint8 time)
{
	uint8 sdata[3];
	sdata[0] = 0x01;
	sdata[1] = E_ZCL_UINT8;

	if(time>60)//限制时间不能大于60秒
	{
		time = 60 ;
	}
	if(ZPS_eAplZdoPermitJoining(time)!= 0)
		sdata[2] = CJP_ERROR;
	else
		sdata[2] = CJP_SUCCESS;

	return fCJP_Tx_Coor( CJP_JOIN_NET_SET_RESP , sdata , 3);
}


PUBLIC CJP_Status fDel_Dev(uint64 mac)
{
	uint8 sdata[13];
	sdata[0] = 2;

	sdata[1] = E_ZCL_UINT8;
	sdata[2] = CJP_SUCCESS;
	sdata[3] = E_ZCL_OSTRING;
	sdata[4] = 8;
	memcpy(&sdata[5],&mac , sdata[4]);
	if(ZPS_u16AplZdoLookupAddr(mac)==0x0000){
		sdata[2] = CJP_ERROR;
	}
	else{
		ZPS_eAplZdoLeaveNetwork(mac,FALSE,FALSE);//地址
		ZPS_bAplZdoTrustCenterRemoveDevice(mac);//从信任中心移除设备信息
		ZPS_vRemoveMacTableEntry(mac); //在MAC地址表中移除设备
	}

	return fCJP_Tx_Coor( CJP_DEL_DEV_RESP , sdata , 13);

}

PUBLIC CJP_Status fDev_Join(uYcl ycl)
{
	/*
	 * 1、更改设备链接密钥
	 * 2、允许设备入网
	 */
	usLinkKey   linkkey_tmp;
	linkkey_tmp = Linkkey_Calculate(ycl);//计算链接密钥
	ZPS_vAplSecSetInitialSecurityState(ZPS_ZDO_PRECONFIGURED_LINK_KEY, (uint8 *)&linkkey_tmp, 0x00, ZPS_APS_GLOBAL_LINK_KEY);//设置链接密钥
	ZPS_eAplZdoPermitJoining(PERMIT_JOIN_TIME);
}

PUBLIC CJP_Status fReset_Def_Set(void)
{
	//删除所有内容，清空存储器EEPROM
	PDM_vDeleteAllDataRecords();//
	vAHI_SwReset();//复位
	return CJP_SUCCESS;
}

PUBLIC CJP_Status fRead_Coor_inf(void)
{
	sReadCoorInf_Resp readcoorinf_resp;
	memcpy(&readcoorinf_resp.C_Sofe_Ver,&CIE_soft_ver ,sizeof(uSoft_Ver));
	memcpy(&readcoorinf_resp.C_YCL,&CIE_Ycl ,sizeof(uYcl));
	return fCJP_Tx_Coor( CJP_READ_COOR_DEV_INF_RESP , (uint8 *)&readcoorinf_resp , sizeof(sReadCoorInf_Resp));
}

/*
 * 向低功耗终端设备发送消息时，不能协调器直接向终端发送数据。协调器向终端发送数据的流程为:
 * 1、终端退出睡眠。
 * 2、终端主动向协调器poll消息
 * 3、协调器接收到终端的poll消息时，将数据传输到终端
 * 根据以上可知，协调器向终端发送消息时，需要将发送的消息进行缓冲，等到协调器接收到终端的poll消息时，再将缓冲区的数据取出，发送到终端。
 * 设计思路：在协调器申请一个数组变量当做缓冲区，此缓冲区存放JNI层发送过来的数据域数据和MAC、ClusterID等，
 */

PUBLIC CJP_Status fRead_End_inf(uint64 mac)
{
	if(ZPS_u16AplZdoLookupAddr(mac)==0x0000){
		return  CJP_ERROR;
	}
	//向终端发送读取基本信息消
	return fEnd_Read_BasicinfReq( mac);

}

PUBLIC CJP_Status fCoor_Self_Test(void)
{
	return CJP_SUCCESS;
	//协调器自检部分
}


PUBLIC CJP_Status fEnd_Read_BasicinfReq( uint64 mac)
{
	uint16 cluster = BASIC_CLUSTER;
	uint8  attrs[9]={4,E_ZCL_UINT8 , 0xF0 ,E_ZCL_UINT8 ,0xF1,E_ZCL_UINT8 ,0xF2,E_ZCL_UINT8 , 0xF3};//基本信息的属性表

	fEnd_Read_AttrsReq( mac , cluster , 8 , attrs);
	return CJP_SUCCESS;
}


/*
 * 读属性处理函数
 * mac ：目的地址
 * cluster：目的clusterID
 * len    ： 数据域长度
 * indata ：数据域指针
 */

PUBLIC CJP_Status fEnd_Read_AttrsReq( uint64 mac , uint16 cluster , uint8 len , uint8 * indata )
{
	//判断mac是否存在,
	//判断clusterID是否存在
	//将输入的数据进行转换为要读的属性列表
	//将属性列表按照转换模型转换为Zigbee属性ID
	//组织数据进行发送
	uint8 i = 0;
	uint8 attrsnum=*indata;//读取的属性个数
	ZPS_tsAfProfileDataReq psProfileDataReq1;
	static uint8 sqen=1;
	volatile uint16 u16PayloadSize=0;
	PDUM_thAPduInstance hAPduInst;
	if(ZPS_u16AplZdoLookupAddr(mac)==0x0000){
				return  CJP_ERROR;
			}
	if(attrsnum>10||attrsnum==0){
		return CJP_ERROR;
	}

	psProfileDataReq1.uDstAddr.u64Addr = mac;
	psProfileDataReq1.u16ClusterId = cluster;//
	psProfileDataReq1.u16ProfileId = PROFILE_ID;
	psProfileDataReq1.u8SrcEp = PORT_NUM;
	psProfileDataReq1.eDstAddrMode= ZPS_E_ADDR_MODE_IEEE;
	psProfileDataReq1.u8DstEp = PORT_NUM;
	psProfileDataReq1.eSecurityMode=ZPS_E_APL_AF_UNSECURE;
	psProfileDataReq1.u8Radius= 0;
	hAPduInst= PDUM_hAPduAllocateAPduInstance(apduZCL);
	if(hAPduInst == NULL){
				/*申请内存不成功*/
		return CJP_ERROR;

	}
	else{

		sqen = u8GetTransactionSequenceNumber();
		u16PayloadSize = u16ZCL_WriteCommandHeader(hAPduInst,
			                   	   	   	   	   	 eFRAME_TYPE_COMMAND_ACTS_ACCROSS_ENTIRE_PROFILE,//统一的命令格式
			        		                     FALSE,
			        		                     ZCL_MANUFACTURER_CODE,
			        		                     TRUE,
			        		                     TRUE,
			        		                     &sqen,
			        		                     E_ZCL_READ_ATTRIBUTES);  //读属性
		// write payload, attribute at a time
		for(i=0; i<attrsnum; i++){
			u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "h",(uint16)*(indata+1+i*2));//属性ID
		}
		PDUM_eAPduInstanceSetPayloadSize(hAPduInst, u16PayloadSize);
		ZPS_eAplAfApsdeDataReq(hAPduInst,(ZPS_tsAfProfileDataReq*)&psProfileDataReq1,&sqen);
	}
	return CJP_SUCCESS;
}


/*
 * 写属性处理函数
 * mac ：目的地址
 * cluster：目的clusterID
 * len    ： 数据域长度
 * indata ：数据域指针
 */

PUBLIC CJP_Status fEnd_Write_AttrsReq( uint64 mac , uint16 cluster ,  uint8 len , uint8 * indata )
{
	uint8 i = 0, j = 0 ,u8stringlen=0, type_addr_offset=0;
	uint8 attrsnum = *indata;
	ZPS_tsAfProfileDataReq psProfileDataReq1;
	static uint8 sqen=1;
	volatile uint16 u16PayloadSize=0;
	PDUM_thAPduInstance hAPduInst;
	if(ZPS_u16AplZdoLookupAddr(mac)==0x0000){
				return  CJP_ERROR;
			}
	if((attrsnum>10)||(attrsnum==0)||(attrsnum%2!=0)){
		return CJP_ERROR;
	}

	psProfileDataReq1.uDstAddr.u64Addr = mac;
	psProfileDataReq1.u16ClusterId = cluster;//
	psProfileDataReq1.u16ProfileId = PROFILE_ID;
	psProfileDataReq1.u8SrcEp = PORT_NUM;
	psProfileDataReq1.eDstAddrMode=ZPS_E_ADDR_MODE_IEEE;
	psProfileDataReq1.u8DstEp = PORT_NUM;
	psProfileDataReq1.eSecurityMode=ZPS_E_APL_AF_UNSECURE;
	psProfileDataReq1.u8Radius=0;
	hAPduInst=PDUM_hAPduAllocateAPduInstance(apduZCL);
	if(hAPduInst == NULL){
					/*申请内存不成功*/
		return CJP_ERROR;

	}
	else{

		sqen = u8GetTransactionSequenceNumber();
		u16PayloadSize = u16ZCL_WriteCommandHeader(	hAPduInst,
				                   	   	   	   	   	eFRAME_TYPE_COMMAND_ACTS_ACCROSS_ENTIRE_PROFILE,//统一的命令格式
				        		                    FALSE,
				        		                    ZCL_MANUFACTURER_CODE,
				        		                    TRUE,
				        		                    TRUE,
				        		                    &sqen,
				        		                    E_ZCL_WRITE_ATTRIBUTES_UNDIVIDED);  //写属性
	}
	for(i=0 ; i<(attrsnum/2) ; i++){
		//属性ID
		u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "h",(uint16)*(indata+1+1+type_addr_offset));//属性ID
		//属性数据类型
		u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "b",*(indata+1+1+1+type_addr_offset));//属性ID
		//属性值
		switch(*(indata+1+1+1+type_addr_offset)){
		case E_ZCL_OSTRING:
			u8stringlen = *(indata+1+type_addr_offset+1);
			if((u8stringlen==0)||(u8stringlen>UART_RX_DATA_MAX_NUM)){
				return CJP_ERROR;
			}

			u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "b",u8stringlen);//属性ID
			type_addr_offset++;
			break;

		default:
			eZCL_GetAttributeTypeSize((teZCL_ZCLAttributeType)*(indata+1+type_addr_offset) , &u8stringlen);	//获取长度
			if((u8stringlen==0)||(u8stringlen>UART_RX_DATA_MAX_NUM)){
				return CJP_ERROR;
			}
			break;
		}
		for(j=0;j<u8stringlen;j++){
			u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "b",*(indata+1+type_addr_offset+1+j));//属性ID
		}
		type_addr_offset+=u8stringlen+3;
	}
	PDUM_eAPduInstanceSetPayloadSize(hAPduInst, u16PayloadSize);
	ZPS_eAplAfApsdeDataReq(hAPduInst,(ZPS_tsAfProfileDataReq*)&psProfileDataReq1,&sqen);
	return CJP_SUCCESS;
}


/*
 * 读属性处理函数
 * mac ：目的地址
 * cluster：目的clusterID
 * indata ：数据域指针（暂时没用）
 */
PUBLIC CJP_Status fEnd_Alarm_ReportResp( uint64 mac,uint16 cluster ,uint8 len , uint8 * indata )
{
	//通知终端不要发送报警数据了。
	fEnd_Write_AttrsReq( mac , cluster ,  len , indata );
	return CJP_SUCCESS;
}









