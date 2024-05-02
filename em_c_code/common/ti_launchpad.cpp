/*
 * ti_settings.c
 *
 *  Created on: Oct 10, 2017
 *      Author: adam jones
 */


#include "ti_launchpad.h"
#include "cla.h"
#include "string.h"
#include "math.h"
#include "misc.h"
#include "tic_toc.h"
#include "version.h"
#include "printf_json_delayed.h"
#include "printf_json.h"
#include "extract_json.h"
#include "scia.h"
#include "limits.h"
#include "stdlibf.h"
#include "serial_link.h"

//singleton
ti_launchpad ti_board;

serial_command_t command_ti_config = {"get_ti_configuration", true, 0, NULL, print_ti_configuration, NULL};
serial_command_t command_clock_scale = {"get_clock_scale", true, 0, NULL, print_clock_scale, NULL};

//watchdog count
static uint16_t max_wd_count = 0;

//lots of EALLOW and EDIS in this file, ignore all warnings on them
#pragma diag_suppress 1463

//UART sci settings
static struct sci_gpio_t sci_settings[] = {
    {84,  5, 85,  5},   //77S
    {42, 15, 43, 15}    //79D
};

//structure used to store the known board info
typedef struct ti_boards_t {
    uint32_t ti_uid;
    uint16_t ti_type;
    const char *launchpad_label;
    const char *launchpad_rev;
    int32 clock_ppb_deviation; // >1 = device is fast, <1 = device is slow
} ti_boards_t;

//list of each of the known boards
static struct ti_boards_t ti_board_settings[] = {
    {3204290601, 77, "S1",  "1.0", -40550},     //2018-01-31 w/func gen (<20 stdev)
    {1782342628, 79, "D1",  "1.1", -42903},     //2018-04-09 w/func gen (<20 stdev)
    {2607319766, 79, "D2",  "1.1", -40062},     //2018-04-09 w/func gen (<60 stdev)
    {2635391020, 79, "D3",  "1.1", -36699},     //2018-04-09 w/func gen (<20 stdev)
    {3910594187, 79, "D4",  "2.0", -34489},     //2018-04-09 w/func gen (<5 stdev)
    {1671270953, 79, "D5",  "2.0", -38825},     //2018-04-09 w/func gen (<10 stdev)
    {4145289644, 79, "D6",  "2.0", -41576},     //2018-04-09 w/func gen (<10 stdev)
    {1199677362, 79, "D7",  "2.0", -38306},     //2018-04-09 w/func gen (<15 stdev)
    {3502398600, 79, "D8",  "2.0", -49844},     //2018-04-09 w/func gen (<10 stdev)
    {3287187956, 79, "D9",  "2.0", -39064},     //2018-04-09 w/func gen (<5 stdev)
    {1902997583, 79, "D10", "2.0", -39605},     //2018-04-09 w/func gen (<2 stdev)
    {1841758641, 79, "D11", "2.0", -40051},     //2018-04-09 w/func gen (<2 stdev)
    //{id_given, 79, "new", "2.0", calibration constant},        //when calibrated
};


//the location so it prints out in the configuration
#ifdef _FLASH
static const char *code_location = "FLASH";
#else
static const char *code_location = "RAM";
#endif


ti_launchpad::ti_launchpad() {
    //first copy the values that need to be there from the beginning
#ifdef _LAUNCHXL_F28377S
    version_str_ = "F28377S";
    //scia
    scia_gpio_ = sci_settings[0];
    //chip
    chip_part_number_ = 0xFF;
    chip_family_ = 4;
    board_type_ = 77;
    //led
    red_led_gpio = 12;
    blue_led_gpio = 13;
#else
    version_str_ = "F28379D";
    //scia
    scia_gpio_ = sci_settings[1];
    //chip
    chip_part_number_ = 0xF9;
    chip_family_ = 3;
    board_type_ = 79;
    //led
    red_led_gpio = 34;
    blue_led_gpio = 31;
#endif

    //main cpu clock
    //For "normal" baud rates, 200MHz is fine
    clock_freq_ = 200000000;
    clock_ppb_deviation_ = 0;       //assumed value
    clock_scale_ = 1.0L;            //assumed value
    xtal_osc_freq_ = 10000000;      //XTAL_OSC, on board oscillator

    //other defaults
    peripheral_clock_divider_ = 1;  //must be equal for now
    launchpad_label_ = "UNKNOWN";
    launchpad_rev_ = "UNKNOWN";
    board_id_ = 0;
    initialized_ = false;
}



