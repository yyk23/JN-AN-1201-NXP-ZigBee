/*
 * app_data_handle.h
 * 此文件主要针对数据的处理，如串口接收数据的转换，数据提取，数据的合成等等
 */
#ifndef APP_DATA_HANDLE_H_
#define APP_DATA_HANDLE_H_

#include "app_events.h"
#include "app_CIE_uart.h"
#pragma pack(1)    //按照1字节对齐,指明对齐方式

typedef struct{
	uint8  u8Channel;
	uint16 u16PanID;
}sBuildNet_Req;

typedef struct{
	CJP_Status u8Status;
	uint8 u8Channel;
	uint16 u16PanID;
}sBuildNet_Notice;

typedef struct{
	uint8 bJoinType;
	uYcl M_YCL;
	uSoft_Ver  M_Soft_Ver;
	uHard_Ver  M_Hard_ver;
	uSoft_Ver  S_Soft_Ver;
	uint16     HeartCyc;
}sDevJoin_Notice;


typedef struct{

	uYcl        C_YCL;
	uSoft_Ver   C_Sofe_Ver;
}sReadCoorInf_Resp;

typedef struct{
	uYcl        E_YCL;
	uint16      E_Hearttime;
}sEnd_Hearttime;


typedef struct{
	uYcl M_YCL;
	uSoft_Ver  M_Soft_Ver;
	uHard_Ver  M_Hard_ver;
	uSoft_Ver  S_Soft_Ver;
	uint16     HeartCyc;

}sReadDevInf_Resp;


typedef union{
	sBuildNet_Req     tBuildNet_Req ;
	sBuildNet_Notice  tBuildNet_Notice;
	sDevJoin_Notice   tDevJoin_Notice;
	sReadCoorInf_Resp tReadCoorInf_Resp;
	sReadDevInf_Resp  ReadDevInf_Resp;
}uCJP_Coor_Message;




PUBLIC usLinkKey Linkkey_Calculate(uYcl Ycl);

PUBLIC uint8 AttrID_Zigbee_to_CJP(uint16 clusterid , uint16 attrid);
PUBLIC uint16 AttrID_CJP_to_Zigbee(uYcl ycl , uint8 attrid);
PUBLIC uint16 ClusterID_Search(uYcl ycl);
PUBLIC uint8  test1(void);
#pragma pack()    //按照1字节对齐结束

#endif /* APP_DATA_HANDLE_H_ */
