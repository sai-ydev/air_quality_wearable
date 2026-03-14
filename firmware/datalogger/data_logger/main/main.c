#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "sensor_task.h"
#include "driver/gpio.h"
#include "logger_task.h"
#include "serial_handler.h"
#include "wifi_init.h"
#include "http_server.h"
#include "ml_inference.h"

#define WIFI_SSID "YinYangHG"
#define WIFI_PASSWORD "Sour2530Vinegar"

static const char *TAG = "main";

void app_main(void)
{
    gpio_reset_pin(ZMOD4510_RESET_GPIO);
    gpio_set_direction(ZMOD4510_RESET_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(ZMOD4510_RESET_GPIO, 1);   


    ESP_LOGI(TAG, "=== Asthma Guardian Logger ===");
    
    // Create reading queue
    QueueHandle_t reading_queue = xQueueCreate(10, sizeof(sensor_reading_t));
    if (!reading_queue) {
        ESP_LOGE(TAG, "Failed to create queue");
        return;
    }
    
    // Start sensor task
    esp_err_t ret = sensor_task_start(reading_queue);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Sensor task failed to start");
        return;
    }

    // Start logger task with same queue
    ret = logger_task_start(reading_queue);
    ESP_ERROR_CHECK(ret);

    // Start serial handler task
    ret = serial_handler_init();
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Initialization complete, entering main loop");

    ret = wifi_init_sta(WIFI_SSID, WIFI_PASSWORD);
    if(ret == ESP_OK){
        vTaskDelay(pdMS_TO_TICKS(5000));  // Wait for WiFi to connect
        http_server_init();
        http_server_print_info();
    }

    ret = ml_inference_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ML inference initialization failed");
        return;
    } else{
        ESP_LOGI(TAG, "ML inference initialized successfully");
    }
    
    // Main loop: just consume queue and print
    sensor_reading_t reading;
    while (1) {
        if (xQueueReceive(reading_queue, &reading, pdMS_TO_TICKS(100)) == pdTRUE) {
            ESP_LOGI(TAG, "Received: %s at %lu ms", 
                     reading.event_name, reading.timestamp_ms);
        }
    }
}