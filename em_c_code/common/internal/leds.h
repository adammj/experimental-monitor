/*
 * leds.h
 *
 *  Created on: Mar 3, 2017
 *      Author: adam jones
 */

#ifndef led_settings_defined
#define led_settings_defined

#include <stdint.h>

class ti_led {
    public:
        ti_led();
        explicit ti_led(uint16_t gpio);
        void on() const;
        void off() const;
        void toggle() const;
        bool is_on() const;

    private:
        uint16_t gpio_;
        uint32_t pin_mask_;
        volatile uint32_t *pin_data_reg_;
        uint16_t pin_shift_;
};


#endif
