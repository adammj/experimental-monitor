/*
 * experiment_inputs.cpp
 *
 *  Created on: Oct 10, 2017
 *      Author: adam jones
 */


#include "experimental_inputs.h"
#include "analog_input.h"
#include "digital_io.h"
#include "printf_json_types.h"
#include "printf_json_delayed.h"    //for safety, should only have delayed prints
#include "scia.h"
#include "extract_json.h"
#include "cpu_timers.h"
#include "misc.h"
#include "math.h"
#include "dsp_output.h"
#include "tic_toc.h"
#include "string.h"
#include "arrays.h"


//internal variables
//strings used for parameter names
static const char *history_enabled_string = "history_enabled";
static const char *history_length_string = "history_length";
static const char *target_met_min_tics_string = "target_met_min_tics";
static const char *target_left_min_tics_string = "target_left_min_tics";
static const char *actions_enabled_string = "actions_enabled";
static const char *actions_disabled_after_met_string = "actions_disabled_after_met";
static const char *all_transitions_string = "all_transitions";
static const char *send_to_computer_string = "send_to_computer";
static const char *event_code_met_string = "event_code_met";
static const char *event_code_left_string = "event_code_left";
static const char *output_enabled_string = "output_enabled";
static const char *output_disabled_after_string = "output_disabled_after_met";
static const char *output_cycles_string = "output_cycles";
static const char *output_delay_tics_string = "output_delay_tics";
static const char *target_string = "target";
static const char *target_type_string = "target_type";
static const char *target_value_string = "target_value";
static const char *target_distance_string = "target_distance";
static const char *readout_enabled_string = "readout_enabled";
static const char *readout_tics_string = "readout_tics";
static const char *timeout_tics_string = "timeout_tics";
static const char *threshold_enabled_string = "threshold_enabled";
static const char *threshold_value_string =  "threshold_value";

//printing settings
static json_object_t print_reason_o = json_string("reason", "");
static json_object_t print_timestamp_o = json_timestamp(0);
static json_object_t print_enabled_o = json_bool("enabled", false);
static json_object_t print_digital_o = json_bool("digital", false);
static json_object_t print_value_o = json_uint16("value", 0);
static json_object_t print_hist_en_o = json_bool(history_enabled_string, false);
static json_object_t print_hist_len_o = json_uint16(history_length_string, 0);
static json_object_t print_all_tran_o = json_bool(all_transitions_string, false);
static json_object_t print_send_c_o = json_bool(send_to_computer_string, false);
static json_object_t print_act_en_o = json_bool(actions_enabled_string, false);
static json_object_t print_act_dis_o = json_bool(actions_disabled_after_met_string, false);
static json_object_t print_read_en_o = json_bool(readout_enabled_string, false);
static json_object_t print_read_x_o = json_uint64(readout_tics_string, 0);
static json_object_t print_timeout_o = json_uint64(timeout_tics_string, 0);
static json_object_t print_ec_met_o = json_uint16(event_code_met_string, 0);
static json_object_t print_ec_left_o = json_uint16(event_code_left_string, 0);
static json_object_t print_out_en_o = json_bool(output_enabled_string, false);
static json_object_t print_has_out_o = json_bool("has_output", false);
static json_object_t print_out_dis_o = json_bool(output_disabled_after_string, false);
static json_object_t print_out_cyc_o = json_uint16(output_cycles_string, 0);
static json_object_t print_out_del_o = json_uint64(output_delay_tics_string, 0);
static json_object_t print_has_p_o = json_bool("has_parent", false);
static json_object_t print_has_c_o = json_bool("has_child", false);
static json_object_t print_tar_set_o = json_bool("target_set", false);
static json_object_t print_tar_met_o = json_bool("target_met", false);
static json_object_t print_met_min_o = json_uint64(target_met_min_tics_string, 0);
static json_object_t print_left_min_o = json_uint64(target_left_min_tics_string, 0);
static json_object_t print_target_o = json_uint16(target_string, 0);
static json_object_t print_target_typ_o = json_string(target_type_string, "");
static json_object_t print_target_val_o = json_uint16(target_value_string, 0);
static json_object_t print_target_dist_o = json_uint16(target_distance_string, 0);
static json_object_t print_threshold_count_o = json_uint16("threshold_count", 0);


//setting-changing
//history
static json_element history_enabled_e(history_enabled_string, t_bool);
static json_element history_length_e(history_length_string, t_uint16);
//readout
static json_element enable_readout_e(readout_enabled_string, t_bool);
static json_element readout_length_e(readout_tics_string, t_uint16);
//min length target
static json_element met_min_tics_e(target_met_min_tics_string, t_uint64);
static json_element left_min_tics_e(target_left_min_tics_string, t_uint64);
static json_element reset_target_e("reset_target", t_bool);
//digital target
static json_element digital_target_e(target_string, t_uint16);
//analog target
static json_element analog_type_e(target_type_string, t_string);
static json_element analog_value_e(target_value_string, t_uint16);
static json_element analog_distance_e(target_distance_string, t_uint16);
//actions
static json_element actions_enable_e(actions_enabled_string, t_bool);
static json_element disable_actions_after_e(actions_disabled_after_met_string, t_bool);
//messages
static json_element all_transitions_e(all_transitions_string, t_bool);
static json_element send_to_computer_e(send_to_computer_string, t_bool);
static json_element event_code_met_e(event_code_met_string, t_uint16);
static json_element event_code_left_e(event_code_left_string, t_uint16);
//child
static json_element enable_child_e("enable_child", t_bool);
static json_element child_number_e("child_number", t_uint16);
//output
static json_element enable_output_e(output_enabled_string, t_bool);
static json_element output_number_e("output_number", t_uint16);
static json_element output_disable_after_e(output_disabled_after_string, t_bool);
static json_element output_cycles_e(output_cycles_string, t_uint16);
static json_element output_delay_tics_e(output_delay_tics_string, t_uint64);
//timeout
static json_element timeout_e(timeout_tics_string, t_uint64);
static json_element settings_e("get_settings", t_bool);
//threshold
static json_element threshold_enabled_e(threshold_enabled_string, t_bool);
static json_element threshold_value_e(threshold_value_string, t_uint16);


