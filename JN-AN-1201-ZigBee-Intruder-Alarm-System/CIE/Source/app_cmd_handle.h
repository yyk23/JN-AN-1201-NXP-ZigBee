/*
 * app_cmd_handle.h
 * 此文件主要针对CIP的相关命令处理函数
 */
#ifndef APP_CMD_HANDLE_H_
#define APP_CMD_HANDLE_H_

#include "app_events.h"
#include "app_data_handle.h"
#include "app_CIE_uart.h"

PUBLIC CJP_Status fCJP_Tx_Coor(uYcl ycl ,CJP_CmdID cmd_coor , uint8 *pdata , uint8 len);
PUBLIC CJP_Status fBuildNet(uint8 channel,uint16 panid);
PUBLIC   void     fBuildNet_Notice(CJP_Status);
PUBLIC CJP_Status fJoinNet_Set(uint8 time);
PUBLIC CJP_Status fDel_Dev(uYcl ycl);
PUBLIC CJP_Status fDev_Leave_Notice(uint64 mac);
PUBLIC CJP_Status fDev_Join(uYcl ycl);
PUBLIC CJP_Status fReset_Def_Set(void);
PUBLIC CJP_Status fRead_Coor_inf(void);
PUBLIC CJP_Status fRead_End_inf(uYcl ycl);
PUBLIC CJP_Status fCoor_Self_Test(void);
PUBLIC CJP_Status fReport_End_Dev_List(void);

PUBLIC CJP_Status fEnd_Read_AttrsReq(uYcl ycl,uint16 cluster ,  uint8 len , uint8 * indata);
PUBLIC CJP_Status fEnd_Write_AttrsReq(uYcl ycl,uint16 cluster , uint8 len , uint8 * indata);
PUBLIC CJP_Status fEnd_Alarm_ReportResp(uYcl ycl,uint16 cluster , uint8 len , uint8 * indata);
PUBLIC CJP_Status fEnd_Read_BasicinfReq( uYcl ycl);
PUBLIC uint8 fCJP_Attr_Handle(uint8 *tarray , uint8 * sourcedata ,sAttr_Charact *tAttr_Charact , uint8 attrnum);



#endif
