/**
 * Copyright (C) Bosch Sensortec GmbH. All Rights Reserved. Confidential.
 *
 * Disclaimer
 *
 * Common:
 * Bosch Sensortec products are developed for the consumer goods industry. They may only be used
 * within the parameters of the respective valid product data sheet. Bosch Sensortec products are
 * provided with the express understanding that there is no warranty of fitness for a particular purpose.
 * They are not fit for use in life-sustaining, safety or security sensitive systems or any system or device
 * that may lead to bodily harm or property damage if the system or device malfunctions. In addition,
 * Bosch Sensortec products are not fit for use in products which interact with motor vehicle systems.
 * The resale and/or use of products are at the purchaser's own risk and his own responsibility. The
 * examination of fitness for the intended use is the sole responsibility of the Purchaser.
 *
 * The purchaser shall indemnify Bosch Sensortec from all third party claims, including any claims for
 * incidental, or consequential damages, arising from any product use not covered by the parameters of
 * the respective valid product data sheet or not approved by Bosch Sensortec and reimburse Bosch
 * Sensortec for all costs in connection with such claims.
 *
 * The purchaser must monitor the market for the purchased products, particularly with regard to
 * product safety and inform Bosch Sensortec without delay of all security relevant incidents.
 *
 * Engineering Samples are marked with an asterisk (*) or (e). Samples may vary from the valid
 * technical specifications of the product series. They are therefore not intended or fit for resale to third
 * parties or for use in end products. Their sole purpose is internal client testing. The testing of an
 * engineering sample may in no way replace the testing of a product series. Bosch Sensortec
 * assumes no liability for the use of engineering samples. By accepting the engineering samples, the
 * Purchaser agrees to indemnify Bosch Sensortec from all claims arising from the use of engineering
 * samples.
 *
 * Special:
 * This software module (hereinafter called "Software") and any information on application-sheets
 * (hereinafter called "Information") is provided free of charge for the sole purpose to support your
 * application work. The Software and Information is subject to the following terms and conditions:
 *
 * The Software is specifically designed for the exclusive use for Bosch Sensortec products by
 * personnel who have special experience and training. Do not use this Software if you do not have the
 * proper experience or training.
 *
 * This Software package is provided `` as is `` and without any expressed or implied warranties,
 * including without limitation, the implied warranties of merchantability and fitness for a particular
 * purpose.
 *
 * Bosch Sensortec and their representatives and agents deny any liability for the functional impairment
 * of this Software in terms of fitness, performance and safety. Bosch Sensortec and their
 * representatives and agents shall not be liable for any direct or indirect damages or injury, except as
 * otherwise stipulated in mandatory applicable law.
 *
 * The Information provided is believed to be accurate and reliable. Bosch Sensortec assumes no
 * responsibility for the consequences of use of such Information nor for any infringement of patents or
 * other rights of third parties which may result from its use. No license is granted by implication or
 * otherwise under any patent or patent rights of Bosch. Specifications mentioned in the Information are
 * subject to change without notice.
 *
 * It is not allowed to deliver the source code of the Software to any third party without permission of
 * Bosch Sensortec.
 *
 */

