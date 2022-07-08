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
        .os_addr = 0x208000,
        .dtb_addr = RT_NULL,
        .ramdisk_addr = RT_NULL
    },
    {
        .nr_vcpus = 2,
        .mm_size = 64,
        .os_type = OS_TYPE_RT_ZEPHYR,

        /* ((void (*)(int))addr)(0); */
        .entry_point = 0x208000,
        .os_addr = 0x208000,
        .dtb_addr = RT_NULL,
        .ramdisk_addr = RT_NULL
    }
};

static char *strlwr(const char *s){
    char *str = (char *)&s;
    
    while(*str != '\0'){
        if(*str >= 'A' && *str <= 'Z') {
            *str += 'a'-'A';
        }
        str++;
    }

    return str;
}

char *vm_status_str(rt_uint16_t status)
{
    switch (status)
    {
    case VM_STATUS_OFFLINE:
        return "offline";
    case VM_STATUS_ONLINE:
        return "online";
    case VM_STATUS_SUSPEND:
        return "suspend";
    case VM_STATUS_NEVER_RUN:
        return "never";
    case VM_STATUS_UNKNOWN:
        return "unknown";
    }

    return "unknown";
}

char *os_type_str(rt_uint16_t os_type)
{
    switch (os_type)
    {
    case OS_TYPE_LINUX:
        return "Linux";
    case OS_TYPE_RT_THREAD:
        return "RT-Thread";
    case OS_TYPE_RT_ZEPHYR:
        return "Zephyr";
    case OS_TYPE_OTHER:
    default:
        return "Other";
    }

    return "Other";
}

rt_uint16_t vm_status(const char *vm_status_str)
{
    rt_uint16_t vm_status_code = VM_STATUS_UNKNOWN;
    char *lower = strlwr(vm_status_str);

    if (strcmp(lower, "offline") == 0)
        vm_status_code = VM_STATUS_OFFLINE;
    else if (strcmp(lower, "online") == 0)
        vm_status_code = VM_STATUS_ONLINE;
    else if (strcmp(lower, "suspend") == 0)
        vm_status_code = VM_STATUS_SUSPEND;
    else if (strcmp(lower, "never") == 0)
        vm_status_code = VM_STATUS_NEVER_RUN;
    else
        vm_status_code = VM_STATUS_UNKNOWN;
    
    return vm_status_code;
}

rt_uint16_t vm_os_type(const char *os_type_str)
{
    rt_uint16_t os_type_code = OS_TYPE_OTHER;

    if (strcmp(os_type_str, "Linux") == 0)
        os_type_code = OS_TYPE_LINUX;
    else if (strcmp(os_type_str, "RT-Thread") == 0)
        os_type_code = OS_TYPE_RT_THREAD;
    else if (strcmp(os_type_str, "Zephyr") == 0)
        os_type_code = OS_TYPE_RT_ZEPHYR;
    else
        os_type_code = OS_TYPE_OTHER;
    
    return os_type_code;
}

static struct vcpu *__create_vcpu(struct vm *vm, rt_uint32_t vcpu_id)
{
    char name[VM_NAME_SIZE];
    struct vcpu *vcpu;
    struct rt_thread *thread;

    vcpu = (struct vcpu *)rt_malloc(sizeof(struct vcpu));
    if (vcpu == RT_NULL)
    {
        rt_kprintf("[Error] create %dth vcpu failure.\n", vcpu_id);
        return RT_NULL;
    }
    
	rt_memset(name, 0, VM_NAME_SIZE);
	sprintf(name, "m%d_c%d", vm->vm_idx, vcpu_id);
	thread = rt_thread_create(name, (void *)vm->os->entry_point, RT_NULL, 
                        2048, FINSH_THREAD_PRIORITY, THREAD_TIMESLICE);
	if (thread == RT_NULL)
    {
        rt_free(vcpu);
		return RT_NULL;
    }

    thread->vcpu = vcpu;
    vcpu->thread = thread;
    vcpu->vcpu_id = vcpu_id;
    vcpu->vm = vm;
    vm->vcpus[vcpu_id] = vcpu;

    if (vcpu_id)
        vm->vcpus[vcpu_id - 1]->next = vcpu;
    if (vcpu_id == MAX_VCPU_NUM)
        vcpu->next = vm->vcpus[0];
    else
        vcpu->next = RT_NULL;

    return vcpu;
}

static void __free_vcpu(struct vcpu *vcpu)
{
    if (vcpu)
    {
        if (vcpu->thread)
            rt_thread_delete(vcpu->thread);
        rt_free(vcpu);
    }
}

static void __go_vcpu(struct vcpu *vcpu)
{
    if (vcpu->thread)
        rt_thread_startup(vcpu->thread);
    else
        rt_kprintf("[Error] Run %dth vcpu failure.\n", vcpu->vcpu_id);
}

static rt_err_t create_vcpus(struct vm *vm)
{
    for (rt_size_t i = 0; i < vm->nr_vcpus; i++)
    {
        struct vcpu *vcpu = __create_vcpu(vm, i);
        if (vcpu == RT_NULL)
        {
            for (rt_size_t j = 0; j < vm->nr_vcpus; j++)
            {
                vcpu = vm->vcpus[j];
                if (vcpu)
                    __free_vcpu(vcpu);
                else
                    continue;
            }

            rt_kprintf("[Error] Create %dth vcpu failure for %dth vm.\n", 
                    i, vm->vm_idx);
            return -RT_ENOMEM;
        }
    }

    rt_kprintf("[Info] Create %d vcpus success for %dth VM\n", 
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

rt_err_t vm_init(struct vm *vm)
{
    rt_err_t ret = RT_EOK;
    /* 
     * It only has memory for struct vm, 
     * It needs more memory for vcpu and device.
     */
    vm->vcpus = (struct vcpu **)rt_malloc(sizeof(struct vcpu *) * vm->nr_vcpus);
    if (vm->vcpus == RT_NULL)
    {
        rt_kprintf("[Error] Allocate memory for vcpus' pointer failure.\n");
        return -RT_ENOMEM;
    }

    ret = create_vcpus(vm);
    if (ret)
        return ret;

    ret = vm_mm_struct_init(vm->mm);
    if (ret)
        return ret;

    /* memory map when vm init */
    ret = vm_memory_init(vm->mm);
    if (ret)
        return ret;

    /* allocate memory for device. TBD */
    // ret = load_os_img();
    // if (ret)
    //     return ret;

    /* fdt_parse gets vcpus affinity and more. */
    return RT_EOK;
}

void go_vm(struct vm *vm)
{
    /* consider VM status? TBD */
    struct vcpu *vcpu = vm->vcpus[0];   /* main core */
    if (vcpu)
        __go_vcpu(vcpu);
    else
        rt_kprintf("[Error] Start %dth vm failure.\n", vm->vm_idx);
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
            rt_kprintf("[Error] pause VM failure at %dth cpu.\n", i);
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
        __free_vcpu(vm->vcpus[i]);

    /* free other resource, like memory resouece & TBD */
}
