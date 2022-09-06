/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-01     Suqier       first version
 */

#include "rtconfig.h"

#include <stdio.h>
#include <string.h>

#include "vm.h"
#include "virt_arch.h"

const struct os_desc os_support[MAX_OS_TYPE] =
{
    {
        .nr_vcpus = 1,
        .mm_size = 8,  /* default MB */
        .os_type = OS_TYPE_RT_THREAD,
        /* for qemu */
        /* ((void (*)(int))addr)(0); */
        .entry_point = 0x40008000,
        .os_addr = 0x45000000,
        .os_img_size = 0x6A180,
        .dtb_addr = RT_NULL,
        .ramdisk_addr = RT_NULL
    },
    {
        .nr_vcpus = 1,
        .mm_size = 8,
        .os_type = OS_TYPE_RT_THREAD,
        /* for a55 */
        .entry_point = 0x208000,
        .os_addr = 0x208000,
        .os_img_size = 0x1DC80,
        .dtb_addr = RT_NULL,
        .ramdisk_addr = RT_NULL
    },
    {
        .nr_vcpus = 1,
        .mm_size = 8,
        .os_type = OS_TYPE_RT_ZEPHYR,
        /* for qemu but zephyr */
        .entry_point = 0x40000ffc,
        .os_addr = 0x45000000,
        .os_img_size = 0x5B844,
        .dtb_addr = RT_NULL,
        .ramdisk_addr = RT_NULL
    }
};

const char* vm_status_str[VM_STATUS_UNKNOWN + 1] =
{
    "offline", "online", "suspend", "never", "unknown"
};

const char* os_type_str[OS_TYPE_OTHER + 1] =
{
    "Linux", "RT-Thread", "Zephyr", "Other"
};

/*
 * For vCPU
 */
static void vcpu_init_spsr_modify(rt_ubase_t from)
{
    /* 
     * When creating vCPU thread, we'll using function rt_hw_stack_init() in 
     * stack.c, which set the SPSR of thread is current exception level. 
     * But vCPU thread need a lower exception level when creating. 
     */
    rt_ubase_t _spsr = SPSR_EL1H | DAIF_FIQ | DAIF_IRQ | DAIF_ABT | DAIF_DBG;

    __asm__ volatile ("mov x2, %0"::"r"(from));
    __asm__ volatile ("mov x3, %0"::"r"(_spsr));
    __asm__ volatile ("add x2, x2, #0x08");
    __asm__ volatile ("str x3, [x2]":::"memory");
}

struct vcpu *vcpu_create(struct vm *vm, rt_uint32_t vcpu_id)
{
    char name[VM_NAME_SIZE];
    struct vcpu *vcpu;
    struct vcpu_arch * arch;
    struct rt_thread *thread;

    vcpu = (struct vcpu*)rt_malloc(sizeof(struct vcpu));
    arch = (struct vcpu_arch*)rt_malloc(sizeof(struct vcpu_arch));
    if (vcpu == RT_NULL || arch == RT_NULL)
    {
        rt_kprintf("[Error] create %dth vCPU failure.\n", vcpu_id);
        return RT_NULL;
    }

	rt_memset(name, 0, VM_NAME_SIZE);
	sprintf(name, "m%d_c%d", vm->vm_idx, vcpu_id);
	thread = rt_thread_create(name, (void *)(vm->os->entry_point), RT_NULL, 
                        4096, FINSH_THREAD_PRIORITY + 1, THREAD_TIMESLICE);
	if (thread == RT_NULL)
    {
        rt_free(vcpu);
		return RT_NULL;
    }

    vcpu->arch = arch;
    vcpu->status = VCPU_STATUS_NEVER_RUN;
    thread->vcpu = vcpu;
    vcpu->thread = thread;
    vcpu->vcpu_id = vcpu_id;
    vcpu->vm = vm;
    vm->vcpus[vcpu_id] = vcpu;
    rt_memset(arch, 0, sizeof(struct vcpu_arch));
    vcpu_state_init(vcpu);
    vcpu_init_spsr_modify((rt_ubase_t)vcpu->thread->sp);

