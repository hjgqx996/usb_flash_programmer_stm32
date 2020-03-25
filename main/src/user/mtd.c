/*
 * mtd.c
 *
 *  Created on: 2020-02-12 15:48
 *      Author: Jack Chen <redchenjs@live.com>
 */

#include <string.h>

#include "stm32f1xx_hal.h"

#include "FreeRTOS.h"
#include "task.h"
#include "ringbuf.h"

#include "sfud.h"
#include "tusb.h"

#include "core/os.h"

#define MTD_TAG "mtd"

#define mtd_send_response(X) \
    do { \
        tud_cdc_write((void const *)rsp_str[X], strlen(rsp_str[X])); \
        tud_cdc_write_flush(); \
    } while (0)

#define mtd_send_data(X, N) \
    do { \
        tud_cdc_write((void const *)X, (uint32_t)N); \
        tud_cdc_write_flush(); \
    } while (0)

#define CMD_FMT_ERASE_ALL   "MTD+ERASE:ALL!"
#define CMD_FMT_ERASE       "MTD+ERASE:0x%x+0x%x"
#define CMD_FMT_WRITE       "MTD+WRITE:0x%x+0x%x"
#define CMD_FMT_READ        "MTD+READ:0x%x+0x%x"
#define CMD_FMT_INFO        "MTD+INFO?"

enum cmd_idx {
    CMD_IDX_ERASE_ALL = 0x0,
    CMD_IDX_ERASE     = 0x1,
    CMD_IDX_WRITE     = 0x2,
    CMD_IDX_READ      = 0x3,
    CMD_IDX_INFO      = 0x4,
};

typedef struct {
    const char prefix;
    const char format[32];
} cmd_fmt_t;

static const cmd_fmt_t cmd_fmt[] = {
    { .prefix = 16, .format = CMD_FMT_ERASE_ALL"\r\n" },   // Erase Full Flash Chip
    { .prefix = 12, .format = CMD_FMT_ERASE"\r\n" },       // Erase Flash: Addr Length
    { .prefix = 12, .format = CMD_FMT_WRITE"\r\n" },       // Write Flash: Addr Length
    { .prefix = 11, .format = CMD_FMT_READ"\r\n" },        // Read Flash:  Addr Length
    { .prefix = 11, .format = CMD_FMT_INFO"\r\n" },        // Flash Info
};

enum rsp_idx {
    RSP_IDX_OK    = 0x0,
    RSP_IDX_FAIL  = 0x1,
    RSP_IDX_DONE  = 0x2,
    RSP_IDX_ERROR = 0x3,
};

static const char rsp_str[][32] = {
    "OK\r\n",           // OK
    "FAIL\r\n",         // Fail
    "DONE\r\n",         // Done
    "ERROR\r\n",        // Error
};

static bool data_err = false;
static bool data_recv = false;
static bool conn_state = false;

static uint8_t buff_data[64] = {0};
static StaticRingbuffer_t buff_struct = {0};

static RingbufHandle_t mtd_buff = NULL;

#define WRITE_STACK_SIZE 1024
static StackType_t write_task_stack[WRITE_STACK_SIZE] = {0};
static StaticTask_t write_task_struct = {0};

#define READ_STACK_SIZE 1024
static StackType_t read_task_stack[READ_STACK_SIZE] = {0};
static StaticTask_t read_task_struct = {0};

static uint32_t data_addr = 0;
static uint32_t addr = 0, length = 0;
static sfud_flash *flash = NULL;

static int mtd_parse_command(uint8_t *data)
{
    for (unsigned int i=0; i<sizeof(cmd_fmt)/sizeof(cmd_fmt_t); i++) {
        if (strncmp(cmd_fmt[i].format, (const char *)data, cmd_fmt[i].prefix) == 0) {
            return i;
        }
    }
    return -1;
}

