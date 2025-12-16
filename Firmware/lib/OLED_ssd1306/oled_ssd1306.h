/********************************************************
 * @file        	oled_ssd1306.h
 * @author      	Junjian Chi (jc2592@cam.ac.uk)
 * @version     	V1.0.0
 * @date        	07/12/2025
 * @brief       	Driver for a 7-pin OLED display
 *
 * @details
 *  - Declaration of initialization and draw functions for ssd1306 OLED
 *  - 128x64 resolution
 * 
 * SPDX-License-Identifier: MIT
 ********************************************************/

#ifndef OLED_SSD1306_H
#define OLED_SSD1306_H

#include "ssd1306_spi_interface.h"
#include "spectro_app.h"

#define SIZE        16
#define Max_Columns 128
#define Max_Rows    64 // resolution of the ssd1306
#define X_WIDTH     128
#define Y_WIDTH     64 // UI coordinates (can be changed later)

extern void oled_ssd1306_setup(void);

extern void oled_clear(void);
extern void oled_clear_lines(unsigned char lineStart, unsigned char lineEnd);

extern void oled_scroll(void);

extern void oled_color_turn(bool status);
extern void oled_display_turn(bool status);
extern void oled_display_status(bool status);

extern void oled_set_position(unsigned char x, unsigned char y);

// Draw functions

extern void oled_test(void); // Test function
extern void oled_show_char(unsigned char x, unsigned char y, const unsigned char chr, unsigned char sizey); // 
extern void oled_show_num(unsigned char x, unsigned char y, unsigned int num, unsigned char len, unsigned char sizey); // len for xxx showing width; sizey for bits
extern void oled_show_string(unsigned char x, unsigned char y, const char *str, unsigned char sizey);
extern void oled_draw_diagram(unsigned char x, unsigned char y, const unsigned char sizex, unsigned char sizey, const unsigned char BMP[]);

// Specific functions in this task
extern void oled_draw_start_go(void);
extern void oled_show_mode(void);

#endif