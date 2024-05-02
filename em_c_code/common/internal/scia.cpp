/*
 * scia.c
 *
 *  Created on: Mar 3, 2017
 *      Author: adam jones
 */

#include "scia.h"
#include "F28x_Project.h"
#include "ring_buffer.h"
#include "math.h"
#include "stdarg.h"
#include "ti_launchpad.h"
#include "printf_json.h"    //just for init status
#include "printf_json_delayed.h"
#include "misc.h"
#include "tic_toc.h"


//internal variables
//SCI FIFO and ring buffers
static const uint16_t sci_fifo_buffer_max = 16;    //max size of fifo buffer (fixed)
static volatile bool scia_initialized = false;

//RX
static const uint16_t sci_rx_isr_on_count = 1;          //call interrupt (need at least 10 spaces to handle a max of 100us in the main interrupt)
static const uint16_t sci_rx_ring_buffer_len = 1000;    //about 92bits could be transferred in the max 1ms main interrupt, so about 5.5X that
static volatile ring_buffer sci_rx_ring_buffer;
__interrupt void sci_rx_isr(void);
static volatile bool had_scia_rx_buffer_full_error = false;
static volatile bool had_scia_rx_buffer_overflow_error = false;

//TX
static const uint16_t sci_tx_isr_on_count = 12;         //call interrupt (need at least 10 spaces to handle a max of 100us in the main interrupt)
static const uint16_t sci_tx_ring_buffer_len = 1000;    //32x the size of the FIFO buffer (no rhyme or reason)
static volatile ring_buffer sci_tx_ring_buffer;
__interrupt void sci_tx_isr(void);


//send character directly to buffer (only internal)
void scia_send_char(uint16_t a_char);


