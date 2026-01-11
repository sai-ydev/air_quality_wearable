/*****************************************************************************
 * Copyright (c) 2024 Renesas Electronics Corporation
 * All Rights Reserved.
 * 
 * This code is proprietary to Renesas, and is license pursuant to the terms and
 * conditions that may be accessed at:
 * https://www.renesas.com/eu/en/document/msc/renesas-software-license-terms-gas-sensor-software
 *****************************************************************************/

/**
 * @file    hsxxxx.c
 * @brief   Renesas humidity sensors (HS3xxx, HS4xxx) abstraction
 * @version 2.7.1
 * @author  Renesas Electronics Corporation
 */

#include <stdlib.h>
#include "hsxxxx.h"
#include "hs3xxx.h"
#include "hs4xxx.h"

int
HSxxxx_Init ( HSxxxx_t*  sensor, Interface_t*  hal ) {
  int err;

  err = HS4xxx_Init ( sensor, hal );
  if ( ! err )  return  err;
  
  err = HS3xxx_Init ( sensor, hal );
  if ( ! err )  return  err;

  sensor -> i2cAddress = 0;
  return err;
}


int
HSxxxx_Measure ( HSxxxx_t*  sensor, HSxxxx_Results_t*  results ) {
  int err = 0;
  
  switch ( sensor -> i2cAddress ) {
  case 0x54:
    err = HS4xxx_Measure ( sensor, results );
    break;

  case 0x44:
    err = HS3xxx_Measure ( sensor, results );
    break;

  default:
    err = 42;
  }

  return err;
}


char const*
HSxxxx_Name ( HSxxxx_t*  sensor ) {
  switch ( sensor -> i2cAddress ) {
  case 0x54:
    return "HS4xxx";
  case 0x44:
    return "HS3xxx";
  default:
    return "Unknown";
  }
}
