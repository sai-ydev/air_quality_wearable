#include "sensor_test.h"
#include "i2c_scanner.h"
#include "esp_log.h"

static const char *TAG = "sensor_test";

// BME68x registers
#define BME68X_REG_CHIP_ID      0xD0
#define BME68X_CHIP_ID          0x61

// ZMOD4510 registers  
#define ZMOD4510_REG_PID        0x00    // Product ID (2 bytes)

esp_err_t test_bme690_communication(i2c_master_bus_handle_t bus_handle)
{
    ESP_LOGI(TAG, "Testing BME690 at 0x%02X...", BME690_ADDR);
    
    // Create device handle
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BME690_ADDR,
        .scl_speed_hz = I2C_SPEED_HZ,
    };
    
    i2c_master_dev_handle_t dev_handle;
    esp_err_t ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add BME690 device: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Read chip ID
    uint8_t reg = BME68X_REG_CHIP_ID;
    uint8_t chip_id = 0;
    
    ret = i2c_master_transmit_receive(dev_handle, &reg, 1, &chip_id, 1, 1000);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "BME690 Chip ID: 0x%02X (expected 0x%02X)", 
                 chip_id, BME68X_CHIP_ID);
        if (chip_id == BME68X_CHIP_ID) {
            ESP_LOGI(TAG, "✓ BME690 communication OK");
        } else {
            ESP_LOGW(TAG, "⚠ Unexpected chip ID - might be BME688?");
        }
    } else {
        ESP_LOGE(TAG, "✗ BME690 read failed: %s", esp_err_to_name(ret));
    }
    
    i2c_master_bus_rm_device(dev_handle);
    return ret;
}

esp_err_t test_zmod4510_communication(i2c_master_bus_handle_t bus_handle)
{
    ESP_LOGI(TAG, "Testing ZMOD4510 at 0x%02X...", ZMOD4510_ADDR);
    
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = ZMOD4510_ADDR,
        .scl_speed_hz = I2C_SPEED_HZ,
    };
    
    i2c_master_dev_handle_t dev_handle;
    esp_err_t ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add ZMOD4510 device: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Read product ID (2 bytes starting at 0x00)
    uint8_t reg = ZMOD4510_REG_PID;
    uint8_t pid[2] = {0};
    
    ret = i2c_master_transmit_receive(dev_handle, &reg, 1, pid, 2, 1000);
    
    if (ret == ESP_OK) {
        uint16_t product_id = (pid[0] << 8) | pid[1];
        ESP_LOGI(TAG, "ZMOD4510 Product ID: 0x%04X", product_id);
        
        // ZMOD4510 PID should be 0x6320 according to datasheet
        if (product_id == 0x6320) {
            ESP_LOGI(TAG, "✓ ZMOD4510 communication OK");
        } else {
            ESP_LOGW(TAG, "⚠ Unexpected PID (expected 0x6320)");
            ESP_LOGW(TAG, "  This might still work - continuing...");
        }
    } else {
        ESP_LOGE(TAG, "✗ ZMOD4510 read failed: %s", esp_err_to_name(ret));
    }
    
    i2c_master_bus_rm_device(dev_handle);
    return ret;
}