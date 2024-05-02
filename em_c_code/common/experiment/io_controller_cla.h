/*
 * io_controller_cla.h
 *
 *  Created on: Jan 12, 2018
 *      Author: adam jones
 */


#ifndef io_controller_cla_defined
#define io_controller_cla_defined

#include "F28x_Project.h"

extern volatile uint32_t cla_uptime_count_32;
extern volatile uint32_t cla_uptime_overflow;


//necessary for cla function to be seen
#ifdef __cplusplus
extern "C" {
#endif

__interrupt void cla_task2_cla_uptime(void);


#ifdef __cplusplus
}
#endif

#endif
