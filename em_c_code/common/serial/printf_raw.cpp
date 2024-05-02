/*
 * printf_raw.cpp
 *
 *  Created on: Apr 14, 2018
 *      Author: adam jones
 */

#include "printf_raw.h"
#include "limits.h"
#include "scia.h"
#include "math.h"
#include <printf_json.h>
#include <printf_json_delayed.h>


__attribute__((ramfunc))
void print_all_int_except_uint64_value(int64_t value) {
    //simple case is 0
    if (value == 0) {
        printf_char('0');
        return;
    }

    //print and remove the negative sign
    bool is_LLONG_MIN = false;
    if (value < 0) {
        printf_char('-');

        if (value != LLONG_MIN) {
            value = -value;
        } else {
            is_LLONG_MIN = true;
            value = LLONG_MAX;
        }
    }

    //get the characters
    char buffer[19];
    uint16_t index = 0;
    while (value > 0) {
        buffer[index] = '0' + static_cast<char>(value % 10);
        value /= 10;
        index++;
    }

    if (is_LLONG_MIN) {
        buffer[0] = '8';
    }

    //reverse print
    while ((index--) > 0) {
        printf_char(static_cast<uint16_t>(buffer[index]));
    }
}

__attribute__((ramfunc))
void print_uint64_value(uint64_t value) {
    //simple case is 0
    if (value == 0) {
        printf_char('0');
        return;
    }

    char buffer[20];
    uint16_t index = 0;
    while (value > 0) {
        buffer[index] = '0' + static_cast<char>(value % 10);
        value /= 10;
        index++;
    }

    //reverse print
    while ((index--) > 0) {
        printf_char(static_cast<uint16_t>(buffer[index]));
    }
}

__attribute__((ramfunc))
void print_uint64_timestamp(uint64_t timestamp, uint64_t ts_multiplier, uint16_t ts_shift) {

    //simple case is 0 (just print 0.xxxx)
    if (timestamp == 0) {
        printf_char('0');
        printf_char('.');
        for (uint16_t i=0; i<ts_shift; i++) {
            printf_char('0');
        }
        return;
    }

    //first step, multiply
    timestamp *= ts_multiplier;

    char buffer[20];
    uint16_t index = 0;
    while (timestamp > 0) {
        buffer[index] = '0' + static_cast<char>(timestamp % 10);
        timestamp /= 10;
        index++;
    }

    //if smaller than 0, print out the beginning
    uint16_t print_before_period = 0;
    if (index <= ts_shift) {
        printf_char('0');
        printf_char('.');
        for (uint16_t i=index; i<ts_shift; i++) {
            printf_char('0');
        }
    } else {
        print_before_period = index - ts_shift;
    }

    uint16_t places_count = 0;
    //reverse print
    while (index > 0) {
        index--;
        printf_char(static_cast<uint16_t>(buffer[index]));

        places_count++;
        if (places_count == print_before_period) {
            printf_char('.');
        }
    }
}


__attribute__((ramfunc))
void print_float32_value(float32 value, uint16_t precision) {
    //simple case is 0
    if (value == 0.0) {
        printf_string("0");
        printf_char('.');
        for (uint16_t i=0; i<precision; i++) {
            printf_char('0');
        }
        return;
    }

    if (isinf(value)) {
        if (value < 0) {
            printf_char('-');
        }
        printf_string("inf");
        return;
    }
    if (isnan(value)) {
        printf_string("nan");
        return;
    }

    //print and remove the negative sign
    if (value < 0.0) {
        printf_char('-');
        value = -value;
    }

    //not really infinite, but too large to work with
    if (value > static_cast<float32>(ULLONG_MAX)) {
        printf_string("inf");
        return;
    }

    //get the integer characters
    uint64_t i_part = static_cast<uint64_t>(floor(value));
    uint16_t max_sig_figs = 7;

    if (i_part == 0) {
        printf_char('0');
    } else {
        char buffer[20];
        uint16_t index = 0;

        while (i_part > 0) {
            buffer[index] = '0' + static_cast<char>(i_part % 10);
            i_part /= 10;
            index++;
        }

        if (index < max_sig_figs) {
            max_sig_figs -= index;
        } else {
            max_sig_figs = 0;
        }

        //reverse print
        while ((index--) > 0) {
            printf_char(static_cast<uint16_t>(buffer[index]));
        }
    }

    if (max_sig_figs > precision) {
        max_sig_figs = precision;
    }

    if (max_sig_figs <= 0) {
        return;
    }

    //print the fractional part
    printf_char('.');

    //remove the integer part
    value -= static_cast<float32>(i_part);

    for (uint16_t i=0; i<max_sig_figs; i++) {
        value *= 10.0;
        i_part = static_cast<uint64_t>(value);
        i_part = i_part % 10;
        printf_char(static_cast<uint16_t>('0') + static_cast<uint16_t>(i_part));
    }
}

