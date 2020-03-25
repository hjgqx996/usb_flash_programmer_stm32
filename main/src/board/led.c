/*
 * led.c
 *
 *  Created on: 2020-03-24 22:08
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "stm32f1xx_hal.h"

#include "board/led.h"

void led1_init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {
        .Pin = LED1_GPIO_PIN,
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_PULLUP,
        .Speed = GPIO_SPEED_FREQ_HIGH,
    };
    HAL_GPIO_Init(LED1_GPIO_PORT, &GPIO_InitStruct);
}
