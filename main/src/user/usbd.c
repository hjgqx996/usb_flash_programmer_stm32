/*
 * usbd.c
 *
 *  Created on: 2020-03-24 20:52
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "stm32f1xx_hal.h"

#include "FreeRTOS.h"
#include "task.h"

#include "tusb.h"

#define USBD_STACK_SIZE 150

static StackType_t stack_usbd[USBD_STACK_SIZE];
static StaticTask_t static_task_usbd;

void usbd_task(void *pvParameter)
{
    (void)pvParameter;

    while (1) {
        tud_task();
    }
}

void usbd_init(void)
{
    tusb_init();

    xTaskCreateStatic(usbd_task, "usbdT", USBD_STACK_SIZE, NULL, configMAX_PRIORITIES - 2, stack_usbd, &static_task_usbd);
}