void experimental_input::init(uint16_t number, uint16_t channel, bool is_digital, const volatile input_get_function get_function, const volatile experimental_input *const inputs_array, const volatile experimental_output *const outputs_array) volatile {
    //default construction
    clock_tic_ = 0;
    is_enabled_ = false;
    is_digital_ = true;

    //target
    target_set_ = false;
    target_met_actions_enabled_ = true;
    target_met_msg_on_all_transitions_ = false;
    disable_actions_after_target_met_ = false;

    //history
    history_enabled_ = false;
    history_copy_enabled_ = false;
    history_is_printing_ = false;
    history_copy_array_ = NULL;
    history_length_ = 0;

    //parents, children
    parent_input_ = NULL;
    child_input_ = NULL;

    //timeout
    is_in_timeout_ = false;
    timeout_length_tics_ = 0;

    //readout
    readout_every_x_tics_ = 0;

    //event codes
    event_codes_.on = 0;
    event_codes_.off = 0;

    //target met/left
    target_met_ = false;
    target_met_min_length_tics_ = 0;
    target_left_min_length_tics_ = 0;
    target_met_msg_to_computer_ = false;
    target_conditionally_met_tic_ = 0;
    target_conditionally_left_tic_ = 0;
    timeout_start_tic_ = 0;

    //output
    output_ptr_ = NULL;
    output_enabled_ = true;
    disable_output_after_target_met_once_ = false;
    output_cycle_counts_ = 1;
    output_delay_tics_ = 0;

    //threshold
    threshold_enabled_ = false;
    threshold_value_ = 0;
    threshold_count_ = 0;

    //original init
    number_ = number;
    channel_ = channel;
    is_digital_ = is_digital;
    get_function_ = get_function;
    inputs_array_ = inputs_array;
    outputs_array_ = outputs_array;

    if (number < 100 ) {
        //label the input
        name_[0] = 'i';
        name_[1] = 'n';
        name_[2] = 'p';
        name_[3] = 'u';
        name_[4] = 't';
        name_[5] = '_';

        //zeros place
        uint16_t temp_value = number % 10;
        name_[7] = '0' + static_cast<char>(temp_value);

        //tens place
        temp_value = ((number - temp_value)/10) % 10;
        name_[6] = '0' + static_cast<char>(temp_value);

        //null terminator
        name_[8] = 0;

    } else {
        //use regular json_error here
        delay_printf_json_error("input name not ready for > 99");
    }

    //check function once
    if (get_function_ == NULL) {
        //use regular json_error here
        this->printf_error("get function points to null");
        is_enabled_ = false;
        return;
    }
    is_enabled_ = true;
}

void experimental_input::enable_child(bool enable, uint16_t number) volatile {
    //if disabling, remove child
    if (!enable) {

        //if the child has a child
        if (child_input_->child_input_ != NULL) {
            volatile experimental_input *const child_ptr = child_input_;
            volatile experimental_input *const grandchild_ptr = child_input_->child_input_;

            //remove child from chain
            child_ptr->parent_input_ = NULL;
            child_ptr->child_input_ = NULL;

            //splice in the grandchild
            child_input_ = grandchild_ptr;
            child_input_->parent_input_ = this;

        } else {
            //if there is a child (with no children)

            if (child_input_ != NULL) {
                //remove self as the child's parent
                child_input_->parent_input_ = NULL;
            }
            //clear the child
            child_input_ = NULL;
        }

        return;
    }

    //check conditions
    if (number_ == number) {
        this->printf_error("child cannot be set by itself");
        return;
    }
    if (number >= get_io_input_count()) {
        this->printf_error("child number is greater than number of inputs");
        return;
    }
    if (parent_input_ != NULL) {
        this->printf_error("current input is not a primary input");
        return;
    }
    if ((!inputs_array_[number].is_enabled_) || (!inputs_array_[number].target_set_)) {
        this->printf_error("child input is not enabled or does not have a target");
        return;
    }
    if ((inputs_array_[number].child_input_) != NULL) {
        this->printf_error("child input already has its own child input");
        return;
    }
    if (is_digital_ != inputs_array_[number].is_digital_) {
        this->printf_error("child input does not have the same input type (digital/analog)");
        return;
    }

    //if analog, check that types are the same
    if (!is_digital_) {
        if (target_.analog.type != inputs_array_[number].target_.analog.type) {
            this->printf_error("child input does not have the same analog_type");
            return;
        }
    }

    //point to the child, and the child pointing back to the parent
    child_input_ = const_cast<volatile experimental_input *>(&(inputs_array_[number]));
    child_input_->parent_input_ = this;
}

