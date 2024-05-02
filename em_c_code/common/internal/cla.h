/*
 * cla.h
 *
 *  Created on: Mar 5, 2017
 *      Author: adam jones
 */

//Initializes the CLA: copies from FLASH (if necessary), and claims the necessary ram for program and data.

#ifndef cla_defined
#define cla_defined

#include "F28x_Project.h"

typedef void (*cla_task_function_ptr) (void);


void init_cla(void);

//enable and disable cla tasks
//the cla_function_addr must be type cast to uin16_t
void enable_cla_task(uint16_t task_number, uint16_t cla_function_addr, uint16_t trigger);
void disable_cla_task(uint16_t task_number);


#endif
