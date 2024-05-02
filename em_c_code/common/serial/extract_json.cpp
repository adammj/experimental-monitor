/*
 * extract_json.c
 *
 *  Created on: Dec 3, 2017
 *      Author: adam jones
 */

#include "extract_json.h"
#include "printf_json_delayed.h"
#include "scia.h"
#include "misc.h"
#include "limits.h"
#include "fpu_vector.h"
#include "stdlib.h"
#include "string.h"
#include "errno.h"
#include "arrays.h"
#include "printf_json.h"


//internal functions
bool set_pointer_with_json_object(const char* const value, const void *const value_ptr, uint16_t index, value_type_t value_type);
bool store_value_for_value_type(const char* const value, const value_type_t value_type, value_union_t &value_union);
bool types_are_compatible(value_type_t value_type, jsonType_t json_type);
const json_t* get_json_property(const json_t *const obj, char const* const property);

//cannot be ramfunc as these are created before the code runs (in experimental_input)
json_element::json_element(const char *const property_name, value_type_t value_type, bool is_required, bool force_array_creation) {

    //set properties
    property_name_ = property_name;
    requirements_.value_type = value_type;
    requirements_.is_required = is_required;
    requirements_.force_array = force_array_creation;

    //set defaults for other
    value_.bool_ = false;
    array_ptr_ = NULL;
    count_found_ = 0;
}

//don't make ramfunc, just in case
json_element::~json_element() {
    //delete array, if not null
    delete_array(array_ptr_);
    array_ptr_ = NULL;
}

__attribute__((ramfunc))
void json_element::reset() {
    //reset all non-set parameters
    //property_name, value_type_t value_type, bool is_required

    //delete array, if not null
    delete_array(array_ptr_);
    array_ptr_ = NULL;

    count_found_ = 0;
    value_.bool_ = false;
}

//73 bytes
__attribute__((ramfunc))
const json_t* get_json_property(const json_t *const obj, char const* const property) {
    const json_t* current_object = obj;

    while (current_object != NULL) {
        if ((current_object->name != NULL) && (strcmp(current_object->name, property) == 0)) {
            return current_object;
        }
        current_object = current_object->sibling;
    }

    return NULL;
}

