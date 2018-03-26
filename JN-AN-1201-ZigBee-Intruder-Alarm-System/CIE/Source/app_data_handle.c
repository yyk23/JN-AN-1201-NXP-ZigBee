/******************************************************************
 * app_data_handle.c
 *
 * 此文件主要是对数据的处理，如数据的提取，数据的整合等等
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






/*
 * 将需要发送给JNI层的消息进行格式转化。
 * 输入命令ID，要发送的数据域结构体，数据类型常量，要存放的地址。
 */
PUBLIC CJP_Status  CoortoJNI_DataSw ( CJP_CmdID tCJP_CmdID)
{
		//

}






/*
 * 处理
 */

PUBLIC usLinkKey Linkkey_Calculate( uYcl Ycl)
{
	usLinkKey   linkkey_tmp;
	uYcl         ycl_tmp=Ycl;

	ycl_tmp.YCL_Array[11]=Ycl.YCL_Array[10];
	ycl_tmp.YCL_Array[10]=Ycl.YCL_Array[11];

	ycl_tmp.YCL_Array[5]=Ycl.YCL_Array[4];
	ycl_tmp.YCL_Array[4]=Ycl.YCL_Array[5];

	ycl_tmp.YCL_Array[3]=Ycl.YCL_Array[1];
	ycl_tmp.YCL_Array[2]=Ycl.YCL_Array[0];
	ycl_tmp.YCL_Array[1]=Ycl.YCL_Array[3];
	ycl_tmp.YCL_Array[0]=Ycl.YCL_Array[2];

	linkkey_tmp.LinkKey_YCL=ycl_tmp;
	linkkey_tmp.LinkKey_Last4By=((uint32)*(&(ycl_tmp.YCL_Array[11]))+(uint32)*(&(ycl_tmp.YCL_Array[7]))+(uint32)*(&(ycl_tmp.YCL_Array[3])))/0xFFFF;

	return linkkey_tmp;
}




