#include "serial_handler.h"
#include "logger_task.h"
#include "esp_log.h"
#include "driver/usb_serial_jtag.h"
#include "esp_spiffs.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


static const char *TAG = "serial_handler";
extern TaskHandle_t s_logger_task_handle;  // From logger_task.c

#define RX_BUF_SIZE 16
#define LOG_FILE_PATH "/spiffs/sensor_log.csv"
portMUX_TYPE serial_mux = portMUX_INITIALIZER_UNLOCKED;

static void dump_log_file() {
    
    FILE *f = fopen(LOG_FILE_PATH, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open log file for reading");
        return;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    ESP_LOGI(TAG, "=== LOG DUMP START (%ld bytes) ===", file_size);
    vTaskDelay(pdMS_TO_TICKS(100));  // Let log print
    
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        printf("%s", line);
        vTaskDelay(pdMS_TO_TICKS(10));  // Let log print
    }
    fclose(f);
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "=== LOG DUMP END ===");
}

static void show_log_stats(void){
    FILE *f = fopen(LOG_FILE_PATH, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open log file for reading");
        return;
    }

    size_t line_count = 0;
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        line_count++;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fclose(f);

    size_t total = 0, used = 0;
    esp_spiffs_info(NULL, &total, &used);

    ESP_LOGI(TAG, "=== LOG STATS ===");
    ESP_LOGI(TAG, "  Samples: %d", line_count - 1);  // -1 for header
    ESP_LOGI(TAG, "  File size: %ld bytes (%.1f KB)", file_size, file_size / 1024.0);
    ESP_LOGI(TAG, "  SPIFFS: %d KB used / %d KB total", used / 1024, total / 1024);
}

static void clear_log_file() {

    ESP_LOGI(TAG, "Clearing log file...");
    
    xTaskNotify(s_logger_task_handle, LOGGER_NOTIFY_BIT, eSetBits);  // Notify logger task to clear file safely
    
}

static void serial_task(void *arg)
{
    usb_serial_jtag_driver_config_t usb_serial_jtag_config = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&usb_serial_jtag_config));

    uint8_t data[RX_BUF_SIZE];
    ESP_LOGI(TAG, "Serial handler task started");
    ESP_LOGI(TAG, "Commands: 0=LOW, 1=MODERATE, 2=HIGH, x=unlabeled, d=dump, c=clear");
    
    while (1) {       
        
        char cmd;
        int len;
        
        len = usb_serial_jtag_read_bytes(data, RX_BUF_SIZE, pdMS_TO_TICKS(10));

        if(len > 0) {
            ESP_LOGI(TAG, "Received %d bytes from UART", len);

            
            for(int i = 0; i < len; i++) {
                cmd = data[i];
                switch (cmd) {
                    case '0':
                        logger_set_label(0);
                        logger_set_event_name("LOW");
                        ESP_LOGI(TAG, "-> Label: LOW (0)");
                        break;
                    case '1':
                        logger_set_label(1);
                        logger_set_event_name("MODERATE");
                        ESP_LOGI(TAG, "-> Label: MODERATE (1)");
                        break;
                    case '2':
                        logger_set_label(2);
                        logger_set_event_name("HIGH");
                        ESP_LOGI(TAG, "-> Label: HIGH (2)");
                        break;
                    case 'x':
                    case 'X':
                        logger_set_label(-1);
                        logger_set_event_name("unlabeled");
                        ESP_LOGI(TAG, "Set label to unlabeled");
                        break;
                    case 'd':
                    case 'D':
                        ESP_LOGI(TAG, "-> Dumping log file contents:");
                        // Implement log dumping if needed
                        dump_log_file();
                        break;
                    case 'c':
                    case 'C':
                        ESP_LOGI(TAG, "-> Clearing log file");
                        // Implement log clearing if needed
                        clear_log_file();
                        break;
                    case 's':
                    case 'S':
                        ESP_LOGI(TAG, "-> Status");
                        show_log_stats();
                        break;
                    
                    case '\n':
                    case '\r':
                        // Ignore newlines
                        break;

                    default:
                        ESP_LOGW(TAG, "Unknown command: %c", cmd);
                }

            }
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Sleep to avoid busy loop
    }
}

esp_err_t serial_handler_init(void)
{
    BaseType_t ret = xTaskCreate(serial_task, "serial_task", 2048, NULL, 2, NULL);

    
    
    return (ret == pdPASS) ? ESP_OK : ESP_ERR_NO_MEM;
}