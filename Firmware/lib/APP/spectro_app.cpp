/********************************************************
 * @file        	spectro_app.cpp
 * @author      	Junjian Chi (jc2592@cam.ac.uk)
 * @version     	V1.0.0
 * @date        	09/12/2025
 * @brief       	High-level application layer for AS7343
 *
 * @details
 *  - Implements data acquisition and high-level processing
 *  - Abstracts different operating modes (logging / inference)
 *
 * SPDX-License-Identifier: MIT
 ********************************************************/

#include "spectro_app.h"

//==================== Static state ====================//

static SpectroAppMode_t s_appMode = SPECTRO_APP_MODE_DATA_LOG;
static SpectroPrecisionMode_t s_precMode = SPECTRO_PRECISION_MEDIUM;

//==================== Internal helpers (forward decl.) ====================//

static void spectro_app_handle_data_log(const SpectroMeasurement_t *meas);
static void spectro_app_handle_infer_local(const SpectroMeasurement_t *meas);
static void spectro_app_handle_infer_pc(const SpectroMeasurement_t *meas);

//==================== Public API implementation ====================//

void spectro_app_init(void)
{
    s_appMode = SPECTRO_APP_MODE_DATA_LOG;
    s_precMode = SPECTRO_PRECISION_MEDIUM;
    spectro_app_set_precision_mode(s_precMode);
}

void spectro_app_set_mode(SpectroAppMode_t mode)
{
    s_appMode = mode;
}

SpectroAppMode_t spectro_app_get_mode(void)
{
    return s_appMode;
}

void spectro_app_set_precision_mode(SpectroPrecisionMode_t prec)
{
    s_precMode = prec;

    switch (prec)
    {
    case SPECTRO_PRECISION_LOW:
        // Short integration time
        // ATIME=0, ASTEP=999 → ~2.8ms per cycle
        AS7343_set_integration_time(0x00, 999);
        AS7343_set_data_ready_timeout(50);   // 50ms
        break;

    case SPECTRO_PRECISION_MEDIUM:
        AS7343_set_integration_time(0x01, 20000);
        AS7343_set_data_ready_timeout(500);  
        break;

    case SPECTRO_PRECISION_HIGH:
    default:
        AS7343_set_integration_time(0x00, 65534);
        AS7343_set_data_ready_timeout(800);  
        break;
    }
}

SpectroPrecisionMode_t spectro_app_get_precision_mode(void)
{
    return s_precMode;
}


bool spectro_app_acquire(SpectroMeasurement_t *meas)
{
    if (meas == NULL)
        return false;

    // 1) get 18 raw channel
    if (!AS7343_read_all_channels(meas->raw, AS7343_NUM_CHANNELS))
        return false;

    // 2) get 12 channel
    if (!AS7343_get_sorted_spectral_channels(meas->sorted))
        return false;

    return true;
}

void spectro_app_run_once(void)
{
    SpectroMeasurement_t meas;

    if (!spectro_app_acquire(&meas))
    {
        Serial.println(F("[spectro_app] ERROR: Failed to acquire measurement."));
        return;
    }

    switch (s_appMode)
    {
    case SPECTRO_APP_MODE_DATA_LOG:
        spectro_app_handle_data_log(&meas);
        break;

    case SPECTRO_APP_MODE_INFER_LOCAL:
        spectro_app_handle_infer_local(&meas);
        break;

    case SPECTRO_APP_MODE_INFER_PC:
        spectro_app_handle_infer_pc(&meas);
        break;

    default:
        // Fallback: treat as data logging
        spectro_app_handle_data_log(&meas);
        break;
    }
}

//==================== Internal helpers ====================//

/*******************************************************
 * @brief  Mode 0: simple data logging over Serial
 *******************************************************/
static void spectro_app_handle_data_log(const SpectroMeasurement_t *meas)
{
    if (meas == NULL)
        return;

    Serial.print(F("SORTED(405-855nm): "));
    for (int i = 0; i < AS7343_NUM_SORTED_CHANNELS; ++i)
    {
        Serial.print(meas->sorted[i]);
        if (i < AS7343_NUM_SORTED_CHANNELS - 1)
            Serial.print(',');
    }
    Serial.println();

    // 如需调试 raw 通道，也可以顺带打印：
    /*
    Serial.print(F("RAW: "));
    for (int i = 0; i < AS7343_NUM_CHANNELS; ++i)
    {
        Serial.print(meas->raw[i]);
        if (i < AS7343_NUM_CHANNELS - 1)
            Serial.print(',');
    }
    Serial.println();
    */
}

/*******************************************************
 * @brief  Mode 1: local ML inference (on-board)
 *
 * @note   这里先放一个占位实现，后续你可以在此调用
 *         Arduino Nano 33 BLE Sense 上部署的模型 API。
 *******************************************************/
static void spectro_app_handle_infer_local(const SpectroMeasurement_t *meas)
{
    if (meas == NULL)
        return;

    // TODO: 在这里接你的本地模型：
    //  1. 将 meas->sorted (12 维) 归一化 / 转 float
    //  2. 塞进你的 inference 函数
    //  3. 输出分类 / 浓度等结果到串口或 OLED

    Serial.print(F("[spectro_app] Local inference stub. Inputs: "));
    for (int i = 0; i < AS7343_NUM_SORTED_CHANNELS; ++i)
    {
        Serial.print(meas->sorted[i]);
        if (i < AS7343_NUM_SORTED_CHANNELS - 1)
            Serial.print(',');
    }
    Serial.println();
}

/*******************************************************
 * @brief  Mode 2: remote ML inference via PC
 *
 * @details
 *  - 当前简单协议（建议）：
 *      * 发送一行 CSV："MEAS,v0,v1,...,v11\r\n"
 *      * PC 端解析后跑模型
 *      * PC 再返回一条结果行："RES,<whatever>\r\n"
 *
 *  - 这里先实现发送端 + 简单回显，后续你可以根据
 *    实际协议改写。
 *******************************************************/
static void spectro_app_handle_infer_pc(const SpectroMeasurement_t *meas)
{
    if (meas == NULL)
        return;

    // 1) 通过串口发送数据到 PC
    Serial.print(F("MEAS,"));
    for (int i = 0; i < AS7343_NUM_SORTED_CHANNELS; ++i)
    {
        Serial.print(meas->sorted[i]);
        if (i < AS7343_NUM_SORTED_CHANNELS - 1)
            Serial.print(',');
    }
    Serial.println();

    // 2) 可选：等待 PC 返回一行结果
    //    这里暂时只做 echo，占位方便以后扩展协议
    if (Serial.available())
    {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line.length() > 0)
        {
            Serial.print(F("[spectro_app] PC response: "));
            Serial.println(line);
        }
    }
}
