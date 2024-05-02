/*
 * digital_io.c
 *
 *  Created on: Mar 24, 2017
 *      Author: adam jones
 */


#include "printf_json_delayed.h"
#include "digital_io.h"
#include "F28x_Project.h"
#include "extract_json.h"
#include "misc.h"
#include "daughterboard.h"
#include "arrays.h"
#include "serial_link.h"


serial_command_t command_get_digital_values = {"get_digital_input_values", true, 0, NULL, printf_digital_in_values, NULL};
serial_command_t command_set_digital_values = {"get_digital_output_values", true, 0, NULL, printf_digital_out_values, NULL};

//digital in variables
static volatile bool digital_io_initialized = false;
static volatile uint16_t digital_in_count = 0;
static volatile uint16_t *digital_in_gpio;
static uint16_t *input_states;
static volatile uint32_t *digital_in_pin_mask;
static volatile uint32_t **digital_in_pin_data_reg;
static volatile uint16_t *digital_in_pin_shift;

//digital out variables
static volatile uint16_t digital_out_count = 0;
static volatile uint16_t *digital_out_gpio;
static uint16_t *output_states;
static volatile uint32_t *digital_out_pin_mask;
static volatile uint32_t **digital_out_pin_data_reg;
static volatile uint16_t *digital_out_pin_shift;

//internal functions
void get_digital_out_states(uint16_t *const temp_output_states);


bool init_digital_io(const digital_io_settings_t digital_io_settings) {
    if (!db_board.is_enabled()) {
        delay_printf_json_error("cannot initialize digital io without daughterboard");
        return false;
    }
    if (digital_io_initialized) {return true;}


    //set up in
    digital_in_count = digital_io_settings.digital_in_count;

    //use heap array, since size is unknown
    digital_in_gpio = create_array_of<uint16_t>(digital_in_count, "digital_in_gpio");
    if (digital_in_gpio == NULL) {return false;}
    input_states = create_array_of<uint16_t>(digital_in_count, "input_states");
    if (input_states == NULL) {return false;}
    digital_in_pin_mask = create_array_of<uint32_t>(digital_in_count, "digital_in_pin_mask");
    if (digital_in_pin_mask == NULL) {return false;}
    digital_in_pin_data_reg = create_array_of<volatile uint32_t *>(digital_in_count, "digital_in_pin_data_reg");
    if (digital_in_pin_data_reg == NULL) {return false;}
    digital_in_pin_shift = create_array_of<uint16_t>(digital_in_count, "digital_in_pin_shift");
    if (digital_in_pin_shift == NULL) {return false;}

    for (uint16_t i=0; i<digital_in_count; i++) {
        digital_in_gpio[i] = digital_io_settings.digital_in_gpio[i];
        digital_in_pin_mask[i] = calculate_pin_mask(digital_in_gpio[i]);
        digital_in_pin_data_reg[i] = calculate_pin_data_reg(digital_in_gpio[i]);
        digital_in_pin_shift[i] = (static_cast<uint16_t>(digital_in_gpio[i]) % 32);
    }

    //set up out
    digital_out_count = digital_io_settings.digital_out_count;

    //use heap array, since size is unknown
    digital_out_gpio = create_array_of<uint16_t>(digital_out_count, "digital_out_gpio");
    if (digital_out_gpio == NULL) {return false;}
    output_states = create_array_of<uint16_t>(digital_out_count, "output_states");
    if (output_states == NULL) {return false;}
    digital_out_pin_mask = create_array_of<uint32_t>(digital_out_count, "digital_out_pin_mask");
    if (digital_out_pin_mask == NULL) {return false;}
    digital_out_pin_data_reg = create_array_of<volatile uint32_t *>(digital_out_count, "digital_out_pin_data_reg");
    if (digital_out_pin_data_reg == NULL) {return false;}
    digital_out_pin_shift = create_array_of<uint16_t>(digital_out_count, "digital_out_pin_shift");
    if (digital_out_pin_shift == NULL) {return false;}

    //gpio setup only exist for CPU1
    //set dout pins to direction out
    for (uint16_t i=0; i<digital_out_count; i++) {
        digital_out_gpio[i] = digital_io_settings.digital_out_gpio[i];
        GPIO_SetupPinOptions(digital_out_gpio[i], GPIO_OUTPUT, GPIO_PUSHPULL);

        digital_out_pin_mask[i] = calculate_pin_mask(digital_out_gpio[i]);
        digital_out_pin_data_reg[i] = calculate_pin_data_reg(digital_out_gpio[i]);
        digital_out_pin_shift[i] = (static_cast<uint16_t>(digital_out_gpio[i]) % 32);
    }

    add_serial_command(&command_get_digital_values);
    add_serial_command(&command_set_digital_values);

    digital_io_initialized = true;
    delay_printf_json_status("initialized internal digital io");
    return true;
}

void set_digital_out(uint16_t output_number, uint16_t value) {
    if (!digital_io_initialized) {return;}

    if (output_number < digital_out_count) {
        gpio_write_pin(digital_out_pin_data_reg[output_number], digital_out_pin_mask[output_number], (value>0));    //makes all >0 values = true
    }
}

__attribute__((ramfunc))
uint16_t get_digital_in(uint16_t input_number) {
    if (!digital_io_initialized) {return 0;}

    if (input_number < digital_in_count) {
        const uint16_t value = gpio_read_pin(digital_in_pin_data_reg[input_number], digital_in_pin_shift[input_number]);
        return static_cast<uint16_t>(value > 0);
    } else {
        return 0;
    }
}

uint16_t get_digital_in_count() {
    return digital_in_count;
}
uint16_t get_digital_out_count() {
    return digital_out_count;
}

__attribute__((ramfunc))
void get_digital_in_states(uint16_t *const temp_input_states) {
    if (!digital_io_initialized) {return;}

    for (uint16_t i=0; i<digital_in_count; i++) {
        const uint16_t value = gpio_read_pin(digital_in_pin_data_reg[i], digital_in_pin_shift[i]);
        temp_input_states[i] = static_cast<uint16_t>(value > 0);
    }
}

__attribute__((ramfunc))
void printf_digital_in_values() {
    if (!digital_io_initialized) {return;}

    get_digital_in_states(input_states);
    delay_printf_json_objects(1, json_uint16_array("digital_input_states", digital_in_count, input_states, true));
}

__attribute__((ramfunc))
void printf_digital_out_values() {
    if (!digital_io_initialized) {return;}

    get_digital_out_states(output_states);
    delay_printf_json_objects(1, json_uint16_array("digital_output_states", digital_out_count, output_states, true));
}

__attribute__((ramfunc))
void get_digital_out_states(uint16_t *const temp_output_states) {
    if (!digital_io_initialized) {return;}

    for (uint16_t i=0; i<digital_out_count; i++) {
        const uint16_t value = gpio_read_pin(digital_out_pin_data_reg[i], digital_out_pin_shift[i]);
        temp_output_states[i] = static_cast<uint16_t>(value > 0);
    }
}
