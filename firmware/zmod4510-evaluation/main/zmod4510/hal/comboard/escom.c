/*****************************************************************************
 * Copyright (c) 2024 Renesas Electronics Corporation
 * All Rights Reserved.
 * 
 * This code is proprietary to Renesas, and is license pursuant to the terms and
 * conditions that may be accessed at:
 * https://www.renesas.com/eu/en/document/msc/renesas-software-license-terms-gas-sensor-software
 *****************************************************************************/

/**
 * @file    escom.c
 * @brief   ESCom board function definitions
 * @version 2.7.1
 * @author  Renesas Electronics Corporation
 */

#include <stdio.h>
#include "hal/comboard/escom.h"

typedef struct {
  struct {
    uint8_t  channel : 3;
    uint8_t  delay   : 1;
    uint8_t  start   : 1;
    uint8_t  read    : 1;
    uint8_t  stop    : 1;
    uint8_t  lAddr   : 1;
  };
  uint8_t    len;
  uint16_t   addr   : 10;
  uint16_t   status :  6;
  uint8_t    data [];
} I2CCtrl_t;


static libusb_context*  _context;
static uint8_t  i2cBuffer [ 1024 ];


int
ESCom_Find ( ESComInterface_t*  handle, int*  count ) {

  libusb_device**  device;
  int  deviceIndex = 0;
  
  int errorCode = libusb_init ( &_context );
  if ( errorCode )
    return HAL_SetError ( errorCode & 0xffff, iesLibUSB, ESCom_GetErrorString );
  
  ssize_t cnt = libusb_get_device_list ( _context, &device );
  for ( ssize_t i = 0; i < cnt && deviceIndex < *count; ++ i ) {
    handle -> device = device [ i ];
    struct libusb_device_descriptor  desc;
    
    if ( libusb_get_device_descriptor ( handle -> device, &desc ) )
      continue;

    if ( desc . idVendor  != 0x45b ) continue;
    if ( desc . idProduct != 0x268 ) continue;

    handle -> handle = 0;
    handle -> inEP  = 0x83;
    handle -> outEP = 0x03;
    ++deviceIndex;
    ++handle;
  }

  *count = deviceIndex;
  return  ecSuccess;
}

int
ESCom_Connect ( ESComInterface_t*  handle ) {
  struct libusb_device_descriptor  desc;
  int errorCode = libusb_get_device_descriptor ( handle -> device, &desc );
  if ( errorCode )
    return HAL_SetError ( errorCode & 0xffff, iesLibUSB, ESCom_GetErrorString );

  errorCode = libusb_open ( handle -> device, & handle -> handle );
  if ( errorCode )
    return HAL_SetError ( errorCode & 0xffff, iesLibUSB, ESCom_GetErrorString );
  
  int  cfg = -1;
  errorCode = libusb_get_configuration ( handle -> handle, &cfg );
  if ( errorCode )
    return HAL_SetError ( errorCode & 0xffff, iesLibUSB, ESCom_GetErrorString );

  if ( cfg != 1 ) {
    errorCode = libusb_set_configuration ( handle -> handle, 1 );
    if ( errorCode )
      return HAL_SetError ( errorCode & 0xffff, iesLibUSB, ESCom_GetErrorString );
  }

  int  count = libusb_get_string_descriptor ( handle -> handle, desc . iSerialNumber, 0, ( uint8_t* ) handle -> serial, 64 );
  if ( count > 0 ) {
    handle -> serial [ count ] = 0;
    handle -> serial [ count + 1 ] = 0;
  }
  else
    return HAL_SetError ( errorCode & 0xffff, iesLibUSB, ESCom_GetErrorString );

  errorCode = libusb_claim_interface ( handle -> handle, 2 );
  if ( errorCode )
    return HAL_SetError ( errorCode & 0xffff, iesLibUSB, ESCom_GetErrorString );
  
  return ecSuccess;
}

int
ESCom_Disconnect ( ESComInterface_t*  handle ) {
  int errorCode = libusb_release_interface ( handle -> handle, 2 );
  libusb_close ( handle -> handle );
  if ( errorCode )
    return HAL_SetError ( errorCode & 0xffff, iesLibUSB, ESCom_GetErrorString );
  return ecSuccess;
}

int
ESCom_SetPower ( ESComInterface_t*  handle, bool  on ) {
  uint8_t  data [] = { 4, ( uint8_t ) on };

  int transferred;
  int errorCode = libusb_bulk_transfer ( handle -> handle, handle -> outEP, data, 2, &transferred, 100 );
  if ( errorCode )
    return HAL_SetError ( errorCode & 0xffff, iesLibUSB, ESCom_GetErrorString );
  
  errorCode = libusb_bulk_transfer ( handle -> handle, handle -> inEP, data, 2, &transferred, 500 );
  if ( errorCode )
    return HAL_SetError ( errorCode & 0xffff, iesLibUSB, ESCom_GetErrorString );
  
  if ( data [ 0 ] )
    return HAL_SetError ( data [ 0 ], iesCommand, ESCom_GetErrorString );

  return ecSuccess;
}

