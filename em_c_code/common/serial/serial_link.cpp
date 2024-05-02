/*
 * serial_link.c
 *
 *  Created on: Mar 5, 2017
 *      Author: adam jones
 */

#include "serial_link.h"
#include "ti_launchpad.h"
#include "printf_json_delayed.h"    //should not use direct
#include "scia.h"
#include "tic_toc.h"
#include "io_controller.h"
#include "extract_json.h"
#include "string.h"
#include "arrays.h"


//internal variables
static const uint16_t serial_json_pool_size = 256;         //max tokens
static const uint16_t serial_max_rx_command_len = 1536;    //commands (some padding for event_code strings)
static char *current_serial_command_string;
static json_t *serial_json_pool;
static bool has_initialized = false;
static bool command_echo_on = false;   //default is off

//waiting code
static uint16_t max_waiting_messages = 10;
uint16_t waiting_count = 0;
uint16_t sec_until_waiting = 90;
uint16_t sec_until_waiting_reload = 90;
bool received_input = false;

//internal functions
void test_string_for_json_and_process(const char *const current_rx_command);
const json_t* test_and_point_to_command(const json_t *const json_root);
serial_command_t* get_command_from_json(const char *const json_cmd_value);
void set_command_echo(const json_t *const json_root);
void reset_command_validation();
void set_waiting_delay(const json_t *const json_root);

//first command that all others will link to
serial_command_t serial_command_list = {"get_commands", true, 0, print_all_serial_commands, NULL, NULL};

//additional commands
serial_command_t command_test_connection = {"test_connection", true, 0, NULL, print_connection_success, NULL};
serial_command_t command_serial_config = {"get_serial_configuration", true, 0, NULL, print_serial_configuration, NULL};
serial_command_t command_echo = {"set_command_echo", false, 0, set_command_echo, NULL, NULL};
serial_command_t command_status = {"set_status_messages", false,  0, set_status_messages, NULL, NULL};
serial_command_t command_waiting_delay = {"set_waiting_delay", false,  0, set_waiting_delay, NULL, NULL};


void add_serial_command(serial_command_t *new_serial_command) {
    uint32_t new_command_hash = djb2_hash(new_serial_command->command_string);

    new_serial_command->command_hash = new_command_hash;
    new_serial_command->next_function = NULL;  //safety check

    serial_command_t *last_serial_command = &serial_command_list;

    //FIXME: need to check that there are not hash collisions before appending
//    delay_printf_json_status("add new command, print list first");
    while (true) {
//        delay_printf_json_objects(1, json_string("command", last_serial_command->command_string));

        if (last_serial_command->next_function) {
            last_serial_command = last_serial_command->next_function;
        } else {
            break;
        }
    }

//    delay_printf_json_objects(1, json_string("command", new_serial_command->command_string));
    last_serial_command->next_function = new_serial_command;
}

__attribute__((ramfunc))
void printf_end_of_echo_valid_command(bool is_valid_command) {
    if (!command_echo_on) {return;}

    if (is_valid_command) {
        printf_char('+');
    } else {
        printf_char('-');
    }
    printf_return();
}

void init_serial_link() {
    if (has_initialized) {return;}

    //create string buffer
    current_serial_command_string = create_array_of<char>(serial_max_rx_command_len+1, "current_serial_command_string");
    if (current_serial_command_string == NULL) {
        return;
    }

    //create json pool
    serial_json_pool = create_array_of<json_t>(serial_json_pool_size, "current_serial_command_string");
    if (serial_json_pool == NULL) {
        return;
    }

    //calculate the root command hash
    serial_command_list.command_hash = djb2_hash(serial_command_list.command_string);

    //now, add the additional commands
    add_serial_command(&command_test_connection);
    add_serial_command(&command_serial_config);
    add_serial_command(&command_status);
    add_serial_command(&command_echo);
    add_serial_command(&command_waiting_delay);

    //print_serial_configuration();

    has_initialized = true;
}

uint16_t current_serial_command_len = 0;
bool serial_command_is_being_validated = false;

