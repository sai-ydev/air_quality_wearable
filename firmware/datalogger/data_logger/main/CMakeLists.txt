idf_component_register(SRCS "main.c"
                    "sensor_task.c"
                    "i2c_scanner.c"
                    "sensor_test.c"
                    "bme690_reader.c"
                    "logger_task.c"
                    "serial_handler.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver esp_timer bme68x_bsec zmod4510_idf spiffs
)