/*!
 * @file bsec_integration.c
 *
 * @brief
 * Private part of the example for using of BSEC library.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "bsec_integration.h"
#include "esp_task_wdt.h"
#include "driver/i2c_master.h"

static struct bme69x_conf bme69x_config[NUM_OF_SENS];
static struct bme69x_dev bme69x[NUM_OF_SENS];
static struct bme69x_heatr_conf bme69x_heater_config[NUM_OF_SENS];
static struct bme69x_data sensor_data[3];
extern i2c_master_bus_handle_t s_i2c_bus;

uint8_t lastOpMode[NUM_OF_SENS] =  {0};
uint8_t opMode[NUM_OF_SENS] = {0};
uint8_t nFields, iFields;
uint8_t n_sensors = NUM_OF_SENS;

uint8_t	bsec_mem_block[NUM_OF_SENS][BSEC_INSTANCE_SIZE];
uint8_t *bsecInstance[NUM_OF_SENS];

float extTempOffset = 0.0f;

static bsec_library_return_t update_subscription(float sample_rate, uint8_t sens_no)
{
    bsec_library_return_t status;
    bsec_sensor_configuration_t requested_virtual_sensors[NUM_USED_OUTPUTS];
    uint8_t n_requested_virtual_sensors = NUM_USED_OUTPUTS;
    
    bsec_sensor_configuration_t required_sensor_settings[BSEC_MAX_PHYSICAL_SENSOR];
    uint8_t n_required_sensor_settings = BSEC_MAX_PHYSICAL_SENSOR;
    
	requested_virtual_sensors[0].sensor_id = BSEC_OUTPUT_RAW_PRESSURE;
    requested_virtual_sensors[0].sample_rate = sample_rate;
    requested_virtual_sensors[1].sensor_id = BSEC_OUTPUT_RAW_TEMPERATURE;
    requested_virtual_sensors[1].sample_rate = sample_rate;
    requested_virtual_sensors[2].sensor_id = BSEC_OUTPUT_RAW_HUMIDITY;
    requested_virtual_sensors[2].sample_rate = sample_rate;
    requested_virtual_sensors[3].sensor_id = BSEC_OUTPUT_RAW_GAS;
    requested_virtual_sensors[3].sample_rate = sample_rate;
#if (OUTPUT_MODE == CLASSIFICATION)
    requested_virtual_sensors[4].sensor_id = BSEC_OUTPUT_GAS_ESTIMATE_1;
    requested_virtual_sensors[4].sample_rate = sample_rate;
    requested_virtual_sensors[5].sensor_id = BSEC_OUTPUT_GAS_ESTIMATE_2;
    requested_virtual_sensors[5].sample_rate = sample_rate;
    requested_virtual_sensors[6].sensor_id = BSEC_OUTPUT_GAS_ESTIMATE_3;
    requested_virtual_sensors[6].sample_rate = sample_rate;
    requested_virtual_sensors[7].sensor_id = BSEC_OUTPUT_GAS_ESTIMATE_4;
    requested_virtual_sensors[7].sample_rate = sample_rate;
    requested_virtual_sensors[8].sensor_id = BSEC_OUTPUT_RAW_GAS_INDEX;
    requested_virtual_sensors[8].sample_rate = sample_rate;
#elif (OUTPUT_MODE == REGRESSION)
    requested_virtual_sensors[4].sensor_id = BSEC_OUTPUT_REGRESSION_ESTIMATE_1;
    requested_virtual_sensors[4].sample_rate = sample_rate;
    requested_virtual_sensors[5].sensor_id = BSEC_OUTPUT_REGRESSION_ESTIMATE_2;
    requested_virtual_sensors[5].sample_rate = sample_rate;
    requested_virtual_sensors[6].sensor_id = BSEC_OUTPUT_REGRESSION_ESTIMATE_3;
    requested_virtual_sensors[6].sample_rate = sample_rate;
    requested_virtual_sensors[7].sensor_id = BSEC_OUTPUT_REGRESSION_ESTIMATE_4;
    requested_virtual_sensors[7].sample_rate = sample_rate;
    requested_virtual_sensors[8].sensor_id = BSEC_OUTPUT_RAW_GAS_INDEX;
    requested_virtual_sensors[8].sample_rate = sample_rate;
#elif (OUTPUT_MODE == IAQ)
	requested_virtual_sensors[4].sensor_id = BSEC_OUTPUT_IAQ;
    requested_virtual_sensors[4].sample_rate = sample_rate;
    requested_virtual_sensors[5].sensor_id = BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE;
    requested_virtual_sensors[5].sample_rate = sample_rate;
    requested_virtual_sensors[6].sensor_id = BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY;
    requested_virtual_sensors[6].sample_rate = sample_rate;
    requested_virtual_sensors[7].sensor_id = BSEC_OUTPUT_STATIC_IAQ;
    requested_virtual_sensors[7].sample_rate = sample_rate;
    requested_virtual_sensors[8].sensor_id = BSEC_OUTPUT_CO2_EQUIVALENT;
    requested_virtual_sensors[8].sample_rate = sample_rate;
    requested_virtual_sensors[9].sensor_id = BSEC_OUTPUT_BREATH_VOC_EQUIVALENT;
    requested_virtual_sensors[9].sample_rate = sample_rate;
    requested_virtual_sensors[10].sensor_id = BSEC_OUTPUT_STABILIZATION_STATUS;
    requested_virtual_sensors[10].sample_rate = sample_rate;
    requested_virtual_sensors[11].sensor_id = BSEC_OUTPUT_RUN_IN_STATUS;
    requested_virtual_sensors[11].sample_rate = sample_rate;
    requested_virtual_sensors[12].sensor_id = BSEC_OUTPUT_GAS_PERCENTAGE;
    requested_virtual_sensors[12].sample_rate = sample_rate;
#endif
    
    /* To enable the requested virtual sensors */
    status = bsec_update_subscription(bsecInstance[sens_no], requested_virtual_sensors, n_requested_virtual_sensors, 
                                            required_sensor_settings, &n_required_sensor_settings);
    
    return status;
}

