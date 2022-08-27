/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-05-30     Suqier       first version
 */

#ifndef __VM_H__
#define __VM_H__

#include "mm.h"
#include "os.h"

#define THREAD_TIMESLICE    5

#define MAX_VCPU_NUM    4      /* per vm */
#define MAX_OS_TYPE     3
#define VM_NAME_SIZE    16
#define MAX_VM_NUM      64
#define L1_CACHE_BYTES  64

#define VCPU_JUST_CREATE        (0UL)
#define VCPU_NEED_LOAD_REGS     (1 << 0)
#define VCPU_NEED_SAVE_REGS     (1 << 1)

#define VCPU_PENDING_EXCEPT	    (1 << 8) /* Exception pending */
#define VCPU_INCREMENT_PC		(1 << 9) /* Increment PC */
#define VCPU_EXCEPT_MASK		(7 << 9) /* Target EL/MODE */
#define VCPU_EXCEPT_ELx_SYNC	(0 << 9)
#define VCPU_EXCEPT_ELx_IRQ	    (1 << 9)
#define VCPU_EXCEPT_ELx_FIQ	    (2 << 9)
#define VCPU_EXCEPT_ELx_SERR	(3 << 9)
#define VCPU_EXCEPT_EL1	        (0 << 11)
#define VCPU_EXCEPT_EL2	        (1 << 11)

struct vcpu
{
    rt_uint32_t vcpu_id;
    
    struct vm *vm;
    struct vcpu *next;
    struct rt_thread *thread;

    rt_uint64_t flag;
    struct vcpu_arch *arch;  /* vcpu arch related content. */
}__attribute__((aligned(L1_CACHE_BYTES)));

enum 
{
    VM_STATUS_OFFLINE = 0,
    VM_STATUS_ONLINE,
    VM_STATUS_SUSPEND,
    VM_STATUS_NEVER_RUN,
    VM_STATUS_UNKNOWN,
};

struct vm
{
    int vmid;
    rt_uint16_t vm_idx;     /* index in hypervisor vms array */ 
    rt_uint16_t status;
    
    char name[VM_NAME_SIZE];
    struct mm_struct *mm;    /* userspace tied to this vm */
    const struct os_desc *os;

#ifdef RT_USING_SMP
    rt_hw_spinlock_t vm_lock;
#endif

    struct vm_arch *arch;

    /* A array for collect vcpu. */
    rt_uint8_t nr_vcpus;
    rt_uint32_t vcpu_affinity[MAX_VCPU_NUM];
    struct vcpu **vcpus;
}__attribute__((aligned(L1_CACHE_BYTES)));

rt_inline rt_uint32_t get_vcpu_id(struct vcpu *vcpu)
{
    return vcpu->vcpu_id;
}

rt_inline int get_vm_id(struct vcpu *vcpu)
{
    return vcpu->vm->vmid;
}

rt_inline struct vm *get_vm_by_vcpu(struct vcpu *vcpu)
{
	return vcpu->vm;
}

rt_inline struct vcpu *get_vcpu_by_thread(struct rt_thread *thread)
{
	return (struct vcpu *)thread->user_data;
}

rt_inline struct vm *get_vm_by_thread(struct rt_thread *thread)
{
	struct vcpu *vcpu = get_vcpu_by_thread(thread);
    return vcpu->vm;
}

rt_inline struct vcpu *get_curr_vcpu(void)
{
    return get_vcpu_by_thread(rt_thread_self());
}

rt_inline struct vm *get_curr_vm(void)
{
    return get_vm_by_thread(rt_thread_self());
}

void vm_config_init(struct vm *vm, rt_uint16_t vm_idx);
rt_err_t vm_init(struct vm *vm);
void go_vm(struct vm *vm);
void suspend_vm(struct vm *vm);
void shutdown_vm(struct vm *vm);
void free_vm(struct vm *vm);

#endif  /* __VM_H__ */ 