/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-09-02     Suqier       first version
 */

#ifndef __SWITCH_H__
#define __SWITCH_H__

#include <rtdef.h>

/* Define switch processes. */
#define HOST_TO_GUEST    1  /* From Host switch into Guest vCPU */
#define GUEST_TO_HOST    2  /* From Guest vCPU switch out Host */
#define VCPU_TO_VCPU     3  /* From vCPU_1 to vCPU_2 in the same Guest */
#define GUEST_TO_GUEST   4  /* From Guest_1 to Guest_2 */
#define HOST_TO_HOST     5  /* Thread switch in Host */

rt_bool_t is_vcpu_thread(rt_thread_t thread);
rt_uint8_t vcpu_thread_vm_idx(rt_thread_t thread);
rt_bool_t is_vcpu_threads_same_vm(rt_thread_t from, rt_thread_t to);
rt_uint8_t this_switch_type(rt_thread_t from, rt_thread_t to);

/* Different type of switch handler. */
void host_to_guest_handler(struct vcpu *vcpu);
void guest_to_host_handler(struct vcpu *vcpu);
void vcpu_to_vcpu_handler(struct vcpu *from, struct vcpu *to);
void guest_to_guest_handler(struct vcpu *from, struct vcpu *to);

void switch_hook(rt_thread_t from, rt_thread_t to);

#endif  /* __SWITCH_H__ */