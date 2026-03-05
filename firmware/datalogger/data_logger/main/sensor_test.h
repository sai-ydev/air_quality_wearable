#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"

// Test basic sensor communication
esp_err_t test_bme690_communication(i2c_master_bus_handle_t bus_handle);
esp_err_t test_zmod4510_communication(i2c_master_bus_handle_t bus_handle);