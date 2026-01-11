/*****************************************************************************
 * Copyright (c) 2024 Renesas Electronics Corporation
 * All Rights Reserved.
 * 
 * This code is proprietary to Renesas, and is license pursuant to the terms and
 * conditions that may be accessed at:
 * https://www.renesas.com/eu/en/document/msc/renesas-software-license-terms-gas-sensor-software
 *****************************************************************************/

/**
 * @addtogroup hicom_api
 * @ingroup    hal_api
 * @{
 * @file    hicom.h
 * @brief   HiCom board type and function declarations
 * @version 2.7.1
 * @author  Renesas Electronics Corporation
 */

#ifndef _HICOM__H_
#define _HICOM__H_

#include <stdint.h>
#include <stdbool.h>

#include "hal/hal.h"
#include "hal/3rdParty/ftdi/libmpsse_i2c.h"


#define HICOM_NAME       "Dual RS232-HS A"
#define HICOM_I2C_SPEED  100000


typedef FT_STATUS HiComStatus_t;
typedef FT_HANDLE HiComHandle_t;

typedef enum {
  iesFTDI    = 0x21
} HiComErrorScope_t;


typedef struct {
  FT_DEVICE_LIST_INFO_NODE  node;
  int                       index;
} HiComInterface_t;

/**
 * @brief  Enumerate HiCom boards
 * Scans USB ports for connected HiCom boards.
 * 
 * @param board pointer to a buffer storing board information
 * @param count [in] maximum count of boards to be stored
 *              [out] actual number of boards stored
 * @return int  0 on success, error code otherwise
 */
int  HiCom_Find ( HiComInterface_t*  board, int*  count );

/**
 * @brief Connect a HiCom instance
 * Tries to connect to an interface that has been discovered with 
 * HiCom_Find() previously
 * 
 * @param board Pointer to HiCom board discovered by HiCom_Find()
 * @return int  0 on success, error code otherwise
 */
int  HiCom_Connect ( HiComInterface_t* board );

/**
 * @brief Disconnect a HicomBoard
 * 
 * @param board Pointer to HiCom board to be disconnected
 * @return int 
 */
int  HiCom_Disconnect ( HiComInterface_t*  board );

/**
 * @brief Switch sensor power supply on or off
 * 
 * @param board Pointer to HiCom board to be operated
 * @return int  0 on success, error code otherwise
 */
int  HiCom_SetPower ( HiComInterface_t*  board, bool  on );

/** HiCom implementation of HAL_t::i2cWrite */
int  HiCom_I2CWrite ( HiComInterface_t*  board, uint8_t  slAddr, uint8_t*  wData1, int  wSize1, uint8_t*  wData2, int  wSize2 );

/** HiCom implementation of HAL_t::i2cRead */
int  HiCom_I2CRead ( HiComInterface_t*  board, uint8_t  slAddr, uint8_t*  wData, int  wSize, uint8_t*  rData, int  rSize );

/** Generate a descriptive error message
 * @param [in]  error  Error code
 * @param [in]  scope  Error scope
 * @return      Error string
 */
char const*  HiCom_GetErrorString ( int  error, int  scope, char*  buf, int  bufLen );

/** @} */

#endif /* _HICOM__H_ */
