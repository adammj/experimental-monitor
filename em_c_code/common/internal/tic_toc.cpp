/*
 * tic_toc.c
 *
 *  Created on: Apr 19, 2017
 *      Author: adam jones
 */

#include "tic_toc.h"
#include "daughterboard.h"
#include "ti_launchpad.h"
#include "cla.h"
#include "tic_toc_cla.h"
#include "ring_buffer.h"
#include "math.h"
#include "printf_json_delayed.h"
#include "extract_json.h"
#include "cpu_timers.h"
#include "scia.h"
#include "serial_link.h"


serial_command_t command_calibrate = {"calibrate_clock_with_sync", true, 0, calibrate_with_sync, NULL, NULL};


//global variables
//constants
float32 cla_us_per_div;
float32 cpu_us_per_div;
float32 cpu_hs_us_per_div;
float64 sync_us_per_div;
float32 cpu_ts_tics_us_per_div;
volatile debug_ts_t debug_timestamps;


//estimates assume 200 MHz clock

//p62 of datasheet says EPWM max freq is 100 MHz
//so it must be divided by 2
static const uint16_t EPWMCLKDIV = 1;      //1 = div of 2

//cla =  0.1 us/tic    max 6.55 ms
static const uint16_t cla_CLKDIV = 0;      //0 = div of 1
static const uint16_t cla_HSPCLKDIV = 5;   //5 = div of 10

//cpu =  17.92 us/tic    max 1.17 s
static const uint16_t cpu_CLKDIV = 7;      //7 = div of 128
static const uint16_t cpu_HSPCLKDIV = 7;   //7 = div of 14

//cpu_hs =  0.1 us/tic    max 6.55 ms
static const uint16_t cpu_hs_CLKDIV = 0;       //0 = div of 1
static const uint16_t cpu_hs_HSPCLKDIV = 5;    //5 = div of 10

//sync = 0.010 us/tic   max 655 us
static const uint16_t sync_CLKDIV = 0;         //0 = div of 1
static const uint16_t sync_HSPCLKDIV = 0;      //0 = div of 1

//internal function
float32 calc_us_per_div(uint16_t EPWMCLKDIV_, uint16_t CLKDIV_, uint16_t HSPCLKDIV_);


#pragma diag_suppress 1463

