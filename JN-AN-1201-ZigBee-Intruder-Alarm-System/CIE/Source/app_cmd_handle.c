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
#include "Array_list.h"
#include "app_CIE_save.h"
#include "app_cmd_handle.h"
#include "zha_CIE_node.h"


PUBLIC CJP_Status fCJP_Tx_Coor(uYcl ycl ,CJP_CmdID cmd_coor , uint8 *pdata , uint8 len)
{
	sCJP_Head *   CJP_Head;
	CJP_Status    status;
	CJP_Head=(sCJP_Head *)Uart_STxBuf ;
	CJP_Head->u16ASeq = AFrame_Seq;
	CJP_Head->u16FSeq = Frame_Seq ;
	CJP_Head->u8CType = C_ZIGBEE_DEV ;
	if((cmd_coor>=0x00) && (cmd_coor<=0x80))
	{
		CJP_Head->u8FrType = F_JNI_COOR ;
	}
	else
	{
		CJP_Head->u8FrType = F_JNI_END ;
	}
	CJP_Head->Ycl=ycl;
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
    uint16 pdm_id=0x8000;
	if((channel>26)||(channel<11))
		sdata[2] = CJP_ERROR;
	else
		sdata[2] = CJP_SUCCESS;

	status=fCJP_Tx_Coor(CIE_Ycl, CJP_BUILD_NET_RESP, sdata , 3);
	//开始建网
	if(ZPS_u8AplZdoGetRadioChannel()!=channel)
	{
		DBG_vPrintf(TRACE_APP_UART, "%02x",channel );
		DBG_vPrintf(TRACE_APP_UART, "%04x",panid );
		ZPS_eAplAibSetApsChannelMask(1<<channel);
		//保存信道数据
		PDM_eSaveRecordData( PDM_ID_CIE_CHANNEL,
		        					 &channel,
		        					 sizeof(uint8));

		while(pdm_id < 0xFFFF)
		{
			PDM_vDeleteDataRecord(pdm_id++);
		}
		vAHI_SwReset();//复位

	}
	else
	{
		fBuildNet_Notice(CJP_SUCCESS);
	}

	return status;

}


PUBLIC void fBuildNet_Notice(CJP_Status status)
{
	sBuildNet_Notice  BuildNetNotice;
	uint8 sdata[30],i=0;
	BuildNetNotice.u8Status = status;
	BuildNetNotice.u16PanID = ZPS_u16AplZdoGetNetworkPanId(); //获取PANID
	BuildNetNotice.u8Channel = ZPS_u8AplZdoGetRadioChannel(); //获取CHannelID

	sdata[i]= 0x03;
	i++;
	sdata[i] = E_ZCL_UINT8;
	i++;
	sdata[i] = BuildNetNotice.u8Status;
	i++;
	sdata[i] = E_ZCL_UINT16;
	i++;
	sdata[i] = (uint8)((BuildNetNotice.u16PanID>>8)&0x00FF);
	sdata[i] = (uint8)((BuildNetNotice.u16PanID)&0x00FF);
	i+=sizeof(uint16);
	sdata[i] = E_ZCL_UINT8;
	i++;
	sdata[i] = BuildNetNotice.u8Channel;
	i++;

	Frame_Seq++;//主动发送的序列号+1
	fCJP_Tx_Coor(CIE_Ycl, CJP_BUILD_NET_NOTICE, sdata , i);
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

	return fCJP_Tx_Coor(CIE_Ycl,  CJP_JOIN_NET_SET_RESP , sdata , 3);
}



