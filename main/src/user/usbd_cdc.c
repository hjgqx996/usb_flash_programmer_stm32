/*
 * usbd_cdc.c
 *
 *  Created on: 2020-03-24 20:57
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "stm32f1xx_hal.h"

#include "FreeRTOS.h"
#include "event_groups.h"
#include "task.h"

#include "tusb.h"

#include "core/os.h"
#include "user/mtd.h"

#define TAG "usbd_cdc"

#define CDC_STACK_SIZE 512

static StackType_t stack_cdc[CDC_STACK_SIZE];
static StaticTask_t static_task_cdc;

static uint8_t recv_buff[64] = {0};
static uint32_t recv_size = 0;

void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    (void)itf;

    if (dtr && rts) {
        OS_LOGI(TAG, "connected.");
    }

    if (!dtr && !rts) {
        OS_LOGI(TAG, "disconnected.");

        mtd_end();

        xEventGroupClearBits(user_event_group, USB_CDC_DATA_BIT);
    }
}

void tud_cdc_rx_wanted_cb(uint8_t itf, char wanted_char)
{
    (void)itf;

    EventBits_t uxBits = xEventGroupGetBits(user_event_group);
    if (!(uxBits & USB_CDC_DATA_BIT)) {
        switch (wanted_char) {
        case 'M':
            tud_cdc_set_wanted_char('\n');
            break;
        case '\n':
            recv_size = tud_cdc_available();

            tud_cdc_set_wanted_char('M');

            xEventGroupSetBits(user_event_group, USB_CDC_CMD_BIT);
            break;
        default:
            break;
        }
    }
}

void usbd_cdc_task(void *pvParameter)
{
    (void)pvParameter;

    OS_LOGI(TAG, "started.");

    tud_cdc_set_wanted_char('M');

    while (1) {
        xEventGroupWaitBits(
            user_event_group,
            USB_CDC_CMD_BIT | USB_CDC_DATA_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY
        );

        EventBits_t uxBits = xEventGroupGetBits(user_event_group);
        if (uxBits & USB_CDC_CMD_BIT) {
            xEventGroupClearBits(user_event_group, USB_CDC_CMD_BIT);

            tud_cdc_read(recv_buff, recv_size);

            mtd_exec(recv_buff, recv_size);

            continue;
        }

        if ((recv_size = tud_cdc_available()) != 0) {
            tud_cdc_read(recv_buff, recv_size);

            mtd_exec(recv_buff, recv_size);
        }

        taskYIELD();
    }
}

void usbd_cdc_init(void)
{
    xTaskCreateStatic(usbd_cdc_task, "usbdCdcT", CDC_STACK_SIZE, NULL, configMAX_PRIORITIES - 3, stack_cdc, &static_task_cdc);
}
