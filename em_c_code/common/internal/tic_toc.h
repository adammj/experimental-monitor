/*
 * tic_toc.h
 *
 *  Created on: Apr 19, 2017
 *      Author: adam jones
 */

#ifndef tic_toc_defined
#define tic_toc_defined

#include "F28x_Project.h"
#include "tiny_json.h"

extern float32 cla_us_per_div;
extern float32 cpu_us_per_div;
extern float32 cpu_hs_us_per_div;
extern float32 cpu_ts_tics_us_per_div;


//init clocks
void init_tic_toc(void);
void calibrate_with_sync(const json_t *const json_root);


class tic_toc {
    public:
        tic_toc();
        //uses freerunning timer on 79D or hs_cpu timer on 77S
        void tic() volatile;
        void toc() volatile;
        void reset() volatile;

        uint32_t tics() volatile const {return tics_;}
        uint32_t max_tics() volatile const {return max_tics_;}
        float32 us() volatile const;
        float32 max_us() volatile const;

    private:
        uint32_t last_tic_;
        uint32_t tics_;
        uint32_t max_tics_;
};


//timestamp define
#ifdef _LAUNCHXL_F28379D
    //the free-running 64bit counter only exists of the 79D
    //only need the lower 32bits (2^32 * 1/200Mhz is ~21sec)
    #define CPU_TIMESTAMP   IpcRegs.IPCCOUNTERL

#else
    //no timestamp for the 77S
    #define CPU_TIMESTAMP   0

#endif


__attribute__((ramfunc))
inline void update_cpu_cycles(const uint32_t start_time, volatile uint32_t &cycles, volatile uint32_t &max_cycles) {
    cycles = CPU_TIMESTAMP - start_time;

    if (cycles > max_cycles) {
        max_cycles = cycles;
    }
}

//keep all timestamps in one struct
typedef struct debug_ts_ {
    //for caring about actual timestamps
    uint32_t main_b_scia;
    uint32_t main_b_printf_delayed;
    uint32_t main_a_printf_delayed;
    uint32_t dsp_s_processing;
    uint32_t dsp_b_send_code;
    uint32_t dsp_a_send_code;
    uint32_t scia_rx_open_bracket;
    uint32_t scia_tx_close_bracket;
    uint32_t printf_s_print;
    uint32_t printf_s_loop;
    uint32_t printf_b_read_i;
    uint32_t printf_a_read_i;
    uint32_t printf_is_valid;
    uint32_t printf_a_count;
    uint32_t printf_b_obj_loop;
    uint32_t printf_s_obj_loop;
    uint32_t printf_a_copy;
    uint32_t printf_a_print;
    uint32_t printf_b_delete;
    uint32_t printf_a_delete;
    uint32_t serial_s_bracket;
    uint32_t serial_call_func;
    uint32_t io_print_history_1;
    uint32_t io_print_history_2;
    uint32_t io_print_history_3;
    uint32_t io_print_history_4;
    uint32_t delay_printf_1;
    uint32_t delay_printf_2;

    uint32_t exp_in_print_1;
    uint32_t exp_in_print_2;
    uint32_t exp_in_print_3;
    uint32_t exp_in_print_4;
    uint32_t exp_in_print_5;

    //for just caring about cycle times
    tic_toc exp_input_set_settings;
    tic_toc exp_output_set_settings;
    tic_toc io_set_print_history;
    tic_toc io_send_print_history;

} debug_ts_t;
extern volatile debug_ts_t debug_timestamps;


#pragma diag_suppress 1463

//read the time in microseconds
float32 cla_cycles_in_us(uint16_t cycles);
float32 cpu_cycles_in_us(uint16_t cycles);
float32 cpu_hs_cycles_in_us(uint16_t cycles);
float32 cpu_ts_tics_in_us(uint32_t ts_tics);

//restart clocks
void cpu_hs_tic(void);

//read in cycles
uint16_t cpu_hs_toc(void);

#pragma diag_default 1463


#endif
