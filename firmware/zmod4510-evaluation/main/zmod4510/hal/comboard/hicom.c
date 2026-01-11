/*****************************************************************************
 * Copyright (c) 2024 Renesas Electronics Corporation
 * All Rights Reserved.
 * 
 * This code is proprietary to Renesas, and is license pursuant to the terms and
 * conditions that may be accessed at:
 * https://www.renesas.com/eu/en/document/msc/renesas-software-license-terms-gas-sensor-software
 *****************************************************************************/

/**
 * @file    hicom.c
 * @brief   HiCom interrface function definitions
 * @version 2.7.1
 * @author  Renesas Electronics Corporation
 */

#include <stdio.h>
#include "hal/comboard/hicom.h"

#ifndef EXCLUDE_HICOM

int
HiCom_Find ( HiComInterface_t*  handle, int*  count ) {
  HiComStatus_t  errorCode;

  DWORD  deviceCount;

  uint32_t  maxCount = *count;
  *count = 0;

  errorCode = Init_libMPSSE ( );
  if ( errorCode ) {
    printf ( "Failed to load ftd2xx.dll - Please install FTDI drivers\n" );
    return ecSuccess;
  }
    

  errorCode = I2C_GetNumChannels ( &deviceCount );
  if ( errorCode )
    return HAL_SetError ( errorCode & 0xffff, iesFTDI, HiCom_GetErrorString );

  if ( deviceCount < 1 ) {
    *count = 0;
    return ecSuccess;
  }

  /* search for HiCom device */
  for ( int i = 0; i < deviceCount; ++i ) {
    /* ask for device name */
    errorCode = I2C_GetChannelInfo ( i, (FT_DEVICE_LIST_INFO_NODE*) handle );
    if ( errorCode )
      continue;

    if ( strcmp ( handle -> node . SerialNumber, "A" ) )
      continue;

    handle -> index = i;

    ++handle;
    *count += 1;
    if ( *count >= maxCount )
      break;
  }

  return ecSuccess;
}

int
HiCom_Connect ( HiComInterface_t*  handle ) {
  HiComStatus_t  errorCode;

  errorCode = I2C_OpenChannel ( handle -> index, &handle -> node . ftHandle );
  if ( errorCode )
    return HAL_SetError ( errorCode & 0xffff, iesFTDI, HiCom_GetErrorString );

  ChannelConfig  conf = { HICOM_I2C_SPEED, 2, 0 };
  /*  initialize channel */
  errorCode = I2C_InitChannel ( handle -> node . ftHandle, &conf );
  if ( errorCode )
    return HAL_SetError ( errorCode & 0xffff, iesFTDI, HiCom_GetErrorString );

  return ecSuccess;
}

int
HiCom_Disconnect ( HiComInterface_t*  handle ) {
  HiComStatus_t errorCode;
  errorCode = I2C_CloseChannel ( handle -> node . ftHandle );
  if ( errorCode )
    return HAL_SetError ( errorCode & 0xffff, iesFTDI, HiCom_GetErrorString );

  return ecSuccess;
}

int
HiCom_SetPower ( HiComInterface_t*  handle, bool  on ) {
  HiComStatus_t  errorCode;
  errorCode = FT_WriteGPIOLow ( handle -> node . ftHandle, 0x80, on ? 0x80 : 0 );
  if ( errorCode )
    return HAL_SetError ( errorCode & 0xffff, iesFTDI, HiCom_GetErrorString );

  return ecSuccess;
}


int
HiCom_I2CWrite ( HiComInterface_t*  hal, uint8_t  addr, uint8_t*  wData1, int  wSize1, uint8_t*  wData2, int  wSize2 ) {
  DWORD  transferred;
  HiComStatus_t  errorCode;

  if ( wSize1 && wSize2 ) {
    errorCode = I2C_DeviceWrite ( hal -> node . ftHandle, addr,
                                  wSize1, wData1, &transferred, 0x1d ) |
                I2C_DeviceWrite ( hal -> node . ftHandle, addr,
                                  wSize2, wData2, &transferred, 0x5e );
  }
  else
    errorCode = I2C_DeviceWrite ( hal -> node . ftHandle, addr,
                                  wSize2 ? wSize2 : wSize1, 
                                  wSize2 ? wData2 : wData1,
                                  &transferred, 0x1f );

  if ( errorCode )
    return HAL_SetError ( errorCode & 0xffff, iesFTDI, HiCom_GetErrorString );

  return ecSuccess;
}

int
HiCom_I2CRead ( HiComInterface_t*  hal, uint8_t  addr, uint8_t*  wrData, int  wrSize, uint8_t*  rdData, int  rdSize ) {
  DWORD  transferred;
  HiComStatus_t  errorCode;
  if ( wrSize ) {
    errorCode = I2C_DeviceWrite ( hal -> node . ftHandle, addr,
                                  wrSize, wrData, &transferred, 0x1d );
    if ( errorCode )
      return HAL_SetError ( errorCode & 0xffff, iesFTDI, HiCom_GetErrorString );
  }

  errorCode = I2C_DeviceRead ( hal -> node . ftHandle, addr,
                               rdSize, rdData, &transferred, 0x1f );
  if ( errorCode )
    return HAL_SetError ( errorCode & 0xffff, iesFTDI, HiCom_GetErrorString );

  return ecSuccess;
}


char const*
HiCom_GetErrorString ( int  error, int  scope, char*  str, int  bufLen ) {
  snprintf ( str, bufLen, "HiCom error %x:\nNote: The HiCom is obsoleted. Consider ESCom board instead.\n", error );
  return str;
}

#endif
