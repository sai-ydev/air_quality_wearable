#include "bme690_reader.h"
#include "bsec_integration.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include "bsec_iaq.h"
#include "driver/i2c_master.h"
#include "rom/ets_sys.h"
#include "sensor_task.h"


static const char *TAG = "bme690";

// Store the I2C bus handle globally (needed by bsec_integration.c)
static  i2c_master_bus_handle_t s_bus_handle = NULL;
static i2c_master_dev_handle_t s_i2c_dev = NULL;

// Latest sensor data from BSEC
static bme690_reading_t s_latest_reading = {0};
static bool s_data_ready = false;
static uint8_t dev_adress = BME69X_I2C_ADDR_LOW;  // BME690 I2C address (0x77 if SDO pin is high)

extern SemaphoreHandle_t s_i2c_mutex;  // Declare the mutex handle from sensor_task.c
//
// Callback functions required by BSEC integration
//
/*!
 * I2C read function map to BME690 sensor
 */
BME69X_INTF_RET_TYPE bme69x_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    (void)intf_ptr;
    
    if(!s_i2c_dev){
        return -1;
    }

    if(xSemaphoreTake(s_i2c_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take I2C mutex for read");
        return -1;
    }

    esp_err_t ret = i2c_master_transmit_receive(s_i2c_dev, &reg_addr, 1, reg_data, len, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    
    xSemaphoreGive(s_i2c_mutex);
    
    return (ret == ESP_OK) ? ESP_OK : -1;
}

/*!
 * I2C write function map to COINES platform
 */
BME69X_INTF_RET_TYPE bme69x_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    (void)intf_ptr;
    if(!s_i2c_dev){
        return -1;
    }
    uint8_t write_buf[1 + len];
    write_buf[0] = reg_addr;
    memcpy(&write_buf[1], reg_data, len);

    i2c_master_transmit_multi_buffer_info_t write_buf_info[] = {
        {
            .write_buffer = write_buf,
            .buffer_size = sizeof(write_buf),
        }
    };

    if(xSemaphoreTake(s_i2c_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take I2C mutex for write");
        return -1;
    }

    esp_err_t ret = i2c_master_multi_buffer_transmit(s_i2c_dev, write_buf_info, 1, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));

    xSemaphoreGive(s_i2c_mutex);    
    return (ret == ESP_OK) ? ESP_OK : -1;
}

 

/*!
 * Delay function map to COINES platform
 */
void bme69x_delay_us(uint32_t period, void *intf_ptr)
{
    if (period < 1000) {
        esp_rom_delay_us(period);
    } else {
        vTaskDelay(pdMS_TO_TICKS((period + 999) / 1000));
    }
}

// Get timestamp in milliseconds
static uint32_t get_timestamp_ms(void)
{
    return esp_timer_get_time() / 1000LL;
}

// Called when BSEC has new output data
static void output_ready(char *output)
{
    // Parse CSV from BSEC
    // Format: sens_no,timestamp_s,iaq,iaq_acc,static_iaq,raw_temp,raw_hum,
    //         comp_temp,comp_hum,pressure,gas,gas%,co2,voc,stab,runin,status
    
    uint8_t sens_no, iaq_acc;
    uint32_t timestamp_s;
    float iaq, static_iaq, raw_temp, raw_hum, comp_temp, comp_hum;
    float pressure, gas, gas_pct, co2, voc, stab, runin;
    int status;
    
    int parsed = sscanf(output, "%hhu,%u,%f,%hhu,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%d",
                       &sens_no, (unsigned int*)&timestamp_s, &iaq, &iaq_acc, &static_iaq,
                       &raw_temp, &raw_hum, &comp_temp, &comp_hum, &pressure,
                       &gas, &gas_pct, &co2, &voc, &stab, &runin, &status);
    
    if (parsed >= 10) {
        s_latest_reading.temperature = comp_temp;
        s_latest_reading.humidity = comp_hum;
        s_latest_reading.pressure = pressure;
        s_latest_reading.iaq = iaq;
        s_latest_reading.iaq_accuracy = iaq_acc;
        s_latest_reading.static_iaq = static_iaq;
        s_latest_reading.co2_equivalent = co2;
        s_latest_reading.breath_voc = voc;
        s_latest_reading.gas_resistance = gas;
        s_latest_reading.data_valid = true;
        s_data_ready = true;

        // NEW: Also send directly to logger queue
        extern QueueHandle_t s_reading_queue;  // Declare the queue
        
        if (s_reading_queue) {
            sensor_reading_t reading = {0};
            reading.timestamp_ms = timestamp_s * 1000;  // Convert to ms
            
            // BME690 data
            reading.temperature = comp_temp;
            reading.humidity = comp_hum;
            reading.pressure = pressure;
            reading.iaq = iaq;
            reading.iaq_accuracy = iaq_acc;
            reading.static_iaq = static_iaq;
            reading.co2_equivalent = co2;
            reading.breath_voc = voc;
            reading.gas_resistance = gas;
            
            // ZMOD data will be zeros for now (filled later if needed)
            // Or you can call zmod4510_read() here if you want
            
            reading.event_label = 0;
            snprintf(reading.event_name, sizeof(reading.event_name), "bme690");
            
            xQueueSend(s_reading_queue, &reading, 0);
        }
        
        ESP_LOGI(TAG, "T=%.1f°C H=%.1f%% P=%.0fhPa IAQ=%.0f(acc:%d) Gas=%.0fΩ CO2=%.0fppm VOC=%.2fppm",
                 comp_temp, comp_hum, pressure, iaq, iaq_acc, gas, co2, voc);
    }
}

