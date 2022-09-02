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
#include "vm.h"

/* 
 * OS ops implement for RT-Thread 
 */

void rt_thread_vm_init(struct vm *vm)
{
    /* do nothing */
}

void rt_thread_vcpu_init(struct vcpu *vcpu)
{
    

}
    
void rt_thread_vcpu_power_on(struct vcpu *vcpu, rt_uint64_t entry_point)
{

}

struct os_ops os_rt_thread =
{
    .os_type = OS_TYPE_RT_THREAD,
    .vm_init = rt_thread_vm_init,
    .vcpu_init = rt_thread_vcpu_init,
    .vcpu_power_on = rt_thread_vcpu_power_on
};