static void bsec_new_data(uint8_t sens_no, bsec_output_t *bsec_outputs, uint8_t num_bsec_outputs, output_ready_fct output_ready, bsec_library_return_t bsec_status)
{
    if (num_bsec_outputs == 0)
        return;
    
    outputs_t output;
    memset(&output, 0, sizeof(output));

    for (uint8_t id = 0; id < num_bsec_outputs; id++)
    {
        switch (bsec_outputs[id].sensor_id)
        {
            case BSEC_OUTPUT_RAW_PRESSURE:
                output.raw_pressure = bsec_outputs[id].signal;
                break;
            case BSEC_OUTPUT_RAW_TEMPERATURE:
                output.raw_temp = bsec_outputs[id].signal;
                break;
            case BSEC_OUTPUT_RAW_HUMIDITY:
                output.raw_humidity = bsec_outputs[id].signal;
                break;
            case BSEC_OUTPUT_RAW_GAS:
                output.raw_gas = bsec_outputs[id].signal;
                break;
            case BSEC_OUTPUT_IAQ:
                output.iaq = bsec_outputs[id].signal;
                output.iaq_accuracy = bsec_outputs[id].accuracy;
                break;
            case BSEC_OUTPUT_STATIC_IAQ:
                output.static_iaq = bsec_outputs[id].signal;
                break;
            case BSEC_OUTPUT_CO2_EQUIVALENT:
                output.co2_equivalent = bsec_outputs[id].signal;
                break;
            case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:
                output.breath_voc_equivalent = bsec_outputs[id].signal;
                break;
            case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
                output.compensated_temperature = bsec_outputs[id].signal;
                break;
            case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
                output.compensated_humidity = bsec_outputs[id].signal;
                break;
            case BSEC_OUTPUT_STABILIZATION_STATUS:
                output.stabStatus = bsec_outputs[id].signal;
                break;
            case BSEC_OUTPUT_RUN_IN_STATUS:
                output.runInStatus = bsec_outputs[id].signal;
                break;
            case BSEC_OUTPUT_GAS_PERCENTAGE:
                output.gas_percentage = bsec_outputs[id].signal;
                break;
#if (OUTPUT_MODE == CLASSIFICATION || OUTPUT_MODE == REGRESSION)
            case BSEC_OUTPUT_GAS_ESTIMATE_1:
                output.gas_estimate_1 = bsec_outputs[id].signal;
                output.gas_accuracy_1 = bsec_outputs[id].accuracy;
                break;
            case BSEC_OUTPUT_GAS_ESTIMATE_2:
                output.gas_estimate_2 = bsec_outputs[id].signal;
                output.gas_accuracy_2 = bsec_outputs[id].accuracy;
                break;
            case BSEC_OUTPUT_GAS_ESTIMATE_3:
                output.gas_estimate_3 = bsec_outputs[id].signal;
                output.gas_accuracy_3 = bsec_outputs[id].accuracy;
                break;
            case BSEC_OUTPUT_GAS_ESTIMATE_4:
                output.gas_estimate_4 = bsec_outputs[id].signal;
                output.gas_accuracy_4 = bsec_outputs[id].accuracy;
                break;
            case BSEC_OUTPUT_REGRESSION_ESTIMATE_1:
                output.gas_estimate_1 = bsec_outputs[id].signal;
                output.gas_accuracy_1 = bsec_outputs[id].accuracy;
                break;
            case BSEC_OUTPUT_REGRESSION_ESTIMATE_2:
                output.gas_estimate_2 = bsec_outputs[id].signal;
                output.gas_accuracy_2 = bsec_outputs[id].accuracy;
                break;
            case BSEC_OUTPUT_REGRESSION_ESTIMATE_3:
                output.gas_estimate_3 = bsec_outputs[id].signal;
                output.gas_accuracy_3 = bsec_outputs[id].accuracy;
                break;
            case BSEC_OUTPUT_REGRESSION_ESTIMATE_4:
                output.gas_estimate_4 = bsec_outputs[id].signal;
                output.gas_accuracy_4 = bsec_outputs[id].accuracy;
                break;
            case BSEC_OUTPUT_RAW_GAS_INDEX:
                output.raw_gas_index = (uint8_t)bsec_outputs[id].signal;
                break;
#endif
            default:
                continue;
        }
        output.timestamp = bsec_outputs[id].time_stamp;
    }

    char msg[300] = {0};

#if (OUTPUT_MODE == IAQ)
    sprintf(msg,"%u,%ld,%f,%u,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%d\r\n",sens_no, (uint32_t)(output.timestamp/1000000),
                output.iaq, output.iaq_accuracy, output.static_iaq, output.raw_temp, output.raw_humidity, output.compensated_temperature, 
                output.compensated_humidity, output.raw_pressure, output.raw_gas, output.gas_percentage, output.co2_equivalent,
                output.breath_voc_equivalent, output.stabStatus, output.runInStatus, bsec_status);
#else
    sprintf(msg,"%u,%ld,%f,%f,%f,%f,%u,%u,%u,%u,%f,%f,%f,%f,%u,%d\r\n",sens_no, (uint32_t)(output.timestamp/1000000),
                output.gas_estimate_1, output.gas_estimate_2, output.gas_estimate_3, output.gas_estimate_4,
                output.gas_accuracy_1, output.gas_accuracy_2, output.gas_accuracy_3, output.gas_accuracy_4,
                output.raw_pressure, output.raw_temp, output.raw_humidity, output.raw_gas, output.raw_gas_index, bsec_status);
#endif
    output_ready(msg);
}

