/*
 * analog_input.c
 *
 *  Created on: Apr 4, 2017
 *      Author: adam
 */

#include "analog_input.h"
#include "tic_toc.h"
#include "printf_json_delayed.h"
#include "math.h"
#include "scia.h"
#include "internal_adc.h"
#include "internal_adc_cla.h"
#include "ADS8688_adc.h"
#include "ADS8688_adc_cla.h"
#include "misc.h"
#include "daughterboard.h"
#include "arrays.h"
#include "serial_link.h"


serial_command_t command_get_values = {"get_analog_input_values", true, 0, NULL, printf_analog_in_values, NULL};
serial_command_t command_get_conversion = {"get_analog_conversion", true, 0,  NULL, printf_analog_conversion, NULL};
serial_command_t command_multiplier = {"set_analog_multiplier", true, 0, set_analog_multiplier, NULL, NULL};

//internal variables
static const uint16_t max_analog_input_channels = 8;
static volatile bool analog_in_initialized = false;

//cla variables
#pragma DATA_SECTION("cla_data");
volatile uint16_t cla_analog_in_voltages[max_analog_input_channels];    //cannot malloc, must use a constant
#pragma DATA_SECTION("cla_data");
uint16_t cla_analog_in_count = max_analog_input_channels;
#pragma DATA_SECTION("cla_data");
uint16_t cla_analog_in_type = 2;

//conversion factor
// 3200 = +/-10.24, 6400 = +/-5.12, 12800 = +/-2.56
uint16_t voltage_conversion_factor = 3200;

bool init_analog_in(const analog_in_settings_t analog_in_settings) {
    if (!db_board.is_enabled()) {
        delay_printf_json_error("cannot initialize analog in without daughterboard");
        return false;
    }

    if (analog_in_initialized) {return true;}

    //set the count
    const uint16_t temp_count = analog_in_settings.analog_in_count;

    //check
    if (temp_count > max_analog_input_channels) {
        delay_printf_json_error("too many analog input channels");
        return false;
    }
    cla_analog_in_count = temp_count;

    //set the type
    cla_analog_in_type = analog_in_settings.analog_in_type;

    switch (analog_in_settings.analog_in_type) {
        case 0:
            analog_in_initialized = init_internal_adc(analog_in_settings.specific_settings.ti_settings);
            break;

        case 1:
            analog_in_initialized = init_ADS8688_adc(analog_in_settings.specific_settings.spi_settings);
            break;

        default:
            delay_printf_json_error("analog input type not defined");
            break;
    }

    //if not initialized, set count to zero
    if (!analog_in_initialized) {
        cla_analog_in_count = 0;
    }

    add_serial_command(&command_get_values);
    add_serial_command(&command_get_conversion);
    if (cla_analog_in_type == 1) {
        add_serial_command(&command_multiplier);
    }

    return analog_in_initialized;
}

void get_analog_in_voltages_raw(uint16_t *const voltages) {
    if (!analog_in_initialized) {return;}
    if (voltages == NULL) {return;}

    for (uint16_t i=0; i<cla_analog_in_count; i++) {
        voltages[i] = cla_analog_in_voltages[i];
    }
}

uint16_t get_analog_in_count(void) {
    if (!analog_in_initialized) {return 0;}
    return cla_analog_in_count;
}

__attribute__((ramfunc))
uint16_t get_analog_in(uint16_t input_number) {
    if (!analog_in_initialized) {return 0;}

    if (input_number < cla_analog_in_count) {
        return cla_analog_in_voltages[input_number];
    } else {
        return 0;
    }
}

uint16_t get_analog_in_voltage_raw(uint16_t ain_num) {
    if (!analog_in_initialized) {return 0;}

    if ((ain_num >= cla_analog_in_count) || (ain_num == 0)) {
        return 0;
    }

    return cla_analog_in_voltages[ain_num - 1];
}

__attribute__((ramfunc))
void get_analog_in_voltages_converted(float32 *const voltages) {
    if (!analog_in_initialized) {return;}
    if (voltages == NULL) {return;}

    for (uint16_t i=0; i<cla_analog_in_count; i++) {
        //convert to actual voltage
        voltages[i] = convert_analog_in_voltage(cla_analog_in_voltages[i]);
    }
}

void set_voltage_conversion_factor(const uint16_t new_conversion_factor) {
    voltage_conversion_factor = new_conversion_factor;
}

void printf_analog_conversion(void) {
    delay_printf_json_objects(1, json_uint16("analog_conversion", voltage_conversion_factor));
}

void set_analog_multiplier(const json_t *const json_root) {
    if (cla_analog_in_type != 1) {
        delay_printf_json_error("cannot change multiplier for internal adc");
        return;
    }

    json_element multiplier_e("multiplier", t_uint16, true);

    if (!multiplier_e.set_with_json(json_root, true)) {
        return;
    }

    const uint16_t multiplier = multiplier_e.value().uint16_;
    switch (multiplier) {
        case 1:
            change_ADS8688_range(0);
            break;

        case 2:
            change_ADS8688_range(1);
            break;

        case 4:
            change_ADS8688_range(2);
            break;

        default:
            delay_printf_json_error("invalid multiplier");
            return;
    }

    delay_printf_json_status("analog multiplier set");
}

__attribute__((ramfunc))
float32 convert_analog_in_voltage(const uint16_t raw_voltage) {
    return (static_cast<float32>(raw_voltage) - 32768.0)/(static_cast<float32>(voltage_conversion_factor));
}

bool are_adc_values_valid(void) {
    bool at_least_one_value_gt_zero = false;

    for (uint16_t i=0; i<cla_analog_in_count; i++) {
        if (cla_analog_in_voltages[i] > 0) {
            at_least_one_value_gt_zero = true;
        }
    }

    return at_least_one_value_gt_zero;
}

__attribute__((ramfunc))
void printf_analog_in_values(void) {
    if (!analog_in_initialized) {return;}

    //use heap array, since size is unknown
    float32 *const temp_voltages = create_array_of<float32>(cla_analog_in_count, "analog_voltages");
    if (temp_voltages != NULL) {
        get_analog_in_voltages_converted(temp_voltages);
        delay_printf_json_objects(1, json_float32_array("analog_input_values", cla_analog_in_count, temp_voltages, true, 3));

        delete_array(temp_voltages);
    }
}

float32 analog_in_cla_task_length_in_us(void) {
    float32 value = 0;

    switch (cla_analog_in_type) {
        case 0:
            value = internal_adc_cla_task_length_in_us();
            break;

        case 1:
            value = ADS8688_adc_cla_task_length_in_us();
            break;

        default:
            break;
    }

    return value;
}
