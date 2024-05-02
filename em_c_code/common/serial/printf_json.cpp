/*
 * printf_json.cpp
 *
 *  Created on: May 17, 2017
 *      Author: adam jones
 */


#include <printf_json.h>
#include <printf_json_delayed.h>
#include <stdint.h>
#include <stdbool.h>
#include "math.h"
#include "stdarg.h"
#include "scia.h"
#include "ring_buffer.h"
#include "fpu_vector.h"
#include "misc.h"
#include "printf_raw.h"
#include "tic_toc.h"
#include "arrays.h"




//internal variables
//error checks
static volatile bool has_had_print_error = false;
static volatile bool has_had_fatal_print_error = false;

//delayed queue size
static const uint16_t delayed_json_object_queue_length = 200;

//queue objects need to be volatile since they can be added at any time
static volatile bool has_init_delayed_queue = false;
static volatile delayed_json_t *delayed_json_objects;
static volatile ring_buffer delayed_json_object_queue;

//timestamp constants
static volatile uint64_t ts_multiplier = 1;
static volatile uint16_t ts_shift = 0;

//internal functions
void internal_printf_json_object(const json_object_t &json_object);
void internal_printf_value(value_type_t value_type, const void *const value_ptr, uint16_t index, uint16_t precision_or_sig_figs);
void internal_printf_array(value_type_t value_type, uint16_t array_count, const array_ptr_union_t values_ptr, uint16_t precision_or_sig_figs);
json_object_t create_blank_json_object();

//global variable
const json_object_t blank_json_object = create_blank_json_object();


__attribute__((ramfunc))
void internal_inner_print_json_objects(const json_object_t &json_object, uint16_t object_i, uint16_t object_count, uint16_t &object_child_count) {
    if (object_i == 0) {
        //open the JSON string
        printf_char('{');
    }

    const uint16_t final_object_i = (object_count - 1);

    if (json_object.combined_params.value_type != t_parent) {
        //default is not an object, print
        internal_printf_json_object(json_object);

        if (object_child_count == 0) {
            //print a comma, if not the last object
            if (object_i != final_object_i) {
                printf_char(',');
            }
        }

    } else {
        //print the string, and open bracket
        printf_char('"');
        printf_string(json_object.parameter_name);  //this is for a parent
        printf_string("\":");
        printf_char('{');
        object_child_count = json_object.array_count;
    }

    //if not an object, but the child > 0, then
    if ((object_child_count > 0) && (json_object.combined_params.value_type != t_parent)) {
        object_child_count--;

        //if the last child, or no more objects, then close bracket
        if ((object_child_count == 0) || (object_i == final_object_i)) {
            printf_char('}');
        }

        if (object_i != final_object_i) {
            printf_char(',');
        }
    }

    if (object_i == final_object_i) {
        //close the JSON string and return
        printf_char('}');
        printf_return();
    }

    //set the callback bool pointer, if defined
    if (json_object.is_printing_bool_ptr != NULL) {
        *(json_object.is_printing_bool_ptr) = false;
    }
}

__attribute__((ramfunc))
void printf_json_objects(uint16_t object_count, ...) {

    va_list json_objects;
    uint16_t object_child_count = 0;

    va_start(json_objects, object_count);

    //loop through each object
    for (uint16_t object_i = 0; object_i<object_count; object_i++) {
        const json_object_t json_object = va_arg(json_objects, json_object_t);

        internal_inner_print_json_objects(json_object, object_i, object_count, object_child_count);
    }

    va_end(json_objects);
}


