/*
 * analog_input_cla.h
 *
 *  Created on: Apr 4, 2017
 *      Author: adam
 */
//shared cla header for both ADS8688 and internal adc

#ifndef analog_input_cla_defined
#define analog_input_cla_defined

#include "stdint.h"
#include "stdbool.h"
#include "tic_toc_cla.h"

extern uint16_t cla_analog_in_count;

//voltages are stored in a common format
//0 = -10.24V, 32768 = 0V, and 65535 = 10.2396875V
extern volatile uint16_t cla_analog_in_voltages[];

#endif
