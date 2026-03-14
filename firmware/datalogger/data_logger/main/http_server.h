#pragma once

#include "esp_err.h"

// start HTTP server for CSV download and stats
esp_err_t http_server_init(void);

// Get server status
void http_server_print_info(void);