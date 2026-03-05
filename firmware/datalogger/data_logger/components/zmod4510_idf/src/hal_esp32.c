/*****************************************************************************
 * Copyright (c) 2024 Renesas Electronics Corporation
 * All Rights Reserved.
 * 
 * This code is proprietary to Renesas, and is license pursuant to the terms and
 * conditions that may be accessed at:
 * https://www.renesas.com/eu/en/document/msc/renesas-software-license-terms-gas-sensor-software
 *****************************************************************************/

/** 
 * @addtogroup hal_api
 * @file template.c
 * 
 * @brief This file serves as a template for a customer specific HAL.
 * 
 * If not otherwise specified in the comments, all functions must be 
 * implemented. Detailed description of the HAL API can be found in
 * the firmware documentation for the Renesas product you're trying 
 * to interface.
 */

#include "hal.h"
/* add your includes here */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "sensor_task.h"
#include "zmod4510_config_no2_o3.h"


static i2c_master_dev_handle_t s_i2c_zmod4510_dev = NULL;
static const char *TAG = "ZMOD4510_HAL";
extern SemaphoreHandle_t s_i2c_mutex;  // Declare the mutex handle from sensor_task.c

static void
_Sleep ( uint32_t  ms ) {
    /** This function should do nothing but returning after the number of
     *  milliseconds passed in the 'ms' argument. */
     vTaskDelay(pdMS_TO_TICKS(ms));
}


static int
_I2CRead ( void*     handle, uint8_t  slAddr, 
           uint8_t*  wrData, int      wrLen, 
           uint8_t*  rdData, int      rdLen ) {
  /** The implementation of this function shall read data from I2C, strictly
   *  following the procedure below. If wrLen is not zero, the read must be
   *  preceded by an I2C write.
   *
   *  - I2C Start
   *  - if wrLen != 0:
   *    - Send: slAddr + WRITE
   *    - Send: wrLen bytes from wrData
   *    - I2C Restart  ( no stop before this start condition! )
   *  - Send: SlaveAddres + READ
   *  - Receive: rdLen bytes into rdData
   *  - I2CStop
   */
    if(handle == NULL){
        return -1;  // Invalid handle
    }
    if(xSemaphoreTake(s_i2c_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take I2C mutex for read");
        return -1;
    }
    esp_err_t ret;
    if(wrLen > 0) {
        ret = i2c_master_transmit_receive(s_i2c_zmod4510_dev, wrData, wrLen, rdData, rdLen, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    } else {
        ret = i2c_master_receive(s_i2c_zmod4510_dev, rdData, rdLen, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    }

    xSemaphoreGive(s_i2c_mutex);

    return (ret == ESP_OK) ? ESP_OK : -1;
}


static int
_I2CWrite ( void*     handle,  uint8_t  slAddr, 
            uint8_t*  wrData1, int      wrLen1, 
            uint8_t*  wrData2, int      wrLen2 ) {

  /** The implementation of this function shall send the data provided in
   *  arguments wrData1 and wrData2 over the I2C bus. Both data blocks shall
   *  be sent directly after each other without a new start/restart condition.
   * 
   *  - I2C Start
   *  - Send: slAddr + WRITE
   *  - if wrLen1 != 0:
   *    - Send: wrLen1 bytes from wrData1
   *  - if wrLen2 != 0;
   *    - Send: wrLen2 bytes from wrData2
   *  - I2CStop
   */
    if(handle == NULL){
        return -1;  // Invalid handle
    }

    uint8_t buffer_count = 0;

    i2c_master_transmit_multi_buffer_info_t write_buf_info[2] = {0};

    if(wrLen1 > 0) {
        write_buf_info[buffer_count].write_buffer = wrData1;
        write_buf_info[buffer_count].buffer_size = wrLen1;
        buffer_count++;
    }
    if(wrLen2 > 0) {
        write_buf_info[buffer_count].write_buffer = wrData2;
        write_buf_info[buffer_count].buffer_size = wrLen2;
        buffer_count++;
    }

    if(xSemaphoreTake(s_i2c_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take I2C mutex for write");
        return -1;
    }

    esp_err_t ret = i2c_master_multi_buffer_transmit(s_i2c_zmod4510_dev, write_buf_info, buffer_count, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));

    xSemaphoreGive(s_i2c_mutex);
    return (ret == ESP_OK) ? ESP_OK : -1;
}


static int
_Reset ( ) {
    /** This function shall pulse the reset pin of the sensor. 
     * @note Refer to the datasheet of the sensor(s) being interfaced for reset
     * timing requirements.
     */
     gpio_set_level(ZMOD4510_RESET_GPIO, 0);  // Set reset pin low
     vTaskDelay(pdMS_TO_TICKS(10));            // Hold for 10 ms   
     gpio_set_level(ZMOD4510_RESET_GPIO, 1);  // Set reset pin high
     vTaskDelay(pdMS_TO_TICKS(10));            // Wait for sensor to reset
    
     return ESP_OK;  // Return 0 on success
}


int
HAL_Init ( Interface_t*  hal ) {
  int  errorCode = 0;

  /* hal is initialized before init is called*/
  if(!hal->handle){
      return -1;  // Invalid handle
  }

  // Add device to I2C bus
  i2c_device_config_t dev_cfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = ZMOD4510_I2C_ADDR,  // ZMOD4510 address
      .scl_speed_hz = 100000,
  };

  if (i2c_master_bus_add_device(hal->handle, &dev_cfg, &s_i2c_zmod4510_dev) != ESP_OK) {
    ESP_LOGE(TAG, "Failed to add I2C device");
    return -1;  // Failed to add I2C device
  }

  /* Populate the hal object. Some of the functions may be optional
   *  for the product you're trying to interface. Please check the
   *  firmware documentation of your product.
   * If a function is not used, it must be set to NULL. */
  hal -> msSleep  = _Sleep;
  hal -> i2cRead  = _I2CRead;
  hal -> i2cWrite = _I2CWrite;
  hal -> reset    = _Reset;

  return errorCode;
}

int
HAL_Deinit ( Interface_t*  hal ) {
  int  errorCode = 0;
  
  ESP_ERROR_CHECK(i2c_master_bus_rm_device(s_i2c_zmod4510_dev));
  s_i2c_zmod4510_dev = NULL;
  ESP_LOGI(TAG, "HAL deinitialized successfully");

  return errorCode;
}

void
HAL_HandleError ( int  errorCode, void const*  context ) {
  /** Define what happens in case of error */
  ESP_LOGE(TAG, "HAL error occurred: %d", errorCode);

}
