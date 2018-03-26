/*****************************************************************************
 *
 * MODULE:             JN-AN-1201
 *
 * COMPONENT:          app_ias_enroll_req.c
 *
 * DESCRIPTION:        ZHA Demo : IAS Enroll Request from IAS Zone Server
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
#include "zcl.h"
#include "IASZONE.h"
#include "app_ias_indicator.h"
#include "app_timer_driver.h"
#include "app_ias_save.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifndef DEBUG_UNENROLL_REQ
#define TRACE_UNENROLL_REQ FALSE
#else
#define TRACE_UNENROLL_REQ TRUE
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

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: vProcessWriteAttributeRangeCheck
 *
 * DESCRIPTION:
 * Process Write attribute request, if the request is from other than the CIE,
 * rejects the request.
 *
 * PARAMETERS:
 * tsZCL_CallBackEvent *psEvent , stack event from end point call back
 *
 * RETURNS:
 * PDM_teStatus
 *
 ****************************************************************************/
PUBLIC void vProcessWriteAttributeRangeCheck(tsZCL_CallBackEvent *psEvent)
{
    tsZCL_IndividualAttributesResponse   *psIndividualAttributeResponse = &psEvent->uMessage.sIndividualAttributeResponse;
    uint64 u64IEEEAddress=0;
    uint64 u64SenderIEEEAddress=0;
    DBG_vPrintf(TRACE_UNENROLL_REQ,"The Checking Range \n");

    eZCL_ReadLocalAttributeValue(
            psEvent->pZPSevent->uEvent.sApsDataIndEvent.u8DstEndpoint,                           /*uint8                      u8SrcEndpoint,*/
                             SECURITY_AND_SAFETY_CLUSTER_ID_IASZONE,                             /*uint16                     u16ClusterId,*/
                             TRUE,                                                               /*bool                       bIsServerClusterInstance,*/
                             FALSE,                                                              /*bool                       bManufacturerSpecific,*/
                             FALSE,                                                              /*bool_t                     bIsClientAttribute,*/
                             E_CLD_IASZONE_ATTR_ID_IAS_CIE_ADDRESS,                              /*uint16                     u16AttributeId,*/
                             &u64IEEEAddress);                                                   /*void                      *pvAttributeValue);*/

    u64SenderIEEEAddress= ZPS_u64AplZdoLookupIeeeAddr(psEvent->pZPSevent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr);

    DBG_vPrintf(TRACE_UNENROLL_REQ,"\n\n CIE IEEE Address = 0x%016llx\t",u64IEEEAddress);
    DBG_vPrintf(TRACE_UNENROLL_REQ,"Senders IEEE Address = 0x%016llx\n\n",u64SenderIEEEAddress);

    if(( u64IEEEAddress != u64SenderIEEEAddress ) &&
            (u64IEEEAddress !=0))
    {
        DBG_vPrintf(TRACE_UNENROLL_REQ, "Deny Access\n\n");
        psIndividualAttributeResponse->eAttributeStatus = E_ZCL_CMDS_ACTION_DENIED;
    }
    else
    {
        if(
                ( SECURITY_AND_SAFETY_CLUSTER_ID_IASZONE == psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum ) &&
                ( E_CLD_IASZONE_ATTR_ID_IAS_CIE_ADDRESS == psIndividualAttributeResponse->u16AttributeEnum )&&
                ( E_ZCL_CMDS_SUCCESS == psIndividualAttributeResponse->eAttributeStatus)
            )
        {
            DBG_vPrintf(TRACE_UNENROLL_REQ,"Range Check for IEEE address \n");
            uint8 i =0;
            uint8 u8Result=0;
            /*Just ORing all to see if it 0 - the pointer is not able to typecast - TBD to see*/
            for(i=0;i<8;i++)
            {
                DBG_vPrintf(TRACE_UNENROLL_REQ,"psIndividualAttributeResponse->pvAttributeData[%d]=0x%02x\n",i,((uint8*)psIndividualAttributeResponse->pvAttributeData)[i]);
                u8Result |=((uint8*)psIndividualAttributeResponse->pvAttributeData)[i];
            }


            if(   u8Result == 0      )
            {
                DBG_vPrintf(TRACE_UNENROLL_REQ,"IEEE address is 0\n");
                eCLD_IASZoneUpdateZoneState (
                        psEvent->pZPSevent->uEvent.sApsDataIndEvent.u8DstEndpoint,
                        E_CLD_IASZONE_STATE_NOT_ENROLLED
                                               );
                eCLD_IASZoneUpdateZoneID (
                        psEvent->pZPSevent->uEvent.sApsDataIndEvent.u8DstEndpoint,
                        0xff);
                eCLD_IASZoneUpdateCIEAddress(
                        psEvent->pZPSevent->uEvent.sApsDataIndEvent.u8DstEndpoint,
                        0);

                DBG_vPrintf(TRACE_UNENROLL_REQ,"The Unbind Address 0x%016llx \n",u64IEEEAddress);
                ZPS_teStatus eStatus=ZPS_eAplZdoUnbind(
                                        psEvent->pZPSevent->uEvent.sApsDataIndEvent.u16ClusterId,              /*uint16 u16ClusterId, */
                                        psEvent->pZPSevent->uEvent.sApsDataIndEvent.u8DstEndpoint,             /*uint8 u8SrcEndpoint,*/
                                        psEvent->pZPSevent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr,       /*uint16 u16DstAddr,*/
                                        u64IEEEAddress,                                                         /*uint64 u64DstIeeeAddr,*/
                                        psEvent->pZPSevent->uEvent.sApsDataIndEvent.u8SrcEndpoint);             /*uint8 u8DstEndpoint); */

                DBG_vPrintf(TRACE_UNENROLL_REQ,"The Unbind Status %d\n",eStatus);
                vSetIASDeviceState(E_IAS_DEV_STATE_JOINED);
                vSaveIASZoneAttributes(psEvent->pZPSevent->uEvent.sApsDataIndEvent.u8DstEndpoint);
                OS_eStartSWTimer(APP_IndicatorTimer,APP_TIME_MS(250) , NULL);
            }
        }
    }
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
