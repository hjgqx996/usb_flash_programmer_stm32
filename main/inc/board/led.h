/*
 * led.h
 *
 *  Created on: 2020-03-24 22:08
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_BOARD_LED_H_
#define INC_BOARD_LED_H_

#define LED1_GPIO_PORT    GPIOC
#define LED1_GPIO_PIN     GPIO_PIN_13

extern void led1_init(void);

#endif /* INC_BOARD_LED_H_ */
