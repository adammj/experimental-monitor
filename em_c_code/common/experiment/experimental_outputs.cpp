/*
 * experiment_outputs.cpp
 *
 *  Created on: Oct 10, 2017
 *      Author: adam jones
 */

#include "experimental_outputs.h"
#include "dsp_output.h"
#include "printf_json_delayed.h"    //for safety, should only have delayed prints
#include "extract_json.h"
#include "tic_toc.h"


//internal variables
static json_element enable_e("enabled", t_bool);
static json_element on_value_e("on_value", t_uint16);
static json_element off_value_e("off_value", t_uint16);
static json_element is_cont_e("is_continuous", t_bool);
static json_element on_tics_e("on_tics", t_uint64);
static json_element off_tics_e("off_tics", t_uint64);
static json_element event_on_e("event_code_on", t_uint16);
static json_element event_off_e("event_code_off", t_uint16);
static json_element value_e("set_value", t_uint16);
static json_element send_to_computer_out_e("send_to_computer", t_bool);
static json_element all_transitions_out_e("all_transitions", t_bool);
static json_element settings_out_e("get_settings", t_bool);


void experimental_output::init(uint16_t number, uint16_t channel, const volatile output_set_function set_function) volatile {
    //default construction
    is_enabled_ = false;
    current_value_ = 0;
    is_continuous_ = false;
    is_in_a_cycle_now_ = false;
    cycle_count_ = 0;
    on_value_ = 1;
    off_value_ = 0;
    length_on_tics_ = 1;    //must always be greater than 0
    length_off_tics_ = 1;   //must always be greater than 0
    event_codes_.on = 0;
    event_codes_.off = 0;
    is_digital_ = true;
    output_start_cycle_msg_to_computer_ = false;
    all_transitions_ = false;
    output_cycle_off_tic_ = 0;
    output_cycle_finished_tic_ = 0;
    clock_tic_ = 0;
    start_delay_tic_ = 0;

    //from init
    number_ = number;
    channel_ = channel;
    set_function_ = set_function;

    if (number < 100 ) {
        //label the input
        name_[0] = 'o';
        name_[1] = 'u';
        name_[2] = 't';
        name_[3] = 'p';
        name_[4] = 'u';
        name_[5] = 't';
        name_[6] = '_';

        //zeros place
        uint16_t temp_value = number % 10;
        name_[8] = '0' + static_cast<char>(temp_value);

        //tens place
        temp_value = ((number - temp_value)/10) % 10;
        name_[7] = '0' + static_cast<char>(temp_value);

        //null terminator
        name_[9] = 0;

    } else {
        delay_printf_json_error("output name not ready for > 99");
    }

    if (set_function_ == NULL) {
        this->printf_error("set function points to null");
        return;
    }

    this->set_current_value(current_value_, false);

    is_enabled_ = true;
}


void experimental_output::set_on_off_tics(uint64_t on_tics, uint64_t off_tics) {
    length_on_tics_ = on_tics;
    length_off_tics_ = off_tics;
}

void experimental_output::set_current_value(uint16_t new_value, bool send_codes_and_messages) volatile {
    set_function_(channel_, new_value);
    current_value_ = new_value;

    if (!send_codes_and_messages) {
        return;
    }

    if (current_value_ == on_value_) {
        if (event_codes_.on > 0) {
            send_high_priority_event_code(event_codes_.on);
        }

        if (output_start_cycle_msg_to_computer_) {
            delay_printf_json_objects(4, json_parent(const_cast<const char*>(name_), 3),
                                         json_string("reason", "external_event"),
                                         json_timestamp(clock_tic_),
                                         json_bool("output_on", true));
        }
    } else if (current_value_ == off_value_) {
        if (event_codes_.off > 0) {
            send_high_priority_event_code(event_codes_.off);
        }

        if (output_start_cycle_msg_to_computer_ && all_transitions_) {
            delay_printf_json_objects(4, json_parent(const_cast<const char*>(name_), 3),
                                         json_string("reason", "external_event"),
                                         json_timestamp(clock_tic_),
                                         json_bool("output_on", false));
        }
    }
}

