/*
 * tusb_config.h
 *
 *  Created on: 2020-03-24 18:33
 *      Author: Jack Chen <redchenjs@live.com>
 */

#ifndef INC_TUSB_CONFIG_H_
#define INC_TUSB_CONFIG_H_

/* COMMON CONFIGURATION */

// defined by compiler flags for flexibility
#ifndef CFG_TUSB_MCU
    #error CFG_TUSB_MCU must be defined
#endif

#define CFG_TUSB_RHPORT0_MODE       OPT_MODE_DEVICE
#define CFG_TUSB_OS                 OPT_OS_FREERTOS

#ifndef CFG_TUSB_MEM_SECTION
    #define CFG_TUSB_MEM_SECTION
#endif

#ifndef CFG_TUSB_MEM_ALIGN
    #define CFG_TUSB_MEM_ALIGN      __attribute__ ((aligned(4)))
#endif

/* DEVICE CONFIGURATION */

#define CFG_TUD_ENDPOINT0_SIZE  64

// Class
#define CFG_TUD_CDC         1

// CDC FIFO size of TX and RX
#define CFG_TUD_CDC_RX_BUFSIZE  64
#define CFG_TUD_CDC_TX_BUFSIZE  64

#endif /* INC_TUSB_CONFIG_H_ */