__attribute__((ramfunc))
void print_float64_value(float64 value, uint16_t precision) {
    //simple case is 0
    if (value == 0.0) {
        printf_string("0");
        printf_char('.');
        for (uint16_t i=0; i<precision; i++) {
            printf_char('0');
        }
        return;
    }

    if (isinf(value)) {
        if (value < 0) {
            printf_char('-');
        }
        printf_string("inf");
        return;
    }
    if (isnan(value)) {
        printf_string("nan");
        return;
    }

    //print and remove the negative sign
    if (value < 0.0L) {
        printf_char('-');
        value = -value;
    }

    //not really infinite, but too large to work with
    if (value > static_cast<float64>(ULLONG_MAX)) {
        printf_string("inf");
        return;
    }

    //get the integer characters
    uint64_t i_part = static_cast<uint64_t>(floorl(value));
    uint16_t max_sig_figs = 15;

    if (i_part == 0) {
        printf_char('0');
    } else {
        char buffer[20];
        uint16_t index = 0;

        while (i_part > 0) {
            buffer[index] = '0' + static_cast<char>(i_part % 10);
            i_part /= 10;
            index++;
        }

        if (index < max_sig_figs) {
            max_sig_figs -= index;
        } else {
            max_sig_figs = 0;
        }

        //reverse print
        while ((index--) > 0) {
            printf_char(static_cast<uint16_t>(buffer[index]));
        }
    }

    if (max_sig_figs > precision) {
        max_sig_figs = precision;
    }

    if (max_sig_figs <= 0) {
        return;
    }

    //print the fractional part
    printf_char('.');

    //remove the integer part
    value -= static_cast<float64>(i_part);

    for (uint16_t i=0; i<max_sig_figs; i++) {
        value *= 10.0L;
        i_part = static_cast<uint64_t>(value);
        i_part = i_part % 10;
        printf_char(static_cast<uint16_t>('0') + static_cast<uint16_t>(i_part));
    }
}

//keeping around for posterity
void print_compress1_base(uint32_t value) {

    //original way (takes 57 shifts)
//    //flip the order (to account for flipped 32bit storage)
//    value = ((value & 0xFFFF) << 16) | (value >> 16);
//
//    uint16_t charval0 = (value & 0b00001111) | 0b10000000;
//    value = value >> 4;
//
//    uint16_t charval1 = (value & 0b01111111) | 0b10000000;
//    value = value >> 7;
//
//    uint16_t charval2 = (value & 0b01111111) | 0b10000000;
//    value = value >> 7;
//
//    uint16_t charval3 = (value & 0b01111111) | 0b10000000;
//    value = value >> 7;
//
//    uint16_t charval4 = (value & 0b01111111) | 0b10000000;

    //tested and works (takes 32 shifts)
    //don't flip the order, work with as-is (to account for flipped 32bit storage)
    //take the upper two bits of charval2
    uint16_t charval2 = (value & 0b00000011);
    value = value >> 2;

    const uint16_t charval3 = (value & 0b01111111) | 0b10000000;
    value = value >> 7;

    const uint16_t charval4 = (value & 0b01111111) | 0b10000000;
    value = value >> 7;

    const uint16_t charval0 = (value & 0b00001111) | 0b10000000;
    value = value >> 4;

    const uint16_t charval1 = (value & 0b01111111) | 0b10000000;
    value = value >> 7;

    //finally, add the lower 5 bits to charvar2
    charval2 = (charval2 << 5) | (value & 0b00011111) | 0b10000000;

    //send char is correct order
    printf_char(charval4);
    printf_char(charval3);
    printf_char(charval2);
    printf_char(charval1);
    printf_char(charval0);
}

const char *base64_char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