__attribute__((ramfunc))
void printf_delayed_json_objects(bool print_all) {
    if (!has_init_delayed_queue) {return;}

    debug_timestamps.printf_s_print = CPU_TIMESTAMP;

    //if there is an object in the queue, print it (only one per each call)
    while (!delayed_json_object_queue.is_empty()) {
        debug_timestamps.printf_s_loop = CPU_TIMESTAMP;

        debug_timestamps.printf_b_read_i = CPU_TIMESTAMP;

        //grab the next object and copy locally
        uint16_t queue_i;
        if (!delayed_json_object_queue.read(queue_i)) {
            lockup_cpu();
        }

        debug_timestamps.printf_a_read_i = CPU_TIMESTAMP;

        //first check that the object is valid
        if ((delayed_json_objects[queue_i].object_count > 0) && (delayed_json_objects[queue_i].objects != NULL)) {

            debug_timestamps.printf_is_valid = CPU_TIMESTAMP;

            //get the count
            const uint16_t object_count = delayed_json_objects[queue_i].object_count;

            debug_timestamps.printf_a_count = CPU_TIMESTAMP;

            //reset the child count
            uint16_t object_child_count = 0;

            debug_timestamps.printf_b_obj_loop = CPU_TIMESTAMP;

            //loop through each object
            for (uint16_t object_i = 0; object_i<object_count; object_i++) {

                debug_timestamps.printf_s_obj_loop = CPU_TIMESTAMP;

                //copy locally
                json_object_t json_object;
                copy_json_object(const_cast<json_object_t *>(&(delayed_json_objects[queue_i].objects[object_i])), &json_object);

                debug_timestamps.printf_a_copy = CPU_TIMESTAMP;

                internal_inner_print_json_objects(json_object, object_i, object_count, object_child_count);

                debug_timestamps.printf_a_print = CPU_TIMESTAMP;
            }

            debug_timestamps.printf_b_delete = CPU_TIMESTAMP;

            //delete the objects
            delete_array(delayed_json_objects[queue_i].objects);
            delayed_json_objects[queue_i].objects = NULL;

            //reset count to zero
            delayed_json_objects[queue_i].object_count = 0;

            debug_timestamps.printf_a_delete = CPU_TIMESTAMP;

        }

        //if not printing all, then exit after 1
        if (!print_all) {
            break;
        }
    }
}

bool init_delayed_json_object_queue() {
    if (has_init_delayed_queue) {return true;}

    //alloc both the object array and the ring buffer
    delayed_json_objects = create_array_of<delayed_json_t>(delayed_json_object_queue_length, "delayed_json_objects");
    if (delayed_json_objects == NULL) {
        lockup_cpu();
        return false;
    }

    if (!delayed_json_object_queue.init_alloc(delayed_json_object_queue_length)) {
        lockup_cpu();
        return false;
    }

    has_init_delayed_queue = true;
    return true;
}


//cannot be a ramfunc, as it is used before available
json_object_t create_blank_json_object() {
    json_object_t json_object;

    //should have all defaults set
    json_object.parameter_name = "BLANK";
    json_object.combined_params.value_type = t_bool;
    json_object.combined_params.precision = 0;
    json_object.combined_params.free_array_after_print = false;
    json_object.combined_params.is_array = false;
    json_object.is_printing_bool_ptr = NULL;
    json_object.array_count = 0;
    json_object.combined_params.is_array = false;
    json_object.value.bool_ = false;

    return json_object;
}



__attribute__((ramfunc))
void copy_json_object(const json_object_t *const copy_from, json_object_t *const copy_to) {
    //a uint16_t is the smallest unit (1), so no reason to divide by it
    memcpy_fast(copy_to, copy_from, static_cast<uint16_t>(sizeof(json_object_t)));
}

__attribute__((ramfunc))
uint16_t get_next_delayed_objects_queue_i_for_writing() {
    //disable and store the interrupt state
    const uint16_t interrupt_settings = __disable_interrupts();

    //atomically get the index and write it to the queue
    const uint16_t queue_i = delayed_json_object_queue.write_index();

    if (!delayed_json_object_queue.write(queue_i)) {
        lockup_cpu();
    }

    //restore the interrupt state
    __restore_interrupts(interrupt_settings);

    return queue_i;
}

void deal_with_delay_printf_error() {
    has_had_print_error = true;

    //otherwise, show an error (if able)
    if (!delayed_json_object_queue.is_full()) {

        //allocate first, then copy
        json_object_t *const objects = create_array_of<json_object_t>(1, "printf_objects");
        if (objects == NULL) {
            has_had_fatal_print_error = true;
            lockup_cpu();
            return;
        }

        const uint16_t queue_i = get_next_delayed_objects_queue_i_for_writing();
        const json_object_t json_object = json_string("error", "printf delay queue is full");
        copy_json_object(&json_object, const_cast<json_object_t *>(&(objects[0])));
        delayed_json_objects[queue_i].object_count = 1;
        delayed_json_objects[queue_i].objects = objects;

    } else {
        has_had_fatal_print_error = true;
    }
}

