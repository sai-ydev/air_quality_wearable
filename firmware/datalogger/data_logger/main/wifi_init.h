#pragma once

#include "esp_err.h"

// Initialize WiFi in STA mode
esp_err_t wifi_init_sta(const char *ssid, const char *password);