PUBLIC CJP_Status fDel_Dev(uYcl ycl)
{
	uint8 sdata[20],i=0;
	sdata[i] = 2;
	i++;
	sdata[i] = E_ZCL_UINT8;
	i++;
	sdata[i] = CJP_SUCCESS;
	i++;
	sdata[i] = E_ZCL_OSTRING;
	i++;
	sdata[i] = sizeof(uYcl);
	i++;
	memcpy(&sdata[i],&ycl , sdata[4]);
	i+= sizeof(uYcl);
	if(ZPS_u16AplZdoLookupAddr(ycl.sYCL.Mac)==0x0000){
		sdata[2] = CJP_ERROR;
	}
	else{
		dele_dev_data_manage(ycl); //删除设备列表信息
		ZPS_eAplZdoLeaveNetwork(ycl.sYCL.Mac,FALSE,FALSE);//地址
		ZPS_bAplZdoTrustCenterRemoveDevice(ycl.sYCL.Mac);//从信任中心移除设备信息
		ZPS_vRemoveMacTableEntry(ycl.sYCL.Mac); //在MAC地址表中移除设备
	}
	DBG_vPrintf(TRACE_APP_UART, "Deleting device's YCL is\n");
	printf_array(&ycl, sizeof(ycl));
	return fCJP_Tx_Coor( CIE_Ycl,CJP_DEL_DEV_RESP , sdata , i);

}


PUBLIC CJP_Status fDev_Leave_Notice(uint64 mac)
{
	uint8 sdata[20],i=0 ,place=0;
	uYcl ycl;
	ycl.sYCL.Mac = mac;
	sEnddev_BasicInf tEnddev_BasicInf;
    //在设备列表中查找设备的基本信息
	place = LocateElem(&Galist ,ycl);
	if(place == 0)
	{
		return CJP_ERROR;
	}

	if(!GetElem(&Galist,place ,&tEnddev_BasicInf))
	{
		return CJP_ERROR;
	}

	if(ZPS_u16AplZdoLookupAddr(ycl.sYCL.Mac)==0x0000)
	{
		return CJP_ERROR;

	}
	else{
		dele_dev_data_manage(ycl); //删除设备列表信息
	}
	DBG_vPrintf(TRACE_APP_UART, "A device leave net ycl is\n");
	printf_array(&ycl, sizeof(ycl));
	ycl = tEnddev_BasicInf.ycl;
	sdata[i] = 1;
	i++;
	sdata[i] = E_ZCL_OSTRING;
	i++;
	sdata[i] = sizeof(uYcl);
	i++;
	memcpy(&sdata[i],&ycl , sdata[4]);
	i+= sizeof(uYcl);
	i++;
	Frame_Seq ++;
	return fCJP_Tx_Coor( CIE_Ycl,CJP_DEV_LEAVED_NOTICE , sdata , i);

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
	DBG_vPrintf(TRACE_APP_UART, "Adding device's YCL is\n");
	printf_array(&ycl, sizeof(ycl));
	return CJP_SUCCESS;
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
	uint8 sdata[30],len=0;
	sAttr_Charact tAttr_Charact[2]={
				{E_ZCL_OSTRING , (uint32)(&((sReadCoorInf_Resp*)(0))->C_YCL), YCL_LENGTH},
				{E_ZCL_OSTRING , (uint32)(&((sReadCoorInf_Resp*)(0))->C_Sofe_Ver) , SV_LENGTH},

	};
	memcpy(&readcoorinf_resp.C_YCL,&CIE_Ycl ,sizeof(uYcl));
	memcpy(&readcoorinf_resp.C_Sofe_Ver,&CIE_soft_ver ,sizeof(uSoft_Ver));

	len = fCJP_Attr_Handle(sdata , (uint8 *)&readcoorinf_resp ,tAttr_Charact ,2);
	return fCJP_Tx_Coor(CIE_Ycl, CJP_READ_COOR_DEV_INF_RESP , sdata , len);
}

/*
 * 向低功耗终端设备发送消息时，不能协调器直接向终端发送数据。协调器向终端发送数据的流程为:
 * 1、终端退出睡眠。
 * 2、终端主动向协调器poll消息
 * 3、协调器接收到终端的poll消息时，将数据传输到终端
 * 根据以上可知，协调器向终端发送消息时，需要将发送的消息进行缓冲，等到协调器接收到终端的poll消息时，再将缓冲区的数据取出，发送到终端。
 * 设计思路：在协调器申请一个数组变量当做缓冲区，此缓冲区存放JNI层发送过来的数据域数据和MAC、ClusterID等，
 */

