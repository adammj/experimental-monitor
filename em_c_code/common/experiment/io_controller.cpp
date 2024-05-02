/*
 * io_controller.cpp
 *
 *  Created on: Jan 5, 2018
 *      Author: adam jones
 */

#include "io_controller.h"
#include "io_controller_cla.h"
#include "ti_launchpad.h"
#include "daughterboard.h"
#include "dsp_output.h"
#include "status_messages.h"
#include "printf_json_delayed.h"
#include "cpu_timers.h"
#include "experimental_inputs.h"
#include "experimental_outputs.h"
#include "analog_input.h"
#include "digital_io.h"
#include "misc.h"
#include "string.h"
#include "extract_json.h"
#include "cla.h"
#include "math.h"
#include "tic_toc.h"
#include "limits.h"
#include "arrays.h"
#include "serial_link.h"


serial_command_t command_twiddle = {"twiddle_leds", true, 0, NULL, twiddle_leds_ten_times, NULL};
serial_command_t command_uptime = {"get_uptime", true, 0, NULL, print_uptime, NULL};
serial_command_t command_reset_clock = {"reset_experiment_clock", true, 0, reset_experiment_clock, NULL, NULL};
serial_command_t command_timestamp = {"get_experiment_timestamp", true, 0, NULL, get_experiment_timestamp, NULL};
serial_command_t command_set_input_settings = {"set_input_settings", true, 0, set_input_settings, NULL, NULL};
serial_command_t command_get_input_settings = {"get_input_settings", true, 0, get_input_settings, NULL, NULL};
serial_command_t command_set_input_actions = {"set_input_actions", true, 0, set_input_actions, NULL, NULL};
serial_command_t command_get_input_history = {"get_input_history", true, 0, get_input_history, NULL, NULL};
serial_command_t command_set_output_settings = {"set_output_settings", true, 0, set_output_settings, NULL, NULL};
serial_command_t command_get_output_settings = {"get_output_settings", true, 0, get_output_settings, NULL, NULL};
serial_command_t command_set_output_actions = {"set_output_actions", true, 0, set_output_actions, NULL, NULL};


///internal variables
static volatile bool has_init_io_controller = false;

//timing info
static const uint16_t freq_multiplier = 10; //affects code sending, where code rate is 1/2 of multiplier
static volatile uint64_t experiment_tic = 0;      //reset whenever desired
static volatile uint32_t main_loop_freq = 1;
static volatile bool is_even_second = false;
static volatile int64_t off_led_tic = 0;
static volatile uint32_t on_duration_count = 1;
static volatile uint16_t experiment_io_count_tic = 0;
static volatile uint32_t second_count_tic = 0;
static volatile bool need_to_reset_experiment_clock = false;
static volatile uint16_t reset_experiment_clock_event_code = 0;

//uptime count for comparison
static volatile int64_t cpu_uptime_count = 0;     //never reset, used just for uptime info
static volatile int64_t cla_uptime_count = 0;     //must be updated manually
static volatile int64_t cumulative_error = 0;
static volatile int64_t previous_cumulative_error = 0;

//cla variables
#pragma DATA_SECTION("cla_data");
volatile uint32_t cla_uptime_count_32 = 0;
#pragma DATA_SECTION("cla_data");
volatile uint32_t cla_uptime_overflow = 0;

//led stuff
static volatile uint16_t twiddle_count = 0;
static volatile bool twiddle_on = false;
static volatile uint32_t twiddle_pause_count = 0;
static const uint16_t twiddle_length_ms = 50;  //50ms
static volatile uint32_t twiddle_cpu_tics = 500;   //default to 500ms

//controller holds the count and pointers to the array of exp io
static volatile uint16_t input_count = 0;
static volatile uint16_t output_count = 0;
static volatile experimental_input *experimental_inputs_;
static volatile experimental_output *experimental_outputs_;

//input history printing
static volatile bool should_print_input_history_bool = false;
static volatile bool *should_print_input_history_array;
static volatile uint16_t desired_history_count = 0;
static volatile uint64_t desired_history_tics = 1; //must be 1 or greater