int
ESCom_I2CWrite ( ESComInterface_t*  hal, uint8_t  addr, uint8_t*  wData1, int  wSize1, uint8_t*  wData2, int  wSize2 ) {
  i2cBuffer [ 0 ] = 0;

  I2CCtrl_t*  b = ( I2CCtrl_t* ) ( i2cBuffer + 1 );
  b -> channel = 3;
  b -> start = 1;
  b -> delay = 0;
  b -> read  = 0;
  b -> stop  = 1;
  b -> lAddr = 0;
  b -> len   = ( wSize1 + wSize2 ) & 0xff;
  b -> addr  = addr;
  memcpy ( b -> data, wData1, wSize1 );
  memcpy ( b -> data + wSize1, wData2, wSize2 );
  int transferred;
  int errorCode = libusb_bulk_transfer ( hal -> handle, hal -> outEP, i2cBuffer,
                                         1 + sizeof ( I2CCtrl_t ) + wSize1 + wSize2,
                                         &transferred, 100 );
  if ( errorCode )
    return HAL_SetError ( errorCode & 0xffff, iesLibUSB, ESCom_GetErrorString );
  
  errorCode = libusb_bulk_transfer ( hal -> handle, hal -> inEP, i2cBuffer, 256, &transferred, 500 );
  if ( errorCode )
    return HAL_SetError ( errorCode & 0xffff, iesLibUSB, ESCom_GetErrorString );
  
  b = ( I2CCtrl_t* ) i2cBuffer;
  if ( b -> status != 0 )
    return HAL_SetError ( b -> status, iesI2C, ESCom_GetErrorString );
  return ecSuccess;
}

int
ESCom_I2CRead ( ESComInterface_t*  hal, uint8_t  addr, uint8_t*  wData, int  wSize, uint8_t*  rData, int  rSize ) {
  i2cBuffer [ 0 ] = 0;

  int  txSize = 1 + sizeof ( I2CCtrl_t );

  I2CCtrl_t*  b = ( I2CCtrl_t* ) ( i2cBuffer + 1 );
  if ( wSize ) {
    b -> channel = 3;
    b -> start = 1;
    b -> delay = 0;
    b -> read  = 0;
    b -> stop  = 0;
    b -> lAddr = 0;
    b -> len   = wSize & 0xff;
    b -> addr  = addr;
    memcpy ( b -> data, wData, wSize );

    b = ( I2CCtrl_t* ) ( b -> data + wSize );

    txSize += sizeof ( I2CCtrl_t ) + wSize;
  }
  b -> channel = 3;
  b -> start = 1;
  b -> delay = 0;
  b -> read  = 1;
  b -> stop  = 1;
  b -> lAddr = 0;
  b -> len   = rSize & 0xff;
  b -> addr  = addr;

  int transferred;
  int errorCode = libusb_bulk_transfer ( hal -> handle, hal -> outEP, i2cBuffer, txSize, &transferred, 100 );
  if ( errorCode )
    return HAL_SetError ( errorCode & 0xffff, iesLibUSB, ESCom_GetErrorString );
  
  errorCode = libusb_bulk_transfer ( hal -> handle, hal -> inEP, i2cBuffer, 256, &transferred, 500 );
  if ( errorCode )
    return HAL_SetError ( errorCode & 0xffff, iesLibUSB, ESCom_GetErrorString );

  b = ( I2CCtrl_t* ) i2cBuffer;
  if ( b -> status != 0 )
    return HAL_SetError ( b -> status, iesI2C, ESCom_GetErrorString );
  
  if ( wSize && ( ++b ) -> status != 0 )
    return HAL_SetError ( b -> status, iesI2C, ESCom_GetErrorString );
  
  memcpy ( rData, b -> data, rSize );
  return ecSuccess;
}

char const*
ESCom_GetDescriptionString ( ESComInterface_t*  ifce ) {
  static char  strBuf [ 100 ];
  sprintf ( strBuf, "ESCom Board, Serial='%ls'", ifce -> serial + 2 );
  return strBuf;
}

int
ESCom_GetSensorVoltage ( ESComInterface_t*  board, float*  voltage ) {
  uint8_t  data [ 10 ] = { 3, };

  int transferred;
  int errorCode = libusb_bulk_transfer ( board -> handle, board -> outEP, data, 1, &transferred, 100 );
  if ( errorCode )
    return HAL_SetError ( errorCode & 0xffff, iesLibUSB, ESCom_GetErrorString );
  
  errorCode = libusb_bulk_transfer ( board -> handle, board -> inEP, data, 5, &transferred, 500 );
  if ( errorCode )
    return HAL_SetError ( errorCode & 0xffff, iesLibUSB, ESCom_GetErrorString );
  
  if ( data [ 0 ] )
    return HAL_SetError ( data [ 0 ], iesCommand, ESCom_GetErrorString );

  *voltage = * ( float* ) ( data + 1 );

  return ecSuccess;
}


char const*
ESCom_GetErrorString ( int  error, int  scope, char*  buf, int  bufSize ) {
  char  bufL [ 30 ];
  char const*  err;
  if ( scope == iesLibUSB )
    snprintf ( buf, bufSize, "LibUSB Error %d: %s", error, libusb_strerror ( error ) );

  else if ( scope == iesI2C ) {
    switch ( error ) {
    case 1:
      err = "Sequence not executed";
      break;
    case 2:
      err = "Device busy";
      break;
    case 3:
      err = "Operation timed out";
      break;
    case 4:
      err = "No Acknowledge";
      break;
    default:
      sprintf ( bufL, "Code %d.", error );
      err = bufL;
    }
    snprintf ( buf, bufSize, "ESCom I2C error: %s", err );
  }

  else if ( scope == iesCommand )
    snprintf ( buf, bufSize, "Command error %d. Please contact Renesas support.", error );

  else
    snprintf ( buf, bufSize, "Error code %d for scope %d. Please contact Renesas support.", error, scope );

  return buf;
}
