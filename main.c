/******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for the PMG1 Using UVOV Blocks Code
* Example for ModusToolbox.
*
* Related Document: See README.md
*
*
*******************************************************************************
*  Copyright 2022-2023, Cypress Semiconductor Corporation (an Infineon company) or
*  an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
* 
*  This software, including source code, documentation and related
*  materials ("Software") is owned by Cypress Semiconductor Corporation
*  or one of its affiliates ("Cypress") and is protected by and subject to
*  worldwide patent protection (United States and foreign),
*  United States copyright laws and international treaty provisions.
*  Therefore, you may use this Software only as provided in the license
*  agreement accompanying the software package from which you
*  obtained this Software ("EULA").
*  If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
*  non-transferable license to copy, modify, and compile the Software
*  source code solely for use in connection with Cypress's
*  integrated circuit products.  Any reproduction, modification, translation,
*  compilation, or representation of this Software except as specified
*  above is prohibited without the express written permission of Cypress.
* 
*  Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
*  EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
*  reserves the right to make changes to the Software without notice. Cypress
*  does not assume any liability arising out of the application or use of the
*  Software or any product or circuit described in the Software. Cypress does
*  not authorize its products for use in any products where a malfunction or
*  failure of the Cypress product may reasonably be expected to result in
*  significant property damage, injury or death ("High Risk Product"). By
*  including Cypress's product in a High Risk Product, the manufacturer
*  of such system or application assumes all risk of such use and in doing
*  so agrees to indemnify Cypress against all liability.
*******************************************************************************/

/*******************************************************************************
 * Include header files
 ******************************************************************************/
#include <stdio.h>
#include <inttypes.h>
#include "uvov.h"

/*******************************************************************************
* Macros
*******************************************************************************/
/* CY ASSERT failure */
#define CY_ASSERT_FAILED                       (0u)

/* Voltage value in mV for calculating overvoltage and undervoltage thresholds */
#define THRESHOLD_VOLT                         (5000u)

/* Hysteresis Voltage used to ignore oscillitation in voltage when Vbus voltage
 * is on the UVP or OVP threshold
 * PMG1-S2 has a higher hysteresis voltage for UVP due to its limited set of UV
 * threshold values and input VBUS voltage step sizes.
 */
#define HYST_OVP_VOLT                          (4950u)
#if defined(CY_DEVICE_SERIES_PMG1S2)
#define HYST_UVP_VOLT                          (5400u)
#else
#define HYST_UVP_VOLT                          (5150u)
#endif

/* Flag indicating the type of gate driver to be controlled, true for Provider FET control */
#define PROVIDER_FET_CTRL                      (1u)

/* Macro values used for Vbus status */
typedef enum
{
    VBUS_UNDERVOLTAGE = 0,
    VBUS_OVERVOLTAGE  = 1,
    VBUS_NORMAL       = 2
} vbus_state_t;

/* Debug print macro to enable UART print */
#define DEBUG_PRINT                            (0u)

/*******************************************************************************
* Global Variables
********************************************************************************/
/* USBPD context*/
cy_stc_usbpd_context_t USBPD_context;

/* Flag used to indicate whether a OVP interrupt has occurred or not */
volatile bool OVP_FLAG = 0;

/* Flag used to indicate whether a UVP interrupt has occurred or not */
volatile bool UVP_FLAG = 0;

/*******************************************************************************
* Function Prototypes
********************************************************************************/
/* OVP callback function */
void ovp_cb(void *context, bool compOut)
{
    (void)context;
    (void)compOut;
    /* OVP interrupt has triggered, set the OVP flag */
    OVP_FLAG = 1;
}

/* UVP callback function */
void uvp_cb(void *context, bool compOut)
{
    (void)context;
    (void)compOut;
    /* UVP interrupt has triggered, set the UVP flag */
    UVP_FLAG = 1;
}

/* Interrupt handler for USBPD Port of the device
 * PMG1-S2 needs a different funciton call compared to other PMG1 devices due to
 * the different interupt register used for PMG1-S2 UVP and OVP.
 */
