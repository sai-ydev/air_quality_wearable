#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "sensor_task.h"  // For sensor_reading_t

// Start the logger task
esp_err_t logger_task_start(QueueHandle_t reading_queue);

// Set current event label (0=LOW, 1=MODERATE, 2=HIGH, -1=unlabeled)
void logger_set_label(int8_t label);

// Set event name (optional, e.g., "COOKING", "TRAFFIC")
void logger_set_event_name(const char *name);