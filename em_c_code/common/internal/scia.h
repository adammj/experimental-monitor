/*
 * scia.h
 *
 *  Created on: Mar 3, 2017
 *      Author: adam jones
 */


#ifndef scia_defined
#define scia_defined

#include <stdint.h>
#include <stdbool.h>
#include "sci.h"


//init
bool init_scia(uint32_t baud_rate, const sci_gpio_t sci_gpio_settings);

// helper (hide printing directly to buffer)
void printf_char(uint16_t c);
void printf_return();
void printf_string(const char *const string);

//receive buffer
bool scia_rx_buffer_read(uint16_t &rx_buffer_byte);

//buffer info
void wait_until_scia_tx_buffer_is_empty();

#endif
