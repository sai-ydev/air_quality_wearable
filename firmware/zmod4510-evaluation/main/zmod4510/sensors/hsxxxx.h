/*****************************************************************************
 * Copyright (c) 2024 Renesas Electronics Corporation
 * All Rights Reserved.
 * 
 * This code is proprietary to Renesas, and is license pursuant to the terms and
 * conditions that may be accessed at:
 * https://www.renesas.com/eu/en/document/msc/renesas-software-license-terms-gas-sensor-software
 *****************************************************************************/

/**
 * @addtogroup hsxxxx_api
 * @{
 * @file    hsxxxx.h
 * @brief   Renesas humidity sensors (HS3xxx, HS4xxx) abstraction
 * @version 2.7.1
 * @author  Renesas Electronics Corporation
 */

#ifndef HSXXXX_H
#define HSXXXX_H

#include <stdbool.h>
#include "hal/hal.h"

/**
 * @brief Data structure holding humidity/temperature results
 */
typedef struct {
  float  temperature;  /**< Temperature value in degree Celsius */
  float  humidity;     /**< Relative humidity */
} HSxxxx_Results_t;

/**
 * @brief Data structure holding information required for HSxxxx API operation 
 */
typedef struct {
  Interface_t*   interface;   /**< Pointer to the hal object for physical communication */
  uint8_t        i2cAddress;  /**< I2C slave address of the HSxxxx sensor */
} HSxxxx_t;

/**
 * @brief  Initialize the sensor object
 * 
 * This function tries searching for a HS4xxx sensor first, by
 *  accessing the corresponding I2C address. If no such sensor is found,
 *  the function searches for a HS3xxx.
 * 
 * The type of sensor being detected can be determined using the
 *  function HSxxxx_Name().
 * 
 * @param sensor  Pointer to sensor object to be initialized.
 * @param hal     Pointer to hal object for physical communication.
 * @return int    Error code
 * @retval 0      On success
 * @retval other  On error
 */
int
HSxxxx_Init ( HSxxxx_t*  sensor, Interface_t*  hal );

/**
 * @brief  Perform one temperature measurement
 * 
 * This function starts a temperature/humidity measurement and waits for
 * the availability of the result before it returns.
 * 
 * @note This function is implemented as blocking function. Thus while the
 * measurement is ongoing no other code is executed. Depending on the sensor
 * the bocking time may be multiple tens of milliseconds.
 * 
 * @param sensor   Pointer to the sensor object to be used.
 * @param results  Pointer to data structure for result storage.
 * @return int     Error code
 * @retval 0       On success
 * @retval other   On error
 */
int
HSxxxx_Measure ( HSxxxx_t*  sensor, HSxxxx_Results_t*  results );

/**
 * @brief  Return the temperature/humidity sensor type name
 * 
 * The function HSxxxx_Init can identify different types of temperature/
 *  humidity sensors. This function may be used to determine the sensor
 *  type that has been identified. The identification is performed based
 *  on the sensors I2C address.
 * 
 * @param sensor  Pointer to the sensor object to be queried.
 * @return int    Error code
 * @retval 0      On success
 * @retval other  On error
 */
char const*
HSxxxx_Name ( HSxxxx_t*  sensor );

/**
 * @brief  Return the duration of a temperature measurement cycle
 * 
 * @param sensor  Pointer to the sensor object to be queried.
 * @return int    Duration of temperature measurement cycle
 */
static inline int 
HSxxxx_MeasurementDuration ( HSxxxx_t*  sensor ) {
  if ( sensor ) {
    if ( sensor -> i2cAddress == 0x44 ) {
      return 35;
    }
    else if ( sensor -> i2cAddress == 0x54 ) {
      return 2;
    }
  }
  return 0;
}

#endif /* HSXXXX_H */

/** @} */
