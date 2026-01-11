/*****************************************************************************
 * Copyright (c) 2024 Renesas Electronics Corporation
 * All Rights Reserved.
 * 
 * This code is proprietary to Renesas, and is license pursuant to the terms and
 * conditions that may be accessed at:
 * https://www.renesas.com/eu/en/document/msc/renesas-software-license-terms-gas-sensor-software
 *****************************************************************************/

/**
 * @addtogroup hs4xxx_api
 * @{
 * @file    hs4xxx.h
 * @brief   HS4xxx sensor declarations
 * @version 2.7.1
 * @author  Renesas Electronics Corporation
 */

#ifndef HS4XXX_H
#define HS4XXX_H

#include <stdbool.h>
#include "sensors/hsxxxx.h"

/**
 * @brief Error code definitions specific for the HS4xxx API
 */
typedef enum {
  hteHS4xxxCRCError = 1     /**< Result CRC error */
} HS4xxx_ErrorCodes_t;


/**
 * @brief Initialize the sensor object
 * 
 * This function tries accessing the HS4xxx I2C address using @a hal as
 * hardware interface. On success, the sensor object is initialized and
 * can be used afterwards. Otherwise, an error code is returned and the
 * sensor object is not usable.
 * 
 * @note The HS4xxx API requires that the HAL interface object has the 
 *       Interface_t::i2cRead and Interface_t::i2cWrite members defined.
 *       In addition, HS4xxx_Measure() requires the Interface_t::msSleep
 *       member.
 * 
 * @param sensor  Pointer to sensor object to be initialized
 * @param hal     Pointer to HAL object providing physical communication
 * @return int    Error code
 * @retval 0      On success
 * @retval other  On error
 */
int
HS4xxx_Init ( HSxxxx_t*  sensor, Interface_t*  hal );

/**
 * @brief Read the unique sensor ID of the HS4xxx
 * 
 * @param sensor  Pointer to an initialized sensor object
 * @param id      Pointer buffer to store the ID
 * @return int    Error code
 * @retval 0      On success
 * @retval other  On error
 */
int
HS4xxx_ReadID ( HSxxxx_t*  sensor, uint32_t*  id );

/**
 * @brief Perform a temperature/humidity measurement cycle
 * 
 * This function starts a measurement cycle in non-hold mode, waits for the
 * result to be available and reads it. If the checksum computation is correct
 * the result is stored in the @a results data structure. Otherwise the 
 * contents of @a results is left unmodified and an error code is returned.
 * 
 * @note Although the HS4xxx is able to perform the requested measurement
 *       with a single I2C transaction (refer to HS4xxx_MeasureHold), this
 *       function uses the HS4xxx_MeasureStart() and HS4xxx_MeasureRead()
 *       functions together with the Interface_t::msSleep function. This
 *       is to allow operation on a wider variety of I2C interfaces. 
 *       HS4xxx_MeasureHold() requires a minimum of 200kHz I2C clock frequency
 *       which is not supported by all interfaces.
 * 
 * @param sensor   Pointer to an initialized sensor object
 * @param results  Pointer to a data structure, to store results in
 * @return int     Error code
 * @retval 0       On success
 * @retval other   On error
 */
int
HS4xxx_Measure ( HSxxxx_t*  sensor, HSxxxx_Results_t*  results );


/**
 * @brief Perform a temperature/humidity measurement cycle using hold mode
 * 
 * This function reads temperature/humidity values from the HS4xxx in hold
 * mode (Refer to the HS4xxx datasheet for an explanation of hold mode).
 * If the checksum computation is correct the result is stored in the 
 * @a results data structure. Otherwise the  contents of @a results is left
 * unmodified and an error code is returned.
 * 
 * @note For hold measurements, the I2C clock frequency must be at least 
 *       200kHz. Otherwise the result readout is unreliable.
 * 
 * @param sensor  Pointer to an initialized sensor object
 * @param results Pointer to a data structure, to store results in
 * @return int    Error code
 * @retval 0      On success
 * @retval other  On error
 */
int
HS4xxx_MeasureHold ( HSxxxx_t*  sensor, HSxxxx_Results_t*  results );


/**
 * @brief Start a temperature/humidity measurement cycle in non-hold mode
 * 
 * @param sensor  Pointer to an initialized sensor object
 * @return int    Error code
 * @retval 0      On success
 * @retval other  On error
 */
int
HS4xxx_MeasureStart ( HSxxxx_t*  sensor );


/**
 * @brief Read temperature/humidity results in non-hold mode
 * 
 * This function reads the temperature/humidity results from a measurement that
 * has been started through HS4xxx_MeasureStart() previously. If the checksum 
 * computation is correct the result is stored in the @a results data structure.
 * Otherwise the contents of @a results is left unmodified and an error code
 * is returned.
 *
 * @param sensor   Pointer to an initialized sensor object
 * @param results  Pointer to a data structure, to store results in
 * @return int     Error code
 * @retval 0       On success
 * @retval other   On error
 */
int
HS4xxx_MeasureRead ( HSxxxx_t*  sensor, HSxxxx_Results_t*  results );

#endif /* HS4XXX_H */

/** @} */
