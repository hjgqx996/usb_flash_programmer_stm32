/*
 * led.c
 *
 *  Created on: 2018-02-13 15:43
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include "stm32f1xx_hal.h"

#include "FreeRTOS.h"
#include "task.h"

#include "core/os.h"
#include "board/led.h"

#define TAG "led"

#define LED_STACK_SIZE 128

static StackType_t stack_led[LED_STACK_SIZE];
static StaticTask_t static_task_led;

static const uint16_t led_mode_table[][2] = {
/*  { delay, count}  */
    {     0,     2},   // 0, Keep off
    {  1000,     2},   // 1,
    {   500,     2},   // 2,
    {   250,     2},   // 3,
    {   100,     2},   // 4,
    {    25,     2},   // 5,
    {    25,    25},   // 6,
    {    25,    50},   // 7,
    {    25,   100},   // 8,
    {    25,     0}    // 9, Keep on
};

static uint8_t led_mode_index = 3;

static void led_task(void *pvParameter)
{
    (void)pvParameter;

    uint16_t i = 0;
    portTickType xLastWakeTime;

    OS_LOGI(TAG, "started.");

    while (1) {
        xLastWakeTime = xTaskGetTickCount();

        if (i++ % led_mode_table[led_mode_index][1]) {
            HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_GPIO_PIN, GPIO_PIN_RESET);
        } else {
            HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_GPIO_PIN, GPIO_PIN_SET);
        }

        vTaskDelayUntil(&xLastWakeTime, led_mode_table[led_mode_index][0] / portTICK_RATE_MS);
    }
}

void led_set_mode(uint8_t idx)
{
    if (idx >= sizeof(led_mode_table)/2) {
        return;
    }
    led_mode_index = idx;
}

void led_init(void)
{
    xTaskCreateStatic(led_task, "ledT", LED_STACK_SIZE, NULL, configMAX_PRIORITIES - 3, stack_led, &static_task_led);
}