//this init makes assumptions about the clock rate, will need to be a parameter
//init
bool init_scia(uint32_t baud_rate, const sci_gpio_t sci_gpio_settings) {
    if (scia_initialized) {return true;}

    //Create the ring buffers (always)
    if (!sci_rx_ring_buffer.init_alloc(sci_rx_ring_buffer_len)) {
        return false;
    }

    if (!sci_tx_ring_buffer.init_alloc(sci_tx_ring_buffer_len)) {
        return false;
    }


    //Turn on the appropriate pins and mux
    GPIO_SetupPinMux(sci_gpio_settings.rx_gpio, GPIO_MUX_CPU1, sci_gpio_settings.rx_mux);
    GPIO_SetupPinOptions(sci_gpio_settings.rx_gpio, GPIO_INPUT, GPIO_ASYNC);
    GPIO_SetupPinMux(sci_gpio_settings.tx_gpio, GPIO_MUX_CPU1, sci_gpio_settings.tx_mux);
    GPIO_SetupPinOptions(sci_gpio_settings.tx_gpio, GPIO_OUTPUT, GPIO_ASYNC);

#pragma diag_suppress 1463
    EALLOW;

    //enable the clock
    CpuSysRegs.PCLKCR7.bit.SCI_A = 1;   //clock enabled for scia

    //Set the interrupt functions
    PieVectTable.SCIA_RX_INT = &sci_rx_isr;
    PieVectTable.SCIA_TX_INT = &sci_tx_isr;

    // 8 char bits, 1 stop bit, no parity
    SciaRegs.SCICCR.bit.SCICHAR = 7;        // 0-2 8 bits per char
    SciaRegs.SCICCR.bit.ADDRIDLE_MODE = 0;  // 3   multi-processor
    SciaRegs.SCICCR.bit.LOOPBKENA = 0;      // 4   loopback
    SciaRegs.SCICCR.bit.PARITYENA = 0;      // 5   parity enable
    SciaRegs.SCICCR.bit.PARITY = 0;         // 6   parity odd
    SciaRegs.SCICCR.bit.STOPBITS = 0;       // 7   1 stop bit

    //These settings are calculated based on the actual clock frequency
    SciaRegs.SCIHBAUD.all = 0x0000; //HI bits of baud, always 0

    //calculate and check that the baud is possible
    const int32_t temp_scibaud = lroundl(((ti_board.get_main_clock_freq()/static_cast<float64>(baud_rate))/8.0L) - 1.0L);
    if ((temp_scibaud < 1) || (temp_scibaud > 255)) {
        printf_json_objects(2, json_string("error", "sci_baud is impossible"), json_int32("SCILBAUD", temp_scibaud));
        return false;
    }
    SciaRegs.SCILBAUD.all = static_cast<uint16_t>(temp_scibaud);

    //enable TX, RX, internal SCICLK, Disable RX ERR, SLEEP, TXWAKE
    SciaRegs.SCICTL1.bit.RXENA = 1;         // 0 SCI receiver enable
    SciaRegs.SCICTL1.bit.TXENA = 1;         // 1 SCI transmitter enable
    SciaRegs.SCICTL1.bit.SLEEP = 0;         // 2 SCI sleep
    SciaRegs.SCICTL1.bit.TXWAKE = 0;        // 3 Transmitter wakeup method
    SciaRegs.SCICTL1.bit.SWRESET = 0;       // 5 Software reset
    SciaRegs.SCICTL1.bit.RXERRINTENA = 0;   // 6 Receive error __interrupt enable

    //enable the interrupts
    SciaRegs.SCICTL2.bit.TXINTENA = 1;      // 0 Transmit interrupt enable
    SciaRegs.SCICTL2.bit.RXBKINTENA = 1;    // 1 Receive interrupt enable

    //Initialize SCI FIFO
    //TX FIFO
    SciaRegs.SCIFFTX.bit.TXFFIL = sci_tx_isr_on_count;    // 4:0 Interrupt level
    SciaRegs.SCIFFTX.bit.TXFFIENA = 1;      // 5 Interrupt enable
    SciaRegs.SCIFFTX.bit.TXFFINTCLR = 1;    // 6 not not clear INT flag
    SciaRegs.SCIFFTX.bit.TXFIFORESET = 0;   // 13 FIFO reset
    SciaRegs.SCIFFTX.bit.SCIFFENA = 1;      // 14 Enhancement enable (both TX and RX FIFO enable)
    SciaRegs.SCIFFTX.bit.SCIRST = 1;        // 15 SCI reset rx/tx channels

    //RX FIFO
    SciaRegs.SCIFFRX.bit.RXFFIL = sci_rx_isr_on_count;    // 4:0 Interrupt level
    SciaRegs.SCIFFRX.bit.RXFFIENA = 1; // 5 Interrupt enable
    SciaRegs.SCIFFRX.bit.RXFFINTCLR = 1;    // 6 Clear INT flag
    SciaRegs.SCIFFRX.bit.RXFFOVRCLR = 1;
    SciaRegs.SCIFFRX.bit.RXFIFORESET = 0;   // 13 FIFO reset

    //FIFO control
    SciaRegs.SCIFFCT.bit.FFTXDLY = 0;       // 7:0 FIFO transmit delay
    SciaRegs.SCIFFCT.bit.CDC = 0;           // 13 Auto baud mode enable
    SciaRegs.SCIFFCT.bit.ABDCLR = 0;        // 14 Auto baud clear
    SciaRegs.SCIFFCT.bit.ABD = 0;           // 15 Auto baud detect

    //Relinquish SCI from Reset (keeping enabled TX, RX)
    SciaRegs.SCICTL1.bit.SWRESET = 1;   // 5 Software reset

    //Enable the FIFO reset
    SciaRegs.SCIFFTX.bit.TXFIFORESET = 1;
    SciaRegs.SCIFFRX.bit.RXFIFORESET = 1;

    //enable the pie interrupts
    PieCtrlRegs.PIEIER9.bit.INTx1 = 1;  // PIE Group 9, INT1   RX
    PieCtrlRegs.PIEIER9.bit.INTx2 = 1;  // PIE Group 9, INT2   TX
    IER |= M_INT9;       // Enable CPU INT

    EDIS;
#pragma diag_default 1463

    scia_initialized = true;
    return true;
}

