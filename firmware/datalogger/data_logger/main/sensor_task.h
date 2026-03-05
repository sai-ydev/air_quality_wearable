#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_err.h"

#define ZMOD4510_RESET_GPIO 22
#define I2C_MASTER_TIMEOUT_MS       1000
// Simplified sensor reading structure - just placeholders for now
typedef struct {
    uint32_t timestamp_ms;
    
    // BME690 data
    float temperature;
    float humidity;
    float pressure;
    float iaq;
    uint8_t iaq_accuracy;
    float static_iaq;
    float co2_equivalent;
    float breath_voc;
    float gas_resistance;
    
    // ZMOD4510 data
    float o3_ppb;
    float no2_ppb;
    float oaq_index;
    float rmox_0;
    float rmox_1;
    float rmox_2;
    float rmox_3;
    
    // Labeling
    int8_t event_label;
    char event_name[16];
} sensor_reading_t;

// Start the sensor task
esp_err_t sensor_task_start(QueueHandle_t reading_queue);

// Expose the queue globally
extern QueueHandle_t s_reading_queue;