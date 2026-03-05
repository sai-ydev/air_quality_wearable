#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"

// Pin definitions - adjust these to match your PCB
#define I2C_SDA_GPIO        3
#define I2C_SCL_GPIO        2
#define I2C_SPEED_HZ        100000  // 100 kHz

// Expected sensor addresses
#define BME690_ADDR         0x76    // or 0x77 if SDO pin is high
#define ZMOD4510_ADDR       0x33

// Initialize I2C bus
esp_err_t i2c_bus_init(i2c_master_bus_handle_t *bus_handle);

// Scan I2C bus and print found devices
void i2c_scan_bus(i2c_master_bus_handle_t bus_handle);