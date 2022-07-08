/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-30     Suqier       first version
 */

#ifndef __OS_H__
#define __OS_H__

#include <rtdef.h>

struct vm;

enum
{
    OS_TYPE_LINUX = 0,
    OS_TYPE_RT_THREAD,
    OS_TYPE_RT_ZEPHYR,
    OS_TYPE_OTHER,
};

struct os_ops
{
    rt_uint16_t os_type;
    void (*vm_init)(struct vm *vm);
    void (*vcpu_init)(struct vcpu *vcpu);
    void (*vcpu_power_on)(struct vcpu *vcpu, rt_uint64_t entry_point);
};

struct os_desc
{
    rt_uint64_t mm_size;    /* default / MB */
    rt_uint16_t os_type;    /* OS_TYPE_ENUM */
    rt_uint8_t nr_vcpus;    /* default */

    rt_uint64_t entry_point;

    rt_uint64_t os_addr;    /* without DFS, using memory address directly */
    rt_uint64_t ramdisk_addr;
    rt_uint64_t dtb_addr;

    struct os_ops *ops;
};




#endif  /* __OS_H__ */