//status messages
static volatile bool status_messages_enabled = false;
static volatile bool status_messages_full = true;

//internal functions
void print_input_history();



__attribute__((ramfunc))
void check_cumulative_error() {

    delay_printf_json_objects(2, json_string("error", "inconsistent cla and cpu uptimes"), json_int64("cumulative error (cla-cpu)", cumulative_error));
    previous_cumulative_error = cumulative_error;
}

//no delays, waits, or non-delayed printing
//takes about 1.5 or 19us (depending on which tic)
__attribute__((ramfunc))
void io_controller_main_loop() {

    //compare cpu and cla clocks before incrementing them, since both should be done by now
    //if there is a difference, print it once per second
    cumulative_error = cla_uptime_count - cpu_uptime_count;
    if ((cumulative_error != previous_cumulative_error) && is_even_second) {
        check_cumulative_error();
    }

    //calculate if even second
    is_even_second = (second_count_tic == 0);

    //increment and check for rollover
    second_count_tic++;
    if (second_count_tic == main_loop_freq) {
        second_count_tic = 0;
    }

    //only process the io at multiples of the freq_multiplier
    if (experiment_io_count_tic == 0) {

        // **** INPUTS ****
        //update all current values first (so that parent and child values don't have to be checked again)
        for (uint16_t i=0; i<input_count; i++) {
            //this is also the only place the experiment_tic is set for all
            experimental_inputs_[i].update_current_value(experiment_tic);
        }
        //process each input (update values must have been done first)
        for (uint16_t i=0; i<input_count; i++) {
            experimental_inputs_[i].process_actions();
        }

        // **** OUTPUTS ****
        for (uint16_t i=0; i<output_count; i++) {
            experimental_outputs_[i].process_actions(experiment_tic);
        }

        // **** PRINT HISTORY ****
        if (should_print_input_history_bool && ((experiment_tic % desired_history_tics) == 0)) {
            print_input_history();
        }

        //reset experiment count (only after everything has run)
        if (need_to_reset_experiment_clock) {

            //if there is an event code, send the code
            if (reset_experiment_clock_event_code != 0) {
                send_high_priority_event_code_at_front_of_queue(reset_experiment_clock_event_code);
            }

            delay_printf_json_status("experiment clock was reset");
            need_to_reset_experiment_clock = false;
            experiment_tic = 0;
        }

        //increment tic
        experiment_tic++;
    }

    //increment and check for rollover
    experiment_io_count_tic++;
    if (experiment_io_count_tic == freq_multiplier) {
        experiment_io_count_tic = 0;
    }

    // **** EVENT CODES ****
    process_dsp_event_code_queues(cpu_uptime_count);


    // **** STATUS MESSAGES ****
    if (status_messages_enabled && is_even_second) {
        //only if full and db_board is enabled
        if (status_messages_full && db_board.is_enabled()) {
            print_full_status_message();
        } else {
            print_uptime_only();
        }
    }

    // **** TWIDDLE LEDS ****
    if ((twiddle_count == 0) && (!twiddle_on)) {

        if (cpu_uptime_count == off_led_tic) {
            ti_board.blue_led.off();
        }

        if (is_even_second) {
            //blink led
            ti_board.blue_led.on();
            off_led_tic = cpu_uptime_count + static_cast<int64_t>(on_duration_count);
        }
    } else {
        process_led_twiddles();
    }

    // **** UPDATE CLOCKS ****
    cpu_uptime_count++;
    cla_uptime_count = (static_cast<int64_t>(cla_uptime_overflow) << 32) | (static_cast<int64_t>(cla_uptime_count_32));
}



void start_io_controller() {
    if (!has_init_io_controller) {
        delay_printf_json_error("io controller has not been initialized yet");
        return;
    }

    //start the timer
    cpu_timer0.start();
}

