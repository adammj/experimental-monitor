/*
 * cpu_timers.c
 *
 *  Created on: Mar 5, 2017
 *      Author: adam jones
 */

#include "cpu_timers.h"
#include "math.h"
#include "ti_launchpad.h"
#include "printf_json_delayed.h"
#include "tic_toc.h"
#include "misc.h"


//isrs
__interrupt void cpu_timer0_isr(void);
__interrupt void cpu_timer1_isr(void);
__interrupt void cpu_timer2_isr(void);

//global variables
//hold and init the 3 timers
volatile cpu_timer cpu_timer0 = cpu_timer(0);
volatile cpu_timer cpu_timer1 = cpu_timer(1);
volatile cpu_timer cpu_timer2 = cpu_timer(2);


//cannot modify any regs before the startup
cpu_timer::cpu_timer(uint16_t timer_number) {
    timer_number_ = timer_number;

    volatile struct CPUTIMER_REGS *cpu_timer_regs[] = {&CpuTimer0Regs, &CpuTimer1Regs, &CpuTimer2Regs};
    timer_regs_ = cpu_timer_regs[timer_number_];

    //set necessary defaults
    cycle_accumulation_scaled_ = 0;
    initialized_ = false;
    scale_factor_ = 1;
    freq_hz_ = 0.0L;
    callback_function_ = NULL;
    desired_cycles_scaled_ = 1;
    count_ = 0;
    count_i = 1;    //to account for offset
    previous_load_ = 0;
    integer_low_cycles_ = 0;
    integer_high_cycles_ = 0;
}

void cpu_timer::initial_cpu_timer_setup() volatile {
#pragma diag_suppress 1463
    //enable the clock and interrupts (for the prd adjustments)
    EALLOW;
    switch (timer_number_) {
        case 0:
            CpuSysRegs.PCLKCR0.bit.CPUTIMER0 = 1;
            PieVectTable.TIMER0_INT = &cpu_timer0_isr;
            PieCtrlRegs.PIEIER1.bit.INTx7 = 1;
            IER |= M_INT1;
            break;

        case 1:
            CpuSysRegs.PCLKCR0.bit.CPUTIMER1 = 1;
            PieVectTable.TIMER1_INT = &cpu_timer1_isr;
            IER |= M_INT13;
            break;

        case 2:
            CpuSysRegs.PCLKCR0.bit.CPUTIMER2 = 1;
            PieVectTable.TIMER2_INT = &cpu_timer2_isr;
            IER |= M_INT14;
            break;
    }
    EDIS;
#pragma diag_default 1463

    //make sure that the timer is stopped
    timer_regs_->TCR.bit.TSS = 1;

    initialized_ = true;
}

void cpu_timer::set_frequency(float64 timer_freq_hz) volatile {
    //store the frequency in hz
    freq_hz_ = timer_freq_hz;

    //only perform the initial setup once
    if (!initialized_) {
        this->initial_cpu_timer_setup();
    }

    //finally, update the timer calculations
    this->update_cpu_timer_calcs_and_regs();

    delay_printf_json_objects(2, json_string("status", "cpu_timer initialized"), json_float64("freq", freq_hz_, 2));
}

void cpu_timer::set_callback(const timer_callback_t callback_func) volatile {
    callback_function_ = callback_func;
}

void cpu_timer::start() volatile {
    if (!initialized_) {return;}
    count_ = 0;
    timer_regs_->TCR.bit.TSS = 0;
}

void cpu_timer::stop() volatile {
    if (!initialized_) {return;}
    timer_regs_->TCR.bit.TSS = 1;
}

bool cpu_timer::is_running() volatile const {
    if (!initialized_) {return false;}
    return (timer_regs_->TCR.bit.TSS == 0);
}

