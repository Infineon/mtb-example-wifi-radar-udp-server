/******************************************************************************
 * File Name:   radar_config_task.c
 *
 * Description: This file contains the task that handles parsing the new
 *              configuration coming from remote server and setting it to the
 *              sensor-xensiv-bgt60trxx library.
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
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

/* Header file from library */
#include "cy_json_parser.h"

#include "rtos_artifacts.h"

/* Header file for local tasks */
#include "radar_config_task.h"
#include "radar_task.h"
#include "udp_server.h"

/* Strings objects and values for radar operation */
#define RADAR_STRING  ("radar_transmission")
#define ENABLE_STRING  ("enable")
#define DISABLE_STRING ("disable")
#define TEST_STRING ("test")

#define RADAR_STR_LENGTH strlen(RADAR_STRING)
#define ENABLE_STR_LENGTH strlen(ENABLE_STRING)
#define DISABLE_STR_LENGTH strlen(DISABLE_STRING)
#define TEST_STR_LENGTH strlen(TEST_STRING)


/*******************************************************************************
 * Global Variables
 ******************************************************************************/
TaskHandle_t radar_config_task_handle = NULL;

/*******************************************************************************
 * Function Name: json_parser_cb
 *******************************************************************************
 * Summary:
 *   Callback function that parses incoming json string.
 *
 * Parameters:
 *      json_object: incoming json object
 *      arg: callback data pointer
 *
 * Return:
 *   error
 ******************************************************************************/
static cy_rslt_t json_parser_cb(cy_JSON_object_t *json_object, void *arg)
{
    /* Supported keys and values for radar data transmission */
    if ((json_object->object_string_length == RADAR_STR_LENGTH) && (memcmp(json_object->object_string, RADAR_STRING, json_object->object_string_length) == 0))
    {

        if (( json_object->value_length == ENABLE_STR_LENGTH) && (memcmp(json_object->value, ENABLE_STRING, json_object->value_length) == 0))
        {
            radar_start(true);
            printf("Radar data transmission is enabled \r\n");
        }
        else if (( json_object->value_length == DISABLE_STR_LENGTH) && (memcmp(json_object->value, DISABLE_STRING, json_object->value_length) == 0))
        {
            radar_start(false);
            printf("Radar data transmission is disabled \r\n");
        }
        else if(( json_object->value_length == TEST_STR_LENGTH) && (memcmp(json_object->value, TEST_STRING, json_object->value_length) == 0))
        {
            if (radar_start(true) != RESULT_SUCCESS)
            {
                printf("Failed to write to radar device\n");
            }
            else
            {
                if (radar_enable_test_mode(true) != RESULT_SUCCESS)
                {
                    printf("Failed to enable test mode for radar device\n");
                }
                else
                {
                printf("Radar test data transmission is enabled \r\n");
                }
            }
        }
        else
        {
            printf("Invalid setting value \r\n");
        }

    }
    else
    {
        printf("Invalid parameter name \r\n");
    }

    return CY_RSLT_SUCCESS;
}

/*******************************************************************************
 * Function Name: radar_config_task
 *******************************************************************************
 * Summary:
 *      Parse incoming json string, and set new configuration to radar.
 *
 * Parameters:
 *   pvParameters: thread
 *
 * Return:
 *   none
 ******************************************************************************/
void radar_config_task(void *pvParameters)
{
    cy_rslt_t result;

    char *msg_payload;

    /* Register JSON parser to parse input configuration JSON string */
    cy_JSON_parser_register_callback(json_parser_cb, NULL);

    while (true)
    {
        /* Block till a notification is received from the udp_server_recv_handler */
        if (xQueueReceive(radar_config_queue, &msg_payload, portMAX_DELAY) == pdPASS )
        {
            printf("Command received from udp client %s \r\n", msg_payload);

            /* Get mutex to block any other json parse jobs */
            if (xSemaphoreTake(sem_udp_payload, portMAX_DELAY) == pdTRUE)
            {

                result = cy_JSON_parser(msg_payload, strlen(msg_payload));
                if (result != CY_RSLT_SUCCESS)
                {
                    printf("Json parser error: invalid json message!\r\n");
                }
            }
            xSemaphoreGive(sem_udp_payload);

        }
    }
}

/* [] END OF FILE */
