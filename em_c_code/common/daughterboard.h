/*
 * daughterboard.h
 *
 *  Created on: Apr 20, 2017
 *      Author: adam jones
 */
// File that just summarizes all of the GPIO, timing, and cla settings


#ifndef daughterboard_defined
#define daughterboard_defined

#include <stdint.h>
#include <stdbool.h>
#include "analog_input.h"
#include "dsp_output.h"
#include "digital_io.h"


class daughterboard {
    public:
        daughterboard();
        static int16_t get_version();
        bool startup();
        void print_configuration() const;

        uint16_t digital_in_count() const {return digital_io_settings_.digital_in_count;}
        uint16_t digital_out_count() const {return digital_io_settings_.digital_out_count;}
        uint16_t analog_in_count() const {return analog_in_settings_.analog_in_count;}
        bool is_enabled() const {return is_enabled_;}
        bool is_not_enabled() const;

        digital_io_settings_t get_digital_io_settings_t() const {return digital_io_settings_;}


    private:
        bool is_enabled_;
        const char *version_str_;
        uint16_t version_num_;

        //digital io
        digital_io_settings_t digital_io_settings_;

        //analog in
        analog_in_settings_t analog_in_settings_;

        //pullup
        uint16_t pullup_count_;
        uint16_t *pullup_gpio_;

        //dsp
        dsp_settings_t dsp_settings_;

        int16_t get_db_board_setting_id(uint16_t db_version) const;
};

//pointer to singleton created in main
extern daughterboard db_board;

void print_db_configuration();

#endif
