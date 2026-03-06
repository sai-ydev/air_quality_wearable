#include "serial_handler.h"
#include "logger_task.h"
#include "esp_log.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

static const char *TAG = "serial_handler";

#define UART_PORT_NUM      UART_NUM_0
#define BUFFER_SIZE        256
#define UART_READ_TIMEOUT_MS 100

static void serial_task(void *arg)
{
    ESP_LOGI(TAG, "Serial handler task started");
    ESP_LOGI(TAG, "Commands: 0=LOW, 1=MODERATE, 2=HIGH, x=unlabeled, d=dump, c=clear");

    fcntl(fileno(stdin), F_SETFL, O_NONBLOCK); // Set stdin to non-blocking mode
    
    while (1) {       
        
        char cmd;
        int len = read(fileno(stdin), &cmd, 1);
        if (len == 1){

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
                    ESP_LOGI(TAG, "-> Dumping log file contents: (not implemented yet)");
                    // Implement log dumping if needed
                    break;
                case 'c':
                case 'C':
                    ESP_LOGI(TAG, "-> Clearing log file (not implemented yet)");
                    // Implement log clearing if needed
                    break;
                case 's':
                case 'S':
                    ESP_LOGI(TAG, "-> Status");
                    break;
                
                case '\n':
                case '\r':
                    // Ignore newlines
                    break;

                default:
                    ESP_LOGW(TAG, "Unknown command: %c", cmd);
            }

        }
        vTaskDelay(pdMS_TO_TICKS(UART_READ_TIMEOUT_MS));     
    }
}

esp_err_t serial_handler_init(void)
{
    
    
    BaseType_t ret = xTaskCreate(serial_task, "serial_task", 2048, NULL, 2, NULL);
    
    return (ret == pdPASS) ? ESP_OK : ESP_ERR_NO_MEM;
}