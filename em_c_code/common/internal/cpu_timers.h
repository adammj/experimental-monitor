/*
 * cpu_timers.h
 *
 *  Created on: Mar 5, 2017
 *      Author: adam jones
 */

// uses 64-bit integers to keep track of the error between the desired and actual count
// which should maintain accuracy indefinitely (depending on how well the calibration was)

#ifndef cpu_timers_defined
#define cpu_timers_defined

#include "stdint.h"
#include <stdbool.h>
#include "F28x_Project.h"
#include "ring_buffer.h"
#include "tic_toc.h"


typedef void (*timer_callback_t) (void);

class cpu_timer {
    public:
        explicit cpu_timer(uint16_t timer_number);
        void set_frequency(float64 timer_freq_hz) volatile;
        void set_callback(const timer_callback_t callback_func) volatile;
        void start() volatile;
        void stop() volatile;

        bool is_running() volatile const;
        float64 freq_hz() volatile const {return freq_hz_;}
        float32 cycle_us() volatile const;
        float32 cycle_max_us() volatile const;
        uint64_t count() volatile const {return count_;}

        //this must be public for the isr to work
        void execute_isr_routine() volatile;

    private:
        //parameters
        bool initialized_;
        uint16_t timer_number_;
        uint32_t previous_load_;
        uint32_t integer_low_cycles_;
        uint32_t integer_high_cycles_;
        float64 freq_hz_;
        uint64_t count_;
        int64_t scale_factor_;
        int64_t cycle_accumulation_scaled_;
        int64_t desired_cycles_scaled_;
        volatile timer_callback_t callback_function_;
        volatile struct CPUTIMER_REGS *timer_regs_;

        uint32_t counts[10];
        uint16_t count_i;

        //timing info
        tic_toc timing_;

        //methods
        void update_cpu_timer_calcs_and_regs() volatile;
        void update_timer_regs() volatile;
        void initial_cpu_timer_setup() volatile;
};

//the 3 timers, pre-made inside the cpp file
extern volatile cpu_timer cpu_timer0;
extern volatile cpu_timer cpu_timer1;
extern volatile cpu_timer cpu_timer2;

#endif