__attribute__((ramfunc))
void experimental_input::update_current_value(uint64_t experiment_tic) volatile {
    if (!is_enabled_) {return;}

    clock_tic_ = experiment_tic;
    current_value_ = get_function_(channel_);

    if (history_enabled_) {

        //before writing to history, check if threshold is enabled
        if (threshold_enabled_) {
            //if full, then pop off the value first
            if (history_ring_buffer_.is_full()) {

                uint16_t read_value;
                if (history_ring_buffer_.read(read_value)) {
                    if (read_value < threshold_value_) {
                        //sanity check
                        if (threshold_count_ > 0) {
                            threshold_count_--;
                        }
                    }
                } else {
                    history_enabled_ = false;
                    this->printf_error("unable to read from history");
                }
            }

            //now, update the count
            if (current_value_ < threshold_value_) {
                threshold_count_++;

                //sanity check
                if (threshold_count_ > history_length_) {
                    threshold_count_ = history_length_;
                }
            }
        }

        if (!history_ring_buffer_.write(current_value_)) {
            history_enabled_ = false;
            this->printf_error("could not write to history");
        }
    }
}

__attribute__((ramfunc))
void experimental_input::print_current_value() volatile const {
    delay_printf_json_objects(4, json_parent(const_cast<const char*>(name_), 3),
                              json_string("reason","get"),
                              json_timestamp(clock_tic_),
                              json_uint16("value", current_value_));
}

__attribute__((ramfunc))
void experimental_input::print_threshold_count() volatile const {
    if (threshold_enabled_) {
        delay_printf_json_objects(4, json_parent(const_cast<const char*>(name_), 3),
                                  json_string("reason","get"),
                                  json_timestamp(clock_tic_),
                                  json_uint16("threshold_count", threshold_count_));
    } else {
        this->printf_error("threshold is not enabled");
    }
}

//code to process all of the actions
__attribute__((ramfunc))
void experimental_input::process_actions() volatile {
    //exit, if not enabled
    if (!is_enabled_) {return;}

    //assumes that current value is already current

    //if readout is enabled and the time is right (mod == 0), then print
    if ((readout_every_x_tics_ > 0) && ((clock_tic_ % readout_every_x_tics_) == 0)) {
        this->print_current_value();
    }

    //non-primary input statuses will be set by the up-most parent
    if (parent_input_ != NULL) {
        return;
    }

    //if in timeout, see if it still is, or can leave
    if (is_in_timeout_) {
        //if still in timeout, exit function
        if ((timeout_start_tic_ + timeout_length_tics_) > clock_tic_) {
            return;
        } else {
            //else, exit timeout
            is_in_timeout_ = false;
            timeout_start_tic_ = 0;
        }
    }

    //if no target, then skip any further tests
    if (!target_set_) {return;}

    //for one, simple (for multiple, check all)
    const bool target_conditionally_met = this->does_value_meet_all_targets();
    if (target_conditionally_met && (target_conditionally_met_tic_ == 0)) {
        target_conditionally_met_tic_ = clock_tic_;
    } else if ((!target_conditionally_met) && (target_conditionally_left_tic_ == 0)) {
        target_conditionally_left_tic_ = clock_tic_;
    }

    //reset conditional met/left
    if (target_conditionally_met) {
        target_conditionally_left_tic_ = 0;
    } else {
        target_conditionally_met_tic_ = 0;
    }

    //check how long it has been met
    bool target_actually_met_or_left = false;
    //if the target is conditionally met, and was left
    if (target_conditionally_met && (!target_met_)) {
        const uint64_t tics_since_target_conditionally_met_or_left = clock_tic_ - target_conditionally_met_tic_;
        if (tics_since_target_conditionally_met_or_left >= target_met_min_length_tics_) {
            this->do_target_actions(target_conditionally_met);
            target_actually_met_or_left = true;
        }

    //else, if the target is conditionally left, and was met
    } else if ((!target_conditionally_met) && target_met_){
        const uint64_t tics_since_target_conditionally_met_or_left = clock_tic_ - target_conditionally_left_tic_;
        if (tics_since_target_conditionally_met_or_left >= target_left_min_length_tics_) {
            this->do_target_actions(target_conditionally_met);
            target_actually_met_or_left = true;
        }
    }

    //inform all children
    if (target_actually_met_or_left && (child_input_ != NULL)) {
        volatile experimental_input *input_pointer = this;
        while (input_pointer->child_input_ != NULL) {
            input_pointer = input_pointer->child_input_;
            input_pointer->do_target_actions(target_conditionally_met);
        }
    }
}

__attribute__((ramfunc))
void experimental_input::do_target_actions(bool target_met) volatile {
    //uint32_t start_time = get_cpu_timestamp();
    bool should_send_target_met_messages = false;
    bool output_was_queued = false;

    //if actions are disabled, then just set and exit
    if (!target_met_actions_enabled_) {
        target_met_ = target_met;
        return;
    }

    //if tracking transitions, and there was a change, then send it
    if (target_met_msg_on_all_transitions_ && (target_met != target_met_)) {
        should_send_target_met_messages = true;
    }

    //if the target is met, and it wasn't before
    if (target_met && (!target_met_)) {
        should_send_target_met_messages = true;

        if (timeout_length_tics_ > 0) {
            timeout_start_tic_ = clock_tic_;
            is_in_timeout_ = true;
        }

        if (output_enabled_ && (output_ptr_ != NULL)) {
            output_was_queued = true;

            if (disable_output_after_target_met_once_) {
                output_enabled_ = false;
            }

            if (output_ptr_->is_continuous()) {
                output_ptr_->enable(true);
            } else {
                output_ptr_->add_cycles(output_cycle_counts_, clock_tic_ + output_delay_tics_);
            }
        }
    }

    if (should_send_target_met_messages) {
        this->send_target_met_message(target_met, output_was_queued);
    }

    //if the target isn't met, and it was before
    if ((!target_met) && target_met_) {
        //reset everything
        target_conditionally_met_tic_ = 0;
        //if the output is enabled and continuous
        if (output_enabled_ && (output_ptr_ != NULL) && output_ptr_->is_continuous()) {
            output_ptr_->enable(false);
        }
    }

    //finally, update the value
    target_met_ = target_met;

    //disable the actions, if desired
    if (disable_actions_after_target_met_ && target_met_ && target_met_actions_enabled_) {
        target_met_actions_enabled_ = false;
    }
}