__attribute__((ramfunc))
void experimental_output::add_cycles(uint16_t add_cycles, uint64_t start_tic) volatile {

    //if there are currently no cycles in the queue, then set the start_tic delay
    if (cycle_count_ == 0) {
        start_delay_tic_ = start_tic;
    } else {
        start_delay_tic_ = 0;
    }

    cycle_count_ += add_cycles;
}

void experimental_output::set_event_codes(const io_event_codes_t event_codes) volatile {
    event_codes_.off = event_codes.off;
    event_codes_.on = event_codes.on;
}

void experimental_output::set_on_off_values(uint16_t on_value, uint16_t off_value) {
    on_value_ = on_value;
    off_value_ = off_value;
}

__attribute__((ramfunc))
void experimental_output::print_current_value() volatile const {
    delay_printf_json_objects(4, json_parent(const_cast<const char*>(name_), 3),
                              json_string("reason","get"),
                              json_timestamp(clock_tic_),
                              json_uint16("value", current_value_));
}

__attribute__((ramfunc))
void experimental_output::process_actions(uint64_t experiment_tic) volatile {
    clock_tic_ = experiment_tic;

    //exit, if not enabled
    if (!is_enabled_) {

        //if it was in a cycle, make sure to exit gracefully
        if (is_in_a_cycle_now_) {
            is_in_a_cycle_now_ = false;
            //if current value is not off, then set off
            if (current_value_ != off_value_) {
                this->set_current_value(off_value_, true);
            }
        }

        //exit
        return;
    }

    if (is_in_a_cycle_now_) {

        //if the cycle hasn't finished
        if (clock_tic_ < output_cycle_finished_tic_) {

            //if the off part is ready, then set off
            if ((clock_tic_ >= output_cycle_off_tic_) && (current_value_ != off_value_)) {
                this->set_current_value(off_value_, true);
            }

            //exit (not done with cycle)
            return;
        } else {

            //otherwise, done with cycle
            is_in_a_cycle_now_ = false;

            //sanity check on off value
            if (current_value_ != off_value_) {
                this->set_current_value(off_value_, true);
            }

        }
    }

    if ((cycle_count_ > 0) || is_continuous_) {
        bool actually_start_cycle = false;

        //check if continuous
        if (is_continuous_) {
            cycle_count_ = 0;
            actually_start_cycle = true;

        } else {
            //otherwise, check if start_delay_tic_ is now or has passed
            if (clock_tic_ >= start_delay_tic_) {
                cycle_count_--;
                actually_start_cycle = true;
            }
        }

        if (actually_start_cycle) {
            this->set_current_value(on_value_, true);
            is_in_a_cycle_now_ = true;
            output_cycle_off_tic_ = clock_tic_ + length_on_tics_;
            output_cycle_finished_tic_ = output_cycle_off_tic_ + length_off_tics_;
        }
    }
}

void experimental_output::get_settings(const json_t *const json_root) volatile const {
    json_element get_value_e("get_value", t_bool);

    const uint16_t interrupt_settings = __disable_interrupts();

    if (get_value_e.set_with_json(json_root, false))  {
        this->print_current_value();
    } else {
        this->print_settings(get_r);
    }

    //restore the interrupt state
    __restore_interrupts(interrupt_settings);
}

void experimental_output::print_settings(reason_t reason_code) volatile const {
    const char *const reason = get_reason_name(reason_code);

    delay_printf_json_objects(13, json_parent(const_cast<const char*>(name_), 12),
                                  json_string("reason", reason),
                                  json_timestamp(clock_tic_),
                                  json_bool("enabled", is_enabled_),
                                  json_uint16("value", current_value_),
                                  json_uint16("on_value", on_value_),
                                  json_uint16("off_value", off_value_),
                                  json_bool("is_continuous", is_continuous_),
                                  json_uint64("on_tics", length_on_tics_),
                                  json_uint64("off_tics", length_off_tics_),
                                  json_uint16("event_code_on", event_codes_.on),
                                  json_uint16("event_code_off", event_codes_.off),
                                  json_bool("send_to_computer_on_start", output_start_cycle_msg_to_computer_));
}


