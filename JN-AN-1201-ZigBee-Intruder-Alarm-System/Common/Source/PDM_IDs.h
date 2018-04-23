/*****************************************************************************
 *
 * MODULE:             JN-AN-1201
 *
 * COMPONENT:          PDM_IDs.h
 *
 * DESCRIPTION:        Persistant Data Manager Id's
 *
 *****************************************************************************
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
 * Copyright NXP B.V. 2013. All rights reserved
 *
 ****************************************************************************/

#ifndef  PDMIDS_H_INCLUDED
#define  PDMIDS_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif


/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/

#include <jendefs.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#ifdef PDM_USER_SUPPLIED_ID

//#define PDM_ID_APP_ACE                  0x1
//#define PDM_ID_APP_LIGHT_TABLE          0x2
#define PDM_ID_APP_COORD                  0x3
#define PDM_ID_APP_APP_ROUTER             0x5
//#define PDM_ID_APP_SCENES_CL            0x7
//#define PDM_ID_APP_SCENES_ATTB          0x8
//#define PDM_ID_APP_SCENES_DATA          0x9
#define PDM_ID_POWER_ON_COUNTER           0xa
#define PDM_ID_APP_REPORTS                0xb
#define PDM_ID_OTA_DATA                   0xc
#define PDM_ID_APP_IASZONE                0xd
//#define PDM_ID_APP_IASZONE_NODE         0xe
//#define PDM_ID_APP_IASACE_ZONE_TABLE    0xf
//#define PDM_ID_APP_IASACE_ZONE_PARAM    0x10
//#define PDM_ID_APP_IASACE_PANEL_PARAM   0x11
#define PDM_ID_APP_IASCIE_STRUCT        0x12
#define PDM_ID_APP_IASCIE_NODE          0x13

//注意0x8000以上是ZDO占用，0x3000~0x3007被JenNet-IP libraries 占用
#define PDM_ID_APP_EP_DEV_INF           0x20


/*
 * 协调器设备管理区
*/

//协调器设备的基本管理信息，包括设备数量，模型数量，模型内存管理信息
#define PDM_ID_CIE_DEV_MANAGE_INF          0x30  //所占空间sizeof(sCoor_Dev_manage)

//设备基本列表
#define PDM_ID_CIE_END_DEV_TABLE       0x40  //设备列表   所占最大空间30*sizeof(sEnddev_BasicInf)


//设备模型列表
#define PDM_ID_CIE_DEV_MODEL_TABLE_1   0x50  //模型列表的首地址  所占空间sizeof(uAttr_Model)*属性个数 //最大占用空间为sizeof(uAttr_Model)*(MAX_DEV_MODEL_ATTR_NUM+1)
#define PDM_ID_CIE_DEV_MODEL_TABLE_2   0x51  //模型列表的第2
#define PDM_ID_CIE_DEV_MODEL_TABLE_3   0x52  //模型列表的第3
#define PDM_ID_CIE_DEV_MODEL_TABLE_4   0x53  //模型列表的第4
#define PDM_ID_CIE_DEV_MODEL_TABLE_5   0x54  //模型列表的第5
#define PDM_ID_CIE_DEV_MODEL_TABLE_6   0x55  //模型列表的第6
#define PDM_ID_CIE_DEV_MODEL_TABLE_7   0x56  //模型列表的第7
#define PDM_ID_CIE_DEV_MODEL_TABLE_8   0x57  //模型列表的第8

#define PDM_ID_CIE_CHANNEL             0x0090  //信道存储


#else

#define PDM_ID_APP_REMOTE_CONTROL   "ACE_CONTROLLER"
#define PDM_ID_APP_LIGHT_TABLE      "LIGHT_TABLE"
#define PDM_ID_APP_COORD            "APP_COORD"
#define PDM_ID_APP_APP_ROUTER       "APP_ROUTER"
#define PDM_ID_APP_SCENES_CL        "SCENES_CL"
#define PDM_ID_APP_SCENES_ATTB      "SCENES_ATTB"
#define PDM_ID_APP_SCENES_DATA      "SCENES_DATA"

#endif


/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif /* PDMIDS_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
