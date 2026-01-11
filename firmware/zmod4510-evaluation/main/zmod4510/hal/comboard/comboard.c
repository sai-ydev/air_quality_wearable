/*****************************************************************************
 * Copyright (c) 2024 Renesas Electronics Corporation
 * All Rights Reserved.
 * 
 * This code is proprietary to Renesas, and is license pursuant to the terms and
 * conditions that may be accessed at:
 * https://www.renesas.com/eu/en/document/msc/renesas-software-license-terms-gas-sensor-software
 *****************************************************************************/

/**
 * @addtogroup comboard_hal
 * @{
 * @file    comboard.c
 * @brief   HAL implementation of ESCom/HiCom based code
 * @version 2.7.1
 * @author  Renesas Electronics Corporation
 */


#include <stdio.h>
#include <signal.h>
#include <windows.h>

#include "hal/hal.h"

#include "hal/comboard/escom.h"

#ifndef EXCLUDE_HICOM
#  include "hal/comboard/hicom.h"
#endif


#define  ESCOM_MAX_COUNT  3
#define  HICOM_MAX_COUNT  3

typedef void*  BoardHandle_t;


static ESComInterface_t   _esComList [ ESCOM_MAX_COUNT ];
#ifndef EXCLUDE_HICOM
static HiComInterface_t   _hiComList [ HICOM_MAX_COUNT ];
#endif

static BoardHandle_t      _boardHandle;

static int ( *_deinitFunction ) ( Interface_t* );

/* remember hal object for deinitialization */
static Interface_t*   _hal = NULL;


int
_ESComInit ( Interface_t*  hal, ESComInterface_t**  board ) {
  int  count = ESCOM_MAX_COUNT;

  /* Scan for ESCom boards connected to the host.
   * Up to ESCOM_MAX_COUNT devices are detected. */
  int  errorCode = ESCom_Find ( _esComList, &count );
  if ( errorCode )
    return errorCode;

  /* Iterate over list of ESCom boards and try to connect.
   * The first one that succeeds will be used. */
  for ( int i = 0; i < count; ++ i ) {
    errorCode = ESCom_Connect ( &_esComList [ i ] );
    if ( errorCode )
      /* Try next board if we cannot connect to this one. */
      continue;

    hal -> handle   = &_esComList [ i ];
    hal -> i2cRead  = ( I2CImpl_t ) ESCom_I2CRead;
    hal -> i2cWrite = ( I2CImpl_t ) ESCom_I2CWrite;

    *board = & _esComList [ i ];
    return ecSuccess;
  }

  return HAL_SetError ( heNoInterface, esHAL, HAL_GetErrorString );
}


static int
_ESComDeinit ( Interface_t*  hal ) {
  int errorCode = ESCom_SetPower ( ( ESComInterface_t* ) ( hal -> handle ), false );
  
  /* Report error, but don't return: Try disconnecting regardless of the error. */
  if ( errorCode )
    printf ( "ERROR code x%02x received during ESCom power down.\n", 
             errorCode );

  return ESCom_Disconnect ( ( ESComInterface_t* ) ( hal -> handle ) );
}

#ifndef EXCLUDE_HICOM
static int
_HiComInit ( Interface_t*  hal, HiComInterface_t**  board ) {
  int  count = HICOM_MAX_COUNT;

  /* Scan for HiCom boards connected to the host.
   * Up to HICOM_MAX_COUNT devices are detected. */
  int  errorCode = HiCom_Find ( _hiComList, &count );
  if ( errorCode )
    return errorCode;

  /* Iterate over list of HiCom boards and try to connect.
   *  The first one that succeeds will be used. */
  for ( int i = 0; i < count; ++ i ) {
    errorCode = HiCom_Connect ( &_hiComList [ i ] );
    if ( errorCode )
      /* Try next board if we cannot connect to this one. */
      continue;

    hal -> handle       = & _hiComList [ i ];
    hal -> i2cRead      = ( I2CImpl_t ) HiCom_I2CRead;
    hal -> i2cWrite     = ( I2CImpl_t ) HiCom_I2CWrite;

    *board = & _hiComList [ i ];
    return ecSuccess;
  }

  return HAL_SetError ( heNoInterface, esHAL, HAL_GetErrorString );
}

static int
_HiComDeinit ( Interface_t*  hal ) {
  int errorCode = HiCom_SetPower ( ( HiComInterface_t* ) ( hal -> handle ), false );

  /* Report error, but don't return: Try disconnecting regardless of the error. */
  if ( errorCode )
    printf ( "ERROR code x%02x received during HiCom power down.\n", 
             errorCode );

  return HiCom_Disconnect ( ( HiComInterface_t* ) ( hal -> handle ) );
}
#endif


void
_Terminate ( int  sig ) {
  printf ( "Termination requested by user\n" );
  HAL_HandleError ( ecSuccess, NULL );
}

void
_Sleep ( uint32_t  ms ) {
  Sleep ( ms );
}


int
HAL_Init ( Interface_t*  hal ) {
  int errorCode;

  _hal = hal;
  printf ( "This application can be be terminated at any time by pressing Ctrl-C\n\n" );
  printf ( "Looking for ESCom Boards ...\n" );

  /* register signal handler for Ctlr-C */
  signal ( SIGINT, _Terminate );

  hal -> msSleep = _Sleep;
 
  /* Try to connect an ESCom board */
  errorCode = _ESComInit ( hal, ( ESComInterface_t** ) &_boardHandle );
  if ( !errorCode ) {
    printf ( "Detected ESCom board\n" );
    printf ( "  USB Serial: %ls\n",  ( ( ESComInterface_t* ) _boardHandle ) -> serial + 1 );
 
    /* board is connected - set deinit function  */
    _deinitFunction = _ESComDeinit;

    /* swtich on sensor voltage */
    errorCode = ESCom_SetPower ( _boardHandle, true );
    if ( errorCode ) return errorCode;

    /* allow setteling of sensor supply voltage */
    hal -> msSleep ( 100 );
    
    /* Verify the sensor is powered - if not, notify user and
     *  exit demo. Typically this is due to missing jumper. */
    float  voltage;
    errorCode = ESCom_GetSensorVoltage ( _boardHandle, &voltage );
    if ( errorCode ) return errorCode;
    if ( voltage < 1.8 ) {
      printf ( "  ERROR: Bad sensor supply voltage. Please check that the supply voltage jumpers are set correctly.\n\n" );
      return -1;
    }
    else
      printf ( "  Sensor voltage: %.1f\n\n", voltage );

    /* we're connected and powered - return successfully */
    return ecSuccess;
  }

#ifndef EXCLUDE_HICOM
  /* Try to connect an HiCom board */
  printf ( "Looking for HiCom Boards\n" );
  errorCode = _HiComInit ( hal, ( HiComInterface_t** ) &_boardHandle );
  if ( !errorCode ) {
    printf ( "Detected HiCom board\n" );
 
    /* board is connected - set deinit function  */
    _deinitFunction = _HiComDeinit;

    /* swtich on sensor voltage */
    errorCode = HiCom_SetPower ( _boardHandle, true );
    if ( errorCode ) return errorCode;

    hal -> msSleep ( 100 );

    /* we're connected and powered - return successfully */
    return ecSuccess;
  }
#endif

  return errorCode;
}


int
HAL_Deinit ( Interface_t*  hal ) {
  int err = _deinitFunction ( hal );
  memset ( hal, 0, sizeof ( Interface_t ) );
  return err;
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
