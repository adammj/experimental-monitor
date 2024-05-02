/*
 *  printf_json_delayed.h
 *
 *  Created on: May 17, 2017
 *      Author: adam jones
 */


#ifndef json_delayed_defined
#define json_delayed_defined

#include "stdbool.h"
#include "stdint.h"
#include "printf_json_types.h"


typedef struct delayed_json_t {
    uint16_t object_count;
    json_object_t *objects;
} delayed_json_t;


//delayed init and actual print
bool init_delayed_json_object_queue();
void printf_delayed_json_objects(bool print_all = false);

//delay print json objects
void delay_printf_json_objects(uint16_t object_count, ...);
void delay_printf_json_objects(const delayed_json_t delayed_json_object);

//helper functions with pre-defined json_string
void delay_printf_json_error(const char *const error_string);
void delay_printf_json_status(const char *const text_string);


#endif