//cannot be ramfunc, because it copies the ramfuncs
bool ti_launchpad::startup() {

    //check if it was reset by the wd before the value is erased
    const bool was_reset_by_wd = CpuSysRegs.RESC.bit.WDRSn;

    //first, verify the chip (and halt if issue)
    this->verify_expected_chip();

    //then, init sys control
    //copies ramfunc, initializes flash, etc
    this->init_sys_control();

    //define leds (after ramfunc are copied)
    red_led = ti_led(red_led_gpio);
    blue_led = ti_led(blue_led_gpio);
    red_led.on();   //turn on red as an indicator of an issue

    //get the rest of the info
    this->update_specific_board_settings();

    //then, init the vector table
    InitPieVectTable(); //needs struct def

    //must be enabled before scia fifo buffers can be used
    this->enable_global_interrupts();

    //wait at least 0.1 seconds before init scia and sending first bytes
    //this allows the FTDI chip to correctly detect the baud
    DELAY_US(1000000);   //using 0.5 seconds (works on linux)

    //next, enable the serial communication (necessary for printf)
    //baud=  921600 works in Matlab for macOS, Windows10, and Ubuntu
    //baud= 1843200 works in Matlab for macOS (not for Ubuntu, and need to test on Windows10)
    //baud= 2777777 works in Matlab for macOS (Apr2020)
    const bool scia_success = init_scia(921600, this->get_scia_gpio());
    //try to replace the sci later... not now
    //static sci uart_sci = sci(SCI_A, 1843200, sci_gpiosettings);
    //static em_printf em_printf_device = em_printf(&uart_sci);
    if (!scia_success) {
        return false;
    }

    //feedback to user that it is starting up
#ifdef _FLASH
    if (was_reset_by_wd) {
        printf_json_objects(2, json_string("error","booting from FLASH, after reset"), json_string("software_version", experimental_monitor_version_str));
    } else {
        printf_json_objects(2, json_string("status","booting from FLASH"), json_string("software_version", experimental_monitor_version_str));
    }
#else
    printf_json_objects(2, json_string("status","booting from RAM"), json_string("software_version", experimental_monitor_version_str));
#endif

    //init other important things
    init_cla();     //necessary for any cla use (analog_in)
    init_tic_toc(); //must happen after clock scale and speed is known

    //now that tic toc has been initalized, run the performance test
    performance_test();

    if (board_id_ == 0) {
        const uint32_t local_ti_uid = get_ti_uid();
        printf_json_objects(2, json_string("error", "board not found"), json_uint32("id", local_ti_uid));
        return false;
    }

    add_serial_command(&command_ti_config);
    add_serial_command(&command_clock_scale);

    //turn off the red led to show no issues
    red_led.off();
    initialized_ = true;
    return true;
}

uint16_t ti_launchpad::get_board_type() const {
    if (board_id_ > 0) {
        return board_type_;
    } else {
        return 0;
    }
}

void ti_launchpad::update_specific_board_settings() {
    const uint16_t known_ti_boards = static_cast<uint16_t>(sizeof(ti_board_settings)/sizeof(ti_board_settings[0]));
    const uint32_t local_ti_uid = get_ti_uid();

    for (uint16_t i=0; i<known_ti_boards; i++) {
        //find the matching board
        if (local_ti_uid == ti_board_settings[i].ti_uid) {
            board_id_ = local_ti_uid;

            this->set_clock_deviation(ti_board_settings[i].clock_ppb_deviation, false);
            launchpad_label_ = ti_board_settings[i].launchpad_label;
            launchpad_rev_ = ti_board_settings[i].launchpad_rev;
            //exit, once done
            return;
        }
    }
}


