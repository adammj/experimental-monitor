/*
 * main_cpu2.c
 *
 *  Created on: Mar 2, 2017
 *      Author: adam jones
 */


#include <db_settings.h>
#include <dsp_output.h>
#include <printf_json.h>
#include <serial_link.h>
#include "scia.h"
#include "cpu_timers.h"
#include "leds.h"
#include "cla.h"
#include "experiment.h"
#include "digital_io.h"
#include "ring_buffer.h"
#include "tic_toc.h"
#include "F28x_Project.h"
#include "misc.h"
#include "ti_misc.h"
#include "external_adc.h"
#include "experiment_setup.h"


//main function
void main() {

    //initialize system, and PIE vector table
    InitSysCtrl();      //need to make this more universal, or just leave alone
    InitPieVectTable(); //needs struct def


    //initializes the basics (clocks, timers, scia, leds, tic_toc, pullups)
    init_experimental_monitor();


    //initialize additional functions
    init_cla();             //main cla tasks triggered by cpu_timer0
    //init_digital_io();
    //init_external_adc(false);    //runs within main cla task, and has one triggered by spia_rx
    //init_dsp_output();      //runs within main cla task
    //init_experiment();      //runs within main cla task


    //enable global interrupts and higher priority real-time debug events:
    EINT;   // Enable Global interrupt INTM
    ERTM;   // Enable Global realtime interrupt DBGM


    //start timers after interrupts are enabled
    cpu_timer_start(0);
    cpu_timer_start(1);
    enable_watchdog();


    //feedback to user that it has started successfully
    print_welcome_message();


    //start main loop
    while (1) {
        kick_watchdog();
        process_scia_buffer_for_commands();
    }
}
