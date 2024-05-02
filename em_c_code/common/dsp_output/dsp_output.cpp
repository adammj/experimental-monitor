/*
 * dsp_output.c
 *
 *  Created on: Mar 21, 2017
 *      Author: adam jones
 */

#include "dsp_output.h"
#include "printf_json_delayed.h"
#include "tic_toc.h"
#include "ring_buffer.h"
#include "extract_json.h"
#include "math.h"
#include "misc.h"
#include "cpu_timers.h"
#include "cla.h"
#include "daughterboard.h"
#include "arrays.h"
#include "string.h"
#include "serial_link.h"


serial_command_t command_send_codes = {"send_event_codes", true, 0, send_event_codes_with_json, NULL, NULL};
serial_command_t command_dsp_test = {"set_dsp_testing_mode", true, 0, set_dsp_testing_mode, NULL, NULL};
serial_command_t command_get_queue = {"get_event_code_queue_available", true, 0, NULL, get_event_code_queue_available_void, NULL};
serial_command_t command_set_queue = {"set_event_code_queue_notification", true, 0, set_event_code_queue_notification, NULL, NULL};


//internal variables
static const uint16_t dsp_high_code_queue_length = 20;      //only need a few slots for high priority
static const uint16_t dsp_low_code_queue_length =  500;     //keep more slots for low priority
static const uint16_t code_to_strobe_us_delay = 1; //delay between setting code and strobe (and vice-versa)
static uint32_t dsp_output_cycles = 0;
static uint32_t dsp_output_cycles_max = 0;

//******** QUEUES *******
//must be volatile, as they are accessed inside an interrupt
static volatile bool dsp_initialized = false;
static volatile ring_buffer dsp_high_code_queue;
static volatile ring_buffer dsp_low_code_queue;
static volatile uint16_t event_code_queue_notification_available_count = 0;

//******** EVENT CODE PROCESSING ********
//these variables all need initialization, but entirely inside the loop
static volatile bool dsp_strobe_enabled = false;
static volatile bool dsp_normal_mode = true;   //for sending a constant toggle
static volatile uint16_t max_event_code_value = 255;   //default value - will be updated
static volatile uint16_t highest_ascii_event_code = 127;   //don't allow event codes at this, or text above this
static volatile uint16_t dsp_testing_code = 0;

//******** GPIO HELPERS (speed) ********
static volatile uint16_t dsp_bit_count;
static volatile uint32_t *dsp_strobe_gpio_data_reg;    //points to hardware, must be volatile
static volatile uint32_t **dsp_gpio_data_reg;          //points to hardware, must be volatile
static volatile uint32_t dsp_strobe_pin_mask;
static volatile uint32_t *dsp_pin_mask;

//internal functions
void dsp_set_gpio_for_strobe(bool strobe_on);
void dsp_set_gpio_for_code(uint16_t code);
void send_low_priority_event_code(uint16_t code);