const volatile experimental_input *experimental_input::highest_primary() volatile const {
    const volatile experimental_input *input_pointer = this;

    //go up the chain to the top
    while (input_pointer->parent_input_ != NULL) {
        input_pointer = input_pointer->parent_input_;
    }

    return input_pointer;
}

__attribute__((ramfunc))
bool experimental_input::does_value_meet_this_target() volatile const {
    bool does_meet = false;

    if (is_digital_) {
        does_meet = (current_value_ == target_.digital.polarity);
    } else {
        switch(target_.analog.type) {
            case less_than:
                does_meet = (current_value_ < target_.analog.value);
                break;

            case less_than_or_equal_to:
                does_meet = (current_value_ <= target_.analog.value);
                break;

            case greater_than:
                does_meet = (current_value_ > target_.analog.value);
                break;

            case greater_than_or_equal_to:
                does_meet = (current_value_ >= target_.analog.value);
                break;

            case rectangular_distance:
                does_meet = (labs(static_cast<int32_t>(current_value_) - static_cast<int32_t>(target_.analog.value)) <= static_cast<int32_t>(target_.analog.distance));
                break;
        }
    }

    //catch all
    return does_meet;
}

__attribute__((ramfunc))
bool experimental_input::does_value_meet_all_targets() volatile const {
    //uint32_t start_time = get_cpu_timestamp();

    if (parent_input_ != NULL) {
        this->printf_error("should not be checking a child");
        return false;
    }

    const volatile experimental_input *input_pointer = this;

    //it is analog, less_than_or_equal_to_distance, and has a child
    if ((!is_digital_) && (child_input_ != NULL) && ((target_.analog.type == circular_distance) || (target_.analog.type == elliptical_distance))) {

        const bool is_elliptical = (target_.analog.type == elliptical_distance);
        float32 sum_of_terms = 0;
        float32 current_term = 0;
        float32 current_divisor = static_cast<float32>(input_pointer->target_.analog.distance);

        //if circular, then calculate current_divisor once
        if (!is_elliptical) {
            current_divisor *= current_divisor; //square current_divisor
            current_divisor = 1.0 / current_divisor; //invert to make it multiplication
        }

        //calculate each term (from parent on down), and add to sum_of_terms
        while (input_pointer->child_input_ != NULL) {

            //if elliptical, then calculate current_divisor each time
            if (is_elliptical) {
                current_divisor = static_cast<float32>(input_pointer->target_.analog.distance);
                current_divisor *= current_divisor; //square current_divisor
                current_divisor = 1.0 / current_divisor; //invert to make it multiplication
            }

            //get the current term
            current_term = static_cast<float32>(input_pointer->current_value_) - static_cast<float32>(input_pointer->target_.analog.value);
            current_term *= current_term;   //square self
            current_term *= current_divisor;  //"divide" by current_divisor

            //add to sum
            sum_of_terms += current_term;

            //get the child pointer
            input_pointer = input_pointer->child_input_;
        }

        if (is_elliptical) {
            current_divisor = static_cast<float32>(input_pointer->target_.analog.distance);
            current_divisor *= current_divisor; //square child_divisor
            current_divisor = 1.0 / current_divisor; //invert to make it multiplication
        }

        //now, calculate final one
        current_term = static_cast<float32>(input_pointer->current_value_) - static_cast<float32>(input_pointer->target_.analog.value);
        current_term *= current_term;   //square self
        current_term *= current_divisor; //"divide" by current_divisor

        //add to sum
        sum_of_terms += current_term;

        //and make sure inside the N-D circle or ellipse
        return (sum_of_terms <= 1.0);

    } else {

        //else, check each input independently, down the tree

        //now, go down - checking each value against its target
        while (input_pointer->child_input_ != NULL) {
            //if any are bad, the exit false
            if (!input_pointer->does_value_meet_this_target()) {
                return false;
            }

            //get the child pointer
            input_pointer = input_pointer->child_input_;
        }

        //now, check the final one
        return (input_pointer->does_value_meet_this_target());
    }
}

__attribute__((ramfunc))
void experimental_input::printf_error(const char *const message) volatile const {
    delay_printf_json_objects(3, json_string("error", message),
                              json_uint16("input", number_),
                              json_timestamp(clock_tic_));
}

__attribute__((ramfunc))
void experimental_input::printf_status(const char *const message) volatile const {
    delay_printf_json_objects(3, json_string("status", message),
                              json_uint16("input", number_),
                              json_timestamp(clock_tic_));
}

__attribute__((ramfunc))
void experimental_input::send_target_met_message(bool target_met, bool output_queued) volatile const {

    if (target_met_msg_to_computer_) {
        delay_printf_json_objects(5, json_parent(const_cast<const char*>(name_), 4),
                                  json_string("reason","external_event"),
                                  json_timestamp(clock_tic_),
                                  json_bool("target_met", target_met),
                                  json_bool("output_queued", output_queued));
    }

    if (target_met) {
        if (event_codes_.on > 0) {
            send_high_priority_event_code(event_codes_.on);    //send with high priority
        }
    } else {
        if (event_codes_.off > 0) {
            send_high_priority_event_code(event_codes_.off);    //send with high priority
        }
    }
}

