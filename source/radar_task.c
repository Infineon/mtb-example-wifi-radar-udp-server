/*****************************************************************************
 * File name: radar_task.c
 *
 * Description: This file uses RadarSensing library APIs to demonstrate
 * reading data from radar with specific configuration.
 *
 * Related Document: See README.md
 *
 * ===========================================================================
 * Copyright (C) 2022 Infineon Technologies AG. All rights reserved.
 * ===========================================================================
 *
 * ===========================================================================
 * Infineon Technologies AG (INFINEON) is supplying this file for use
 * exclusively with Infineon's sensor products. This file can be freely
 * distributed within development tools and software supporting such
 * products.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 * OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 * INFINEON SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES, FOR ANY REASON
 * WHATSOEVER.
 * ===========================================================================
 */

/* Header file from system */
#include <inttypes.h>
#include <stdio.h>

/* Header file includes */
#include "cybsp.h"
#include "cyhal.h"

#include "rtos_artifacts.h"

/* Header file for local task */
#include "radar_config_task.h"

#include "radar_task.h"
#include "udp_server.h"
#include "xensiv_bgt60trxx_mtb.h"

#define XENSIV_BGT60TRXX_CONF_IMPL
#include "radar_settings.h"
/*******************************************************************************
 * Macros
 ******************************************************************************/
#define PIN_XENSIV_BGT60TRXX_SPI_SCLK       CYBSP_SPI_CLK
#define PIN_XENSIV_BGT60TRXX_SPI_MOSI       CYBSP_SPI_MOSI
#define PIN_XENSIV_BGT60TRXX_SPI_MISO       CYBSP_SPI_MISO
#define PIN_XENSIV_BGT60TRXX_SPI_CSN        CYBSP_SPI_CS
#define PIN_XENSIV_BGT60TRXX_IRQ            CYBSP_GPIO10
#define PIN_XENSIV_BGT60TRXX_RSTN           CYBSP_GPIO11
#define PIN_XENSIV_BGT60TRXX_LDO_EN         CYBSP_GPIO5

#define XENSIV_BGT60TRXX_SPI_FREQUENCY      (25000000UL)

#define NUM_SAMPLES_PER_FRAME               (XENSIV_BGT60TRXX_CONF_NUM_SAMPLES_PER_CHIRP *\
                                             XENSIV_BGT60TRXX_CONF_NUM_CHIRPS_PER_FRAME *\
                                             XENSIV_BGT60TRXX_CONF_NUM_RX_ANTENNAS)

#define GPIO_INTERRUPT_PRIORITY             (6)


/*******************************************************************************
 * Global Variables
 ******************************************************************************/
TaskHandle_t radar_task_handle = NULL;

static cyhal_spi_t spi_obj;
static xensiv_bgt60trxx_mtb_t bgt60_obj;
static uint16_t bgt60_buffer[NUM_SAMPLES_PER_FRAME + 3] __attribute__((aligned(2)));

static uint32_t frame_num = 0;
static publisher_data_t udp_data = {
    .data = (uint8_t *)bgt60_buffer,
    .cmd =  1,
    .length = (NUM_SAMPLES_PER_FRAME * 2) + 6
};
static publisher_data_t * publisher_msg = &udp_data;
static bool test_mode = false;

