/********************************************************
 * @file        	main.c
 * @author      	Junjian Chi (jc2592@cam.ac.uk)
 * @version     	V1.0.0
 * @date        	07/12/2025
 * @brief       	Arduino-spectrophotometer main file
 * 
 * @details
 *  -This file initializes the system and runs the main loop.
 *  -High-level scheduling and application logic are located here.
 * 
 * @note 
 *   Three program modes name
 *  -SPECTRO_APP_MODE_DATA_LOG,       ///< Pure data acquisition: print spectral channels
 *  -SPECTRO_APP_MODE_INFER_LOCAL,    ///< Run on-board ML model (e.g. Nano 33 BLE Sense)
 *  -SPECTRO_APP_MODE_INFER_PC        ///< Send data to host PC, wait for inference result
 *
 * SPDX-License-Identifier: MIT
 ********************************************************/

#include <Arduino.h>
#include "oled_ssd1306.h"
#include "Pimoroni_AS7343.h"
#include "spectro_app.h"

// put function declarations here:


void setup() {
  Serial.begin(115200);

  oled_ssd1306_setup();

  AS7343_init();
  if (!AS7343_is_connected()) {
    while (1) {
      Serial.println("AS7343 Not Found!");
      delay(500);
    }
  }
  Serial.println("AS7343 Connected!");

  spectro_app_init();                         
  spectro_app_set_mode(SPECTRO_APP_MODE_DATA_LOG); // Manually set program mode
  spectro_app_set_precision_mode(SPECTRO_PRECISION_HIGH); // Manually set precision
}

void loop() {
  oled_draw_start_go();   
  oled_show_mode();
  while(1){
    spectro_app_run_once();
  }
}