PUBLIC CJP_Status fRead_End_inf(uYcl ycl)
{

	uint8 place=1,len=0;
	uYcl tycl=ycl;
	sEnddev_BasicInf tEnddev_BasicInf ;
	sReadDevInf_Resp ReadDevInf_Resp;
	uint8 sdata[80];
	sAttr_Charact tAttr_Charact[5]={
			{E_ZCL_OSTRING , (uint32)(&((sReadDevInf_Resp*)(0))->M_YCL), YCL_LENGTH},
			{E_ZCL_OSTRING , (uint32)(&((sReadDevInf_Resp*)(0))->M_Soft_Ver) , SV_LENGTH},
			{E_ZCL_OSTRING , (uint32)(&((sReadDevInf_Resp*)(0))->M_Hard_ver) , HV_LENGTH},
			{E_ZCL_OSTRING , (uint32)(&((sReadDevInf_Resp*)(0))->S_Soft_Ver) , SV_LENGTH},
			{E_ZCL_UINT16 , (uint32)(&((sReadDevInf_Resp*)(0))->HeartCyc) , 2}
	};

	DBG_vPrintf(TRACE_APP_UART, "reading information End device's YCL is\n");
	printf_array(&ycl, sizeof(ycl));
	if(ZPS_u16AplZdoLookupAddr(ycl.sYCL.Mac)==0x0000){
		return  CJP_ERROR;
	}
	//在设备列表中查找设备的基本信息
	place = LocateElem(&Galist,tycl);
	if(place == 0)
	{
		return CJP_ERROR;
	}

	if(!GetElem(&Galist,place ,&tEnddev_BasicInf))
	{
		return CJP_ERROR;
	}
	ReadDevInf_Resp.M_YCL = ycl;
	ReadDevInf_Resp.M_Soft_Ver.sSoft_Ver.Sv_YCL_ID = ycl.sYCL.YCL_ID.u32YCL_ID;
	ReadDevInf_Resp.M_Soft_Ver.sSoft_Ver.Sv_Mainv_Num =(uint8)(tEnddev_BasicInf.Msoftver>>8);
	ReadDevInf_Resp.M_Soft_Ver.sSoft_Ver.Sv_Secv_Num = (uint8)tEnddev_BasicInf.Msoftver;

	ReadDevInf_Resp.S_Soft_Ver.sSoft_Ver.Sv_YCL_ID = ycl.sYCL.YCL_ID.u32YCL_ID;
	ReadDevInf_Resp.S_Soft_Ver.sSoft_Ver.Sv_Mainv_Num = (uint8)(tEnddev_BasicInf.Ssoftver>>8);
	ReadDevInf_Resp.S_Soft_Ver.sSoft_Ver.Sv_Secv_Num = (uint8)tEnddev_BasicInf.Ssoftver;

	ReadDevInf_Resp.M_Hard_ver.sHard_Ver.Hv_Logo = HV_LOGO;
	ReadDevInf_Resp.M_Hard_ver.sHard_Ver.Hv_YCL_ID = ycl.sYCL.YCL_ID.u32YCL_ID;
	ReadDevInf_Resp.M_Hard_ver.sHard_Ver.Hv_TecPro = tEnddev_BasicInf.Hardver;
	ReadDevInf_Resp.M_Hard_ver.sHard_Ver.Hv_Dev_Company = tEnddev_BasicInf.Factcode;

	ReadDevInf_Resp.HeartCyc = tEnddev_BasicInf.hearttime;

	len = fCJP_Attr_Handle(sdata , (uint8 *)&ReadDevInf_Resp , tAttr_Charact ,5);

	return fCJP_Tx_Coor(CIE_Ycl , CJP_READ_END_DEV_INF_RESP , sdata , len);

}


/*协调器上报设备的列表包换设备的基本信息
 * 指令ID=0x16
 *
 */
