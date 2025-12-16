/********************************************************
 * @file        	Pimoroni_AS7343.h
 * @author      	Junjian Chi (jc2592@cam.ac.uk)
 * @version     	V1.0.0
 * @date        	07/12/2025
 * @brief       	Driver for the Pimoroni AS7343 spectral sensor
 *
 * @details
 *  - Declaration of initialization and control functions for Pimoroni AS7343
 *  - Spectral sensor with 14 multiple channels
 * 
 * SPDX-License-Identifier: MIT
 ********************************************************/

#ifndef PIMORONI_AS7343_H
#define PIMORONI_AS7343_H

#include "AS7343_i2c_interface.h"


//==================== device address & ID ====================//

#define AS7343_I2C_ADDRESS   0x39
#define AS7343_DEVICE_ID     0x81

//==================== Bank 1 registers (0x58~0x66) ====================//

#define AS7343_REG_AUXID     0x58
#define AS7343_REG_REVID     0x59
#define AS7343_REG_ID        0x5A

//==================== Bank 0 registers (0x80+) ====================//

#define AS7343_REG_ENABLE    0x80
#define AS7343_REG_ATIME     0x81
#define AS7343_REG_WTIME     0x83
#define AS7343_REG_SP_TH_L   0x84
#define AS7343_REG_SP_TH_H   0x86
#define AS7343_REG_STATUS2   0x90
#define AS7343_REG_STATUS    0x93
#define AS7343_REG_CFG0      0xBF
#define AS7343_REG_CFG1      0xC6
#define AS7343_REG_CFG20     0xD6   // auto_smux 
#define AS7343_REG_ASTEP_L   0xD4
#define AS7343_REG_ASTEP_H   0xD5
//==================== Channel data registers (Bank 0) ====================//

#define AS7343_REG_DATA0_L   0x95
#define AS7343_REG_DATA1_L   0x97
#define AS7343_REG_DATA2_L   0x99
#define AS7343_REG_DATA3_L   0x9B
#define AS7343_REG_DATA4_L   0x9D
#define AS7343_REG_DATA5_L   0x9F
#define AS7343_REG_DATA6_L   0xA1
#define AS7343_REG_DATA7_L   0xA3
#define AS7343_REG_DATA8_L   0xA5
#define AS7343_REG_DATA9_L   0xA7
#define AS7343_REG_DATA10_L  0xA9
#define AS7343_REG_DATA11_L  0xAB
#define AS7343_REG_DATA12_L  0xAD
#define AS7343_REG_DATA13_L  0xAF
#define AS7343_REG_DATA14_L  0xB1
#define AS7343_REG_DATA15_L  0xB3
#define AS7343_REG_DATA16_L  0xB5
#define AS7343_REG_DATA17_L  0xB7

//==================== Channel indices (0~17) ====================//

typedef enum {
    AS7343_CH_BLUE_FZ_450NM = 0,   // Data0
    AS7343_CH_GREEN_FY_555NM,      // Data1
    AS7343_CH_ORANGE_FXL_600NM,    // Data2
    AS7343_CH_NIR_855NM,           // Data3
    AS7343_CH_VIS_1,               // Data4 (clear / VIS)
    AS7343_CH_FD_1,                // Data5 (flicker detect)
    AS7343_CH_DARK_BLUE_F2_425NM,  // Data6
    AS7343_CH_LIGHT_BLUE_F3_475NM, // Data7
    AS7343_CH_BLUE_F4_515NM,       // Data8
    AS7343_CH_BROWN_F6_640NM,      // Data9
    AS7343_CH_VIS_2,               // Data10
    AS7343_CH_FD_2,                // Data11
    AS7343_CH_PURPLE_F1_405NM,     // Data12
    AS7343_CH_RED_F7_690NM,        // Data13
    AS7343_CH_DARK_RED_F8_745NM,   // Data14
    AS7343_CH_GREEN_F5_550NM,      // Data15
    AS7343_CH_VIS_3,               // Data16
    AS7343_CH_FD_3                 // Data17
} AS7343_Channel_t;

#define AS7343_NUM_CHANNELS          18
#define AS7343_NUM_SORTED_CHANNELS   12  // 11 VIS bands + 1 NIR

//==================== bank choice ====================//

typedef enum {
    AS7343_REG_BANK_0 = 0x00, // Access 0x80+
    AS7343_REG_BANK_1 = 0x01  // Access 0x58~0x66
} AS7343_RegBank_t;

//==================== gain settings ====================//

typedef enum {
    AS7343_GAIN_0_5X = 0x00,
    AS7343_GAIN_1X,
    AS7343_GAIN_2X,
    AS7343_GAIN_4X,
    AS7343_GAIN_8X,
    AS7343_GAIN_16X,
    AS7343_GAIN_32X,
    AS7343_GAIN_64X,
    AS7343_GAIN_128X,
    AS7343_GAIN_256X,
    AS7343_GAIN_512X,
    AS7343_GAIN_1024X,
    AS7343_GAIN_2048X
} AS7343_Gain_t;

//==================== public API ====================//

bool AS7343_init(void);
bool AS7343_is_connected(void);
bool AS7343_set_reg_bank(AS7343_RegBank_t bank);
bool AS7343_set_gain(AS7343_Gain_t gain);
bool AS7343_read_all_channels(uint16_t *data, size_t length);
bool AS7343_read_single_channel(AS7343_Channel_t ch, uint16_t *value);
/**
 * @brief  F1~F8 + FZ + FY + NIR）
 * @note   Must ensure data11 can hold at least 12 uint16_t values
 */
bool AS7343_get_sorted_spectral_channels(uint16_t *data11);

bool AS7343_set_integration_time(uint8_t atime, uint16_t astep); // different resolution readout
void AS7343_set_data_ready_timeout(uint16_t timeout_ms);
#endif // PIMORONI_AS7343_H
