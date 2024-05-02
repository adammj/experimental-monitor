/*
 * experiment_outputs.h
 *
 *  Created on: Oct 10, 2017
 *      Author: adam jones
 */


#ifndef experiment_outputs_defined
#define experiment_outputs_defined

#include <stdint.h>
#include <stdbool.h>
#include "F28x_Project.h"
#include "tiny_json.h"
#include "io_controller.h"


typedef void (*output_set_function) (uint16_t channel, uint16_t value);

class experimental_output {
    public:
        //init
        void init(uint16_t number, uint16_t channel, const volatile output_set_function set_function) volatile;


        void enable(bool enable) volatile {is_enabled_ = enable;}

        void print_current_value() volatile const;

        void set_on_off_values(uint16_t on_value, uint16_t off_value);

        uint16_t get_current_value() const {return current_value_;}
        void set_on_off_tics(uint64_t on_tics, uint64_t off_tics);
        void set_continuous(bool is_continuous) {is_continuous_ = is_continuous;}
        void set_event_codes(const io_event_codes_t dsp_codes) volatile;
        void process_actions(uint64_t experiment_tic) volatile;
        bool is_continuous() volatile const {return is_continuous_;}

        //void trigger() {cycle_count_++;}
        void add_cycles(uint16_t add_cycles, uint64_t start_tic = 0) volatile;

        //name
        const char *name() const {return const_cast<char*>(name_);}

        //settings/actions
        void set_actions(const json_t *const json_root) volatile;
        void set_settings(const json_t *const json_root) volatile;
        void get_settings(const json_t *const json_root) volatile const;
        void print_settings(reason_t reason_code) volatile const;

    private:
        //basic information
        uint16_t number_;
        bool is_enabled_;
        uint16_t current_value_;
        uint16_t on_value_;
        uint16_t off_value_;
        bool is_digital_;   //always true right now

        //set function
        uint16_t channel_; //raw io channel

        //output
        bool is_continuous_;    //so a constant on/off can be set (like a sync signal)
        uint16_t cycle_count_;
        bool is_in_a_cycle_now_;
        bool output_start_cycle_msg_to_computer_;
        bool all_transitions_;

        //larger properties packed at end
        output_set_function set_function_;
        uint64_t length_on_tics_;
        uint64_t length_off_tics_;
        uint64_t output_cycle_off_tic_;
        uint64_t output_cycle_finished_tic_;
        uint64_t clock_tic_;
        uint64_t start_delay_tic_;
        io_event_codes_t event_codes_;
        char name_[10]; //limits the max number to 99

        void set_current_value(uint16_t new_value, bool send_codes_and_messages = true) volatile;
        void printf_error(const char *const message) volatile const;
};


#endif
