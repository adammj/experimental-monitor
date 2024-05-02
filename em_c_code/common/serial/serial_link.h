/*
 * serial_link.h
 *
 *  Created on: Mar 5, 2017
 *      Author: adam jones
 */

#ifndef serial_link_defined
#define serial_link_defined

#include "stdbool.h"
#include "stdint.h"
#include "tiny_json.h"


typedef void (*serial_command_func_json_t) (const json_t *const json_root);
typedef void (*serial_command_func_void_t) (void);

typedef struct serial_command_t {
    const char *command_string;
    bool is_visible;
    uint32_t command_hash;
    serial_command_func_json_t json_function;
    serial_command_func_void_t void_function;
    serial_command_t *next_function;
} serial_command_t;

void init_serial_link();

//main function
void process_scia_buffer_for_commands();

//others
void print_connection_success();
void print_all_serial_commands(const json_t *const json_root);
void print_serial_configuration();

void print_all_siblings(const json_t *json_cmd_object);

void check_if_waiting_for_input();

void add_serial_command(serial_command_t *new_serial_command);


#endif
