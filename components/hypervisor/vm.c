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
#include "os.h"
#include "virt_arch.h"
#include "vtimer.h"

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
static void vcpu_spsr_init(rt_ubase_t from)
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

vcpu_t vcpu_create(vm_t vm, rt_uint32_t vcpu_id)
{
    char name[VM_NAME_SIZE];
    vcpu_t vcpu;
    struct vcpu_arch *arch;
    rt_thread_t tid;

    vcpu = (struct vcpu*)rt_malloc(sizeof(struct vcpu));
    arch = (struct vcpu_arch*)rt_malloc(sizeof(struct vcpu_arch));
    if (vcpu == RT_NULL || arch == RT_NULL)
    {
        rt_kprintf("[Error] create %dth vCPU failure.\n", vcpu_id);
        return RT_NULL;
    }

	rt_memset(name, 0, VM_NAME_SIZE);
	sprintf(name, "m%d_c%d", vm->id, vcpu_id);
	tid = rt_thread_create(name, (void *)(vm->os->img.ep), RT_NULL, 
                        4096, FINSH_THREAD_PRIORITY + 1, THREAD_TIMESLICE);
	if (tid == RT_NULL)
    {
        rt_free(vcpu);
		return RT_NULL;
    }

    vcpu->affinity = vm->os[vm->os_idx].cpu.affinity[vcpu_id];    /* affinity */
    vcpu->arch = arch;
    vcpu->status = VCPU_STATUS_NEVER_RUN;
    tid->vcpu = vcpu;
    vcpu->tid = tid;
    vcpu->id = vcpu_id;
    vcpu->vm = vm;
    vm->vcpus[vcpu_id] = vcpu;
    
    vtimer_ctxt_create(vcpu);
    rt_memset(arch, 0, sizeof(struct vcpu_arch));
    vcpu_state_init(vcpu);
    vcpu_spsr_init((rt_ubase_t)vcpu->tid->sp);

    return vcpu;
}

void vcpu_free(vcpu_t vcpu)
{
    if (vcpu)
    {
        if (vcpu->tid)
            rt_thread_delete(vcpu->tid);
        rt_free(vcpu);
    }
}

rt_err_t vcpus_create(vm_t vm)
{
    for (rt_size_t i = 0; i < vm->nr_vcpus; i++)
    {
        vcpu_t vcpu = vcpu_create(vm, i);
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

            rt_kprintf("[Error] %dth VM: Create %dth vCPU failure\n", vm->id, i);
            return -RT_ENOMEM;
        }
    }

    rt_kprintf("[Info] %dth VM: Create %d vCPUs success\n", vm->id, vm->nr_vcpus);
    return RT_EOK;
}

void vcpu_go(vcpu_t vcpu)
{
    RT_ASSERT(vcpu->tid);
    switch (vcpu->status)
    {
    case VCPU_STATUS_ONLINE:
        break;
    case VCPU_STATUS_OFFLINE:
    case VCPU_STATUS_NEVER_RUN:
        vcpu->status = VCPU_STATUS_ONLINE;
        rt_thread_startup(vcpu->tid);
        break;
    case VCPU_STATUS_SUSPEND:
        vcpu->status = VCPU_STATUS_ONLINE;
        rt_thread_resume(vcpu->tid);
        break;

    case VCPU_STATUS_UNKNOWN:
    default:
        rt_kprintf("[Error] %dth VM: Run %dth vCPU failure\n", vcpu->vm->id, vcpu->id); 
        break;
    }
}

void vcpu_suspend(vcpu_t vcpu)
{
    RT_ASSERT(vcpu->tid);
    switch (vcpu->status)
    {
    case VCPU_STATUS_ONLINE:
        vcpu->status = VCPU_STATUS_SUSPEND;
        rt_thread_suspend(vcpu->tid);
        rt_schedule();
        break;

    case VCPU_STATUS_NEVER_RUN:
    case VCPU_STATUS_OFFLINE:
    case VCPU_STATUS_SUSPEND:
        break;

    case VCPU_STATUS_UNKNOWN:
    default:
        rt_kprintf("[Error] %dth VM: Suspend %dth vCPU failure\n", vcpu->vm->id, vcpu->id);
        break;
    }
}