__attribute__((ramfunc))
void delay_printf_json_objects(const delayed_json_t delayed_json_object) {
    if (!has_init_delayed_queue) {return;}

    //must always be at least 1 extra free space, otherwise show error
    if (delayed_json_object_queue.available() > 1) {

        //just copy the delayed_json object
        const uint16_t queue_i = get_next_delayed_objects_queue_i_for_writing();
        delayed_json_objects[queue_i].object_count = delayed_json_object.object_count;
        delayed_json_objects[queue_i].objects = delayed_json_object.objects;

    } else {

        deal_with_delay_printf_error();

        //and dealloc any objects that couldn't be added
        for (uint16_t object_i = 0; object_i<delayed_json_object.object_count; object_i++) {
            json_object_t temp_json_object;
            copy_json_object(const_cast<json_object_t *>(&(delayed_json_object.objects[object_i])), const_cast<json_object_t *>(&temp_json_object));
            if (temp_json_object.combined_params.is_array && temp_json_object.combined_params.free_array_after_print) {
                delete_array_for_value_type(temp_json_object.combined_params.value_type, temp_json_object.value.array_ptr);
            }
        }
    }
}

__attribute__((ramfunc))
void delay_printf_json_objects(uint16_t object_count, ...) {
    debug_timestamps.delay_printf_1 = CPU_TIMESTAMP;

    if (!has_init_delayed_queue) {return;}

    //va start before modification
    va_list json_objects;
    va_start(json_objects, object_count);

    //must always be at least 1 extra free space, otherwise show error
    if (delayed_json_object_queue.available() > 1) {

        json_object_t *const objects = create_array_of<json_object_t>(object_count, "printf_objects");
        if (objects == NULL) {
            has_had_fatal_print_error = true;
            lockup_cpu();
            return;
        }

        const uint16_t queue_i = get_next_delayed_objects_queue_i_for_writing();

        //add each object
        for (uint16_t object_i = 0; object_i<object_count; object_i++) {
            const json_object_t json_object = va_arg(json_objects, json_object_t);
            copy_json_object(&json_object, const_cast<json_object_t *>(&(objects[object_i])));
        }

        //copy to queue
        delayed_json_objects[queue_i].object_count = object_count;
        delayed_json_objects[queue_i].objects = objects;

    } else {

        deal_with_delay_printf_error();

        //and dealloc any objects that couldn't be added
        for (uint16_t object_i = 0; object_i<object_count; object_i++) {
            const json_object_t json_object = va_arg(json_objects, json_object_t);
            if (json_object.combined_params.is_array && json_object.combined_params.free_array_after_print) {
                delete_array_for_value_type(json_object.combined_params.value_type, json_object.value.array_ptr);
            }
        }
    }

    va_end(json_objects);

    debug_timestamps.delay_printf_2 = CPU_TIMESTAMP;
}

void printf_json_error(const char *const error_string) {
    printf_json_objects(1, json_string("error", error_string));
}

void printf_json_status(const char *const text_string) {
    printf_json_objects(1, json_string("status", text_string));
}

__attribute__((ramfunc))
void delay_printf_json_error(const char *const error_string) {
    if (!has_init_delayed_queue) {return;}

    delay_printf_json_objects(1, json_string("error", error_string));
}

__attribute__((ramfunc))
void delay_printf_json_status(const char *const text_string) {
    if (!has_init_delayed_queue) {return;}

    delay_printf_json_objects(1, json_string("status", text_string));
}

__attribute__((ramfunc))
void internal_printf_json_object(const json_object_t &json_object) {
    //print the string and colon
    printf_char('"');
    printf_string(json_object.parameter_name);
    printf_string("\":");

    //then print the value or array
    //all pointers inside pointer union should be the same (just test against NULL)
    if (json_object.combined_params.is_array) {
        internal_printf_array(json_object.combined_params.value_type, json_object.array_count, json_object.value.array_ptr, json_object.combined_params.precision);

        //now that it has been printed, free the pointer (if necessary)
        if (json_object.combined_params.free_array_after_print) {
            delete_array_for_value_type(json_object.combined_params.value_type, json_object.value.array_ptr);
        }

    } else {
        internal_printf_value(json_object.combined_params.value_type, &(json_object.value), 0, json_object.combined_params.precision);
    }

}

