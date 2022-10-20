/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-09-06     Suqier       first version
 */

#ifndef __VDEV_H__
#define __VDEV_H__

#include <rthw.h>
#include <rtdef.h>
#include <mmu.h>

#include "trap.h"

#define MAX_MMAP_NUM    2

struct vdev_ops
{
    void (*mmio)(gp_regs_t regs, access_info_t acc);
};

struct vdev
{
    rt_device_t dev;
    struct vm *vm;

    rt_uint8_t mmap_num;
    struct mem_desc region[MAX_MMAP_NUM];   /* mmap region */

    const struct vdev_ops *ops;
    rt_list_t node;
    rt_bool_t is_open;
};
typedef struct vdev *vdev_t;

#endif  /* __VDEV_H__ */