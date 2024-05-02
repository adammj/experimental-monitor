/*
 * misc.h
 *
 *  Created on: Mar 20, 2017
 *      Author: adam jones
 */

#ifndef misc_defined
#define misc_defined

#include <stdint.h>
#include <stdbool.h>


//performance memcpy test (must be started after tic_toc is initalized)
void performance_test();

// random
uint32_t djb2_hash(const char *const string);
bool is_power_of_two(uint64_t value);
void lockup_cpu();

//gpio
void gpio_write_pin(volatile uint32_t *const pin_data_reg, uint32_t pin_mask, bool set_pin);
bool gpio_read_pin(const volatile uint32_t *const pin_data_reg, uint16_t pin_shift);
volatile uint32_t *calculate_pin_data_reg(uint16_t gpio_number);
uint32_t calculate_pin_mask(uint16_t gpio_number);

//stack and heap
uint16_t get_max_stack_free();
uint32_t get_max_heap_free();
void error_allocating_messages(const char *const variable_name);
void not_enough_free_space(const char *const variable_name, uint16_t needed, uint16_t available);

#endif
