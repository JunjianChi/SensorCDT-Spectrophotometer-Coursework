/********************************************************
 * @file        	spectro_app.h
 * @author      	Junjian Chi (jc2592@cam.ac.uk)
 * @version     	V1.0.0
 * @date        	09/12/2025
 * @brief       	High-level application layer for AS7343
 *
 * @details
 *  - Encapsulates application logic on top of the AS7343 driver
 *  - Supports multiple modes:
 *      * Raw data acquisition
 *      * Local ML inference on Arduino
 *      * Remote ML inference via PC over serial
 *
 * SPDX-License-Identifier: MIT
 ********************************************************/

#ifndef SPECTRO_APP_H
#define SPECTRO_APP_H

#include <Arduino.h>
#include "Pimoroni_AS7343.h"

//==================== Application modes ====================//

/**
 * @brief High-level operating mode of the spectrophotometer
 */
typedef enum
{
    SPECTRO_APP_MODE_DATA_LOG = 0,   ///< Pure data acquisition: print spectral channels
    SPECTRO_APP_MODE_INFER_LOCAL,    ///< Run on-board ML model (e.g. Nano 33 BLE Sense)
    SPECTRO_APP_MODE_INFER_PC        ///< Send data to host PC, wait for inference result
} SpectroAppMode_t;

typedef enum
{
    SPECTRO_PRECISION_LOW = 0,     
    SPECTRO_PRECISION_MEDIUM,      
    SPECTRO_PRECISION_HIGH         
} SpectroPrecisionMode_t;

//==================== Measurement container ====================//

/**
 * @brief Container for a single AS7343 measurement
 *
 * @details
 *  - raw[0..17]   : 18 hardware channels as read from device
 *  - sorted[0..11]: 12 spectral channels sorted by wavelength (405 → 855nm)
 */
typedef struct
{
    uint16_t raw[AS7343_NUM_CHANNELS];
    uint16_t sorted[AS7343_NUM_SORTED_CHANNELS];
} SpectroMeasurement_t;

//==================== Public API ====================//

/**
 * @brief Initialise application-layer state.
 */
void spectro_app_init(void);

/**
 * @brief Set current application mode.
 */
void spectro_app_set_mode(SpectroAppMode_t mode);

/**
 * @brief Get current application mode.
 */
SpectroAppMode_t spectro_app_get_mode(void);

/*
 * @brief Set current precision mode
 */
void spectro_app_set_precision_mode(SpectroPrecisionMode_t prec);

/*
 * @brief Get current precision mode
 */
SpectroPrecisionMode_t spectro_app_get_precision_mode(void);

/**
 * @brief Acquire one measurement from AS7343.
 *
 * @param[out] meas   Pointer to SpectroMeasurement_t to fill.
 * @return true on success
 */
bool spectro_app_acquire(SpectroMeasurement_t *meas);

/**
 * @brief Perform one high-level application step.
 *
 * @details
 *  - Acquires one measurement from the sensor.
 *  - Dispatches processing depending on current mode:
 *      * DATA_LOG     : print channels via Serial
 *      * INFER_LOCAL  : run on-board model (TODO stub)
 *      * INFER_PC     : send to PC and read back result (TODO stub)
 *
 *  - Intended to be called from loop().
 */
void spectro_app_run_once(void);

#endif // SPECTRO_APP_H
