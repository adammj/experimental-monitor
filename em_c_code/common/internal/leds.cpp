/*
 * leds.c
 *
 *  Created on: Mar 3, 2017
 *      Author: adam jones
 */

#include "F28x_Project.h"
#include "leds.h"
#include "misc.h"


//class methods
ti_led::ti_led() {
    gpio_ = 0;
    pin_mask_ = 0;
    pin_data_reg_ = NULL;
    pin_shift_ = 0;
}

ti_led::ti_led(uint16_t gpio) {
    gpio_ = gpio;
    GPIO_SetupPinOptions(gpio_, GPIO_OUTPUT, GPIO_PUSHPULL);

    pin_mask_ = calculate_pin_mask(gpio_);
    pin_data_reg_ = calculate_pin_data_reg(gpio_);
    pin_shift_ = (gpio_ % 32);

    this->off();
}

__attribute__((ramfunc))
void ti_led::on() const {
    if (gpio_ != 0) {
        gpio_write_pin(pin_data_reg_, pin_mask_, 0);    //low turns led on
    }
}

__attribute__((ramfunc))
void ti_led::off() const {
    if (gpio_ != 0) {
        gpio_write_pin(pin_data_reg_, pin_mask_, 1);    //high turns led off
    }
}

__attribute__((ramfunc))
void ti_led::toggle() const {
    if (gpio_ != 0) {
        const bool current_value = gpio_read_pin(pin_data_reg_, pin_shift_);
        gpio_write_pin(pin_data_reg_, pin_mask_, !current_value);
    }
}

__attribute__((ramfunc))
bool ti_led::is_on() const {
    if (gpio_ != 0) {
        return !gpio_read_pin(pin_data_reg_, pin_shift_);   //low is on
    } else {
        return false;
    }
}
