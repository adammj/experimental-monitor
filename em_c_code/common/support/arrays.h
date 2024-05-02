/*
 * arrays.h
 *
 *  Created on: May 3, 2018
 *      Author: adam jones
 */

#ifndef arrays_defined
#define arrays_defined

#include "misc.h"

//ignore all warnings for template
#pragma diag_suppress 1463

typedef struct heap_t {
    uint32_t bytes;
    uint16_t count;
    const char *variable;
} heap_t;


//thread-safe create and delete array of any type
template<typename T>
__attribute__((ramfunc))
T* create_array_of(uint16_t count, const char *const variable_name) {

    //before checking free or allocating, disable interrupts
    const uint16_t interrupt_settings = __disable_interrupts();
    T *ptr = NULL;

    //bytes are in terms of 16-bit "bytes" (the smallest unit)
    const uint32_t max_bytes_free = get_max_heap_free();
    const uint32_t necessary_bytes = (static_cast<uint32_t>(count)*static_cast<uint32_t>(sizeof(T))) + 2;   //2 "bytes" of padding for heap pointers

    if (necessary_bytes <= max_bytes_free) {
        //allocate
        ptr = new T[count];

        //restore after allocating
        __restore_interrupts(interrupt_settings);

        //check pointer
        if (ptr == NULL) {
            //give error if null
            error_allocating_messages(variable_name);
            lockup_cpu();
        }

    } else {
        //else, there was not enough free memory

        //restore after finding out there isn't enough space
        __restore_interrupts(interrupt_settings);

        //get error
        not_enough_free_space(variable_name, static_cast<uint16_t>(necessary_bytes), static_cast<uint16_t>(max_bytes_free));
    }

    return ptr;
}

template<typename T>
__attribute__((ramfunc))
void delete_array(const T* const ptr) {
    if (ptr == NULL) {return;}  //just return if it points to NULL

    const uint16_t interrupt_settings = __disable_interrupts();

    delete[] ptr;
    get_max_heap_free();   //update the max_bytes_free

    __restore_interrupts(interrupt_settings);
}

#pragma diag_default 1463

#endif