static uint8_t process_data(int64_t currTimeNs, struct bme69x_data data, int32_t bsec_process_data, uint8_t sens_no, output_ready_fct output_ready)
{
    bsec_input_t inputs[BSEC_MAX_PHYSICAL_SENSOR] = {0}; /* Temp, Pres, Hum & Gas */
	bsec_library_return_t bsec_status = BSEC_OK;
    uint8_t nInputs = 0;

    /* Checks all the required sensor inputs, required for the BSEC library for the requested outputs */
    if (BSEC_CHECK_INPUT(bsec_process_data, BSEC_INPUT_HEATSOURCE))
    {
        inputs[nInputs].sensor_id = BSEC_INPUT_HEATSOURCE;
        inputs[nInputs].signal = extTempOffset;
        inputs[nInputs].time_stamp = currTimeNs;
        nInputs++;
    }
    if (BSEC_CHECK_INPUT(bsec_process_data, BSEC_INPUT_TEMPERATURE))
    {
#ifdef BME69X_USE_FPU
        inputs[nInputs].signal = data.temperature;
#else
        inputs[nInputs].signal = data.temperature / 100.0f;
#endif
        inputs[nInputs].sensor_id = BSEC_INPUT_TEMPERATURE;
        inputs[nInputs].time_stamp = currTimeNs;
        nInputs++;
    }
    if (BSEC_CHECK_INPUT(bsec_process_data, BSEC_INPUT_HUMIDITY))
    {
#ifdef BME69X_USE_FPU
        inputs[nInputs].signal = data.humidity;
#else
        inputs[nInputs].signal = data.humidity / 1000.0f;
#endif
        inputs[nInputs].sensor_id = BSEC_INPUT_HUMIDITY;
        inputs[nInputs].time_stamp = currTimeNs;
        nInputs++;
    }
    if (BSEC_CHECK_INPUT(bsec_process_data, BSEC_INPUT_PRESSURE))
    {
        inputs[nInputs].sensor_id = BSEC_INPUT_PRESSURE;
        inputs[nInputs].signal = data.pressure;
        inputs[nInputs].time_stamp = currTimeNs;
        nInputs++;
    }
    if (BSEC_CHECK_INPUT(bsec_process_data, BSEC_INPUT_GASRESISTOR) &&
            (data.status & BME69X_GASM_VALID_MSK))
    {
        inputs[nInputs].sensor_id = BSEC_INPUT_GASRESISTOR;
        inputs[nInputs].signal = data.gas_resistance;
        inputs[nInputs].time_stamp = currTimeNs;
        nInputs++;
    }
    if (BSEC_CHECK_INPUT(bsec_process_data, BSEC_INPUT_PROFILE_PART) &&
            (data.status & BME69X_GASM_VALID_MSK))
    {
        inputs[nInputs].sensor_id = BSEC_INPUT_PROFILE_PART;
        inputs[nInputs].signal = (opMode[sens_no] == BME69X_FORCED_MODE) ? 0 : data.gas_index;
        inputs[nInputs].time_stamp = currTimeNs;
        nInputs++;
    }

    if (nInputs > 0)
    {
        bsec_output_t bsec_outputs[BSEC_NUMBER_OUTPUTS];
        uint8_t num_bsec_outputs = BSEC_NUMBER_OUTPUTS;

        /* Perform processing of the data by BSEC 
           Note:
           * The number of outputs you get depends on what you asked for during bsec_update_subscription(). This is
             handled under bsec_update_subscription() function in this example file.
           * The number of actual outputs that are returned is written to num_bsec_outputs. */
        bsec_status = bsec_do_steps(bsecInstance[sens_no], inputs, nInputs, bsec_outputs, &num_bsec_outputs);
		
        bsec_new_data(sens_no, bsec_outputs, num_bsec_outputs, output_ready, bsec_status);

        if (bsec_status != BSEC_OK)
            return 0;
    }
    return 1;
}

