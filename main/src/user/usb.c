/*
 * usb.c
 *
 *  Created on: 2020-03-24 20:52
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "stm32f1xx_hal.h"

#include "FreeRTOS.h"
#include "task.h"

#include "tusb.h"

#include "core/os.h"

#define TAG "usb"

void usb_task(void *pvParameter)
{
    (void)pvParameter;

    OS_LOGI(TAG, "started.");

    tusb_init();

    while (1) {
        tud_task();
    }
}

void usb_init(void)
{
    xTaskCreate(usb_task, "usbT", 150, NULL, configMAX_PRIORITIES - 2, NULL);
}