//main functions
__attribute__((ramfunc))
void process_scia_buffer_for_commands() {
    if (!has_initialized) {return;}

    //grab bytes, until buffer is empty, and if successful, continue
    uint16_t incoming_byte;
    while (scia_rx_buffer_read(incoming_byte)) {

        //for debugging purposes
        if ((!serial_command_is_being_validated) && (incoming_byte == '{')) {
            debug_timestamps.serial_s_bracket = CPU_TIMESTAMP;
        }

        //if length is 0 and first character is the start character, or if the command is being validated
        if (serial_command_is_being_validated || ((!serial_command_is_being_validated) && (incoming_byte == '{'))) {

            if (command_echo_on) {printf_char(incoming_byte);}

            //copy the command bytes
            serial_command_is_being_validated = true;
            current_serial_command_string[current_serial_command_len] = static_cast<char>(incoming_byte);
            current_serial_command_len++;


            //if a newline or return character is received, then stop processing
            //macox+linux is LF (10), and windows is CR+LR (13 + 10)
            if ((incoming_byte == 10) || (incoming_byte == 13)) {

                //upon receiving the first "command", stop waiting for input messages
                received_input = true;

                if (current_serial_command_len > 6) {
                    //null terminate the string, in preparation
                    current_serial_command_string[current_serial_command_len] = 0;
                    //try to process
                    test_string_for_json_and_process(current_serial_command_string);

                } else {
                    //otherwise, it was too short and don't bother
                    printf_end_of_echo_valid_command(false);
                    delay_printf_json_error("no valid JSON string found");
                }

                //either the command was processed, or it wasn't really valid, so reset
                reset_command_validation();

            //else, if at the last possible location, start over
            } else if (current_serial_command_len == serial_max_rx_command_len) {
                printf_end_of_echo_valid_command(false);
                delay_printf_json_error("no valid JSON string found");
                reset_command_validation();
            }
        }
    }
}

__attribute__((ramfunc))
void reset_command_validation() {
    serial_command_is_being_validated = false;
    current_serial_command_len = 0;
    //serial_command_json_bracket_count = 0;
    //add two nulls to the start
    current_serial_command_string[0] = 0;
    current_serial_command_string[1] = 0;
}

__attribute__((ramfunc))
void test_string_for_json_and_process(const char *const current_rx_command) {

    //create/test the JSON string
    const json_t *const json_root = json_create(const_cast<char *>(current_rx_command), serial_json_pool, serial_json_pool_size);

    //get the command root
    const json_t *const command_root = test_and_point_to_command(json_root);

    if (command_root != NULL) {
        const char *const json_cmd_value = command_root->u.value;

        //get the command_index from the string
        serial_command_t *serial_command = get_command_from_json(json_cmd_value);

        //if the command is valid
        //if (serial_command_index < serial_commands_count) {
        if (serial_command) {
            //for echo, acknowledge valid command
            printf_end_of_echo_valid_command(true);

            debug_timestamps.serial_call_func = CPU_TIMESTAMP;

            //if the json or void function exists
            //if (serial_commands[serial_command_index].json_function != NULL) {
            if (serial_command->json_function != NULL) {

                //json function (pass on the command sibling, which is the first object)
                //delay_printf_json_status("calling json function");
                //serial_commands[serial_command_index].json_function(command_root->sibling);
                serial_command->json_function(command_root->sibling);

            //} else if (serial_commands[serial_command_index].void_function != NULL) {
            } else if (serial_command->void_function != NULL) {

                //void function
                //delay_printf_json_status("calling void function");
//                serial_commands[serial_command_index].void_function();
                serial_command->void_function();
            } else {

                //give error
                delay_printf_json_objects(2, json_string("error", "command exists, but no function defined yet"), json_string("command", json_cmd_value));
            }

        } else {
            //else, command doesn't exist
            printf_end_of_echo_valid_command(false);
            delay_printf_json_objects(2, json_string("error", "unrecognized command"), json_string("command", json_cmd_value));
        }
    }
}

__attribute__((ramfunc))
const json_t* test_and_point_to_command(const json_t *const json_root) {
    if (json_root == NULL) {
        printf_end_of_echo_valid_command(false);
        delay_printf_json_error("no valid JSON string found");
        return NULL;

    }

    //test to make sure that the first object is the command
    const json_t *const command_json = json_root->u.child;    //this is because real root is just the whole object
    if ((command_json->type != JSON_TEXT) || (strcmp(command_json->name, "command") != 0)) {
        printf_end_of_echo_valid_command(false);
        delay_printf_json_error("'command' was not the first parameter");
        return NULL;
    }

    //now, test that none of the siblings are objects
    bool has_sibling_object = false;
    const json_t *current_json = command_json->sibling;
    while (current_json != NULL) {
        if (current_json->type == JSON_OBJ) {
            has_sibling_object = true;
        }
        current_json = current_json->sibling;
    }

    if (has_sibling_object) {
        printf_end_of_echo_valid_command(false);
        delay_printf_json_error("no nested objects are allowed");
        return NULL;
    }

    printf_end_of_echo_valid_command(true);
    //delay_printf_json_status("valid");
    return command_json;
}

