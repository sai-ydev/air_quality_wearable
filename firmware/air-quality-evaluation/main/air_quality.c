/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* i2c - Simple Example

   Simple I2C example that shows how to initialize I2C
   as well as reading and writing from and to registers for a sensor connected over I2C.

   The sensor used in this example is a MPU9250 inertial measurement unit.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "air_quality.h"
#include "bme69x.h"
#include "i2c_interface.h"

/* Macro for count of samples to be displayed */
#define SAMPLE_COUNT  UINT8_C(300)

static const char *TAG = "Air Quality Example";


/**
 * @brief i2c master initialization
 */
static void i2c_master_init(i2c_master_bus_handle_t *bus_handle)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, bus_handle));

}

void app_main(void)
{
    i2c_master_bus_handle_t bus_handle;

    struct bme69x_dev bme;
    int8_t rslt;
    struct bme69x_conf conf;
    struct bme69x_heatr_conf heatr_conf;
    struct bme69x_data data[3];
    uint32_t del_period;
    uint32_t time_ms = 0;
    uint8_t n_fields;
    uint16_t sample_count = 1;
    
    i2c_master_init(&bus_handle);
    ESP_LOGI(TAG, "I2C initialized successfully");

     /* Heater temperature in degree Celsius */
    uint16_t temp_prof[10] = { 200, 240, 280, 320, 360, 360, 320, 280, 240, 200 };

    /* Heating duration in milliseconds */
    uint16_t dur_prof[10] = { 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 };

    /* Interface preference is updated as a parameter
     * For I2C : BME69X_I2C_INTF
     * For SPI : BME69X_SPI_INTF
     */
    rslt = bme69x_interface_init(&bme, BME69X_I2C_INTF, &bus_handle);
    bme69x_check_rslt("bme69x_interface_init", rslt);


    rslt = bme69x_init(&bme);
    bme69x_check_rslt("bme69x_init", rslt);

    
    /* Check if rslt == BME69X_OK, report or handle if otherwise */
    rslt = bme69x_get_conf(&conf, &bme);
    bme69x_check_rslt("bme69x_get_conf", rslt);

    /* Set configuration */
    conf.filter = BME69X_FILTER_OFF;
    conf.odr = BME69X_ODR_NONE; /* This parameter defines the sleep duration after each profile */
    conf.os_hum = BME69X_OS_16X;
    conf.os_pres = BME69X_OS_1X;
    conf.os_temp = BME69X_OS_2X;
    rslt = bme69x_set_conf(&conf, &bme);
    bme69x_check_rslt("bme69x_set_conf", rslt);

    /* Set heater configuration  */
    heatr_conf.enable = BME69X_ENABLE;
    heatr_conf.heatr_temp_prof = temp_prof;
    heatr_conf.heatr_dur_prof = dur_prof;
    heatr_conf.profile_len = 10;
    rslt = bme69x_set_heatr_conf(BME69X_SEQUENTIAL_MODE, &heatr_conf, &bme);
    bme69x_check_rslt("bme69x_set_heatr_conf", rslt);

    
    rslt = bme69x_set_op_mode(BME69X_SEQUENTIAL_MODE, &bme);
    bme69x_check_rslt("bme69x_set_op_mode", rslt);

        /* Check if rslt == BME69X_OK, report or handle if otherwise */
    printf(
        "Sample, TimeStamp(ms), Temperature(deg C), Pressure(Pa), Humidity(%%), Gas resistance(ohm), Status, Profile index, Measurement index\n");
    while (sample_count <= SAMPLE_COUNT)
    {
        /* Calculate delay period in microseconds */
        del_period = bme69x_get_meas_dur(BME69X_SEQUENTIAL_MODE, &conf, &bme) + (heatr_conf.heatr_dur_prof[0] * 1000);
        bme.delay_us(del_period, bme.intf_ptr);

        time_ms = pdTICKS_TO_MS(xTaskGetTickCount());

        rslt = bme69x_get_data(BME69X_SEQUENTIAL_MODE, data, &n_fields, &bme);
        bme69x_check_rslt("bme69x_get_data", rslt);

        /* Check if rslt == BME69X_OK, report or handle if otherwise */
        for (uint8_t i = 0; i < n_fields; i++)
        {
#ifdef BME69X_USE_FPU
            ESP_LOGI(TAG, "%u,%lu,%.2f,%.2f,%.2f,%.2f,0x%x,%d,%d\n",
                   sample_count,
                   (long unsigned int)time_ms + (i * (del_period / 2000)),
                   data[i].temperature,
                   data[i].pressure,
                   data[i].humidity,
                   data[i].gas_resistance,
                   data[i].status,
                   data[i].gas_index,
                   data[i].meas_index);
#else
            ESP_LOGI(TAG,"%u, %lu, %d, %lu, %lu, %lu, 0x%x, %d, %d\n",
                   sample_count,
                   (long unsigned int)time_ms + (i * (del_period / 2000)),
                   (data[i].temperature),
                   (long unsigned int)data[i].pressure,
                   (long unsigned int)(data[i].humidity),
                   (long unsigned int)data[i].gas_resistance,
                   data[i].status,
                   data[i].gas_index,
                   data[i].meas_index);
#endif
            sample_count++;
           
        }
        vTaskDelay(pdMS_TO_TICKS(100)); /* Small delay to allow logging output */
    }
    
    ESP_ERROR_CHECK(i2c_del_master_bus(bus_handle));
    ESP_LOGI(TAG, "I2C de-initialized successfully");


}
