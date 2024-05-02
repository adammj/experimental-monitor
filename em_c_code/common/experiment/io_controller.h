/*
 * io_controller.h
 *
 *  Created on: Jan 5, 2018
 *      Author: adam jones
 */


#ifndef io_controller_defined
#define io_controller_defined


#include <stdint.h>
#include <stdbool.h>
#include "F28x_Project.h"
#include "tiny_json.h"


typedef struct io_event_codes_t {
    uint16_t on     : 8;    //8bits is enough
    uint16_t off    : 8;    //8bits is enough
} io_event_codes_t;

typedef enum {
    get_r,
    set_r,
    ext_r
} reason_t;

//init and start
bool init_io_controller();
void start_io_controller();

//reset experiment count
void reset_experiment_clock(const json_t *const json_root);
void get_experiment_timestamp();

//get uptime information
float64 get_uptime_seconds();
void print_uptime();

uint16_t get_io_input_count();
uint16_t get_io_output_count();

void set_status_messages(const json_t *const json_root);

//twiddle led stuff (shouldn't be in ti_board)
void process_led_twiddles();
void twiddle_leds_ten_times();
bool is_currently_twiddling_leds();

// **** serial setting functions ****
void get_input_history(const json_t *const json_root);
void set_input_settings(const json_t *const json_root);
void set_output_settings(const json_t *const json_root);
void get_input_settings(const json_t *const json_root);
void get_output_settings(const json_t *const json_root);
void set_input_actions(const json_t *const json_root);
void set_output_actions(const json_t *const json_root);

const char *get_reason_name(reason_t reason_i);


#endif
