/*
 * spi.h
 *
 *  Created on: Mar 3, 2017
 *      Author: adam jones
 */

#ifndef spi_defined
#define spi_defined

#include <stdint.h>
#include "stdbool.h"
#include "F28x_Project.h"

typedef enum {
    SPI_A = 1,
    SPI_B,
    SPI_C
} spi_letter_t;

typedef struct spi_gpio_t {
    uint16_t clk_gpio;
    uint16_t clk_mux;
    uint16_t mosi_gpio;
    uint16_t mosi_mux;
    uint16_t miso_gpio;
    uint16_t miso_mux;

    //chip_select - if false, then pins are ignored
    bool auto_cs;
    uint16_t cs_gpio;
    uint16_t cs_mux;
} spi_gpio_t;

typedef struct spi_params_t {
    bool is_master;
    float32 Hz;
    uint16_t phase;     //(0 == rising, 1 == falling)
    uint16_t polarity;  //(0 == normal, 1 == delayed)
    uint16_t bits;      //1-16
    bool loopback_enabled;
    bool rx_interrupt_enabled;
    uint16_t rx_interrupt_blocks;
    spi_gpio_t gpio;
} spi_params_t;

//kept because of spi inclusion in ADS8688 cla header
#ifdef __cplusplus

class spi {
    public:
        spi();
        void init(spi_letter_t spi_letter, const spi_params_t spi_params);
        bool is_tx_buffer_empty() const;
        void add_byte_to_tx_buffer(uint16_t byte_to_send) const;
        void wait_until_bytes_received(uint16_t number_to_receive) const;
        uint16_t get_byte_from_rx_buffer() const;
        void empty_rx_buffer() const;
        volatile struct SPI_REGS *get_spi_regs_ptr() const;   //necessary for cla code

    private:
        bool initialized_;
        spi_letter_t letter;
        spi_params_t params;
        volatile struct SPI_REGS *spi_regs;
};

#endif

#endif