static uint32_t get_measure_duration(uint8_t mode, uint8_t sens_no)
{
	if (mode == BME69X_SLEEP_MODE)
		mode = lastOpMode[sens_no];

	return bme69x_get_meas_dur(mode, &bme69x_config[sens_no], &bme69x[sens_no]);
}

static void set_bme69x_parallel(bsec_bme_settings_t *sensor_settings, uint8_t sens_no)
{
    int8_t status = BME69X_OK;
    uint16_t sharedHeaterDur = 0;
    
	/* Set the filter, odr, temperature, pressure and humidity settings */
	status = bme69x_get_conf(&bme69x_config[sens_no], &bme69x[sens_no]);

	if (status != BME69X_OK)
		return;

	bme69x_config[sens_no].os_hum = sensor_settings->humidity_oversampling;
	bme69x_config[sens_no].os_temp = sensor_settings->temperature_oversampling;
	bme69x_config[sens_no].os_pres = sensor_settings->pressure_oversampling;
	status = bme69x_set_conf(&bme69x_config[sens_no], &bme69x[sens_no]);

    if (status != BME69X_OK)
		return;
        

    sharedHeaterDur = BSEC_TOTAL_HEAT_DUR - (get_measure_duration(BME69X_PARALLEL_MODE, sens_no) / INT64_C(1000));
	bme69x_heater_config[sens_no].enable = BME69X_ENABLE;
	bme69x_heater_config[sens_no].heatr_temp_prof = sensor_settings->heater_temperature_profile;
	bme69x_heater_config[sens_no].heatr_dur_prof = sensor_settings->heater_duration_profile;
	bme69x_heater_config[sens_no].shared_heatr_dur = sharedHeaterDur;
	bme69x_heater_config[sens_no].profile_len = sensor_settings->heater_profile_len;
    status = bme69x_set_heatr_conf(BME69X_PARALLEL_MODE, &bme69x_heater_config[sens_no], &bme69x[sens_no]);

    if (status != BME69X_OK)
		return;

    status = bme69x_set_op_mode(BME69X_PARALLEL_MODE, &bme69x[sens_no]);

	if (status != BME69X_OK)
		return;
		
	lastOpMode[sens_no] = BME69X_PARALLEL_MODE;
    opMode[sens_no] = BME69X_PARALLEL_MODE;
}