void bme69x_intf_init(struct bme69x_dev *bme, uint8_t intf, void *intf_ptr)
{
       
    // Add device to I2C bus
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x76,  // BME690 address
        .scl_speed_hz = 100000,
    };

    
    if (i2c_master_bus_add_device(s_bus_handle, &dev_cfg, &s_i2c_dev) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add I2C device");
        return;
    }
    
    bme->read = bme69x_i2c_read;
    bme->write = bme69x_i2c_write;
    bme->delay_us = bme69x_delay_us;
    bme->intf = BME69X_I2C_INTF;
    bme->intf_ptr = &dev_adress;
    bme->amb_temp = 25;
}

// Load BSEC state from NVS (stub for now)
static uint32_t state_load(uint8_t *state_buffer, uint32_t n_buffer)
{
    // TODO: Implement NVS state loading later
    return 0;  // Return 0 = no saved state
}

// Save BSEC state to NVS (stub for now)
static void state_save(const uint8_t *state_buffer, uint32_t length)
{
    // TODO: Implement NVS state saving later
    ESP_LOGD(TAG, "State save called (%u bytes)", length);
}

// Load BSEC configuration (stub - use defaults)
static uint32_t config_load(uint8_t *config_buffer, uint32_t n_buffer)
{
    memcpy(config_buffer, bsec_config_iaq, n_buffer);
    return n_buffer;
}

//
// Public API
//

esp_err_t bme690_init(i2c_master_bus_handle_t i2c_bus)
{
    ESP_LOGI(TAG, "Initializing BME690 + BSEC2");
    
    s_bus_handle = i2c_bus;

    // DEBUG - verify it's set
    ESP_LOGI(TAG, "s_bus_handle = %p", (void*)s_bus_handle);
    
    if (!s_bus_handle) {
        ESP_LOGE(TAG, "ERROR: bus handle is NULL!");
        return ESP_ERR_INVALID_ARG;
    }
    // Initialize BSEC with Low Power mode (3 second intervals)
    return_values_init ret = bsec_iot_init(
        BSEC_SAMPLE_RATE_LP,
        bme69x_intf_init,  // Defined in bsec_integration.c
        state_load,
        config_load
    );
    
    if (ret.bme69x_status != BME69X_OK) {
        ESP_LOGE(TAG, "BME69x init failed: %d", ret.bme69x_status);
        return ESP_FAIL;
    }
    
    if (ret.bsec_status != BSEC_OK) {
        ESP_LOGE(TAG, "BSEC init failed: %d", ret.bsec_status);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "BME690 + BSEC2 initialized successfully");
    return ESP_OK;
}

esp_err_t bme690_read(bme690_reading_t *reading)
{
    if (!s_data_ready) {
        return ESP_ERR_NOT_FOUND;
    }
    
    memcpy(reading, &s_latest_reading, sizeof(*reading));
    return ESP_OK;
}

// BSEC processing task - call this from sensor_task
void bme690_process_loop(void* arg)
{
    (void)arg;  // Suppress unused parameter warning
    
    ESP_LOGI(TAG, "Starting BSEC processing loop");
    
    // This function never returns - it's an infinite loop
    bsec_iot_loop(
        state_save,
        get_timestamp_ms,
        output_ready
    );
}