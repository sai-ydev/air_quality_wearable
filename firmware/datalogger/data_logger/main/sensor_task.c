#include "sensor_task.h"
#include "i2c_scanner.h"
#include "sensor_test.h"
#include "bme690_reader.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "zmod4xxx.h"
#include "zmod4xxx_hal.h"
#include "zmod4xxx_cleaning.h"
#include "hsxxxx.h"
#include "no2_o3.h"
#include "zmod4510_config_no2_o3.h"

static const char *TAG = "sensor_task";

QueueHandle_t s_reading_queue = NULL;
SemaphoreHandle_t s_i2c_mutex = NULL;

i2c_master_bus_handle_t s_i2c_bus = NULL;


// Declare the BSEC loop function
extern void bme690_process_loop(void* arg);

static Interface_t  hal;
static char const*  errContext;

/* Gas sensor related declarations */
static zmod4xxx_dev_t dev;
static uint8_t zmod4xxx_status;
static uint8_t adc_result[ZMOD4510_ADC_DATA_LEN];
static uint8_t prod_data[ZMOD4510_PROD_DATA_LEN];

/* Algorithm related declarations */
static no2_o3_handle_t  algo_handle;
static no2_o3_results_t algo_results;
static no2_o3_inputs_t  algo_input;

/* Temperature and humidity sensor related declarations */
static HSxxxx_t          hsxxxx;
static HSxxxx_t*         htSensor = NULL;  /* pointer will be set if HSxxxx is found */
static HSxxxx_Results_t  htResults;

int  detect_and_configure(zmod4xxx_dev_t*, int, char const**);
void read_and_verify(zmod4xxx_dev_t*, uint8_t*, char const*);

static void sensor_poll_task(void *arg)
{
    ESP_LOGI(TAG, "Sensor poll task started");
        
    while (1) {
        /* If a sensor was detected, read temperature and humidity from it.
         * Errors occuring during read are ignored: In case of error the 
         * HSxxx API leaves data in the result data structure unmodified. */
        if (htSensor) {
            HSxxxx_Measure(htSensor, &htResults);
        }
        
        /* Start a measurement. */
        esp_err_t ret = zmod4xxx_start_measurement(&dev);
        if (ret) {
            HAL_HandleError(ret, "Starting measurement");
        }
    
        /* Perform delay. Required to keep proper measurement timing and keep algorithm accuracy.
         * For more information, read the Programming Manual, section
         * "Interrupt Usage and Measurement Timing". */
        dev.delay_ms(ZMOD4510_NO2_O3_SAMPLE_TIME);
        
    
        read_and_verify ( &dev, adc_result, "ZMOD4510" );
        
        /* Assign algorithm inputs: raw sensor data and ambient conditions. */
        algo_input.adc_result       = adc_result;
        algo_input.humidity_pct     = htResults.humidity;
        algo_input.temperature_degc = htResults.temperature;
        
        /* Calculate algorithm results. */
        ret = calc_no2_o3(&algo_handle, &dev, &algo_input, &algo_results);
    
        ESP_LOGI(TAG, "*********** Measurements ***********");
        for (int i = 0; i < 4; i++) {
            ESP_LOGI(TAG, "  Rmox[%d]     = %8.3f kOhm", i, algo_results.rmox[i]/1e3);
        }
        ESP_LOGI(TAG, "  O3_conc     = %8.3f ppb", algo_results.O3_conc_ppb);
        ESP_LOGI(TAG, "  NO2_conc    = %8.3f ppb", algo_results.NO2_conc_ppb);
        ESP_LOGI(TAG, "  Fast AQI    = %8i", algo_results.FAST_AQI);
        ESP_LOGI(TAG, "  EPA AQI     = %8i", algo_results.EPA_AQI);
        if ( htSensor ) {
            ESP_LOGI(TAG, "  Temperature = %8.2f degC", htResults.temperature);
            ESP_LOGI(TAG, "  Humidity    = %8.2f %%rH", htResults.humidity);
        }
    
        /* Check validity of the algorithm results. */
        switch (ret) {
        case NO2_O3_STABILIZATION:
            /* The sensor should run for at least 50 cycles to stabilize.
             * Algorithm results obtained during this period SHOULD NOT be
             * considered as valid outputs! */
            ESP_LOGI(TAG, "Warm-Up!");
            break;
        case NO2_O3_OK:
            ESP_LOGI(TAG, "Valid");
            sensor_reading_t reading = {0};
            reading.timestamp_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);
            
            // ZMOD4510 data
            reading.o3_ppb = algo_results.O3_conc_ppb;
            reading.no2_ppb = algo_results.NO2_conc_ppb;
            reading.oaq_index = (float)algo_results.FAST_AQI;
            reading.rmox_0 = algo_results.rmox[0];
            reading.rmox_1 = algo_results.rmox[1];
            reading.rmox_2 = algo_results.rmox[2];
            reading.rmox_3 = algo_results.rmox[3];

            // Default label
            reading.event_label = 0;
            snprintf(reading.event_name, sizeof(reading.event_name), "zmod");
            
            // Send to queue
            if (xQueueSend(s_reading_queue, &reading, 0) != pdTRUE) {
                ESP_LOGW(TAG, "Queue full, dropping ZMOD reading");
            }
            break;
        /* Notification from Sensor self-check. For more information, read the
         * Datasheet, section "Conditioning, Sensor Self-Check Status, and 
         * Stability". */
        case NO2_O3_DAMAGE:
            ESP_LOGI(TAG, "Error: Sensor probably damaged. Algorithm results may be incorrect.");
            break;
        /* Exit program due to unexpected error. */
        default:
            HAL_HandleError(ret, "Algorithm calculation");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));  // Delay before next measurement cycle
    }
}

