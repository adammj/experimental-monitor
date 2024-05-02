/*
 * extract_json.h
 *
 *  Created on: Dec 3, 2017
 *      Author: adam jones
 */

#ifndef extract_json_defined
#define extract_json_defined

#include "tiny_json.h"
#include "stdint.h"
#include "stdbool.h"
#include "printf_json_types.h"

typedef struct requirements_t {
    value_type_t value_type : 6;
    bool is_required        : 1;
    bool force_array        : 1;
} requirements_t;


class json_element {
    public:
        //alloc or dealloc
        json_element(const char *const property_name, value_type_t value_type, bool is_required = false, bool force_array_creation = false);
        ~json_element();

        //check or set
        bool does_exist_in_json(const json_t *const json_root) const;
        bool set_with_json(const json_t *const json_root, bool print_if_missing);

        //print details
        void print_element() const;
        bool is_required() const {return requirements_.is_required;}
        bool force_array() const {return requirements_.force_array;}
        const char *property_name() const {return property_name_;}
        uint16_t count_found() const {return count_found_;}

        //functions to get the value(s)
        value_union_t value() const {return value_;}
        value_type_t value_type() const {return requirements_.value_type;}

        //functions to get the array (in the correct type)
        const bool *get_bool_array() const {return reinterpret_cast<bool *>(array_ptr_);}
        const uint16_t *get_uint16_array() const {return array_ptr_;}
        const int16_t *get_int16_array() const {return reinterpret_cast<int16_t *>(array_ptr_);}
        const uint32_t *get_uint32_array() const {return reinterpret_cast<uint32_t *>(array_ptr_);}
        const int32_t *get_int32_array() const {return reinterpret_cast<int32_t *>(array_ptr_);}
        const uint64_t *get_uint64_array() const {return reinterpret_cast<uint64_t *>(array_ptr_);}
        const int64_t *get_int64_array() const {return reinterpret_cast<int64_t *>(array_ptr_);}
        const float32 *get_float32_array() const {return reinterpret_cast<float32 *>(array_ptr_);}
        const float64 *get_float64_array() const {return reinterpret_cast<float64 *>(array_ptr_);}

        void reset();   //reset the element

    private:
        //basic properties required and set during creation
        //ordered to be packed better in memory
        const char *property_name_;     //2bytes
        requirements_t requirements_;   //1byte
        uint16_t count_found_;          //1byte  //found count (0 = not found)

        //everything else is set by json object
        uint16_t *array_ptr_;       //2bytes //array is stored as equivalent uint16_t, malloced if at least one value is found
        value_union_t value_;       //4byte  //first or only value
};

//work with several elements
uint16_t set_elements_with_json(const json_t *const json_root, uint16_t element_count, ...);
uint16_t fill_element_array_with_json(const json_t *const json_root, uint16_t element_count, json_element **const element_array);

bool has_help_element(const json_t *const json_root);

void test_extraction();

#endif
