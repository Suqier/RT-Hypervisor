/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-05-30     Suqier       first version
 */

#ifndef __HYPERVISOR_H__
#define __HYPERVISOR_H__

#include <rthw.h>
#include <rtthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <optparse.h>
#include <virt_arch.h>

#include "rtconfig.h"
#include "vm.h"

#ifdef RT_USING_NVHE
#include "nvhe/nvhe.h"
#else
#include "vhe/vhe.h"
#endif

#ifndef MAX_VM_NUM
#define MAX_VM_NUM  8
#endif

/*
 * Need change these macro to CONFIG_XXX.
 */
#define HYP_MEM_SIZE    64 * (1024) * (1024)

struct hypervisor
{
    rt_uint64_t phy_mem_size;
    rt_uint64_t phy_mem_used;

#ifdef RT_USING_SMP
    rt_hw_spinlock_t hyp_lock;
#endif

    rt_uint32_t vm_bitmap;
    rt_uint8_t next_vm_idx;
    rt_uint8_t curr_vm_idx;
    rt_uint8_t total_vm;
    vm_t vms[MAX_VM_NUM];

    rt_uint8_t curr_vc_idx;

    struct hyp_arch arch;
};

extern struct hypervisor rt_hyp;

rt_err_t rt_hypervisor_init(void);

void list_os_img(void);
void list_vm(void);
void help_vm(void);
rt_err_t create_vm(int argc, char **argv);
void pick_vm(int argc, char **argv);
rt_err_t run_vm(void);
rt_err_t pause_vm(void);
rt_err_t halt_vm(void);
rt_err_t delete_vm(void);

#endif  /* __HYPERVISOR_H__ */ 