void init_tic_toc(void) {

    EALLOW;

    //enable the clocks
    CpuSysRegs.PCLKCR2.bit.EPWM1 = 1;
    CpuSysRegs.PCLKCR2.bit.EPWM2 = 1;
    CpuSysRegs.PCLKCR2.bit.EPWM3 = 1;
    CpuSysRegs.PCLKCR2.bit.EPWM4 = 1;
    CpuSysRegs.PCLKCR2.bit.EPWM5 = 1;
    CpuSysRegs.PCLKCR2.bit.EPWM6 = 1;
    CpuSysRegs.PCLKCR2.bit.EPWM7 = 1;

    //disable sync
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 0;

    //0 = CLK, 1 = CLK/2
    ClkCfgRegs.PERCLKDIVSEL.bit.EPWMCLKDIV = EPWMCLKDIV;

    //CLA timing
    CLA_EPWM.TBCTL.bit.CTRMODE = TB_COUNT_UP;
    CLA_EPWM.TBPRD = 0xFFFF;
    CLA_EPWM.TBCTL.bit.PHSEN = TB_DISABLE;
    CLA_EPWM.TBPHS.bit.TBPHS = 0x0000;
    CLA_EPWM.TBCTR = 0x0000;
    CLA_EPWM.TBCTL.bit.CLKDIV = cla_CLKDIV;
    CLA_EPWM.TBCTL.bit.HSPCLKDIV = cla_HSPCLKDIV;

    //CPU timing
    CPU_EPWM.TBCTL.bit.CTRMODE = TB_COUNT_UP;
    CPU_EPWM.TBPRD = 0xFFFF;
    CPU_EPWM.TBCTL.bit.PHSEN = TB_DISABLE;
    CPU_EPWM.TBPHS.bit.TBPHS = 0x0000;
    CPU_EPWM.TBCTR = 0x0000;
    CPU_EPWM.TBCTL.bit.CLKDIV = cpu_CLKDIV;
    CPU_EPWM.TBCTL.bit.HSPCLKDIV = cpu_HSPCLKDIV;

    //CPU_HS timing
    CPU_HS_EPWM.TBCTL.bit.CTRMODE = TB_COUNT_UP;
    CPU_HS_EPWM.TBPRD = 0xFFFF;
    CPU_HS_EPWM.TBCTL.bit.PHSEN = TB_DISABLE;
    CPU_HS_EPWM.TBPHS.bit.TBPHS = 0x0000;
    CPU_HS_EPWM.TBCTR = 0x0000;
    CPU_HS_EPWM.TBCTL.bit.CLKDIV = cpu_hs_CLKDIV;
    CPU_HS_EPWM.TBCTL.bit.HSPCLKDIV = cpu_hs_HSPCLKDIV;

    //timer0 timing (same as hs)
    TIMER0_EPWM.TBCTL.bit.CTRMODE = TB_COUNT_UP;
    TIMER0_EPWM.TBPRD = 0xFFFF;
    TIMER0_EPWM.TBCTL.bit.PHSEN = TB_DISABLE;
    TIMER0_EPWM.TBPHS.bit.TBPHS = 0x0000;
    TIMER0_EPWM.TBCTR = 0x0000;
    TIMER0_EPWM.TBCTL.bit.CLKDIV = cpu_hs_CLKDIV;
    TIMER0_EPWM.TBCTL.bit.HSPCLKDIV = cpu_hs_HSPCLKDIV;

    //timer1 timing (same as hs)
    TIMER1_EPWM.TBCTL.bit.CTRMODE = TB_COUNT_UP;
    TIMER1_EPWM.TBPRD = 0xFFFF;
    TIMER1_EPWM.TBCTL.bit.PHSEN = TB_DISABLE;
    TIMER1_EPWM.TBPHS.bit.TBPHS = 0x0000;
    TIMER1_EPWM.TBCTR = 0x0000;
    TIMER1_EPWM.TBCTL.bit.CLKDIV = cpu_hs_CLKDIV;
    TIMER1_EPWM.TBCTL.bit.HSPCLKDIV = cpu_hs_HSPCLKDIV;

    //timer2 timing (same as hs)
    TIMER2_EPWM.TBCTL.bit.CTRMODE = TB_COUNT_UP;
    TIMER2_EPWM.TBPRD = 0xFFFF;
    TIMER2_EPWM.TBCTL.bit.PHSEN = TB_DISABLE;
    TIMER2_EPWM.TBPHS.bit.TBPHS = 0x0000;
    TIMER2_EPWM.TBCTR = 0x0000;
    TIMER2_EPWM.TBCTL.bit.CLKDIV = cpu_hs_CLKDIV;
    TIMER2_EPWM.TBCTL.bit.HSPCLKDIV = cpu_hs_HSPCLKDIV;

    //sync timing
    SYNC_EPWM.TBCTL.bit.CTRMODE = TB_COUNT_UP;
    SYNC_EPWM.TBPRD = 0xFFFF;
    SYNC_EPWM.TBCTL.bit.PHSEN = TB_DISABLE;
    SYNC_EPWM.TBPHS.bit.TBPHS = 0x0000;
    SYNC_EPWM.TBCTR = 0x0000;
    SYNC_EPWM.TBCTL.bit.CLKDIV = sync_CLKDIV;
    SYNC_EPWM.TBCTL.bit.HSPCLKDIV = sync_HSPCLKDIV;

    //enable sync
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 1;
    EDIS;


#pragma diag_suppress 64

    cla_us_per_div = calc_us_per_div(EPWMCLKDIV, cla_CLKDIV, cla_HSPCLKDIV);
    cpu_us_per_div = calc_us_per_div(EPWMCLKDIV, cpu_CLKDIV, cpu_HSPCLKDIV);
    cpu_hs_us_per_div = calc_us_per_div(EPWMCLKDIV, cpu_hs_CLKDIV, cpu_hs_HSPCLKDIV);
    sync_us_per_div = calc_us_per_div(EPWMCLKDIV, sync_CLKDIV, sync_HSPCLKDIV);

    cpu_ts_tics_us_per_div = static_cast<float32>(1000000.0L/(ti_board.get_main_clock_freq()));

#pragma diag_default 64

    add_serial_command(&command_calibrate);
}

float32 calc_us_per_div(uint16_t EPWMCLKDIV_, uint16_t CLKDIV_, uint16_t HSPCLKDIV_) {
    float64 div0;
    float64 div1;
    float64 div2;

    //div0
    div0 = static_cast<float64>(EPWMCLKDIV_ + 1);
    div0 *= 1000000.0L/(ti_board.get_main_clock_freq()); //convert to microseconds

    //div1
    if (CLKDIV_ > 0) {
        div1 = static_cast<float64>(2 << (CLKDIV_ - 1));   //2^CLKDIV
    } else {
        div1 = 1.0L;
    }

    //div2
    div2 = static_cast<float64>(2 * HSPCLKDIV_);            //2*HSPCLKDIV
    if (HSPCLKDIV_ == 0) {div2 = 1.0L;}

    return static_cast<float32>(div0*div1*div2);
}


// **** SYNC variables and functions ****

//new version using cpu
volatile uint32_t sync_counts;
volatile uint32_t sync_counts_needed;
volatile uint32_t total_sync_tics;

