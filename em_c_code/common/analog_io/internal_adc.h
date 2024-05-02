/*
 * internal_adc.h
 *
 *  Created on: Apr 4, 2017
 *      Author: adam
 */
// uses cpu1


#ifndef internal_adc_defined
#define internal_adc_defined

#include <stdint.h>
#include <stdbool.h>
#include "analog_input.h"


//functions
bool init_internal_adc(const analog_in_ti_t analog_in_settings);
float32 internal_adc_cla_task_length_in_us();

#endif
