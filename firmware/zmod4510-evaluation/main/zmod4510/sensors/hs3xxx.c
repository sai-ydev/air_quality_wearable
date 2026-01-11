/*****************************************************************************
 * Copyright (c) 2024 Renesas Electronics Corporation
 * All Rights Reserved.
 * 
 * This code is proprietary to Renesas, and is license pursuant to the terms and
 * conditions that may be accessed at:
 * https://www.renesas.com/eu/en/document/msc/renesas-software-license-terms-gas-sensor-software
 *****************************************************************************/

/**
 * @file    hs3xxx.c
 * @brief   HS3xxx sensor function definitions
 * @version 2.7.1
 * @author  Renesas Electronics Corporation
 */

#include <stdlib.h>
#include <stdio.h>
#include "hs3xxx.h"


static char const*
_GetErrorString ( int  error, int  scope,  char*  buf, int  bufLen ) {
  if ( error == hteStaleData )
    snprintf ( buf, bufLen, "HS3xxx Error: Stale data - new data is not yet ready for readout." );
  else
    snprintf ( buf, bufLen, "HS3xxx Error: Unkown error code %d", error );
  
  return buf;
}

int
HS3xxx_Init ( HSxxxx_t*  sensor, Interface_t*  hal ) {
  int err;

  uint8_t  dummy[1];

  sensor -> interface = NULL;
  sensor -> i2cAddress = 0;

  /* Ensure the HAL is appropriately initialized. */
  if ( !hal -> i2cWrite ) return HAL_SetError ( heI2CWriteMissing, 0x44, HAL_GetErrorString );
  if ( !hal -> i2cRead )  return HAL_SetError ( heI2CReadMissing,  0x44, HAL_GetErrorString );

  /* HAL is appropriate - try to access sensor */
  sensor -> i2cAddress = 0x44;
  sensor -> interface = hal;
  err = sensor -> interface -> i2cWrite ( sensor -> interface -> handle, 
                                          sensor -> i2cAddress,
                                          dummy, 0, dummy, 0 );

  if ( err )
    sensor -> interface = NULL;

  return err;
}

int
HS3xxx_ReadID ( HSxxxx_t*  sensor, uint32_t*  id ) {
  return  HAL_SetError ( heNotImplemented, 0x44, HAL_GetErrorString );
}

int
HS3xxx_Measure ( HSxxxx_t*  sensor, HSxxxx_Results_t*  results ) {
  int error;

  if ( !sensor -> interface -> msSleep ) 
    return HAL_SetError ( heSleepMissing, 0x44, HAL_GetErrorString );

  error = HS3xxx_MeasureStart ( sensor );
  if ( error )
    return error;

  sensor -> interface -> msSleep ( 35 );

  return HS3xxx_MeasureRead ( sensor, results );
}

int
HS3xxx_MeasureStart ( HSxxxx_t*  sensor ) {
  /* Issue meausurement request (write without data). */
  uint8_t  dummy;
  return sensor -> interface -> i2cWrite ( sensor -> interface -> handle, 
                                           sensor -> i2cAddress,
                                           &dummy, 0, NULL, 0 );
}

int
HS3xxx_MeasureRead ( HSxxxx_t*  sensor, HSxxxx_Results_t*  results ) {
  int  error;
  uint8_t  buf [ 4 ];

  error = sensor -> interface -> i2cRead ( sensor -> interface -> handle,
                                           sensor -> i2cAddress,
                                           NULL, 0, buf, 4 );
  if ( error )
    return error;

  if ( buf [ 3 ] & 0x01 ) 
    return HAL_SetError ( hteStaleData, 0x44, _GetErrorString );

  float  rawHumidity    = ( ( buf [ 0 ] & 0x3f ) << 8 ) | buf [ 1 ];
  float  rawTemperature = ( ( buf [ 2 ] << 8 ) | ( buf [ 3 ] & 0xfc ) ) >> 2;
  
  results -> humidity    = 100 * rawHumidity    / 0x3fff;
  results -> temperature = 165 * rawTemperature / 0x3fff - 40;
  
  return ecSuccess;
}
