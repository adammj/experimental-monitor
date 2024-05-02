/*
 * internal_adc_cla.h
 *
 *  Created on: Apr 20, 2017
 *      Author: adam
 */

#ifndef internal_adc_cla_defined
#define internal_adc_cla_defined

#include "cla_ptr.h"
#include "tic_toc_cla.h"

//necessary for cla function to be seen
#ifdef __cplusplus
extern "C" {
#endif

//cla prototype and timing
__interrupt void cla_task3_process_analog_io(void);

#ifdef __cplusplus
}
#endif

extern uint16_t cla_internal_adc_cycle_count;
extern volatile bool cla_internal_adc_enabled;

//ain result ptrs
extern volatile_aligned_uint16_t_ptr cla_ain_result_ptrs[];

//conversion values
extern uint16_t cla_internal_adc_conversion_offset;
extern float cla_internal_adc_conversion_scale;

#endif