bool init_io_controller() {
    if (db_board.is_not_enabled()) {
        delay_printf_json_error("cannot initialize io controller without daughterboard");
        return false;
    }

    if (has_init_io_controller) {return true;}

    //copy the main frequency
    main_loop_freq = static_cast<uint32_t>(freq_multiplier) * static_cast<uint32_t>(ti_board.main_frequency);
    twiddle_cpu_tics = static_cast<uint32_t>(lroundl((static_cast<float64>(twiddle_length_ms)*static_cast<float64>(main_loop_freq))/1000.0L));
    on_duration_count = (25*main_loop_freq)/1000;

    //set up cpu_timer0
    //configure timer0 for time_sensitive_code
    //should not use any interrupts higher than timer0
    cpu_timer0.set_frequency(static_cast<float64>(main_loop_freq));
    cpu_timer0.set_callback(io_controller_main_loop);

    //enable the cla task
    enable_cla_task(2, reinterpret_cast<uint16_t>(&cla_task2_cla_uptime), CLA_TRIG_TINT0);


    //init io

    // **** INIT OUTPUTS ****
    //first, so that the pointer can be sent to the input
    output_count = get_digital_out_count(); // db_board.digital_out_count();

    //allocate the outputs
    //use heap array, since size is unknown
    experimental_outputs_ = create_array_of<experimental_output>(output_count, "experimental_outputs_");
    if (experimental_outputs_ == NULL) {return false;}

    for (uint16_t output_number=0; output_number<output_count; output_number++) {
        experimental_outputs_[output_number].init(output_number, output_number, set_digital_out);
    }

    // **** INIT INPUTS ****
    //input_count = db_board.digital_in_count() + db_board.analog_in_count();
    uint16_t temp_digital_in_count = get_digital_in_count();
    input_count = temp_digital_in_count + get_analog_in_count();

    //allocate and set up for the history
    //use heap array, since size is unknown
    should_print_input_history_array = create_array_of<bool>(input_count, "should_print_input_history_array");
    if (should_print_input_history_array == NULL) {return false;}

    //allocate the inputs (no longer has a default init)
    experimental_inputs_ = create_array_of<experimental_input>(input_count, "experimental_inputs_");
    if (experimental_inputs_ == NULL) {return false;}

    for (uint16_t input_number=0; input_number<input_count; input_number++) {
        should_print_input_history_array[input_number] = false;

        const bool is_digital = (input_number < temp_digital_in_count);
        const uint16_t channel = (input_number % temp_digital_in_count);

        if (is_digital) {
            experimental_inputs_[input_number].init(input_number, channel, is_digital, get_digital_in, experimental_inputs_, experimental_outputs_);
        } else {
            experimental_inputs_[input_number].init(input_number, channel, is_digital, get_analog_in, experimental_inputs_, experimental_outputs_);
        }
    }

    add_serial_command(&command_twiddle);
    add_serial_command(&command_reset_clock);
    add_serial_command(&command_uptime);
    add_serial_command(&command_timestamp);
    add_serial_command(&command_set_input_settings);
    add_serial_command(&command_get_input_settings);
    add_serial_command(&command_set_input_actions);
    add_serial_command(&command_get_input_history);
    add_serial_command(&command_set_output_settings);
    add_serial_command(&command_get_output_settings);
    add_serial_command(&command_set_output_actions);

    has_init_io_controller = true;
    delay_printf_json_status("initialized io controller");
    return true;
}

__attribute__((ramfunc))
float64 get_uptime_seconds() {
    //since cpu_uptime_count is scaled per clock rate, this will also be correct
    //using 64bit for the calculation to hopefully improve the likelihood it will be correct to the end
    return static_cast<float64>(cpu_uptime_count)/static_cast<float64>(main_loop_freq);
}

void print_uptime() {
    const float64 uptime_f = get_uptime_seconds();
    delay_printf_json_objects(1, json_float64("uptime", uptime_f, 4));
}

void twiddle_leds_ten_times() {
    twiddle_count = twiddle_count + 10;
}

bool is_currently_twiddling_leds() {
    return (twiddle_on || twiddle_count > 0);
}