// **** setting functions ****
void experimental_output::set_actions(const json_t *const json_root) volatile {
    json_element number_e("output_number", t_uint16, true);
    if ((!number_e.set_with_json(json_root, true)) || (number_e.value().uint16_ != number_)) {
        this->printf_error("'output_number' is wrong");
        return;
    }

    //actions
    json_element add_cycles_e("add_cycles", t_uint16);

    if (!add_cycles_e.set_with_json(json_root, true)) {
        return;
    }

    //only after setting the elements
    //disable and store the interrupt state
    const uint16_t interrupt_settings = __disable_interrupts();

    cycle_count_ += add_cycles_e.value().uint16_;

    //restore the interrupt state
    __restore_interrupts(interrupt_settings);
}

__attribute__((ramfunc))
void experimental_output::printf_error(const char *const message) volatile const {
    delay_printf_json_objects(3, json_string("error", message),
                              json_uint16("output", number_),
                              json_timestamp(clock_tic_));
}

void experimental_output::set_settings(const json_t *const json_root) volatile {
    json_element number_e("output_number", t_uint16, true);
    if ((!number_e.set_with_json(json_root, true)) || (number_e.value().uint16_ != number_)) {
        this->printf_error("'output_number' is wrong");
        return;
    }

    const uint16_t found_count = set_elements_with_json(json_root, 13, &number_e, &enable_e, &on_value_e, &off_value_e,
                                       &is_cont_e, &on_tics_e, &off_tics_e, &event_on_e,
                                       &event_off_e, &value_e, &send_to_computer_out_e, &settings_out_e,
                                       &all_transitions_out_e);

    if (found_count == 0) {
        return;
    }

    //only after setting the elements
    //disable and store the interrupt state
    const uint16_t interrupt_settings = __disable_interrupts();

    debug_timestamps.exp_output_set_settings.tic();

    if (on_tics_e.count_found() > 0) {
        uint64_t temp_value = on_tics_e.value().uint64_;
        if (temp_value == 0) {
            this->printf_error("on_tics cannot be 0");
        } else {
            length_on_tics_ =temp_value;
        }
    }
    if (off_tics_e.count_found() > 0) {
        uint64_t temp_value = off_tics_e.value().uint64_;
        if (temp_value == 0) {
            this->printf_error("off_tics cannot be 0");
        } else {
            length_off_tics_ = temp_value;
        }
    }
    if (enable_e.count_found() > 0) {
        is_enabled_ = enable_e.value().bool_;
    }
    if (on_value_e.count_found() > 0) {
        uint16_t tmp_val = on_value_e.value().uint16_;
        if (is_digital_ && (tmp_val > 1)) {
            tmp_val = 1;
        }
        on_value_ = tmp_val;
    }
    if (off_value_e.count_found() > 0) {
        uint16_t tmp_val = off_value_e.value().uint16_;
        if (is_digital_ && (tmp_val > 1)) {
            tmp_val = 1;
        }
        off_value_ = tmp_val;
    }
    if (event_on_e.count_found() > 0) {
        if ((event_on_e.value().uint16_ > 127) && (event_on_e.value().uint16_ < 256)) {
            event_codes_.on = event_on_e.value().uint16_;
        } else {
            this->printf_error("event code outside of range 128-255");
        }
    }
    if (event_off_e.count_found() > 0) {
        if ((event_off_e.value().uint16_ > 127) && (event_off_e.value().uint16_ < 256)) {
            event_codes_.off = event_off_e.value().uint16_;
        } else {
            this->printf_error("event code outside of range 128-255");
        }
    }
    if (value_e.count_found() > 0) {
        uint16_t tmp_val = value_e.value().uint16_;
        if (is_digital_ && (tmp_val > 1)) {
            tmp_val = 1;
        }

        //this is the only time codes/messages are not sent
        this->set_current_value(tmp_val, false);
    }
    if (is_cont_e.count_found() > 0) {
        is_continuous_ = is_cont_e.value().bool_;
    }
    if (send_to_computer_out_e.count_found() > 0) {
        output_start_cycle_msg_to_computer_ = send_to_computer_out_e.value().bool_;
    }
    if (all_transitions_out_e.count_found() > 0) {
        all_transitions_ = all_transitions_out_e.value().bool_;
    }

    if ((settings_out_e.count_found() > 0) && (settings_out_e.value().bool_)) {
        this->print_settings(set_r);
    }

    debug_timestamps.exp_output_set_settings.toc();

    //restore the interrupt state
    __restore_interrupts(interrupt_settings);
}