esp_err_t sensor_task_start(QueueHandle_t reading_queue)
{
    s_reading_queue = reading_queue;

    // Create I2C mutex BEFORE initializing sensors
    s_i2c_mutex = xSemaphoreCreateMutex();
    if (!s_i2c_mutex) {
        ESP_LOGE(TAG, "Failed to create I2C mutex");
        return ESP_ERR_NO_MEM;
    }
    
    // Initialize I2C bus
    esp_err_t ret = i2c_bus_init(&s_i2c_bus);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C init failed");
        return ret;
    }
    
    // Scan and test
    i2c_scan_bus(s_i2c_bus);
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "=== Testing Sensor Communication ===");
    test_bme690_communication(s_i2c_bus);
    test_zmod4510_communication(s_i2c_bus);
    ESP_LOGI(TAG, "====================================");
    ESP_LOGI(TAG, "");


    
    // Initialize BME690 + BSEC2
    ret = bme690_init(s_i2c_bus);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BME690 init failed");
        return ret;
    }

    // CREATE THIS TASK - this is probably what you're missing!
    BaseType_t bsec_task_ret = xTaskCreate(
        bme690_process_loop,  // Function that runs bsec_iot_loop
        "bsec_loop",          // Task name
        8192,                 // Stack size (8KB)
        NULL,                 // No parameters
        6,                    // Priority (higher than sensor_poll which is 5)
        NULL                  // No task handle needed
    );

    if (bsec_task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create BSEC processing task");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "BSEC processing task created");

    hal.handle = s_i2c_bus;  // Pass the I2C bus handle to the HAL

    ret = HAL_Init(&hal);
    if (ret) {
        HAL_HandleError(ret, "Hardware initialization");
    }

    ESP_LOGI(TAG, "##### Setting up ZMOD4510 sensor #####\n");
    /* Initialize sensor data structure: This data is used to find
     * and configure the sensor for the NO2_O3 algorithm */
    dev.i2c_addr = ZMOD4510_I2C_ADDR;
    dev.pid = ZMOD4510_PID;
    dev.init_conf = &zmod_no2_o3_sensor_cfg[INIT];
    dev.meas_conf = &zmod_no2_o3_sensor_cfg[MEASUREMENT];
    dev.prod_data = prod_data;
    
    ret = detect_and_configure(&dev, ZMOD4510_PROD_DATA_LEN, &errContext);
    if (ret) {
        HAL_HandleError ( ret, errContext );
    }
    
    ESP_LOGI(TAG, "\n##### Setting up HSxxxx sensor #####\n");

    /* Try finding a Renesas temperature and humidity sensor - example will work 
     * without it, but results will be less accurate. */
    ret = HSxxxx_Init(&hsxxxx, &hal);
    if ( ret ) {
        ESP_LOGI(TAG, "No temperature/humidity sensor found, using on-chip");
        ESP_LOGI(TAG, " temperature sensor and 50%% relative humidity!\n\n");
    }
    else {
        htSensor = &hsxxxx;
        ESP_LOGI(TAG, "Found %s humdity & temperature sensor\n\n", HSxxxx_Name ( htSensor ));
    }
    /* Set default values for temperature and humidity: These values will be used
     * if no sensor is detected or if reading the sensor fails. 
     * The temperature value of -300°C causes the algo to use the on-chip temperature
     * measurement of the gas sensor. However, an external temperature and humidity
     * sensor provides better accuracy and is the preferred input source. */
    htResults.temperature = -300;
    htResults.humidity    =  50;

    /* One-time initialization of the algorithm. Handle passed to calculation
     * function. */
    ret = init_no2_o3(&algo_handle);
    if (ret) {
        HAL_HandleError(ret, "Algorithm initialization");
    }
    // Start polling task
    BaseType_t task_ret = xTaskCreate(
        sensor_poll_task,
        "sensor_poll",
        4096,  // Increased stack for BSEC2
        NULL,
        5,
        NULL
    );
    
    return (task_ret == pdPASS) ? ESP_OK : ESP_ERR_NO_MEM;
}

