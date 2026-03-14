#pragma once

#include "sensor_task.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize ML model
esp_err_t ml_inference_init(void);

// Run inference on sensor reading
// Returns predicted class: 0=LOW, 1=MODERATE, 2=HIGH
int8_t ml_inference_predict(const sensor_reading_t *reading, float *confidence);

// Get class name
const char* ml_inference_get_class_name(int8_t class_id);

#ifdef __cplusplus
}
#endif