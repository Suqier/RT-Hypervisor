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

#include "virt_arch.h"

enum
{
    OS_TYPE_LINUX = 0,
    OS_TYPE_RT_THREAD,
    OS_TYPE_RT_ZEPHYR,
    OS_TYPE_OTHER,
};

/*
 * struct os_desc describes a OS's basic information:
 * -> OS image info
 * -> CPU related info
 * -> Memory layout
 * -> device detail
 * -> arch related info
 * -> ......
 * 
 * The device tree format will be used later.
 */
struct img_info
{
    rt_uint64_t addr;
    rt_uint64_t size;
    rt_uint64_t ep;      /* OS entry point */
    rt_uint8_t  type;    /* OS_TYPE_ENUM */
};

struct cpu_info
{
    rt_uint64_t affinity;
    rt_uint8_t  num;
};

struct mem_info          /* only one memory region supported now */
{
    rt_uint64_t addr;    /* mmap basic address */
    rt_uint64_t size;    /* default / MB */
};

struct dev_info
{
    rt_uint64_t paddr;
    rt_uint64_t vaddr;
    rt_uint64_t size;
    rt_uint64_t *interrupts;
    rt_uint8_t  interrupt_num;
};

struct devs_info
{
    struct dev_info *dev;
    rt_uint8_t num;
};

struct os_ops
{
    rt_uint16_t os_type;
    void (*vm_init)(struct vm *vm);
    void (*vcpu_init)(struct vcpu *vcpu);
    void (*vcpu_power_on)(struct vcpu *vcpu, rt_uint64_t ep);
};

struct os_desc
{
    struct img_info img;
    struct cpu_info cpu;
    struct mem_info mem;
    struct devs_info devs;
    struct arch_info arch;
    struct os_ops *ops;
};

#endif  /* __OS_H__ */