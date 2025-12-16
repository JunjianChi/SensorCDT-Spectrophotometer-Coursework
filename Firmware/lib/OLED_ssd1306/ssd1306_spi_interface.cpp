/********************************************************
 * @file        	ssd1306_spi_interface.cpp
 * @author      	Junjian Chi (jc2592@cam.ac.uk)
 * @version     	V1.0.0
 * @date        	07/12/2025
 * @brief       	Implementation of SPI interface functions for ssd1306 OLED
 *
 * @details
 *  - OLED SCK	D13
 *  - OLED COPI	D11
 *  - OLED RES	A0
 *  - OLED DC	A7
 *  - OLED CS	A6
 * 
 * SPDX-License-Identifier: MIT
 ********************************************************/
    
#include "ssd1306_spi_interface.h"


// SPI GPIO SETTING (Except default SPI pins)
#define OLED_RES A0   // RES
#define OLED_DC A7    // DC
#define OLED_CS A6    // CS


void write_byte_cmd(unsigned char dat)
{
    digitalWrite(OLED_DC, LOW);
    digitalWrite(OLED_CS, LOW);
    SPI.transfer(dat);
    digitalWrite(OLED_CS, HIGH);
}

void write_byte_data(unsigned char dat)
{
    digitalWrite(OLED_DC, HIGH);
    digitalWrite(OLED_CS, LOW);
    SPI.transfer(dat);
    digitalWrite(OLED_CS, HIGH);
}

void oled_spi_init(void)
{
    // GPIO Initialisation
    pinMode(OLED_RES, OUTPUT);
    pinMode(OLED_DC, OUTPUT);
    pinMode(OLED_CS, OUTPUT);

    // SPI Initialisation
    SPI.begin();
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
}

void oled_spi_reset(void)
{
    digitalWrite(OLED_RES, HIGH);
    delay(200);
    digitalWrite(OLED_RES, LOW);
    delay(200);
    digitalWrite(OLED_RES, HIGH);
    delay(200);
}