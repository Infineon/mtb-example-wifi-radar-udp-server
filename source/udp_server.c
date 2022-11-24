/******************************************************************************
* File Name:   udp_server.c
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

/* Header file includes */
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include <inttypes.h>

#include "rtos_artifacts.h"

/* Wi-Fi connection manager header files */
#include "cy_wcm.h"
#include "cy_wcm_error.h"

/* UDP server task header file. */
#include "udp_server.h"
#include "radar_task.h"

#include "wifi_config.h"

/*******************************************************************************
* Macros
********************************************************************************/
/* RTOS related macros for UDP server task. */
#define RTOS_TASK_TICKS_TO_WAIT                   (1000)

#define TASK_QUEUE_LENGTH     (3u)
/*******************************************************************************
* Function Prototypes
********************************************************************************/
static cy_rslt_t connect_to_wifi_ap(void);
static cy_rslt_t create_udp_server_socket(void);
static cy_rslt_t udp_server_recv_handler(cy_socket_t socket_handle, void *arg);

/*******************************************************************************
* Global Variables
********************************************************************************/
/* Secure socket variables. */
cy_socket_sockaddr_t udp_server_addr, peer_addr;
cy_socket_t server_radar_data;

/* Semaphore used to protect message payload */
SemaphoreHandle_t sem_udp_payload = NULL;

char udp_msg_payload[MAX_UDP_RECV_BUFFER_SIZE];

char *sub_msg = udp_msg_payload;

/* Handle of the queue holding the commands for the subscriber task */
QueueHandle_t radar_data_queue;
QueueHandle_t radar_config_queue;
/*******************************************************************************
 * Function Name: udp_server_task
 *******************************************************************************
 * Summary:
 *  Task used to establish a connection to a remote UDP client.
 *
 * Parameters:
 *  void *args : Task parameter defined during task creation (unused)
 *
 * Return:
 *  void
 *
 *******************************************************************************/
void udp_server_task(void *arg)
{
    cy_rslt_t result;

    publisher_data_t *msg;

    /* Variable to store number of bytes sent over UDP socket. */
    uint32_t bytes_sent = 0;

    /* Initialize semaphore to protect payload */
    sem_udp_payload = xSemaphoreCreateMutex();
    if (sem_udp_payload == NULL)
    {
        printf(" 'sem_udp_payload' semaphore creation failed... Task suspend\n\n");
        CY_ASSERT(0);
    }

    /* Connect to Wi-Fi AP */
    if(connect_to_wifi_ap() != CY_RSLT_SUCCESS )
    {
        printf("\n Failed to connect to Wi-Fi AP.\n");
        CY_ASSERT(0);
    }

    /* Secure Sockets initialization */
    result = cy_socket_init();
    if (result != CY_RSLT_SUCCESS)
    {
        printf("Secure Sockets initialization failed!\n");
        CY_ASSERT(0);
    }

    /* Create UDP Server*/
    result = create_udp_server_socket();
    if (result != CY_RSLT_SUCCESS)
    {
        printf("UDP Server Socket creation failed. Error: %"PRIu32"\n", result);
        CY_ASSERT(0);
    }

    /* Create a message queue to communicate with other tasks and callbacks. */
    radar_data_queue = xQueueCreate(TASK_QUEUE_LENGTH, sizeof(publisher_data_t * ));
    if (radar_data_queue == NULL)
    {
        printf(" 'udp_server_task' queue creation failed... Task suspended\n\n");
        CY_ASSERT(0);
    }

    radar_config_queue = xQueueCreate(TASK_QUEUE_LENGTH, sizeof(sub_msg));
    if (radar_config_queue == NULL)
    {
        printf(" 'radar_config_queue' queue creation failed... Task suspended\n\n");
        CY_ASSERT(0);
    }


    if(pdPASS !=  xTaskCreate(radar_task, "radar_task", configMINIMAL_STACK_SIZE * 8, NULL, 7, &radar_task_handle))
    {
        printf("Failed to create radar data task!\n");
        CY_ASSERT(0);
    }

    while(true)
    {

        if (pdTRUE ==  xQueueReceive( radar_data_queue, &msg, ( TickType_t ) portMAX_DELAY ))
        {
            switch(msg->cmd)
            {
                case RADAR_DATA_COMMAND:
                {

                    result = cy_socket_sendto(server_radar_data, msg->data, msg->length, CY_SOCKET_FLAGS_NONE,
                                             &peer_addr, sizeof(cy_socket_sockaddr_t), &bytes_sent);
                    if(result == CY_RSLT_SUCCESS )
                    {
                        printf("Data with length:%" PRIu32 " sent to udp client\n", bytes_sent);
                    }
                    else
                    {
                        printf("Failed to send data to client. Error: %"PRIu32"\n", result);
                    }
                    break;
                }
            }
        }
      }
 }

/*******************************************************************************
 * Function Name: connect_to_wifi_ap()
 *******************************************************************************
 * Summary:
 *  Connects to Wi-Fi AP using the user-configured credentials, retries up to a
 *  configured number of times until the connection succeeds.
 *
 * Return:
 *   error
 *******************************************************************************/