//internal variables
const uint16_t calibration_steps = 6;
volatile uint32_t total_tics_per_step[calibration_steps];
volatile uint16_t calibration_step;
volatile uint32_t target_calibration_tics;
volatile float64 seconds_per_step;

void calculate_ppb_deviation() {

    //turn off both
    ti_board.red_led.off();
    ti_board.blue_led.off();

    //get the average
    float64 average_value = 0;
    for (uint16_t i=0; i<calibration_steps; i++) {
        average_value += static_cast<float64>(total_tics_per_step[i]);
    }
    average_value /= static_cast<float64>(calibration_steps);

    //get the stdev
    float64 stdev_value = 0;
    for (uint16_t i=0; i<calibration_steps; i++) {
        const float64 current_value = static_cast<float64>(total_tics_per_step[i]);
        stdev_value += (current_value - average_value)*(current_value - average_value);
    }
    stdev_value /= static_cast<float64>(calibration_steps - 1); //divide by (n-1) for sample variance
    stdev_value = static_cast<float64>(sqrtf(static_cast<float32>(stdev_value)));   //don't need super precise sqrt here

    const int32_t ppb_deviation = lroundl(roundl(average_value) - static_cast<float64>(target_calibration_tics));   //round to nearest
    delay_printf_json_objects(3, json_string("status", "calibration done"), json_int32("clock_ppb_deviation", ppb_deviation), json_float64("stdev", stdev_value, 2));
    delay_printf_json_status("device will need to be rebooted for any experiments");
}


__attribute__((ramfunc))
__interrupt void sync_isr() {

    //make sure to grab the counter at the beginning (and try to prevent any compiler optimization)
    //if using 79D use the actual internal counter
#ifdef _LAUNCHXL_F28379D
    static uint32_t last_sync_tic = 0;
    const volatile uint32_t current_sync_tic = IpcRegs.IPCCOUNTERL;    //1 cycle resolution, 5ns
    const volatile uint32_t delta_sync_tics = current_sync_tic - last_sync_tic;
#else
    //otherwise use the sync epwm
    static uint16_t last_sync_tic = 0;
    const volatile uint16_t current_sync_tic = SYNC_EPWM.TBCTR;        //2 cycle resolution, 10ns
    volatile uint32_t delta_sync_tics = static_cast<uint16_t>(current_sync_tic - last_sync_tic);
    delta_sync_tics *= 2; //have to account for 2 cycles
#endif

    //code copied from cla task
    if (sync_counts < sync_counts_needed) {

        last_sync_tic = current_sync_tic;

        //if not the first tic, add to total
        if (sync_counts != 0) {
            total_sync_tics += delta_sync_tics;
        }

        sync_counts++;
    }

    if (sync_counts == sync_counts_needed) {
        if (calibration_step < calibration_steps) {
            total_tics_per_step[calibration_step] = total_sync_tics;

            calibration_step++;

            //for all but the last one, reset the counts
            if (calibration_step < calibration_steps) {
                //reset tics and counts
                total_sync_tics = 0;
                sync_counts = 0;
                ti_board.red_led.toggle();
                ti_board.blue_led.toggle();

            } else {
                //disable the external interrupt
                EALLOW;
                XintRegs.XINT1CR.bit.ENABLE = false;
                PieCtrlRegs.PIEIER1.bit.INTx4 = 0;  //disable PIE 1.4 (XINT1)
                EDIS;

                //now, calculate
                calculate_ppb_deviation();
            }
        }
    }

    //XINT1 is in group 1
    PieCtrlRegs.PIEACK.bit.ACK1 = 1;
}

