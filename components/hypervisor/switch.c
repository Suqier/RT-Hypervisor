/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-09-02     Suqier       first version
 */

#include "switch.h"
#include "virt_arch.h"

rt_bool_t is_vcpu_thread(rt_thread_t thread)
{
    return thread->vcpu != RT_NULL;
}

rt_uint8_t vcpu_thread_vm_idx(rt_thread_t thread)
{
    RT_ASSERT(is_vcpu_thread(thread));
    return thread->vcpu->vm->vm_idx;
}

rt_bool_t is_vcpu_threads_same_vm(rt_thread_t from, rt_thread_t to)
{
    RT_ASSERT(is_vcpu_thread(from) && is_vcpu_thread(to));
    return vcpu_thread_vm_idx(from) == vcpu_thread_vm_idx(to);
}

rt_uint8_t this_switch_type(rt_thread_t from, rt_thread_t to)
{
    rt_bool_t is_from_vcpu, is_to_vcpu;
    is_from_vcpu = is_vcpu_thread(from);
    is_to_vcpu = is_vcpu_thread(to);
    
    if (is_from_vcpu == RT_FALSE && is_to_vcpu == RT_FALSE)
        return HOST_TO_HOST;
    else
    {
        if (is_from_vcpu && is_to_vcpu)
        {
            if(is_vcpu_threads_same_vm(from, to))
                return VCPU_TO_VCPU;
            else
                return GUEST_TO_GUEST;
        }
        else
        {
            if (is_from_vcpu)
                return GUEST_TO_HOST;
            else
                return HOST_TO_GUEST;
        }
    }
}

void host_to_guest_handler(struct vcpu* vcpu)
{
    host_to_guest_arch_handler(vcpu);
}

void guest_to_host_handler(struct vcpu* vcpu)
{
    guest_to_host_arch_handler(vcpu);
}

void vcpu_to_vcpu_handler(struct vcpu* from, struct vcpu* to)
{
    vcpu_to_vcpu_arch_handler(from, to);
}

void guest_to_guest_handler(struct vcpu* from, struct vcpu* to)
{
    guest_to_guest_arch_handler(from, to);
}

void switch_hook(rt_thread_t from, rt_thread_t to)
{
    /* 
     * According thread switch type to choose different switch handler.
     */
    rt_uint8_t thread_switch_type = this_switch_type(from, to);
    switch (thread_switch_type)
    {
    case HOST_TO_GUEST:
        host_to_guest_handler(to->vcpu);
        break;
    case GUEST_TO_HOST:
        guest_to_host_handler(from->vcpu);
        break;
    case VCPU_TO_VCPU:
        vcpu_to_vcpu_handler(from->vcpu, to->vcpu);
        break;
    case GUEST_TO_GUEST:
        guest_to_guest_handler(from->vcpu, to->vcpu);
        break;
    case HOST_TO_HOST:
        /* Do nothing. */
        break;
    default:
        break;
    }
}
