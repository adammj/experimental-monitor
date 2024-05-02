/*
 * printf_raw.h
 *
 *  Created on: Apr 14, 2018
 *      Author: adam jones
 */


#ifndef printf_raw_defined
#define printf_raw_defined

#include <stdint.h>
#include <stdbool.h>
#ifdef _LAUNCHXL_F28379D
#include "F2837xD_device.h"
#else
#include "F2837xS_device.h"
#endif

void print_uint64_value(uint64_t value);
void print_all_int_except_uint64_value(int64_t value);
void print_float32_value(float32 value, uint16_t precision);
void print_float64_value(float64 value, uint16_t precision);
void print_uint64_timestamp(uint64_t timestamp, uint64_t ts_multiplier, uint16_t ts_shift);

void print_base64_array(const uint16_t *const array_ptr, uint16_t array_count, uint16_t precision);
void test_base64();

#endif