void calibrate_with_sync(const json_t *const json_root) {
    if (db_board.is_not_enabled()) {return;}

    json_element source_frequency_e("source_frequency", t_uint16, true);

    if (!source_frequency_e.set_with_json(json_root, true)) {
        return;
    }

    const float64 source_frequency = static_cast<float64>(source_frequency_e.value().uint16_);

    if ((source_frequency < 4000.0L) || (source_frequency > 40000.0L)) {
        delay_printf_json_error("'source_frequency' outside allowed values: 4000-40000 Hz");
        return;
    }

    //disable the timers first
    if (cpu_timer0.is_running()) {
        delay_printf_json_status("stopping timer0");
        cpu_timer0.stop();
    }
    if (cpu_timer1.is_running()) {
        delay_printf_json_status("stopping timer1");
        cpu_timer1.stop();
    }
    if (cpu_timer2.is_running()) {
        delay_printf_json_status("stopping timer2");
        cpu_timer2.stop();
    }

    //defaults to digital in 0 of the current daughterboard
    const digital_io_settings_t digital_io = db_board.get_digital_io_settings_t();
    const uint16_t gpio_pin_num = digital_io.digital_in_gpio[0];

    //target exactly 1 billion to get ppb
    target_calibration_tics = 1000000000;

    //calculates the nearest number of seconds
    seconds_per_step = static_cast<float64>(target_calibration_tics) / static_cast<float64>(ti_board.get_unscaled_clock_freq());

    //1 extra to account for the first pass (setting the last_sync_tic value)
    sync_counts_needed = static_cast<uint32_t>(lroundl(seconds_per_step * source_frequency) + 1);

    //reset intermediate values
    sync_counts = 0;
    total_sync_tics = 0;
    calibration_step = 0;

    EALLOW;
    //setup the input interrupt
    InputXbarRegs.INPUT4SELECT = gpio_pin_num;  //XINT1 = INPUT4
    XintRegs.XINT1CR.bit.POLARITY = true;   //true = positive edge

    PieVectTable.XINT1_INT = sync_isr;  //set the interrupt vector
    PieCtrlRegs.PIEIER1.bit.INTx4 = 1;  //enable PIE 1.4 (XINT1)
    IER |= M_INT1;  //make sure group 1 is enabled

    EDIS;

    //let the user know
    delay_printf_json_objects(4, json_parent("calibration", 3), json_float64("source_frequency(Hz)", source_frequency, 2), json_uint16("steps", calibration_steps), json_float64("sec/step", seconds_per_step, 2));
    delay_printf_json_status("leds will twiddle while calibration is working");

    //print everything, and clear the tx buffer before continuing
    printf_delayed_json_objects(true);
    wait_until_scia_tx_buffer_is_empty();

    ti_board.red_led.off();
    ti_board.blue_led.on();

    //finally, enable the interrupt
    EALLOW;
    XintRegs.XINT1CR.bit.ENABLE = true;
    EDIS;
}

__attribute__((ramfunc))
void cpu_hs_tic(void) {
    EALLOW;
    CPU_HS_EPWM.TBCTL.bit.CTRMODE = TB_FREEZE;
    CPU_HS_EPWM.TBCTR = 0;
    CPU_HS_EPWM.TBCTL.bit.CTRMODE = TB_COUNT_UP;
    EDIS;
}

__attribute__((ramfunc))
uint16_t cpu_hs_toc(void) {
    EALLOW;
    CPU_HS_EPWM.TBCTL.bit.CTRMODE = TB_FREEZE;
    const uint16_t cycles = CPU_HS_EPWM.TBCTR;
    EDIS;
    return cycles;
}

float32 cla_cycles_in_us(uint16_t cycles) {
    return cla_us_per_div*static_cast<float32>(cycles);
}
float32 cpu_cycles_in_us(uint16_t cycles) {
    return cpu_us_per_div*static_cast<float32>(cycles);
}
float32 cpu_hs_cycles_in_us(uint16_t cycles) {
    return cpu_hs_us_per_div*static_cast<float32>(cycles);
}
float32 cpu_ts_tics_in_us(uint32_t ts_tics) {
    return cpu_ts_tics_us_per_div*static_cast<float32>(ts_tics);
}


tic_toc::tic_toc() {
    last_tic_ = 0;
    tics_ = 0;
    max_tics_ = 0;
}
__attribute__((ramfunc))
void tic_toc::tic() volatile {
#ifdef _LAUNCHXL_F28379D
    last_tic_ = CPU_TIMESTAMP;
#else
    cpu_hs_tic();
#endif
}

__attribute__((ramfunc))
void tic_toc::toc() volatile {
#ifdef _LAUNCHXL_F28379D
    //update_cpu_cycles(last_tic_, tics_, max_tics_);
    tics_ = CPU_TIMESTAMP - last_tic_;

    if (tics_ > max_tics_) {
        max_tics_ = tics_;
    }
#else
    tics_ = static_cast<uint32_t>(cpu_hs_toc());
    if (tics_ > max_tics_) {
        max_tics_ = tics_;
    }
#endif
}

__attribute__((ramfunc))
void tic_toc::reset() volatile {
    last_tic_ = 0;
    tics_ = 0;
    max_tics_ = 0;
}

__attribute__((ramfunc))
float32 tic_toc::us() volatile const {
#ifdef _LAUNCHXL_F28379D
    return cpu_ts_tics_us_per_div * static_cast<float32>(tics_);
#else
    return cpu_hs_us_per_div * static_cast<float32>(tics_);
#endif
}

__attribute__((ramfunc))
float32 tic_toc::max_us() volatile const {
#ifdef _LAUNCHXL_F28379D
    return cpu_ts_tics_us_per_div * static_cast<float32>(max_tics_);
#else
    return cpu_hs_us_per_div * static_cast<float32>(max_tics_);
#endif
}

#pragma diag_default 1463
