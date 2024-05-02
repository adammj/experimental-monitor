/*
 * cla_ptr.h
 *
 *  Created on: Apr 26, 2017
 *      Author: adam jones
 */

#ifndef cla_pts_defined
#define cla_pts_defined

#include <stdint.h>

//union to allow buffer to correctly work with cpu and cla (different pointer lengths)
typedef union {
    uint16_t *ptr;  //aligned to lower 16-bits
    uint32_t pad;   //32-bits
} aligned_uint16_t_ptr;

typedef union {
    volatile uint16_t *ptr;  //aligned to lower 16-bits
    uint32_t pad;   //32-bits
} aligned_v_uint16_t_ptr;

typedef union {
    uint32_t *ptr;  //aligned to lower 16-bits
    uint32_t pad;   //32-bits
} aligned_uint32_t_ptr;

typedef union {
    volatile uint16_t *ptr;  //aligned to lower 16-bits
    uint32_t pad;   //32-bits
} volatile_aligned_uint16_t_ptr;

typedef union {
    volatile uint32_t *ptr;  //aligned to lower 16-bits
    uint32_t pad;   //32-bits
} volatile_aligned_uint32_t_ptr;

#endif