__attribute__((ramfunc))
void add_to_buffer(uint32_t bits_to_add_to_buffer, uint16_t count_of_new_bits, uint32_t *buffer, uint16_t *buffer_len) {
    if (count_of_new_bits > 32) {
        return;
    }

    //add the bits to the buffer
    if (count_of_new_bits < 32) {
        uint32_t bit_mask = (1ul << count_of_new_bits) - 1;
        (*buffer) = ((*buffer) << count_of_new_bits) | (bits_to_add_to_buffer & bit_mask);
    } else {
        (*buffer) = bits_to_add_to_buffer;
    }

    //update the buffer_len
    (*buffer_len) += count_of_new_bits;
    if ((*buffer_len) > 32) {
        (*buffer_len) = 32;
    }
}

__attribute__((ramfunc))
uint16_t remove_and_send_base64_char(uint32_t buffer, uint16_t *buffer_len) {
    //the bit count is updated as each character is removed and sent

    uint32_t local_buffer;
    uint16_t char_count = 0;

    while ((*buffer_len) > 5) {
        local_buffer = buffer;
        local_buffer = (local_buffer >> ((*buffer_len) - 6)) & 0b111111;
        (*buffer_len) -= 6;

        printf_char(base64_char[local_buffer]);
        char_count++;
    }

    return char_count;
}

__attribute__((ramfunc))
void print_base64_array(const uint16_t *const array_ptr, uint16_t array_count, uint16_t precision) {
    //precision: bits (1 and 16 allowed)

    if (array_count == 0 || (!(precision == 1 || precision == 16))) {
        return;
    }

    uint32_t base64_buffer = 0;
    uint16_t base64_buffer_len = 0;
    uint16_t char_count_sent = 0;

    //print the precision and count as the first 4 chars
    add_to_buffer(precision, 8, &base64_buffer, &base64_buffer_len);
    add_to_buffer(array_count, 16, &base64_buffer, &base64_buffer_len);
    char_count_sent += remove_and_send_base64_char(base64_buffer, &base64_buffer_len);

    if (precision == 1) {
        for (uint16_t array_index=0; array_index<array_count; array_index++) {
            base64_buffer = (base64_buffer << 1) | (array_ptr[array_index] & 0b1);
            base64_buffer_len++;

            //remove extraneous function calls
            if (base64_buffer_len > 5) {
                char_count_sent += remove_and_send_base64_char(base64_buffer, &base64_buffer_len);
            }
        }
    } else if (precision == 16) {
        for (uint16_t array_index=0; array_index<array_count; array_index++) {
            base64_buffer = (base64_buffer << 16) | (array_ptr[array_index] & 0xFFFF);
            base64_buffer_len += 16;

            char_count_sent += remove_and_send_base64_char(base64_buffer, &base64_buffer_len);
        }

    }

    //if any bits remain, it'll be fewer than 6
    if (base64_buffer_len > 0) {
        base64_buffer = (base64_buffer << (6 - base64_buffer_len));
        base64_buffer_len = 6;
        char_count_sent += remove_and_send_base64_char(base64_buffer, &base64_buffer_len);
    }

    //padding
    uint16_t pad_count = fmodf(4 - char_count_sent, 4);
    for (uint16_t pad_i=0; pad_i<pad_count; pad_i++) {
        printf_char('=');
    }
}


void test_base64() {

    //disable after the first and every
    //command_echo_on = false;
    uint16_t array[24];
    array[0] = 1000;
    array[1] = 0;
    array[2] = 100;
    array[3] = 101;
    array[4] = 10001;
    array[5] = 10000;
    array[6] = 65535;
    array[7] = 65535;
    array[8] = 50000;
    array[9] = 501;
    array[10] =500;
    array[11] =50001;
    array[12] = 1000;
    array[13] = 0;
    array[14] = 100;
    array[15] = 101;
    array[16] = 10001;
    array[17] = 10000;
    array[18] = 65535;
    array[19] = 65535;
    array[20] = 50000;
    array[21] = 501;
    array[22] =500;
    array[23] =50001;

    for (uint16_t i = 1; i<24; i++) {
        delay_printf_json_objects(1, json_uint16_array("uint16_array", i, array, true));
        delay_printf_json_objects(1, json_base64_array("base64_array_16", i, array, true, 16));
        delay_printf_json_objects(1, json_base64_array("base64_array_1", i, array, true, 1));
        printf_delayed_json_objects(true);
    }
}
