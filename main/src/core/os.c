/*
 * os.c
 *
 *  Created on: 2020-03-24 21:00
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <stdio.h>

#include "stm32f1xx_hal.h"

#include "FreeRTOS.h"
#include "event_groups.h"
#include "task.h"

#include "core/os.h"
#include "chip/uart.h"

#define TAG "os"

static StaticEventGroup_t static_event_group;

EventGroupHandle_t user_event_group;

int __attribute__((used)) _write(int file, char *ptr, int len)
{
    (void)file;

    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);

	return len;
}

void _init(void) {}

void os_start(void)
{
    OS_LOGI(TAG, "Starting scheduler...");

    vTaskStartScheduler();
}

void os_init(void)
{
    /* Configure Flash prefetch */
    __HAL_FLASH_PREFETCH_BUFFER_ENABLE();

    /* Set Interrupt Group Priority */
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

    /* Use systick as time base source and configure 1ms tick (default clock after Reset is HSI) */
    HAL_InitTick(TICK_INT_PRIORITY);

    /* PendSV_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0);

    user_event_group = xEventGroupCreateStatic(&static_event_group);
}
