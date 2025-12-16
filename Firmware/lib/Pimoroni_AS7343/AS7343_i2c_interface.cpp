/********************************************************
 * @file        	AS7343_i2c_interface.cpp
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

#include "AS7343_i2c_interface.h"

#define AS7343_INT A1

void AS7343_i2c_init(void) {
    Wire.begin(); // Arduino I2C init
    Wire.setClock(100000); // Set I2C frequency to 100kHz
}

bool AS7343_i2c_write(uint8_t dev_address,uint8_t reg, uint8_t *data, size_t length) {
    Wire.beginTransmission(dev_address);
    Wire.write(reg);
    Wire.write(data, length);
    uint8_t err = Wire.endTransmission();
    return (err == 0); // if sucess return 1
}


bool AS7343_i2c_read(uint8_t dev_address, uint8_t reg, uint8_t *data, size_t length) {
    // For AS7343 data is 2 bytes
    Wire.beginTransmission(dev_address);
    Wire.write(reg); // write the register address
    uint8_t err = Wire.endTransmission();

    if (err != 0) return false;

    uint8_t n = Wire.requestFrom(dev_address, (uint8_t) length);
    
    if (n != (uint8_t) length){
        Serial.print("Error: Requested ");
        Serial.print(length);
        Serial.print(" bytes but received ");
        Serial.print(n);
        Serial.println(" bytes");
        return false;
    }

    for (size_t i = 0; i < length; i++) {
        data[i] = Wire.read();
    }
    return true;
}

bool AS7343_i2c_write_reg(uint8_t dev_address,uint8_t reg, uint8_t *value) {
    return AS7343_i2c_write(dev_address, reg, value, 1);
}

bool AS7343_i2c_read_reg(uint8_t dev_address,uint8_t reg, uint8_t *value) {
    return AS7343_i2c_read(dev_address, reg, value, 1);
}