__attribute__((ramfunc))
void internal_printf_value(value_type_t value_type, const void *const value_ptr, uint16_t index, uint16_t precision_or_sig_figs) {

    switch (value_type) {
		//16 bits
        case t_bool:
            if ((reinterpret_cast<const bool *>(value_ptr))[index] == true) {
                printf_string("true");
            } else {
                printf_string("false");
            }
            break;

        case t_uint16:
            print_all_int_except_uint64_value(static_cast<int64_t>( (reinterpret_cast<const uint16_t *>(value_ptr))[index] ));
			break;

        case t_int16:
            print_all_int_except_uint64_value(static_cast<int64_t>( (reinterpret_cast<const int16_t *>(value_ptr))[index] ));
			break;

		//32 bits
        case t_uint32:
            print_all_int_except_uint64_value(static_cast<int64_t>( (reinterpret_cast<const uint32_t *>(value_ptr))[index] ));
			break;

		case t_int32:
		    print_all_int_except_uint64_value(static_cast<int64_t>( (reinterpret_cast<const int32_t *>(value_ptr))[index] ));
			break;

        case t_string:
            //has quotes on both sides because all strings require them in JSON
            printf_char('"');
            printf_string(reinterpret_cast<char **>(const_cast<void *>(value_ptr))[index]);
            printf_char('"');
            break;

		case t_float32:
		    print_float32_value( (reinterpret_cast<const float32 *>(value_ptr))[index] , precision_or_sig_figs);
			break;

		//64 bits
        case t_uint64:
            print_uint64_value( (reinterpret_cast<const uint64_t *>(value_ptr))[index] );
            break;

        case t_int64:
            print_all_int_except_uint64_value( (reinterpret_cast<const int64_t *>(value_ptr))[index] );
            break;

        case t_float64:
            print_float64_value( (reinterpret_cast<const float64 *>(value_ptr))[index] , precision_or_sig_figs);
            break;

        case t_timestamp:
            print_uint64_timestamp( (reinterpret_cast<const uint64_t *>(value_ptr))[index] , ts_multiplier, ts_shift);
            break;

        //default
        default:
            printf_string("invalid value_type (internal_printf)");
            break;
    }
}

//was static
__attribute__((ramfunc))
void internal_printf_array(value_type_t value_type, uint16_t array_count, const array_ptr_union_t values_ptr, uint16_t precision_or_sig_figs) {

    //open the array
    if (value_type == t_base64) {
        printf_char('\"');
    } else {
        printf_char('[');
    }

    // minimize conditionals inside printing loop, so a case for each type
    switch (value_type) {
        case t_base64:
            //new base64 printing
            print_base64_array(values_ptr.uint16_, array_count, precision_or_sig_figs);
            break;

        default:
            for (uint16_t value_index=0; value_index<array_count; value_index++) {
                //the values_ptr union should be a pointer to all
                internal_printf_value(value_type, values_ptr.bool_, value_index, precision_or_sig_figs);

                //print a comma, except after the last value
                if (value_index != (array_count - 1)) {
                    printf_char(',');
                }
            }
    }

    //close the array
    if (value_type == t_base64) {
        printf_char('\"');
    } else {
        printf_char(']');
    }
}

const char *json_type_name(jsonType_t json_type) {
    const char *name = "error";

    switch (json_type) {
        case JSON_OBJ:
            name = "JSON_OBJ";
            break;

        case JSON_ARRAY:
            name = "JSON_ARRAY";
            break;

        case JSON_TEXT:
            name = "JSON_TEXT";
            break;

        case JSON_BOOLEAN:
            name = "JSON_BOOLEAN";
            break;

        case JSON_INTEGER:
            name = "JSON_INTEGER";
            break;

        case JSON_REAL:
            name = "JSON_REAL";
            break;

        case JSON_NULL:
            name = "JSON_NULL";
            break;
    }

    return name;
}