//366 bytes
__attribute__((ramfunc))
bool json_element::set_with_json(const json_t *const json_root, bool print_if_missing) {
    //if root is null, then exit
    if (json_root == NULL) {
        if (print_if_missing) {
            delay_printf_json_objects(2, json_string("error", "property missing"), json_string("property", property_name_));
        }
        return false;
    }

    const json_t *json_property_ = get_json_property(json_root, property_name_);

    //if the property doesn't exist, exit
    if (json_property_ != NULL) {

        bool is_array = false;

        //if the type is an array, then get the first child
        if (json_property_->type == JSON_ARRAY) {
            is_array = true;
            json_property_ = json_property_->u.child;
        }

        //see if the the type matches the expected
        if (!types_are_compatible((requirements_.value_type), json_property_->type)) {
            delay_printf_json_objects(4, json_string("error", "wrong type of input type for property"),
                                      json_string("property", property_name_),
                                      json_string("expected", value_type_name(requirements_.value_type)),
                                      json_string("received", json_type_name(json_property_->type)));
            json_property_ = NULL;
            return false;
        }

        //set the first value
        bool success = store_value_for_value_type(json_property_->u.value, (requirements_.value_type), value_);

        if (!success) {
            delay_printf_json_objects(3, json_string("error", "error in converting to expected type"),
                                      json_string("expected_type", value_type_name(requirements_.value_type)),
                                      json_string("received_text", json_property_->u.value));
            json_property_ = NULL;
            return false;
        }

        //now, count the siblings
        uint16_t count = 1;
        const json_t *temp_property_ptr = json_property_;

        //only if array, go looking for siblings
        if (is_array) {
            while (temp_property_ptr->sibling != NULL) {
                temp_property_ptr = temp_property_ptr->sibling;
                count++;
            }
        }

        //create an array for the values, if count > 1, or forced
        if ((count > 1) || (requirements_.force_array)) {

            //create equivalent array and check it
            const uint16_t multiplier = array_size_multiplier_for_value_type(requirements_.value_type);
            const uint64_t equivalent_length = static_cast<uint64_t>(count)*static_cast<uint64_t>(multiplier);
            const uint64_t max_length = get_max_heap_free() - 2; //subtract 2 for padding
            if (equivalent_length > max_length) {
                delay_printf_json_error("not enough space to allocate extracted array");
                json_property_ = NULL;
                return false;
            }

            //use heap array, since size is unknown
            array_ptr_ = create_array_of<uint16_t>(static_cast<uint16_t>(equivalent_length), "array_ptr_");
            if (array_ptr_ == NULL) {
                json_property_ = NULL;
                return false;
            }

            count_found_ = count;

            //reset the temp ptr
            temp_property_ptr = json_property_;
            for (uint16_t i=0; i<count; i++) {
                //set each sibling, and move to the next
                success = set_pointer_with_json_object(temp_property_ptr->u.value, array_ptr_, i, (requirements_.value_type));

                //check if there was a failure
                if (!success) {
                    delay_printf_json_objects(3, json_string("error", "error in converting to expected type"),
                                                          json_string("expected", value_type_name(requirements_.value_type)),
                                                          json_string("received", temp_property_ptr->u.value));
                    json_property_ = NULL;
                    delete_array(array_ptr_);
                    array_ptr_ = NULL;
                    return false;
                }

                temp_property_ptr = temp_property_ptr->sibling;
            }
        } else {
            //make sure this is set
            array_ptr_ = NULL;
            count_found_ = count;
        }

        return true;
    }

    if (print_if_missing) {
        delay_printf_json_objects(2, json_string("error", "property missing"), json_string("property", property_name_));
    }

    return false;
}

__attribute__((ramfunc))
bool json_element::does_exist_in_json(const json_t *const json_root) const {
    return (get_json_property(json_root, property_name_) != NULL);
}

//49 bytes
__attribute__((ramfunc))
void json_element::print_element() const {
    delay_printf_json_objects(3, json_string("property", property_name_),
                                 json_string("type", value_type_name(requirements_.value_type)),
                                 json_bool("required", requirements_.is_required));
}

//52 bytes
__attribute__((ramfunc))
bool types_are_compatible(value_type_t value_type, jsonType_t json_type) {
    bool are_compatible = false;

    switch (value_type) {
        //bool can only store bool
        case t_bool:
            are_compatible = (json_type == JSON_BOOLEAN);
            break;

        //integers can only store integers
        case t_uint16:
        case t_int16:
        case t_uint32:
        case t_int32:
        case t_uint64:
        case t_int64:
            are_compatible = (json_type == JSON_INTEGER);
            break;

        //string can only store text
        case t_string:
            are_compatible = (json_type == JSON_TEXT);
            break;

        //floats can store floats or integers
        case t_float32:
        case t_float64:
            are_compatible = ((json_type == JSON_REAL) || (json_type == JSON_INTEGER));
            break;

        default:
            are_compatible = false;
            break;
    }

    return are_compatible;
}


__attribute__((ramfunc))
bool has_help_element(const json_t *const json_root) {
    return (get_json_property(json_root, "help") != NULL);
}

//68 bytes
__attribute__((ramfunc))
uint16_t set_elements_with_json(const json_t *const json_root, uint16_t element_count, ...) {
    va_list element_list;

    //if error or no elements
    if (json_root == NULL) {
        delay_printf_json_error("invalid JSON object");
        return 0;
    }

    //copy pointers to array of pointers
    va_start(element_list, element_count);

    //use heap array, since size is unknown
    json_element **const temp_element_array = create_array_of<json_element *>(element_count, "temp_element_array");
    if (temp_element_array == NULL) {
        return 0;
    }

    for (uint16_t element_i=0; element_i<element_count; element_i++) {
        json_element *const element = va_arg(element_list, json_element*);
        temp_element_array[element_i] = element;
    }

    va_end(element_list);

    //now, fill the elements from the json objects
    const uint16_t filled_count = fill_element_array_with_json(json_root, element_count, temp_element_array);

    delete_array(temp_element_array);

    return filled_count;
}

