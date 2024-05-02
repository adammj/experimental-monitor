/*
 * misc.cpp
 *
 *  Created on: Mar 20, 2017
 *      Author: adam jones
 */

#include "misc.h"
#include "stddef.h"
#include "printf_json.h"        //just for init and major errors
#include "printf_json_delayed.h"
#include "fpu_vector.h"
#include "tic_toc.h"
#include "scia.h"
#include "stdlibf.h"
#include "stdlib.h"
#include "arrays.h"
#ifdef _LAUNCHXL_F28379D
#include "F2837xD_Gpio_defines.h"
#else
#include "F2837xS_Gpio_defines.h"
#endif

//internal variables
static volatile bool max_byte_free_initialized = false;
static volatile uint32_t max_heap_bytes_free = 0;
static volatile uint16_t *stack_end = reinterpret_cast<uint16_t *>(0x0800 - 1);
static volatile uint16_t max_stack_bytes_free = 0;

uint16_t get_max_stack_free() {
    volatile uint16_t *ptr = stack_end;

    while (*ptr == 0) {
        ptr--;
    }

    max_stack_bytes_free = stack_end - ptr;
    return max_stack_bytes_free;
}

//way too huge to put in ramfunc
void performance_test() {


    //the memcpy_fast is supposed to take 1cycle per count  (@ 200,000,000 Hz, that is 5ns)
    //currently, this seems to hold true (for the minimum time) for lengths of 1000-6500
    //at 7000 and above, this progressively slows down to a minimum of ~5ns/count
    const uint16_t loops = 10;
    const uint16_t iterations = 10;
    uint16_t lengths[iterations] = {16,32,64,128,256,512,1024,2048,4096,8192};
    float32  max_allowed_times[iterations] = {1, 1, 1, 1, 2, 3, 6, 12, 23, 48};
    uint64_t failures = 0;
    float32 sum_times[iterations] = {0,0,0,0,0,0,0,0,0,0};
    float32 max_times[iterations] = {0,0,0,0,0,0,0,0,0,0};
    float32 min_times[iterations] = {9999,9999,9999,9999,9999,9999,9999,9999,9999,9999};

    printf_json_status("running internal performance test");
    wait_until_scia_tx_buffer_is_empty();   //to make sure the notice gets printed

    //disable the interrupts because there are many new/delete combos
    const uint16_t interrupt_settings = __disable_interrupts();

    for (uint16_t j=0; j<loops; j++) {
        for (uint16_t i=0; i<iterations; i++) {
            uint16_t *const destination_array = create_array_of<uint16_t>(lengths[i], "destination_array");
            const uint16_t *const source_array = create_array_of<uint16_t>(lengths[i], "source_array");

            if ((destination_array != NULL) && (source_array != NULL)) {

                //just do the memcpy inside the timing
                //maybe this should be using the rb copy of this? dunno

#ifdef _LAUNCHXL_F28379D
                const uint32_t start_ts = CPU_TIMESTAMP;
#else
                cpu_hs_tic();
#endif

                memcpy_fast(destination_array, &source_array, lengths[i]);

#ifdef _LAUNCHXL_F28379D
                const uint32_t delta_ts = CPU_TIMESTAMP - start_ts;
                const float32 current_time = cpu_ts_tics_in_us(delta_ts);
#else
                const uint16_t current_tic = cpu_hs_toc();
                const float32 current_time = cpu_hs_cycles_in_us(current_tic);
#endif

                //sum and check the max
                sum_times[i] += current_time;
                if (current_time > max_times[i]) {
                    max_times[i] = current_time;
                }
                if (current_time < min_times[i]) {
                    min_times[i]= current_time;
                }
                if (current_time > max_allowed_times[i]) {
                    failures++;
                }
            } else {
                failures++;
            }

            delete_array(destination_array);
            delete_array(source_array);
        }
    }

    //restore interrupts
    __restore_interrupts(interrupt_settings);

    //get average
    for (uint16_t i = 0; i<iterations; i++) {
        sum_times[i] /= static_cast<float32>(loops);
    }

    //if more than 50% of the iterations fail
    if (((failures*100)/static_cast<uint64_t>(iterations*loops)) > 50) {
        printf_json_objects(3, json_string("error", "performance test failure"), json_uint64("trials", static_cast<uint64_t>(iterations*loops)), json_uint64("failures", failures));

        for (uint16_t i = 0; i<iterations; i++) {
            printf_json_objects(5, json_uint16("length", lengths[i]),
                                json_float32("min_timing_us", min_times[i], 2),
                                json_float32("avg_timing_us", sum_times[i], 2),
                                json_float32("max_timing_us", max_times[i], 2),
                                json_float32("max_allowed_us", max_allowed_times[i], 2));
        }

        lockup_cpu();
    } else {
        printf_json_objects(3, json_string("status", "performance test passed"),
                            json_uint64("trials", static_cast<uint64_t>(iterations*loops)),
                            json_uint64("failures", failures));
    }

    return;
}


