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
#include "vconsole.h"

#define THREAD_TIMESLICE    5

#define MAX_VCPU_NUM    4      /* per vm */
#define VM_NAME_SIZE    16
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

enum 
{
    VCPU_STATUS_OFFLINE = 0,
    VCPU_STATUS_ONLINE,
    VCPU_STATUS_SUSPEND,
    VCPU_STATUS_NEVER_RUN,
    VCPU_STATUS_UNKNOWN,
};

struct vcpu
{
    rt_uint32_t id;
    rt_uint32_t affinity;
    rt_uint16_t status;

    struct vm *vm;
    struct vcpu *next;
    rt_thread_t tid;
    struct vtimer_context *vtc;

    struct vcpu_arch *arch;  /* vcpu arch related content. */
}__attribute__((aligned(L1_CACHE_BYTES)));
typedef struct vcpu *vcpu_t;

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
    rt_uint8_t id;     /* index in hypervisor vms array */ 
    rt_uint16_t status;
    
    char name[VM_NAME_SIZE];
    struct mm_struct *mm;    /* userspace tied to this vm */
    const struct os_desc *os;
    rt_uint8_t os_idx;

#ifdef RT_USING_SMP
    rt_hw_spinlock_t vm_lock;
#endif

    struct vm_arch *arch;

    /* A array for collect vcpu. */
    rt_uint8_t nr_vcpus;
    rt_uint32_t vcpu_affinity[MAX_VCPU_NUM];
    vcpu_t *vcpus;

    /* vGIC */
    struct vgic *vgic;

    /* vTimer | TBD */
    
    /* For TTY and so on */
    rt_list_t dev_list;
}__attribute__((aligned(L1_CACHE_BYTES)));
typedef struct vm *vm_t;

rt_inline vcpu_t get_vcpu_by_thread(rt_thread_t tid){ return (vcpu_t)tid->vcpu; }
rt_inline vcpu_t get_curr_vcpu(void) { return get_vcpu_by_thread(rt_thread_self()); }
rt_inline vm_t get_vm_by_thread(rt_thread_t tid) { return get_vcpu_by_thread(tid)->vm; }
rt_inline vm_t get_curr_vm(void) { return get_vm_by_thread(rt_thread_self()); }

/*
 * For vCPU
 */
vcpu_t vcpu_create(vm_t vm, rt_uint32_t vcpu_id);
rt_err_t vcpus_create(vm_t vm);
void vcpu_init(void);
void vcpu_go(vcpu_t vcpu);
void vcpu_suspend(vcpu_t vcpu);
void vcpu_shutdown(vcpu_t vcpu);
void vcpu_free(vcpu_t vcpu);
void vcpu_fault(vcpu_t vcpu);

/*
 * For VM
 */
rt_err_t os_img_load(vm_t vm);
void vm_config_init(vm_t vm, rt_uint8_t vm_idx);
rt_err_t vm_init(vm_t vm);
void vm_go(vm_t vm);
void vm_suspend(vm_t vm);
void vm_shutdown(vm_t vm);
void vm_free(vm_t vm);

#endif  /* __VM_H__ */ 