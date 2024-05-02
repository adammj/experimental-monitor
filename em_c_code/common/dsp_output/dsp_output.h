/*
 * dsp_output.h
 *
 *  Created on: Mar 21, 2017
 *      Author: adam jones
 */

/*
    Code assumes that the DSP connection has X bits and a strobe. (where X is set by the user)
    The bits are set first, then the strobe is toggled on then off, and then the bits are cleared
    (to make it easier to see with a scope)

As of May 1, 2018, I've tested codes at 5 kHz on both Blackrock and Plexon
This is on for 0.1ms and off for 0.1ms.


 */

#ifndef dsp_output_defined
#define dsp_output_defined

#include <stdbool.h>
#include <stdint.h>
#include "tiny_json.h"
#include "F28x_Project.h"


typedef struct dsp_settings_t {
    uint16_t dsp_strobe_gpio;
    uint16_t dsp_bit_count;
    const uint16_t *dsp_bit_gpio;
} dsp_settings_t;


//functions
bool init_dsp_output(const dsp_settings_t dsp_settings);
void send_high_priority_event_code_at_front_of_queue(uint16_t code);
void send_high_priority_event_code(uint16_t code);
void send_event_codes_with_json(const json_t *const json_root);
void get_event_code_queue_available_void();
void get_event_code_queue_available(bool send_info = false, uint32_t id = 0, uint32_t i = 0, uint32_t count = 0);
void set_event_code_queue_notification(const json_t *const json_root);

//only for use inside the time_sensitive code
void process_dsp_event_code_queues(int64_t uptime_count);

//constant count up
void set_dsp_testing_mode(const json_t *const json_root);


#endif
