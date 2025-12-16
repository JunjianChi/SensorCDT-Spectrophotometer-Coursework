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




#include "Pimoroni_AS7343.h"

//==================== internal helpers ====================//

// STATUS2.AVALID bit
#define AS7343_STATUS2_AVALID_BIT   (1 << 6)

static uint16_t s_dataReadyTimeoutMs = 100; // global wait time, controlled by spectro_app

/**
 * @brief wait until one time measurement (STATUS2.AVALID = 1)
 * @param s_dataReadyTimeoutMs unit ms
 */
static bool AS7343_wait_data_ready(void)
{
    uint32_t start = millis();
    uint8_t status2 = 0;

    if (!AS7343_set_reg_bank(AS7343_REG_BANK_0))
        return false;

    do
    {
        if (!AS7343_i2c_read_reg(AS7343_I2C_ADDRESS, AS7343_REG_STATUS2, &status2))
            return false;

        if (status2 & AS7343_STATUS2_AVALID_BIT)
            return true;
    }
    while ((uint16_t)(millis() - start) < s_dataReadyTimeoutMs);

    return false; // timeout
}

void AS7343_set_data_ready_timeout(uint16_t timeout_ms)
{
    s_dataReadyTimeoutMs = timeout_ms;
}

//==================== public API implementation ====================//

bool AS7343_init(void)
{
    AS7343_i2c_init();

    // Switch to Bank 0, most configurations are in the 0x80+ region
    if (!AS7343_set_reg_bank(AS7343_REG_BANK_0))
        return false;

    // 1) turn on power PON=1
    uint8_t enable = 0x00;
    if (!AS7343_i2c_read_reg(AS7343_I2C_ADDRESS, AS7343_REG_ENABLE, &enable))
        return false;

    enable |= 0x01; // bit0 = PON
    if (!AS7343_i2c_write_reg(AS7343_I2C_ADDRESS, AS7343_REG_ENABLE, &enable))
        return false;

    delay(3); // datasheet recommends waiting for internal oscillator to stabilize after PON

    // 2) Configure auto_smux = 3 (automatic 18 channel cycling, same as SparkFun example)
    uint8_t cfg20 = 0x00;
    if (!AS7343_i2c_read_reg(AS7343_I2C_ADDRESS, AS7343_REG_CFG20, &cfg20))
        return false;

    cfg20 &= ~(0x3 << 5);   // Clear auto_smux bits [6:5]
    cfg20 |= (0x3 << 5);    // 3: Automatic 18 channel (Cycle1/2/3)
    if (!AS7343_i2c_write_reg(AS7343_I2C_ADDRESS, AS7343_REG_CFG20, &cfg20))
        return false;

    // 3) Set a default gain (16x, commonly used)
    if (!AS7343_set_gain(AS7343_GAIN_16X))
        return false;

    // 4) Finally, enable spectral measurement SP_EN=1
    if (!AS7343_i2c_read_reg(AS7343_I2C_ADDRESS, AS7343_REG_ENABLE, &enable))
        return false;

    enable |= 0x02; // bit1 = SP_EN
    if (!AS7343_i2c_write_reg(AS7343_I2C_ADDRESS, AS7343_REG_ENABLE, &enable))
        return false;

    return true;
}

/*******************************************************
 * Check chip ID to verify connection
 *******************************************************/
bool AS7343_is_connected(void)
{
    uint8_t id = 0;

    if (!AS7343_set_reg_bank(AS7343_REG_BANK_1))
        return false;

    if (!AS7343_i2c_read_reg(AS7343_I2C_ADDRESS, AS7343_REG_ID, &id))
        return false;

    // After reading ID, switch back to Bank 0 to avoid external forgetting to switch
    if (!AS7343_set_reg_bank(AS7343_REG_BANK_0))
        return false;

    return (id == AS7343_DEVICE_ID);
}

/*******************************************************
 * Set register bank
 *******************************************************/
bool AS7343_set_reg_bank(AS7343_RegBank_t bank)
{
    uint8_t cfg0 = 0;

    // CFG0 is located at 0xBF, always in the 0x80+ region, independent of REG_BANK itself
    if (!AS7343_i2c_read_reg(AS7343_I2C_ADDRESS, AS7343_REG_CFG0, &cfg0))
        return false;

    if (bank == AS7343_REG_BANK_1)
        cfg0 |=  (1 << 4);  // REG_BANK = 1
    else
        cfg0 &= ~(1 << 4);  // REG_BANK = 0

    if (!AS7343_i2c_write_reg(AS7343_I2C_ADDRESS, AS7343_REG_CFG0, &cfg0))
        return false;

    return true;
}

/*******************************************************
 * Set sensor gain
 *******************************************************/