//cannot reside in ramfunc
void ti_launchpad::init_sys_control() const {
    //copies ramfunc, initializes flash, etc

#ifdef _FLASH
    //The TI-provided InitSysCtrl does not work properly when running from RAM
    InitSysCtrl();

#else
    //extracted InitSysCtrl() common for 77S and 79D for running from RAM (likely only during debug)
    disable_watchdog();

    EALLOW;

    //
    // Enable pull-ups on unbonded IOs as soon as possible to reduce power
    // consumption.
    //
    GPIO_EnableUnbondedIOPullups();

    CpuSysRegs.PCLKCR13.bit.ADC_A = 1;
    CpuSysRegs.PCLKCR13.bit.ADC_B = 1;
    CpuSysRegs.PCLKCR13.bit.ADC_C = 1;
    CpuSysRegs.PCLKCR13.bit.ADC_D = 1;

    //
    // Check if device is trimmed
    //
    if(*(reinterpret_cast<uint16_t *>(0x5D1B6))   == 0x0000){
        //
        // Device is not trimmed--apply static calibration values
        //
        AnalogSubsysRegs.ANAREFTRIMA.all = 31709;
        AnalogSubsysRegs.ANAREFTRIMB.all = 31709;
        AnalogSubsysRegs.ANAREFTRIMC.all = 31709;
        AnalogSubsysRegs.ANAREFTRIMD.all = 31709;
    }

    CpuSysRegs.PCLKCR13.bit.ADC_A = 0;
    CpuSysRegs.PCLKCR13.bit.ADC_B = 0;
    CpuSysRegs.PCLKCR13.bit.ADC_C = 0;
    CpuSysRegs.PCLKCR13.bit.ADC_D = 0;
    EDIS;
#endif

    //calculate PLL settings
    float64 imult_float64 = static_cast<float64>(clock_freq_);
    imult_float64 *= 2.0L;         //multiply by 2 (PLLCLK_BY_2)
    imult_float64 /= static_cast<float64>(xtal_osc_freq_);  //divide by xtal_osc

    imult_float64 = roundl(imult_float64 * 4.0L);   //multiply by 4 and round to nearest
    const uint16_t fmult = static_cast<uint16_t>(imult_float64) % 4;            //calculate remainder (fractional part)

    uint16_t imult = static_cast<uint16_t>(imult_float64) - fmult;  //remove fractional part
    imult /= 4;             //scale back down (will always be a whole number)

    //set PLL (this will just return if the clock is already correctly set)
    InitSysPll(XTAL_OSC, imult, fmult, PLLCLK_BY_2);

    //next, setup the rest of the hardware
    this->set_peripheral_clock_divider();

    this->disable_all_peripheral_clocks();
}

void ti_launchpad::enable_pullups(uint16_t pullup_count, const uint16_t *const gpio_array) {

    //gpio setup only exists for CPU1
    for (uint16_t i=0; i<pullup_count; i++) {
        GPIO_SetupPinOptions(gpio_array[i], GPIO_INPUT, GPIO_PULLUP);
    }
}

void ti_launchpad::set_clock_deviation(int32_t ppb_deviation, bool print_message) {

    const float64 setting_clock_scale = 1.0L + (static_cast<float64>(ppb_deviation)/1e9L);

    if (setting_clock_scale > 1.01) {
        delay_printf_json_error("clock scale cannot be more than 1.01");
    } else if (setting_clock_scale < 0.99) {
        delay_printf_json_error("clock scale cannot be less than 0.99");
    } else {

        //store the rate and deviations
        clock_ppb_deviation_ = ppb_deviation;
        clock_scale_ = setting_clock_scale;
        if (print_message) {
            delay_printf_json_objects(2, json_int32("deviation",clock_ppb_deviation_), json_float64("scale",clock_scale_));
        }
    }
}

void print_clock_scale() {
    const float64 temp_value = ti_board.get_clock_scale();
    delay_printf_json_objects(1, json_float64("clock_scale", temp_value));
}

void ti_launchpad::print_configuration() const {
    delay_printf_json_objects(12, json_parent("ti_configuration", 11),
                        json_string("code_location", code_location),
                        json_string("ti_launchpad", version_str_),
                        json_string("launchpad_label", launchpad_label_),
                        json_string("launchpad_rev", launchpad_rev_),
                        json_uint32("hardware_id", get_ti_uid()),
                        json_uint32("clock_freq", clock_freq_),
                        json_int32("clock_ppb_deviation", clock_ppb_deviation_),
                        json_uint32("main_freq", main_frequency),
                        json_string("software_version", experimental_monitor_version_str),
                        json_string("compiled_date", compile_date_str),
                        json_string("compiled_time", compile_time_str));
}

