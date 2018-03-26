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
static PDM_teStatus eStatusReloadCIE1,eStatusReloadCIE2,eStatusReloadCIE3,eStatusReloadCIE4,eStatusReloadCIE5;


/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

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
	/* Loading Number of Discovered devices From EEPROM */
    eStatusReloadCIE1 = PDM_eReadDataFromRecord(        PDM_ID_APP_IASCIE_NODE,
                                                        &u8Discovered,
                                                        sizeof(uint8),
                                                        &u16ByteRead);
    DBG_vPrintf(TRACE_PDM_SAVE,"eStatusReload=%d\n",eStatusReloadCIE1);

    /* Loading Discovered Table From EEPROM */
    eStatusReloadCIE2 = PDM_eReadDataFromRecord(        PDM_ID_APP_IASCIE_STRUCT,
                                                        &sDiscovedZoneServers[0],
                                                        sizeof(tsDiscovedZoneServers) * MAX_ZONE_SERVER_NODES,
                                                        &u16ByteRead);
    DBG_vPrintf(TRACE_PDM_SAVE,"eStatusReload=%d\n",eStatusReloadCIE2);


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
