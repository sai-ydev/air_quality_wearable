/**
 * Copyright (C) 2025 Bosch Sensortec GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "bme69x.h"
#include "i2c_interface.h"
#include "rom/ets_sys.h"
#include "air_quality.h"
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "FreeRTOSConfig.h"
#include "esp_log.h"
/******************************************************************************/
/*!                Static variable definition                                 */
static uint8_t dev_addr;
static i2c_master_dev_handle_t bme690_dev_handle;
static const char* TAG = "BME690_I2C";
/******************************************************************************/
/*!                User interface functions                                   */


/*!
 * I2C read function map to BME690 sensor
 */
BME69X_INTF_RET_TYPE bme69x_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    (void)intf_ptr;
    ESP_ERROR_CHECK(i2c_master_transmit_receive(bme690_dev_handle, &reg_addr, 1, reg_data, len, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS)));
    return BME69X_OK;
}

/*!
 * I2C write function map to COINES platform
 */
BME69X_INTF_RET_TYPE bme69x_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    (void)intf_ptr;
    uint8_t write_buf[1 + len];
    write_buf[0] = reg_addr;
    memcpy(&write_buf[1], reg_data, len);

    i2c_master_transmit_multi_buffer_info_t write_buf_info[] = {
        {
            .write_buffer = write_buf,
            .buffer_size = sizeof(write_buf),
        }
    };

    return i2c_master_multi_buffer_transmit(bme690_dev_handle, write_buf_info, 1,  pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
}

 

/*!
 * Delay function map to COINES platform
 */
void bme69x_delay_us(uint32_t period, void *intf_ptr)
{
    (void)intf_ptr;
    ets_delay_us(period);
}

void bme69x_check_rslt(const char api_name[], int8_t rslt)
{
    switch (rslt)
    {
        case BME69X_OK:

            /* Do nothing */
            break;
        case BME69X_E_NULL_PTR:
            ESP_LOGI(TAG, "API name [%s]  Error [%d] : Null pointer\r\n", api_name, rslt);
            break;
        case BME69X_E_COM_FAIL:
            ESP_LOGI(TAG, "API name [%s]  Error [%d] : Communication failure\r\n", api_name, rslt);
            break;
        case BME69X_E_INVALID_LENGTH:
            ESP_LOGI(TAG, "API name [%s]  Error [%d] : Incorrect length parameter\r\n", api_name, rslt);
            break;
        case BME69X_E_DEV_NOT_FOUND:
            ESP_LOGI(TAG, "API name [%s]  Error [%d] : Device not found\r\n", api_name, rslt);
            break;
        case BME69X_E_SELF_TEST:
            ESP_LOGI(TAG, "API name [%s]  Error [%d] : Self test error\r\n", api_name, rslt);
            break;
        case BME69X_W_NO_NEW_DATA:
            ESP_LOGI(TAG, "API name [%s]  Warning [%d] : No new data found\r\n", api_name, rslt);
            break;
        default:
            ESP_LOGI(TAG, "API name [%s]  Error [%d] : Unknown error code\r\n", api_name, rslt);
            break;
    }
}

int8_t bme69x_interface_init(struct bme69x_dev *bme, uint8_t intf, i2c_master_bus_handle_t* bus_handle)
{
    int8_t rslt = BME69X_OK;
    if (bme != NULL)
    {

        /* Bus configuration : I2C */
        if (intf == BME69X_I2C_INTF)
        {
            dev_addr = BME69X_I2C_ADDR_LOW;
            bme->read = bme69x_i2c_read;
            bme->write = bme69x_i2c_write;
            bme->intf = BME69X_I2C_INTF;

            i2c_device_config_t dev_config = {
                .dev_addr_length = I2C_ADDR_BIT_LEN_7,
                .device_address = dev_addr,
                .scl_speed_hz = I2C_MASTER_FREQ_HZ,
            };

            ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &dev_config, &bme690_dev_handle));
        }
        else
        {
            rslt = BME69X_E_COM_FAIL;
            return rslt;
        }


        bme->delay_us = bme69x_delay_us;
        bme->intf_ptr = &dev_addr;
        bme->amb_temp = 25; /* The ambient temperature in deg C is used for defining the heater temperature */
    }
    else
    {
        rslt = BME69X_E_NULL_PTR;
    }

    return rslt;
}