void experimental_input::enable_readout(bool enable, uint64_t readout_every_x_tics) volatile{
    if (!enable) {
        readout_every_x_tics_ = 0;
        return;
    }

    readout_every_x_tics_ = readout_every_x_tics;
}

__attribute__((ramfunc))
void experimental_input::enable_history_copy(bool enable) volatile {
    //this is only called once the history is actually requested
    //(honestly, probably don't even need to do this, could have some master memory block to use for history)
    //so no memory is allocated for the copy array until asked for

    //if enable
    if (enable) {
        if(!history_enabled_) {
            this->printf_error("history is not enabled, cannot enable copy");
            return;
        }

        if (!history_copy_enabled_) {
            //use heap array, since size is unknown
            const uint16_t size = history_ring_buffer_.size();
            history_copy_array_ = create_array_of<uint16_t>(size, "history_copy_array");
            if (history_copy_array_ != NULL) {
                history_copy_enabled_ = true;
            }
        }
    } else if (history_copy_enabled_) {
        //else disable, and the copy is enabled
        delete_array(history_copy_array_);
        history_copy_enabled_ = false;
    }
}

void experimental_input::enable_threshold(bool enable, uint16_t threshold_value) volatile {

    //if disabling, just disable and exit
    if (!enable) {
        threshold_enabled_ = false;
        threshold_value_ = 0;
        threshold_count_ = 0;
        return;
    }

    if (!history_enabled_) {
        this->printf_error("history must be enabled to enable threshold");
        return;
    }

    //set values and reset count
    threshold_value_ = threshold_value;
    threshold_count_ = 0;
    threshold_enabled_ = true;
    history_ring_buffer_.reset();  //must force empty to get accurate counts
}

void experimental_input::enable_history(bool enable, uint16_t history_length) volatile {
    if (history_enabled_ == enable) {
        //just exit if the same
        return;
    }

    //give warning, but continue
    if (history_length > 4096) {
        this->printf_status("warning: input history lengths > 4096 incur a progressively worse penalty during copies");
    }

    if (enable) {
        if (history_length == 0) {
            this->printf_error("history_length must be > 0");
            return;
        }

        //enable after alloc
        if (history_ring_buffer_.init_alloc(history_length)) {
            history_length_ = history_length;
            history_enabled_ = true;
        } else {

        }

    } else if (history_enabled_) {
        //make sure to disable everything (history and threshold)
        threshold_enabled_ = false;
        threshold_value_ = 0;
        threshold_count_ = 0;
        history_enabled_ = false;
        history_length_ = 0;
        history_ring_buffer_.dealloc_buffer();

        //make sure to dealloc the history copy
        enable_history_copy(false);
    }
}

__attribute__((ramfunc))
uint16_t experimental_input::history_used() volatile const {
    if (history_enabled_) {
        return history_ring_buffer_.in_use();
    } else {
        return 0;
    }
}

__attribute__((ramfunc))
uint16_t experimental_input::history_length() volatile const {
    if (history_enabled_) {
        return history_ring_buffer_.size();
    } else {
        return 0;
    }
}

__attribute__((ramfunc))
bool experimental_input::get_copy_of_history(uint16_t count, json_object_t &json_object) volatile {

    //reset the object
    json_object = blank_json_object;

    if ((history_is_printing_) || (!history_copy_enabled_) || (!history_enabled_) || (history_ring_buffer_.is_empty()) || (count > history_ring_buffer_.in_use()) || (count == 0)) {
        json_object.parameter_name = "error";
        json_object.combined_params.value_type = t_string;
        json_object.value.string_ = "error copying";
        return false;
    }

    //do a quick block copy
    if (!history_ring_buffer_.read(count, history_copy_array_)) {

        //clean up if there is an issue
        json_object.parameter_name = "error";
        json_object.combined_params.value_type = t_string;
        json_object.value.string_ = "error copying";
        return false;
    }

    history_is_printing_ = true;

    //success, set the other properties
    json_object.parameter_name = "history";
    json_object.array_count = count;
    json_object.combined_params.value_type = t_base64;

    if (is_digital_) {
        json_object.combined_params.precision = 1;  //1bit
    } else {
        json_object.combined_params.precision = 16; //16bits (analog)
    }

    json_object.combined_params.free_array_after_print = false;  //do not free, since it needs to be kept
    json_object.combined_params.is_array = true;
    json_object.value.array_ptr.uint16_ = history_copy_array_;  //store the copy

    json_object.is_printing_bool_ptr = const_cast<bool *>(&history_is_printing_);

    return true;
}

// **** setting functions ****

void experimental_input::get_settings(const json_t *const json_root) volatile const {
//need separate outputs for analog and digital
    json_element get_value_e("get_value", t_bool);
    json_element get_threshold_count_e("get_threshold_count", t_bool);

    const uint16_t interrupt_settings = __disable_interrupts();

    if (get_value_e.set_with_json(json_root, false))  {
        this->print_current_value();
    } else if (get_threshold_count_e.set_with_json(json_root, false)) {
        this->print_threshold_count();
    } else {
        this->print_settings(get_r);
    }

    //restore the interrupt state
    __restore_interrupts(interrupt_settings);
}