void ti_launchpad::disable_watchdog() {
    volatile Uint16 temp;

    //
    // Grab the clock config first so we don't clobber it
    //
    EALLOW;
    temp = WdRegs.WDCR.all & 0x0007;
    WdRegs.WDCR.all = 0x0068 | temp;

    //clear the reset flag
    CpuSysRegs.RESC.bit.WDRSn = 1;
    EDIS;
}

void ti_launchpad::enable_watchdog() {
//    printf_json_status("enable watchdog");
    EALLOW;

    //first, send the key
    WdRegs.WDKEY.bit.WDKEY = 0x0055;
    WdRegs.WDKEY.bit.WDKEY = 0x00AA;

    //then set up the watchdog
    WdRegs.SCSR.all = 0x00;
    WdRegs.WDCR.all = 0b10101111;   //INTOSC1 (10MHz)/512/64 = 3.2768ms/tic * 256 tics = 838.86ms

    EDIS;
}


__attribute__((ramfunc))
uint16_t ti_launchpad::kick_watchdog() {
    //get the count
    const uint16_t wd_count = WdRegs.WDCNTR.bit.WDCNTR;

    //service the dog
    EALLOW;
    WdRegs.WDKEY.bit.WDKEY = 0x0055;
    WdRegs.WDKEY.bit.WDKEY = 0x00AA;
    EDIS;

    if (wd_count > max_wd_count) {
        max_wd_count = wd_count;
    }

    //if count exceeds 80% of allowed, let the user know
    //will warn if exceeds ~670ms
    if (wd_count > 204) {
        delay_printf_json_objects(2, json_string("error", "watchdog"), json_uint16("count", wd_count));
//      lockup_cpu();
    }

    return wd_count;
}

void ti_launchpad::reboot() {
    //just keep an infinite loop going, watchdog will restart the device
    printf_json_status("rebooting");
    ti_board.enable_watchdog();

    //infinite loop
    for (;;) {
    }
}

//c passthrough
void print_ti_configuration() {
    ti_board.print_configuration();
}

void ti_reboot() {
    //print all objects before rebooting
    printf_delayed_json_objects(true);
    ti_board.reboot();
}

//cannot reside in ramfunc
void ti_launchpad::verify_expected_chip() const {

    //test the PARTNO, and halt if error
    if (DevCfgRegs.PARTIDH.bit.PARTNO != chip_part_number_) {
        //keep this asm call stop immediately
        asm ("      ESTOP0");
        for (;;) {
        }
    }

    //test the FAMILY, and halt if error
    if (DevCfgRegs.PARTIDH.bit.FAMILY != chip_family_) {
        //keep this asm call stop immediately
        asm ("      ESTOP0");
        for (;;) {
        }
    }
}

void ti_launchpad::enable_global_interrupts() const {

    //enable global interrupts and higher priority real-time debug events
    EINT;   // Enable Global interrupt INTM (just enables the normal interrupts)
    ERTM;   // Enable Global real-time interrupt DBGM  (enables debugging interrupts)

}

void ti_launchpad::set_peripheral_clock_divider() const {
    uint16_t divider = peripheral_clock_divider_;

    //scale divider
    if (divider>1) {
        divider = divider/2;
    } else {
        divider = 0;
    }

    EALLOW;
    ClkCfgRegs.LOSPCP.bit.LSPCLKDIV = divider;
    EDIS;
}

sci_gpio_t ti_launchpad::get_scia_gpio() const {
    return scia_gpio_;
}


