/*
 * ti_launchpad.h
 *
 *  Created on: Oct 10, 2017
 *      Author: adam jones
 */
//settings for both 77S and 79D, regardless of daughterboard


#ifndef ti_settings_defined
#define ti_settings_defined


#if defined(_LAUNCHXL_F28377S) || defined(_LAUNCHXL_F28379D)
#else
    #error error, code written expecting 77S or 79D
#endif


#include <stdint.h>
#include <stdbool.h>
#include "leds.h"
#include "F28x_Project.h"
#include "sci.h"
#include "tiny_json.h"


class ti_launchpad {
    public:
        //basic
        ti_launchpad();
        bool startup(); //must be run after constructing
        static void reboot();

        //operators
        static void enable_pullups(uint16_t count, const uint16_t *const gpio_array);
        void print_configuration() const;
        static void enable_watchdog();
        static void disable_watchdog();
        static uint16_t kick_watchdog();
        uint16_t get_board_type() const;

        //clocks
        float64 get_clock_scale() const {return clock_scale_;}
        void set_clock_deviation(int32_t ppb_deviation, bool print_message);
        float64 get_peripheral_clock_freq() const {return (static_cast<float64>(clock_freq_)*clock_scale_)/static_cast<float64>(peripheral_clock_divider_);}
        float64 get_main_clock_freq() const {return static_cast<float64>(clock_freq_)*clock_scale_;}
        uint32_t get_unscaled_clock_freq() const {return clock_freq_;}

        //leds
        ti_led red_led;
        ti_led blue_led;

        //scia
        sci_gpio_t get_scia_gpio() const;

        //main loop (not specifically ti, but should go somewhere constant)
        static const uint32_t main_frequency = 1000;   //1kHz

        static uint32_t get_ti_uid() {return *reinterpret_cast<uint32_t *>(0x703C0);} //for 7xD and for 7xS: 0x000703C0 inside OTP


    private:
        //initialized
        bool initialized_;

        //clock
        uint32_t clock_freq_;   //32bit covers 200MHz
        int32_t clock_ppb_deviation_;
        float64 clock_scale_;   //must be 64bit for the precision required
        uint16_t peripheral_clock_divider_; //is fixed to 1 for now
        uint32_t xtal_osc_freq_;

        //board and chip
        uint16_t board_type_; //77 or 79 (simple for now)
        const char *version_str_;
        const char *launchpad_label_;
        const char *launchpad_rev_;
        uint32_t board_id_;
        uint16_t chip_part_number_;
        uint16_t chip_family_;

        //leds
        uint16_t red_led_gpio;
        uint16_t blue_led_gpio;

        //scia
        sci_gpio_t scia_gpio_;

        //functions
        void verify_expected_chip() const;
        void set_peripheral_clock_divider() const;
        void init_sys_control() const;
        void update_specific_board_settings();
        void disable_all_peripheral_clocks() const;
        void enable_global_interrupts() const;
};


//pointer to singleton, held here
extern ti_launchpad ti_board;

void print_clock_scale();
void print_ti_configuration();
void ti_reboot();


#endif
