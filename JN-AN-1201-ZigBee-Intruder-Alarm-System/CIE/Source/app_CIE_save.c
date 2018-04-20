/*****************************************************************************
 *
 * MODULE:             JN-AN-1201
 *
 * COMPONENT:          app_ias_save.c
 *
 * DESCRIPTION:        ZHA Demo : IAS PDM saving
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
#include "dbg.h"
#include "pdm.h"
#include "pdm_ids.h"
#include "app_zone_client.h"
#include "app_zcl_CIE_task.h"
#include "app_CIE_uart.h"
#include "Array_list.h"
#include "app_CIE_save.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifndef DEBUG_PDM_SAVE
#define TRACE_PDM_SAVE TRUE
#else
#define TRACE_PDM_SAVE TRUE
#endif
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
static PDM_teStatus eStatusReloadCIE1,eStatusReloadCIE2,eStatusReloadCIE3,eStatusReloadCIE4,eStatusReloadCIE5;

const sAttr_Model_Array  Alarm_Attr_Model_Array={{
		{0x0500,4},
		{0xFF01,0x01},
		{0xFF02,0x02},
		{0xFF03,0x03},
		{0xFF04,0x04},
}};

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/


/****************************************************************************
 *
 * NAME: vLoadIASCIEFromEEPROM
 *
 * DESCRIPTION:
 * Loads IAS CIE Tables/ACE tabled & Attributes from EEPROM
 * This function shall be called before afinit.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vLoadIASCIEFromEEPROM(uint8 u8SourceEndpoint)
{
	uint16 u16ByteRead;
	uint8 i=0;
	PDM_teStatus ecie_end_dev_table,ecie_dev_manage_inf,ecie_dev_model_table;

	memset(&Coor_Dev_manage, 0, sizeof(sCoor_Dev_manage));
	memset(Enddev_BasicInf,  0, sizeof(Enddev_BasicInf));
	memset(Attr_Model_Array, 0, sizeof(Attr_Model_Array));

	/* Loading Number of Discovered devices From EEPROM */
    eStatusReloadCIE1 = PDM_eReadDataFromRecord(        PDM_ID_APP_IASCIE_NODE,
                                                        &u8Discovered,
                                                        sizeof(uint8),
                                                        &u16ByteRead);
    DBG_vPrintf(TRACE_PDM_SAVE,"eStatusReload=%d\n",eStatusReloadCIE1);

    /* Loading Discovered Table From EEPROM */
    eStatusReloadCIE2 = PDM_eReadDataFromRecord(        PDM_ID_APP_IASCIE_STRUCT,
                                                        &sDiscovedZoneServers[0],
                                                        sizeof(tsDiscovedZoneServers) * 6,
                                                        &u16ByteRead);
    DBG_vPrintf(TRACE_PDM_SAVE,"eStatusReload=%d\n",eStatusReloadCIE2);

    //从EEPROM中提取数据到RAM中
    ecie_dev_manage_inf = PDM_eReadDataFromRecord(      PDM_ID_CIE_DEV_MANAGE_INF,
                                                        &Coor_Dev_manage,
                                                        sizeof(sCoor_Dev_manage),
                                                        &u16ByteRead);
    DBG_vPrintf(TRACE_PDM_SAVE,"ecie_dev_manage_inf=%d\n",ecie_dev_manage_inf);

    if (Coor_Dev_manage.valid_flag != VALID_VALUE)
    {
    	DBG_vPrintf(TRACE_PDM_SAVE,"ecie_end_dev_inf not VALID");
    	Coor_Dev_manage.valid_flag = VALID_VALUE;
    	Coor_Dev_manage.model_num = 0x01;
    	PDM_eSaveRecordData( PDM_ID_CIE_DEV_MANAGE_INF,
        					 &Coor_Dev_manage,
        					 sizeof(sCoor_Dev_manage));
    	//设置系统的默认模型——0x0500报警设备模型
    	Attr_Model_Array[0]=Alarm_Attr_Model_Array;
    	PDM_eSaveRecordData( PDM_ID_CIE_DEV_MODEL_TABLE_1,
    	        			 &Attr_Model_Array[0],
    	        			 sizeof(sAttr_Model_Array));//保存默认模型-报警设备模型

    }

    else
    {


    	ecie_end_dev_table = PDM_eReadDataFromRecord(       PDM_ID_CIE_END_DEV_TABLE,
    	                                                    &Enddev_BasicInf[0],
    	                                                    sizeof(sEnddev_BasicInf)*Coor_Dev_manage.dev_num,
    	                                                    &u16ByteRead);
    	 DBG_vPrintf(TRACE_PDM_SAVE,"ecie_end_dev_table=%d\n",ecie_end_dev_table);

    	 for(i=0;i<Coor_Dev_manage.model_num;i++)
    	 {
    		 ecie_dev_model_table = PDM_eReadDataFromRecord(       PDM_ID_CIE_DEV_MODEL_TABLE_1+i,
    		     	                                               &Attr_Model_Array[i],
    		     	                                               sizeof(sAttr_Model_Array),
    		     	                                               &u16ByteRead);
    		 DBG_vPrintf(TRACE_PDM_SAVE,"ecie_dev_model_table_%d =%d\n",i,ecie_dev_model_table);
    	 }
    }

}