    if (vcpu_id)
        vm->vcpus[vcpu_id - 1]->next = vcpu;
    if (vcpu_id == MAX_VCPU_NUM)
        vcpu->next = vm->vcpus[0];
    else
        vcpu->next = RT_NULL;

    return vcpu;
}

void vcpu_free(struct vcpu *vcpu)
{
    if (vcpu)
    {
        if (vcpu->thread)
            rt_thread_delete(vcpu->thread);
        rt_free(vcpu);
    }
}

rt_err_t vcpus_create(struct vm *vm)
{
    for (rt_size_t i = 0; i < vm->nr_vcpus; i++)
    {
        struct vcpu *vcpu = vcpu_create(vm, i);
        if (vcpu == RT_NULL)
        {
            for (rt_size_t j = 0; j < vm->nr_vcpus; j++)
            {
                vcpu = vm->vcpus[j];
                if (vcpu)
                    vcpu_free(vcpu);
                else
                    continue;
            }

            rt_kprintf("[Error] Create %dth vCPU failure for %dth VM.\n", 
                    i, vm->vm_idx);
            return -RT_ENOMEM;
        }
    }

    rt_kprintf("[Info] Create %d vCPUs success for %dth VM.\n", 
                        vm->nr_vcpus, vm->vm_idx);
    return RT_EOK;
}

void vcpu_go(struct vcpu *vcpu)
{
    if (vcpu->thread && vcpu->status != VCPU_STATUS_ONLINE)
    {
        vcpu->status = VCPU_STATUS_ONLINE;
        rt_thread_startup(vcpu->thread);
    }
    else
        rt_kprintf("[Error] Run %dth vCPU failure for %dth VM.\n", 
                vcpu->vcpu_id, vcpu->vm->vm_idx);
}

void vcpu_suspend(struct vcpu *vcpu)
{
    /* Turn vcpu->thread into RT_THREAD_SUSPEND status. */
    rt_kprintf("[Info] Suspend %dth vCPU now for %dth VM.\n", 
            vcpu->vcpu_id, vcpu->vm->vm_idx);
    if (vcpu->status == VCPU_STATUS_ONLINE)
    {
        vcpu->status = VCPU_STATUS_SUSPEND;
        rt_thread_suspend(vcpu->thread);
    }
    else
        rt_kprintf("[Error] Suspend %dth vCPU failure for %dth VM.\n", 
                vcpu->vcpu_id, vcpu->vm->vm_idx);
}

void vcpu_shutdown(struct vcpu *vcpu)
{
    /* Turn vcpu->thread into RT_THREAD_CLOSE status and free vCPU resource. */
    rt_kprintf("[Info] Shutdown %dth vCPU now for %dth VM.\n", 
            vcpu->vcpu_id, vcpu->vm->vm_idx);
    if (vcpu->status == VCPU_STATUS_ONLINE)
    {
        vcpu->status = VCPU_STATUS_OFFLINE;
        rt_thread_suspend(vcpu->thread);
        rt_thread_delete(vcpu->thread);
    }
    else if (vcpu->status == VCPU_STATUS_SUSPEND)
    {
        vcpu->status = VCPU_STATUS_OFFLINE;
        rt_thread_delete(vcpu->thread);
    }
    else
        rt_kprintf("[Error] Shutdown %dth vCPU failure for %dth VM.\n", 
            vcpu->vcpu_id, vcpu->vm->vm_idx);
}

void vcpu_fault(struct vcpu *vcpu)
{
    /* Report Error, dump vCPU register info and shutdown vCPU. */
    vcpu->status = VCPU_STATUS_UNKNOWN; /* vCPU fault. */
    rt_kprintf("[Fault] %dth vCPU Fault for %dth VM.\n", 
            vcpu->vcpu_id, vcpu->vm->vm_idx);
    vcpu_regs_dump(vcpu);
    vcpu_shutdown(vcpu);
}

/*
 * For VM
 */
