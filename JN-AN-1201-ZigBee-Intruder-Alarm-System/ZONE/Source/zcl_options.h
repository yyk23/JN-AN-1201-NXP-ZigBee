/*****************************************************************************
 *
 * MODULE:             JN-AN-1201
 *
 * COMPONENT:          zcl_options.h
 *
 * DESCRIPTION:        ZCL Options Header for ZHA DimmableLight
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
 * Copyright NXP B.V. 2013. All rights reserved
 *
 ***************************************************************************/

#ifndef ZCL_OPTIONS_H
#define ZCL_OPTIONS_H

#include <jendefs.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
/* No APS ACK */
#define HA_NO_APS_ACK

/* This is the NXP manufacturer code.If creating new a manufacturer         */
/* specific command apply to the Zigbee alliance for an Id for your company */
/* Also update the manufacturer code in .zpscfg: Node Descriptor->misc      */

#define ZCL_MANUFACTURER_CODE                               0x5950   //公司代码“YP(宜仆)”

#define HA_NUMBER_OF_ZCL_APPLICATION_TIMERS                 3
#define HA_NUMBER_OF_ENDPOINTS                              3
#define EZ_NUMBER_OF_ENDPOINTS                              HA_NUMBER_OF_ENDPOINTS

#define COOPERATIVE

/* Clusters used by this application */

#define CLD_BASIC
#define BASIC_SERVER

#define CLD_IDENTIFY
#define IDENTIFY_SERVER
#define CLD_IDENTIFY_TICKS_PER_SECOND                       10

#define CLD_IASZONE
#define IASZONE_SERVER


/* Disabling APS ACK for Bound transmission */
#define CLD_IASZONE_BOUND_TX_WITH_APS_ACK_DISABLED

/****************************************************************************/
/*             Basic Cluster - Optional Attributes                          */
/*                                                                          */
/* Add the following #define's to your zcl_options.h file to add optional   */
/* attributes to the basic cluster.                                         */
/****************************************************************************/

#define ZCL_ATTRIBUTE_READ_SERVER_SUPPORTED
#define ZCL_ATTRIBUTE_READ_CLIENT_SUPPORTED
#define ZCL_ATTRIBUTE_WRITE_SERVER_SUPPORTED
#define ZCL_ATTRIBUTE_WRITE_CLIENT_SUPPORTED
#define ZCL_ATTRIBUTE_WRITE_SERVER_SUPPORTED

#define ZCL_ATTRIBUTE_REPORTING_SERVER_SUPPORTED
#define ZCL_CONFIGURE_ATTRIBUTE_REPORTING_SERVER_SUPPORTED
#define ZCL_READ_ATTRIBUTE_REPORTING_CONFIGURATION_SERVER_SUPPORTED

#define ZCL_COMMAND_DISCOVERY_SUPPORTED

#define HA_NUMBER_OF_REPORTS 2
#define HA_SYSTEM_MIN_REPORT_INTERVAL   1
#define HA_SYSTEM_MAX_REPORT_INTERVAL   0x3c

#define CLD_BAS_ATTR_APPLICATION_VERSION
#define CLD_BAS_ATTR_STACK_VERSION
#define CLD_BAS_ATTR_HARDWARE_VERSION
#define CLD_BAS_ATTR_MANUFACTURER_NAME
#define CLD_BAS_ATTR_MODEL_IDENTIFIER
#define CLD_BAS_ATTR_DATE_CODE

//设置端点的BAS参数，如软件版本，硬件版本

#define CLD_BAS_APP_VERSION         (1)
#define CLD_BAS_STACK_VERSION       (2)
#define CLD_BAS_HARDWARE_VERSION    (1)
#define CLD_BAS_MANUF_NAME_SIZE     (4)
#define CLD_BAS_MODEL_ID_SIZE       (7)
#define CLD_BAS_DATE_SIZE           (8)
#define CLD_BAS_POWER_SOURCE        E_CLD_BAS_PS_BATTERY    //电池

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

#endif /* ZCL_OPTIONS_H */
