/*****************************************************************************
 *
 * MODULE:             JN-AN-1189
 *
 * COMPONENT:          PingParent.c
 *
 * DESCRIPTION:        Ping Parent if child has been mistakenly aged out
 *
 ****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5148, JN5142,
 * JN5139]. You, and any third parties must reproduce the copyright and
 * warranty notice and any other legend of ownership on each copy or partial
 * copy of the software.
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
 ***************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

/* Stack Includes */
#include <jendefs.h>
#include "dbg.h"
#include "os.h"
#include "os_gen.h"
#include "pdum_apl.h"
#include "pdum_gen.h"
#include "pdm.h"
#include "zps_apl_af.h"
#include "zps_apl_zdp.h"
#include "zps_apl_aib.h"
#include "zps_nwk_nib.h"
#include "zps_nwk_pub.h"
#include "app_timer_driver.h"
#include "Utilities.h"
#include "PingParent.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#ifndef DEBUG_PING_PARENT
#define TRACE_PING_PARENT FALSE
#else
#define TRACE_PING_PARENT TRUE
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
uint32 u32PingTime=0;
static uint16 u16PingNwkAddr = 0;
bool_t bPingSent = FALSE;
bool_t bPingRespRcvd = TRUE;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/


/****************************************************************************/
/***        Tasks                                                         ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: vIncrementPingTime
 *
 * DESCRIPTION:
 * Increment the Time for ping
 *
 * PARAMETERS:
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vIncrementPingTime(uint8 u8Time)
{
    u32PingTime +=u8Time;
}

/****************************************************************************
 *
 * NAME: vResetPingTime
 *
 * DESCRIPTION:
 * Reset the Time for ping
 *
 * PARAMETERS:
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vResetPingTime(void)
{
    u32PingTime = 0;
}

/****************************************************************************
 *
 * NAME: vSetPingAddress
 *
 * DESCRIPTION:
 * Set the Nwk Address where Ping should be done If not set by default ping to coordinator of the system
 *
 * PARAMETERS:
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vSetPingAddress(uint16 u16NwkAddr)
{
    u16PingNwkAddr = u16NwkAddr;
}
/****************************************************************************
 *
 * NAME: bPingParent
 *
 * DESCRIPTION:
 * Read Basic Cluster attribute
 *
 * PARAMETERS:
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC bool_t bPingParent(void)
{
    teZCL_Status eStatus = E_ZCL_FAIL;
    tsZCL_Address              sDestinationAddress;
    uint8                      u8TransactionSequenceNumber;
    uint16                     au16IASZoneAttributeList[1] = {0x0000};

    bPingSent = FALSE;

    if(u32PingTime >= PING_PARENT_TIME )
    {
        u32PingTime = 0;

        /* Send to CIE as communication is always with CIE */
        sDestinationAddress.eAddressMode = E_ZCL_AM_SHORT;
        sDestinationAddress.uAddress.u16DestinationAddress = u16PingNwkAddr;


        eStatus = eZCL_SendReadAttributesRequest(
                    1,                                     /*uint8                        u8SourceEndPointId,*/
                    0xFF,                                  /*uint8                        u8DestinationEndPointId,*/
                    GENERAL_CLUSTER_ID_BASIC,              /*uint16                       u16ClusterId,*/
                    FALSE,                                 /*bool                         bDirectionIsServerToClient,*/
                    &sDestinationAddress,                  /*tsZCL_Address               *psAddress,*/
                    &u8TransactionSequenceNumber,          /*uint8                       *pu8TransactionSequenceNumber,*/
                    1,                                     /*uint8                       u8NumberOfAttributesInRequest,*/
                    FALSE,                                 /*bool                         bIsManufacturerSpecific,*/
                    ZCL_MANUFACTURER_CODE,                 /*uint16                       u16ManufacturerCode);*/
                    &au16IASZoneAttributeList[0]);         /*uint16                     *pu16AttributeRequestList)*/

        if(!eStatus)
        {
            bPingSent=TRUE;
            bPingRespRcvd = FALSE;
            return TRUE;
        }

    }

    return FALSE;
}

/****************************************************************************
 *
 * NAME: vPingRecv
 *
 * DESCRIPTION:
 * called on Read individual attribute response packet to see if
 * its parent is recieved else go for rejoin
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vPingRecv(tsZCL_CallBackEvent  * psEvent)
{
    if(psEvent->pZPSevent->uEvent.sApsDataIndEvent.u16ClusterId == GENERAL_CLUSTER_ID_BASIC)
    {
        if(bPingSent)
        {
            bPingRespRcvd = TRUE;
        }
    }
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