void ti_launchpad::disable_all_peripheral_clocks() const {
    //disable unnecessary clocks saves about 30ma

    EALLOW;

    CpuSysRegs.PCLKCR0.bit.CLA1 = 0;
    CpuSysRegs.PCLKCR0.bit.DMA = 0;
    CpuSysRegs.PCLKCR0.bit.CPUTIMER0 = 0;
    CpuSysRegs.PCLKCR0.bit.CPUTIMER1 = 0;
    CpuSysRegs.PCLKCR0.bit.CPUTIMER2 = 0;

    CpuSysRegs.PCLKCR0.bit.HRPWM = 0;

    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 0;

    CpuSysRegs.PCLKCR1.bit.EMIF1 = 0;
    CpuSysRegs.PCLKCR1.bit.EMIF2 = 0;

    CpuSysRegs.PCLKCR2.bit.EPWM1 = 0;
    CpuSysRegs.PCLKCR2.bit.EPWM2 = 0;
    CpuSysRegs.PCLKCR2.bit.EPWM3 = 0;
    CpuSysRegs.PCLKCR2.bit.EPWM4 = 0;
    CpuSysRegs.PCLKCR2.bit.EPWM5 = 0;
    CpuSysRegs.PCLKCR2.bit.EPWM6 = 0;
    CpuSysRegs.PCLKCR2.bit.EPWM7 = 0;
    CpuSysRegs.PCLKCR2.bit.EPWM8 = 0;
    CpuSysRegs.PCLKCR2.bit.EPWM9 = 0;
    CpuSysRegs.PCLKCR2.bit.EPWM10 = 0;
    CpuSysRegs.PCLKCR2.bit.EPWM11 = 0;
    CpuSysRegs.PCLKCR2.bit.EPWM12 = 0;

    CpuSysRegs.PCLKCR3.bit.ECAP1 = 0;
    CpuSysRegs.PCLKCR3.bit.ECAP2 = 0;
    CpuSysRegs.PCLKCR3.bit.ECAP3 = 0;
    CpuSysRegs.PCLKCR3.bit.ECAP4 = 0;
    CpuSysRegs.PCLKCR3.bit.ECAP5 = 0;
    CpuSysRegs.PCLKCR3.bit.ECAP6 = 0;

    CpuSysRegs.PCLKCR4.bit.EQEP1 = 0;
    CpuSysRegs.PCLKCR4.bit.EQEP2 = 0;
    CpuSysRegs.PCLKCR4.bit.EQEP3 = 0;

    CpuSysRegs.PCLKCR6.bit.SD1 = 0;
    CpuSysRegs.PCLKCR6.bit.SD2 = 0;

    CpuSysRegs.PCLKCR7.bit.SCI_A = 0;
    CpuSysRegs.PCLKCR7.bit.SCI_B = 0;
    CpuSysRegs.PCLKCR7.bit.SCI_C = 0;
    CpuSysRegs.PCLKCR7.bit.SCI_D = 0;

    CpuSysRegs.PCLKCR8.bit.SPI_A = 0;
    CpuSysRegs.PCLKCR8.bit.SPI_B = 0;
    CpuSysRegs.PCLKCR8.bit.SPI_C = 0;

    CpuSysRegs.PCLKCR9.bit.I2C_A = 0;
    CpuSysRegs.PCLKCR9.bit.I2C_B = 0;

    CpuSysRegs.PCLKCR10.bit.CAN_A = 0;
    CpuSysRegs.PCLKCR10.bit.CAN_B = 0;

    CpuSysRegs.PCLKCR11.bit.McBSP_A = 0;
    CpuSysRegs.PCLKCR11.bit.McBSP_B = 0;

    CpuSysRegs.PCLKCR11.bit.USB_A = 0;

    CpuSysRegs.PCLKCR12.bit.uPP_A = 0;

    CpuSysRegs.PCLKCR13.bit.ADC_A = 0;
    CpuSysRegs.PCLKCR13.bit.ADC_B = 0;
    CpuSysRegs.PCLKCR13.bit.ADC_C = 0;
    CpuSysRegs.PCLKCR13.bit.ADC_D = 0;

    CpuSysRegs.PCLKCR14.bit.CMPSS1 = 0;
    CpuSysRegs.PCLKCR14.bit.CMPSS2 = 0;
    CpuSysRegs.PCLKCR14.bit.CMPSS3 = 0;
    CpuSysRegs.PCLKCR14.bit.CMPSS4 = 0;
    CpuSysRegs.PCLKCR14.bit.CMPSS5 = 0;
    CpuSysRegs.PCLKCR14.bit.CMPSS6 = 0;
    CpuSysRegs.PCLKCR14.bit.CMPSS7 = 0;
    CpuSysRegs.PCLKCR14.bit.CMPSS8 = 0;

    CpuSysRegs.PCLKCR16.bit.DAC_A = 0;
    CpuSysRegs.PCLKCR16.bit.DAC_B = 0;
    CpuSysRegs.PCLKCR16.bit.DAC_C = 0;

    EDIS;
}

//lots of EALLOW and EDIS in this file, ignore all warnings on them
#pragma diag_default 1463