//376 bytes
__attribute__((ramfunc))
uint16_t fill_element_array_with_json(const json_t *const json_root, uint16_t element_count, json_element **const element_array) {
    //for all required elements, first check that the properties exist - throw error if not
    //then, fill all elements into their properties
    //internal, so extra checks are eliminated

    json_element *current_element;
    bool found_required_elements = true;
    uint16_t filled_count = 0;

    //if one element is "help"
    if (has_help_element(json_root)) {

        const uint16_t total_count = 1 + (3*element_count);

        delayed_json_t delayed_json_object;
        delayed_json_object.object_count = total_count;
        //use heap array, since size is unknown
        delayed_json_object.objects = create_array_of<json_object_t>(total_count, "printf_objects");
        if (delayed_json_object.objects == NULL) {
            return 0;
        }

        json_object_t temp_object = json_parent("help", (total_count - 1));
        copy_json_object(const_cast<json_object_t *>(&temp_object), &(delayed_json_object.objects[0]));

        for (uint16_t i=0; i<element_count; i++) {
            current_element = element_array[i];

            //property
            temp_object = json_string("property", current_element->property_name());
            copy_json_object(const_cast<json_object_t *>(&temp_object), &(delayed_json_object.objects[1+(3*i)]));

            //type
            temp_object = json_string("type", value_type_name(current_element->value_type()));
            copy_json_object(const_cast<json_object_t *>(&temp_object), &(delayed_json_object.objects[2+(3*i)]));

            //required
            temp_object = json_bool("property", current_element->is_required());
            copy_json_object(const_cast<json_object_t *>(&temp_object), &(delayed_json_object.objects[3+(3*i)]));
        }

        //add to the queue
        delay_printf_json_objects(delayed_json_object);

        return 0;
    }

    //first check required elements and also reset each (just in case)
    for (uint16_t i=0; i<element_count; i++) {
        current_element = element_array[i];
        current_element->reset();

        if (current_element->is_required()) {
            if (!current_element->does_exist_in_json(json_root)) {
                delay_printf_json_objects(2, json_string("error", "missing required parameter"), json_string("parameter", current_element->property_name()));
                found_required_elements = false;
            }
        }
    }

    //next, check for any extra parameters not asked for
    const json_t *current_json_property = json_root;

    //use heap array, since size is unknown
    bool *const still_not_found = create_array_of<bool>(element_count, "still_not_found");
    if (still_not_found == NULL) {
        return 0;
    }

    for (uint16_t i=0; i<element_count; i++) {
        still_not_found[i] = true;
    }

    while (current_json_property != NULL) {
        const char *const property_name = current_json_property->name;

        if (property_name != NULL) {
            bool invalid_property = true;

            //look through each element
            for (uint16_t i=0; i<element_count; i++) {

                current_element = element_array[i];
                const bool matches_property = (strcmp(current_element->property_name(), property_name) == 0);

                if (matches_property) {
                    invalid_property = false;

                    if (still_not_found[i]) {
                        still_not_found[i] = false;
                    } else {
                        delay_printf_json_objects(2, json_string("error", "duplicate property"), json_string("property", property_name));

                    }
                }
            }

            if (invalid_property) {
                delay_printf_json_objects(2, json_string("error", "invalid property"), json_string("property", property_name));
            }
        }

        //go to next sibling
        current_json_property = current_json_property->sibling;
    }

    delete_array(still_not_found);

    //if not all required elements were found, then exit with error
    if (!found_required_elements) {
        return 0;
    }

    for (uint16_t i=0; i<element_count; i++) {
        current_element = element_array[i];
        if (current_element->set_with_json(json_root, false)) {
            filled_count++;
        }
    }

    return filled_count;
}

