/*
 * flash.h
 *
 *  Created on: 2020-03-24 22:29
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_BOARD_FLASH_H_
#define INC_BOARD_FLASH_H_

#define FLASH_CS_PORT    GPIOA
#define FLASH_CS_PIN     GPIO_PIN_4

extern void flash_init(void);

#endif /* INC_BOARD_FLASH_H_ */