static void cy_usbpd0_intr1_handler(void)
{
/* The MACRO CY_DEVICE_SERIES_PMG1S2 is automatically set to 1 by ModusToolbox
 * when PMG1-S2 device is selected for the application
 */
 #if defined(CY_DEVICE_SERIES_PMG1S2)
    PMG1S2_USBPD_Intr1Handler(&USBPD_context);
 #else
     Cy_USBPD_Intr1Handler(&USBPD_context);
 #endif
}

/* Interrupt configuration for USBPD Port of the device */
const cy_stc_sysint_t usbpd_port0_intr1_config =
{
    .intrSrc = (IRQn_Type)mtb_usbpd_port0_DS_IRQ,
    .intrPriority = 1U,
};

/* Part of USBPD driver initialization */
cy_stc_pd_dpm_config_t* get_dpm_connect_stat()
{
    /* This value is not required here, hence NULL is returned */
    return NULL;
}

/*******************************************************************************
* Function Name: enable_ovp
********************************************************************************
* Summary:
*  Calls the API to enable the OVP block and interrupt
*
* Parameters:
*  context - the USBPD context
*  volt - the voltage used to calculate the OVP threshold
*
* Return:
*  void
*
*******************************************************************************/
void enable_ovp(cy_stc_usbpd_context_t *context, uint16_t volt)
{
    uint8_t intr_state = Cy_SysLib_EnterCriticalSection();
    Cy_USBPD_Fault_Vbus_OvpEnable(context, volt, (cy_cb_vbus_fault_t)ovp_cb, PROVIDER_FET_CTRL);
    Cy_SysLib_ExitCriticalSection(intr_state);
}

/*******************************************************************************
* Function Name: enable_uvp
********************************************************************************
* Summary:
*  Calls the API to enable the UVP block and interrupt
*
* Parameters:
*  context - the USBPD context
*  volt - the voltage used to calculate the UVP threshold
*
* Return:
*  void
*
*******************************************************************************/
void enable_uvp(cy_stc_usbpd_context_t *context, uint16_t volt)
{
    uint8_t intr_state = Cy_SysLib_EnterCriticalSection();
#if defined(CY_DEVICE_SERIES_PMG1S2)
    PMG1S2_Vbus_UvpEnable(context, volt, (cy_cb_vbus_fault_t)uvp_cb, PROVIDER_FET_CTRL);
#else
    Cy_USBPD_Fault_Vbus_UvpEnable(context, volt, (cy_cb_vbus_fault_t)uvp_cb, PROVIDER_FET_CTRL);
#endif /* defined(CY_DEVICE_SERIES_PMG1S2) */
    Cy_SysLib_ExitCriticalSection(intr_state);
}

/*******************************************************************************
* Function Name: vbus_status
********************************************************************************
* Summary:
*  Returns the voltage status of Vbus
*
* Parameters:
*  context - the USBPD context
*
* Return:
*  uint8_t
*
*******************************************************************************/
/* Function that returns the voltage status of Vbus */
vbus_state_t vbus_status(cy_stc_usbpd_context_t *context)
{
    vbus_state_t vbus_uvov_status = VBUS_NORMAL;
    uint8_t comp_status;
#if defined(CY_DEVICE_SERIES_PMG1S2)
    /* PMG1-S2 overvoltage and undervoltage comparators status bits are available
     * on the ncell_status register
     */
    comp_status = (context->base->ncell_status);
    if(comp_status & PDSS_NCELL_STATUS_OV_STATUS)
    {
        vbus_uvov_status = VBUS_OVERVOLTAGE;
    }
    else if(comp_status & PDSS_NCELL_STATUS_UV_STATUS)
    {
        vbus_uvov_status = VBUS_UNDERVOLTAGE;
    }
#else
    /* On other PMG1 devices the overvoltage and undervoltage comparator ouput bits
     * are available on the intr5_status_0 register
     */
    comp_status = (context->base->intr5_status_0);
    if(comp_status & (1U << CY_USBPD_VBUS_FILTER_ID_OV))
    {
        vbus_uvov_status = VBUS_OVERVOLTAGE;
    }
    else if(!(comp_status & (1U << CY_USBPD_VBUS_FILTER_ID_UV)))
    {
        vbus_uvov_status = VBUS_UNDERVOLTAGE;
    }
#endif /* defined(CY_DEVICE_SERIES_PMG1S2) */
    return vbus_uvov_status;
}

