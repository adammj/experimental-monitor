/*
 * ADS8688_adc_cla.h
 *
 *  Created on: May 23, 2017
 *      Author: adam jones
 */

#ifndef ADS8688_adc_cla_defined
#define ADS8688_adc_cla_defined

#include "stdint.h"
#include "stdbool.h"
#include "tic_toc_cla.h"
#include "spi.h"
#include "analog_input.h"
#include "cla_ptr.h"
#include "analog_input_cla.h"


//necessary for cla function to be seen
#ifdef __cplusplus
extern "C" {
#endif

//task takes around 20us total
__interrupt void cla_task4_read_ADS8688_adc_voltages(void);

#ifdef __cplusplus
}
#endif

extern uint16_t cla_ads8688_channel_to_ain[];
extern volatile struct SPI_REGS *cla_ADS8688_spi_regs;
extern volatile uint16_t cla_ADS8688_cycle_count;
extern volatile bool cla_ADS8688_adc_enabled;
extern volatile bool has_had_fatal_adc_error;

#endif
