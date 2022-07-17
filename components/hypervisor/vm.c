/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-01     Suqier       first version
 */

#include <stdio.h>
#include <string.h>

#include "vm.h"

const struct os_desc os_support[MAX_OS_TYPE] =
{
    {
        .nr_vcpus = 1,
        .mm_size = 64,  /* default MB */
        .os_type = OS_TYPE_RT_THREAD,

        /* ((void (*)(int))addr)(0); */
        .entry_point = 0x208000,
        .os_addr = 0x210000,
        .os_img_size = 0x49560,
        .dtb_addr = RT_NULL,
        .ramdisk_addr = RT_NULL
    },
    {
        .nr_vcpus = 2,
        .mm_size = 64,
        .os_type = OS_TYPE_RT_ZEPHYR,

        /* ((void (*)(int))addr)(0); */
        .entry_point = 0x208000,
        .os_addr = 0x210000,
        .os_img_size = 0x49560,
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

static struct vcpu *create_vcpu(struct vm *vm, rt_uint32_t vcpu_id)
{
    char name[VM_NAME_SIZE];
    struct vcpu *vcpu;
    struct vcpu_arch * arch;
    struct rt_thread *thread;

    vcpu = (struct vcpu *)rt_malloc(sizeof(struct vcpu));
    arch = (struct vcpu_arch *)rt_malloc(sizeof(struct vcpu_arch));
    if (vcpu == RT_NULL || arch == RT_NULL)
    {
        rt_kprintf("[Error] create %dth vCPU failure.\n", vcpu_id);
        return RT_NULL;
    }

	rt_memset(name, 0, VM_NAME_SIZE);
	sprintf(name, "m%d_c%d", vm->vm_idx, vcpu_id);
	thread = rt_thread_create(name, (void *)vm_entry, (void *)vcpu, 
                        4096, FINSH_THREAD_PRIORITY + 1, THREAD_TIMESLICE);
	if (thread == RT_NULL)
    {
        rt_free(vcpu);
		return RT_NULL;
    }

    vcpu->arch = arch;
    vcpu->flag = VCPU_JUST_CREATE;
    thread->vcpu = vcpu;
    vcpu->thread = thread;
    vcpu->vcpu_id = vcpu_id;
    vcpu->vm = vm;
    vm->vcpus[vcpu_id] = vcpu;
    rt_memset(arch, 0, sizeof(struct vcpu_arch));
    hook_vcpu_state_init(vcpu);

    if (vcpu_id)
        vm->vcpus[vcpu_id - 1]->next = vcpu;
    if (vcpu_id == MAX_VCPU_NUM)
        vcpu->next = vm->vcpus[0];
    else
        vcpu->next = RT_NULL;

    return vcpu;
}

static void free_vcpu(struct vcpu *vcpu)
{
    if (vcpu)
    {
        if (vcpu->thread)
            rt_thread_delete(vcpu->thread);
        rt_free(vcpu);
    }
}

static void go_vcpu(struct vcpu *vcpu)
{
    if (vcpu->thread)
    {
        /* prepare EL1 sys regs, then run vcpu */
        rt_thread_startup(vcpu->thread);
    }
    else
        rt_kprintf("[Error] Run %dth vCPU failure.\n", vcpu->vcpu_id);
}

static rt_err_t create_vcpus(struct vm *vm)
{
    for (rt_size_t i = 0; i < vm->nr_vcpus; i++)
    {
        struct vcpu *vcpu = create_vcpu(vm, i);
        if (vcpu == RT_NULL)
        {
            for (rt_size_t j = 0; j < vm->nr_vcpus; j++)
            {
                vcpu = vm->vcpus[j];
                if (vcpu)
                    free_vcpu(vcpu);
                else
                    continue;
            }

            rt_kprintf("[Error] Create %dth vCPU failure for %dth VM.\n", 
                    i, vm->vm_idx);
            return -RT_ENOMEM;
        }
    }

    rt_kprintf("[Info] Create %d vCPUs success for %dth VM\n", 
                                            vm->nr_vcpus, vm->vm_idx);
    return RT_EOK;
}

void vm_config_init(struct vm *vm, rt_uint16_t vm_idx)
{
    vm->vm_idx = vm_idx;
    rt_kprintf("[Info] Allocate VM id is %03d\n", vm_idx);
    vm->status = VM_STATUS_NEVER_RUN;
    rt_hw_spin_lock_init(&vm->vm_lock);
    vm->mm->mem_size = vm->os->mm_size;
    vm->mm->mem_used = 0;
    vm->nr_vcpus = vm->os->nr_vcpus;
}

static rt_err_t load_os_img(struct vm *vm)		
{
    void *src = (void *)vm->os->os_addr;
    rt_uint64_t dst_va = vm->os->entry_point;
    rt_ubase_t dst_pa = 0x0UL;
    rt_ubase_t count = vm->os->os_img_size, copy_size;
    rt_err_t ret;

    do
    {
        ret = stage2_translate(vm->mm, dst_va, &dst_pa);
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
        count -= copy_size;
        dst_va += copy_size;
    } while (count > 0);

    rt_kprintf("[Info] Load OS img OK\n");
    return RT_EOK;
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

    ret = create_vcpus(vm);
    if (ret)
        return ret;
    
    /* allocate memory for device. TBD */
    ret = load_os_img(vm);
    if (ret)
        return ret;

    /* fdt_parse gets vcpus affinity and more. */
    return RT_EOK;
}

void go_vm(struct vm *vm)
{
    /* consider VM status? TBD */
    struct vcpu *vcpu = vm->vcpus[0];   /* main core */
    if (vcpu)
        go_vcpu(vcpu);
    else
        rt_kprintf("[Error] Start %dth VM failure.\n", vm->vm_idx);
}

void suspend_vm(struct vm *vm)
{
    rt_err_t ret;
    struct vcpu *vcpu;

    rt_hw_spin_lock(&vm->vm_lock);
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
    rt_hw_spin_unlock(&vm->vm_lock);
}

void shutdown_vm(struct vm *vm)
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

void free_vm(struct vm *vm)
{
    /* free vcpus resource */
    for (rt_size_t i = 0; i < vm->nr_vcpus; i++)
        free_vcpu(vm->vcpus[i]);

    /* free other resource, like memory resouece & TBD */
}
