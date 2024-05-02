/*
 * digital_io.h
 *
 *  Created on: Mar 24, 2017
 *      Author: adam jones
 */


#ifndef digital_io_defined
#define digital_io_defined

#include <stdint.h>
#include <stdbool.h>


typedef struct digital_io_settings_t {
    uint16_t digital_in_count;
    uint16_t digital_out_count;
    const uint16_t *digital_in_gpio;
    const uint16_t *digital_out_gpio;
} digital_io_settings_t;


//init
bool init_digital_io(const digital_io_settings_t digital_io_settings);

//used by experimental_inputs_outputs (index 0)
void set_digital_out(uint16_t output_number, uint16_t value);
uint16_t get_digital_in(uint16_t input_number); //kept uint16 for io controller compatibility

//for status messages
void printf_digital_in_values();
void printf_digital_out_values();

uint16_t get_digital_in_count();
uint16_t get_digital_out_count();


#endif
