/*
 * printf_json_types.h
 *
 *  Created on: May 17, 2017
 *      Author: adam jones
 */



#ifndef json_types_defined
#define json_types_defined

#include "stdint.h"
#include "stdbool.h"
#include "F28x_Project.h"
#include "tiny_json.h"


typedef enum {
    //just a parent
    t_parent,

    //16 bits
    t_bool,
    t_uint16,
    t_int16,

    //32 bits
    t_uint32,
    t_int32,
    t_string,
    t_float32,

    //64 bits
    t_uint64,
    t_int64,
    t_float64,

    //compressed types
    t_base64,

    //timestamp
    t_timestamp,

    //nothing
    t_nothing
} value_type_t; //allow for 32 options

//confirmed to be the same size as any/all pointers contained within
typedef union {
    //16 bits
    const bool     *bool_;
    const uint16_t *uint16_;
    const int16_t  *int16_;

    //32 bits
    const uint32_t *uint32_;
    const int32_t  *int32_;
    const char    **string_;
    const float32  *float32_;

    //64 bits
    const uint64_t *uint64_;
    const int64_t  *int64_;
    const float64  *float64_;
} array_ptr_union_t;    //2bytes

typedef union {
    //16 bits
    bool        bool_;
    uint16_t    uint16_;
    int16_t     int16_;

    //32 bits
    uint32_t    uint32_;
    int32_t     int32_;
    const char *string_;
    float32     float32_;

    //64 bits
    uint64_t    uint64_;
    int64_t     int64_;
    float64     float64_;

    //array pointer (if array)
    array_ptr_union_t   array_ptr;
} value_union_t;    //4bytes

//combined params (to shrink)
typedef struct json_params_t {
    uint16_t        precision               : 5;    //max value 31
    value_type_t    value_type              : 5;    //32 options
    bool            is_array                : 1;    //bool
    bool            free_array_after_print  : 1;    //bool
    uint16_t        unused                  : 4;    //unused
} json_params_t;    //2bytes


//11bytes (+ 1 bytes buffer) = 12bytes
typedef struct json_object_t {
    //ordered to be packed better in memory
    const char         *parameter_name;         //2bytes
    json_params_t       combined_params;        //2bytes
    bool               *is_printing_bool_ptr;   //2bytes    //set to false, once printed
    value_union_t       value;                  //4bytes
    uint16_t            array_count;            //1byte
} json_object_t;

extern const json_object_t blank_json_object;

//overloaded pair
json_object_t json_parent(const char *const parameter_name, uint16_t count);
json_object_t json_bool(const char *const parameter_name, bool value);
json_object_t json_uint16(const char *const parameter_name, uint16_t value);
json_object_t json_int16(const char *const parameter_name, int16_t value);
json_object_t json_uint32(const char *const parameter_name, uint32_t value);
json_object_t json_int32(const char *const parameter_name, int32_t value);
json_object_t json_string(const char *const parameter_name, const char *const value);
json_object_t json_float32(const char *const parameter_name, float32 value, uint16_t float_precision = 7);
json_object_t json_uint64(const char *const parameter_name, uint64_t value);
json_object_t json_int64(const char *const parameter_name, int64_t value);
json_object_t json_float64(const char *const parameter_name, float64 value, uint16_t float_precision = 15);

//overloaded array
json_object_t json_bool_array(const char *const parameter_name, uint16_t array_count, const bool *const array, bool copy = false);
json_object_t json_uint16_array(const char *const parameter_name, uint16_t array_count, const uint16_t *const array, bool copy = false);
json_object_t json_int16_array(const char *const parameter_name, uint16_t array_count, const int16_t *const array, bool copy = false);
json_object_t json_uint32_array(const char *const parameter_name, uint16_t array_count, const uint32_t *const array, bool copy = false);
json_object_t json_int32_array(const char *const parameter_name, uint16_t array_count, const int32_t *const array, bool copy = false);
json_object_t json_float32_array(const char *const parameter_name, uint16_t array_count, const float32 *const array, bool copy = false, uint16_t float_precision = 7);
json_object_t json_uint64_array(const char *const parameter_name, uint16_t array_count, const uint64_t *const array, bool copy = false);
json_object_t json_int64_array(const char *const parameter_name, uint16_t array_count, const int64_t *const array, bool copy = false);
json_object_t json_float64_array(const char *const parameter_name, uint16_t array_count, const float64 *const array, bool copy = false, uint16_t float_precision = 15);
json_object_t json_string_array(const char *const parameter_name, uint16_t array_count, const char **const array, bool copy = false);
json_object_t json_base64_array(const char *const parameter_name, uint16_t array_count, const uint16_t *const array, bool copy = false, uint16_t bits = 16);


//helpers
void copy_json_object(const json_object_t *const copy_from, json_object_t *const copy_to);
const char *value_type_name(value_type_t value_type);
const char *json_type_name(jsonType_t json_type);

//timestamps
void set_json_timestamp_freq(uint32_t timestamp_freq);
json_object_t json_timestamp(uint64_t timestamp);


#endif