#if DEBUG_PRINT
cy_stc_scb_uart_context_t UART_context;

/* Variable used for tracking the print status */
volatile bool ENTER_LOOP = true;

/*******************************************************************************
* Function Name: check_status
********************************************************************************
* Summary:
*  Prints the error message.
*
* Parameters:
*  error_msg - message to print if any error encountered.
*  status - status obtained after evaluation.
*
* Return:
*  void
*
*******************************************************************************/
void check_status(char *message, cy_rslt_t status)
{
    char error_msg[50];

    sprintf(error_msg, "Error Code: 0x%08" PRIX32 "\n", status);

    Cy_SCB_UART_PutString(CYBSP_UART_HW, "\r\n=====================================================\r\n");
    Cy_SCB_UART_PutString(CYBSP_UART_HW, "\nFAIL: ");
    Cy_SCB_UART_PutString(CYBSP_UART_HW, message);
    Cy_SCB_UART_PutString(CYBSP_UART_HW, "\r\n");
    Cy_SCB_UART_PutString(CYBSP_UART_HW, error_msg);
    Cy_SCB_UART_PutString(CYBSP_UART_HW, "\r\n=====================================================\r\n");
}
#endif

/*******************************************************************************
* Function Name: main
********************************************************************************
* Summary:
*  System entrance point. This function performs
*  - initial setup of device
*  - USBPD driver initialization 
*  - enables the UVP and OVP blocks
*  - prints UVP and OVP detection message to UART when applicable
*
* Parameters:
*  none
*
* Return:
*  int
*
*******************************************************************************/
int main(void)
{
    cy_rslt_t result;
    cy_en_usbpd_status_t usbpd_result;
    cy_en_sysint_status_t sysint_result;

    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board initialization failed. Stop program execution */
    if(result != CY_RSLT_SUCCESS)
    {
        /* Insert error handler here */
        CY_ASSERT(CY_ASSERT_FAILED);
    }

#if DEBUG_PRINT
    /* Configure and enable the UART peripheral */
    Cy_SCB_UART_Init(CYBSP_UART_HW, &CYBSP_UART_config, &UART_context);
    Cy_SCB_UART_Enable(CYBSP_UART_HW);

    /* Sequence to clear screen */
    Cy_SCB_UART_PutString(CYBSP_UART_HW, "\x1b[2J\x1b[;H");

    /* Print "Program Start" */
    Cy_SCB_UART_PutString(CYBSP_UART_HW, "****************** ");
    Cy_SCB_UART_PutString(CYBSP_UART_HW, "PMG1 MCU: Using UVOV Blocks ");
    Cy_SCB_UART_PutString(CYBSP_UART_HW, "****************** \r\n\n");
#endif

    /* Enable global interrupts */
    __enable_irq();

    /* To set data field in USBPD context structure to NULL.
     * Required for uninterrupted USBPD driver initialization */
    memset((void *)&USBPD_context, 0, sizeof (cy_stc_usbpd_context_t));

    /* Configure and enable the USBPD interrupts */
    sysint_result = Cy_SysInt_Init(&usbpd_port0_intr1_config, &cy_usbpd0_intr1_handler);
    /* System Interrupt init failed. Stop program execution */
    if (sysint_result != CY_SYSINT_SUCCESS)
    {
#if DEBUG_PRINT
        check_status("API Cy_SysInt_Init failed with error code", sysint_result);
#endif
        CY_ASSERT(CY_ASSERT_FAILED);
    }
    NVIC_EnableIRQ(usbpd_port0_intr1_config.intrSrc);

    /* Initialize the USBPD driver */
#if defined(CY_DEVICE_SERIES_PMG1S2)
    usbpd_result = Cy_USBPD_Init(&USBPD_context, 0, mtb_usbpd_port0_HW, NULL,
            (cy_stc_usbpd_config_t *)&mtb_usbpd_port0_config, get_dpm_connect_stat);
#else
    usbpd_result = Cy_USBPD_Init(&USBPD_context, 0, mtb_usbpd_port0_HW, mtb_usbpd_port0_HW_TRIM,
            (cy_stc_usbpd_config_t *)&mtb_usbpd_port0_config, get_dpm_connect_stat);
#endif /* defined(CY_DEVICE_SERIES_PMG1S2) */

    /* USBPD driver init failed. Stop program execution */
    if (usbpd_result != CY_USBPD_STAT_SUCCESS)
    {
#if DEBUG_PRINT
        check_status("API Cy_USBPD_Init failed with error code", usbpd_result);
#endif
        CY_ASSERT(CY_ASSERT_FAILED);
    }

    /* If the USBPD UVP feature is enabled, enable and configure the UVP block */
    cy_stc_fault_vbus_uvp_cfg_t * uvp_config = (cy_stc_fault_vbus_uvp_cfg_t *) USBPD_context.usbpdConfig->vbusUvpConfig;
    if (uvp_config->enable)
    {
        enable_uvp(&USBPD_context, THRESHOLD_VOLT);
    }

    /* If the USBPD OVP feature is enabled, enable and configure the OVP block */
    cy_stc_fault_vbus_ovp_cfg_t * ovp_config = (cy_stc_fault_vbus_ovp_cfg_t *) USBPD_context.usbpdConfig->vbusOvpConfig;
    if (ovp_config->enable)
    {
        enable_ovp(&USBPD_context, THRESHOLD_VOLT);
    }

    for(;;)
    {
        /* Check if an OVP interrupt has occurred */
        if(OVP_FLAG)
        {
#if DEBUG_PRINT
            Cy_SCB_UART_PutString(CYBSP_UART_HW, "OVP Fault detected.\r\n");
#endif
            /* Set the OVP comparator using OVP hysteresis voltage to ignore small changes on VBUS */
            enable_ovp(&USBPD_context, HYST_OVP_VOLT);
            /* Toggle the LED 8 times per second while there is overvoltage on Vbus */
            while(vbus_status(&USBPD_context) == VBUS_OVERVOLTAGE)
            {
                /* Toggle the user LED state */
                Cy_GPIO_Inv(CYBSP_USER_LED_PORT, CYBSP_USER_LED_PIN);
                /* Delay between LED toggles during Vbus overvoltage */
                Cy_SysLib_Delay(125);
            }
            /* Clear the OVP Flag and enable the OVP interrupt */
            OVP_FLAG = 0 ;
            enable_ovp(&USBPD_context, THRESHOLD_VOLT);
            /* Set User LED to ON (Vbus normal status) */
            Cy_GPIO_Clr(CYBSP_USER_LED_PORT, CYBSP_USER_LED_PIN);
#if DEBUG_PRINT
                Cy_SCB_UART_PutString(CYBSP_UART_HW, "No overvoltage detected.\r\n");
#endif
        }
        /* Check if a UVP interrupt has occurred */
        if(UVP_FLAG)
        {
#if DEBUG_PRINT
            Cy_SCB_UART_PutString(CYBSP_UART_HW, "UVP Fault detected.\r\n");
#endif
            /* Set the UVP comparator using UVP hysteresis voltage to ignore small changes on VBUS */
            enable_uvp(&USBPD_context, HYST_UVP_VOLT);
            /* Toggle the LED once every second while there is undervoltage on Vbus */
            while(vbus_status(&USBPD_context) == VBUS_UNDERVOLTAGE)
            {
                /* Toggle the user LED state */
                Cy_GPIO_Inv(CYBSP_USER_LED_PORT, CYBSP_USER_LED_PIN);
                /* Delay between LED toggles during Vbus undervoltage */
                Cy_SysLib_Delay(1000);
            }
            /* Clear the UVP Flag and enable the UVP interrupt */
            UVP_FLAG = 0 ;
            enable_uvp(&USBPD_context, THRESHOLD_VOLT);
            /* Set User LED to ON (Vbus normal status) */
            Cy_GPIO_Clr(CYBSP_USER_LED_PORT, CYBSP_USER_LED_PIN);
#if DEBUG_PRINT
                Cy_SCB_UART_PutString(CYBSP_UART_HW, "No undervoltage detected.\r\n");
#endif
        }
#if DEBUG_PRINT
        if (ENTER_LOOP)
        {
            Cy_SCB_UART_PutString(CYBSP_UART_HW, "Entered for loop\r\n");
            ENTER_LOOP = false;
        }
#endif
    }
}
/* [] END OF FILE */
