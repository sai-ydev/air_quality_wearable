#include "http_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include <stdio.h>

static const char *TAG = "http_server";

#define LOG_FILE_PATH "/spiffs/sensor_log.csv"

static httpd_handle_t server = NULL;

// Handler for GET /download
static esp_err_t download_get_handler(httpd_req_t *req) {

    ESP_LOGI(TAG, "Received request for /download");

    FILE *f = fopen(LOG_FILE_PATH, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open log file for reading");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // Set response headers
    httpd_resp_set_type(req, "text/csv");
    httpd_resp_set_hdr(req, "Content-Disposition", "attachment; filename=\"sensor_log.csv\"");

    // Read file and send in chunks
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), f)) {
        if (httpd_resp_sendstr_chunk(req, buffer) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to send log file chunk");
            fclose(f);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
    }
    fclose(f);
    httpd_resp_sendstr_chunk(req, NULL);  // Signal end of response
    ESP_LOGI(TAG, "Finished sending log file");
    return ESP_OK;
}

//HANDLER FOR GET /(root)
static esp_err_t root_handler(httpd_req_t* req){
    const char* html = "<html><head><title>Asthma Guardian Logger</title></head>"
                       "<body><h1>Asthma Guardian Logger</h1>"
                       "<p><a href=\"/download\">Download Sensor Log (CSV)</a></p>"
                       "</body></html>";

    httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t http_server_init(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.max_uri_handlers = 5;  // Allow up to 5 URI handlers


    ESP_LOGI(TAG, "Starting HTTP server on port %d", config.server_port);
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return ESP_FAIL;
    }

    httpd_uri_t root_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &root_uri);

    // Register URI handlers
    httpd_uri_t download_uri = {
        .uri = "/download",
        .method = HTTP_GET,
        .handler = download_get_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &download_uri);

    ESP_LOGI(TAG, "HTTP server started successfully");
    return ESP_OK;
}

void http_server_print_info(void) {
    if(!server){
        ESP_LOGI(TAG, "HTTP server is not running");
        return;
    }

    // Get IP address
    esp_netif_ip_info_t ip_info;
    esp_netif_t*netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
        ESP_LOGI(TAG, "Download URL: http://" IPSTR "/download", IP2STR(&ip_info.ip));
    } else {
        ESP_LOGI(TAG, "HTTP Server IP: Not connected");
    }


}