#define MAX_DEV_INF_NUM    10
PUBLIC CJP_Status fReport_End_Dev_List(void)
{
	uint8 sdata[200],len=0,i=0,k=0,frame_num=0,frame_seq=0;
	sEnddev_BasicInf tEnddev_BasicInf;
	uint8 *p=NULL;
	if(Coor_Dev_manage.dev_num == 0)
	{
		return CJP_SUCCESS;
	}
	frame_num=Coor_Dev_manage.dev_num/10+1;//根据设备总数算出需要发送多少帧数据
	frame_seq=1;
	p=sdata;
	len=3;
	for(i=1;i<=Coor_Dev_manage.dev_num;i++)
	{
		if(!GetElem(&Galist , i , &tEnddev_BasicInf))
		{
			return CJP_ERROR;
		}
		memcpy( p+len,  &tEnddev_BasicInf.ycl , sizeof(uYcl));
		len+=sizeof(uYcl);
		memcpy(p+len,&tEnddev_BasicInf.hearttime ,sizeof(uint16));
		len+=sizeof(uint16);
		if(k>=(MAX_DEV_INF_NUM-1))
		{
			sdata[0]=k;
			sdata[1]=frame_seq;
			sdata[2]=frame_num;
			fCJP_Tx_Coor(CIE_Ycl , CJP_READ_END_DEV_LIST_RESP , sdata , len);
			len=3;
			frame_seq++;
			k=0;
		}
		else
		{
			k++;
		}

	}
	sdata[0]=k;
	sdata[1]=frame_seq;
	sdata[2]=frame_num;
	fCJP_Tx_Coor(CIE_Ycl , CJP_READ_END_DEV_LIST_RESP , sdata , len);
	return CJP_SUCCESS;
}


/*
 * 通知JNI层终端设备的心跳时间更改
 * 指令ID=0x17
 */
PUBLIC CJP_Status fUpdate_End_Dev_Hearttime_Notice(uYcl ycl)
{
	sEnd_Hearttime  tEnd_Hearttime;
	sEnddev_BasicInf tEnddev_BasicInf;
	uint8 sdata[20],len=0,place=0;
	sAttr_Charact tAttr_Charact[2]={
					{E_ZCL_OSTRING , (uint32)(&((sEnd_Hearttime*)(0))->E_YCL), YCL_LENGTH},
					{E_ZCL_UINT16 ,  (uint32)(&((sEnd_Hearttime*)(0))->E_Hearttime) , sizeof(uint16)},

	};
	place = LocateElem(&Galist,ycl);
	if(place == 0)
	{
		return CJP_ERROR;
	}

	if(!GetElem(&Galist,place ,&tEnddev_BasicInf))
	{
		return CJP_ERROR;
	}
	memcpy(&tEnd_Hearttime.E_YCL,&ycl ,sizeof(uYcl));
	memcpy(&tEnd_Hearttime.E_Hearttime,&tEnddev_BasicInf.hearttime ,sizeof(uint16));
	len = fCJP_Attr_Handle(sdata , (uint8 *)&tEnd_Hearttime ,tAttr_Charact ,2);
	Frame_Seq++;//主动发送的序列号+1
	return fCJP_Tx_Coor(CIE_Ycl , CJP_UPDATE_END_DEV_HEARTTIME_REQ , sdata , len);
}


PUBLIC CJP_Status fCoor_Self_Test(void)
{
	return CJP_SUCCESS;
	//协调器自检部分
}


PUBLIC CJP_Status fEnd_Read_BasicinfReq( uYcl ycl)
{
	uint16 cluster = BASIC_CLUSTER;
	uint8  attrs[9]={4,E_ZCL_UINT8 , 0xF0 ,E_ZCL_UINT8 ,0xF1,E_ZCL_UINT8 ,0xF2,E_ZCL_UINT8 , 0xF3};//基本信息的属性表

	fEnd_Read_AttrsReq( ycl , cluster , 8 , attrs);
	return CJP_SUCCESS;
}


