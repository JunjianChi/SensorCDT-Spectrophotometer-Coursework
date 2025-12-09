/********************************************************
 * @file        	oled_ssd1306.cpp
 * @author      	Junjian Chi
 * @version     	V1.0.0
 * @date        	07/12/2025
 * @brief       	Driver for a 7-pin OLED display
 *
 * @details
 *  - Implementation of initialization and draw functions for ssd1306 OLED
 *  - 128x64 resolution
 * 
 * SPDX-License-Identifier: MIT
 ********************************************************/

#include "oled_ssd1306.h"
#include "oled_font_picture.h"


//*******************************Initialize static function***************************************
#define INIT_WAY2

#define SCROLL_DATA 10
#define DISPLAY_DATA 6

#ifdef INIT_WAY1
    #define INIT_DATA 25
    static uint8_t oled_init_data[INIT_DATA] = 
    {
        0xAE, 
        0x00, 
        0x10, 
        0x40,
        0xB0,
        0x81, 
        0xFF, 
        0xA1,  
        0xA6, 
        0xA8, 
        0x3F,
        0xC8,
        0xD3, 
        0x00, 
        0xD5, 
        0x80, 
        0xD9, 
        0xF1, 
        0xDA, 
        0x12,
        0xDB, 
        0x40, 
        0x8D, 
        0x14, 
        0xAF
    };
#endif

#ifdef INIT_WAY2
    #define INIT_DATA 28
    static uint8_t oled_init_data[INIT_DATA] = 
    {
        0xAE, // turn off oled panel
        0x00, // set low column address
        0x10, // set high column address
        0x40, // set start line address  Set Mapping RAM Display Start Line (0x00~0x3F)
        0x81, // set contrast control register
        0xCF, // set SEG Output Current Brightness
        0xA1, // set SEG/Column Mapping     0xa0 reverse 0xa1 normal
        0xC8, // set COM/Row Scan Direction   0xc0 reverse 0xc8 normal
        0xA6, // set normal display
        0xA8, // set multiplex ratio(1 to 64)
        0x3F, // 1/64 duty
        0xD3, // set display offset Shift Mapping RAM Counter (0x00~0x3F)
        0x00, // not offset
        0xD5, // set display clock divide ratio/oscillator frequency
        0x80, // set divide ratio, Set Clock as 100 Frames/Sec
        0xD9, // set pre-charge period
        0xF1, // set Pre-Charge as 15 Clocks & Discharge as 1 Clock
        0xDA, // set com pins hardware configuration
        0x12,
        0xDB, // set vcomh
        0x40, // set VCOM Deselect Level 0x30 some programme
        0x20, // set Page Addressing Mode (0x00/0x01/0x02)
        0x02,
        0x8D, // set Charge Pump enable/disable
        0x14, // set(0x10) disable
        0xA4, // disable Entire Display On (0xa4/0xa5)
        0xA6, // disable Inverse Display On (0xa6/a7)
        0xAF
    };
#endif

static uint8_t oled_scroll_data[SCROLL_DATA] = 
{
    0x2E,
    0x27,
    0x00,
    0x00,
    0x00,
    0x00,
    0x01,
    0x00,
    0xFF,
    0x2F
};

static uint8_t oled_display_data[DISPLAY_DATA] = {
    0x8D,
    0x14,
    0xAF,
    0x8D,
    0x10,
    0xAE
}; 


//*******************************Driver function***************************************

/**
 * @brief write byte spi
 */
void oled_write_byte(unsigned char dat, unsigned char cmd)
{
    if (cmd == OLED_DATA)
    {
        write_byte_data(dat);
    }
    else if (cmd == OLED_CMD)
    {
        write_byte_cmd(dat);
    }
}

/**
 * @brief clean the screen
*/
void oled_clear(void)
{
    uint8_t i, j;
    for (i = 0; i < 8; i++)
    {
        oled_write_byte(0xb0 + i, OLED_CMD);
        oled_write_byte(0x00, OLED_CMD);
        oled_write_byte(0x10, OLED_CMD);
        for (j = 0; j < 128; j++)
        {
            oled_write_byte(0, OLED_DATA);
        }
    }
}




