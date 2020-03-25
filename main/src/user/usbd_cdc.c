/*
 * usbd_cdc.c
 *
 *  Created on: 2020-03-24 20:57
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "stm32f1xx_hal.h"

#include "FreeRTOS.h"
#include "task.h"

#include "tusb.h"

#include "user/mtd.h"

#define CDC_STACK_SIZE 256

static StackType_t stack_cdc[CDC_STACK_SIZE];
static StaticTask_t static_task_cdc;

// invoked when cdc when line state changed e.g connected / disconnected
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    (void)itf;

    if (dtr && rts) {
        tud_cdc_read_flush();
        tud_cdc_write_flush();
    }

    if (!dtr && !rts) {
        mtd_end();

        tud_cdc_read_flush();
        tud_cdc_write_flush();
    }
}

// invoked when CDC interface received data from host
void tud_cdc_rx_cb(uint8_t itf)
{
    (void)itf;
}

void usbd_cdc_task(void *pvParameter)
{
    (void)pvParameter;
    uint8_t data[64] = {0};
    uint32_t size = 0;
    uint16_t count = 0;

    while (1) {
        if (tud_cdc_connected()) {
            if ((size = tud_cdc_available()) != 0) {
                if ((size != 64) && (++count < 200)) {
                    vTaskDelay(1 / portTICK_RATE_MS);

                    continue;
                }

                count = 0;

                tud_cdc_read(data, size);
                tud_cdc_read_flush();

                mtd_exec(data, size);
            }
        }

        taskYIELD();
    }
}

void usbd_cdc_init(void)
{
    xTaskCreateStatic(usbd_cdc_task, "usbdCdcT", CDC_STACK_SIZE, NULL, configMAX_PRIORITIES - 2, stack_cdc, &static_task_cdc);
}
