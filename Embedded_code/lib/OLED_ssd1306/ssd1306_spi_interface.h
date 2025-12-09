/********************************************************
 * @file        	ssd1306_spi_interface.h
 * @author      	Junjian Chi (jc2592@cam.ac.uk)
 * @version     	V1.0.0
 * @date        	07/12/2025
 * @brief       	Declaration of SPI interface functions for ssd1306 OLED
 *
 * @details
 *  - OLED SCK	D13 (Default)
 *  - OLED COPI	D11 (Default)
 *  - OLED RES	A0
 *  - OLED DC	A7
 *  - OLED CS	A6
 * 
 * SPDX-License-Identifier: MIT
 ********************************************************/

#ifndef SSD1306_SPI_INTERFACE_H
#define SSD1306_SPI_INTERFACE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
// Arduino libs
#include <Arduino.h>
#include <SPI.h>

#define OLED_CMD 0
#define OLED_DATA 1

extern void oled_spi_init(void);
extern void write_byte_cmd(unsigned char dat);
extern void write_byte_data(unsigned char dat);
extern void oled_spi_reset(void);


#endif




