/*****************************************************************************
 * Copyright (c) 2024 Renesas Electronics Corporation
 * All Rights Reserved.
 * 
 * This code is proprietary to Renesas, and is license pursuant to the terms and
 * conditions that may be accessed at:
 * https://www.renesas.com/eu/en/document/msc/renesas-software-license-terms-gas-sensor-software
 *****************************************************************************/

/** 
 * @addtogroup hal_api
 * @file template.c
 * 
 * @brief This file serves as a template for a customer specific HAL.
 * 
 * If not otherwise specified in the comments, all functions must be 
 * implemented. Detailed description of the HAL API can be found in
 * the firmware documentation for the Renesas product you're trying 
 * to interface.
 */

#include "hal/hal.h"
/* add your includes here */


static void
_Sleep ( uint32_t  ms ) {
  /** This function should do nothing but returning after the number of
   *  milliseconds passed in the 'ms' argument. */
}


static int
_I2CRead ( void*     handle, uint8_t  slAddr, 
           uint8_t*  wrData, int      wrLen, 
           uint8_t*  rdData, int      rdLen ) {
  /** The implementation of this function shall read data from I2C, strictly
   *  following the procedure below. If wrLen is not zero, the read must be
   *  preceded by an I2C write.
   *
   *  - I2C Start
   *  - if wrLen != 0:
   *    - Send: slAddr + WRITE
   *    - Send: wrLen bytes from wrData
   *    - I2C Restart  ( no stop before this start condition! )
   *  - Send: SlaveAddres + READ
   *  - Receive: rdLen bytes into rdData
   *  - I2CStop
   */
}


static int
_I2CWrite ( void*     handle,  uint8_t  slAddr, 
            uint8_t*  wrData1, int      wrLen1, 
            uint8_t*  wrData2, int      wrLen2 ) {
  /** The implementation of this function shall send the data provided in
   *  arguments wrData1 and wrData2 over the I2C bus. Both data blocks shall
   *  be sent directly after each other without a new start/restart condition.
   * 
   *  - I2C Start
   *  - Send: slAddr + WRITE
   *  - if wrLen1 != 0:
   *    - Send: wrLen1 bytes from wrData1
   *  - if wrLen2 != 0;
   *    - Send: wrLen2 bytes from wrData2
   *  - I2CStop
   */
}


static int
_Reset ( ) {
  /** This function shall pulse the reset pin of the sensor. 
   * @note Refer to the datasheet of the sensor(s) being interfaced for reset
   * timing requirements.
   */
}


int
HAL_Init ( Interface_t*  hal ) {
  int  errorCode = 0;

  /* Initialze your hardware: */
  errorCode = <YourHardwareInitFunction> ( );

  /* The handle assigned below will be passed to _I2Cxxx functions */
  hal -> handle = <YourInterfaceHandle>;

  /* Populate the hal object. Some of the functions may be optional
   *  for the product you're trying to interface. Please check the
   *  firmware documentation of your product.
   * If a function is not used, it must be set to NULL. */
  hal -> msSleep  = _Sleep;
  hal -> i2cRead  = _I2CRead;
  hal -> i2cWrite = _I2CWrite;
  hal -> reset    = _Reset;

  return errorCode;
}

int
HAL_Deinit ( Interface_t*  hal ) {
  int  errorCode = 0;
  
  errorCode = <YourHardwareDeinitFunction> ( );
  hal -> handle = NULL;

  return errorCode;
}

void
HAL_HandleError ( int  errorCode, void const*  context ) {
  /** Define what happens in case of error */
  
}
