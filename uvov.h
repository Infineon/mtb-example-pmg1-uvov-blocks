/******************************************************************************
* File Name: ovuv.h
*
* Description: This is header file for the PMG1 MCU Using UVOV Blocks Code Example.
*              This file defines the function and macros used to enable the UV block
*              on PMG1-S2 and
*
*
* Related Document: See README.md
*
*******************************************************************************
* Copyright 2022-2023, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

#ifndef _UVOV_H_
#define _UVOV_H_

#include "cybsp.h"
#include "cy_pdl.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#if defined(CY_DEVICE_SERIES_PMG1S2)
/*
 * Minimum supported voltage for UVP on PMG1-S2. Any voltage lower may cause system to
 * not work as expected; the block references can get affected. This is now
 * limited to 3.1V.
 */
#define UVP_MIN_VOLT                (3100)

#define MAX_UVP_DEBOUNCE_CYCLES     (0x20u)

/*
 *  Input ladder voltages, code limits and step sizes from Table 26 of the
 *  HardIP BROS (001-98391).
 */
#define UVOV_LADDER_BOT     (2750u)
#define UVOV_LADDER_MID     (9000u)
#define UVOV_LADDER_TOP     (21500u)
#define UVOV_CODE_BOT       (0)
#define UVOV_CODE_MID       (25u)
#define UVOV_CODE_TOP       (50u)
#define UVOV_CODE_MAX       (63u)
#define UVOV_CODE_6V0       (13u)
#define UVOV_LO_STEP_SZ     (250u)
#define UVOV_HI_STEP_SZ     (500u)
#endif /* defined(CY_DEVICE_SERIES_PMG1S2) */

/*******************************************************************************
* Function Prototypes
*******************************************************************************/
#if defined(CY_DEVICE_SERIES_PMG1S2)

void PMG1S2_Vbus_UvpEnable(cy_stc_usbpd_context_t *context, uint16_t volt, cy_cb_vbus_fault_t cb, bool pctrl);

void PMG1S2_Vbus_UvpIntrHandler(cy_stc_usbpd_context_t *context);

void PMG1S2_USBPD_Intr1Handler (cy_stc_usbpd_context_t *context);

#endif /* defined(CY_DEVICE_SERIES_PMG1S2) */

#endif /* _UVOV_H_ */

/* End of file [] */