//ISRs
__attribute__((ramfunc))
__interrupt void sci_rx_isr(void) {

    //if the overflow happened, flag it
    if (SciaRegs.SCIFFRX.bit.RXFFOVF > 0) {
        delay_printf_json_error("scia rx buffer overflow");
        had_scia_rx_buffer_overflow_error = true;
        SciaRegs.SCIFFRX.bit.RXFFOVRCLR = 1;
    }

    //read and copy each byte from the fifo buffer to the ring buffer
    while (SciaRegs.SCIFFRX.bit.RXFFST > 0) {

        //will overwrite if necessary
        const uint16_t incoming_byte = SciaRegs.SCIRXBUF.bit.SAR;

        //if the buffer is full
        if (sci_rx_ring_buffer.is_full()) {
            delay_printf_json_error("scia rx buffer full");
            had_scia_rx_buffer_full_error = true;
        }

        if (!sci_rx_ring_buffer.write(incoming_byte)) {
            lockup_cpu();
        }
    }

    //clear the flag and acknowledge the isr
    SciaRegs.SCIFFRX.bit.RXFFINTCLR = 1;    // Clear Interrupt flag
    PieCtrlRegs.PIEACK.bit.ACK9 = 1;        // Issue PIE ack for SCIA interrupt (group 9)
}

//volatile uint16_t tx_was_empty_count = 0;

__attribute__((ramfunc))
__interrupt void sci_tx_isr(void) {
    uint16_t outgoing_byte;

    //if the FIFO isn't full, and there are bytes in the ring buffer, copy them to the FIFO buffer
    while ((SciaRegs.SCIFFTX.bit.TXFFST < sci_fifo_buffer_max) && (!sci_tx_ring_buffer.is_empty())) {

        //copy bytes to the FIFO buffer
        if (sci_tx_ring_buffer.read(outgoing_byte)) {
            SciaRegs.SCITXBUF.bit.TXDT = outgoing_byte;
            if (outgoing_byte == '}') {
                debug_timestamps.scia_tx_close_bracket = CPU_TIMESTAMP;
            }
        }
    }

    //if there are still bytes in the ring buffer, clear the SCI interrupt flag,
    //so it can be called again
    if (!sci_tx_ring_buffer.is_empty()) {
        SciaRegs.SCIFFTX.bit.TXFFINTCLR = 1;    // Clear SCI Interrupt flag
    }

    //acknowledge the isr
    PieCtrlRegs.PIEACK.bit.ACK9 = 1;        // Issue PIE ack for SCIA interrupt (group 9)
}

//transmit characters
__attribute__((ramfunc))
void scia_send_char(uint16_t send_char) {
    if (send_char > 255) {return;}  // only allow 7bit char

    //if the ring buffer is full, wait, then add
    while (sci_tx_ring_buffer.is_full()) {
    }

    if (!sci_tx_ring_buffer.write(send_char)) {
        lockup_cpu();
    }

    //wait until there is at least one buffer full, or a return has been issued,
    //then, allow interrupt to start again
    if ((sci_tx_ring_buffer.in_use() > sci_fifo_buffer_max) || (send_char == 10)) {
        //allow the interrupt to start again
        SciaRegs.SCIFFTX.bit.TXFFINTCLR = 1;    // Clear SCI Interrupt flag
    }

}

//read from rx buffer
__attribute__((ramfunc))
bool scia_rx_buffer_read(uint16_t &rx_buffer_byte) {
    return sci_rx_ring_buffer.read(rx_buffer_byte);
}

void wait_until_scia_tx_buffer_is_empty() {
    //wait while either the FIFO or ring buffer still have bytes in them
    while ((SciaRegs.SCIFFTX.bit.TXFFST > 0) || (!sci_tx_ring_buffer.is_empty())) {
    }
}

__attribute__((ramfunc))
void printf_string(const char *const string) {
    for (char *c = const_cast<char*>(string); (*c) != 0; c++) {
        scia_send_char(static_cast<uint16_t>(*c));
    }
}

__attribute__((ramfunc))
void printf_char(uint16_t c) {
    scia_send_char(c);
}

__attribute__((ramfunc))
void printf_return() {
    //CR+LF to be compatible with windows
    scia_send_char(13);
    scia_send_char(10);
}
