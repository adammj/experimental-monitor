/*
 * internal_adc.c
 *
 *  Created on: Apr 4, 2017
 *      Author: adam
 */

#include "F28x_Project.h"
#include "internal_adc.h"
#include "cla.h"
#include "ti_launchpad.h"
#include "printf_json_delayed.h"
#include "cla_ptr.h"
#include "internal_adc_cla.h"
#include "misc.h"
#include "analog_input_cla.h"
#include "daughterboard.h"
#include "math.h"
#include "tic_toc.h"


//internal variables
//minimum 75ns for 12bit, and 320ns for 16bit (per datasheet)
static const uint16_t ain_sample_window_ns = 80;
static volatile bool internal_adc_initialized = false;
//must have a constant for the number
const uint16_t max_internal_adc_channels = 8;

//cla variables
#pragma DATA_SECTION("cla_data");
uint16_t cla_internal_adc_cycle_count = 0;
#pragma DATA_SECTION("cla_data");
uint16_t cla_internal_adc_conversion_offset = 32768;    //for converting from 12 to 16 bit
#pragma DATA_SECTION("cla_data");
float cla_internal_adc_conversion_scale = 3.90721;      //for converting from 12 to 16 bit
#pragma DATA_SECTION("cla_data");
volatile bool cla_internal_adc_enabled = false;
#pragma DATA_SECTION("cla_data");
volatile_aligned_uint16_t_ptr cla_ain_result_ptrs[max_internal_adc_channels];   //cannot malloc


