/*****************************************************************************
 * Copyright (c) 2024 Renesas Electronics Corporation
 * All Rights Reserved.
 * 
 * This code is proprietary to Renesas, and is license pursuant to the terms and
 * conditions that may be accessed at:
 * https://www.renesas.com/eu/en/document/msc/renesas-software-license-terms-gas-sensor-software
 *****************************************************************************/

/**
 * @file    hs4xxx.c
 * @brief   HS4xxx sensor function definitions
 * @version 2.7.1
 * @author  Renesas Electronics Corporation
 */

#include <stdlib.h>
#include <stdio.h>
#include "hs4xxx.h"


static char const*
_GetErrorString ( int  error, int  scope, char*  str, int  bufSize ) {
  if ( error == hteHS4xxxCRCError )
    snprintf ( str, bufSize, "HS4xxx ERROR: Checksum verification failed" );
  else
    snprintf ( str, bufSize, "HS4xxx ERROR: Unkown error %d", error );

  return str;
}


static uint8_t
_ComputeCRC ( uint8_t*  data, int  len ) {
  uint16_t  g   = 0x11d;
  uint16_t  crc = 0xff;

  for ( int i = 0; i < len; ++ i ) {
    crc ^= data [ i ];

    for ( int j = 0; j < 8; ++ j ) {
      crc <<= 1;
      if ( crc & ( 1 << 8 ) )
        crc ^= g;
    }
  }

  return crc & 0xff;
}


int
HS4xxx_Init ( HSxxxx_t*  sensor, Interface_t*  hal ) {
  int err;
  uint8_t  dummy[1];

  sensor -> i2cAddress = 0x54;
  sensor -> interface = NULL;

  if ( !hal -> i2cRead )   return HAL_SetError ( heI2CReadMissing, 0x54, HAL_GetErrorString );
  if ( !hal -> i2cWrite )   return HAL_SetError ( heI2CWriteMissing, 0x54, HAL_GetErrorString );

  sensor -> interface = hal;
  err = sensor -> interface -> i2cWrite ( sensor -> interface -> handle, 
                                          sensor -> i2cAddress,
                                          dummy, 0, dummy, 0 );
  if ( err )
    sensor -> interface = NULL;

  return err;
}


int
HS4xxx_ReadID ( HSxxxx_t*  sensor, uint32_t*  id ) {
  uint8_t  buf [ 4 ] = { 0xd7, };

  int errorCode = sensor -> interface -> i2cRead ( sensor -> interface -> handle, 
                                                   sensor -> i2cAddress,
                                                   buf, 1, buf, 4 );
  if ( errorCode )
    return errorCode;

  *id = ( buf [ 0 ] << 24 ) 
      + ( buf [ 1 ] << 16 )
      + ( buf [ 2 ] <<  8 )
      +   buf [ 3 ];

  return  ecSuccess;
}


static inline int
_ProcessRawResult ( uint8_t*  raw, HSxxxx_Results_t*  results ) {

  if ( _ComputeCRC ( raw, 4 ) != raw [ 4 ] )
    return HAL_SetError ( hteHS4xxxCRCError, esSensor, _GetErrorString );

  float  humidity    = ( ( raw [ 0 ] & 0x3f ) << 8 ) | raw [ 1 ];
  float  temperature = ( ( raw [ 2 ] & 0x3f ) << 8 ) | raw [ 3 ];

  results -> humidity    = 100 * humidity / 0x3fff;
  results -> temperature = 165 * temperature / 0x3fff - 40;
  
  return ecSuccess;
}


int
HS4xxx_Measure ( HSxxxx_t*          sensor, 
                 HSxxxx_Results_t*  results ) {

  /* this function requires the msSleep function of the HAL */
  if ( ! sensor -> interface -> msSleep )
    return HAL_SetError ( heSleepMissing, sensor -> i2cAddress,
                          HAL_GetErrorString );

  int errorCode = HS4xxx_MeasureStart ( sensor );
  if ( errorCode )
    return errorCode;

  /* wait for result to be available */
  sensor -> interface -> msSleep ( 2 );

  return HS4xxx_MeasureRead ( sensor, results );
}


int
HS4xxx_MeasureStart ( HSxxxx_t*  sensor ) {
  uint8_t  cmd = 0xf5;

  /* this function requires the i2cWrite function of the HAL */
  if ( ! sensor -> interface -> i2cWrite )
    return HAL_SetError ( heI2CWriteMissing, sensor -> i2cAddress, 
                          HAL_GetErrorString );

  return sensor -> interface -> i2cWrite ( sensor -> interface -> handle,
                                           sensor -> i2cAddress,
                                           &cmd, 1, NULL, 0 );
}


int
HS4xxx_MeasureRead ( HSxxxx_t*          sensor, 
                     HSxxxx_Results_t*  results ) {
  uint8_t  buf [ 5 ];

  int errorCode = 
    sensor -> interface -> i2cRead ( sensor -> interface -> handle,
                                     sensor -> i2cAddress,
                                     buf, 0, buf, 5 );
  if ( errorCode )
    return errorCode;

  return _ProcessRawResult ( buf, results );
}


int
HS4xxx_MeasureHold ( HSxxxx_t*          sensor, 
                     HSxxxx_Results_t*  results ) {
  uint8_t  buf [ 5 ] = { 0xe5, };

  int errorCode = 
    sensor -> interface -> i2cRead ( sensor -> interface -> handle,
                                     sensor -> i2cAddress,
                                     buf, 1, buf, 5 );
  if ( errorCode )
    return errorCode;

  return _ProcessRawResult ( buf, results );
}