static void set_bme69x_forced_mode(bsec_bme_settings_t *sensor_settings, uint8_t sens_no)
{
    int8_t status = BME69X_OK;

    status = bme69x_set_op_mode(BME69X_SLEEP_MODE, &bme69x[sens_no]);

    /* Set the filter, odr, temperature, pressure and humidity settings */
	status = bme69x_get_conf(&bme69x_config[sens_no], &bme69x[sens_no]);

    if (status != BME69X_OK)
		return;

	bme69x_config[sens_no].os_hum = sensor_settings->humidity_oversampling;
	bme69x_config[sens_no].os_temp = sensor_settings->temperature_oversampling;
	bme69x_config[sens_no].os_pres = sensor_settings->pressure_oversampling;
	status = bme69x_set_conf(&bme69x_config[sens_no], &bme69x[sens_no]);

    if (status != BME69X_OK)
		return;

    bme69x_heater_config[sens_no].enable = BME69X_ENABLE;
	bme69x_heater_config[sens_no].heatr_temp = sensor_settings->heater_temperature;
	bme69x_heater_config[sens_no].heatr_dur = sensor_settings->heater_duration;
	status = bme69x_set_heatr_conf(BME69X_FORCED_MODE, &bme69x_heater_config[sens_no], &bme69x[sens_no]);

    if (status != BME69X_OK)
		return;

    status = bme69x_set_op_mode(BME69X_FORCED_MODE, &bme69x[sens_no]);

	if (status != BME69X_OK)
		return;

	lastOpMode[sens_no] = BME69X_FORCED_MODE;
    opMode[sens_no] = BME69X_FORCED_MODE;
}

static uint8_t get_data(struct bme69x_data *data, uint8_t sens_no)
{
	if (lastOpMode[sens_no] == BME69X_FORCED_MODE)
	{
		*data = sensor_data[0];
	} else
	{
		if (nFields)
		{
			/* iFields spans from 0-2 while nFields spans from
			 * 0-3, where 0 means that there is no new data
			 */
			*data = sensor_data[iFields];
			iFields++;

			/* Limit reading continuously to the last fields read */
			if (iFields >= nFields)
			{
				iFields = nFields - 1;
				return 0;
			}

			/* Indicate if there is something left to read */
			return nFields - iFields;
		}
	}

	return 0;
}

return_values_init bsec_iot_init(float sample_rate, bme69x_interface_fct bme69x_intf_init, state_load_fct state_load, config_load_fct config_load)
{
    return_values_init ret = {BME69X_OK, BSEC_OK};
    static uint8_t bsec_config[BSEC_MAX_PROPERTY_BLOB_SIZE] = {0};
    static uint8_t work_buffer[BSEC_MAX_WORKBUFFER_SIZE] = {0};
    static uint8_t bsec_state[BSEC_MAX_STATE_BLOB_SIZE] = {0};
    uint32_t bsec_state_len;

    /*
	 *	The default offset provided has been determined by testing the sensor in LP and ULP mode on application board 3.0
	 *	Please update the offset value after testing this on your product 
	 */
    extTempOffset = (sample_rate == BSEC_SAMPLE_RATE_LP) ? TEMP_OFFSET_LP :
                    (sample_rate == BSEC_SAMPLE_RATE_ULP) ? TEMP_OFFSET_ULP : 0;

    for (uint8_t sens_no = 0; sens_no < n_sensors; sens_no++)
    {
        memset(work_buffer, 0, sizeof(work_buffer));
        memset(bsec_state, 0, sizeof(bsec_state));
        memset(bsec_config, 0, sizeof(bsec_config));
        memset(&bme69x[sens_no], 0, sizeof(bme69x[sens_no]));

        // sensor communication
        bme69x_intf_init(&bme69x[sens_no], BME69X_I2C_INTF, s_i2c_bus);

        // bme69x initialization
        ret.bme69x_status = bme69x_init(&bme69x[sens_no]);

        // assign the bsec instance memory
        allocate_memory(bsec_mem_block[sens_no], sens_no);
        
        /* Initialize BSEC library */
        ret.bsec_status = bsec_init(bsecInstance[sens_no]);

        /* Set the configuration */
        /* Load library config, if available */
        int32_t bsec_config_len;
        bsec_config_len = config_load(bsec_config, sizeof(bsec_config));
        if (bsec_config_len != 0) {  
            ret.bsec_status = bsec_set_configuration(bsecInstance[sens_no], bsec_config, BSEC_MAX_PROPERTY_BLOB_SIZE, work_buffer, sizeof(work_buffer));
        }

        /* Load previous library state, if available */
        bsec_state_len = state_load(bsec_state, sizeof(bsec_state));
        
        if (bsec_state_len != 0)
        {     
            ret.bsec_status = bsec_set_state(bsecInstance[sens_no], bsec_state, bsec_state_len, work_buffer, sizeof(work_buffer));
        }

        /* Call to the function which sets the library with subscription information */
        ret.bsec_status = update_subscription(SAMPLE_RATE, sens_no);
    }

    return ret;
}