//init ADC
bool init_internal_adc(const analog_in_ti_t analog_in_settings) {
    if (db_board.is_not_enabled()) {
        delay_printf_json_error("cannot initialize analog in without daughterboard");
        return false;
    }

    if (internal_adc_initialized) {return true;}

    // 12 bit = 16 bit
    // 0      = 32768  (0V)
    // 4095 (max value for 3V) = 48768  (5V, for db input)
    cla_internal_adc_conversion_offset = 32768;
    cla_internal_adc_conversion_scale = 3.90721;

    //determine minimum acquisition window (in SYSCLKS) based on resolution
    //sample window is acqps + 1 SYSCLK cycles
    const uint16_t acqps = static_cast<uint16_t>(lroundl(((ti_board.get_main_clock_freq())*static_cast<float64>(ain_sample_window_ns))/1000000000.0L) + 0.5L);

#pragma diag_suppress 1463
    EALLOW;

    //**** ADC *****

    //first turn on the clocks
    CpuSysRegs.PCLKCR13.bit.ADC_A = 1;
    CpuSysRegs.PCLKCR13.bit.ADC_B = 1;
    DELAY_US(1000);

    //ADCA
    AdcaRegs.ADCCTL2.bit.PRESCALE = 6; //set ADCCLK divider to /4   (gives 50Mhz, which is max per datasheet)
    AdcSetMode(ADC_ADCA, ADC_RESOLUTION_12BIT, ADC_SIGNALMODE_SINGLE);  //sets the correct resolution and trim
    AdcaRegs.ADCCTL1.bit.INTPULSEPOS = 1;   // Set pulse positions to late
    AdcaRegs.ADCCTL1.bit.ADCPWDNZ = 1;  // power up the ADC

    //ADCB
    AdcbRegs.ADCCTL2.bit.PRESCALE = 6; //set ADCCLK divider to /4   (gives 50Mhz, which is max per datasheet)
    AdcSetMode(ADC_ADCB, ADC_RESOLUTION_12BIT, ADC_SIGNALMODE_SINGLE);  //sets the correct resolution and trim
    AdcbRegs.ADCCTL1.bit.INTPULSEPOS = 1;   // Set pulse positions to late
    AdcbRegs.ADCCTL1.bit.ADCPWDNZ = 1;  // power up the ADC

    // delay for 1ms to allow ADC time to power up
    DELAY_US(1000);

//these don't allow the use of actually changing the letter
#ifdef _LAUNCHXL_F28379D
    //initialize the conversions and store the result pointers
    //ADCA SOC0: Analog IN 1
    AdcaRegs.ADCSOC0CTL.bit.CHSEL = analog_in_settings.analog_in_adc_number[0];   //which ADCA
    AdcaRegs.ADCSOC0CTL.bit.ACQPS = acqps - 1;  // sample window
    AdcaRegs.ADCSOC0CTL.bit.TRIGSEL = 1;    // 1 = cpu_timer0 (pg 1230 of manual)
    cla_ain_result_ptrs[0].ptr = reinterpret_cast<volatile uint16_t *>(&AdcaResultRegs.ADCRESULT0);

    //ADCB SOC0: Analog IN 2
    AdcbRegs.ADCSOC0CTL.bit.CHSEL = analog_in_settings.analog_in_adc_number[1];   //which ADCB
    AdcbRegs.ADCSOC0CTL.bit.ACQPS = acqps - 1;  // sample window
    AdcbRegs.ADCSOC0CTL.bit.TRIGSEL = 1;    // 1 = cpu_timer0 (pg 1230 of manual)
    cla_ain_result_ptrs[1].ptr = reinterpret_cast<volatile uint16_t *>(&AdcbResultRegs.ADCRESULT0);

    //ADCB SOC1: Analog IN 3
    AdcbRegs.ADCSOC1CTL.bit.CHSEL = analog_in_settings.analog_in_adc_number[2];   //which ADCB
    AdcbRegs.ADCSOC1CTL.bit.ACQPS = acqps - 1;  // sample window
    AdcbRegs.ADCSOC1CTL.bit.TRIGSEL = 1;    // 1 = cpu_timer0 (pg 1230 of manual)
    cla_ain_result_ptrs[2].ptr = reinterpret_cast<volatile uint16_t *>(&AdcbResultRegs.ADCRESULT1);

    //ADCA SOC1: Analog IN 4
    AdcaRegs.ADCSOC1CTL.bit.CHSEL = analog_in_settings.analog_in_adc_number[3];   //which ADCA
    AdcaRegs.ADCSOC1CTL.bit.ACQPS = acqps - 1;  // sample window
    AdcaRegs.ADCSOC1CTL.bit.TRIGSEL = 1;    // 1 = cpu_timer0 (pg 1230 of manual)
    cla_ain_result_ptrs[3].ptr = reinterpret_cast<volatile uint16_t *>(&AdcaResultRegs.ADCRESULT1);

    //set end of conversion interrupt
    AdcbRegs.ADCINTSEL1N2.bit.INT1SEL = 1;  // end of ADCB.SOC1 will set INT1 flag
    AdcbRegs.ADCINTSEL1N2.bit.INT1E = 1;    // enable INT1 flag
    AdcbRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;  // make sure INT1 flag is cleared

#else
    //initialize the conversions and store the result pointers
    //ADCA SOC0: Analog IN 1
    AdcaRegs.ADCSOC0CTL.bit.CHSEL = analog_in_settings.analog_in_adc_number[0];   //which ADCA
    AdcaRegs.ADCSOC0CTL.bit.ACQPS = acqps - 1;  // sample window
    AdcaRegs.ADCSOC0CTL.bit.TRIGSEL = 1;    // 1 = cpu_timer0 (pg 1230 of manual)
    cla_ain_result_ptrs[0].ptr = &AdcaResultRegs.ADCRESULT0;

    //ADCB SOC0: Analog IN 2
    AdcbRegs.ADCSOC0CTL.bit.CHSEL = analog_in_settings.analog_in_adc_number[1];   //which ADCB
    AdcbRegs.ADCSOC0CTL.bit.ACQPS = acqps - 1;  // sample window
    AdcbRegs.ADCSOC0CTL.bit.TRIGSEL = 1;    // 1 = cpu_timer0 (pg 1230 of manual)
    cla_ain_result_ptrs[1].ptr = &AdcbResultRegs.ADCRESULT0;

    //ADCB SOC1: Analog IN 3
    AdcbRegs.ADCSOC1CTL.bit.CHSEL = analog_in_settings.analog_in_adc_number[2];   //which ADCB
    AdcbRegs.ADCSOC1CTL.bit.ACQPS = acqps - 1;  // sample window
    AdcbRegs.ADCSOC1CTL.bit.TRIGSEL = 1;    // 1 = cpu_timer0 (pg 1230 of manual)
    cla_ain_result_ptrs[2].ptr = &AdcbResultRegs.ADCRESULT1;

    //ADCB SOC2: Analog IN 4
    AdcbRegs.ADCSOC2CTL.bit.CHSEL = analog_in_settings.analog_in_adc_number[3];   //which ADCB
    AdcbRegs.ADCSOC2CTL.bit.ACQPS = acqps - 1;  // sample window
    AdcbRegs.ADCSOC2CTL.bit.TRIGSEL = 1;    // 1 = cpu_timer0 (pg 1230 of manual)
    cla_ain_result_ptrs[3].ptr = &AdcbResultRegs.ADCRESULT2;

    //set end of conversion interrupt
    AdcbRegs.ADCINTSEL1N2.bit.INT1SEL = 2;  // end of ADCB.SOC2 will set INT1 flag
    AdcbRegs.ADCINTSEL1N2.bit.INT1E = 1;    // enable INT1 flag
    AdcbRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;  // make sure INT1 flag is cleared

#endif
    EDIS;

#pragma diag_default 1463

    //enable the cla task
    enable_cla_task(3, reinterpret_cast<uint16_t>(&cla_task3_process_analog_io), CLA_TRIG_ADCBINT1);

    cla_internal_adc_enabled = true;
    internal_adc_initialized = true;
    delay_printf_json_status("initialized internal analog in");

    return true;
}

float32 internal_adc_cla_task_length_in_us() {
    return cla_cycles_in_us(cla_internal_adc_cycle_count);
}