/*******************************************************************************
* Function Name: xensiv_bgt60trxx_interrupt_handler
********************************************************************************
* Summary:
* This is the interrupt handler to react on sensor indicating the availability
* of new data
*    1. Notifies radar task that there is an interrupt from the sensor
*
* Parameters:
*  args : pointer to pass parameters to callback
*  event: gpio event
* Return:
*  none
*
*******************************************************************************/
#if defined(CYHAL_API_VERSION) && (CYHAL_API_VERSION >= 2)
static void xensiv_bgt60trxx_interrupt_handler(void *args, cyhal_gpio_event_t event)
#else
static void xensiv_bgt60trxx_interrupt_handler(void *args, cyhal_gpio_irq_event_t event)
#endif
{
    CY_UNUSED_PARAMETER(args);
    CY_UNUSED_PARAMETER(event);

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    vTaskNotifyGiveFromISR(radar_task_handle, &xHigherPriorityTaskWoken);

    /* Context switch needed? */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/*******************************************************************************
* Function Name: init_sensor
********************************************************************************
* Summary:
* This function configures the SPI interface, initializes radar and interrupt
* service routine to indicate the availability of radar data.
*
* Parameters:
*  void
*
* Return:
*  Success or error
*
*******************************************************************************/
static int32_t init_sensor(void)
{
    if (cyhal_spi_init(&spi_obj,
                       PIN_XENSIV_BGT60TRXX_SPI_MOSI,
                       PIN_XENSIV_BGT60TRXX_SPI_MISO,
                       PIN_XENSIV_BGT60TRXX_SPI_SCLK,
                       NC,
                       NULL,
                       8,
                       CYHAL_SPI_MODE_00_MSB,
                       false) != CY_RSLT_SUCCESS)
    {
        printf("ERROR: cyhal_spi_init failed\n");
        return RESULT_ERROR;
    }

    /* Reduce drive strength to improve EMI */
    Cy_GPIO_SetSlewRate(CYHAL_GET_PORTADDR(PIN_XENSIV_BGT60TRXX_SPI_MOSI), CYHAL_GET_PIN(PIN_XENSIV_BGT60TRXX_SPI_MOSI), CY_GPIO_SLEW_FAST);
    Cy_GPIO_SetDriveSel(CYHAL_GET_PORTADDR(PIN_XENSIV_BGT60TRXX_SPI_MOSI), CYHAL_GET_PIN(PIN_XENSIV_BGT60TRXX_SPI_MOSI), CY_GPIO_DRIVE_1_8);
    Cy_GPIO_SetSlewRate(CYHAL_GET_PORTADDR(PIN_XENSIV_BGT60TRXX_SPI_SCLK), CYHAL_GET_PIN(PIN_XENSIV_BGT60TRXX_SPI_SCLK), CY_GPIO_SLEW_FAST);
    Cy_GPIO_SetDriveSel(CYHAL_GET_PORTADDR(PIN_XENSIV_BGT60TRXX_SPI_SCLK), CYHAL_GET_PIN(PIN_XENSIV_BGT60TRXX_SPI_SCLK), CY_GPIO_DRIVE_1_8);

    /* Set the data rate to 25 Mbps */
    if (cyhal_spi_set_frequency(&spi_obj, XENSIV_BGT60TRXX_SPI_FREQUENCY) != CY_RSLT_SUCCESS)
    {
        printf("ERROR: cyhal_spi_set_frequency failed\n");
        return RESULT_ERROR;
    }

    /* Enable LDO */
    if (cyhal_gpio_init(PIN_XENSIV_BGT60TRXX_LDO_EN,
                        CYHAL_GPIO_DIR_OUTPUT,
                        CYHAL_GPIO_DRIVE_STRONG,
                        true) != CY_RSLT_SUCCESS)
    {
        printf("ERROR: LDO_EN cyhal_gpio_init failed\n");
        return RESULT_ERROR;
    }

    /* Wait LDO stable */
    (void)cyhal_system_delay_ms(5);

    if (xensiv_bgt60trxx_mtb_init(&bgt60_obj,
                                  &spi_obj,
                                  PIN_XENSIV_BGT60TRXX_SPI_CSN,
                                  PIN_XENSIV_BGT60TRXX_RSTN,
                                  register_list,
                                  XENSIV_BGT60TRXX_CONF_NUM_REGS) != CY_RSLT_SUCCESS)
    {
        printf("ERROR: xensiv_bgt60trxx_mtb_init failed\n");
        return RESULT_ERROR;
    }

    if (xensiv_bgt60trxx_mtb_interrupt_init(&bgt60_obj,
                                            NUM_SAMPLES_PER_FRAME,
                                            PIN_XENSIV_BGT60TRXX_IRQ,
                                            GPIO_INTERRUPT_PRIORITY,
                                            xensiv_bgt60trxx_interrupt_handler,
                                            NULL) != CY_RSLT_SUCCESS)
    {
        printf("ERROR: xensiv_bgt60trxx_mtb_interrupt_init failed\n");
        return RESULT_ERROR;
    }

    return RESULT_SUCCESS;
}
/*******************************************************************************
 * Function Name: test_radar_spi_data_1rx
 *******************************************************************************
 * Summary:
 *  This function takes radar input data and verifies it for 1 rx antenna with
 *  generated test data.
 *
 * Parameters:
 *   samples : pointer to hold frame buffer containing samples
 *
 * Return:
 *   error
 *
 ******************************************************************************/
static void test_radar_spi_data_1rx(const uint16_t *samples)
{
    static uint32_t frame_idx = 0;
    static uint16_t test_word = XENSIV_BGT60TRXX_INITIAL_TEST_WORD;

    /* Check received data */
    for (int32_t sample_idx = 0; sample_idx < NUM_SAMPLES_PER_FRAME; ++sample_idx)
    {
        if ((sample_idx % XENSIV_BGT60TRXX_CONF_NUM_RX_ANTENNAS) == 0)
        {
            if (test_word != samples[sample_idx])
            {
                printf("Frame %" PRIu32 " error detected. "
                       "Expected: %" PRIu16 ". "
                       "Received: %" PRIu16 "\n",
                       frame_idx, test_word, samples[sample_idx]);
                CY_ASSERT(false);
            }
        }

        // Generate next test_word
        test_word = xensiv_bgt60trxx_get_next_test_word(test_word);
    }

    sprintf((char *)publisher_msg->data, "Frame %" PRIu32 " received correctly", frame_idx);
    publisher_msg->length = strlen((const char *) publisher_msg->data);

    /* Send message back to publish queue. */
    xQueueSendToBack(radar_data_queue, &publisher_msg, 0 );

    frame_idx++;
}

/*******************************************************************************
 * Function Name: radar_task
 *******************************************************************************
 * Summary:
 *
 *   Initializes radar sensor, create configuration task and continuously
 *   process data acquired from radar.
 *
 * Parameters:
 *   pvParameters: thread
 *
 * Return:
 *   none
 ******************************************************************************/
void radar_task(void *pvParameters)
{

    (void)pvParameters;


    if (init_sensor() != RESULT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /**
     * Create task for radar configuration. Configuration parameters come from
     * udp client task.
     */
    if (pdPASS != xTaskCreate(radar_config_task,
                              RADAR_CONFIG_TASK_NAME,
                              RADAR_CONFIG_TASK_STACK_SIZE,
                              NULL,
                              RADAR_CONFIG_TASK_PRIORITY,
                              &radar_config_task_handle))
    {
        printf("Failed to create Radar config task!\n");
        CY_ASSERT(0);
    }

    printf("Radar device initialized successfully. Waiting for start from UDP client...\n\n");

    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (xensiv_bgt60trxx_get_fifo_data(&bgt60_obj.dev,
                                           &bgt60_buffer[3],
                                           NUM_SAMPLES_PER_FRAME) == XENSIV_BGT60TRXX_STATUS_OK)
        {
            if(!test_mode)
            {
                frame_num++;
                publisher_msg->data[0] = RADAR_DATA_COMMAND;
                publisher_msg->data[1] = DUMMY_BYTE;
                publisher_msg->data[2] = (uint8_t)(frame_num & 0x000000ff);
                publisher_msg->data[3] = (uint8_t)((frame_num & 0x0000ff00) >> 8);
                publisher_msg->data[4] = (uint8_t)((frame_num & 0x00ff0000) >> 16);
                publisher_msg->data[5] = (uint8_t)((frame_num & 0xff000000) >> 24);

                publisher_msg->length = (NUM_SAMPLES_PER_FRAME * 2) + 6;

                /* Send message back to publish queue. */
                xQueueSendToBack(radar_data_queue, &publisher_msg, 0 );
            }
            else
            {
                test_radar_spi_data_1rx(&bgt60_buffer[3]);
            }

        }
    }
}
/*******************************************************************************
 * Function Name: radar_start
 *******************************************************************************
 * Summary:
 *   to start/stop radar device.
 *
 * Parameters:
 *   start : start/stop value for radar device
 *
 * Return:
 *   error
 ******************************************************************************/
int32_t radar_start(bool start)
{
    if (xensiv_bgt60trxx_start_frame(&bgt60_obj.dev, start) != XENSIV_BGT60TRXX_STATUS_OK)
    {
        return RESULT_ERROR;
    }

    return RESULT_SUCCESS;
}
/*******************************************************************************
 * Function Name: radar_enable_test_mode
 *******************************************************************************
 * Summary:
 *   Starting/stopping radar test mode.
 *
 * Parameters:
 *   start : start/stop value for radar device in test mode
 *
 * Return:
 *   error
 ******************************************************************************/
int32_t radar_enable_test_mode(bool start)
{
    test_mode = start;

    /* Enable sensor data test mode. The data received on antenna RX1 will be overwritten by
       a deterministic sequence of data generated by the test pattern generator */
    if (xensiv_bgt60trxx_enable_data_test_mode(&bgt60_obj.dev,true) != XENSIV_BGT60TRXX_STATUS_OK)
    {
        return RESULT_ERROR;
    }

    return RESULT_SUCCESS;
}

/* [] END OF FILE */
