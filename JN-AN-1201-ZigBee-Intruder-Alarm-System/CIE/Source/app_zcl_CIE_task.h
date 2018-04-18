/*****************************************************************************
 *
 * MODULE:             JN-AN-1201 ZHA Demo
 *
 * COMPONENT:          app_zcl_CIE_task.h
 *
 * DESCRIPTION:        ZHA CIE Application Behavior - Interface
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
 * Copyright NXP B.V. 2012. All rights reserved
 *
 ***************************************************************************/

/****************************************************************************/
/* Description.                                                             */
/* If you do not need this file to be parsed by doxygen then delete @file   */
/****************************************************************************/

/** @file
 * Add brief description here.
 * Add more detailed description here
 */

/****************************************************************************/
/* Description End                                                          */
/****************************************************************************/

#ifndef APP_ZCL_TASK_H_
#define APP_ZCL_TASK_H_

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/

#include <jendefs.h>
#include "control_and_indicating_equipment.h"
#include "app_CIE_uart.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define ALIGN(n, v)     ( ((uint32)(v) + ((n) - 1)) & (~((n) - 1)) )

#define CLD_IASACE_PANEL_PARAMTER_SECONDS_REMAINING     10
#define DISABLE_WARNING                                 0
#define ENTRY_EXIT_DELAY_WARNING_DURATION               10
#define ALARM_WARNING_DURATION                          30
#define STROBE_LEVEL                                    1
#define STROBE_DUTY_CYCLE                               20
#define WARNING_MODE_STROBE_AND_SIREN_LEVEL_BURGLAR     0x17
#define WARNING_MODE_STROBE_AND_SIREN_LEVEL_FIRE        0x27
#define WARNING_MODE_STROBE_AND_SIREN_LEVEL_EMERGENCY   0x37
#define WARNING_MODE_STROBE_AND_SIREN_LEVEL_ENTRY_EXIT_DELAY         0x47
#define SQUAWK_MODE_STROBE_AND_LEVEL_ARMED				0x0B
#define SQUAWK_MODE_STROBE_AND_LEVEL_DISARMED			0x1B




typedef enum{
	E_ZCL_FRAME_HEART_DATA_REPORT = 0x01  ,
	E_ZCL_FRAME_NORMAL_DATA_REPORT   ,
	E_ZCL_FRAME_ALARM_DATA_REPORT    ,
	E_ZCL_FRAME_DEV_INF_DATA_REPORT  ,
	E_ZCL_FRAME_DEV_SW_MODEL_DATA_REPORT  ,
	E_ZCL_FRAME_READ_INDIVIDUAL_ATTRIBUTE_RESPONSE,
	E_ZCL_FRAME_WRITE_ATTRIBUTES_RESPONSE,


	E_ZCL_FRAME_OTA_DATA  =0x50

}eZCL_Frametype;


typedef struct{
	uint16 u16AttrID;
	uint8  u8Datatype;

}sReportAttr_Desc;

typedef struct{
	uint8  u16CAttrID;
	uint8  u8Datatype;

}sCJPReportAttr_Desc;

typedef enum{
	    E_CLD_BAS_ATTR_ID_M_YCL                = 0xF001, /* Mandatory */
	    E_CLD_BAS_ATTR_ID_M_CLUSTERID,
	    E_CLD_BAS_ATTR_ID_HEARTTIME,
	    E_CLD_BAS_ATTR_ID_M_SOFT_VER,
	    E_CLD_BAS_ATTR_ID_M_HARD_VER,
	    E_CLD_BAS_ATTR_ID_S_SOFT_VER          =0xF022,
	    E_CLD_BAS_ATTR_ID_ATTR_SW_TABLE       =0xF023,
}eBasic_AttrID;


typedef enum{
	E_CLD_COMMON_ATTR_ID_POWER_VALUE =0xFF01,
	E_CLD_COMMON_ATTR_ID_SIG_VALUE ,
	E_CLD_COMMON_ATTR_ID_DEV_STATUS ,
	E_CLD_COMMON_ATTR_ID_HEARTTIME
}eCommon_AttrID;
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
PUBLIC void APP_ZCL_vInitialise(void);
PUBLIC void vPermitJoinIndication(void);
PUBLIC bool bCheckNotReadyToArm(uint8 u8ConfigFlag);
PUBLIC void vStartArmingSystem(void);


PUBLIC CJP_Status fEndDev_BasicInf_Handle(uint64 mac ,uint16 clusterID ,uint8 commandID ,uint8 * sdata ,uint8 u8attrnum , uint16 u16len);
PUBLIC CJP_Status fEndDev_SwModle_Handle(uint64 mac ,uint16 clusterID ,uint8 commandID ,uint8 * sdata ,uint8 u8attrnum , uint16 u16len);
PUBLIC CJP_Status fEndDev_WriteAttr_Resp_Handle(uint64 mac ,uint16 clusterID ,uint8 commandID ,uint8 * sdata ,uint8 u8attrnum , uint16 u16len);
PUBLIC CJP_Status fEndDev_ReportAttr_Handle(uint64 mac ,uint16 clusterID ,uint8 commandID ,uint8 * sdata ,uint8 u8attrnum , uint16 u16len);

PUBLIC uint16 App_u16BufferReadNBO ( uint8         *pu8Struct,
                                     const char    *szFormat,
                                     void          *pvData);
PUBLIC uint16 APP_u16GetAttributeActualSize ( uint32    u32Type , uint16    u16NumberOfItems );
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern uint8 u8LastPanelStatus;
extern uint8 u8PanelStatusB4NotReadyToArm;
extern uint8 u8ConfigFlag;
extern tsHA_IASCIE sDevice;
#endif /* APP_ZCL_TASK_H_ */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