/*
 * 添加设备
 * sEnddev_BasicInf
 */
PUBLIC bool add_dev_data_manage(sEnddev_BasicInf  BasicInf)
{
	uint8 dev_place;
	if(Coor_Dev_manage.model_num>=MAX_DEV_MANAGE_NUM)
	{
		return FALSE;
	}
	dev_place = LocateElem(&Galist, BasicInf.ycl);
	if(dev_place==0)//检查是否存在要添加的设备
	{
		AddElem( &Galist, BasicInf);
		Coor_Dev_manage.dev_num++;
		if(Coor_Dev_manage.model_num>=MAX_DEV_MANAGE_NUM)
		{
			Coor_Dev_manage.dev_full_flag=0x01;
		}

	}
	else
	{
		Coor_Dev_manage.dev_num = Galist.current_num;//同步一下
	}

	PDM_eSaveRecordData( 		PDM_ID_CIE_END_DEV_TABLE,
	                             &Enddev_BasicInf[0],
	                            sizeof(sEnddev_BasicInf) * MAX_DEV_MANAGE_NUM);


	PDM_eSaveRecordData( 		PDM_ID_CIE_DEV_MANAGE_INF,
		                        &Coor_Dev_manage,
		                        sizeof(sCoor_Dev_manage));
	return TRUE;

}

/*
 * 删除设备数据处理函数
 * 按照YCL删除设备列表的基本信息
 */

PUBLIC bool dele_dev_data_manage(uYcl ycl)
{
	uint8 dev_place;
	dev_place = LocateElem(&Galist, Enddev_BasicInf->ycl);
	if(dev_place==0)
	{
		return FALSE;
	}
	DelElem(&Galist,dev_place);
	Coor_Dev_manage.dev_num--;
	Coor_Dev_manage.dev_full_flag = 0;
	Coor_Dev_manage.dev_num = Galist.current_num;
	PDM_eSaveRecordData( 		PDM_ID_CIE_END_DEV_TABLE,
		                        &Enddev_BasicInf[0],
		                        sizeof(sEnddev_BasicInf) * MAX_DEV_MANAGE_NUM);


	PDM_eSaveRecordData( 		PDM_ID_CIE_DEV_MANAGE_INF,
			                    &Coor_Dev_manage,
			                    sizeof(sCoor_Dev_manage));
	return TRUE;

}

