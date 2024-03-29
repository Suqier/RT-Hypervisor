/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2011-09-15     Bernard      first version
 * 2021-12-28     GuEe-GUI     add fpu support
 * 2022-06-02     Suqier       add cpu register macro
 */

#ifndef __ARMV8_H__
#define __ARMV8_H__

#include <rtdef.h>

/* the exception stack without VFP registers */
struct rt_hw_exp_stack
{
    unsigned long long pc;
    unsigned long long spsr;
    unsigned long long x30;
    unsigned long long xzr;
    unsigned long long fpcr;
    unsigned long long fpsr;
    unsigned long long x28;
    unsigned long long x29;
    unsigned long long x26;
    unsigned long long x27;
    unsigned long long x24;
    unsigned long long x25;
    unsigned long long x22;
    unsigned long long x23;
    unsigned long long x20;
    unsigned long long x21;
    unsigned long long x18;
    unsigned long long x19;
    unsigned long long x16;
    unsigned long long x17;
    unsigned long long x14;
    unsigned long long x15;
    unsigned long long x12;
    unsigned long long x13;
    unsigned long long x10;
    unsigned long long x11;
    unsigned long long x8;
    unsigned long long x9;
    unsigned long long x6;
    unsigned long long x7;
    unsigned long long x4;
    unsigned long long x5;
    unsigned long long x2;
    unsigned long long x3;
    unsigned long long x0;
    unsigned long long x1;

    unsigned long long fpu[16];
};
typedef struct rt_hw_exp_stack *gp_regs_t;

#define SP_ELx                      ( ( unsigned long long ) 0x01 )
#define SP_EL0                      ( ( unsigned long long ) 0x00 )
#define PSTATE_EL1                  ( ( unsigned long long ) 0x04 )
#define PSTATE_EL2                  ( ( unsigned long long ) 0x08 )
#define PSTATE_EL3                  ( ( unsigned long long ) 0x0c )

static rt_uint16_t xn_offset[32] = 
{
     [0] = 0x110,  [1] = 0x118,  [2] = 0x100,  [3] = 0x108, 
     [4] = 0x0F0,  [5] = 0x0F8,  [6] = 0x0E0,  [7] = 0x0E8, 
     [8] = 0x0D0,  [9] = 0x0D8, [10] = 0x0C0, [11] = 0x0C8, 
    [12] = 0x0B0, [13] = 0x0B8, [14] = 0x0A0, [15] = 0x0A8, 
    [16] = 0x090, [17] = 0x098, [18] = 0x080, [19] = 0x088, 
    [20] = 0x070, [21] = 0x078, [22] = 0x060, [23] = 0x068, 
    [24] = 0x050, [25] = 0x058, [26] = 0x040, [27] = 0x048, 
    [28] = 0x030, [29] = 0x038, [30] = 0x010, [31] = 0x018, 
};

rt_inline unsigned long long *regs_xn(struct rt_hw_exp_stack *r, rt_uint32_t n)
{
    return (unsigned long long *)((rt_uint64_t)r + (rt_uint64_t)xn_offset[n]);
}


rt_ubase_t rt_hw_get_current_el(void);
void rt_hw_set_elx_env(void);
void rt_hw_set_current_vbar(rt_ubase_t addr);

#endif