static void mtd_write_task(void *pvParameter)
{
    (void)pvParameter;

    uint8_t *data = NULL;
    uint32_t size = 0;
    sfud_err err = SFUD_SUCCESS;

    OS_LOGI(MTD_TAG, "write started.");

    while ((data_addr - addr) != length) {
        if (!conn_state || !data_recv) {
            OS_LOGE(MTD_TAG, "write aborted.");

            goto write_fail;
        }

        uint32_t remain = length - (data_addr - addr);

        if (remain >= 64) {
            data = (uint8_t *)xRingbufferReceiveUpTo(mtd_buff, (size_t *)&size, 1 / portTICK_RATE_MS, 64);
        } else {
            data = (uint8_t *)xRingbufferReceiveUpTo(mtd_buff, (size_t *)&size, 1 / portTICK_RATE_MS, remain);
        }

        if (data == NULL) {
            continue;
        }

        err = sfud_write(flash, data_addr, size, (const uint8_t *)data);

        data_addr += size;

        if (err != SFUD_SUCCESS) {
            OS_LOGE(MTD_TAG, "write failed.");

            data_err = true;

            mtd_send_response(RSP_IDX_FAIL);

            goto write_fail;
        }

        vRingbufferReturnItem(mtd_buff, (void *)data);
    }

    OS_LOGI(MTD_TAG, "write done.");

    mtd_send_response(RSP_IDX_DONE);

write_fail:
    vRingbufferDelete(mtd_buff);
    mtd_buff = NULL;

    data_recv = false;
    conn_state = false;

    vTaskDelete(NULL);
}

static void mtd_read_task(void *pvParameter)
{
    (void)pvParameter;

    sfud_err err = SFUD_SUCCESS;

    OS_LOGI(MTD_TAG, "read started.");

    uint32_t pkt = 0;
    for (pkt=0; pkt<length/64; pkt++) {
        err = sfud_read(flash, data_addr, 64, buff_data);

        data_addr += 64;

        if (err != SFUD_SUCCESS) {
            OS_LOGE(MTD_TAG, "read failed.");

            mtd_send_response(RSP_IDX_FAIL);

            goto read_fail;
        }

        while (tud_cdc_write_flush() == 0) {
            taskYIELD();

            if (!conn_state) {
                OS_LOGE(MTD_TAG, "read aborted.");

                goto read_fail;
            }
        }

        if (!conn_state) {
            OS_LOGE(MTD_TAG, "read aborted.");

            goto read_fail;
        }

        mtd_send_data(buff_data, 64);
    }

    uint32_t data_remain = length - pkt * 64;
    if (data_remain != 0 && data_remain < 64) {
        err = sfud_read(flash, data_addr, data_remain, buff_data);

        data_addr += data_remain;

        if (err != SFUD_SUCCESS) {
            OS_LOGE(MTD_TAG, "read failed.");

            mtd_send_response(RSP_IDX_FAIL);

            goto read_fail;
        }

        while (tud_cdc_write_flush() == 0) {
            taskYIELD();

            if (!conn_state) {
                OS_LOGE(MTD_TAG, "read aborted.");

                goto read_fail;
            }
        }

        if (!conn_state) {
            OS_LOGE(MTD_TAG, "read aborted.");

            goto read_fail;
        }

        mtd_send_data(buff_data, data_remain);
    }

    OS_LOGI(MTD_TAG, "read done.");

read_fail:
    conn_state = false;

    vTaskDelete(NULL);
}

