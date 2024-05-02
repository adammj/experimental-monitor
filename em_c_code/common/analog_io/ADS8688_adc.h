/*
 *  ADS8688_adc.h
 *
 *  Created on: May 23, 2017
 *      Author: adam jones
 */

#ifndef ADS8688_adc_defined
#define ADS8688_adc_defined

#include "stdint.h"
#include "stdbool.h"
#include "analog_input.h"


bool init_ADS8688_adc(const analog_in_spi_t analog_in_spi_settings);
float32 ADS8688_adc_cla_task_length_in_us();
void change_ADS8688_range(uint16_t range_id);


#endif
