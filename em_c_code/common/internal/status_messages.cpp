/*
 * em_main.c
 *
 *  Created on: Oct 13, 2017
 *      Author: adam jones
 */

#include "status_messages.h"
#include "io_controller.h"
#include "F28x_Project.h"
#include "digital_io.h"
#include "analog_input.h"
#include "printf_json_delayed.h"
#include "scia.h"
#include "extract_json.h"
#include "misc.h"
#include "cpu_timers.h"


__attribute__((ramfunc))
void print_uptime_only() {
    delay_printf_json_objects(1, json_float64("uptime", get_uptime_seconds(), 4));
}

__attribute__((ramfunc))
void print_full_status_message() {

    //uptime
    delay_printf_json_objects(1, json_float64("uptime", get_uptime_seconds(), 4));

    //print out the cycles
    float status_tic_toc_in_us[3];
    status_tic_toc_in_us[0] = cpu_timer0.cycle_us(); //get_time_sensitive_cycle_count_us();
    status_tic_toc_in_us[1] = cpu_timer0.cycle_max_us(); //get_time_sensitive_cycle_count_max_us();
    status_tic_toc_in_us[2] = analog_in_cla_task_length_in_us();
    delay_printf_json_objects(1, json_float32_array("tic_toc main_current, main_max, analog_in", 3, status_tic_toc_in_us, true, 2));

    //print current values
    printf_digital_in_values();
    printf_analog_in_values();
    printf_digital_out_values();
}