//stack 358 -> 216
void experimental_input::print_settings(reason_t reason_code) volatile const {

    debug_timestamps.exp_in_print_3 = CPU_TIMESTAMP;

    uint16_t child_count = 29; //digital child_count
    if (!is_digital_) {
        child_count += 2;
    }

    json_object_t print_parent_o = json_parent(const_cast<const char*>(name_), child_count);

    //update all settings
    print_reason_o.value.string_ = get_reason_name(reason_code);
    print_timestamp_o.value.uint64_ = clock_tic_;
    print_enabled_o.value.bool_ = is_enabled_;
    print_digital_o.value.bool_ = is_digital_;
    print_value_o.value.uint16_ = current_value_;
    print_hist_en_o.value.bool_ = history_enabled_;
    print_hist_len_o.value.uint16_ = this->history_length();
    print_all_tran_o.value.bool_ = target_met_msg_on_all_transitions_;
    print_send_c_o.value.bool_ = target_met_msg_to_computer_;
    print_act_en_o.value.bool_ = target_met_actions_enabled_;
    print_act_dis_o.value.bool_ = disable_actions_after_target_met_;
    print_read_en_o.value.bool_ = (readout_every_x_tics_ > 0);
    print_read_x_o.value.uint64_ = readout_every_x_tics_;
    print_timeout_o.value.uint64_ = timeout_length_tics_;
    print_ec_met_o.value.uint16_ = event_codes_.on;
    print_ec_left_o.value.uint16_ = event_codes_.off;
    print_out_en_o.value.bool_ = output_enabled_;
    print_has_out_o.value.bool_ = (output_ptr_ != NULL);
    print_out_dis_o.value.bool_ = disable_output_after_target_met_once_;
    print_out_cyc_o.value.uint16_ = output_cycle_counts_;
    print_out_del_o.value.uint64_ = output_delay_tics_;
    print_has_p_o.value.bool_ = (parent_input_ != NULL);
    print_has_c_o.value.bool_ = (child_input_ != NULL);
    print_tar_set_o.value.bool_ = target_set_;
    print_tar_met_o.value.bool_ = target_met_;
    print_met_min_o.value.uint64_ = target_met_min_length_tics_;
    print_left_min_o.value.uint64_ = target_left_min_length_tics_;
    print_threshold_count_o.value.uint16_ = threshold_count_;
    if (is_digital_) {
        print_target_o.value.uint16_ = static_cast<uint16_t>(target_.digital.polarity);
    } else {
        print_target_typ_o.value.string_ = get_analog_target_type_name(target_.analog.type);
        print_target_val_o.value.uint16_ = target_.analog.value;
        print_target_dist_o.value.uint16_ = target_.analog.distance;
    }

    delayed_json_t delayed_json_object;
    delayed_json_object.object_count = child_count + 1;
    //use heap array, since size is unknown
    delayed_json_object.objects = create_array_of<json_object_t>(child_count + 1, "exp_in_printf_objects");
    if (delayed_json_object.objects == NULL) {
        return;
    }

    //copy each of the objects
    copy_json_object(const_cast<json_object_t *>(&print_parent_o), &(delayed_json_object.objects[0]));
    copy_json_object(const_cast<json_object_t *>(&print_reason_o), &(delayed_json_object.objects[1]));
    copy_json_object(const_cast<json_object_t *>(&print_timestamp_o), &(delayed_json_object.objects[2]));
    copy_json_object(const_cast<json_object_t *>(&print_enabled_o), &(delayed_json_object.objects[3]));
    copy_json_object(const_cast<json_object_t *>(&print_digital_o), &(delayed_json_object.objects[4]));
    copy_json_object(const_cast<json_object_t *>(&print_value_o), &(delayed_json_object.objects[5]));
    copy_json_object(const_cast<json_object_t *>(&print_hist_en_o), &(delayed_json_object.objects[6]));
    copy_json_object(const_cast<json_object_t *>(&print_hist_len_o), &(delayed_json_object.objects[7]));
    copy_json_object(const_cast<json_object_t *>(&print_all_tran_o), &(delayed_json_object.objects[8]));
    copy_json_object(const_cast<json_object_t *>(&print_send_c_o), &(delayed_json_object.objects[9]));
    copy_json_object(const_cast<json_object_t *>(&print_act_en_o), &(delayed_json_object.objects[10]));
    copy_json_object(const_cast<json_object_t *>(&print_act_dis_o), &(delayed_json_object.objects[11]));
    copy_json_object(const_cast<json_object_t *>(&print_read_en_o), &(delayed_json_object.objects[12]));
    copy_json_object(const_cast<json_object_t *>(&print_read_x_o), &(delayed_json_object.objects[13]));
    copy_json_object(const_cast<json_object_t *>(&print_timeout_o), &(delayed_json_object.objects[14]));
    copy_json_object(const_cast<json_object_t *>(&print_ec_met_o), &(delayed_json_object.objects[15]));
    copy_json_object(const_cast<json_object_t *>(&print_ec_left_o), &(delayed_json_object.objects[16]));
    copy_json_object(const_cast<json_object_t *>(&print_out_en_o), &(delayed_json_object.objects[17]));
    copy_json_object(const_cast<json_object_t *>(&print_has_out_o), &(delayed_json_object.objects[18]));
    copy_json_object(const_cast<json_object_t *>(&print_out_dis_o), &(delayed_json_object.objects[19]));
    copy_json_object(const_cast<json_object_t *>(&print_out_cyc_o), &(delayed_json_object.objects[20]));
    copy_json_object(const_cast<json_object_t *>(&print_out_del_o), &(delayed_json_object.objects[21]));
    copy_json_object(const_cast<json_object_t *>(&print_has_p_o), &(delayed_json_object.objects[22]));
    copy_json_object(const_cast<json_object_t *>(&print_has_c_o), &(delayed_json_object.objects[23]));
    copy_json_object(const_cast<json_object_t *>(&print_tar_set_o), &(delayed_json_object.objects[24]));
    copy_json_object(const_cast<json_object_t *>(&print_tar_met_o), &(delayed_json_object.objects[25]));
    copy_json_object(const_cast<json_object_t *>(&print_met_min_o), &(delayed_json_object.objects[26]));
    copy_json_object(const_cast<json_object_t *>(&print_left_min_o), &(delayed_json_object.objects[27]));
    copy_json_object(const_cast<json_object_t *>(&print_threshold_count_o), &(delayed_json_object.objects[28]));

    //finally, the unique target options
    if (is_digital_) {
        copy_json_object(const_cast<json_object_t *>(&print_target_o), &(delayed_json_object.objects[29]));
    } else {
        copy_json_object(const_cast<json_object_t *>(&print_target_typ_o), &(delayed_json_object.objects[29]));
        copy_json_object(const_cast<json_object_t *>(&print_target_val_o), &(delayed_json_object.objects[30]));
        copy_json_object(const_cast<json_object_t *>(&print_target_dist_o), &(delayed_json_object.objects[31]));
    }

    delay_printf_json_objects(delayed_json_object);

    debug_timestamps.exp_in_print_4 = CPU_TIMESTAMP;
}