__attribute__((ramfunc))
void process_led_twiddles() {

    if (!twiddle_on) {
        //start with one off and one on
        ti_board.red_led.off();
        ti_board.blue_led.on();
        twiddle_pause_count = 0;
        twiddle_count = twiddle_count - 1;  //this counts as a twiddle
        twiddle_on = true;

    } else {

        twiddle_pause_count = twiddle_pause_count + 1;

        if ((twiddle_pause_count % twiddle_cpu_tics) == 0) {

            if (twiddle_count > 0) {

                //toggle each led
                ti_board.red_led.toggle();
                ti_board.blue_led.toggle();
                twiddle_count = twiddle_count - 1;

            } else {

                //turn off at the end
                ti_board.red_led.off();
                ti_board.blue_led.off();
                twiddle_pause_count = 0;
                twiddle_on = false;
            }
        }
    }
}

__attribute__((ramfunc))
void clear_all_should_print_flags() {
    //reset all flags back to "off"
    should_print_input_history_bool = false;
    desired_history_tics = 1;
    for (uint16_t i=0; i<input_count; i++) {
        should_print_input_history_array[i] = false;
    }
}

__attribute__((ramfunc))
void print_input_history() {

    debug_timestamps.io_send_print_history.tic();
    debug_timestamps.io_print_history_1 = CPU_TIMESTAMP;

    uint16_t available_history_count = 0;
    uint16_t num_of_inputs_to_print = 0;
    bool has_history_still_being_printed = false;

    //loop through each input and check:
    // if the history is actually enabled
    // and verify that the counts are the same for those that are selected (must be)
    for (uint16_t i=0; i<input_count; i++) {
        if (should_print_input_history_array[i]) {
            num_of_inputs_to_print++;

            if (experimental_inputs_[i].is_history_being_printed()) {
                has_history_still_being_printed = true;
                break;
            }

            //for the first input, set the available
            if (num_of_inputs_to_print == 1) {
                available_history_count = experimental_inputs_[i].history_used();
            } else {

                //otherwise, check for inconsistent count
                if (available_history_count != experimental_inputs_[i].history_used()) {
                    clear_all_should_print_flags();
                    debug_timestamps.io_send_print_history.toc();
                    return;
                }
            }
        }
    }

    //if nothing is to be printed, or at least one history is still being printed, exit (but don't clear flags)
    if ((num_of_inputs_to_print == 0) || (available_history_count == 0) || (has_history_still_being_printed)) {
        debug_timestamps.io_send_print_history.toc();
        return;
    }

    uint16_t actual_history_count = desired_history_count;

    //check if actual (i.e. desired) count is greater than available
    if (actual_history_count > available_history_count) {

        //check if the actual is actually just set at the max value (flag for whatever is available)
        if (actual_history_count == USHRT_MAX) {
            //if so, reduce actual to available
            actual_history_count = available_history_count;
        } else {
            //if not (meaning a specific count was requested), just exit (but don't erase flags, let it print next time)
            debug_timestamps.io_send_print_history.toc();
            return;
        }
    }

    debug_timestamps.io_print_history_2 = CPU_TIMESTAMP;

    //if it was rounded down to zero, just exit (but don't erase flags, let it print next time)
    if (actual_history_count == 0) {
        debug_timestamps.io_send_print_history.toc();
        return;
    }

    debug_timestamps.io_print_history_3 = CPU_TIMESTAMP;

    //save a little, create these once
    const json_object_t json_timestamp_obj = json_timestamp(experiment_tic);
    const json_object_t json_reason_obj = json_string("reason","get");
    json_object_t json_parent_obj = blank_json_object;
    //parameter_name is set below
    json_parent_obj.combined_params.value_type = t_parent;
    json_parent_obj.array_count = 3;    //always 3 children (reason, timestamp, history)

    for (uint16_t i=0; i<input_count; i++) {
        if (should_print_input_history_array[i]) {

            //try to copy the history
            json_object_t json_history_obj;
            experimental_inputs_[i].get_copy_of_history(actual_history_count, json_history_obj);

            //if it succeeds or not, print what was copied
            json_parent_obj.parameter_name = experimental_inputs_[i].get_name();
            delay_printf_json_objects(4, json_parent_obj, json_reason_obj, json_timestamp_obj, json_history_obj);
        }
    }

    debug_timestamps.io_print_history_4 = CPU_TIMESTAMP;

    //clear flags if desired_history_tics is 1 (min value meaning erase every time)
    if (desired_history_tics == 1) {
        clear_all_should_print_flags();
    }

    debug_timestamps.io_send_print_history.toc();
}