/*
 * 读属性处理函数
 * mac ：目的地址
 * cluster：目的clusterID
 * len    ： 数据域长度
 * indata ：CJP数据域指针,不包括CJP帧头部分
 *
*/

PUBLIC CJP_Status fEnd_Read_AttrsReq( uYcl ycl , uint16 cluster , uint8 len , uint8 * indata )
{
	//判断mac是否存在,
	//判断clusterID是否存在
	//将输入的数据进行转换为要读的属性列表
	//将属性列表按照转换模型转换为Zigbee属性ID
	//组织数据进行发送
	uint8 i = 0,place=0;
	uint16 zattrID=0;
	uint8 attrsnum=*indata;//读取的属性个数
	sEnddev_BasicInf tEnddev_BasicInf;
	sAttr_Model_Array tAttr_Model_Array;
	ZPS_tsAfProfileDataReq psProfileDataReq1;
	static uint8 sqen=1;
	volatile uint16 u16PayloadSize=0;
	PDUM_thAPduInstance hAPduInst;
	DBG_vPrintf(TRACE_APP_UART, "read attrs req    YCL is\n");
	printf_array(&ycl, sizeof(ycl));
	DBG_vPrintf(TRACE_APP_UART, "Attr num  \n", attrsnum);
	if(ZPS_u16AplZdoLookupAddr(ycl.sYCL.Mac)==0x0000){
				return  CJP_ERROR;
	}
	if(attrsnum>10||attrsnum==0){
		return CJP_ERROR;
	}
	//进行属性ID的转换处理
	//在设备列表中查找设备的基本信息
	place = LocateElem(&Galist,ycl);
	if(place == 0)
	{
		return CJP_ERROR;
	}

	if(!GetElem(&Galist,place ,&tEnddev_BasicInf))
	{
		return CJP_ERROR;
	}
	if(!get_dev_model(tEnddev_BasicInf.clusterID, &tAttr_Model_Array))
	{
		return CJP_ERROR;
	}

	psProfileDataReq1.uDstAddr.u64Addr = ycl.sYCL.Mac;
	psProfileDataReq1.u16ClusterId = tEnddev_BasicInf.clusterID;//
	psProfileDataReq1.u16ProfileId = PROFILE_ID;
	psProfileDataReq1.u8SrcEp = PORT_NUM;
	psProfileDataReq1.eDstAddrMode= ZPS_E_ADDR_MODE_IEEE;
	psProfileDataReq1.u8DstEp = PORT_NUM;
	psProfileDataReq1.eSecurityMode= ZPS_E_APL_AF_UNSECURE;
	psProfileDataReq1.u8Radius= 0;
	hAPduInst = PDUM_hAPduAllocateAPduInstance(apduZCL);
	if(hAPduInst == NULL){
		DBG_vPrintf(TRACE_APP_UART, "PDUM_hAPduAllocateAPduInstance  error");
				/*申请内存不成功*/
		return CJP_ERROR;

	}
	else{

		sqen = u8GetTransactionSequenceNumber();
		u16PayloadSize = u16ZCL_WriteCommandHeader(hAPduInst,
			                   	   	   	   	   	 eFRAME_TYPE_COMMAND_ACTS_ACCROSS_ENTIRE_PROFILE,//统一的命令格式
			        		                     TRUE,
			        		                     ZCL_MANUFACTURER_CODE,
			        		                     TRUE,
			        		                     TRUE,
			        		                     &sqen,
			        		                     E_ZCL_READ_ATTRIBUTES);  //读属性
		// write payload, attribute at a time
		for(i=0; i<attrsnum; i++)
		{
			zattrID=get_zigbee_attrID( &tAttr_Model_Array , *(indata+2+i*2));
			DBG_vPrintf(TRACE_APP_UART, "CattrID is %02x" , *(indata+1+i*2));
			if(zattrID==0)
			{
				DBG_vPrintf(TRACE_APP_UART, "zattrID cannot find " );
				return CJP_ERROR;
			}
			u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "h", zattrID);//属性ID
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
 * indata ：CJP数据域指针,不包括CJP帧头部分
 */

PUBLIC CJP_Status fEnd_Write_AttrsReq( uYcl ycl , uint16 cluster ,  uint8 len , uint8 * indata )
{
	uint8 i = 0, j = 0 ,u8stringlen=0, type_addr_offset=0 ,place=0;
	uint8 attrsnum = *indata;
	uint16 zattrID=0;
	sEnddev_BasicInf tEnddev_BasicInf;
	sAttr_Model_Array tAttr_Model_Array;
	ZPS_tsAfProfileDataReq psProfileDataReq1;
	static uint8 sqen=1;
	volatile uint16 u16PayloadSize=0;
	PDUM_thAPduInstance hAPduInst;
	if(ZPS_u16AplZdoLookupAddr(ycl.sYCL.Mac)==0x0000){
				return  CJP_ERROR;
			}
	if((attrsnum>10)||(attrsnum==0)||(attrsnum%2!=0)){
		return CJP_ERROR;
	}
	//进行属性ID的转换处理
	//在设备列表中查找设备的基本信息
	place = LocateElem(&Galist,ycl);
	if(place == 0)
	{
		return CJP_ERROR;
	}

	if(!GetElem(&Galist,place ,&tEnddev_BasicInf))
	{
		return CJP_ERROR;
	}
	if(!get_dev_model(tEnddev_BasicInf.clusterID, &tAttr_Model_Array))
	{
		return CJP_ERROR;
	}
	//进行属性ID的转换处理
	//根据ycl寻找clusterID,根据clusterID寻找属性的ID对应关系。
	psProfileDataReq1.uDstAddr.u64Addr = ycl.sYCL.Mac;
	psProfileDataReq1.u16ClusterId = tEnddev_BasicInf.clusterID;//
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
				        		                    TRUE,
				        		                    ZCL_MANUFACTURER_CODE,
				        		                    TRUE,
				        		                    TRUE,
				        		                    &sqen,
				        		                    E_ZCL_WRITE_ATTRIBUTES_UNDIVIDED);  //写属性
	}
	for(i=0 ; i<(attrsnum/2) ; i++){
		//属性ID
		zattrID=get_zigbee_attrID( &tAttr_Model_Array , *(indata+2+type_addr_offset));
		if(zattrID==0)
		{
			return CJP_ERROR;
		}
		u16PayloadSize+=PDUM_u16APduInstanceWriteNBO(hAPduInst,u16PayloadSize, "h", zattrID);//zigbee属性ID
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
PUBLIC CJP_Status fEnd_Alarm_ReportResp( uYcl ycl,uint16 cluster ,uint8 len , uint8 * indata )
{
	//通知终端不要发送报警数据了。
	fEnd_Write_AttrsReq( ycl , cluster ,  len , indata );
	return CJP_SUCCESS;
}

/*
 * CJP数据域的通用数据合成函数
 * tAttr_Charact[]:描述数据变量的数据类型、偏移地址、长度等
 *  *tarray       :要填入的数组
 *  * sourcedata  :源数据
  */

PUBLIC uint8 fCJP_Attr_Handle(uint8 *tarray , uint8 * sourcedata ,sAttr_Charact *tAttr_Charact , uint8 attrnum)
{
	uint8  num=0, i=0,len=0;
	uint8 *dst;
	uint8 *srt;
	dst = tarray;
	srt = sourcedata;
	num = attrnum;
	DBG_vPrintf(TRACE_APP_UART," num = %x" , num);
	*dst = num;
	len++;
	for(i=0;i<num;i++)
	{
		*(dst+len) = tAttr_Charact[i].data_type;
		len++;
		if(tAttr_Charact[i].data_type == E_ZCL_OSTRING)//数组类型的数据，加上数据长度
		{
			*(dst+len) = tAttr_Charact[i].datalenth;
			len++;
		}
		memcpy(  dst+len , srt+tAttr_Charact[i].Offsetlength ,   *(srt+tAttr_Charact[i].Offsetlength));
		len +=  tAttr_Charact[i].datalenth;
	}
	return len;
}






