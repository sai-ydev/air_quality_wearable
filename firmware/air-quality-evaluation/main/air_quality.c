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
#include "bsec_integration.h"
#include "bsec_selectivity.h"
#include "driver/gpio.h"
#include "esp_task_wdt.h"

/* Macro for count of samples to be displayed */
#define SAMPLE_COUNT  UINT8_C(300)
#define LED_PIN         GPIO_NUM_23
#define RESET_PIN       GPIO_NUM_22
#define ON          1
#define OFF         0

static const char *TAG = "Air Quality Example";
extern uint8_t n_sensors;
extern uint8_t *bsecInstance[NUM_OF_SENS];
i2c_master_bus_handle_t bus_handle;

static uint32_t state_load(uint8_t *state_buffer, uint32_t n_buffer);

static uint32_t config_load(uint8_t *config_buffer, uint32_t n_buffer);

static uint32_t get_timestamp_ms();

static void state_save(const uint8_t *state_buffer, uint32_t length);

static void output_ready(char *outputs);

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
    bsec_version_t version;
    return_values_init ret = {BME69X_OK, BSEC_OK};
    char header[400];
  
    i2c_master_init(&bus_handle);
    gpio_reset_pin(LED_PIN);
    gpio_reset_pin(RESET_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(RESET_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(RESET_PIN, 1);
    ESP_LOGI(TAG, "I2C initialized successfully");

	ret = bsec_iot_init(SAMPLE_RATE, bme69x_interface_init, state_load, config_load);
	
	if (ret.bme69x_status != BME69X_OK) {
		ESP_LOGI(TAG, "ERROR while initializing BME69x: %d\r\n", ret.bme69x_status);
	}
	if (ret.bsec_status < BSEC_OK) {
		ESP_LOGI(TAG, "\nERROR while initializing BSEC library: %d\n", ret.bsec_status);
	}
	else if (ret.bsec_status > BSEC_OK) {
		ESP_LOGI(TAG, "\nWARNING while initializing BSEC library: %d\n", ret.bsec_status);
	}

	ret.bsec_status = bsec_get_version(bsecInstance, &version);

    ESP_LOGI(TAG, "BSEC Version : %u.%u.%u.%u\r\n",version.major,version.minor,version.major_bugfix,version.minor_bugfix);

#if (OUTPUT_MODE == IAQ)
    sprintf(header, "Sensor_No, Time(ms), IAQ,  IAQ_accuracy, Static_IAQ, Raw_Temperature(degC), Raw_Humidity(%%rH), Comp_Temperature(degC),  Comp_Humidity(%%rH), Raw_pressure(Pa), Raw_Gas(ohms), Gas_percentage, CO2, bVOC, Stabilization_status, Run_in_status, Bsec_status\r\n");
#else
    sprintf(header, "Sensor_No, Time(ms), Class/Target_1_prediction, Class/Target_2_prediction, Class/Target_3_prediction, Class/Target_4_prediction, Prediction_accuracy_1, Prediction_accuracy_2, Prediction_accuracy_3, Prediction_accuracy_4, Raw_pressure(Pa), Raw_Temperature(degC),  Raw_Humidity(%%rH), Raw_Gas(ohm), Raw_Gas_Index(num), Bsec_status\r\n");
#endif

    ESP_LOGI(TAG, "%s", header);

    

    bsec_iot_loop(state_save, get_timestamp_ms, output_ready);
   

    ESP_ERROR_CHECK(i2c_del_master_bus(bus_handle));
    ESP_LOGI(TAG, "I2C de-initialized successfully");


}


static uint32_t state_load(uint8_t *state_buffer, uint32_t n_buffer)
{
    // ...
    // Load a previous library state from non-volatile memory, if available.
    //
    // Return zero if loading was unsuccessful or no state was available, 
    // otherwise return length of loaded state string.
    // ...
    return 0;
}

static uint32_t config_load(uint8_t *config_buffer, uint32_t n_buffer)
{
	memcpy(config_buffer, bsec_config_selectivity, n_buffer);
    return n_buffer;
}

static uint32_t get_timestamp_ms()
{    
    return pdTICKS_TO_MS(xTaskGetTickCount());
}

static void state_save(const uint8_t *state_buffer, uint32_t length)
{
    // ...
    // Save the string some form of non-volatile memory, if possible.
    // ...
}

static void output_ready(char *outputs)
{
    gpio_set_level(LED_PIN, ON);
    
    ESP_LOGI(TAG, "%s", outputs);

    gpio_set_level(LED_PIN, OFF);
}