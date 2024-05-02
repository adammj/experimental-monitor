/*
 * sci.h
 *
 *  Created on: Dec 29, 2017
 *      Author: adam jones
 */

#ifndef sci_defined
#define sci_defined

#include <stdint.h>
#include <stdbool.h>
#include "F28x_Project.h"
#include "ring_buffer.h"

typedef enum {
    SCI_A = 1,
    SCI_B,
    SCI_C
} sci_letter_t;

typedef struct sci_gpio_t {
    uint16_t tx_gpio;
    uint16_t tx_mux;
    uint16_t rx_gpio;
    uint16_t rx_mux;
} sci_gpio_t;

typedef __interrupt void (*sci_isr_function) (void);

typedef struct sci_params_t {
    sci_gpio_t gpio;

    //tx interrupt
    bool tx_int_enabled;
    bool tx_isr_on_count;
    ring_buffer *tx_buffer;
    sci_isr_function tx_isr;

    //rx interrupt
    bool rx_int_enabled;
    bool rx_isr_on_count;
    ring_buffer *rx_buffer;
    sci_isr_function rx_isr;
} sci_params_t;


class sci {
    public:
        sci();
        void init(sci_letter_t sci_letter, uint32_t baud_rate, const sci_gpio_t sci_gpio_settings);
    private:
        bool initialized_;
        sci_letter_t letter;
        sci_gpio_t gpio;
        sci_params_t params;
        volatile struct SCI_REGS *sci_regs;
        volatile bool using_sci_tx_ring_buffer;
};


#endif