void reset_experiment_clock(const json_t *const json_root) {

    json_element code_e("event_code", t_uint16);

    //only after setting the elements
    //disable and store the interrupt state
    const uint16_t interrupt_settings = __disable_interrupts();

    need_to_reset_experiment_clock = true;

    if (code_e.set_with_json(json_root, false)) {
        reset_experiment_clock_event_code = code_e.value().uint16_;
    }

    //restore the interrupt state
    __restore_interrupts(interrupt_settings);
}

void get_experiment_timestamp() {
    delay_printf_json_objects(1, json_timestamp(experiment_tic));
}


// **** serial setting functions ****
void get_input_history(const json_t *const json_root) {
    if (db_board.is_not_enabled()) {return;}

    json_element numbers_e("input_numbers", t_uint16, true, true);
    json_element count_e("count", t_uint16);
    json_element tics_e("tics", t_uint64);
    json_element stop_e("stop", t_bool);

    stop_e.set_with_json(json_root, false);
    if ((stop_e.count_found() > 0) && (stop_e.value().bool_)) {
        clear_all_should_print_flags();
        return;
    }

    const uint16_t found_count = set_elements_with_json(json_root, 3, &numbers_e, &count_e, &tics_e);
    if (found_count == 0) {
        return;
    }

    if (numbers_e.count_found() > 0) {
        //clear flags before disabling interrupts
        clear_all_should_print_flags();

        //actually setting the flag must occur within disabled interrupt section
        //disable and store the interrupt state
        const uint16_t interrupt_settings = __disable_interrupts();

        debug_timestamps.io_set_print_history.tic();

        bool has_length_error = false;
        //forced creation of array
        const uint16_t *const numbers_to_print = numbers_e.get_uint16_array();
        uint16_t available_history_length = 0;

        //set each number found
        for (uint16_t i=0; i<numbers_e.count_found(); i++) {
            const uint16_t possible_number = numbers_to_print[i];
            if (possible_number <= input_count) {
                if (experimental_inputs_[possible_number].is_history_enabled()) {

                    //go ahead and enable the copy history (if already enabled, okay)
                    experimental_inputs_[possible_number].enable_history_copy(true);

                    //if the first input, get the history length
                    if (available_history_length == 0) {
                        available_history_length = experimental_inputs_[possible_number].history_length();
                    } else {
                        //otherwise, compare the history length
                        if (available_history_length != experimental_inputs_[possible_number].history_length()) {
                            //if they don't match, flag error
                            has_length_error = true;
                            break;
                        }
                    }
                    should_print_input_history_array[possible_number] = true;
                    should_print_input_history_bool = true;
                } else {
                    delay_printf_json_objects(1, json_uint16("history not enabled for input_number", possible_number));
                }
            } else {
                delay_printf_json_error("input_number is too high");
            }
        }

        //if there is a length error, then clear all and send message
        if (has_length_error) {
            delay_printf_json_error("history lengths are not consistent for selected inputs");
            clear_all_should_print_flags();

        } else {

            //set the desired count
            if (count_e.count_found() > 0) {
                desired_history_count = count_e.value().uint16_;
            } else {
                //default to all
                desired_history_count = USHRT_MAX;
            }

            //set the desired tics (minimum of 1)
            if ((tics_e.count_found() > 0) && (tics_e.value().uint64_ > 1)) {
                desired_history_tics = tics_e.value().uint64_;
            } else {
                desired_history_tics = 1;
            }
        }

        //tag time
        debug_timestamps.io_set_print_history.toc();

        //restore the interrupt state
        __restore_interrupts(interrupt_settings);
    }
}

