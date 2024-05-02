/*
 * em_main.c
 *
 *  Created on: May 18, 2017
 *      Author: adam jones
 */

#include "em_main.h"
#include "ti_launchpad.h"
#include "daughterboard.h"
#include "printf_json.h"
#include "printf_json_delayed.h"
#include "cpu_timers.h"
#include "tic_toc.h"
#include "misc.h"
#include "io_controller.h"
#include "serial_link.h"
#include "scia.h"
#include "math.h"
#include "arrays.h"


//internal functions
void start_experimental_monitor_main_loop();

//main function, called first
void main() {
    // startup the ti launchpad **** MUST BE FIRST, TO COPY ramfunc ****
    if (!ti_board.startup()) {
        lockup_cpu(); //if ti_board did not start up
    }

    //init the printf queue (to catch all delayed prints that might occur (except those in ti))
    if (!init_delayed_json_object_queue()) {
        lockup_cpu();   //must have the delay json queue
    }

    //enable the daughterboard (if found)
    if (!db_board.startup()) {
        lockup_cpu(); //just lock up if db_board did not start up
    }

    //set the json timestamp
    set_json_timestamp_freq(ti_board.main_frequency);

    //finally, initialize the io_controller (should check db is ready and get clock)
    init_io_controller();

    //print ready, and wait for buffer to empty
    init_serial_link(); //serial link can be initialized

    //print everything delayed first
    printf_delayed_json_objects(true);

    //final print before ready
    wait_until_scia_tx_buffer_is_empty();

    //try to wait an extra second before done
    printf_json_status("ready");
    twiddle_leds_ten_times();

    printf_delayed_json_objects(true);

    //and run the time-insensitive main loop
    start_experimental_monitor_main_loop();
}


//normal loop, can have anything
__attribute__((ramfunc))
void start_experimental_monitor_main_loop() {

    //status led counts
    const uint64_t one_sec_count = static_cast<uint64_t>(llroundl(cpu_timer0.freq_hz()));
    const uint64_t on_duration_count = (25*one_sec_count)/1000;
    uint64_t next_on_count = one_sec_count;
    uint64_t next_off_count = next_on_count + on_duration_count;

    //start the io controller timer
    start_io_controller();

    //main loop, which will be interrupted
    while (true) {

        debug_timestamps.main_b_scia = CPU_TIMESTAMP;

        process_scia_buffer_for_commands();

        debug_timestamps.main_b_printf_delayed = CPU_TIMESTAMP;

        //just print one through each loop
        printf_delayed_json_objects(false);

        debug_timestamps.main_a_printf_delayed = CPU_TIMESTAMP;


        //status led code off
        const uint64_t current_count = cpu_timer0.count();
        if (ti_board.red_led.is_on() && (current_count >= next_off_count)) {
            //only blink status led if not twiddling
            if (!is_currently_twiddling_leds()) {
                ti_board.red_led.off();
            }
        }

        //once-per-sec routines
        if ((!ti_board.red_led.is_on()) && (current_count >= next_on_count)) {
            const uint64_t current_sec_count = static_cast<uint64_t>(current_count / one_sec_count) * one_sec_count;
            next_on_count = current_sec_count + one_sec_count;
            next_off_count = current_sec_count + on_duration_count;

            //only blink status led if not twiddling
            if (!is_currently_twiddling_leds()) {
                ti_board.red_led.on();
            }

            //other updates to happen once per sec
            check_if_waiting_for_input();
        }
    }
}
