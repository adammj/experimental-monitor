/*
 * cla.c
 *
 *  Created on: Mar 5, 2017
 *      Author: adam jones
 */


#include "cla.h"
#include "printf_json_delayed.h"
#include "string.h"


//internal variables
static const uint16_t cla_task_count = 8;
static bool is_cla_task_enabled[cla_task_count];

//internal functions
void set_cla_task(uint16_t task_number, bool enable, uint16_t cla_function_addr, uint16_t trigger);


//initialize the cla
void init_cla(void) {
#pragma diag_suppress 1463
    //******** configure the memory first ********
    extern uint32_t Cla1funcsRunStart, Cla1funcsLoadStart, Cla1funcsLoadSize;
    extern uint32_t Cla1ConstRunStart, Cla1ConstLoadStart, Cla1ConstLoadSize;
    EALLOW;

    //enable the clock
    CpuSysRegs.PCLKCR0.bit.CLA1 = 1;

#ifdef _FLASH
    //
    // Copy over code from FLASH to RAM
    //
    memcpy((uint32_t *)&Cla1funcsRunStart, (uint32_t *)&Cla1funcsLoadStart, (uint32_t)&Cla1funcsLoadSize);
    // Copy CLA_const
    memcpy((uint32_t *)&Cla1ConstRunStart, (uint32_t *)&Cla1ConstLoadStart, (uint32_t)&Cla1ConstLoadSize);
#endif //_FLASH

    // Initialize and wait for cla_to_cpu_msg_ram
    MemCfgRegs.MSGxINIT.bit.INIT_CLA1TOCPU = 1;
    while (MemCfgRegs.MSGxINITDONE.bit.INITDONE_CLA1TOCPU != 1) {
    }

    // Initialize and wait for cpu_to_cla_msg_ram
    MemCfgRegs.MSGxINIT.bit.INIT_CPUTOCLA1 = 1;
    while (MemCfgRegs.MSGxINITDONE.bit.INITDONE_CPUTOCLA1 != 1) {
    }

    // Configure the programming space for the CLA
    // First configure to be shared with CLA and then
    // set the spaces to be program blocks (blocks out CPU access)
    MemCfgRegs.LSxMSEL.bit.MSEL_LS0 = 1;        //CLA accessible
    MemCfgRegs.LSxCLAPGM.bit.CLAPGM_LS0 = 1;    //program block (no CPU access)

    // Configure the data space for the CLA
    // First configure to be shared with CLA and then
    // set the spaces to be data blocks (still CPU accessible)
    MemCfgRegs.LSxMSEL.bit.MSEL_LS1 = 1;        //CLA accessible
    MemCfgRegs.LSxCLAPGM.bit.CLAPGM_LS1 = 0;    //data block (CPU access)

    // Enable the IACK instruction to start a task on CLA in software
    // for all 8 CLA tasks.
    Cla1Regs.MCTL.bit.IACKE = 1;

    //Enable CLA post-task interrupts
    IER |= (M_INT11);

    EDIS;
#pragma diag_default 1463

    //set all cla to disabled
    for (uint16_t i=0; i<cla_task_count; i++) {
        is_cla_task_enabled[i] = false;
    }

}

void enable_cla_task(uint16_t task_number, uint16_t cla_function_addr, uint16_t trigger) {
    if (task_number > cla_task_count) {
        delay_printf_json_error("cla task number is too high");
        return;
    }

    if (task_number == 0) {
        delay_printf_json_error("cla task number cannot be 0");
        return;
    }

    if (is_cla_task_enabled[task_number - 1]) {
        delay_printf_json_error("cla task is already enabled");
        return;
    }


    set_cla_task(task_number, true, cla_function_addr, trigger);
    is_cla_task_enabled[task_number - 1] = true;
}
void disable_cla_task(uint16_t task_number) {
    if (task_number > cla_task_count) {
        delay_printf_json_error("cla task number is too high");
        return;
    }

    if (task_number == 0) {
        delay_printf_json_error("cla task number cannot be 0");
        return;
    }

    set_cla_task(task_number, false, 0, 0);
    is_cla_task_enabled[task_number - 1] = false;
}

void set_cla_task(uint16_t task_number, bool enable, uint16_t cla_function_addr, uint16_t trigger) {
    if (task_number > cla_task_count) {
        delay_printf_json_error("cla task number is too high");
        return;
    }

    if (task_number == 0) {
        delay_printf_json_error("cla task number cannot be 0");
        return;
    }

#pragma diag_suppress 1463
    EALLOW;

    switch (task_number) {
        case 1:
            Cla1Regs.MIER.bit.INT1 = enable;
            Cla1Regs.MVECT1 = cla_function_addr;
            DmaClaSrcSelRegs.CLA1TASKSRCSEL1.bit.TASK1 = trigger;
            break;
        case 2:
            Cla1Regs.MIER.bit.INT2 = enable;
            Cla1Regs.MVECT2 = cla_function_addr;
            DmaClaSrcSelRegs.CLA1TASKSRCSEL1.bit.TASK2 = trigger;
            break;
        case 3:
            Cla1Regs.MIER.bit.INT3 = enable;
            Cla1Regs.MVECT3 = cla_function_addr;
            DmaClaSrcSelRegs.CLA1TASKSRCSEL1.bit.TASK3 = trigger;
            break;
        case 4:
            Cla1Regs.MIER.bit.INT4 = enable;
            Cla1Regs.MVECT4 = cla_function_addr;
            DmaClaSrcSelRegs.CLA1TASKSRCSEL1.bit.TASK4 = trigger;
            break;
        case 5:
            Cla1Regs.MIER.bit.INT5 = enable;
            Cla1Regs.MVECT5 = cla_function_addr;
            DmaClaSrcSelRegs.CLA1TASKSRCSEL2.bit.TASK5 = trigger;
            break;
        case 6:
            Cla1Regs.MIER.bit.INT6 = enable;
            Cla1Regs.MVECT6 = cla_function_addr;
            DmaClaSrcSelRegs.CLA1TASKSRCSEL2.bit.TASK6 = trigger;
            break;
        case 7:
            Cla1Regs.MIER.bit.INT7 = enable;
            Cla1Regs.MVECT7 = cla_function_addr;
            DmaClaSrcSelRegs.CLA1TASKSRCSEL2.bit.TASK7 = trigger;
            break;
        case 8:
            Cla1Regs.MIER.bit.INT8 = enable;
            Cla1Regs.MVECT8 = cla_function_addr;
            DmaClaSrcSelRegs.CLA1TASKSRCSEL2.bit.TASK8 = trigger;
            break;
    }

    EDIS;
#pragma diag_default 1463
}