//init
bool init_dsp_output(const dsp_settings_t dsp_settings) {
    if (!db_board.is_enabled()) {
        delay_printf_json_error("cannot initialize dsp output without daughterboard");
        return false;
    }
    if (dsp_initialized) {return true;}

    //create raw dat reg helpers
    //use heap array, since size is unknown
    dsp_gpio_data_reg = create_array_of<volatile uint32_t *>(dsp_settings.dsp_bit_count, "dsp_gpio_data_reg");
    if (dsp_gpio_data_reg == NULL) {return false;}
    dsp_pin_mask = create_array_of<uint32_t>(dsp_settings.dsp_bit_count, "dsp_pin_mask");
    if (dsp_pin_mask == NULL) {return false;}

    //copy values
    dsp_bit_count = dsp_settings.dsp_bit_count;
    max_event_code_value = (static_cast<uint16_t>(1) << dsp_bit_count) - 1; //2^bits minus 1

    //init the ring buffers
    if (!dsp_high_code_queue.init_alloc(dsp_high_code_queue_length)) {
        return false;
    }
    if (!dsp_low_code_queue.init_alloc(dsp_low_code_queue_length)) {
        return false;
    }


    //set all dsp pins to direction out, and CPU controlled
    for (uint16_t i=0; i<dsp_settings.dsp_bit_count; i++) {
        GPIO_SetupPinOptions(dsp_settings.dsp_bit_gpio[i], GPIO_OUTPUT, GPIO_PUSHPULL);
        GPIO_SetupPinMux(dsp_settings.dsp_bit_gpio[i], GPIO_MUX_CPU1, 0);
    }
    //strobe pin
    GPIO_SetupPinOptions(dsp_settings.dsp_strobe_gpio, GPIO_OUTPUT, GPIO_PUSHPULL);
    GPIO_SetupPinMux(dsp_settings.dsp_strobe_gpio, GPIO_MUX_CPU1, 0);


    //init the gpio settings, for speed
    uint16_t pin = dsp_settings.dsp_strobe_gpio;
    dsp_strobe_gpio_data_reg = calculate_pin_data_reg(pin);
    dsp_strobe_pin_mask = 1UL << (pin % 32);

    for (uint16_t i=0; i<dsp_settings.dsp_bit_count; i++) {
        pin = dsp_settings.dsp_bit_gpio[i];
        dsp_gpio_data_reg[i] = calculate_pin_data_reg(pin);
        dsp_pin_mask[i] = 1UL << (pin % 32);
    }

    add_serial_command(&command_send_codes);
    add_serial_command(&command_dsp_test);
    add_serial_command(&command_get_queue);
    add_serial_command(&command_set_queue);

    dsp_initialized = true;
    delay_printf_json_status("initialized dsp output");
    return true;
}


//dsp sending functions
__attribute__((ramfunc))
void send_low_priority_event_code(uint16_t code) {
    //low priority allows codes between 1 and max_event_code_value
    if (!dsp_initialized) {return;}

    //don't send codes higher than the max value allowed
    if ((code == 0) || (code > max_event_code_value)) {
        delay_printf_json_objects(2, json_string("error","code outside of range"), json_uint16("code", code));
        return;
    }

    //assume it cannot wait
    if (dsp_low_code_queue.is_full()) {
        delay_printf_json_error("the event code queue is full");
        return;
    }

    if (!dsp_low_code_queue.write(code)) {
        delay_printf_json_error("low priority event code not queued");
    }
}

__attribute__((ramfunc))
void send_high_priority_event_code_at_front_of_queue(uint16_t code) {
    if (!dsp_initialized) {return;}

    //don't send codes higher than the max value allowed
    if ((code <= highest_ascii_event_code) || (code > max_event_code_value)) {
        delay_printf_json_objects(2, json_string("error","code outside of range"), json_uint16("code", code));
        return;
    }

    //send a code at the front of the queue
    if (dsp_high_code_queue.is_empty()) {
        //easy case, just add to queue
        if (!dsp_high_code_queue.write(code)) {
            delay_printf_json_error("high priority event code not queued");
        }
    } else {
        //else, have to reorder the codes

        //high priority, can't wait - just drop and flag error
        if (dsp_high_code_queue.is_full()) {
            delay_printf_json_error("no room for high priority event code");
            return;
        }

        //else, count the currently used spaces
        const uint16_t used_spaces = dsp_high_code_queue.in_use();

        //add the code
        if (!dsp_high_code_queue.write(code)) {
            delay_printf_json_error("high priority event code not queued");
        }

        //now, shuffle all codes back to the end
        uint16_t temp_code = 0;
        for (uint16_t i=0; i<used_spaces; i++) {
            if (dsp_high_code_queue.read(temp_code)) {
                if (!dsp_high_code_queue.write(temp_code)) {
                    delay_printf_json_error("high priority event code not queued");
                }
            } else {
                delay_printf_json_error("could not read from high priority event code queue");
            }
        }
    }
}

__attribute__((ramfunc))
void send_high_priority_event_code(uint16_t code) {
    //high priority queue only allows event codes outside ascii range
    if (!dsp_initialized) {return;}

    //don't send codes higher than the max value allowed
    if ((code <= highest_ascii_event_code) || (code > max_event_code_value)) {
        delay_printf_json_objects(2, json_string("error","code outside of range"), json_uint16("code", code));
        return;
    }

    //high priority, can't wait - just drop and flag error
    if (dsp_high_code_queue.is_full()) {
        delay_printf_json_error("no room for high priority event code");
        return;
    }

    if (!dsp_high_code_queue.write(code)) {
        delay_printf_json_error("high priority event code not queued");
    }
}