const char *value_type_name(value_type_t value_type) {
    const char *name;

    switch (value_type) {
        case t_bool:
            name = "bool";
            break;

        case t_uint16:
            name = "uint16";
            break;

        case t_int16:
            name = "int16";
            break;

        case t_uint32:
            name = "uint32";
            break;

        case t_int32:
            name = "int32";
            break;

        case t_string:
            name = "string";
            break;

        case t_float32:
            name = "float32";
            break;

        case t_uint64:
            name = "uint64";
            break;

        case t_int64:
            name = "int64";
            break;

        case t_float64:
            name = "float64";
            break;

        case t_base64:
            name = "base64";
            break;

        case t_timestamp:
            name = "t_timestamp";
            break;

        default:
            name = "error";
            break;
    }

    return name;
}


//cpp-functions
json_object_t json_parent(const char *const parameter_name, uint16_t count) {
    json_object_t json_object = blank_json_object;
    json_object.parameter_name = parameter_name;
    json_object.combined_params.value_type = t_parent;
    json_object.array_count = count;
    return json_object;
}


json_object_t json_bool(const char *const parameter_name, bool value) {
    json_object_t json_object = blank_json_object;
    json_object.parameter_name = parameter_name;
    json_object.combined_params.value_type = t_bool;
    json_object.value.bool_ = value;
    return json_object;
}

json_object_t json_uint16(const char *const parameter_name, uint16_t value) {
    json_object_t json_object = blank_json_object;
    json_object.parameter_name = parameter_name;
    json_object.combined_params.value_type = t_uint16;
    json_object.value.uint16_ = value;
    return json_object;
}

json_object_t json_int16(const char *const parameter_name, int16_t value) {
    json_object_t json_object = blank_json_object;
    json_object.parameter_name = parameter_name;
    json_object.combined_params.value_type = t_int16;
    json_object.value.int16_ = value;
    return json_object;
}

json_object_t json_uint32(const char *const parameter_name, uint32_t value) {
    json_object_t json_object = blank_json_object;
    json_object.parameter_name = parameter_name;
    json_object.combined_params.value_type = t_uint32;
    json_object.value.uint32_ = value;
    return json_object;
}

json_object_t json_int32(const char *const parameter_name, int32_t value) {
    json_object_t json_object = blank_json_object;
    json_object.parameter_name = parameter_name;
    json_object.combined_params.value_type = t_int32;
    json_object.value.int32_ = value;
    return json_object;
}

json_object_t json_string(const char *const parameter_name, const char *const value) {
    json_object_t json_object = blank_json_object;
    json_object.parameter_name = parameter_name;
    json_object.combined_params.value_type = t_string;
    json_object.value.string_ = value;
    return json_object;
}

json_object_t json_float32(const char *const parameter_name, float32 value, uint16_t precision_or_sig_figs) {
    json_object_t json_object = blank_json_object;
    json_object.parameter_name = parameter_name;
    json_object.combined_params.value_type = t_float32;
    json_object.combined_params.precision = precision_or_sig_figs;
    json_object.value.float32_ = value;
    return json_object;
}

json_object_t json_uint64(const char *const parameter_name, uint64_t value) {
    json_object_t json_object = blank_json_object;
    json_object.parameter_name = parameter_name;
    json_object.combined_params.value_type = t_uint64;
    json_object.value.uint64_ = value;
    return json_object;
}

json_object_t json_int64(const char *const parameter_name, int64_t value) {
    json_object_t json_object = blank_json_object;
    json_object.parameter_name = parameter_name;
    json_object.combined_params.value_type = t_int64;
    json_object.value.int64_ = value;
    return json_object;
}

json_object_t json_float64(const char *const parameter_name, float64 value, uint16_t precision_or_sig_figs) {
    json_object_t json_object = blank_json_object;
    json_object.parameter_name = parameter_name;
    json_object.combined_params.value_type = t_float64;
    json_object.combined_params.precision = precision_or_sig_figs;
    json_object.value.float64_ = value;
    return json_object;
}

json_object_t json_timestamp(uint64_t timestamp) {
    json_object_t json_object = blank_json_object;
    json_object.parameter_name = "timestamp";
    json_object.combined_params.value_type = t_timestamp;
    json_object.value.uint64_ = timestamp;
    return json_object;
}

