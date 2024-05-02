/*
 * json_misc.h
 *
 *  Created on: May 17, 2017
 *      Author: adam jones
 */


#ifndef json_immediate_defined
#define json_immediate_defined

#include "stdbool.h"
#include "stdint.h"
#include "printf_json_types.h"


//immediately print json objects
void printf_json_objects(uint16_t object_count, ...);

//helper functions with pre-defined json_string
void printf_json_error(const char *error_string);
void printf_json_status(const char *text_string);

//malloc stuff
uint16_t array_size_multiplier_for_value_type(value_type_t value_type);
void delete_array_for_value_type(value_type_t value_type, const array_ptr_union_t array_ptr);

#endif
