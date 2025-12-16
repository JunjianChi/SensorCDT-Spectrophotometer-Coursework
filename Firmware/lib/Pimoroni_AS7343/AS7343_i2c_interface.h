/********************************************************
 * @file        	AS7343_i2c_interface.h
 * @author      	Junjian Chi (jc2592@cam.ac.uk)
 * @version     	V1.0.0
 * @date        	07/12/2025
 * @brief       	Declaration of I2C interface R/W functions for AS7343 spectral sensor
 *
 * @details
 *  - AS7343 SDA	A4 (Default)
 *  - AS7343 SCL	A5 (Default)
 *  - AS7343 INT	A1
 * 
 * SPDX-License-Identifier: MIT
 ********************************************************/

#ifndef AS7343_I2C_INTERFACE_H
#define AS7343_I2C_INTERFACE_H

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Arduino libs
#include <Arduino.h>
#include <Wire.h>
#include <I2C.h> // not used here
#include <Serial.h>

extern void AS7343_i2c_init(void);

// Read and write from register with one byte
extern bool AS7343_i2c_write_reg(uint8_t dev_address, uint8_t reg, uint8_t *value);
extern bool AS7343_i2c_read_reg(uint8_t dev_address, uint8_t reg, uint8_t *value);

// read and write from register in total, return 1 for success
extern bool AS7343_i2c_write(uint8_t dev_address, uint8_t reg, uint8_t *data, size_t length);
extern bool AS7343_i2c_read(uint8_t dev_address, uint8_t reg, uint8_t *data, size_t length);

#endif

