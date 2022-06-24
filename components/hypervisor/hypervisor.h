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

// #if defined(RT_HYPERVISOR) 
#include "vm.h"

/*
 * Need change these macro to CONFIG_XXX.
 */

#define HYP_MEM_SIZE    64 * (1024) * (1024)

struct hypervisor
{
    rt_uint64_t phy_mem_size;
    rt_uint64_t phy_mem_used;
    
    rt_hw_spinlock_t hyp_lock;

    rt_uint16_t next_vm_idx;
    rt_uint16_t curr_vm_idx;
    
    rt_uint16_t total_vm;
    struct vm *vms[MAX_VM_NUM];

    struct hyp_arch *arch;
};

rt_err_t rt_hypervisor_init(void);

long list_vm(void);
void help_vm(void);
rt_err_t create_vm(int argc, char **argv);
rt_err_t setup_vm(int argc, char **argv);
rt_err_t run_vm(int vm_idx);
rt_err_t pause_vm(int vm_idx);
rt_err_t halt_vm(int vm_idx);
rt_err_t delete_vm(rt_uint16_t vm_idx);

// #endif  /* defined(RT_HYPERVISOR) */ 

#endif  /* __HYPERVISOR_H__ */ 