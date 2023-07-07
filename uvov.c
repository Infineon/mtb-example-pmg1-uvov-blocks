/******************************************************************************
* File Name:  uvov.c
*
* Description:  This file contains all the function definitions required for Enabling the
* UV block on PMG1-S2 and the UVP/OVP Interrupt handlers for PMG1-S2
*
* Related Document: See Readme.md
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

#include "uvov.h"

#if defined(CY_DEVICE_SERIES_PMG1S2)

/*******************************************************************************
* Function Name: PMG1_S2_Vbus_UvpEnable
****************************************************************************//**
*
* Enable Under Voltage Protection (UVP) control for the PMG1-S2 device using the internal UV-OV block.
* UVP is only expected to be used while PD-port is the power source.
*
* \param context
* The pointer to the context structure \ref cy_stc_usbpd_context_t allocated
* by the user. The structure is used during the USBPD operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
* \param volt
* Contract Voltage in mV units.
*
* \param cb
* Callback function to be called on fault detection.
*
* \param pctrl
* Flag indicating the type of gate driver to be controlled, true for
* P_CTRL and false for C_CTRL.
*
*******************************************************************************/
void PMG1S2_Vbus_UvpEnable(cy_stc_usbpd_context_t *context, uint16_t volt, cy_cb_vbus_fault_t cb, bool pctrl)
{
    PPDSS_REGS_T pd = context->base;
    uint16_t threshold;
    uint16_t uvpLimit = UVP_MIN_VOLT;
    uint32_t regVal = 0;
    uint8_t filterSel;
    uint8_t level;

    filterSel = (GET_VBUS_OVP_TABLE(context)->debounce + 1) / 2;
    filterSel = CY_USBPD_GET_MIN (filterSel, MAX_UVP_DEBOUNCE_CYCLES);

    /* Clear AUTO MODE OVP detect to avoid false auto off during reference change */
    if (GET_VBUS_OVP_TABLE(context)->mode == CY_USBPD_VBUS_OVP_MODE_UVOV_AUTOCTRL)
    {
        Cy_USBPD_Fault_FetAutoModeDisable (context, pctrl, CY_USBPD_VBUS_FILTER_ID_OV);
    }

    /* Set up UVP callback. */
    context->vbusUvpCbk = cb;

    /* Calculate required VBUS for UVP. */
    threshold = ((volt * GET_VBUS_UVP_TABLE(context)->threshold) / 100);

    /* Ensure that we are within the limits. */
    if (threshold < uvpLimit)
    {
        threshold = uvpLimit;
    }

    /*
     * Calculate UVP comparator threshold setting.
     */
    if (threshold < UVOV_LADDER_BOT)
    {
        level = UVOV_CODE_BOT;
    }
    else if (threshold > UVOV_LADDER_TOP)
    {
        level = UVOV_CODE_TOP;
    }
    else if (threshold <= UVOV_LADDER_MID)
    {
        level = (uint8_t)((threshold - UVOV_LADDER_BOT) / UVOV_LO_STEP_SZ);
    }
    else
    {
        level = (uint8_t)(((threshold - UVOV_LADDER_MID) / UVOV_HI_STEP_SZ) + UVOV_CODE_MID);
    }

    /* Clear UVP positive edge notification. */
    pd->intr3 = PDSS_INTR3_POS_UV_CHANGED;

    /* Configure the UVOV block. */
    regVal = pd->uvov_ctrl & ~(PDSS_UVOV_CTRL_UV_IN_MASK | PDSS_UVOV_CTRL_PD_UVOV);
    regVal |= PDSS_UVOV_CTRL_UVOV_ISO_N;
    pd->uvov_ctrl = (level << PDSS_UVOV_CTRL_UV_IN_POS) | regVal;

    if (GET_VBUS_OVP_TABLE(context)->mode == CY_USBPD_VBUS_OVP_MODE_UVOV_AUTOCTRL)
    {
        Cy_USBPD_Fault_FetAutoModeEnable (context, pctrl, CY_USBPD_VBUS_FILTER_ID_UV);
        Cy_SysLib_DelayUs (10);
    }

    /* If the UV_DET output is already high, flag it. */
    if (pd->ncell_status & PDSS_NCELL_STATUS_UV_STATUS)
    {
        pd->intr3_set |= PDSS_INTR3_POS_UV_CHANGED;
    }

    /* Enable UVP positive edge detection. */
    pd->intr3_mask |= PDSS_INTR3_POS_UV_CHANGED;
}

/*******************************************************************************
* Function Name: PMG1S2_Vbus_UvpIntrHandler
****************************************************************************//**
*
* VBUS UVP fault interrupt handler function for PMG1-S2
*
* \param context
* The pointer to the context structure \ref cy_stc_usbpd_context_t allocated
* by the user. The structure is used during the USBPD operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
*
*******************************************************************************/
void PMG1S2_Vbus_UvpIntrHandler(cy_stc_usbpd_context_t *context)
{
    PPDSS_REGS_T pd = context->base;

    /* Disable and clear UVOV interrupts. */
    pd->intr3_mask &= ~PDSS_INTR3_POS_UV_CHANGED;
    pd->intr3 = PDSS_INTR3_POS_UV_CHANGED;

    /* Invoke UVP callback. */
    if (context->vbusUvpCbk != NULL)
    {
        context->vbusUvpCbk(context, true);
    }
}

/*******************************************************************************
* Function Name: PMG1S2_USBPD_Intr1Handler
****************************************************************************//**
*
* Handle PMG1-S2 UVP/OVP related wake-up interrupt sources.
* INTR3 is mapped to the wake-up interrupt vector.
*
* \param context
* Pointer to the context structure \ref cy_stc_usbpd_context_t.
*
* \return
* None
*
*******************************************************************************/
void PMG1S2_USBPD_Intr1Handler (cy_stc_usbpd_context_t *context)
{
#if defined(CY_DEVICE_SERIES_PMG1S2)
    PPDSS_REGS_T pd = context->base;

    if ((pd->intr3_masked & PDSS_INTR3_POS_OV_CHANGED) != 0U)
    {
        /* Disable and clear the OV interrupt. */
        pd->intr3_mask &= ~PDSS_INTR3_POS_OV_CHANGED;
        pd->intr3 = PDSS_INTR3_POS_OV_CHANGED;

#if PDL_VBUS_OVP_ENABLE
        Cy_USBPD_Fault_Vbus_OvpIntrHandler(context);
#endif /* PDL_VBUS_OVP_ENABLE */
    }

    if ((pd->intr3_masked & PDSS_INTR3_POS_UV_CHANGED) != 0U)
    {
        /* Disable and clear the UV interrupt. */
        pd->intr3_mask &= ~PDSS_INTR3_POS_UV_CHANGED;
        pd->intr3 = PDSS_INTR3_POS_UV_CHANGED;

#if PDL_VBUS_UVP_ENABLE
        PMG1S2_Vbus_UvpIntrHandler(context);
#endif /* PDL_VBUS_UVP_ENABLE */
    }
#endif /* defined(CY_DEVICE_SERIES_PMG1S2) */
}

#endif /* defined(CY_DEVICE_SERIES_PMG1S2) */