const char *experimental_input::get_analog_target_type_name(analog_target_type_t target_type) {
    const char *ptr;

    switch (target_type) {
        case less_than:
            ptr = "<";
            break;
        case less_than_or_equal_to:
            ptr = "<=";
            break;
        case greater_than:
            ptr = ">";
            break;
        case greater_than_or_equal_to:
            ptr = ">=";
            break;
        case rectangular_distance:
            ptr = "rectangular_distance";
            break;
        case circular_distance:
            ptr = "circular_distance";
            break;
        case elliptical_distance:
            ptr = "elliptical_distance";
            break;
        default:
            ptr = "error";
            break;
    }

    return ptr;
}

void experimental_input::set_actions(const json_t *const json_root) volatile const{
    json_element number_e("input_number", t_uint16, true);
    if ((!number_e.set_with_json(json_root, true)) || (number_e.value().uint16_ != number_)) {
        this->printf_error("input_number is wrong");
        return;
    }

    //only after setting the elements
    //disable and store the interrupt state
    const uint16_t interrupt_settings = __disable_interrupts();

    this->printf_error("to be written");

    //restore the interrupt state
    __restore_interrupts(interrupt_settings);
}

void experimental_input::set_settings(const json_t *const json_root) volatile {
    json_element number_e("input_number", t_uint16, true);
    if ((!number_e.set_with_json(json_root, true)) || (number_e.value().uint16_ != number_)) {
        this->printf_error("input_number is wrong");
        return;
    }


    //down from 358
    //stack 50 w/o
    //stack 152 w/ this
    const uint16_t found_count = set_elements_with_json(json_root, 29,
                           &number_e, &history_enabled_e, &history_length_e, &enable_readout_e,
                           &readout_length_e, &met_min_tics_e, &digital_target_e, &analog_type_e,
                           &analog_value_e, &analog_distance_e, &actions_enable_e, &disable_actions_after_e,
                           &all_transitions_e, &send_to_computer_e, &event_code_met_e, &event_code_left_e,
                           &enable_child_e, &child_number_e, &enable_output_e, &output_number_e,
                           &output_disable_after_e, &output_cycles_e, &timeout_e, &left_min_tics_e,
                           &output_delay_tics_e, &settings_e, &reset_target_e, &threshold_enabled_e,
                           &threshold_value_e);

    //if none found, return
    if (found_count == 0) {
        return;
    }

    //only after setting the elements
    //disable and store the interrupt state
    const uint16_t interrupt_settings = __disable_interrupts();

    debug_timestamps.exp_input_set_settings.tic();

    //history
    if (history_enabled_e.count_found() > 0) {
        if (!history_enabled_e.value().bool_) {
            this->enable_history(false, 0);
        } else {
            if (history_length_e.count_found() > 0) {
                this->enable_history(true, history_length_e.value().uint16_);
            } else {
                this->printf_error("'history_length' is required to enable history");
            }
        }
    }

    //readout
    if (enable_readout_e.count_found() > 0) {
        if (!enable_readout_e.value().bool_) {
            this->enable_readout(false, 0);
        } else {
            if (readout_length_e.count_found() > 0) {
                this->enable_readout(true, readout_length_e.value().uint16_);
            } else {
                this->printf_error("'readout_tics' is required to enable readout");
            }
        }
    }

    //target
    if (met_min_tics_e.count_found() > 0) {
        target_met_min_length_tics_ = met_min_tics_e.value().uint64_;
    }

    if (left_min_tics_e.count_found() > 0) {
        target_left_min_length_tics_ = left_min_tics_e.value().uint64_;
    }

    if (is_digital_) {
        //if digital
        if (digital_target_e.count_found() > 0) {
            target_.digital.polarity = (digital_target_e.value().uint16_ != 0);
            target_set_ = true;
        }

    } else {
        //if analog
        if ((analog_type_e.count_found() > 0) || (analog_distance_e.count_found() > 0) || (analog_value_e.count_found() > 0)) {
            bool had_error = false;

            if (analog_type_e.count_found() == 0) {
                this->printf_error("target_type is required");
                had_error = true;
            }
            const char *const analog_type_string = analog_type_e.value().string_;
            analog_target_type_t temp_analog_type = error_target;

            if (strcmp(analog_type_string, get_analog_target_type_name(less_than)) == 0) {
                temp_analog_type = less_than;
            } else if (strcmp(analog_type_string, get_analog_target_type_name(less_than_or_equal_to)) == 0) {
                temp_analog_type = less_than_or_equal_to;
            } else if (strcmp(analog_type_string, get_analog_target_type_name(greater_than)) == 0) {
                temp_analog_type = greater_than;
            } else if (strcmp(analog_type_string, get_analog_target_type_name(greater_than_or_equal_to)) == 0) {
                temp_analog_type = greater_than_or_equal_to;
            } else if (strcmp(analog_type_string, get_analog_target_type_name(rectangular_distance)) == 0) {
                temp_analog_type = rectangular_distance;
            } else if (strcmp(analog_type_string, get_analog_target_type_name(circular_distance)) == 0) {
                temp_analog_type = circular_distance;
            } else if (strcmp(analog_type_string, get_analog_target_type_name(elliptical_distance)) == 0) {
                    temp_analog_type = elliptical_distance;
            } else {
                this->printf_error("target_type must be one of the following: '<', '<=', '>', '>=', 'rectangular_distance', 'circular_distance', 'elliptical_distance'");
                had_error = true;
            }

            if (analog_value_e.count_found() == 0) {
                this->printf_error("target_value is required");
                had_error = true;
            }

            if (((temp_analog_type == rectangular_distance) || (temp_analog_type == circular_distance) || (temp_analog_type == elliptical_distance)) && (analog_distance_e.count_found() == 0)) {
                this->printf_error("for target_type of '_distance', a target_distance must be supplied");
                had_error = true;
            }

            if (!had_error) {
                target_.analog.type = temp_analog_type;
                target_.analog.value = analog_value_e.value().uint16_;
                if (analog_distance_e.count_found() > 0) {
                    target_.analog.distance = analog_distance_e.value().uint16_;
                } else {
                    target_.analog.distance = 0;
                }
                target_set_ = true;
            }
        }
    }

    if ((reset_target_e.count_found() > 0) && (reset_target_e.value().bool_)) {
        target_met_ = false;
        target_conditionally_met_tic_ = 0;
    }

    //actions
    if (actions_enable_e.count_found() > 0) {
        target_met_actions_enabled_ = actions_enable_e.value().bool_;
    }
    if (disable_actions_after_e.count_found() > 0) {
        disable_actions_after_target_met_ = disable_actions_after_e.value().bool_;
    }

    //messages
    if (all_transitions_e.count_found() > 0) {
        target_met_msg_on_all_transitions_ = all_transitions_e.value().bool_;
    }
    if (send_to_computer_e.count_found() > 0) {
        target_met_msg_to_computer_ = send_to_computer_e.value().bool_;
    }
    if (event_code_met_e.count_found() > 0) {
        if ((event_code_met_e.value().uint16_ > 127) && (event_code_met_e.value().uint16_ < 256)) {
            event_codes_.on = event_code_met_e.value().uint16_;
        } else {
            this->printf_error("event code outside of range 128-255");
        }
    }
    if (event_code_left_e.count_found() > 0) {
        if ((event_code_left_e.value().uint16_ > 127) && (event_code_left_e.value().uint16_ < 256)) {
            event_codes_.off = event_code_left_e.value().uint16_;
        } else {
            this->printf_error("event code outside of range 128-255");
        }
    }

    //child
    if ((enable_child_e.count_found() > 0) && (child_number_e.count_found() > 0)) {
        this->enable_child(enable_child_e.value().bool_, child_number_e.value().uint16_);
    }
    if (enable_output_e.count_found() > 0) {
        output_enabled_ = enable_output_e.value().bool_;
    }
    if (output_number_e.count_found() > 0) {
        if (output_number_e.value().uint16_ >= get_io_output_count()) {
            this->printf_error("'output_number' is too high");
            output_ptr_ = NULL;
            return;
        } else {
            output_ptr_ = const_cast<volatile experimental_output *>(&(outputs_array_[output_number_e.value().uint16_]));
        }
    }
    if (output_disable_after_e.count_found() > 0) {
        disable_output_after_target_met_once_ = output_disable_after_e.value().bool_;
    }
    if (output_cycles_e.count_found() > 0) {
        if (output_cycles_e.value().uint16_ > 0) {
            output_cycle_counts_ = output_cycles_e.value().uint16_;
        } else {
            this->printf_error("'output_cycles' cannot be 0");
        }

    }
    if (output_delay_tics_e.count_found() > 0) {
        output_delay_tics_ = output_delay_tics_e.value().uint64_;
    }

    //timeout
    if (timeout_e.count_found() > 0) {
        //this->set_timeout(timeout_e.value().uint64_);
        timeout_length_tics_ = timeout_e.value().uint64_;
        is_in_timeout_ = false;
    }

    //threshold
    if (threshold_enabled_e.count_found() > 0) {
        if (!threshold_enabled_e.value().bool_) {
            this->enable_threshold(false);
        } else {
            if (threshold_value_e.count_found() > 0) {
                this->enable_threshold(true, threshold_value_e.value().uint16_);
            } else {
                this->printf_error("'threshold_value' is required to enable threshold");
            }
        }
    }

    debug_timestamps.exp_in_print_1 = CPU_TIMESTAMP;
    if ((settings_e.count_found() > 0) && (settings_e.value().bool_)) {
        debug_timestamps.exp_in_print_2 = CPU_TIMESTAMP;
        this->print_settings(set_r);
        debug_timestamps.exp_in_print_5 = CPU_TIMESTAMP;
    }

    debug_timestamps.exp_input_set_settings.toc();

    //restore the interrupt state
    __restore_interrupts(interrupt_settings);
}
