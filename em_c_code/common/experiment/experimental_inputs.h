/*
 * experimental_inputs.h
 *
 *  Created on: Oct 10, 2017
 *      Author: adam jones
 */


#ifndef experiment_inputs_defined
#define experiment_inputs_defined

#include <stdint.h>
#include <stdbool.h>
#include "tiny_json.h"
#include "F28x_Project.h"
#include "experimental_outputs.h"
#include "io_controller.h"
#include "extract_json.h"


typedef struct digital_target_t {
    bool polarity;  //0=off, 1=on
} digital_target_t;

typedef enum {
    less_than,                  //independent for each
    less_than_or_equal_to,      //independent for each
    greater_than,               //independent for each
    greater_than_or_equal_to,   //independent for each
    rectangular_distance,       //independent for each, with distance
    circular_distance,          //dependent, uses distance of parent
    elliptical_distance,        //independent for each, with distance
    error_target                //just an error
} analog_target_type_t;

typedef struct analog_target_t {
    analog_target_type_t type;
    uint16_t value;

    //distance
    uint16_t distance;  //distance to value (for near)
} analog_target_t;


#include "ring_buffer.h"
#include "printf_json_types.h"

typedef uint16_t (*input_get_function) (uint16_t channel);

class experimental_input {
    public:
        //init
        void init(uint16_t number, uint16_t channel, bool is_digital, const volatile input_get_function get_function, const volatile experimental_input *const inputs_array, const volatile experimental_output *const outputs_array) volatile;

        //process
        void update_current_value(uint64_t experiment_tic) volatile;
        void process_actions() volatile;    //must already have experiment_tic set

        //misc
        void print_current_value() volatile const;
        void print_threshold_count() volatile const;
        uint16_t get_current_value() volatile const {return current_value_;}
        uint16_t get_channel() volatile const {return channel_;}
        uint16_t get_number() volatile const {return number_;}
        const char *get_name() volatile const {return const_cast<char*>(name_);}
        bool is_digital() volatile const {return is_digital_;}

        //history
        void enable_history(bool enable, uint16_t history_length = 0) volatile;
        void enable_history_copy(bool enable) volatile;
        bool is_history_enabled() volatile const {return history_enabled_;}
        uint16_t history_used() volatile const;
        uint16_t history_length() volatile const;
        bool get_copy_of_history(uint16_t count, json_object_t &json_object) volatile;
        bool is_history_being_printed() volatile const {return history_is_printing_;}

        //other setting functions
        void enable_threshold(bool enable, uint16_t threshold_value = 0) volatile;
        void enable_readout(bool enable, uint64_t readout_every_x_tics) volatile;
        void enable_child(bool enable, uint16_t number) volatile;

        //functions for overall settings
        void set_settings(const json_t *const json_root) volatile;
        void get_settings(const json_t *const json_root) volatile const;
        void set_actions(const json_t *const json_root) volatile const;
        static const char *get_analog_target_type_name(analog_target_type_t target_type);
        void print_settings(reason_t reason_code) volatile const;

    private:
        //basic information
        uint16_t number_;
        bool is_digital_;
        bool is_enabled_;
        uint16_t current_value_; //can fit both digital and analog

        //history
        bool history_enabled_;
        uint16_t history_length_;
        bool history_copy_enabled_;

        //get function
        uint16_t channel_; //raw io channel

        //target has been met
        bool target_set_;
        bool target_met_;      //meets target conditions
        bool target_met_msg_on_all_transitions_;    //send all transitions
        bool target_met_msg_to_computer_;

        //actions enabled
        bool target_met_actions_enabled_;           //so it can be set without being enabled
        bool disable_actions_after_target_met_;   //so it can be a one-shot input

        //timeout
        bool is_in_timeout_;     //timeout is enabled

        //output
        bool output_enabled_;    //for a reward, or something else
        bool disable_output_after_target_met_once_;
        uint16_t output_cycle_counts_; //add this many "events" to the output queue

        //larger properties packed at end
        io_event_codes_t event_codes_;
        uint64_t readout_every_x_tics_; //modulo with experimental_clock
        uint64_t target_met_min_length_tics_;   //minimum length to be considered valid as met
        uint64_t target_conditionally_met_tic_;
        uint64_t target_left_min_length_tics_;   //minimum length to be considered valid as left
        uint64_t target_conditionally_left_tic_;
        uint64_t timeout_start_tic_;     //reset each time the target is met again
        uint64_t timeout_length_tics_;
        uint64_t output_delay_tics_;

        //target conditions
        union {
            digital_target_t digital;
            analog_target_t  analog;
        } target_;

        ring_buffer history_ring_buffer_;
        bool history_is_printing_;  //true when printing, false once done
        uint16_t *history_copy_array_;

        //threshold tracking (currently just < value)
        bool threshold_enabled_;
        uint16_t threshold_value_;
        uint16_t threshold_count_;

        input_get_function get_function_;
        const volatile experimental_input *inputs_array_;  //might be useful to point to all
        const volatile experimental_output *outputs_array_;  //might be useful to point to all
        volatile experimental_input *parent_input_;
        volatile experimental_input *child_input_;
        volatile experimental_output *output_ptr_;
        uint64_t clock_tic_;
        char name_[9]; //limits the max number to 99

        //private functions
        const volatile experimental_input *highest_primary() volatile const;
        void do_target_actions(bool target_met) volatile;
        bool does_value_meet_this_target() volatile const;
        bool does_value_meet_all_targets() volatile const;
        void send_target_met_message(bool target_met, bool output_queued) volatile const;
        void printf_status(const char *const message) volatile const;
        void printf_error(const char *const message) volatile const;

};

#endif
