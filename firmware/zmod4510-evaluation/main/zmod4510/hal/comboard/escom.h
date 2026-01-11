/*****************************************************************************
 * Copyright (c) 2024 Renesas Electronics Corporation
 * All Rights Reserved.
 * 
 * This code is proprietary to Renesas, and is license pursuant to the terms and
 * conditions that may be accessed at:
 * https://www.renesas.com/eu/en/document/msc/renesas-software-license-terms-gas-sensor-software
 *****************************************************************************/

/**
 * @addtogroup escom_api
 * @ingroup    hal_api
 * @{
 * @file    escom.h
 * @brief   ESCom board type and function declarations
 * @version 2.7.1
 * @author  Renesas Electronics Corporation
 */

#ifndef _ESCOM__H_
#define _ESCOM__H_

#include <stdint.h>
#include <stdbool.h>

#include "hal/hal.h"
#include "hal/3rdParty/libusb/libusb.h"

/** ESCom error scopes
 * 
 * These codes identify the component that generated an error code
 */
typedef enum {
  iesLibUSB    = 0x11,
  iesI2C       = 0x12,
  iesCommand   = 0x14
} ESComErrorScope_t;


/** Data structure containing ESCom board information */
typedef struct {
  libusb_device*         device;
  libusb_device_handle*  handle;
  uint8_t                outEP;
  uint8_t                inEP;
  wchar_t                serial [ 33 ];
} ESComInterface_t;

typedef enum { emUSB, emEVK }  ESComMode_t;

/** Enumerate ESCom boards connected to PC
 *
 * Scan available USB interfaces and save those with matching VID/PID in the
 *  `boards` buffer. Value pointed to by `count` serves as input and output.
 *  As input, `count` specifies the size maximum number of boards to be
 *  stored in `boards`. The buffer must provide sufficient space.
 * Before return, this function stores the number of detected EScom boards in
 *  \a count.
 *
 * @param  [in]     boards Pointer to ::ESComInterface_t objects
 * @param  [in,out] count  Maximum number of boards to be stored in `board` [in],
 *                         actual number of boards stored in `board` [out]
 * @return  0 on success or error code on failure
 */
int  ESCom_Find ( ESComInterface_t*  boards, int*  count );

/** Connect ESCom board
 *
 * This function tries to connect the ESCom board identified by `board` and
 *  allocates the required resources.
 *
 * @param [in] board Pointer to an ::EScomInterface_t instance obtained through
 *                   ESCom_Find()
 * @return   0 on success or error code on failure
 */
int  ESCom_Connect(ESComInterface_t*  board);

/** Disconnect ESCom board and free associated resources
 *
 * @param [in] board Pointer to an ::EScomInterface_t instance
 * @return   0 on success or error code on failure
 */
int  ESCom_Disconnect(ESComInterface_t*  board);

/** Switch on or off the internal sensor supply voltage
 *
 * @param [in] board Pointer to an ::ESComInsteface_t instance obtained
 * @param [in] on    Boolean indicating whether the sensor supply voltage shall be switched on
 * @return   0 on success or error code otherwise
 */
int  ESCom_SetPower(ESComInterface_t*  board, bool  on);

/** This is the ESCom implementation of HAL_t::i2cWrite */
int  ESCom_I2CWrite(ESComInterface_t*  board, uint8_t  slAddr, uint8_t*  wData1, int  wSize1, uint8_t*  wData2, int  wSize2 );

/** This is the ESCom implementation of HAL_t::i2cRead */
int  ESCom_I2CRead(ESComInterface_t*  board, uint8_t  slAddr, uint8_t*  wData, int  wSize, uint8_t*  rData, int  rSize );

/** Read out the measured sensor voltage 
 *  @param [in]  board Pointer to an ::ESComInsteface_t instance
 *  @param [out] voltage voltage value measured by ESCom board
 */
int  ESCom_GetSensorVoltage(ESComInterface_t*  board, float*  voltage );

/** Generate a descriptive error message
 * @param [in]  error  Error code
 * @param [in]  scope  Error scope
 * @return      Error string
 */
char const*  ESCom_GetErrorString ( int  error, int  scope, char*  buf, int  bufLen );

/** @} */

#endif /* _ESCOM__H_ */
