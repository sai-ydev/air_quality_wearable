/*****************************************************************************
 * Copyright (c) 2024 Renesas Electronics Corporation
 * All Rights Reserved.
 * 
 * This code is proprietary to Renesas, and is license pursuant to the terms and
 * conditions that may be accessed at:
 * https://www.renesas.com/eu/en/document/msc/renesas-software-license-terms-gas-sensor-software
 *****************************************************************************/

/**
 * @file    example.c
 * @brief   This is an example for the ZMOD4510 gas sensor to measure NO2 and O3.
 * @version 1.0.1
 * @author  Renesas Electronics Corporation
 **/

#include "zmod4xxx.h"
#include "zmod4xxx_hal.h"
#include "zmod4xxx_cleaning.h"
#include "hsxxxx.h"
#include "no2_o3.h"
#include "zmod4510_config_no2_o3.h"

static int          ret;
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

int main() {
    /* ********************* HARDWARE SPECIFIC FUNCTION *********************
     *
     * HAL Initialization - provide implementations of I2C communication,
     * delay and other hardware specific functionality.
     * 
     * Porting the example to customer specific hardware requires provision 
     * of a custom HAL layer implementation. HAL_Init is part of this. 
     * Please refer to the Firmware documentation for further information.
     * 
     * All sensor API calls in this example will use the HAL implementation
     * to communicate with the sensor.  */
    ret = HAL_Init(&hal);
    if (ret) {
        HAL_HandleError(ret, "Hardware initialization");
    }
    
    printf("##### Setting up ZMOD4510 sensor #####\n");
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

    printf("\n##### Setting up HSxxxx sensor #####\n");

    /* Try finding a Renesas temperature and humidity sensor - example will work 
     * without it, but results will be less accurate. */
    ret = HSxxxx_Init(&hsxxxx, &hal);
    if ( ret ) {
        printf("No temperature/humidity sensor found, using on-chip");
        printf(" temperature sensor and 50%% relative humidity!\n\n");
    }
    else {
        htSensor = &hsxxxx;
        printf("Found %s humdity & temperature sensor\n\n", HSxxxx_Name ( htSensor ));
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

    while ( 1 ) {
        /* If a sensor was detected, read temperature and humidity from it.
         * Errors occuring during read are ignored: In case of error the 
         * HSxxx API leaves data in the result data structure unmodified. */
        if (htSensor) {
            HSxxxx_Measure(htSensor, &htResults);
        }
        
        /* Start a measurement. */
        ret = zmod4xxx_start_measurement(&dev);
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
    
        printf("*********** Measurements ***********\n");
        for (int i = 0; i < 4; i++) {
            printf("  Rmox[%d]     = %8.3f kOhm\n", i, algo_results.rmox[i]/1e3);
        }
        printf("  O3_conc     = %8.3f ppb\n", algo_results.O3_conc_ppb);
        printf("  NO2_conc    = %8.3f ppb\n", algo_results.NO2_conc_ppb);
        printf("  Fast AQI    = %8i\n", algo_results.FAST_AQI);
        printf("  EPA AQI     = %8i\n", algo_results.EPA_AQI);
        if ( htSensor ) {
            printf("  Temperature = %8.2f degC\n", htResults.temperature);
            printf("  Humidity    = %8.2f %%rH\n", htResults.humidity);
        }
    
        /* Check validity of the algorithm results. */
        switch (ret) {
        case NO2_O3_STABILIZATION:
            /* The sensor should run for at least 50 cycles to stabilize.
             * Algorithm results obtained during this period SHOULD NOT be
             * considered as valid outputs! */
            printf("Warm-Up!\n");
            break;
        case NO2_O3_OK:
            printf("Valid\n");
            break;
        /* Notification from Sensor self-check. For more information, read the
         * Datasheet, section "Conditioning, Sensor Self-Check Status, and 
         * Stability". */
        case NO2_O3_DAMAGE:
            printf("Error: Sensor probably damaged. Algorithm results may be incorrect.\n");
            break;
        /* Exit program due to unexpected error. */
        default:
            HAL_HandleError(ret, "Algorithm calculation");
        }
    }
    return 0;
}

/* This function is used to detect and configure a gas sensor.
 * In addition, the cleaning procedure is executed if required (this is 
 * just required once in sensor lifetime) */
int detect_and_configure(zmod4xxx_dev_t* sensor, int pd_len, char const** errContext) {
    uint8_t  track_number[ZMOD4XXX_LEN_TRACKING];

    ret = zmod4xxx_init(sensor, &hal);
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
    printf("Sensor tracking number: x0000");
    for (int i = 0; i < sizeof(track_number); i++) {
        printf("%02X", track_number[i]);
    }
    printf("\n");
    printf("Sensor trimming data:");
    for (int i = 0; i < pd_len; i++) {
        printf(" %i", prod_data[i]);
    }
    printf("\n");

  
    /* Start the cleaning procedure. Check the Datasheet on indications
     * of usage. IMPORTANT NOTE: The cleaning procedure can be run only once
     * during the modules lifetime and takes 1 minute (blocking). */
    printf("Starting cleaning procedure. This might take up to 1 min ...\n");;
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
    ret = zmod4xxx_read_status(sensor, &zmod4xxx_status);
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
