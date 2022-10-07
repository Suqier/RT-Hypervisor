/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-30     Suqier       first version
 */

#include "os.h"

const struct os_desc os_img[MAX_OS_NUM] =
{
    {   /* for QEMU */
        .img = 
        {
            .addr = 0x45000000,
            .size = 0x29CD0,
            .ep   = 0x40008000,
            .type = OS_TYPE_RT_THREAD,
        },
        .cpu = 
        {
            .affinity = /* for SMP, not use now */
            {
                0x0,
            },
            .num = 1,
        },
        .mem = 
        {
            .addr = 0x40000000,
            .size = 8,          /* default MB */
        },
        .devs = 
        {
            .num = 1,
            .dev = (struct dev_info[]) 
            {
                {   /* QEMU UART0 */
                    .paddr = 0x09000000,  /* PL011_UART0_BASE */
                    .vaddr = 0x09000000,  /* flat mapping */
                    .size  = 0x00001000,
                    .interrupt_num = 1,
                    .interrupts = (rt_uint64_t[]) {32 + 1},
                },
            },
        },
        .arch = 
        {
            .vgic = 
            {
                .gicd_addr = 0x08000000,
                .gicr_addr = 0x080A0000,
            },
        },
    },
    /* <-------------------------------------------------------------------> */
    {   /* for Cortex-A55 */
        .img = 
        {
            .addr = 0x00208000,
            .size = 0x1DC80,
            .ep   = 0x00208000,
            .type = OS_TYPE_RT_THREAD,
        },
        .cpu = 
        {
            .affinity =    /* for SMP, not use now */
            {
                0x0
            },
            .num = 1,
        },
        .mem = 
        {
            .addr = 0x40000000,
            .size = 8,          /* default MB */
        },
        .devs = 
        {
            .num = 1,
            .dev = (struct dev_info[]) 
            {
                {   /* RK3568 UART2 */
                    .paddr = 0xFE650000 + 0x10000,
                    .vaddr = 0xFE650000 + 0x10000,
                    .size  = 0x00001000,
                    .interrupt_num = 1,
                    .interrupts = (rt_uint64_t[]) {32 + 116 + 2},
                },
            },
        },
        .arch = 
        {
            .vgic = 
            {
                .gicd_addr = 0xFD400000,
                .gicr_addr = 0xFD460000,
            },
        },
    },
};
