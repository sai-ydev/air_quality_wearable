/*****************************************************************************
 * Copyright (c) 2024 Renesas Electronics Corporation
 * All Rights Reserved.
 * 
 * This code is proprietary to Renesas, and is license pursuant to the terms and
 * conditions that may be accessed at:
 * https://www.renesas.com/eu/en/document/msc/renesas-software-license-terms-gas-sensor-software
 *****************************************************************************/

/**
 * @addtogroup raspi_hal
 * @{
 * @file    rpi.c
 * @brief   Raspberry PI HAL function definitions
 * @version 2.7.1
 * @author  Renesas Electronics Corporation
 */


#include <stdio.h>
#include <string.h>
#include <pigpio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "hal/raspi/rpi.h"
#include "hal/hal.h"


#define I2C_BUS 1

static  int     _rpi = -1;
static uint8_t  _i2cCmdBuf [ 1024 ];

// remember hal object for deinitialization
static Interface_t*   _hal = NULL;

static char const*
_GetErrorString ( int  error, int  scope, char*  str, int  bufLen ) {
  char  msg [ 100 ];
  char const*  err;
  if ( scope == resPiGPIO ) {
    sprintf ( msg, "PiGPIO library error %i. Please check PiGPIO library documentation for details.", error );
    err = msg;
  }
  else if ( scope == resI2C ) {
    if ( recI2CLenMismatch == error ) {
      err = "The number of processed bytes does not match the number of reequested bytes";
    }
    else
      sprintf ( msg, "Unknown error code %d", error );
      err = msg;
  }
  snprintf ( str, bufLen, "  Raspberry PI Error: %s", err );
  return str;
}

static void
_SleepMS ( uint32_t  ms ) {
  usleep ( ms * 1000 );
}

static int
_Connect ( ) {
  int errorCode = gpioInitialise ( );
  if ( errorCode < 0 )
    return HAL_SetError ( errorCode, resPiGPIO, _GetErrorString );
  _rpi = i2cOpen ( I2C_BUS, 0, 0 );
  if ( _rpi < 0 )
    return HAL_SetError ( _rpi, resPiGPIO, _GetErrorString );
  
  // initialize reset pin:
  //  Set as input ...
  errorCode = gpioSetMode ( 26, PI_INPUT );
  if ( errorCode ) 
    return HAL_SetError ( errorCode, resPiGPIO, _GetErrorString );

  //  ... with pullup ...
  errorCode = gpioSetPullUpDown ( 26, PI_PUD_UP );
  if ( errorCode ) 
    return HAL_SetError ( errorCode, resPiGPIO, _GetErrorString );

  return ecSuccess;
}

static int
_I2CRead ( void*  handle, uint8_t  slAddr, uint8_t*  wrData, int  wrLen, uint8_t*  rdData, int  rdLen ) {
  uint8_t*  p = _i2cCmdBuf;
  // set address
  *p++ = PI_I2C_ADDR;
  *p++ = slAddr;
  if ( wrLen ) {
    // write command
    *p++ = PI_I2C_WRITE;
    *p++ = wrLen;
    memcpy ( p, wrData, wrLen );
    p += wrLen;
  }
  // read command
  *p++ = PI_I2C_READ;
  *p++ = rdLen;
  // no more commands
  *p++ = 0;
  
  // for some unknown reason the last parameter must be 1 byte more than the actual read count
  int  count = i2cZip ( _rpi, ( char* ) _i2cCmdBuf, p - _i2cCmdBuf, ( char* ) rdData, rdLen + 1 );
  if ( count < 0 )
    return HAL_SetError ( count, resPiGPIO, _GetErrorString );
  else if ( count != rdLen )
    return HAL_SetError ( recI2CLenMismatch, resI2C, _GetErrorString );
  return ecSuccess;
}

static int
_I2CWrite ( void*  handle, uint8_t  slAddr, uint8_t*  wrData1, int  wrLen1, uint8_t*  wrData2, int  wrLen2 ) {
  uint8_t*  p = _i2cCmdBuf;
  uint8_t  dummy [ 10 ];
  // set address
  *p++ = PI_I2C_ADDR;
  *p++ = slAddr;
  // write command
  *p++ = PI_I2C_WRITE;
  *p++ = wrLen1 + wrLen2;
  memcpy ( p, wrData1, wrLen1 );
  p += wrLen1;
  memcpy ( p, wrData2, wrLen2 );
  p += wrLen2;
  // no more commands
  *p++ = 0;

  int  count = i2cZip ( _rpi, ( char* ) _i2cCmdBuf, p - _i2cCmdBuf, ( char* ) dummy, 0 );
  if ( count < 0 )
    return HAL_SetError ( count, resPiGPIO, _GetErrorString );
  else if ( count )
    return HAL_SetError ( recI2CLenMismatch, resI2C, _GetErrorString );
  return ecSuccess;
}

static int
_Reset ( ) {

  //  ... Define output value:
  //  for a sensor reset, the pin is temporarily configured as output
  int errorCode = gpioWrite ( 26, 0 );
  if ( errorCode ) 
    return HAL_SetError ( errorCode, resPiGPIO, _GetErrorString );

  // initialize reset pin
  _SleepMS ( 1 );
  errorCode = gpioSetMode ( 26, PI_INPUT );
  if ( errorCode ) 
    return HAL_SetError ( errorCode, resPiGPIO, _GetErrorString );

  return ecSuccess;  
}

void
_Terminate ( int  sig ) {
  printf ( "Termination requested by user\n" );
  HAL_HandleError ( ecSuccess, NULL );
}

int
HAL_Init ( Interface_t*  hal ) {

  printf ( "Initializing Raspberry Pi HAL\n\n" );
  printf ( "This application can be be terminated at any "
           "time by pressing Ctrl-C\n\n" );

  _hal = hal;

  int errorCode = _Connect ( );

  // register signal handler for Ctlr-C
  // this needs to be called after gpioInitialize (called from _Connect)
  signal ( SIGINT, _Terminate );

  if ( ! errorCode ) {
    hal -> msSleep        = _SleepMS;
    hal -> i2cRead        = _I2CRead;
    hal -> i2cWrite       = _I2CWrite;
    hal -> reset          = _Reset;
  }
  return errorCode;
}


int
HAL_Deinit ( Interface_t*  hal ) {
  int  errorCode = 0;
  if ( _rpi > -1 )
    errorCode = i2cClose ( _rpi );
  gpioTerminate ( );
  if ( errorCode )
    return HAL_SetError ( errorCode, resPiGPIO, _GetErrorString );
  return ecSuccess;
}


void
HAL_HandleError ( int  errorCode, void const*  contextV ) {
  char const*  context = ( char const* ) contextV;
  int  error, scope;
  char  msg [ 200 ];
  if ( errorCode ) {
    printf ( "ERROR code %i received during %s\n", errorCode, context );
    printf ( "  %s\n", HAL_GetErrorInfo ( &error, &scope, msg, 200 ) );
  }
  errorCode = HAL_Deinit ( _hal );
  if ( errorCode ) {
    printf ( "ERROR code %i received during interface deinitialization\n", 
             errorCode );
    printf ( "  %s\n", HAL_GetErrorInfo ( &error, &scope, msg, 200 ) );
  }

  printf ( "\nExiting\n" );
  exit ( errorCode );
}

/** @} */
