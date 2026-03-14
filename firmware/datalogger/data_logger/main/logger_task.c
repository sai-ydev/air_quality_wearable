#include "logger_task.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include <stdio.h>
#include <string.h>
#include "ml_inference.h"

static const char *TAG = "logger";

#define LOG_FILE_PATH "/spiffs/sensor_log.csv"
#define MAX_EVENT_NAME_LEN 16



static FILE *s_log_file = NULL;
static volatile int8_t s_current_label = 0;  // Default: LOW risk
static char s_event_name[MAX_EVENT_NAME_LEN] = "unlabeled";
static uint32_t s_sample_count = 0;

// Cache for merging sensor data
static sensor_reading_t s_latest_bme690 = {0};
static sensor_reading_t s_latest_zmod4510 = {0};
static bool s_bme690_updated = false;
static bool s_zmod4510_updated = false;
TaskHandle_t s_logger_task_handle = NULL;

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
esp_err_t open_log_file(void)
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

//Write merhged reading to log file
static void write_log_entry(void){
    float confidence;
    
    uint32_t timestamp_ms = (s_latest_bme690.timestamp_ms > s_latest_zmod4510.timestamp_ms) ? 
                        s_latest_bme690.timestamp_ms : s_latest_zmod4510.timestamp_ms;
    
    sensor_reading_t merged_reading = s_latest_bme690;  // Start with BME690 data
    // Overwrite ZMOD4510 fields    
    merged_reading.o3_ppb = s_latest_zmod4510.o3_ppb;
    merged_reading.no2_ppb = s_latest_zmod4510.no2_ppb;
    merged_reading.oaq_index = s_latest_zmod4510.oaq_index;
    merged_reading.rmox_0 = s_latest_zmod4510.rmox_0;
    merged_reading.rmox_1 = s_latest_zmod4510.rmox_1;
    merged_reading.rmox_2 = s_latest_zmod4510.rmox_2;
    merged_reading.rmox_3 = s_latest_zmod4510.rmox_3;
    merged_reading.event_label = s_current_label;
    strncpy(merged_reading.event_name, s_event_name, sizeof(merged_reading.event_name)-1);
    merged_reading.event_name[sizeof(merged_reading.event_name)-1] = '\0';  // Ensure null termination

    int8_t predicted_label = ml_inference_predict(&merged_reading, &confidence);

    ESP_LOGI(TAG, "ML: Predicted label: %d (%s) with confidence %.2f%%, Actual label: %d (%s)", 
             predicted_label, ml_inference_get_class_name(predicted_label), confidence * 100.0f, s_current_label, s_event_name);
    fprintf(s_log_file, "%lu,%.2f,%.2f,%.2f,%.1f,%d,%.1f,%.1f,%.2f,%.1f,"
                        "%.3f,%.3f,%.1f,%.3f,%.3f,%.3f,%.3f,%d,%s\n",
            timestamp_ms,
            s_latest_bme690.temperature,
            s_latest_bme690.humidity,
            s_latest_bme690.pressure,
            s_latest_bme690.iaq,
            s_latest_bme690.iaq_accuracy,
            s_latest_bme690.static_iaq,
            s_latest_bme690.co2_equivalent,
            s_latest_bme690.breath_voc,
            s_latest_bme690.gas_resistance,
            s_latest_zmod4510.o3_ppb,
            s_latest_zmod4510.no2_ppb,
            s_latest_zmod4510.oaq_index,
            s_latest_zmod4510.rmox_0,
            s_latest_zmod4510.rmox_1,
            s_latest_zmod4510.rmox_2,
            s_latest_zmod4510.rmox_3,
            s_current_label,
            s_event_name);
    s_sample_count++;

    // Flush every 10 samples to avoid data loss
    if (s_sample_count % 10 == 0) {
        fflush(s_log_file);
        ESP_LOGI(TAG, "Logged %lu samples", s_sample_count);
    }
}

// Detect if reading is from BME690 or ZMOD4510
static bool is_bme690_reading(const sensor_reading_t *reading) {
    return (reading->temperature != 0 || reading->humidity != 0 || reading->pressure != 0);
}

// Logger task
static void logger_task(void *arg)
{
    QueueHandle_t queue = (QueueHandle_t)arg;
    sensor_reading_t reading;
    
    ESP_LOGI(TAG, "Logger task started");
    
    while (1) {
        uint32_t notify_value;
        xTaskNotifyWait(0, LOGGER_NOTIFY_BIT, &notify_value, 0);
        if(notify_value & LOGGER_NOTIFY_BIT) {
            // We have to delete the log file and start a new one to avoid fragmentation issues in SPIFFS
            fclose(s_log_file);
            s_log_file = NULL;
            s_sample_count = 0;
            remove(LOG_FILE_PATH);
            open_log_file();
            ESP_LOGI(TAG, "Log file cleared and recreated");
        }

        if (xQueueReceive(queue, &reading, portMAX_DELAY) == pdTRUE) {
            if (is_bme690_reading(&reading)) {
                s_latest_bme690 = reading;
                s_bme690_updated = true;
            } else {
                s_latest_zmod4510 = reading;
                s_zmod4510_updated = true;
            }
            
            // Write log entry when we have new data from both sensors
            if (s_bme690_updated && s_zmod4510_updated) {
                write_log_entry();
                s_bme690_updated = false;
                s_zmod4510_updated = false;
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
        &s_logger_task_handle
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