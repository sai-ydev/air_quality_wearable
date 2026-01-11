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
 * @file    rpi.h
 * @brief   Raspberry PI HAL type and function declarations
 * @version 2.7.1
 * @author  Renesas Electronics Corporation
 */

#ifndef RPI_H
#define RPI_H

#include <stdint.h>

typedef enum {
  resPiGPIO         = 0x310000,
  resI2C            = 0x320000,
  recI2CLenMismatch = 0x320001
} RPiErrorDefs_t;

#endif /* RPI_H */

/** @} */
