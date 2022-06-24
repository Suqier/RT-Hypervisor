/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-01     Suqier       first version
 */

#include <string.h>

#include "vm.h"

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
    char *str = "unknown";

    switch (status)
    {
    case VM_STATUS_OFFLINE:
        str = "offline";
    case VM_STATUS_ONLINE:
        str = "online";
    case VM_STATUS_SUSPEND:
        str = "suspend";
    case VM_STATUS_NEVER_RUN:
        str = "never";
    case VM_STATUS_UNKNOWN:
        str = "unknown";
    }

    return str;
}

char *os_type_str(rt_uint16_t os_type)
{
    char *str = "Other";
    switch (os_type)
    {
    case OS_TYPE_LINUX:
        str = "Linux";
    case OS_TYPE_RT_THREAD:
        str = "RT-Thread";
    case OS_TYPE_RT_ZEPHYR:
        str = "Zephyr";
    case OS_TYPE_OTHER:
        str = "Other";
    }

    return str;
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
    char *lower = strlwr(os_type_str);

    if (strcmp(lower, "linux") == 0)
        os_type_code = OS_TYPE_LINUX;
    else if (strcmp(lower, "rt-thread") == 0)
        os_type_code = OS_TYPE_RT_THREAD;
    else if (strcmp(lower, "zephyr") == 0)
        os_type_code = OS_TYPE_RT_ZEPHYR;
    else
        os_type_code = OS_TYPE_OTHER;
    
    return os_type_code;
}

void vm_config_init(struct vm *vm, rt_uint16_t vm_idx)
{
    vm->vm_idx = vm_idx;
    rt_kprintf("[Info] Allocate VM id is %d.\n", vm_idx);
    vm->status = VM_STATUS_NEVER_RUN;

    strcpy(vm->name, "");
    vm->os_type = OS_TYPE_OTHER;
    vm->mm.mem_size = 0;
    vm->mm.mem_used = 0;
    vm->nr_vcpus = 1;
}

rt_err_t vm_init(struct vm *vm)
{
    /* 
     * It only has memory for struct vm, 
     * It needs more memory for vcpu and device.
     */
    vm->vcpus = (struct vcpu **)rt_malloc(sizeof(struct vcpu) * vm->nr_vcpus);
    if (!vm->vcpus)
    {
        rt_kprintf("[Error] Allocate memory for vcpu failure.\n");
        return -RT_ENOMEM;
    }

    /* mm_struct init. */
    rt_err_t ret = vm_mm_struct_init(&vm->mm);
    if (ret)
        return ret;    

    /* allocate memory for device. TBD */


    /* fdt_parse gets vcpus affinity and more. */
    return RT_EOK;
}

void vm_go(struct vm *vm)
{

}

void vm_pause(struct vm *vm)
{

}