bool AS7343_set_gain(AS7343_Gain_t gain)
{
    // gain set CFG1 (0xC6) lower 5 bits, ensure using Bank 0
    if (!AS7343_set_reg_bank(AS7343_REG_BANK_0))
        return false;

    uint8_t cfg1 = 0;
    if (!AS7343_i2c_read_reg(AS7343_I2C_ADDRESS, AS7343_REG_CFG1, &cfg1))
        return false;

    cfg1 &= ~0x1F;               // Clear AGAIN[4:0]
    cfg1 |= (gain & 0x1F);       // Set new gain

    if (!AS7343_i2c_write_reg(AS7343_I2C_ADDRESS, AS7343_REG_CFG1, &cfg1))
        return false;

    return true;
}

/*******************************************************
 * Set integration time (ATIME / ASTEP)
 *******************************************************/
bool AS7343_set_integration_time(uint8_t atime, uint16_t astep)
{
    if (!AS7343_set_reg_bank(AS7343_REG_BANK_0))
        return false;

    // Write ATIME
    if (!AS7343_i2c_write_reg(AS7343_I2C_ADDRESS, AS7343_REG_ATIME, &atime))
        return false;

    // Write ASTEP_L / ASTEP_H
    uint8_t astep_l = astep & 0xFF;
    uint8_t astep_h = (astep >> 8) & 0xFF;

    if (!AS7343_i2c_write_reg(AS7343_I2C_ADDRESS, 0xD4, &astep_l)) // ASTEP_L
        return false;
    if (!AS7343_i2c_write_reg(AS7343_I2C_ADDRESS, 0xD5, &astep_h)) // ASTEP_H
        return false;

    return true;
}

/*******************************************************
 * Read a single 16-bit channel
 *******************************************************/
bool AS7343_read_single_channel(AS7343_Channel_t ch, uint16_t *value)
{
    if (value == NULL)
        return false;

    if (!AS7343_set_reg_bank(AS7343_REG_BANK_0))
        return false;

    // Wait for current measurement to complete
    if (!AS7343_wait_data_ready())
        return false;

    uint8_t raw[2] = {0};
    uint8_t reg = AS7343_REG_DATA0_L + (ch * 2);

    if (!AS7343_i2c_read(AS7343_I2C_ADDRESS, reg, raw, 2))
        return false;

    *value = ((uint16_t)raw[1] << 8) | raw[0];

    return true;
}

/*******************************************************
 * Read all 18 Data registers in a single loop
 *******************************************************/
bool AS7343_read_all_channels(uint16_t *data, size_t length)
{
    if ((data == NULL) || (length < AS7343_NUM_CHANNELS))
        return false;

    // Data registers are in Bank 0
    if (!AS7343_set_reg_bank(AS7343_REG_BANK_0))
        return false;

    // Wait for current measurement to complete
    if (!AS7343_wait_data_ready())
        return false;

    uint8_t raw[2] = {0};
    uint8_t reg = AS7343_REG_DATA0_L;

    for (uint8_t i = 0; i < AS7343_NUM_CHANNELS; i++, reg += 2)
    {
        if (!AS7343_i2c_read(AS7343_I2C_ADDRESS, reg, raw, 2))
            return false;

        data[i] = ((uint16_t)raw[1] << 8) | raw[0];
    }

    return true;
}

/*******************************************************
 * Extract 12 spectral channels (sorted by wavelength)
 * 405 → 855 nm : F1,F2,FZ,F3,F4,F5,FY,FXL,F6,F7,F8,NIR
 * Caller at least 12 uint16_t values buffer
 *******************************************************/
bool AS7343_get_sorted_spectral_channels(uint16_t *data11)
{
    if (data11 == NULL)
        return false;

    uint16_t raw[AS7343_NUM_CHANNELS];

    if (!AS7343_read_all_channels(raw, AS7343_NUM_CHANNELS))
        return false;

    data11[0]  = raw[AS7343_CH_PURPLE_F1_405NM];     // F1 405nm
    data11[1]  = raw[AS7343_CH_DARK_BLUE_F2_425NM];  // F2 425nm
    data11[2]  = raw[AS7343_CH_BLUE_FZ_450NM];       // FZ 450nm
    data11[3]  = raw[AS7343_CH_LIGHT_BLUE_F3_475NM]; // F3 475nm
    data11[4]  = raw[AS7343_CH_BLUE_F4_515NM];       // F4 515nm
    data11[5]  = raw[AS7343_CH_GREEN_F5_550NM];      // F5 550nm
    data11[6]  = raw[AS7343_CH_GREEN_FY_555NM];      // FY 555nm
    data11[7]  = raw[AS7343_CH_ORANGE_FXL_600NM];    // FXL 600nm
    data11[8]  = raw[AS7343_CH_BROWN_F6_640NM];      // F6 640nm
    data11[9]  = raw[AS7343_CH_RED_F7_690NM];        // F7 690nm
    data11[10] = raw[AS7343_CH_DARK_RED_F8_745NM];   // F8 745nm
    data11[11] = raw[AS7343_CH_NIR_855NM];           // NIR 855nm

    return true;
}