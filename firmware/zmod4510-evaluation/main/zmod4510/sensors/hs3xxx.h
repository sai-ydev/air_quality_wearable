/*****************************************************************************
 * Copyright (c) 2024 Renesas Electronics Corporation
 * All Rights Reserved.
 * 
 * This code is proprietary to Renesas, and is license pursuant to the terms and
 * conditions that may be accessed at:
 * https://www.renesas.com/eu/en/document/msc/renesas-software-license-terms-gas-sensor-software
 *****************************************************************************/

/**
 * @addtogroup hs3xxx_api
 * @{
 * @file    hs3xxx.h
 * @brief   HS3xxx sensor declarations
 * @version 2.7.1
 * @author  Renesas Electronics Corporation
 */

#ifndef HS3XXX_H
#define HS3XXX_H

#include <stdbool.h>
#include "sensors/hsxxxx.h"

/**
 * @brief Error code definitions specific for the HS3xxx API
 */
typedef enum {
  hteStaleData = 1   /**< Sensor reported stale data. */
} HS3xxx_ErrorCodes_t;

/**
 * @brief Initialize the sensor object
 * 
 * This function tries accessing the HS3xxx I2C address using @a hal as
 * hardware interface. On success, the sensor object is initialized and
 * can be used afterwards. Otherwise, an error code is returned and the
 * sensor object is not usable.
 * 
 * @note The HS3xxx API requires that the HAL interface object has the 
 *       Interface_t::i2cRead and Interface_t::i2cWrite members defined.
 *       In addition, HS3xxx_Measure() requires the Interface_t::msSleep
 *       member, however, the check for this function is done in
 *       HS3xxx_Measure() and not in HS3xxx_Init().
 * 
 * @param sensor  Pointer to sensor object to be initialized
 * @param hal     Pointer to HAL object providing physical communication
 * @return int    Error code
 * @retval 0      On success
 * @retval other  On error
 */
int
HS3xxx_Init ( HSxxxx_t*  sensor, Interface_t*  hal );


/**
 * @brief Read the unique sensor ID of the HS3xxx
 * 
 * @param sensor  Pointer to an initialized sensor object
 * @param id      Pointer buffer to store the ID
 * @return int    Error code
 * @retval 0      On success
 * @retval other  On error
 */
int
HS3xxx_ReadID ( HSxxxx_t*  sensor, uint32_t*  id );

/**
 * @brief Execute a temperature/humidity measurement cycle
 * 
 * This function starts a measurement cycle in non-hold mode, waits for the
 * result to be available and reads it. This is a convenience function which
 * calls HS3xxx_MeasureStart() and HS3xxx_MeasureRead() with a delay between
 * both calls, to allow the sensor to complete its measurement.
 * 
 * @param sensor  Pointer to an initialized sensor object
 * @param results Pointer to a data structure, to store results in
 * @return int    Error code
 * @retval 0      On success
 * @retval other  On error
 */
int
HS3xxx_Measure ( HSxxxx_t*  sensor, HSxxxx_Results_t*  results );


/**
 * @brief Start a temperature/humidity measurement cycle
 * 
 * @param sensor  Pointer to an initialized sensor object
 * @return int    Error code
 * @retval 0      On success
 * @retval other  On error
 */
int
HS3xxx_MeasureStart ( HSxxxx_t*  sensor );


/**
 * @brief Read temperature/humidity results
 * 
 * This function reads the temperature/humidity results from a measurement that
 * has been started through HS3xxx_MeasureStart() previously.
 *
 * @param sensor  Pointer to an initialized sensor object
 * @param results Pointer to a data structure, to store results in
 * @return int    Error code
 * @retval 0      On success
 * @retval other  On error
 */
int
HS3xxx_MeasureRead ( HSxxxx_t*  sensor, HSxxxx_Results_t*  results );


#endif /* HS3XXX_H */

/** @} */