//140 bytes
__attribute__((ramfunc))
bool store_value_for_value_type(const char* const value, const value_type_t value_type, value_union_t &value_union) {

    char *end_ptr = NULL;
    errno = 0;

    switch (value_type) {
        case t_bool:
            value_union.bool_ = (*value == 't');
            break;

        case t_uint16:
            value_union.uint16_ = static_cast<uint16_t>(strtoul(value, &end_ptr, 10));
            break;

        case t_int16:
            value_union.int16_ = static_cast<int16_t>(strtol(value, &end_ptr, 10));
            break;

        case t_uint32:
            value_union.uint32_ = strtoul(value, &end_ptr, 10);
            break;

        case t_int32:
            value_union.int32_ = strtol(value, &end_ptr, 10);
            break;

        case t_uint64:
            value_union.uint64_ = strtoull(value, &end_ptr, 10);
            break;

        case t_int64:
            value_union.int64_ = strtoll(value, &end_ptr, 10);
            break;

        case t_string:
            value_union.string_ = value;
            break;

        case t_float32:
            value_union.float32_ = strtod(value, &end_ptr);
            break;

        case t_float64:
            value_union.float64_ = static_cast<float64>(strtod(value, &end_ptr)); //float32 value is sufficient
            break;

        default:
            value_union.bool_ = false;
            break;
    }

    //if not failed, then success
    // errno must be 0 and either one of the following
    // - end_ptr is still null
    // - value at end_ptr is 0, and value and end_ptr don't point to the same location
    return ((errno == 0) && ((end_ptr == NULL) || ((*end_ptr == 0) && (value != end_ptr))));
}

//148 bytes
__attribute__((ramfunc))
bool set_pointer_with_json_object(const char* const value, const void *const value_ptr, uint16_t index, value_type_t value_type) {
    //internal, and assume checks have already been made

    char *end_ptr = NULL;
    errno = 0;

    switch (value_type) {

        case t_bool:
            (reinterpret_cast<bool *>(const_cast<void *>(value_ptr)))[index] = (*value == 't');
            break;

        case t_uint16:
            (reinterpret_cast<uint16_t *>(const_cast<void *>(value_ptr)))[index] = static_cast<uint16_t>(strtoul(value, &end_ptr, 10));
            break;

        case t_int16:
            (reinterpret_cast<int16_t *>(const_cast<void *>(value_ptr)))[index] = static_cast<int16_t>(strtol(value, &end_ptr, 10));
            break;

        case t_uint32:
            (reinterpret_cast<uint32_t *>(const_cast<void *>(value_ptr)))[index] = strtoul(value, &end_ptr, 10);
            break;

        case t_int32:
            (reinterpret_cast<int32_t *>(const_cast<void *>(value_ptr)))[index] = strtol(value, &end_ptr, 10);
            break;

        case t_uint64:
            (reinterpret_cast<uint64_t *>(const_cast<void *>(value_ptr)))[index] = strtoull(value, &end_ptr, 10);
            break;

        case t_int64:
            (reinterpret_cast<int64_t *>(const_cast<void *>(value_ptr)))[index] = strtoll(value, &end_ptr, 10);
            break;

        case t_float32:
            (reinterpret_cast<float32 *>(const_cast<void *>(value_ptr)))[index] = strtod(value, &end_ptr);
            break;

        case t_float64:
            (reinterpret_cast<float64 *>(const_cast<void *>(value_ptr)))[index] = static_cast<float64>(strtod(value, &end_ptr));  //float32 value is sufficient
            break;

        case t_string:
            (reinterpret_cast<char **>(const_cast<void *>(value_ptr)))[index] = const_cast<char *>(value);
            break;

        default:
            return false;
    }

    //if not failed, then success
    // errno must be 0 and either one of the following
    // - end_ptr is still null
    // - value at end_ptr is 0, and value and end_ptr don't point to the same location
    return ((errno == 0) && ((end_ptr == NULL) || ((*end_ptr == 0) && (value != end_ptr))));
}
