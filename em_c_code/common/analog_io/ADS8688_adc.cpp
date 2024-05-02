/*
 *  ADS8688_adc.c
 *
 *  Created on: May 23, 2017
 *      Author: adam jones
 */

#include "F28x_Project.h"
#include "ADS8688_adc.h"
#include "cla.h"
#include "printf_json_delayed.h"
#include "scia.h"
#include "ADS8688_adc_cla.h"
#include "misc.h"
#include "daughterboard.h"
#include "tic_toc.h"


//internal variables
static const uint16_t ADS8688_channels = 8;
static volatile bool ads_adc_initialized = false;
static spi ADS8688_spi;

//cla variables
#pragma DATA_SECTION("cla_data");
uint16_t cla_ads8688_channel_to_ain[ADS8688_channels];  //cannot malloc, must use a constant
#pragma DATA_SECTION("cla_data");
volatile struct SPI_REGS *cla_ADS8688_spi_regs;
#pragma DATA_SECTION("cla_data");
volatile uint16_t cla_ADS8688_cycle_count = 0;
#pragma DATA_SECTION("cla_data");
volatile bool cla_ADS8688_adc_enabled = false;
#pragma DATA_SECTION("cla_data");
volatile bool has_had_fatal_adc_error = false;

volatile uint16_t has_printed_fatal_error_count = 0;

//internal functions
uint16_t send_ADS8688_command(uint16_t data);

__attribute__((ramfunc))
__interrupt void cla_task4_post_isr() {
    if (has_had_fatal_adc_error && (has_printed_fatal_error_count < 10)) {
        delay_printf_json_error("has had fatal adc error");
        has_printed_fatal_error_count++;
    }
}

bool init_ADS8688_adc(const analog_in_spi_t analog_in_spi_settings) {
    if (db_board.is_not_enabled()) {
        delay_printf_json_error("cannot initialize analog in without daughterboard");
        return false;
    }

    if (ads_adc_initialized) {return true;}

    if (cla_analog_in_count != ADS8688_channels) {
        delay_printf_json_error("count does not match ADS8688 count");
        return false;
    }

    //copy settings
    const analog_in_spi_t ADS8688_spi_settings = analog_in_spi_settings;

    for (uint16_t i=0; i<ADS8688_channels; i++) {
        cla_ads8688_channel_to_ain[i] = analog_in_spi_settings.device_channel_to_input_num[i];
    }

    //temp params used for init
    spi_params_t spi_params;

    //define the spi
    spi_params.is_master = true;
    spi_params.Hz = 17000000.0;  //max 17 MHz max
    spi_params.phase = 0;
    spi_params.polarity = 0;
    spi_params.bits = 16;
    spi_params.loopback_enabled = false;
    spi_params.rx_interrupt_enabled = false;
    spi_params.rx_interrupt_blocks = 4;
    spi_params.gpio = ADS8688_spi_settings.analog_in_spi_gpio;

    //init the spi
    ADS8688_spi.init(ADS8688_spi_settings.analog_in_spi_letter, spi_params);

    //get the spi regs pointer for the cla code
    cla_ADS8688_spi_regs = ADS8688_spi.get_spi_regs_ptr();


    //enable the reset pin, and reset the device
    GPIO_SetupPinOptions(ADS8688_spi_settings.reset_gpio, GPIO_OUTPUT, GPIO_PUSHPULL);
    GPIO_WritePin(ADS8688_spi_settings.reset_gpio, 0);  //first make sure it is off
    DELAY_US(10);
    GPIO_WritePin(ADS8688_spi_settings.reset_gpio, 1);  //then on
    DELAY_US(10);
    GPIO_WritePin(ADS8688_spi_settings.reset_gpio, 0);  //then off
    DELAY_US(10); //wait 10 us (which is greater than 400 ns necessary)
    GPIO_WritePin(ADS8688_spi_settings.reset_gpio, 1);  //then on


    //send the command to reset ("extra" reset)
    uint16_t test_value = send_ADS8688_command(0x8500);

    //test_value should not be equal to 0
    if (test_value > 0) {

        //send the command to put it in auto mode (cycle through channels)
        send_ADS8688_command(0xA000);

        //enable the cla task
        enable_cla_task(4, reinterpret_cast<uint16_t>(&cla_task4_read_ADS8688_adc_voltages), CLA_TRIG_TINT0);


        //enable the post task for checking on the fatal error
#pragma diag_suppress 1463
        EALLOW;
        PieVectTable.CLA1_4_INT = cla_task4_post_isr;  //set the interrupt vector
        PieCtrlRegs.PIEIER11.bit.INTx4 = 1;  //enable PIE 11.4 (CLA1_4)
        EDIS;
#pragma diag_default 1463

        cla_ADS8688_adc_enabled = true;
        ads_adc_initialized = true;
        delay_printf_json_status("initialized ADS8688 analog in");
    } else {
        delay_printf_json_error("unable to initialize ADS8688 analog in");
    }

    return ads_adc_initialized;
}

float32 ADS8688_adc_cla_task_length_in_us() {
    return cla_cycles_in_us(cla_ADS8688_cycle_count);
}

void change_ADS8688_range(uint16_t range_id) {
    //where 0 is normal(+/-10.24V), 1 is half, and 2 is quarter
    if (range_id > 2) {return;}

    //disable reading before changing range
    cla_ADS8688_adc_enabled = false;

    //update the range for each channel
    for (uint16_t channel=0; channel<ADS8688_channels; channel++) {
        uint16_t command = 5 + channel; //address starts at 5
        command = (command << 1) | 1;   //1 = write
        command = (command << 8) | range_id;
        send_ADS8688_command(command);
    }

    //re-enable reading
    cla_ADS8688_adc_enabled = true;

    //dumb way to set conversion factor
    switch (range_id) {
        case 0:
            set_voltage_conversion_factor(3200);
            break;
        case 1:
            set_voltage_conversion_factor(6400);
            break;
        case 2:
            set_voltage_conversion_factor(12800);
            break;
    }
}

uint16_t send_ADS8688_command(uint16_t data) {
    //wait until tx buffer is empty
    while (ADS8688_spi.is_tx_buffer_empty()) {
    }

    //send two bytes (second is empty)
    ADS8688_spi.add_byte_to_tx_buffer(data);
    ADS8688_spi.add_byte_to_tx_buffer(0);

    ADS8688_spi.wait_until_bytes_received(2);

    //grab two bytes from the buffer (and throw away first)
    uint16_t received_byte = ADS8688_spi.get_byte_from_rx_buffer();
    received_byte = ADS8688_spi.get_byte_from_rx_buffer();

    //return the second one
    return received_byte;
}
