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
#include "dbg.h"
#include "zcl.h"
#include "IASZONE.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifndef DEBUG_ENROLL_REQ
#define TRACE_ENROLL_REQ TRUE
#else
#define TRACE_ENROLL_REQ TRUE
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
PUBLIC void vSendEnrollReq(uint8 u8EndPoint)
{
    tsZCL_Address sDestinationAddress;
    uint8  u8TransactionSequenceNumber;
    tsCLD_IASZone_EnrollRequestPayload   sPayload;


    sDestinationAddress.eAddressMode = E_ZCL_AM_SHORT;
    sDestinationAddress.uAddress.u16DestinationAddress=0x0000;

    eZCL_ReadLocalAttributeValue(
                             u8EndPoint,                           /*uint8                      u8SrcEndpoint,*/
                             SECURITY_AND_SAFETY_CLUSTER_ID_IASZONE,/*uint16                     u16ClusterId,*/
                             TRUE,                                 /*bool                       bIsServerClusterInstance,*/
                             FALSE,                                /*bool                       bManufacturerSpecific,*/
                             FALSE,                                /*bool_t                     bIsClientAttribute,*/
                             E_CLD_IASZONE_ATTR_ID_ZONE_TYPE,      /*uint16                     u16AttributeId,*/
                             &(sPayload.e16ZoneType));             /*void                      *pvAttributeValue);*/

    sPayload.u16ManufacturerCode = ZCL_MANUFACTURER_CODE;
    teZCL_Status eStatus = eCLD_IASZoneEnrollReqSend (
                                    u8EndPoint,                    /*uint8                              u8SourceEndPointId,*/
                                    0xff,                          /*uint8                              u8DestinationEndPointId,*/
                                    &sDestinationAddress,          /*tsZCL_Address                      *psDestinationAddress,*/
                                    &u8TransactionSequenceNumber,  /*uint8                              *pu8TransactionSequenceNumber,*/
                                    &sPayload);                    /*tsCLD_IASZone_EnrollRequestPayload *psPayload);*/
    DBG_vPrintf(TRACE_ENROLL_REQ,"eCLD_IASZoneEnrollReqSend status =%d",eStatus);

}
/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
