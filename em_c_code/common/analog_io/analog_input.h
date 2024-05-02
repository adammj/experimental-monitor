/*
 * analog_input.h
 *
 *  Created on: Apr 4, 2017
 *      Author: adam
 */
//wrapper for either ADS8688 or internal adc
//all adc values are stored as 16bit integers
//0 = -10.24V, 32768 = 0V, 65535 = 10.2396875V


#ifndef analog_input_defined
#define analog_input_defined

#include <stdint.h>
#include <stdbool.h>
#include "spi.h"

typedef struct analog_in_ti_t {
    const uint16_t *analog_in_adc_letter; //0=A, 1=B
    const uint16_t *analog_in_adc_number;
} analog_in_ti_t;

typedef struct analog_in_spi_t {
    spi_letter_t analog_in_spi_letter;
    spi_gpio_t analog_in_spi_gpio;
    uint16_t reset_gpio;
    const uint16_t *device_channel_to_input_num;
} analog_in_spi_t;

typedef union {
    analog_in_ti_t ti_settings;
    analog_in_spi_t spi_settings;
} specific_analog_in_settings_t;

typedef struct analog_in_settings_t {
    uint16_t analog_in_count;
    uint16_t analog_in_type;    //0=built-in, 1=spi
    specific_analog_in_settings_t specific_settings;
} analog_in_settings_t;

//set a new conversion factor
void set_voltage_conversion_factor(const uint16_t new_conversion_factor);

//get the analog_in pointers (raw values)
float32 convert_analog_in_voltage(const uint16_t raw_voltage);

//wrapper
bool init_analog_in(const analog_in_settings_t analog_in_settings);
float32 analog_in_cla_task_length_in_us(void);
uint16_t get_analog_in_count(void);
uint16_t get_analog_in(uint16_t input_number);

//calculated using pointers inside
void get_analog_in_voltages_raw(uint16_t *const voltages);
void get_analog_in_voltages_converted(float32 *const voltages);
uint16_t get_analog_in_voltage_raw(uint16_t ain_num);

void printf_analog_in_values(void);
void printf_analog_conversion(void);
bool are_adc_values_valid(void);


#ifdef __cplusplus

#include "extract_json.h"
void set_analog_multiplier(const json_t *const json_root);

#endif


#endif