serial_command_t* get_command_from_json(const char *const json_cmd_value) {
    //calculate the hash
    uint32_t current_command_hash = djb2_hash(const_cast<char *>(json_cmd_value));

    serial_command_t *current_command = &serial_command_list;
    serial_command_t *match_command = NULL;

//    delay_printf_json_objects(2, json_uint32("test hash", current_command_hash), json_string("test command", const_cast<char *>(json_cmd_value)));

    while (true) {
//        delay_printf_json_objects(1, json_uint32("command hash", current_command->command_hash));
        if (current_command->command_hash == current_command_hash) {
//            delay_printf_json_status("match");
            match_command = current_command;
            break;
        }

        if (current_command->next_function) {
//            delay_printf_json_status("go to next command");
            current_command = current_command->next_function;
        } else {
//            delay_printf_json_status("no matches");
            break;
        }
    }

    return match_command;
}

void print_connection_success() {
    delay_printf_json_objects(1, json_string("connection", "success"));
    twiddle_leds_ten_times();
}

void check_if_waiting_for_input() {
    if (sec_until_waiting_reload == 0) {return;}

    // if received input, then reset the secs until waiting
    if (received_input) {
        waiting_count = 0;
        received_input = false;
        sec_until_waiting = sec_until_waiting_reload;
        return;
    }

    // if there are seconds until waiting, then decrement
    if (sec_until_waiting) {
        sec_until_waiting--;
    } else {
        if (waiting_count < max_waiting_messages) {
            delay_printf_json_objects(1, json_string("status", "waiting for input"));
            waiting_count++;
        } else {
            ti_reboot();
        }
    }
}

void set_waiting_delay(const json_t *const json_root) {
    json_element delay_e("delay", t_uint16, true);

    if (!delay_e.set_with_json(json_root, true)) {
        return;
    }

    sec_until_waiting_reload = delay_e.value().uint16_;

    if (sec_until_waiting_reload == 0) {
        delay_printf_json_status("waiting disabled");
    } else {
        delay_printf_json_objects(1, json_uint16("new_delay", sec_until_waiting_reload));
    }
}

void print_serial_configuration() {
    delay_printf_json_objects(3, json_parent("serial_configuration", 2),
                            json_uint16("max_string_length", serial_max_rx_command_len),
                            json_uint16("max_json_tokens", serial_json_pool_size));
}

void print_all_serial_commands(const json_t *const json_root) {

//    json_element include_hidden_e("include_hidden", t_bool);
//    bool include_hidden = false;
//    if (include_hidden_e.set_with_json(json_root, false)) {
//        include_hidden = include_hidden_e.value().bool_;
//    }

    //FIXME: need to rebuild this function
    delay_printf_json_status("printing serial commands");
//    uint16_t temp_command_count = 0;
//    for (uint16_t i=0; i<serial_commands_count; i++) {
//        if (serial_commands[i].is_visible) {
//            temp_command_count++;
//        } else {
//            if (include_hidden) {
//                temp_command_count++;
//            }
//        }
//    }
//
//    const char **const command_strings = create_array_of<const char *>(temp_command_count, "command_strings");
//
//    if (command_strings != NULL) {
//        uint16_t temp_command_i = 0;
//
//        for (uint16_t i=0; i<serial_commands_count; i++) {
//
//            if (serial_commands[i].is_visible) {
//                if (temp_command_i < temp_command_count) {
//                    command_strings[temp_command_i] = serial_commands[i].command_string;
//                    temp_command_i++;
//                }
//            } else {
//                if (include_hidden) {
//                    if (temp_command_i < temp_command_count) {
//                        command_strings[temp_command_i] = serial_commands[i].command_string;
//                        temp_command_i++;
//                    }
//                }
//            }
//        }
//
//        //must copy, because command_strings array will be deleted
//        delay_printf_json_objects(1, json_string_array("commands", temp_command_count, command_strings, true));
//
//        delete_array(command_strings);
//    }
}

void set_command_echo(const json_t *const json_root) {
    json_element enable_e("enable", t_bool, true);

    if (!enable_e.set_with_json(json_root, true)) {
        return;
    }

    if (enable_e.value().bool_) {
        delay_printf_json_status("enable echo");
        command_echo_on = true;
    } else {
        delay_printf_json_status("disable echo");
        command_echo_on = false;
    }
}
