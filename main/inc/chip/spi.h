/*
 * spi.h
 *
 *  Created on: 2020-03-24 20:44
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_CHIP_SPI_H_
#define INC_CHIP_SPI_H_

#include "stm32f1xx_hal_conf.h"

extern SPI_HandleTypeDef hspi1;

extern void hspi1_init(void);

#endif /* INC_CHIP_SPI_H_ */
