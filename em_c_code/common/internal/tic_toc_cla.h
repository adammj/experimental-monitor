/*
 * tic_toc_cla.h
 *
 *  Created on: Jan 30, 2018
 *      Author: adam jones
 */

#include "F28x_Project.h"


//definitions used for the regs
#define CLA_EPWM        EPwm1Regs
#define CPU_EPWM        EPwm2Regs
#define CPU_HS_EPWM     EPwm3Regs
#define TIMER0_EPWM     EPwm4Regs
#define TIMER1_EPWM     EPwm5Regs
#define TIMER2_EPWM     EPwm6Regs
#define SYNC_EPWM       EPwm7Regs


//wrapper for the cla functions
#ifdef __cplusplus
extern "C" {
#endif

void cla_tic(void);   //cla-only
uint16_t cla_toc(void);  //cla-only

#ifdef __cplusplus
}
#endif
