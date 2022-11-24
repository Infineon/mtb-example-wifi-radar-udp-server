/******************************************************************************
* File Name:   udp_server.h
*
* Description: This file contains declaration of task and functions related to
*              UDP server operation.
*
********************************************************************************
* Copyright 2020-2022, Cypress Semiconductor Corporation (an Infineon company) or
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

#ifndef UDP_SERVER_H_
#define UDP_SERVER_H_

/* Cypress secure socket header file */
#include "cy_secure_sockets.h"

/*******************************************************************************
* Macros
********************************************************************************/

/* UDP server related macros. */
#define UDP_SERVER_PORT                           (57345)
#define UDP_SERVER_MAX_PENDING_CONNECTIONS        (3)
#define UDP_SERVER_RECV_TIMEOUT_MS                (500)


/* RTOS related macros for UDP server task. */
#define UDP_SERVER_TASK_STACK_SIZE                (8 * 1024)
#define UDP_SERVER_TASK_PRIORITY                  (1)

/* Buffer size to store the incoming messages from server, in bytes. */
#define MAX_UDP_RECV_BUFFER_SIZE                  (64)

/* Struct to be passed via the publisher task queue */
typedef struct{
    uint8_t cmd;
    uint32_t length;
    uint8_t *data;
} publisher_data_t;

/*******************************************************************************
* Function Prototypes
********************************************************************************/
void udp_server_task(void *arg);

#endif /* UDP_SERVER_H_ */

/* [] END OF FILE */
