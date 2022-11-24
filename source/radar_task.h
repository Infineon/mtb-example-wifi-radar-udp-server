/******************************************************************************
 * File Name:   radar_task.h
 *
 * Description: This file contains the function prototypes and constants used
 *   in radar_task.c.
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

#ifndef RADAR_TASK_H_
#define RADAR_TASK_H_

/*******************************************************************************
 * Macros
 ******************************************************************************/
#define RADAR_TASK_NAME       "RADAR PRESENCE TASK"
#define RADAR_TASK_STACK_SIZE (1024 * 4)
#define RADAR_TASK_PRIORITY   (3)

#define RESULT_SUCCESS  (0)
#define RESULT_ERROR    (-1)

#define RADAR_DATA_COMMAND  (1)
#define DUMMY_BYTE          (0xFF)

/*******************************************************************************
 * Functions
 ******************************************************************************/
void radar_task(void *pvParameters);
int32_t radar_start(bool start);
int32_t radar_enable_test_mode(bool start);

#endif /* RADAR_TASK_H_ */
/* [] END OF FILE */