void error_allocating_array(json_object_t &json_object) {
    json_object.combined_params.value_type = t_string;
    json_object.value.string_ = "error allocating";
    json_object.combined_params.free_array_after_print = false;
    json_object.combined_params.is_array = false;
    json_object.value.array_ptr.bool_ = NULL;
}

json_object_t json_bool_array(const char *const parameter_name, uint16_t array_count, const bool *const array, bool copy) {
    json_object_t json_object = blank_json_object;
    json_object.parameter_name = parameter_name;
    json_object.combined_params.value_type = t_bool;
    json_object.array_count = array_count;
    json_object.combined_params.is_array = true;
    json_object.combined_params.free_array_after_print = copy;

    if (copy) {
        bool *const copy_array = create_array_of<bool>(array_count, "json_bool_array");
        if (copy_array != NULL) {
            memcpy_fast(copy_array, array, array_count);    //bool is 16bits
            json_object.value.array_ptr.bool_ = copy_array;    //store the copy
        } else {
            error_allocating_array(json_object);
        }
    } else {
        json_object.value.array_ptr.bool_ = array;         //store the original pointer
    }

    return json_object;
}

json_object_t json_uint16_array(const char *const parameter_name, uint16_t array_count, const uint16_t *const array, bool copy) {
    json_object_t json_object = blank_json_object;
    json_object.parameter_name = parameter_name;
    json_object.combined_params.value_type = t_uint16;
    json_object.array_count = array_count;
    json_object.combined_params.is_array = true;
    json_object.combined_params.free_array_after_print = copy;

    if (copy) {
        uint16_t *const copy_array = create_array_of<uint16_t>(array_count, "json_uint16_array");

        if (copy_array != NULL) {
            memcpy_fast(copy_array, array, array_count);    //uint16_t is 16bits
            json_object.value.array_ptr.uint16_ = copy_array;    //store the copy
        } else {
            error_allocating_array(json_object);
        }
    } else {
        json_object.value.array_ptr.uint16_ = array;         //store the original pointer
    }

    return json_object;
}

json_object_t json_int16_array(const char *const parameter_name, uint16_t array_count, const int16_t *const array, bool copy) {
    json_object_t json_object = blank_json_object;
    json_object.parameter_name = parameter_name;
    json_object.combined_params.value_type = t_int16;
    json_object.array_count = array_count;
    json_object.combined_params.is_array = true;
    json_object.combined_params.free_array_after_print = copy;

    if (copy) {
        int16_t *const copy_array = create_array_of<int16_t>(array_count, "json_int16_array");

        if (copy_array != NULL) {
            memcpy_fast(copy_array, array, array_count);    //int16_t is 16bits
            json_object.value.array_ptr.int16_ = copy_array;    //store the copy
        } else {
            error_allocating_array(json_object);
        }
    } else {
        json_object.value.array_ptr.int16_ = array;         //store the original pointer
    }

    return json_object;
}

json_object_t json_uint32_array(const char *const parameter_name, uint16_t array_count, const uint32_t *const array, bool copy) {
    json_object_t json_object = blank_json_object;
    json_object.parameter_name = parameter_name;
    json_object.combined_params.value_type = t_uint32;
    json_object.array_count = array_count;
    json_object.combined_params.is_array = true;
    json_object.combined_params.free_array_after_print = copy;

    if (copy) {
        uint32_t *const copy_array = create_array_of<uint32_t>(array_count, "json_uint32_array");

        if (copy_array != NULL) {
            memcpy_fast(copy_array, array, array_count * 2);    //uint32_t is 32bits
            json_object.value.array_ptr.uint32_ = copy_array;    //store the copy
        } else {
            error_allocating_array(json_object);
        }
    } else {
        json_object.value.array_ptr.uint32_ = array;         //store the original pointer
    }

    return json_object;
}

