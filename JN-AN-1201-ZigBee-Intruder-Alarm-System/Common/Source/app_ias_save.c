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
#include "os.h"
#include "os_gen.h"
#include "dbg.h"
#include "pdm.h"
#include "pdm_ids.h"
#include "zcl.h"
#include "IASZONE.h"
#include "app_ias_indicator.h"
#include "app_timer_driver.h"
#include "Utilities.h"
#if (defined CIE)
#include "app_zone_client.h"
#include "app_zcl_CIE_task.h"
#endif
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifndef DEBUG_PDM_SAVE
#define TRACE_PDM_SAVE FALSE
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
tsCLD_IASZone               sIASZoneDesc;
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: eLoadIASZoneServerAttributesFromEEPROM
 *
 * DESCRIPTION:
 * Loads IAS Zone server attributes from EEPROM
 *
 *
 * RETURNS:
 * PDM_teStatus
 *
 ****************************************************************************/
PUBLIC PDM_teStatus eLoadIASZoneServerAttributesFromEEPROM(void)
{
	uint16 u16ByteRead;
	PDM_teStatus eStatusReload = PDM_eReadDataFromRecord(PDM_ID_APP_IASZONE,
                                                        &sIASZoneDesc,
                                                        sizeof(tsCLD_IASZone),
                                                        &u16ByteRead);

    DBG_vPrintf(TRACE_PDM_SAVE,"eStatusReload=%d\n",eStatusReload);

    return eStatusReload;
}

/****************************************************************************
 *
 * NAME: vLoadIASZoneAttributes
 *
 * DESCRIPTION:
 * Loads IAS Zone indication state by checking the loaded server attributes
 *
 * PARAMETERS:
 * End point Number
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vLoadIASZoneAttributes(uint8 u8SourceEndPointId)
{
    teZCL_Status eStatus;
    tsZCL_EndPointDefinition *psEndPointDefinition;
    tsZCL_ClusterInstance *psClusterInstance;
    tsCLD_IASZone_CustomDataStructure *pCustomDataStructure;
    tsCLD_IASZone *psSharedStruct;

    eStatus = eZCL_FindCluster( SECURITY_AND_SAFETY_CLUSTER_ID_IASZONE,
                                u8SourceEndPointId,
                                TRUE,
                                &psEndPointDefinition,
                                &psClusterInstance,
                                (void*)&pCustomDataStructure);
    if(eStatus == E_ZCL_SUCCESS)
    {
		/* Point to shared struct */
		psSharedStruct = (tsCLD_IASZone *)psClusterInstance->pvEndPointSharedStructPtr;
		memcpy (psSharedStruct,&sIASZoneDesc,sizeof(tsCLD_IASZone));

		if(psSharedStruct->e8ZoneState == 0x01)
		{
			vSetIASDeviceState(E_IAS_DEV_STATE_ENROLLED);
		}
		else if ( psSharedStruct->u64IASCIEAddress !=0)
		{
			vSetIASDeviceState(E_IAS_DEV_STATE_READY_TO_ENROLL);//准备认证
		}
		else
		{
			vSetIASDeviceState(E_IAS_DEV_STATE_JOINED);//还没有认证，只是加入到网络
		}
    }
}
/****************************************************************************
 *
 * NAME: vSaveIASZoneAttributes
 *
 * DESCRIPTION:
 * Saves IAS Zone indication state by checking the loaded server attributes
 *
 * PARAMETERS:
 * End point Number
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vSaveIASZoneAttributes(uint8 u8SourceEndPointId)
{
    teZCL_Status eStatus;
    tsZCL_EndPointDefinition *psEndPointDefinition;
    tsZCL_ClusterInstance *psClusterInstance;
    tsCLD_IASZone_CustomDataStructure *pCustomDataStructure;
    tsCLD_IASZone *psSharedStruct;

    eStatus = eZCL_FindCluster( SECURITY_AND_SAFETY_CLUSTER_ID_IASZONE,
                                u8SourceEndPointId,
                                TRUE,
                                &psEndPointDefinition,
                                &psClusterInstance,
                                (void*)&pCustomDataStructure);
    if(eStatus == E_ZCL_SUCCESS)
    {
		/* Point to shared struct */
		psSharedStruct = (tsCLD_IASZone *)psClusterInstance->pvEndPointSharedStructPtr;//attribute data 列表
		memcpy (&sIASZoneDesc,psSharedStruct,sizeof(tsCLD_IASZone));//使sIASZoneDesc中的值和真正定义的sDevice中的值相等

		PDM_eSaveRecordData( PDM_ID_APP_IASZONE,
							&sIASZoneDesc,
							sizeof(tsCLD_IASZone));//保存
    }
}
/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
