/*
 * spi.c
 *
 *  Created on: Mar 3, 2017
 *      Author: adam jones
 */

#include "spi.h"
#include "ti_launchpad.h"
#include "math.h"
#include "printf_json_delayed.h"


spi::spi() {
    //default to a, but it is unintialized
    spi_regs = &SpiaRegs;
    letter = SPI_A;
    initialized_ = false;
}

void spi::init(spi_letter_t spi_letter, const spi_params_t spi_params) {

    //copy over
    letter = spi_letter;
    params = spi_params;

#pragma diag_suppress 1463
    EALLOW;
    bool not_found = false;
    switch (letter) {
        case SPI_A:
            spi_regs = &SpiaRegs;
            CpuSysRegs.PCLKCR8.bit.SPI_A = 1;
            break;

        case SPI_B:
            spi_regs = &SpibRegs;
            CpuSysRegs.PCLKCR8.bit.SPI_B = 1;
            break;

        case SPI_C:
            spi_regs = &SpicRegs;
            CpuSysRegs.PCLKCR8.bit.SPI_C = 1;
            break;

        default:
            delay_printf_json_error("spi letter incorrect or not set");
            not_found = true;
            break;
    }
    EDIS;

    if (not_found) {
        return;
    }


    //Force the SPI to reset first
    spi_regs->SPICCR.bit.SPISWRESET = 0;

    //then, calculate the best bit rate divider before doing anything else
    //rounds up to nearest
    uint16_t SPI_BIT_RATE = static_cast<uint16_t>((ti_board.get_peripheral_clock_freq()/static_cast<float64>(params.Hz)) + 0.5L);

    SPI_BIT_RATE += (SPI_BIT_RATE & 1); //increase to next even (keep clock symmetry)

    //check if divider is okay
    if ((SPI_BIT_RATE < 10) || (SPI_BIT_RATE > 128)) {
        //printf_"prep", , get_peripheral_clock_freq());
        //printf_"SPI_BIT_RATE", t_uint16, SPI_BIT_RATE);
        delay_printf_json_error("spi bit rate is invalid");
        return;
    }

    //get the actual rate back out
    params.Hz = static_cast<float32>(ti_board.get_peripheral_clock_freq()/static_cast<float32>(SPI_BIT_RATE));

    //set the pins (don't wrap this in EALLOW AND EDIS)
    GPIO_SetupPinOptions(params.gpio.clk_gpio, params.is_master, GPIO_ASYNC);
    GPIO_SetupPinOptions(params.gpio.mosi_gpio, params.is_master, GPIO_ASYNC);
    GPIO_SetupPinOptions(params.gpio.miso_gpio, static_cast<uint16_t>(!(params.is_master)), GPIO_ASYNC);
    if (params.gpio.auto_cs) {
        GPIO_SetupPinOptions(params.gpio.cs_gpio, params.is_master, GPIO_ASYNC);
    }

    //set the pin mux (assuming set by CPU1, but accessed by any through SPI)
    GPIO_SetupPinMux(params.gpio.clk_gpio, GPIO_MUX_CPU1, params.gpio.clk_mux);
    GPIO_SetupPinMux(params.gpio.mosi_gpio, GPIO_MUX_CPU1, params.gpio.mosi_mux);
    GPIO_SetupPinMux(params.gpio.miso_gpio, GPIO_MUX_CPU1, params.gpio.miso_mux);
    if (params.gpio.auto_cs) {
        GPIO_SetupPinMux(params.gpio.cs_gpio, GPIO_MUX_CPU1, params.gpio.cs_mux);
    }

    EALLOW;

    spi_regs->SPIBRR.bit.SPI_BIT_RATE = (SPI_BIT_RATE-1);  //baud rate = LSPCLK/(SPIBRR+1)

    //max1300 is CPOL = 0
    spi_regs->SPICCR.bit.CLKPOLARITY = params.polarity;    //(0 == rising, 1 == falling)
    spi_regs->SPICCR.bit.HS_MODE = 0;        //High Speed Mode Enable Bits
    spi_regs->SPICCR.bit.SPILBK = params.loopback_enabled;         //0 = no loopback

    //make sure bits isn't equal to 0
    if (params.bits == 0) {params.bits = 1;}
    if (params.bits > 16) {params.bits = 16;}
    spi_regs->SPICCR.bit.SPICHAR = params.bits - 1;       //bits for each transmission

    spi_regs->SPICTL.bit.OVERRUNINTENA = 0;  //Overrun Interrupt Enable

    //max1300 is CPHA = 0, however 1 seems to be necessary to receive the full 16 bits
    spi_regs->SPICTL.bit.CLK_PHASE = params.phase;      //(0 == normal, 1 == delayed)
    spi_regs->SPICTL.bit.MASTER_SLAVE = params.is_master;   //1 = spi_master
    spi_regs->SPICTL.bit.TALK = params.is_master;           //1 = transmit
    spi_regs->SPICTL.bit.SPIINTENA = 0;      //1 = interrupt

    //TX FIFO
    spi_regs->SPIFFTX.bit.TXFFIL = 0;        //Transmit FIFO Interrupt Level Bits
    spi_regs->SPIFFTX.bit.TXFFIENA = 0;      //TX FIFO Interrupt Enable
    spi_regs->SPIFFTX.bit.TXFFINTCLR = 1;    //TXFIFO Interrupt Clear
    spi_regs->SPIFFTX.bit.SPIFFENA = 1;      //SPI FIFO Enhancements Enable  - to get a FIFO buffer
    spi_regs->SPIFFTX.bit.TXFIFO = 0;        //TX FIFO Reset


    //RX FIFO
    if (params.rx_interrupt_blocks > 16) {params.rx_interrupt_blocks = 16;}
    spi_regs->SPIFFRX.bit.RXFFIL = params.rx_interrupt_blocks;  //Receive FIFO Interrupt Level
    spi_regs->SPIFFRX.bit.RXFFIENA = params.rx_interrupt_enabled;      //RX FIFO Interrupt Enable
    spi_regs->SPIFFRX.bit.RXFFOVFCLR = 1;    //Receive FIFO Overflow Clear
    spi_regs->SPIFFRX.bit.RXFIFORESET = 0;   //Receive FIFO Reset
    spi_regs->SPIFFRX.bit.RXFFINTCLR = 1;    //Receive FIFO Interrupt Clear

    //FIFO control
    spi_regs->SPIFFCT.bit.TXDLY = 0;         //FIFO Transmit Delay Bits

    //Release the SPI from reset
    spi_regs->SPICCR.bit.SPISWRESET = 1;

    // Halting on a breakpoint will not halt the SPI
    spi_regs->SPIPRI.bit.FREE = 1;

    //Enable the FIFO reset
    spi_regs->SPIFFRX.bit.RXFIFORESET = 1;   //Receive FIFO Reset
    spi_regs->SPIFFTX.bit.TXFIFO = 1;        //TX FIFO Reset

    EDIS;

#pragma diag_default 1463

    initialized_ = true;
}

bool spi::is_tx_buffer_empty() const {
    if (!initialized_) {return false;}
    return spi_regs->SPIFFTX.bit.TXFFST > 0;
}

void spi::add_byte_to_tx_buffer(uint16_t byte_to_send) const {
    if (!initialized_) {return;}
    spi_regs->SPITXBUF = byte_to_send;
}

void spi::wait_until_bytes_received(uint16_t number_to_receive) const {
    if (!initialized_) {return;}
    while (spi_regs->SPIFFRX.bit.RXFFST < number_to_receive) {
    }
}

uint16_t spi::get_byte_from_rx_buffer() const {
    if (!initialized_) {return 0;}
    return spi_regs->SPIRXBUF;
}

void spi::empty_rx_buffer() const {
    if (!initialized_) {return;}
#pragma diag_suppress 552
    volatile uint16_t throw_away;
    while (spi_regs->SPIFFRX.bit.RXFFST > 0) {
        throw_away = spi_regs->SPIRXBUF;
    }
#pragma diag_default 552
}

volatile struct SPI_REGS* spi::get_spi_regs_ptr() const {
    if (!initialized_) {return NULL;}
    return spi_regs;
}