json_object_t json_int32_array(const char *const parameter_name, uint16_t array_count, const int32_t *const array, bool copy) {
    json_object_t json_object = blank_json_object;
    json_object.parameter_name = parameter_name;
    json_object.combined_params.value_type = t_int32;
    json_object.array_count = array_count;
    json_object.combined_params.is_array = true;
    json_object.combined_params.free_array_after_print = copy;

    if (copy) {
        int32_t *const copy_array = create_array_of<int32_t>(array_count, "json_int32_array");

        if (copy_array != NULL) {
            memcpy_fast(copy_array, array, array_count);    //int32_t is 32bits
            json_object.value.array_ptr.int32_ = copy_array;    //store the copy
        } else {
            error_allocating_array(json_object);
        }
    } else {
        json_object.value.array_ptr.int32_ = array;         //store the original pointer
    }

    return json_object;
}

json_object_t json_float32_array(const char *const parameter_name, uint16_t array_count, const float32 *const array, bool copy, uint16_t precision_or_sig_figs) {
    json_object_t json_object = blank_json_object;
    json_object.parameter_name = parameter_name;
    json_object.combined_params.value_type = t_float32;
    json_object.combined_params.precision = precision_or_sig_figs;
    json_object.array_count = array_count;
    json_object.combined_params.is_array = true;
    json_object.combined_params.free_array_after_print = copy;

    if (copy) {
        float32 *const copy_array = create_array_of<float32>(array_count, "json_float32_array");

        if (copy_array != NULL) {
            memcpy_fast(copy_array, array, array_count * 2);    //float32 is 32bits
            json_object.value.array_ptr.float32_ = copy_array;    //store the copy
        } else {
            error_allocating_array(json_object);
        }
    } else {
        json_object.value.array_ptr.float32_ = array;         //store the original pointer
    }

    return json_object;
}

json_object_t json_string_array(const char *const parameter_name, uint16_t array_count, const char **const array, bool copy) {
    json_object_t json_object = blank_json_object;
    json_object.parameter_name = parameter_name;
    json_object.combined_params.value_type = t_string;
    json_object.array_count = array_count;
    json_object.combined_params.is_array = true;
    json_object.combined_params.free_array_after_print = copy;

    if (copy) {
        char **const copy_array = create_array_of<char *>(array_count, "json_string_array");

        if (copy_array != NULL) {
            memcpy_fast(copy_array, array, array_count * 2);    //*char is 32bits
            json_object.value.array_ptr.string_ = const_cast<const char**>(copy_array);    //store the copy
        } else {
            error_allocating_array(json_object);
        }
    } else {
        json_object.value.array_ptr.string_ = array;         //store the original pointer
    }

    return json_object;
}

json_object_t json_uint64_array(const char *const parameter_name, uint16_t array_count, const uint64_t *const array, bool copy) {
    json_object_t json_object = blank_json_object;
    json_object.parameter_name = parameter_name;
    json_object.combined_params.value_type = t_uint64;
    json_object.array_count = array_count;
    json_object.combined_params.is_array = true;
    json_object.combined_params.free_array_after_print = copy;

    if (copy) {
        uint64_t *const copy_array = create_array_of<uint64_t>(array_count, "json_uint64_array");

        if (copy_array != NULL) {
            memcpy_fast(copy_array, array, array_count * 4);    //uint64_t is 64bits
            json_object.value.array_ptr.uint64_ = copy_array;    //store the copy
        } else {
            error_allocating_array(json_object);
        }
    } else {
        json_object.value.array_ptr.uint64_ = array;         //store the original pointer
    }

    return json_object;
}

json_object_t json_int64_array(const char *const parameter_name, uint16_t array_count, const int64_t *const array, bool copy) {
    json_object_t json_object = blank_json_object;
    json_object.parameter_name = parameter_name;
    json_object.combined_params.value_type = t_int64;
    json_object.array_count = array_count;
    json_object.combined_params.is_array = true;
    json_object.combined_params.free_array_after_print = copy;

    if (copy) {
        int64_t *const copy_array = create_array_of<int64_t>(array_count, "json_int64_array");

        if (copy_array != NULL) {
            memcpy_fast(copy_array, array, array_count * 4);    //int64_t is 64bits
            json_object.value.array_ptr.int64_ = copy_array;    //store the copy
        } else {
            error_allocating_array(json_object);
        }
    } else {
        json_object.value.array_ptr.int64_ = array;         //store the original pointer
    }

    return json_object;
}