void set_event_code_queue_notification(const json_t *const json_root) {
    if (!dsp_initialized) {return;}

    json_element count_e("available", t_uint16);

    if (count_e.set_with_json(json_root, true)) {
        const uint16_t count = count_e.value().uint16_;
        //cannot be equal to or greater than the queue length
        if (count >= dsp_low_code_queue_length) {
            delay_printf_json_error("available count is too large");
        } else {
            event_code_queue_notification_available_count = count;
        }
    }
}

__attribute__((ramfunc))
void get_event_code_queue_available_void() {
    get_event_code_queue_available();
}

__attribute__((ramfunc))
void get_event_code_queue_available(bool send_info, uint32_t id, uint32_t i, uint32_t count) {
    if (!dsp_initialized) {return;}

    const uint16_t slots_available = dsp_low_code_queue.available();

    if (send_info) {
        delay_printf_json_objects(5, json_parent("event_code_queue", 4),
                                  json_uint16("available", slots_available),
                                  json_uint32("sent_id", id),
                                  json_uint32("sent_i", i),
                                  json_uint32("sent_count", count));
    } else {
        delay_printf_json_objects(2, json_parent("event_code_queue", 1), json_uint16("available", slots_available));
    }
}

__attribute__((ramfunc))
void send_event_codes_with_json(const json_t *const json_root) {
    if (!dsp_initialized) {return;}

    debug_timestamps.dsp_b_send_code = CPU_TIMESTAMP;

    json_element codes_e("codes", t_uint16, false, true);
    json_element packet_e("packet", t_string);

    //if at least one element was found
    if (set_elements_with_json(json_root, 2, &codes_e, &packet_e) > 0) {
        //check which was found (if both, just do the codes)
        const uint16_t num_codes = codes_e.count_found();
        if (num_codes > 0) {

            // array was forced, so it will exist
            const uint16_t *const codes = codes_e.get_uint16_array();
            for (uint16_t i=0; i<num_codes; i++) {
                if (codes[i] > highest_ascii_event_code) {
                    //wait until there is space for another code
                    while (dsp_low_code_queue.is_full()) {}

                    //add code to queue
                    send_low_priority_event_code(codes[i]);
                } else {
                    delay_printf_json_error("event code outside code range (128-255)");
                }
            }

            //do not notify if just sending codes
            //notify the space available
            //get_event_code_queue_available();

        } else { //if (string_e.count_found() > 0) {
            //convert the string into codes
            const uint16_t *chars = reinterpret_cast<const uint16_t *>(packet_e.value().string_);

            //packet must begin with "pack"
            if (!((chars[2] == 'p') && (chars[3] == 'a') && (chars[4] == 'c') && (chars[5] == 'k'))) {
                delay_printf_json_error("invalid packet");
                return;
            }

            //copy chars
            uint16_t char_i = 0;

            //packet_id
            char packet_id_char[3];
            packet_id_char[char_i++] = static_cast<char>(chars[9 + char_i]);
            packet_id_char[char_i++] = static_cast<char>(chars[9 + char_i]);
            packet_id_char[2] = 0;

            //packet_i
            char packet_i_char[5];
            char_i = 0;
            packet_i_char[char_i++] = static_cast<char>(chars[12 + char_i]);
            packet_i_char[char_i++] = static_cast<char>(chars[12 + char_i]);
            packet_i_char[char_i++] = static_cast<char>(chars[12 + char_i]);
            packet_i_char[char_i++] = static_cast<char>(chars[12 + char_i]);
            packet_i_char[4] = 0;

            //packet_count
            char packet_count_char[5];
            char_i = 0;
            packet_count_char[char_i++] = static_cast<char>(chars[17 + char_i]);
            packet_count_char[char_i++] = static_cast<char>(chars[17 + char_i]);
            packet_count_char[char_i++] = static_cast<char>(chars[17 + char_i]);
            packet_count_char[char_i++] = static_cast<char>(chars[17 + char_i]);
            packet_count_char[4] = 0;

            char *end_ptr = NULL;
            const uint32_t packet_id = strtoul(packet_id_char, &end_ptr, 10);
            const uint32_t packet_i = strtoul(packet_i_char, &end_ptr, 10);
            const uint32_t packet_count = strtoul(packet_count_char, &end_ptr, 10);


            //stop at the first 0
            while (*chars != 0) {
                if (*chars <= highest_ascii_event_code) {
                    //wait until there is space for another code
                    while (dsp_low_code_queue.is_full()) {}

                    //add char to queue
                    send_low_priority_event_code(*chars);
                } else {
                    delay_printf_json_error("string char outside ascii range (1-127)");
                }
                chars++;
            }

            debug_timestamps.dsp_a_send_code = CPU_TIMESTAMP;

            //notify the space available
            get_event_code_queue_available(true, packet_id, packet_i, packet_count);
        }
    }
}