/* This function is used to detect and configure a gas sensor.
 * In addition, the cleaning procedure is executed if required (this is 
 * just required once in sensor lifetime) */
int detect_and_configure(zmod4xxx_dev_t* sensor, int pd_len, char const** errContext) {
    uint8_t  track_number[ZMOD4XXX_LEN_TRACKING];

    esp_err_t ret = zmod4xxx_init(sensor, &hal);
    if (ret) {
        *errContext = "sensor initialization";
        return ret;
    }
    
    /* Read product ID and configuration parameters. */
    ret = zmod4xxx_read_sensor_info(sensor);
    if (ret) {
        *errContext = "reading sensor information";
        return ret;
    }
    
    /* Retrieve sensors unique tracking number and individual trimming information.
     * Provide this information when requesting support from Renesas.
     * Otherwise this function is not required for gas sensor operation. */
    ret = zmod4xxx_read_tracking_number(sensor, track_number);
    if (ret) {
        *errContext = "Reading tracking number";
        return ret;
    }
    ESP_LOGI(TAG, "Sensor tracking number: x0000");
    for (int i = 0; i < sizeof(track_number); i++) {
        ESP_LOGI(TAG, "%02X", track_number[i]);
    }
    ESP_LOGI(TAG, "Sensor trimming data:");
    for (int i = 0; i < pd_len; i++) {
        ESP_LOGI(TAG, " %i", prod_data[i]);
    }
    

  
    /* Start the cleaning procedure. Check the Datasheet on indications
     * of usage. IMPORTANT NOTE: The cleaning procedure can be run only once
     * during the modules lifetime and takes 1 minute (blocking). */
    ESP_LOGI(TAG, "Starting cleaning procedure. This might take up to 1 min ...");
    ret = zmod4xxx_cleaning_run(sensor);
    if (ERROR_CLEANING == ret) {
        printf("Skipping cleaning procedure. It has already been performed\n");;
    } else if (ret) {
        *errContext = "sensor cleaning";
        return ret;
    }
    /* Determine calibration parameters and configure measurement. */
    ret = zmod4xxx_prepare_sensor(sensor);
    if (ret) {
        *errContext = "sensor preparation";
        return ret;
    }
    return 0;
}

/* This function read the gas sensor results and checks for result validity. */
void read_and_verify(zmod4xxx_dev_t* sensor, uint8_t* result, char const* id) {
    /* Verify completion of measurement sequence. */
    esp_err_t ret = zmod4xxx_read_status(sensor, &zmod4xxx_status);
    if (ret) {
        HAL_HandleError(ret, "Reading sensor status");
    }
    /* Check if measurement is running. */
    if (zmod4xxx_status & STATUS_SEQUENCER_RUNNING_MASK) {
        /* Check if reset during measurement occured. For more information,
         * read the Programming Manual, section "Error Codes". */
        ret = zmod4xxx_check_error_event(sensor);
        switch (ret) {
        case ERROR_POR_EVENT:
            HAL_HandleError(ret, "Reading result: Unexpected sensor reset!");
            break;
        case ZMOD4XXX_OK:
            HAL_HandleError(ret, "Reading result: Wrong sensor setup!");
            break;
        default:
            HAL_HandleError(ret, "Reading result: Unknown error!");
            break;
        }
    }
    /* Read sensor ADC output. */
    ret = zmod4xxx_read_adc_result(sensor, result);
    if (ret) {
        HAL_HandleError(ret, "Reading ADC results");
    }
    
    /* Check validity of the ADC results. For more information, read the
     * Programming Manual, section "Error Codes". */
    ret = zmod4xxx_check_error_event(sensor);
    if (ret) {
        HAL_HandleError(ret, "Reading sensor status");
    }
}