//clock rate
void cpu_timer::update_cpu_timer_calcs_and_regs() volatile {
    //use 64bit for increase precision when calculating this
    const float64 desired_cpu_cycles = ti_board.get_main_clock_freq() / static_cast<float64>(freq_hz_);
    const bool was_running = this->is_running();  //check if running

    //don't set if negative
    if (desired_cpu_cycles < 0.0L) {
        delay_printf_json_error("cycles calculated as negative");
        return;
    }

    //stop the timer before making any changes
    timer_regs_->TCR.bit.TSS = 1;     // 1 = Stop timer, 0 = Start/Restart Timer

    //reset the accumulation
    cycle_accumulation_scaled_ = 0;
    count_ = 0;
    timing_.reset();


    //scale is used to increase the accuracy of the accumulation check that can be performed
    //with a 64-bit integer, versus a float. Since the integer calculations are 6X+ faster
    //they are preferred inside the interrupt

    //try to find the best scale factor
    uint64_t scale = 1;
    bool zero_error = false;
    for (uint16_t i=0; i<=10; i++) {
        if (i>0) {scale *= 10;}
        const float64 value = desired_cpu_cycles * static_cast<float64>(scale);
        zero_error = (fabsl(value - roundl(value)) < 0.0001L);
        //delay_printf_json_objects(3, json_uint16("i", i), json_float64("value", diff), json_bool("good", good));
        if (zero_error) {break;}
    }

    if (!zero_error) {
        delay_printf_json_error("timer could have long-term errors");
    }

    //set the desired cycles
    scale_factor_ = static_cast<int64_t>(scale);
    desired_cycles_scaled_ = llroundl(desired_cpu_cycles*static_cast<float64>(scale_factor_));

    //integer cycles above and below desired value
    integer_low_cycles_ =  static_cast<uint32_t>(floorl(desired_cpu_cycles));  //rounds down

    //ceill seems to be broken, this formula does work for exact integers (tested)
    if ((desired_cpu_cycles - static_cast<float64>(integer_low_cycles_)) > 0.0L) {
        integer_high_cycles_ = integer_low_cycles_ + 1; //plus one
    } else {
        integer_high_cycles_ = integer_low_cycles_; //same
    }

    //update the regs
    this->update_timer_regs();

    //finally, if it was running, then restart it
    if (was_running) {
        this->start();
    }
}

void cpu_timer::update_timer_regs() volatile {

    //first, stop the timer
    timer_regs_->TCR.bit.TSS = 1;     // 1 = Stop timer, 0 = Start/Restart Timer

    // Counter decrements PRD+1 times each period
    previous_load_ = integer_high_cycles_;
    timer_regs_->PRD.all = previous_load_ - 1;

    // Set pre-scale counter to divide by 1 (SYSCLKOUT):
    timer_regs_->TPR.all = 0;
    timer_regs_->TPRH.all = 0;

    // Initialize timer control register:
    timer_regs_->TCR.bit.TRB = 1;     // 1 = reload timer
    timer_regs_->TCR.bit.SOFT = 0;
    timer_regs_->TCR.bit.FREE = 0;    // Timer Free Run Disabled
    timer_regs_->TCR.bit.TIE = 1;     // 0 = Disable/ 1 = Enable Timer Interrupt
}

float32 cpu_timer::cycle_us() volatile const {
    return timing_.us();
}

float32 cpu_timer::cycle_max_us() volatile const {
    return timing_.max_us();
}

//normally 75 cycles, w/o callback
__attribute__((ramfunc))
void cpu_timer::execute_isr_routine() volatile {
    //restart the clock
    timing_.tic();

    count_++;

    //30 cycles
    //add the actual time passed (cycle_addition) and subtract the last number of cycles used
    cycle_accumulation_scaled_ += (desired_cycles_scaled_ - (static_cast<int64_t>(previous_load_)*scale_factor_));

    //update the count value (this will take effect next time the clock is reloaded)
    if (cycle_accumulation_scaled_ < 0) {
        previous_load_ = integer_low_cycles_;
    } else {
        previous_load_ = integer_high_cycles_;
    }

    //store the updated count to the register that is used to load the timer
    timer_regs_->PRD.all = previous_load_ - 1;

    //then, call the callback
    if (callback_function_ != NULL) {
        callback_function_();
    }

    //finally, stop the timer
    timing_.toc();
}

//isrs
__attribute__((ramfunc))
__interrupt void cpu_timer0_isr(void) {
    cpu_timer0.execute_isr_routine();
    PieCtrlRegs.PIEACK.bit.ACK1 = 1;    //timer0 is in group 1
}

__attribute__((ramfunc))
__interrupt void cpu_timer1_isr(void) {
    cpu_timer1.execute_isr_routine();
    //timer1 does not go through pie, so no PIEACK
}

__attribute__((ramfunc))
__interrupt void cpu_timer2_isr(void) {
    cpu_timer2.execute_isr_routine();
    //timer2 does not go through pie, so no PIEACK
}
