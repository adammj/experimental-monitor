/*
 * ring_buffer.cpp
 *
 *  Created on: Dec 25, 2017
 *      Author: adam jones
 */

#include "ring_buffer.h"
#include "stdlib.h"
#include "misc.h"
#include "strings.h"
#include "fpu_vector.h"
#include "arrays.h"


//can trap uninitalized ring buffer, otherwise disable for performance
//#define trap_uninitalized


//cannot make ramfunc
ring_buffer::ring_buffer() {
    initialized_ = false;
    size_ = 0;
    in_use_ = 0;
    read_index_ = 0;
    write_index_ = 0;
    buffer_ = NULL;
}

//cannot make ramfunc
ring_buffer::~ring_buffer() {
    //dealloc first
    this->dealloc_buffer();

    //update
    initialized_ = false;
    size_ = 0;
    in_use_ = 0;
    read_index_ = 0;
    write_index_ = 0;
    buffer_ = NULL;
}

//cannot make ramfunc
bool ring_buffer::init_alloc(uint16_t size) volatile {
    if (initialized_) {
        return true;
    }

    size_ = size;
    in_use_ = 0;
    read_index_ = 0;
    write_index_ = 0;

    //allocate the buffer
    buffer_ = create_array_of<uint16_t>(size, "ring_buffer buffer");

    //test if successful
    initialized_ = (buffer_ != NULL);

    return initialized_;
}

void ring_buffer::dealloc_buffer() volatile {
    if (!initialized_) {
        return;
    }

    //disable and store the interrupt state
    const uint16_t interrupt_settings = __disable_interrupts();

    //only if pointer still points somewhere, then delete
    delete_array(buffer_);
    buffer_ = NULL;

    size_ = 0;
    in_use_ = 0;
    read_index_ = 0;
    write_index_ = 0;

    //assume this is successful
    initialized_ = false;

    //restore the interrupt state
    __restore_interrupts(interrupt_settings);
}

__attribute__((ramfunc))
void ring_buffer::reset() volatile {
    //disable and store the interrupt state
    const uint16_t interrupt_settings = __disable_interrupts();

    in_use_ = 0;
    read_index_ = 0;
    write_index_ = 0;

    //restore the interrupt state
    __restore_interrupts(interrupt_settings);
}

__attribute__((ramfunc))
bool ring_buffer::read(uint16_t &read_value) volatile {
    //disable and store the interrupt state
    const uint16_t interrupt_settings = __disable_interrupts();

#ifdef trap_uninitalized
    if (!initialized_) {lockup_cpu();}
#endif

    bool success = false;

    //if initialized and not empty
    if (in_use_ > 0) {
        read_value = buffer_[read_index_++];

        in_use_--;
        success = true;

        if (read_index_ >= size_) {
            read_index_ -= size_;
        }
    } else {
        read_value = 0;
    }

    //restore the interrupt state
    __restore_interrupts(interrupt_settings);

    return success;
}

__attribute__((ramfunc))
bool ring_buffer::read(const uint16_t count, uint16_t *const array) volatile {
    //disable and store the interrupt state
    const uint16_t interrupt_settings = __disable_interrupts();

#ifdef trap_uninitalized
    if (!initialized_) {lockup_cpu();}
#endif

    bool success = false;

    if (count <= in_use_) {

        const uint16_t count_till_end = size_ - read_index_;

        //if there must be a split
        if (count > count_till_end) {

            //copy till end
            memcpy_fast(array, static_cast<void *>(&buffer_[read_index_]), count_till_end);

            //then copy from beginning
            const uint16_t second_group_count = count - count_till_end;
            memcpy_fast(&array[count_till_end], static_cast<void *>(buffer_), second_group_count);

        } else {

            //otherwise, copy with one operation
            memcpy_fast(array, static_cast<void *>(&buffer_[read_index_]), count);
        }

        //now, update in_use_ and read_index_
        in_use_ -= count;
        read_index_ += count;
        if (read_index_ >= size_) {
            read_index_ -= size_;
        }

        success = true;
    } else {
        //failure
    }

    //restore the interrupt state
    __restore_interrupts(interrupt_settings);

    return success;
}

__attribute__((ramfunc))
bool ring_buffer::write(const uint16_t write_value) volatile {
    //disable and store the interrupt state
    const uint16_t interrupt_settings = __disable_interrupts();

#ifdef trap_uninitalized
    if (!initialized_) {lockup_cpu();}
#endif

    //if full, then increment read_index_ first
    if (in_use_ == size_) {
        read_index_++;
        if (read_index_ >= size_) {
            read_index_ -= size_;
        }
    } else {
        // only increment in_use_ when it wasn't already full
        in_use_++;
    }

    buffer_[write_index_++] = write_value;

    if (write_index_ >= size_) {
        write_index_ -= size_;
    }

    //restore the interrupt state
    __restore_interrupts(interrupt_settings);

    return true;  //always possible to write
}

__attribute__((ramfunc))
bool ring_buffer::write(const uint16_t count, const uint16_t *const array) volatile {
    //disable and store the interrupt state
    const uint16_t interrupt_settings = __disable_interrupts();

#ifdef trap_uninitalized
    if (!initialized_) {lockup_cpu();}
#endif

    bool success = false;

    if (count <= size_) {

        const uint16_t count_till_end = size_ - write_index_;

        //if there must be a split
        if (count > count_till_end) {

            //copy till end
            memcpy_fast(static_cast<void *>(&buffer_[write_index_]), array, count_till_end);

            //then copy from beginning
            const uint16_t second_group_count = count - count_till_end;
            memcpy_fast(static_cast<void *>(buffer_), &array[count_till_end], second_group_count);

        } else {

            //otherwise, copy with one operation
            memcpy_fast(static_cast<void *>(&buffer_[write_index_]),  array, count);
        }

        //now, update the write_index_
        write_index_ += count;
        if (write_index_ >= size_) {
            write_index_ -= size_;
        }

        //increment count and check if full
        in_use_ += count;
        if (in_use_ >= size_) {
            in_use_ = size_;
            read_index_ = write_index_;
        } else {
            //else, if in_use_ is still less than size_, there is no reason to move read_index_
        }

        success = true;
    } else {
        //failure
    }

    //restore the interrupt state
    __restore_interrupts(interrupt_settings);

    return success;
}


void testing_rb(){
    ring_buffer test_rb;
    test_rb.init_alloc(6);
    for (uint16_t value=0; value<2; value++) {
        test_rb.write(value);
    }

//    printf_json_objects(1, json_bool("full", test_rb.is_full()));

    uint16_t something[3];
    something[0] = 123;
    something[1] = 223;
    something[2] = 323;
    test_rb.write(3, something);

    for (uint16_t value=0; value<6; value++) {
        uint16_t read_value;
        if (test_rb.read(read_value)) {
//            printf_json_objects(1, json_uint16("value", read_value));
        } else {
//            printf_json_error("no spots");
        }
    }

//    printf_json_objects(1, json_bool("empty", test_rb.is_empty()));

//    printf_json_status("next");

    for (uint16_t value=0; value<8; value++) {
        test_rb.write(value);
    }

//    printf_json_objects(1, json_bool("full", test_rb.is_full()));

    test_rb.read(3, something);

    for (uint16_t value=0; value<8; value++) {
        uint16_t read_value;
        if (test_rb.read(read_value)) {
//            printf_json_objects(1, json_uint16("value", read_value));
        } else {
//            printf_json_error("no spots");
        }
    }

//    printf_json_objects(1, json_bool("empty", test_rb.is_empty()));
}
