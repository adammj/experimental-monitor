/*
 * daughterboard.c
 *
 *  Created on: Jun 6, 2017
 *      Author: adam jones
 */

#include "daughterboard.h"
#include "printf_json.h"
#include "printf_json_delayed.h"
#include "cpu_timers.h"
#include "ti_launchpad.h"
#include "misc.h"
#include "extract_json.h"
#include "serial_link.h"


//singleton
daughterboard db_board;

serial_command_t command_db_config = {"get_daughterboard_configuration", true, 0, NULL, print_db_configuration, NULL};

typedef struct db_boards_t {
    uint16_t db_version;
    uint16_t ti_board_type;
    const char *ver_string;

    //pullup
    uint16_t pullup_count;
    const uint16_t *pullup_gpio;

    //dsp
    dsp_settings_t dsp_settings_;

    //digital io
    digital_io_settings_t digital_io_settings;

    //analog in
    analog_in_settings_t analog_in_settings;
} db_boards_t;

//internal variables
//db2_77s gpio
static const uint16_t db2_77s_pullup[] = {41,42,43,60,61,70,89};
static const uint16_t db2_77s_dsp_bit_gpio[] = {99,92,63,64,18,19,10,11,3,91,87,2,86,65,69};
static const uint16_t db2_77s_digital_in_gpio[]  = {4,14,62,15,16};
static const uint16_t db2_77s_digital_out_gpio[] = {78,73,21,72,20,59,17,61};
static const uint16_t db2_77s_analog_in_adc_letter[] = {0,1,1,1};  //not actually used
static const uint16_t db2_77s_analog_in_gpio[] = {14,4,0,2};
static analog_in_ti_t db2_77s_analog_settings = {db2_77s_analog_in_adc_letter, db2_77s_analog_in_gpio};
static dsp_settings_t db2_77s_dsp_settings = {66, 15, db2_77s_dsp_bit_gpio};
static digital_io_settings_t db2_77s_digital_io_settings = {5, 8, db2_77s_digital_in_gpio, db2_77s_digital_out_gpio};

//db2_79d gpio
static const uint16_t db2_79d_pullup[] = {0,1,14,15,18,22,25,52,60,67,94,95,97,104,105,111,122,130,131,157,158,159,160};
static const uint16_t db2_79d_dsp_bit_gpio[] = {26,27,63,64,10,11,8,9,7,66,139,6,56,65,41};
static const uint16_t db2_79d_digital_in_gpio[]  = {61,2,123,3,4};
static const uint16_t db2_79d_digital_out_gpio[] = {29,125,16,124,24,59,5,58};
static const uint16_t db2_79d_analog_in_adc_letter[] = {0,1,1,1};  //not actually used
static const uint16_t db2_79d_analog_in_gpio[] = {14,3,2,3};
static analog_in_ti_t db2_79d_analog_settings = {db2_79d_analog_in_adc_letter, db2_79d_analog_in_gpio};
static dsp_settings_t db2_79d_dsp_settings = {40, 15, db2_79d_dsp_bit_gpio};
static digital_io_settings_t db2_79d_digital_io_settings = {5, 8, db2_79d_digital_in_gpio, db2_79d_digital_out_gpio};

//db3_79d gpio
static const uint16_t db3_79d_pullup[] = {0,1,2,3,4,5,16,18,24,40,157,158,159,160};
static const uint16_t db3_79d_dsp_bit_gpio[] = {130,10,131,9,66,8,6,7};
static const uint16_t db3_79d_digital_in_gpio[]  = {63, 11, 64, 14, 26, 15, 27, 25};
static const uint16_t db3_79d_digital_out_gpio[] = {41, 52, 65, 94, 97, 56, 139, 95};
static const spi_gpio_t db3_79d_spi_adc = {58, 6, 60, 6, 61, 6, true, 59, 6};
static const uint16_t db3_79d_dev_chan_to_in[] = {5,1,2,6,3,7,4,0};
static analog_in_spi_t db3_79d_analog_settings = {SPI_B, db3_79d_spi_adc, 104, db3_79d_dev_chan_to_in};
static dsp_settings_t db3_79d_dsp_settings = {29, 8,  db3_79d_dsp_bit_gpio};
static digital_io_settings_t db3_79d_digital_io_settings = {8, 8, db3_79d_digital_in_gpio, db3_79d_digital_out_gpio};

static struct db_boards_t db_board_settings[] = {
{2, 77, "2", 7,  db2_77s_pullup, db2_77s_dsp_settings, db2_77s_digital_io_settings, {4, 0, {ti_settings: db2_77s_analog_settings}}},
{2, 79, "2", 23, db2_79d_pullup, db2_79d_dsp_settings, db2_79d_digital_io_settings, {4, 0, {ti_settings: db2_79d_analog_settings}}},
{3, 79, "3", 10, db3_79d_pullup, db3_79d_dsp_settings, db3_79d_digital_io_settings, {8, 1, {spi_settings: db3_79d_analog_settings}}},
};

