/*
 * sci.cpp
 *
 *  Created on: Dec 29, 2017
 *      Author: adam jones
 */

#include "sci.h"
#include "ti_launchpad.h"
#include "math.h"
#include "printf_json_delayed.h"


sci::sci() {
    initialized_ = false;
    letter = SCI_A;
    sci_regs = &SciaRegs; //default to Scia
}

void sci::init(sci_letter_t sci_letter, uint32_t baud_rate, const sci_gpio_t sci_gpio_settings) {

    //copy
    letter = sci_letter;
    gpio = sci_gpio_settings;


    //Turn on the appropriate pins and mux
    GPIO_SetupPinMux(gpio.rx_gpio, GPIO_MUX_CPU1, gpio.rx_mux);
    GPIO_SetupPinOptions(gpio.rx_gpio, GPIO_INPUT, GPIO_ASYNC);
    GPIO_SetupPinMux(gpio.tx_gpio, GPIO_MUX_CPU1, gpio.tx_mux);
    GPIO_SetupPinOptions(gpio.tx_gpio, GPIO_OUTPUT, GPIO_ASYNC);

#pragma diag_suppress 1463
    EALLOW;
    bool not_found = false;
    switch (letter) {
        case SCI_A:
            sci_regs = &SciaRegs;
            CpuSysRegs.PCLKCR7.bit.SCI_A = 1;
            //Set the interrupt functions
            if (params.rx_int_enabled) {
                PieVectTable.SCIA_RX_INT = reinterpret_cast<PINT>(&(params.rx_isr));
            }
            if (params.tx_int_enabled) {
                PieVectTable.SCIA_TX_INT = reinterpret_cast<PINT>(&(params.tx_isr));
            }
            break;

        case SCI_B:
            sci_regs = &ScibRegs;
            CpuSysRegs.PCLKCR7.bit.SCI_B = 1;
            //Set the interrupt functions
            //if (sci_rx_int_enabled) PieVectTable.SCIB_RX_INT = &sci_rx_isr;
            //if (sci_tx_int_enabled) PieVectTable.SCIB_TX_INT = &sci_tx_isr;
            break;

        case SCI_C:
            sci_regs = &ScicRegs;
            CpuSysRegs.PCLKCR7.bit.SCI_C = 1;
            //Set the interrupt functions
            //if (sci_rx_int_enabled) PieVectTable.SCIC_RX_INT = &sci_rx_isr;
            //if (sci_tx_int_enabled) PieVectTable.SCIC_TX_INT = &sci_tx_isr;
            break;

        default:
            delay_printf_json_error("sci letter incorrect or not set");
            not_found = true;
            break;
    }
    EDIS;

    if (not_found) {
        return;
    }


    EALLOW;
    // 8 char bits, 1 stop bit, no parity
    sci_regs->SCICCR.all = 0x0007;

    //These settings are calculated based on the actual clock frequency
    sci_regs->SCIHBAUD.all = 0x0000; //HI bits of baud, always 0

    //calculate and check that the baud is possible
    const int32_t temp_scibaud = lroundl(((ti_board.get_main_clock_freq()/static_cast<float64>(baud_rate))/8.0L) - 1.0L);
    if ((temp_scibaud < 1) || (temp_scibaud > 255)) {
        delay_printf_json_objects(2, json_string("error", "sci_baud is impossible"), json_int32("SCILBAUD", temp_scibaud));
        return;
    }
    sci_regs->SCILBAUD.all = static_cast<uint16_t>(temp_scibaud);

    //enable TX, RX, internal SCICLK, Disable RX ERR, SLEEP, TXWAKE
    sci_regs->SCICTL1.bit.RXENA = 1;         // 0 SCI receiver enable
    sci_regs->SCICTL1.bit.TXENA = 1;         // 1 SCI transmitter enable
    sci_regs->SCICTL1.bit.SLEEP = 0;         // 2 SCI sleep
    sci_regs->SCICTL1.bit.TXWAKE = 0;        // 3 Transmitter wakeup method
    sci_regs->SCICTL1.bit.SWRESET = 0;       // 5 Software reset
    sci_regs->SCICTL1.bit.RXERRINTENA = 0;   // 6 Receive error __interrupt enable

    //enable the interrupts
    sci_regs->SCICTL2.bit.TXINTENA = params.tx_int_enabled;      // 0 Transmit interrupt enable
    sci_regs->SCICTL2.bit.RXBKINTENA = params.rx_int_enabled;    // 1 Receive interrupt enable

    //Initialize SCI FIFO
    //TX FIFO
    sci_regs->SCIFFTX.bit.TXFFIL = params.tx_isr_on_count;    // 4:0 Interrupt level
    sci_regs->SCIFFTX.bit.TXFFIENA = params.tx_int_enabled;      // 5 Interrupt enable
    sci_regs->SCIFFTX.bit.TXFFINTCLR = 1;    // 6 not not clear INT flag
    sci_regs->SCIFFTX.bit.TXFIFORESET = 0;   // 13 FIFO reset
    sci_regs->SCIFFTX.bit.SCIFFENA = 1;      // 14 Enhancement enable (both TX and RX FIFO enable)
    sci_regs->SCIFFTX.bit.SCIRST = 1;        // 15 SCI reset rx/tx channels

    //RX FIFO
    sci_regs->SCIFFRX.bit.RXFFIL = params.rx_isr_on_count;    // 4:0 Interrupt level
    sci_regs->SCIFFRX.bit.RXFFIENA = params.rx_int_enabled; // 5 Interrupt enable
    sci_regs->SCIFFRX.bit.RXFFINTCLR = 1;    // 6 Clear INT flag
    sci_regs->SCIFFRX.bit.RXFIFORESET = 0;   // 13 FIFO reset

    //FIFO control
    sci_regs->SCIFFCT.bit.FFTXDLY = 0;       // 7:0 FIFO transmit delay
    sci_regs->SCIFFCT.bit.CDC = 0;           // 13 Auto baud mode enable
    sci_regs->SCIFFCT.bit.ABDCLR = 0;        // 14 Auto baud clear
    sci_regs->SCIFFCT.bit.ABD = 0;           // 15 Auto baud detect

    //Relinquish SCI from Reset (keeping enabled TX, RX)
    sci_regs->SCICTL1.bit.SWRESET = 1;   // 5 Software reset

    //Enable the FIFO reset
    sci_regs->SCIFFTX.bit.TXFIFORESET = 1;
    sci_regs->SCIFFRX.bit.RXFIFORESET = 1;

    switch (letter) {
        case SCI_A:
            //enable the pie interrupts
            if (params.rx_int_enabled) {
                PieCtrlRegs.PIEIER9.bit.INTx1 = 1;  // PIE Group 9, INT1   RX
            }
            if (params.tx_int_enabled) {
                PieCtrlRegs.PIEIER9.bit.INTx2 = 1;  // PIE Group 9, INT2   TX
            }
            if (params.rx_int_enabled || params.tx_int_enabled) {
                IER|=M_INT9;       // Enable CPU INT
            }
            break;

        case SCI_B:
        case SCI_C:
            break;

        default:
            delay_printf_json_error("sci letter incorrect or not set");
            not_found = true;
            break;
    }
    EDIS;

    if (not_found) {
        return;
    }

#pragma diag_default 1463

    initialized_ = true;
}