void set_dsp_testing_mode(const json_t *const json_root) {
    if (!dsp_initialized) {return;}

    json_element enable_e("enable", t_bool, true);

    if (!enable_e.set_with_json(json_root, true)) {
        return;
    }

    if (enable_e.value().bool_) {
        delay_printf_json_status("enable dsp testing mode");
        dsp_testing_code = 0;
        dsp_normal_mode = false;
    } else {
        delay_printf_json_status("disable dsp testing mode");
        dsp_normal_mode = true;
    }
}


__attribute__((ramfunc))
void process_dsp_event_code_queues(int64_t uptime_count) {
    if (!dsp_initialized) {return;}

    const uint32_t start_time = CPU_TIMESTAMP;
    debug_timestamps.dsp_s_processing = start_time;

    //if an odd count, can only disable strobe and exit
    if ((uptime_count & 0b1) > 0) {
        //if strobe enabled
        if (dsp_strobe_enabled) {
            //turn off the strobe
            dsp_set_gpio_for_strobe(false);

            //delay
            DELAY_US(code_to_strobe_us_delay);

            //set code for 0
            dsp_set_gpio_for_code(0);
        }

        return;
    }

    //otherwise, an even count, and continue processing

    //check if sending notification
    if (event_code_queue_notification_available_count > 0) {
        //equal, so that it is just sent once
        if (dsp_low_code_queue.available() == event_code_queue_notification_available_count) {
            get_event_code_queue_available();
        }
    }

    //check if normal or testing mode
    if (dsp_normal_mode) {

        //if not sending, and either buffer has at least one byte in it
        if ((!dsp_high_code_queue.is_empty()) || (!dsp_low_code_queue.is_empty())) {
            uint16_t dsp_current_code = 0;

            //one of the buffers has available bytes, so grab from the high priority queue if possible, otherwise low
            if (!dsp_high_code_queue.is_empty()) {
                //grab from the buffer
                if (!dsp_high_code_queue.read(dsp_current_code)) {
                    delay_printf_json_error("error reading from dsp_high_code_queue");
                }
            } else {
                //grab from the buffer
                if (!dsp_low_code_queue.read(dsp_current_code)) {
                    delay_printf_json_error("error reading from dsp_low_code_queue");
                }
            }

            //set to the current code
            dsp_set_gpio_for_code(dsp_current_code);

            //delays
            DELAY_US(code_to_strobe_us_delay);

            //turn on the strobe
            dsp_set_gpio_for_strobe(true);
        }

    } else {

        //set to the current testing code
        dsp_set_gpio_for_code(dsp_testing_code);

        //delay
        DELAY_US(code_to_strobe_us_delay);

        //turn on strobe
        dsp_set_gpio_for_strobe(true);

        if (dsp_testing_code < max_event_code_value) {
            dsp_testing_code++;
        } else {
            dsp_testing_code = 0;
        }
    }

    update_cpu_cycles(start_time, dsp_output_cycles, dsp_output_cycles_max);
}

__attribute__((ramfunc))
void dsp_set_gpio_for_strobe(bool strobe_on) {
    gpio_write_pin(dsp_strobe_gpio_data_reg, dsp_strobe_pin_mask, strobe_on);
    dsp_strobe_enabled = strobe_on; //set the value to the same
}

__attribute__((ramfunc))
void dsp_set_gpio_for_code(uint16_t code) {
    //incrementally set the pins for the code starting at the lowest bit, working up
    for (uint16_t i=0; i<dsp_bit_count; i++) {
        gpio_write_pin(dsp_gpio_data_reg[i], dsp_pin_mask[i], (code & 1));
        code = code >> 1;
    }
}