/**
 * @brief clean the screen lines
*/
void oled_clear_lines(unsigned char lineStart, unsigned char lineEnd)
{
    uint8_t i, j;
    for (i = lineStart; i < lineEnd; i++)
    {
        oled_write_byte(0xb0 + i, OLED_CMD);
        oled_write_byte(0x00, OLED_CMD);
        oled_write_byte(0x10, OLED_CMD);
        for (j = 0; j < 128; j++)
        {
            oled_write_byte(0, OLED_DATA);
        }
    }
}

/**
 * @brief set the pixel position on the OLED
*/
void oled_set_position(unsigned char x, unsigned char y)
{
    oled_write_byte(0xb0 + y, OLED_CMD);
    oled_write_byte(((x & 0xf0) >> 4) | 0x10, OLED_CMD);
    oled_write_byte((x & 0x0f) | 0x01, OLED_CMD);
}

/**
 * @brief Screen colour trun
 */
void oled_color_turn(bool status)
{
    if (status == false)
    {
        oled_write_byte(0xA6, OLED_CMD); // normal
    }
    else if (status == true)
    {
        oled_write_byte(0xA7, OLED_CMD); // reverse
    }
}

/**
 * @brief Screen rotation 180 degrees
 */
void oled_display_turn(bool status)
{
    if (status == false)
    {
        oled_write_byte(0xC8, OLED_CMD); // normal
        oled_write_byte(0xA1, OLED_CMD);
    }
    else if (status == true)
    {
        oled_write_byte(0xC0, OLED_CMD); // turn 180 degrees
        oled_write_byte(0xA0, OLED_CMD);
    }
}

/**
 * @brief OLED scroll function, range 0 to 1 page, horizontal to left
 */
void oled_scroll(void)
{
    uint8_t i;
    for (i = 0; i < SCROLL_DATA; i++)
    {
        oled_write_byte(oled_scroll_data[i], OLED_CMD);
    }
}

/**
 * @brief open and close the display
 */
void oled_display_status(bool status)
{
    uint8_t i;
    // on
    if (status == true)
    {
        for (i = 0; i < DISPLAY_DATA / 2; i++)
        {
            oled_write_byte(oled_display_data[i], OLED_CMD);  
        }
    }
    // off
    else if (status == false)
    {
        for (i = 3; i < DISPLAY_DATA; i++)
        {
            oled_write_byte(oled_display_data[i], OLED_CMD);
        }
    }
}

/**
 * @remark should be init after the spi interface init
 */
void oled_ssd1306_init(void)
{
    uint8_t i;
    oled_spi_reset();
    for (i = 0; i < INIT_DATA; i++)
    {
        oled_write_byte(oled_init_data[i], OLED_CMD);
    }
    oled_clear();
    // oled_set_position(0, 0);
}

/**
 * @brief the set up for all (spi and oled)
*/
void oled_ssd1306_setup(void)
{
    oled_spi_init();
    oled_ssd1306_init();
    oled_display_turn(false);
    oled_color_turn(false);
    oled_display_status(true);
}

//*******************************draw***************************************

/**
 * @brief Display a character
 */
void oled_show_char(unsigned char x, unsigned char y, const unsigned char chr, unsigned char sizey)
{
    unsigned char c = 0, sizex = sizey / 2, temp;
    unsigned short i = 0, size1;
    if (sizey == 8)
        size1 = 6;
    else
        size1 = (sizey / 8 + ((sizey % 8) ? 1 : 0)) * (sizey / 2);
    c = chr - ' ';
    oled_set_position(x, y);
    for (i = 0; i < size1; i++)
    {
        if (i % sizex == 0 && sizey != 8)
            oled_set_position(x, y++);
        if (sizey == 8)
        {
            temp = asc2_0806[c][i];
            oled_write_byte(temp, OLED_DATA); // 6X8 size
        }        
        else if (sizey == 12)
        {
            temp = asc2_1206[c][i];
            oled_write_byte(temp, OLED_DATA); // 6x12 size
        }
        else if (sizey == 16)
        {
            temp = asc2_1608[c][i];
            oled_write_byte(temp, OLED_DATA); // 8x16 size
        }
        else if (sizey == 32)
        {
            temp = asc2_2412[c][i];
            oled_write_byte(temp, OLED_DATA); // 12x24 size
        }
        else
            return;
    }
}