void vcpu_shutdown(vcpu_t vcpu)
{
    /* Turn vcpu->thread into RT_THREAD_CLOSE status and free vCPU resource. */
    rt_kprintf("[Info] %dth VM: Shutdown %dth vCPU\n", vcpu->vm->id, vcpu->id);
    if (vcpu->status == VCPU_STATUS_ONLINE)
    {
        vcpu->status = VCPU_STATUS_OFFLINE;
        rt_thread_suspend(vcpu->tid);
        rt_thread_delete(vcpu->tid);
    }
    else if (vcpu->status == VCPU_STATUS_SUSPEND)
    {
        vcpu->status = VCPU_STATUS_OFFLINE;
        rt_thread_delete(vcpu->tid);
    }
    else
    {
        rt_kprintf("[Error] Shutdown %dth vCPU failure for %dth VM\n", 
            vcpu->id, vcpu->vm->id);
    }
    
    vcpu->status = VCPU_STATUS_UNKNOWN; /* vCPU fault. */
}

void vcpu_fault(vcpu_t vcpu)
{
    /* Report Error, dump vCPU register info and shutdown vCPU. */
    rt_kprintf("[Fault] %dth VM: %dth vCPU Fault\n", vcpu->vm->id, vcpu->id);
    vcpu_regs_dump(vcpu);
    vcpu_shutdown(vcpu);
}

/*
 * For VM
 */
rt_err_t os_img_load(vm_t vm)		
{
    void *src = (void *)vm->os->img.addr;
    rt_uint64_t dst_va = vm->os->img.ep;
    rt_ubase_t dst_pa = 0x0UL;
    rt_ubase_t count = vm->os->img.size, copy_size;
    rt_err_t ret;

    do
    {
        ret = s2_translate(vm->mm, dst_va, &dst_pa);
        if (ret)
        {
            rt_kputs("[Error] Load OS img failure\n");
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

    rt_kputs("[Info] Load OS img OK\n");
    return RT_EOK;
}

void vm_config_init(vm_t vm, rt_uint8_t vm_idx)
{
    vm->id = vm_idx;
    rt_kprintf("[Info] Allocate VM id is %03d\n", vm_idx);
    vm->status = VM_STATUS_NEVER_RUN;

#ifdef RT_USING_SMP
    rt_hw_spin_lock_init(&vm->vm_lock);
#endif
    
    vm->mm->mem_size = vm->os->mem.size;
    vm->mm->mem_used = 0;
    vm->nr_vcpus = vm->os->cpu.num;
    rt_list_init(&vm->dev_list);
}

rt_err_t vm_init(vm_t vm)
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
        rt_kputs("[Error] Allocate memory for VM's pointers failure\n");
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
    
    vgic_init(vm);
    
    vc_create(vm);
    
    /* allocate memory for device. TBD */
    ret = os_img_load(vm);
    if (ret)
        return ret;

    /* fdt_parse gets vcpus affinity and more. */
    return RT_EOK;
}

void vm_go(vm_t vm)
{
    /* consider VM status? TBD */
    vcpu_t vcpu = vm->vcpus[0];   /* main core */
    if (vcpu)
        vcpu_go(vcpu);
    else
        rt_kprintf("[Error] %dth VM: Start failure.\n", vm->id);
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
        ret = rt_thread_suspend(vcpu->tid);
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

void vm_shutdown(vm_t vm)
{
    if (vm)
    {
        for (rt_size_t i = 0; i < vm->nr_vcpus; i++)
        {
            vcpu_t vcpu = vm->vcpus[i];
            rt_thread_delete(vcpu->tid);
        }

        /* 
         * and release VM memory resource. TBD 
         */

        vm->status = VM_STATUS_OFFLINE;
    }
}

void vm_free(vm_t vm)
{
    vgic_free(vm->vgic);

    /* free vCPUs resource */
    for (rt_size_t i = 0; i < vm->nr_vcpus; i++)
        vcpu_free(vm->vcpus[i]);

    /* free other resource, like memory resouece & TBD */
}