typedef struct db_boards_sense_t {
    uint16_t ti_board_type;
    uint16_t sense_gpio_1;
    uint16_t sense_gpio_2;
} db_boards_sense_t;

static struct db_boards_sense_t db_board_sense[] = {
{77, 71, 90},
{79, 32, 19}
};




daughterboard::daughterboard() {
    is_enabled_ = false;
    version_str_ = NULL;
    version_num_ = 0;
    pullup_count_ = 0;
    pullup_gpio_ = NULL;
}

int16_t daughterboard::get_version() {
    const uint16_t ti_board_type = ti_board.get_board_type();
    const uint16_t boards_to_check = static_cast<uint16_t>(sizeof(db_board_sense)/sizeof(db_board_sense[0]));
    uint16_t sense_id = boards_to_check;

    //loop through each sense
    for (uint16_t i=0; i<boards_to_check; i++) {
        if (db_board_sense[i].ti_board_type == ti_board_type) {
            sense_id = i;
        }
    }

    if (sense_id == boards_to_check) {
        return -1;
    }

    //read the sense pins
    const bool sense_1 = !(GPIO_ReadPin(db_board_sense[sense_id].sense_gpio_1) == 0);
    const bool sense_2 = !(GPIO_ReadPin(db_board_sense[sense_id].sense_gpio_2) == 0);

    int16_t db_version = -1;

    if ((sense_1 == 0) && (sense_2 == 0)) {
        db_version = 2;
    } else if ((sense_1 == 1) && (sense_2 == 1)) {
        db_version = 3;
    } else {
        db_version = -1;
    }

    //delay_printf_json_objects(5, json_uint16("ti_type", db_board_sense[sense_id].ti_board_type), json_uint16("pin1", db_board_sense[sense_id].sense_gpio_1), json_uint16("pin2", db_board_sense[sense_id].sense_gpio_2),  json_bool("sense1", sense1), json_bool("sense2", sense2));
    return db_version;
}

bool daughterboard::startup() {

    if (is_enabled_) {return true;}

    const int16_t db_version = this->get_version();

    if (db_version <= 0) {
        printf_json_error("unknown daughterboard");
        return false;
    }

    const int16_t id = this->get_db_board_setting_id(static_cast<uint16_t>(db_version));

    if (id == -1) {
        printf_json_error("settings not defined for this ti and daughterboard combination");
        return false;
    }

    //give notice
    printf_json_objects(2, json_string("status", "daughterboard found"), json_int16("version", db_version));

    //make a local copy
    const db_boards_t board_settings = db_board_settings[id];

    //copy from board settings
    version_num_ = board_settings.db_version;
    version_str_ = board_settings.ver_string;
    digital_io_settings_ =  board_settings.digital_io_settings;
    analog_in_settings_ = board_settings.analog_in_settings;
    pullup_count_ = board_settings.pullup_count;
    pullup_gpio_ = const_cast<uint16_t *>(board_settings.pullup_gpio);
    dsp_settings_ = board_settings.dsp_settings_;

    //enable pullup on unused gpio
    ti_launchpad::enable_pullups(pullup_count_, pullup_gpio_);

    //mark enabled so the downstream init knows that it is
    is_enabled_ = true;

    //initialize everything on the daughterboard
    init_dsp_output(dsp_settings_);
    init_digital_io(digital_io_settings_);
    init_analog_in(board_settings.analog_in_settings);

    //print the configuration
    //this->print_configuration();

    add_serial_command(&command_db_config);

    return true;
}

void print_db_configuration() {
    if (db_board.is_not_enabled()) {return;}

    db_board.print_configuration();
}

bool daughterboard::is_not_enabled() const{
    if (!is_enabled_) {
        delay_printf_json_error("daughterboard has not been set yet.");
        return true;
    }

    return false;
}

void daughterboard::print_configuration() const{

    const uint16_t din_count = digital_in_count();
    const uint16_t ain_count = analog_in_count();
    const uint16_t dout_count = digital_out_count();

    delay_printf_json_objects(5, json_parent("db_configuration", 4),
                        json_string("daughterboard_version", version_str_),
                        json_uint16("digital_in_count", din_count),
                        json_uint16("analog_in_count", ain_count),
                        json_uint16("digital_out_count", dout_count));
}

int16_t daughterboard::get_db_board_setting_id(uint16_t db_version) const {

    //calculate the number of known boards
    const uint16_t known_db_boards = static_cast<uint16_t>(sizeof(db_board_settings)/sizeof(db_board_settings[0]));

    const uint16_t ti_board_type = ti_board.get_board_type();

    //loop through each configuration
    for (uint16_t i=0; i<known_db_boards; i++) {
        if ((db_board_settings[i].db_version == db_version) && (db_board_settings[i].ti_board_type == ti_board_type)) {
            return static_cast<int16_t>(i);
        }
    }

    return -1;
}