/**
 * @brief calculated m^n
 */
unsigned int oled_pow(unsigned char m, unsigned char n)
{
    unsigned int result = 1;
    while (n--)
        result *= m;
    return result;
}

/**
 * @brief Display a number of the specified length at the specified position
 */
void oled_show_num(unsigned char x, unsigned char y, unsigned int num, unsigned char len, unsigned char sizey)
{
    unsigned char t, temp, m = 0;
    unsigned char enshow = 0;
    if (sizey == 8)
        m = 2;
    for (t = 0; t < len; t++)
    {
        temp = (num / oled_pow(10, len - t - 1)) % 10;
        if (enshow == 0 && t < (len - 1))
        {
            if (temp == 0)
            {
                oled_show_char(x + (sizey / 2 + m) * t, y, ' ', sizey);
                continue;
            }
            else
                enshow = 1;
        }
        oled_show_char(x + (sizey / 2 + m) * t, y, temp + '0', sizey);
    }
}

/**
 * @brief Display a number of the specified length at the specified position
 */
void oled_show_string(unsigned char x, unsigned char y, const char *chr, unsigned char sizey)
{
    unsigned char j = 0;
    while (chr[j] != '\0')
    {
        oled_show_char(x, y, chr[j++], sizey);
        if (sizey == 8)
            x += 6;
        else
            x += sizey / 2;
    }
}

/**
 * @brief Show images
 */
void oled_draw_diagram(unsigned char x, unsigned char y, unsigned char sizex, unsigned char sizey, const unsigned char BMP[])
{
    unsigned short j = 0;
    unsigned char i, m, temp;
    sizey = sizey / 8 + ((sizey % 8) ? 1 : 0);
    for (i = 0; i < sizey; i++)
    {
        oled_set_position(x, i + y);
        for (m = 0; m < sizex; m++)
        {
            temp = BMP[j++];
            oled_write_byte(temp, OLED_DATA);
        }
    }
}

/**
 * @brief a loop show test
 */
void oled_test(void)
{
    int i,j;
    char show_time[60] = {0};

    oled_draw_diagram(0, 0, 128, 64, image_test);
    delay(2000);
    oled_clear();

    oled_show_string(45, 0, "Alfred", 8);
    i = 0;
    j = 0;
    while(true)
    {
        sprintf(show_time, "%d : %d", j, i);

        oled_show_string(45, 4, show_time, 16);
        i++;
        if(i == 60){
            i = 0;
            j++;
            oled_clear_lines(3,8);
        }
        if(j == 60){
            j = 0;
            oled_clear_lines(3,8);
        }
        delay(1000);
    }
}


void oled_draw_start_go(void)
{
    oled_clear();
    oled_show_string(10, 0, "Program Starts", 16); 
    delay(1500);

    // couting
    for (int num = 3; num > 0; num--)
    {
      oled_clear();
      oled_show_num(60, 3, num, 1, 16);  
      delay(1000);
    }

    oled_clear();
    oled_show_string(52, 3, "GO!", 16); 
    delay(1000);
    oled_clear();
}


void oled_show_mode(void){
  if (spectro_app_get_mode() == SPECTRO_APP_MODE_DATA_LOG) {
    oled_show_string(45, 0, "Mode", 16);
    oled_show_string(35, 2, "Data Log", 16);
  } else if (spectro_app_get_mode() == SPECTRO_APP_MODE_INFER_LOCAL) {
    oled_show_string(45, 0, "Mode", 16);
    oled_show_string(35, 2, "Infer Local", 16);
  } else if (spectro_app_get_mode() == SPECTRO_APP_MODE_INFER_PC) {
    oled_show_string(45, 0, "Mode", 16);
    oled_show_string(35, 2, "Infer PC", 16);
  }
}