void bsec_iot_loop(state_save_fct state_save, get_timestamp_ms_fct get_timestamp_ms, output_ready_fct output_ready)
{
    static uint8_t work_buffer[BSEC_MAX_WORKBUFFER_SIZE] = {0};
    static uint8_t bsec_state[BSEC_MAX_STATE_BLOB_SIZE] = {0};
    uint32_t bsec_state_len;
    uint64_t time_stamp = 0;
    
    /* BSEC sensor settings struct */
    bsec_bme_settings_t sensor_settings[NUM_OF_SENS];

    bsec_library_return_t status;
    int8_t bme69x_status;

    /* Save state variables */
    uint32_t n_samples = 0;

	for (uint8_t i = 0; i < n_sensors; i++)
    {
		memset(&sensor_settings[i], 0, sizeof(sensor_settings[i]));
		opMode[i] = sensor_settings[i].op_mode;
		sensor_settings[i].next_call = 0;
	}
	
    while (1)
    {
       
        for (uint8_t sens_no = 0; sens_no < n_sensors; sens_no++)
        {
            uint8_t nFieldsLeft = 0;
	        struct bme69x_data data = {0};
            opMode[sens_no] = sensor_settings[sens_no].op_mode;

		    time_stamp = get_timestamp_ms() * INT64_C(1000000);
			if (time_stamp >= (uint64_t)sensor_settings[sens_no].next_call)
			{
				/* Retrieve sensor settings to be used in this time instant by calling bsec_sensor_control */
				status = bsec_sensor_control(bsecInstance[sens_no], time_stamp, &sensor_settings[sens_no]);
				
				switch (sensor_settings[sens_no].op_mode)
				{
					case BME69X_FORCED_MODE:
						set_bme69x_forced_mode(&sensor_settings[sens_no], sens_no);
						break;
					case BME69X_PARALLEL_MODE:
						if (opMode[sens_no] != sensor_settings[sens_no].op_mode)
						{
							set_bme69x_parallel(&sensor_settings[sens_no], sens_no);
						}
						break;
					case BME69X_SLEEP_MODE:
						if (opMode[sens_no] != sensor_settings[sens_no].op_mode)
						{
							bme69x_status = bme69x_set_op_mode(BME69X_SLEEP_MODE, &bme69x[sens_no]);

                            if ((bme69x_status == BME69X_OK) && (opMode[sens_no] != BME69X_SLEEP_MODE))
							{
								opMode[sens_no] = BME69X_SLEEP_MODE;
							}
						}
						break;
                    default: break;
				}

				if (sensor_settings[sens_no].trigger_measurement && sensor_settings[sens_no].op_mode != BME69X_SLEEP_MODE)
				{
					nFields = 0;
					bme69x_status = bme69x_get_data(lastOpMode[sens_no], &sensor_data[0], &nFields, &bme69x[sens_no]);
					iFields = 0;

					if(nFields)
					{
						do
						{
							nFieldsLeft = get_data(&data, sens_no);

							/* check for valid gas data */
							if (data.status & BME69X_GASM_VALID_MSK)
							{
								if (!process_data(time_stamp, data, sensor_settings[sens_no].process_data, sens_no, output_ready))
								{
									return;
								}
							}
						}while(nFieldsLeft);
					}
				}
				
				/* Increment sample counter */
				n_samples++;
				
				/* Retrieve and store state if the passed save_intvl */
                bsec_get_state(bsecInstance[sens_no], 0, bsec_state, sizeof(bsec_state), work_buffer, sizeof(work_buffer), &bsec_state_len);
                
                uint32_t save_intvl = 100;
				if (n_samples >= save_intvl)
				{
                    if (status == BSEC_OK)
					{
						state_save(bsec_state, bsec_state_len);
					}
					n_samples = 0;
				}
			}
            taskYIELD();
            vTaskDelay(pdMS_TO_TICKS(100));
		}
    }
}

void allocate_memory(uint8_t *mem_block, uint8_t sens_no)
{
    bsecInstance[sens_no] = mem_block;
}