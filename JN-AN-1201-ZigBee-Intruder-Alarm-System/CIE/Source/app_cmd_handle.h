/*
 * app_cmd_handle.h
 * 此文件主要针对CIP的相关命令处理函数
 */
#ifndef APP_CMD_HANDLE_H_
#define APP_CMD_HANDLE_H_

#include "app_events.h"
#include "app_data_handle.h"




PUBLIC CJP_Status fBuildNet(uint8 channel,uint16 panid);
PUBLIC   void     fBuildNet_Notice(CJP_Status);
PUBLIC CJP_Status fJoinNet_Set(uint8 time);
PUBLIC CJP_Status fDel_Dev(uint64 mac);
PUBLIC CJP_Status fDev_Join(uYcl ycl);
PUBLIC CJP_Status fReset_Def_Set(void);
PUBLIC CJP_Status fRead_Coor_inf(void);
PUBLIC CJP_Status fRead_End_inf(uint64 mac);
PUBLIC CJP_Status fCoor_Self_Test(void);

PUBLIC CJP_Status fEnd_Read_AttrsReq(uint64 mac,uint16 cluster ,  uint8 len , uint8 * indata);
PUBLIC CJP_Status fEnd_Write_AttrsReq(uint64 mac,uint16 cluster , uint8 len , uint8 * indata);
PUBLIC CJP_Status fEnd_Alarm_ReportResp(uint64 mac,uint16 cluster , uint8 len , uint8 * indata);
PUBLIC CJP_Status fEnd_Read_BasicinfReq( uint64 mac);









#endif
