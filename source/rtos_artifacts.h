/******************************************************************************
 * File Name:   radar_artifacts.h
 *
 * Description: This file contains the declaration of services used in the
 * application from FreeRTOS i.e. task, queues, and semaphores
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

#ifndef SOURCE_RTOS_ARTIFACTS_H_
#define SOURCE_RTOS_ARTIFACTS_H_



/* Header file includes */
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "queue.h"


/*******************************************************************************
 * Macros
 ******************************************************************************/


/*******************************************************************************
 * Global Variables
 ******************************************************************************/

/* FreeRTOS task handle for radar task which performs radar configurations,
 * data acquisition, and forwards the frame data to udp server queue */
extern TaskHandle_t radar_task_handle;

/* FreeRTOS task handle for radar configuration task which receives messages
 * in configuration queue, parse them, and updates radar operation */

extern TaskHandle_t radar_config_task_handle;

/* FreeRTOS queue handle to forward radar data */
extern QueueHandle_t radar_data_queue;

/* FreeRTOS queue handle for udp server callback queue used for radar mode */
extern QueueHandle_t radar_config_queue;

/* FreeRTOS semaphore handle to update/consume radar mode data form udp client*/
extern SemaphoreHandle_t sem_udp_payload;

#endif /* SOURCE_RTOS_ARTIFACTS_H_ */
