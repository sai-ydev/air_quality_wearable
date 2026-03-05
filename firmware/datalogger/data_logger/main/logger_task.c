#include "logger_task.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "logger";

#define LOG_FILE_PATH "/spiffs/sensor_log.csv"
#define MAX_EVENT_NAME_LEN 16

static FILE *s_log_file = NULL;
static int8_t s_current_label = 0;  // Default: LOW risk
static char s_event_name[MAX_EVENT_NAME_LEN] = "unlabeled";
static uint32_t s_sample_count = 0;

// Mount SPIFFS
static esp_err_t init_spiffs(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");
    
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };
    
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SPIFFS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SPIFFS: %d KB total, %d KB used", total / 1024, used / 1024);
    }
    
    return ESP_OK;
}

// Open log file and write CSV header
static esp_err_t open_log_file(void)
{
    s_log_file = fopen(LOG_FILE_PATH, "a");
    if (!s_log_file) {
        ESP_LOGE(TAG, "Failed to open log file");
        return ESP_FAIL;
    }
    
    // Write CSV header if file is empty
    fseek(s_log_file, 0, SEEK_END);
    if (ftell(s_log_file) == 0) {
        fprintf(s_log_file, "timestamp_ms,temperature,humidity,pressure,iaq,iaq_accuracy,"
                            "static_iaq,co2_equiv,breath_voc,gas_resistance,"
                            "o3_ppb,no2_ppb,oaq_index,rmox_0,rmox_1,rmox_2,rmox_3,"
                            "label,event_name\n");
        fflush(s_log_file);
        ESP_LOGI(TAG, "Created new log file with header");
    }
    
    return ESP_OK;
}

// Logger task
static void logger_task(void *arg)
{
    QueueHandle_t queue = (QueueHandle_t)arg;
    sensor_reading_t reading;
    
    ESP_LOGI(TAG, "Logger task started");
    
    while (1) {
        if (xQueueReceive(queue, &reading, portMAX_DELAY) == pdTRUE) {
            // Write CSV row
            fprintf(s_log_file, "%lu,%.2f,%.2f,%.2f,%.1f,%d,%.1f,%.1f,%.2f,%.1f,"
                               "%.3f,%.3f,%.1f,%.3f,%.3f,%.3f,%.3f,%d,%s\n",
                    reading.timestamp_ms,
                    reading.temperature,
                    reading.humidity,
                    reading.pressure,
                    reading.iaq,
                    reading.iaq_accuracy,
                    reading.static_iaq,
                    reading.co2_equivalent,
                    reading.breath_voc,
                    reading.gas_resistance,
                    reading.o3_ppb,
                    reading.no2_ppb,
                    reading.oaq_index,
                    reading.rmox_0,
                    reading.rmox_1,
                    reading.rmox_2,
                    reading.rmox_3,
                    s_current_label,
                    s_event_name);
            
            s_sample_count++;
            
            // Flush every 10 samples to avoid data loss
            if (s_sample_count % 10 == 0) {
                fflush(s_log_file);
                ESP_LOGI(TAG, "Logged %lu samples", s_sample_count);
            }
        }
    }
}

esp_err_t logger_task_start(QueueHandle_t reading_queue)
{
    // Mount SPIFFS
    esp_err_t ret = init_spiffs();
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Open log file
    ret = open_log_file();
    if (ret != ESP_OK) {
        return ret;
    }
    
    // Start logger task
    BaseType_t task_ret = xTaskCreate(
        logger_task,
        "logger",
        4096,
        (void*)reading_queue,
        3,  // Lower priority than sensor tasks
        NULL
    );
    
    return (task_ret == pdPASS) ? ESP_OK : ESP_ERR_NO_MEM;
}

void logger_set_label(int8_t label)
{
    s_current_label = label;
    ESP_LOGI(TAG, "Label set to: %d", label);
}

void logger_set_event_name(const char *name)
{
    snprintf(s_event_name, MAX_EVENT_NAME_LEN, "%s", name);
    ESP_LOGI(TAG, "Event name set to: %s", s_event_name);
}