void mtd_exec(uint8_t *data, uint32_t len)
{
    if (data_err) {
        return;
    }

    if (!data_recv) {
        int cmd_idx = mtd_parse_command(data);

        if (flash) {
            memset(&flash->chip, 0x00, sizeof(sfud_flash_chip));
        }

        conn_state = true;

        switch (cmd_idx) {
            case CMD_IDX_ERASE_ALL: {
                OS_LOGI(MTD_TAG, "GET command: "CMD_FMT_ERASE_ALL);

                sfud_err err = sfud_init();
                if (err == SFUD_ERR_NOT_FOUND) {
                    OS_LOGE(MTD_TAG, "target flash not found or not supported.");

                    mtd_send_response(RSP_IDX_FAIL);
                } else if (err != SFUD_SUCCESS) {
                    OS_LOGE(MTD_TAG, "failed to init target flash.");

                    mtd_send_response(RSP_IDX_FAIL);
                } else {
                    flash = sfud_get_device(SFUD_TARGET_DEVICE_INDEX);

                    OS_LOGI(MTD_TAG, "chip erase started.");

                    err = sfud_chip_erase(flash);

                    if (err != SFUD_SUCCESS) {
                        OS_LOGE(MTD_TAG, "chip erase failed.");

                        mtd_send_response(RSP_IDX_FAIL);
                    } else {
                        OS_LOGI(MTD_TAG, "chip erase done.");

                        mtd_send_response(RSP_IDX_DONE);
                    }
                }

                break;
            }
            case CMD_IDX_ERASE: {
                addr = length = 0;
                sscanf((const char *)data, CMD_FMT_ERASE, (unsigned int *)&addr, (unsigned int *)&length);
                OS_LOGI(MTD_TAG, "GET command: "CMD_FMT_ERASE, (unsigned int)addr, (unsigned int)length);

                if (length != 0) {
                    sfud_err err = sfud_init();
                    if (err == SFUD_ERR_NOT_FOUND) {
                        OS_LOGE(MTD_TAG, "target flash not found or not supported.");

                        mtd_send_response(RSP_IDX_FAIL);
                    } else if (err != SFUD_SUCCESS) {
                        OS_LOGE(MTD_TAG, "failed to init target flash.");

                        mtd_send_response(RSP_IDX_FAIL);
                    } else {
                        flash = sfud_get_device(SFUD_TARGET_DEVICE_INDEX);

                        OS_LOGI(MTD_TAG, "erase started.");

                        err = sfud_erase(flash, addr, length);

                        if (err != SFUD_SUCCESS) {
                            OS_LOGE(MTD_TAG, "erase failed.");

                            mtd_send_response(RSP_IDX_FAIL);
                        } else {
                            OS_LOGI(MTD_TAG, "erase done.");

                            mtd_send_response(RSP_IDX_DONE);
                        }
                    }
                } else {
                    mtd_send_response(RSP_IDX_ERROR);
                }

                break;
            }
            case CMD_IDX_WRITE: {
                addr = length = 0;
                sscanf((const char *)data, CMD_FMT_WRITE, (unsigned int *)&addr, (unsigned int *)&length);
                OS_LOGI(MTD_TAG, "GET command: "CMD_FMT_WRITE, (unsigned int)addr, (unsigned int)length);

                if (length != 0) {
                    data_recv = true;

                    sfud_err err = sfud_init();
                    if (err == SFUD_ERR_NOT_FOUND) {
                        OS_LOGE(MTD_TAG, "target flash not found or not supported.");

                        mtd_send_response(RSP_IDX_FAIL);
                    } else if (err != SFUD_SUCCESS) {
                        OS_LOGE(MTD_TAG, "failed to init target flash.");

                        mtd_send_response(RSP_IDX_FAIL);
                    } else {
                        data_addr = addr;
                        flash = sfud_get_device(SFUD_TARGET_DEVICE_INDEX);

                        memset(&buff_struct, 0x00, sizeof(StaticRingbuffer_t));
                        mtd_buff = xRingbufferCreateStatic(sizeof(buff_data), RINGBUF_TYPE_BYTEBUF, buff_data, &buff_struct);

                        xTaskCreateStatic(mtd_write_task, "mtdWriteT", WRITE_STACK_SIZE, NULL, configMAX_PRIORITIES - 2, write_task_stack, &write_task_struct);

                        mtd_send_response(RSP_IDX_OK);
                    }
                } else {
                    mtd_send_response(RSP_IDX_ERROR);
                }

                break;
            }
            case CMD_IDX_READ: {
                addr = length = 0;
                sscanf((const char *)data, CMD_FMT_READ, (unsigned int *)&addr, (unsigned int *)&length);
                OS_LOGI(MTD_TAG, "GET command: "CMD_FMT_READ, (unsigned int)addr, (unsigned int)length);

                if (length != 0) {
                    sfud_err err = sfud_init();
                    if (err == SFUD_ERR_NOT_FOUND) {
                        OS_LOGE(MTD_TAG, "target flash not found or not supported.");

                        mtd_send_response(RSP_IDX_FAIL);
                    } else if (err != SFUD_SUCCESS) {
                        OS_LOGE(MTD_TAG, "failed to init target flash.");

                        mtd_send_response(RSP_IDX_FAIL);
                    } else {
                        data_addr = addr;
                        flash = sfud_get_device(SFUD_TARGET_DEVICE_INDEX);

                        mtd_send_response(RSP_IDX_OK);

                        xTaskCreateStatic(mtd_read_task, "mtdReadT", READ_STACK_SIZE, NULL, configMAX_PRIORITIES - 2, read_task_stack, &read_task_struct);
                    }
                } else {
                    mtd_send_response(RSP_IDX_ERROR);
                }

                break;
            }
            case CMD_IDX_INFO: {
                OS_LOGI(MTD_TAG, "GET command: "CMD_FMT_INFO);

                sfud_err err = sfud_init();
                if (err == SFUD_ERR_NOT_FOUND) {
                    OS_LOGE(MTD_TAG, "target flash not found or not supported.");

                    mtd_send_response(RSP_IDX_FAIL);
                } else if (err != SFUD_SUCCESS) {
                    OS_LOGE(MTD_TAG, "failed to init target flash.");

                    mtd_send_response(RSP_IDX_FAIL);
                } else {
                    const sfud_mf mf_table[] = SFUD_MF_TABLE;
                    const char *flash_mf_name = NULL;

                    flash = sfud_get_device(SFUD_TARGET_DEVICE_INDEX);

                    for (unsigned int i=0; i<sizeof(mf_table)/sizeof(sfud_mf); i++) {
                        if (mf_table[i].id == flash->chip.mf_id) {
                            flash_mf_name = mf_table[i].name;
                            break;
                        }
                    }

                    char str_buf[40] = {0};
                    if (flash_mf_name && flash->chip.name) {
                        snprintf(str_buf, sizeof(str_buf), "%s,%s,%lu\r\n", flash_mf_name, flash->chip.name, flash->chip.capacity);
                        OS_LOGI(MTD_TAG, "manufactor: %s, chip name: %s, capacity: %lu bytes", (char *)flash_mf_name, (char *)flash->chip.name, flash->chip.capacity);
                    } else if (flash_mf_name) {
                        snprintf(str_buf, sizeof(str_buf), "%s,%lu\r\n", flash_mf_name, flash->chip.capacity);
                        OS_LOGI(MTD_TAG, "manufactor: %s, capacity: %lu bytes", (char *)flash_mf_name, flash->chip.capacity);
                    } else {
                        snprintf(str_buf, sizeof(str_buf), "%lu\r\n", flash->chip.capacity);
                        OS_LOGI(MTD_TAG, "capacity: %lu bytes", flash->chip.capacity);
                    }

                    mtd_send_data(str_buf, strlen(str_buf));
                }

                break;
            }
            default:
                OS_LOGW(MTD_TAG, "unknown command.");

                mtd_send_response(RSP_IDX_ERROR);

                break;
        }
    } else {
        if (mtd_buff) {
            xRingbufferSend(mtd_buff, (void *)data, len, portMAX_DELAY);
        }
    }
}

void mtd_end(void)
{
    conn_state = false;

    data_err = false;
    data_recv = false;
}
