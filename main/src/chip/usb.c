/*
 * usb.c
 *
 *  Created on: 2020-03-24 20:36
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "stm32f1xx_hal.h"

void usb_init(void)
{
    __HAL_RCC_USB_CLK_ENABLE();

    RCC_PeriphCLKInitTypeDef PeriphClkInit = {
        .PeriphClockSelection = RCC_PERIPHCLK_USB,
        .UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5,
    };
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {
        .Pin = (GPIO_PIN_11 | GPIO_PIN_12),
        .Mode = GPIO_MODE_INPUT,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_HIGH,
    };
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}
