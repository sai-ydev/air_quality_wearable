#include "i2c_scanner.h"
#include "esp_log.h"

static const char *TAG = "i2c_scan";

esp_err_t i2c_bus_init(i2c_master_bus_handle_t *bus_handle)
{
    ESP_LOGI(TAG, "Initializing I2C bus (SDA=%d, SCL=%d, %d Hz)",
             I2C_SDA_GPIO, I2C_SCL_GPIO, I2C_SPEED_HZ);
    
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = 0,
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,  // Use external pull-ups
    };
    
    esp_err_t ret = i2c_new_master_bus(&bus_cfg, bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C bus init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "I2C bus initialized successfully");
    return ESP_OK;
}

void i2c_scan_bus(i2c_master_bus_handle_t bus_handle)
{
    ESP_LOGI(TAG, "Scanning I2C bus...");
    ESP_LOGI(TAG, "     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f");
    
    for (uint8_t row = 0; row < 8; row++) {
        printf("%02x: ", row * 16);
        for (uint8_t col = 0; col < 16; col++) {
            uint8_t addr = row * 16 + col;
            
            // Skip reserved addresses
            if (addr < 0x03 || addr > 0x77) {
                printf("   ");
                continue;
            }
            
            esp_err_t ret = i2c_master_probe(bus_handle, addr, 100);
            if (ret == ESP_OK) {
                printf("%02x ", addr);
            } else {
                printf("-- ");
            }
        }
        printf("\n");
    }
    
    ESP_LOGI(TAG, "Scan complete. Looking for:");
    ESP_LOGI(TAG, "  BME690:   0x76 or 0x77");
    ESP_LOGI(TAG, "  ZMOD4510: 0x32");
}