void lockup_cpu() {
    printf_delayed_json_objects(true);
    DELAY_US(500000);   //wait 0.5 sec to print any messages
#pragma diag_suppress 1463
    asm ("      ESTOP0");
#pragma diag_default 1463
    for (;;) {
    }
}


__attribute__((ramfunc))
uint32_t get_max_heap_free() {
    const uint16_t interrupt_settings = __disable_interrupts();

    if (!max_byte_free_initialized) {
        //for some reason, something must be created on the heap before running max_free
        const volatile uint16_t *const junk = new uint16_t[2];
        delete[] junk;
        max_byte_free_initialized = true;
    }

    //get the value using TI function (this does work, as the initial value matches the heap allocation)
    max_heap_bytes_free = __TI_heap_total_available();

    //restore the interrupts and return
    __restore_interrupts(interrupt_settings);
    return max_heap_bytes_free;
}

void error_allocating_messages(const char *const variable_name) {
    printf_json_objects(2, json_string("error", "error allocating"), json_string("variable", variable_name));
}
void not_enough_free_space(const char *const variable_name, uint16_t needed, uint16_t available) {
    printf_json_objects(4, json_string("error", "not enough free memory to allocate"),
                        json_string("variable", variable_name),
                        json_uint16("needed", needed),
                        json_uint16("available", available));
}

uint32_t djb2_hash(const char *const string) {

    uint32_t hash = 5381;   //magic number

    for (char *c = const_cast<char*>(string); (*c) != 0; c++) {
        //hash = ((hash << 5) + hash) + c;    // hash * 33 + c
        hash = (hash * static_cast<uint32_t>(33)) ^ static_cast<uint32_t>(*c);   //alternate with xor
    }

    return hash;
}

bool is_power_of_two(uint64_t value) {
    return ((value > 0) && ((value & (value - 1)) == 0));
}


//simplified write_pin function
__attribute__((ramfunc))
void gpio_write_pin(volatile uint32_t *const pin_data_reg, uint32_t pin_mask, bool set_pin) {

    if (set_pin) {
        pin_data_reg[GPYSET] = pin_mask;
    } else {
        pin_data_reg[GPYCLEAR] = pin_mask;
    }
}

__attribute__((ramfunc))
bool gpio_read_pin(const volatile uint32_t *const pin_data_reg, uint16_t pin_shift) {

    return (pin_data_reg[GPYDAT] >> pin_shift) & 0x1;
}

volatile uint32_t *calculate_pin_data_reg(uint16_t gpio_number) {
    return (reinterpret_cast<volatile uint32_t *>(&GpioDataRegs) + ((gpio_number/32)*GPY_DATA_OFFSET));
}

uint32_t calculate_pin_mask(uint16_t gpio_number) {
    return (1UL << (gpio_number % 32));
}
