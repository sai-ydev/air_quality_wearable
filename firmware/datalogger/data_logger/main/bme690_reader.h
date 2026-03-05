#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"

typedef struct {
    float temperature;
    float humidity;
    float pressure;
    float iaq;
    uint8_t iaq_accuracy;
    float static_iaq;
    float co2_equivalent;
    float breath_voc;
    float gas_resistance;
    bool data_valid;
} bme690_reading_t;

// Initialize BME690 with BSEC2
esp_err_t bme690_init(i2c_master_bus_handle_t bus_handle);

// Get latest reading (non-blocking)
esp_err_t bme690_read(bme690_reading_t *reading);

// BSEC processing loop (blocking - run in separate task)
void bme690_process_loop(void* arg);