void set_status_messages(const json_t *const json_root) {
    json_element enable_e("enable", t_bool, true);
    json_element minimal_e("minimal", t_bool);

    if (!enable_e.set_with_json(json_root, true)) {
        return;
    }

    const uint16_t interrupt_settings = __disable_interrupts();

    if (enable_e.value().bool_) {
        if ((minimal_e.count_found() > 0) && minimal_e.value().bool_) {
            delay_printf_json_status("minimal status messages enabled");
            status_messages_full = false;
        } else {
            delay_printf_json_status("full status messages enabled");
            status_messages_full = true;
        }

        status_messages_enabled = true;

    } else {
        delay_printf_json_status("status messages disabled");
        status_messages_enabled = false;
    }

    __restore_interrupts(interrupt_settings);
}

__attribute__((ramfunc))
bool is_valid_input_number(const json_t *const json_root, uint16_t &input_number) {
    json_element number_e("input_number", t_uint16, true);
    input_number = input_count + 1; //invalid number on purpose

    //use set_with_json to not grab the "help"
    if (!number_e.set_with_json(json_root, true)) {
        //delay_printf_json_error("input_number is missing");
        return false;
    } else if (number_e.value().uint16_ < input_count) {
        input_number = number_e.value().uint16_;
        return true;
    } else {
        delay_printf_json_error("input_number is too high");
        return false;
    }
}

__attribute__((ramfunc))
bool is_valid_ouput_number(const json_t *const json_root, uint16_t &output_number) {
    json_element number_e("output_number", t_uint16, true);
    output_number = output_count + 1; //invalid number on purpose

    //use set_with_json to not grab the "help"
    if (!number_e.set_with_json(json_root, true)) {
        //delay_printf_json_error("output_number is missing");
        return false;
    } else if (number_e.value().uint16_ < output_count) {
        output_number = number_e.value().uint16_;
        return true;
    } else {
        delay_printf_json_error("output_number is too high");
        return false;
    }
}


void set_input_settings(const json_t *const json_root) {
    if (db_board.is_not_enabled()) {return;}

    uint16_t number;
    if (is_valid_input_number(json_root, number)) {
        //delay_printf_json_status("here4");
        experimental_inputs_[number].set_settings(json_root);
    }
}

void set_output_settings(const json_t *const json_root) {
    if (db_board.is_not_enabled()) {return;}

    uint16_t number;
    if (is_valid_ouput_number(json_root, number)) {
        experimental_outputs_[number].set_settings(json_root);
    }
}

void get_input_settings(const json_t *const json_root) {
    if (db_board.is_not_enabled()) {return;}

    //FIXME: if help, then send to 0 (or could I make a generic function???)

    uint16_t number;
    if (is_valid_input_number(json_root, number)) {
        experimental_inputs_[number].get_settings(json_root);
    }
}

void get_output_settings(const json_t *const json_root) {
    if (db_board.is_not_enabled()) {return;}

    uint16_t number;
    if (is_valid_ouput_number(json_root, number)) {
        experimental_outputs_[number].get_settings(json_root);
    }
}

void set_input_actions(const json_t *const json_root) {
    if (db_board.is_not_enabled()) {return;}

    uint16_t number;
    if (is_valid_input_number(json_root, number)) {
        experimental_inputs_[number].set_actions(json_root);
    }
}

void set_output_actions(const json_t *const json_root) {
    if (db_board.is_not_enabled()) {return;}

    uint16_t number;
    if (is_valid_ouput_number(json_root, number)) {
        experimental_outputs_[number].set_actions(json_root);
    }
}

const char *get_reason_name(reason_t reason_i) {
    const char *ptr;

    switch (reason_i) {
        case get_r:
            ptr = "get";
            break;
        case set_r:
            ptr = "set";
            break;
        case ext_r:
            ptr = "external_event";
            break;
        default:
            ptr = "error";
            break;
    }

    return ptr;
}

uint16_t get_io_input_count() {
    return input_count;
}

uint16_t get_io_output_count() {
    return output_count;
}