rt_err_t os_img_load(struct vm *vm)		
{
    void *src = (void *)vm->os->os_addr;
    rt_uint64_t dst_va = vm->os->entry_point;
    rt_ubase_t dst_pa = 0x0UL;
    rt_ubase_t count = vm->os->os_img_size, copy_size;
    rt_err_t ret;

    do
    {
        ret = s2_translate(vm->mm, dst_va, &dst_pa);
        if (ret)
        {
            rt_kprintf("[Error] Load OS img failure\n");
            return ret;
        }

        if (count >= MEM_BLOCK_SIZE)
            copy_size = MEM_BLOCK_SIZE;
        else
            copy_size = count;

        rt_memcpy((void *)dst_pa, (const void *)src, copy_size);
        rt_kprintf("[Debug] memcpy src: 0x%08x to dst: 0x%08x\n", src, dst_pa);
        count -= copy_size;
        dst_va += copy_size;
    } while (count > 0);

    rt_kprintf("[Info] Load OS img OK\n");
    return RT_EOK;
}

void vm_config_init(struct vm *vm, rt_uint8_t vm_idx)
{
    vm->vm_idx = vm_idx;
    rt_kprintf("[Info] Allocate VM id is %03d\n", vm_idx);
    vm->status = VM_STATUS_NEVER_RUN;

#ifdef RT_USING_SMP
    rt_hw_spin_lock_init(&vm->vm_lock);
#endif
    
    vm->mm->mem_size = vm->os->mm_size;
    vm->mm->mem_used = 0;
    vm->nr_vcpus = vm->os->nr_vcpus;
}

rt_err_t vm_init(struct vm *vm)
{
    rt_err_t ret = RT_EOK;
    /* 
     * It only has memory for struct vm, 
     * It needs more memory for vcpu and device.
     */
    vm->vcpus = (struct vcpu **)rt_malloc(sizeof(struct vcpu *) * vm->nr_vcpus);
    vm->arch =  (struct vm_arch *)rt_malloc(sizeof(struct vm_arch));
    if (vm->vcpus == RT_NULL || vm->arch == RT_NULL)
    {
        rt_kprintf("[Error] Allocate memory for VM's pointers[vcpus&arch] failure\n");
        return -RT_ENOMEM;
    }

    ret = vm_mm_struct_init(vm->mm);
    if (ret)
        return ret;

    /* memory map when vm init */
    ret = vm_memory_init(vm->mm);
    if (ret)
        return ret;

    ret = vcpus_create(vm);
    if (ret)
        return ret;
    
    /* allocate memory for device. TBD */
    ret = os_img_load(vm);
    if (ret)
        return ret;

    /* fdt_parse gets vcpus affinity and more. */
    return RT_EOK;
}

void vm_go(struct vm *vm)
{
    /* consider VM status? TBD */
    struct vcpu *vcpu = vm->vcpus[0];   /* main core */
    if (vcpu)
        vcpu_go(vcpu);
    else
        rt_kprintf("[Error] Start %dth VM failure.\n", vm->vm_idx);
}

void vm_suspend(struct vm * vm)
{
    rt_err_t ret;
    struct vcpu * vcpu;

#ifdef RT_USING_SMP
    rt_hw_spin_lock(&vm->vm_lock);
#endif

    for (rt_size_t i = 0; i < vm->nr_vcpus; i++)
    {
        vcpu = vm->vcpus[i];
        ret = rt_thread_suspend(vcpu->thread);
        if (ret)
        {
            rt_kprintf("[Error] Pause VM failure at %dth vCPU.\n", i);
            vm->status = VM_STATUS_UNKNOWN;
            return;
        }
    }
    vm->status = VM_STATUS_SUSPEND;

#ifdef RT_USING_SMP
    rt_hw_spin_unlock(&vm->vm_lock);
#endif
}

void vm_shutdown(struct vm *vm)
{
    if (vm)
    {
        for (rt_size_t i = 0; i < vm->nr_vcpus; i++)
        {
            struct vcpu *vcpu = vm->vcpus[i];
            rt_thread_delete(vcpu->thread);
        }

        /* 
         * and release VM memory resource. TBD 
         */

        vm->status = VM_STATUS_OFFLINE;
    }
}

void vm_free(struct vm *vm)
{
    /* free vCPUs resource */
    for (rt_size_t i = 0; i < vm->nr_vcpus; i++)
        vcpu_free(vm->vcpus[i]);

    /* free other resource, like memory resouece & TBD */
}