json_object_t json_float64_array(const char *const parameter_name, uint16_t array_count, const float64 *const array, bool copy, uint16_t precision_or_sig_figs) {
    json_object_t json_object = blank_json_object;
    json_object.parameter_name = parameter_name;
    json_object.combined_params.value_type = t_float64;
    json_object.combined_params.precision = precision_or_sig_figs;
    json_object.array_count = array_count;
    json_object.combined_params.is_array = true;
    json_object.combined_params.free_array_after_print = copy;

    if (copy) {
        float64 *const copy_array = create_array_of<float64>(array_count, "json_float64_array");

        if (copy_array != NULL) {
            memcpy_fast(copy_array, array, array_count * 4);    //float64 is 64bits
            json_object.value.array_ptr.float64_ = copy_array;    //store the copy
        } else {
            error_allocating_array(json_object);
        }
    } else {
        json_object.value.array_ptr.float64_ = array;         //store the original pointer
    }

    return json_object;
}

json_object_t json_base64_array(const char *const parameter_name, uint16_t array_count, const uint16_t *const array, bool copy, uint16_t bits) {
    json_object_t json_object = blank_json_object;

    json_object.parameter_name = parameter_name;
    json_object.combined_params.value_type = t_base64;
    json_object.combined_params.precision = bits;  //number of bits
    json_object.array_count = array_count;
    json_object.combined_params.is_array = true;
    json_object.combined_params.free_array_after_print = copy;

    if (copy) {
        uint16_t *const copy_array = create_array_of<uint16_t>(array_count, "json_base64_array");

        if (copy_array != NULL) {
            memcpy_fast(copy_array, array, array_count);
            json_object.value.array_ptr.uint16_ = copy_array;    //store the copy
        } else {
            error_allocating_array(json_object);
        }
    } else {
        json_object.value.array_ptr.uint16_ = array;         //store the original pointer
    }

    return json_object;
}

void set_json_timestamp_freq(uint32_t timestamp_freq) {

    //lazy solution
    switch (timestamp_freq) {
        case 1000:
            ts_multiplier = 1;
            ts_shift = 3;
            break;

        case 2000:
            ts_multiplier = 5;
            ts_shift = 4;
            break;

        case 2500:
            ts_multiplier = 4;
            ts_shift = 4;
            break;

        case 4000:
            ts_multiplier = 25;
            ts_shift = 5;
            break;

        default:
            delay_printf_json_error("json_timestamp is not ready for this frequency");
            ts_multiplier = 1;
            ts_shift = 0;
            break;
    }
}

__attribute__((ramfunc))
uint16_t array_size_multiplier_for_value_type(value_type_t value_type) {
    uint16_t multiplier = 0;

    switch (value_type) {
        //16 bits
        case t_bool:
        case t_uint16:
        case t_int16:
            multiplier = 1;
            break;

        //32 bits
        case t_uint32:
        case t_int32:
        case t_string:
        case t_float32:
            multiplier = 2;
            break;

        //64 bits
        case t_uint64:
        case t_int64:
        case t_float64:
            multiplier = 4;
            break;

        default:
            multiplier = 0;
            break;
    }

    return multiplier;
}

__attribute__((ramfunc))
void delete_array_for_value_type(value_type_t value_type, const array_ptr_union_t array_ptr) {

    switch (value_type) {
        case t_bool:
            delete_array(array_ptr.bool_);
            break;

        case t_uint16:
            delete_array(array_ptr.uint16_);
            break;

        case t_int16:
            delete_array(array_ptr.int16_);
            break;

        case t_uint32:
            delete_array(array_ptr.uint32_);
            break;

        case t_int32:
            delete_array(array_ptr.int32_);
            break;

        case t_string:
            delete_array(array_ptr.string_);
            break;

        case t_float32:
            delete_array(array_ptr.float32_);
            break;

        case t_uint64:
            delete_array(array_ptr.uint64_);
            break;

        case t_int64:
            delete_array(array_ptr.int64_);
            break;

        case t_float64:
            delete_array(array_ptr.float64_);
            break;

        case t_base64:
            delete_array(array_ptr.uint16_);
            break;

        default:
            delay_printf_json_error("invalid type, cannot delete array");
            break;
    }
}