PUBLIC bool add_dev_model_data_manage(sAttr_Model_Array Model_Array)
{
	uint8 mp=0;

	if(Coor_Dev_manage.model_num>=MAX_DEV_MODEL_NUM)
	{
		return FALSE;
	}
	//检查是否存在相同的模型，
	mp = find_dev_model(Model_Array.Attr_Model->head.clusterID);
	if(mp == 255)
	{
	//添加到模型列表
		Attr_Model_Array[Coor_Dev_manage.model_num] = Model_Array;
		Coor_Dev_manage.model_num++;
	}
	else
	{
	//更新模型列表
		Attr_Model_Array[mp-1] = Model_Array;

	}

	PDM_eSaveRecordData( 		 PDM_ID_CIE_DEV_MODEL_TABLE_1+Coor_Dev_manage.model_num	,
								 &Model_Array,
		                         sizeof(sAttr_Model_Array));//


	PDM_eSaveRecordData( 		  PDM_ID_CIE_DEV_MANAGE_INF,
				                  &Coor_Dev_manage,
				                  sizeof(sCoor_Dev_manage));//
	return TRUE;

}

/*
 * 获取设备模型的位置
 * 根据clusterID来寻找模型，存在返回位置，不存在则返回255
 *
 */

PUBLIC  uint8  find_dev_model(uint16  tclusterId)
{
	uint8 i=0;
	for(i=0;i<Coor_Dev_manage.model_num;i++)
	{
		if(Attr_Model_Array[i].Attr_Model->head.clusterID == tclusterId)
		{
			return i+1;
		}

	}
	return 255;
}
/*
 * 获取设备模型
 *
 */

PUBLIC bool get_dev_model(uint16 tclusterId,sAttr_Model_Array *Model_Array)
{
	uint8 model_position;
	model_position = find_dev_model(tclusterId);
	if(model_position <= MAX_DEV_MODEL_NUM)
	{
		*Model_Array = Attr_Model_Array[model_position-1];
		return TRUE;
	}

	return FALSE;
}


/*
 * 根据模型数据和CJP协议的属性ID找到对应的ZigBee协议的属性nID
 *
 */
PUBLIC uint8 get_CJP_attrID(sAttr_Model_Array *Model_Array , uint16 z_attrID)
{
	uint8 i=0;
	for(i=0; i<Model_Array->Attr_Model[0].head.attrnum; i++)
	{
		if(Model_Array->Attr_Model[i+1].attr.zattrID==z_attrID)
		{
			return Model_Array->Attr_Model[i+1].attr.CattrID;
		}
	}
	return 0;
}


/*
 * 根据模型数据和zigbee协议的属性ID找到对应的CJP协议的属性nID
 *
 */
PUBLIC uint16 get_zigbee_attrID(sAttr_Model_Array *Model_Array , uint8 c_attrID)
{
	uint8 i=0;
	for(i=0; i<Model_Array->Attr_Model[0].head.attrnum; i++)
	{
		if(Model_Array->Attr_Model[i+1].attr.CattrID==c_attrID)
		{
			return Model_Array->Attr_Model[i+1].attr.zattrID;
		}
	}
	return 0;
}

/****************************************************************************
 *
 * NAME: vVerifyIASCIELoad
 *
 * DESCRIPTION:
 * Verifies the Load of IAS CIE tables & ACE Tables/Attributes from EEPROM
 * This function shall always be called afinit , in case the record is not recovered save it.
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vVerifyIASCIELoad(uint8 u8SourceEndpoint)
{
    if (eStatusReloadCIE1 != PDM_E_STATUS_OK)
    {
    	PDM_eSaveRecordData( PDM_ID_APP_IASCIE_NODE,
    						 &u8Discovered,
    						 sizeof(uint8));
    }

   if (eStatusReloadCIE2 != PDM_E_STATUS_OK)
   {
       PDM_eSaveRecordData( PDM_ID_APP_IASCIE_STRUCT,
                            &sDiscovedZoneServers[0],
                            sizeof(tsDiscovedZoneServers) * MAX_ZONE_SERVER_NODES);
   }

   if (eStatusReloadCIE3 != PDM_E_STATUS_OK)
   {

   }

   if (eStatusReloadCIE4 != PDM_E_STATUS_OK)
   {

   }

   if (eStatusReloadCIE5 != PDM_E_STATUS_OK)
   {

   }

}



/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
