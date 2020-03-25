/*
 * app_main.c
 *
 *  Created on: 2020-03-24 18:33
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "core/os.h"

#include "chip/rcc.h"
#include "chip/usb.h"
#include "chip/spi.h"
#include "chip/uart.h"

#include "board/led.h"
#include "board/flash.h"

#include "user/led.h"
#include "user/usbd.h"
#include "user/usbd_cdc.h"

static void core_init(void)
{
    os_init();
}

static void chip_init(void)
{
    rcc_init();

    usb_init();

    hspi1_init();

    huart1_init();
}

static void board_init(void)
{
    led1_init();

    flash_init();
}

static void user_init(void)
{
    led_init();

    usbd_init();

    usbd_cdc_init();

    os_start();
}

int main(void)
{
    core_init();

    chip_init();

    board_init();

    user_init();

    return 0;
}