cy_rslt_t connect_to_wifi_ap(void)
{
    cy_rslt_t result;

    /* Variables used by Wi-Fi connection manager. */
    cy_wcm_connect_params_t wifi_conn_param;

    cy_wcm_config_t wifi_config = {
            .interface = CY_WCM_INTERFACE_TYPE_STA
    };

    cy_wcm_ip_address_t ip_address;

    /* Variable to track the number of connection retries to the Wi-Fi AP specified
     * by WIFI_SSID macro */
    int conn_retries = 0;

    /* Initialize Wi-Fi connection manager. */
    result = cy_wcm_init(&wifi_config);

    if (result != CY_RSLT_SUCCESS)
    {
        printf("Wi-Fi Connection Manager initialization failed!\n");
        return result;
    }
    printf("Wi-Fi Connection Manager initialized. \n");

    /* Set the Wi-Fi SSID, password and security type. */
    memset(&wifi_conn_param, 0, sizeof(cy_wcm_connect_params_t));
    memcpy(wifi_conn_param.ap_credentials.SSID, WIFI_SSID, sizeof(WIFI_SSID));
    memcpy(wifi_conn_param.ap_credentials.password, WIFI_PASSWORD, sizeof(WIFI_PASSWORD));
    wifi_conn_param.ap_credentials.security = WIFI_SECURITY;

    /* Join the Wi-Fi AP. */
    for(conn_retries = 0; conn_retries < MAX_WIFI_CONN_RETRIES; conn_retries++ )
    {
        result = cy_wcm_connect_ap(&wifi_conn_param, &ip_address);

        if(result == CY_RSLT_SUCCESS)
        {
            printf("Successfully connected to Wi-Fi network '%s'.\n",
                    wifi_conn_param.ap_credentials.SSID);
            printf("IP Address Assigned: %d.%d.%d.%d\n", (uint8)ip_address.ip.v4,
                    (uint8)(ip_address.ip.v4 >> 8), (uint8)(ip_address.ip.v4 >> 16),
                    (uint8)(ip_address.ip.v4 >> 24));

            /* IP address and UDP port number of the UDP server */
            udp_server_addr.ip_address.ip.v4 = ip_address.ip.v4;
            udp_server_addr.ip_address.version = CY_SOCKET_IP_VER_V4;
            udp_server_addr.port = UDP_SERVER_PORT;
            return result;
        }

        printf("Connection to Wi-Fi network failed with error code %d."
                "Retrying in %d ms...\n", (int)result, WIFI_CONN_RETRY_INTERVAL_MS);
        vTaskDelay(pdMS_TO_TICKS(WIFI_CONN_RETRY_INTERVAL_MS));
    }

    /* Stop retrying after maximum retry attempts. */
    printf("Exceeded maximum Wi-Fi connection attempts\n");

    return result;
}

/*******************************************************************************
 * Function Name: create_udp_server_socket
 *******************************************************************************
 * Summary:
 *  Function to create a socket and set the socket options.
 *
 *  Return:
 *   error
 *
 *******************************************************************************/
cy_rslt_t create_udp_server_socket(void)
{
    cy_rslt_t result;

    /* Variable used to set socket options. */
    cy_socket_opt_callback_t udp_recv_option = {
            .callback = udp_server_recv_handler,
            .arg = NULL
    };

    /* Create a UDP server socket. */
    result = cy_socket_create(CY_SOCKET_DOMAIN_AF_INET, CY_SOCKET_TYPE_DGRAM, CY_SOCKET_IPPROTO_UDP, &server_radar_data);
    if (result != CY_RSLT_SUCCESS)
    {
        return result;
    }

    /* Register the callback function to handle messages received from UDP client. */
    result = cy_socket_setsockopt(server_radar_data, CY_SOCKET_SOL_SOCKET,
            CY_SOCKET_SO_RECEIVE_CALLBACK,
            &udp_recv_option, sizeof(cy_socket_opt_callback_t));
    if (result != CY_RSLT_SUCCESS)
    {
        return result;
    }

    /* Bind the UDP socket created to Server IP address and port. */
    result = cy_socket_bind(server_radar_data, &udp_server_addr, sizeof(udp_server_addr));
    if (result == CY_RSLT_SUCCESS)
    {
         printf("Socket bound to port: %d\n", udp_server_addr.port);
    }

    return result;
}

/*******************************************************************************
 * Function Name: udp_server_recv_handler
 *******************************************************************************
 * Summary:
 *  Callback function to handle incoming  message from UDP client.
 *
 *  Parameters:
 *  socket_handle : pointer to parameters passed to callback
 *  arg : pointer to socket to read data from
 *
 *  Return:
 *   error
 *
 *******************************************************************************/
cy_rslt_t udp_server_recv_handler(cy_socket_t socket_handle, void *arg)
{
    cy_rslt_t result;

    /* Variable to store the number of bytes received. */
    uint32_t bytes_received = 0;

    if (xSemaphoreTake(sem_udp_payload, portMAX_DELAY) == pdTRUE)
    {
        /* Receive incoming message from UDP server. */
        result = cy_socket_recvfrom(server_radar_data, udp_msg_payload, MAX_UDP_RECV_BUFFER_SIZE,
                                    CY_SOCKET_FLAGS_NONE, &peer_addr, NULL,
                                    &bytes_received);
        printf("message received %s\n", udp_msg_payload);

        udp_msg_payload[bytes_received] = '\0';

        xSemaphoreGive(sem_udp_payload);

        if(result == CY_RSLT_SUCCESS)
        {
            xQueueSend( radar_config_queue, ( void * ) &sub_msg, ( TickType_t ) 0 );

        }
        else
        {
            printf("Failed to receive message from client. Error: %"PRIu32"\n", result);
        }
    }
    else
    {
        result = 0;
    }

    return result;
}


/* [] END OF FILE */

