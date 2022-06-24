/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-05-30     Suqier       first version
 */

#include <rthw.h>
#include <rtthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <optparse.h>

#include <armv8.h>
#include <lib_helpers.h>
#include <vhe.h>

#include "hypervisor.h"
#include "vm.h"

#ifndef RT_USING_SMP
#define RT_CPUS_NR      1
extern int rt_hw_cpu_id(void);
#else
extern rt_uint64_t rt_cpu_mpidr_early[];
#endif /* RT_USING_SMP */

#define THREAD_TIMESLICE    5

static struct rt_thread hyp_cpu_init[RT_CPUS_NR];
static rt_uint8_t hyp_stack[RT_CPUS_NR][2048];
static int parameter[RT_CPUS_NR];

// #if defined(RT_HYPERVISOR)

static struct hypervisor rt_hyp;

static rt_uint32_t __convert_width(rt_uint32_t parange)
{
	switch (parange) 
    {
    default:
	case 0: return 32;
	case 1: return 36;
	case 2: return 40;
	case 3: return 42;
	case 4: return 44;
	case 5: return 48;
	case 6: return 52;
	}
}

static void __set_ipa_size(void)
{
    rt_uint64_t val;
    rt_uint32_t parange, t0sz;

    GET_SYS_REG(ID_AA64MMFR0_EL1, val);
    parange = __convert_width(val & ID_AA64MMFR0_MASK);

    GET_SYS_REG(VTCR_EL2, val);
    t0sz = VTCR_EL2_T0SZ(val & VTCR_EL2_T0SZ_MASK);
    
    /* min, set as 40bit */
    rt_hyp.arch->ipa_size = parange <= t0sz ? parange : t0sz;
}

static void __set_vmid_bits(void)
{
    rt_hyp.arch->vmid_bits = arm_vmid_bits();
    rt_kprintf("[Info] Hypervisor support vmid is %d bits.\n", 
                    rt_hyp.arch->vmid_bits);
}

/* per cpu should run this function to enable hyp mode. */
static void __cpu_hyp_enable_entry(void* parameter)
{
    int *i = parameter;

    /* Judge whether currentEL is EL2 or not */
    rt_ubase_t currEL = rt_hw_get_current_el();
    if (currEL != 2)
    {
        rt_kprintf("[Error] %d-th CPU currentEL is %d.\n", *i, currEL);
        return;
    }

    rt_hyp.arch->cpu_hyp_enabled[*i] = RT_TRUE;
    return;
}

static void __init_cpu(void)
{
    char hyp_thread_name[RT_NAME_MAX];

    for (rt_size_t i = 0; i < RT_CPUS_NR; i++)
    {
        parameter[i] = i;
        rt_sprintf(hyp_thread_name, "hyp%d", i);
        rt_thread_init(&hyp_cpu_init[i], 
                    hyp_thread_name,
                    __cpu_hyp_enable_entry, 
                    (void *)&parameter[i], 
                    &hyp_stack[i],
                    2048, 
                    FINSH_THREAD_PRIORITY, 
                    THREAD_TIMESLICE);
        rt_thread_control(&hyp_cpu_init[i], 
                        RT_THREAD_CTRL_BIND_CPU, 
                        (void *)i);
        rt_thread_startup(&hyp_cpu_init[i]);
    }

    /* Give up the current CPU so that it can initialize itself */
    rt_thread_yield();  
}

static rt_err_t __init_subsystems(void)
{
    rt_uint64_t ret = RT_EOK;
    rt_bool_t cpu_hyp = RT_TRUE;

    /* init cpu part */
    __init_cpu();
    for (rt_size_t i = 0; i < RT_CPUS_NR; i++)
        cpu_hyp &= rt_hyp.arch->cpu_hyp_enabled[i];

    if (cpu_hyp)
        rt_kprintf("[Info] All cpu hyp enabled.\n");
    else
    {
        rt_kprintf("[Error] cpu hyp disable.\n");
        return -RT_ERROR;
    }
    
    /* init vgic */

    /* init vtimer */

    return ret;
}

static rt_err_t __rt_hyp_init(void)
{
    rt_hyp.total_vm = 0;

    rt_hyp.phy_mem_size = HYP_MEM_SIZE;
    rt_hyp.phy_mem_used = 0;

    rt_hyp.next_vm_idx = 0;
    rt_hyp.curr_vm_idx = 0;
    rt_kprintf("[Info] Support %d VMs max.\n", MAX_VM_NUM);

    rt_hw_spin_lock_init(&rt_hyp.hyp_lock);
    rt_hyp.arch = (struct hyp_arch *)rt_malloc(sizeof(struct hyp_arch));
    if (rt_hyp.arch == RT_NULL)
    {
        rt_kprintf("[Error] Allocate memory for rt_hyp.arch failure.\n");
        return -RT_ENOMEM;
    }
    
    for (rt_size_t i = 0; i < RT_CPUS_NR; i++)
        rt_hyp.arch->cpu_hyp_enabled[i] = RT_FALSE;

    rt_hyp.arch->hyp_init_ok = RT_FALSE;

    return RT_EOK;
}

static rt_err_t __hyp_arch_init(void)
{
    rt_err_t ret;
    
    /* Software part init: arch feature detect and rt_hyp set */
    __set_ipa_size();
    __set_vmid_bits();

    /* Hardware part init: such as cpu, gic, timer and so on */
    ret = __init_subsystems();

    return ret;
}

rt_err_t rt_hypervisor_init(void)
{
    rt_err_t ret;
    ret = __rt_hyp_init();
    if (ret != RT_EOK)
        return ret;
    
    ret = __hyp_arch_init();
    if (ret != RT_EOK)
        return ret;

    rt_kprintf("[Info] RT-Hypervisor init over.\n");
    return ret;
}

void help_vm(void)
{
    rt_kprintf("RT-Hypervisor shell command:\n");    
    rt_kprintf("%2s- %s\n", "help_vm", "print hypervisor related command.");
    rt_kprintf("%2s- %s\n", "list_vm", "list all vm details.");
    rt_kprintf("%2s- %s\n", "create_vm", "create new vm.");
}

static rt_uint16_t __find_next_vm_idx(void)
{
    /*
     * The value rt_hyp.next_vmid always remains 
     * the currently available minimum value.
     */
    rt_uint16_t vm_idx = rt_hyp.next_vm_idx;

    do
    {
        rt_hyp.next_vm_idx++;
    } while (rt_hyp.next_vm_idx == MAX_VM_NUM 
          || rt_hyp.vms[rt_hyp.next_vm_idx] != RT_NULL);

    return vm_idx;
}

rt_err_t create_vm(int argc, char **argv)
{
    rt_err_t ret = RT_EOK;
    rt_uint16_t vm_idx;
    struct vm* new_vm;
    char *arg;
    int opt;
    struct optparse options;

    /* First time to create vm need to init all hyp system. */
    if (!rt_hyp.arch->hyp_init_ok)
    {
        ret = rt_hypervisor_init();
        if (ret != RT_EOK)
            return ret;
        else
            rt_hyp.arch->hyp_init_ok = RT_TRUE;
    }

    /* new vm: allcate memory and index */
    if (rt_hyp.total_vm == MAX_VM_NUM)
    {
        rt_kprintf("[Error] The number of VMs is full.\n");
        return -RT_ERROR;
    }
    else
        vm_idx = __find_next_vm_idx();

    new_vm = (struct vm*)rt_malloc(sizeof(struct vm));
    if (!new_vm)
    {
        rt_kprintf("[Error] Allocate memory for new vm failure.\n");
        return -RT_ENOMEM;
    }
    else
    {
        vm_config_init(new_vm, vm_idx);
        
        rt_hw_spin_lock(&rt_hyp.hyp_lock);
        rt_hyp.vms[vm_idx] = new_vm;
        rt_hyp.curr_vm_idx = vm_idx;
        rt_hyp.total_vm++;
        rt_hw_spin_unlock(&rt_hyp.hyp_lock);
    }

    /* 
     * "t" for os type and "n" for vm name. for example:
     * -t linux -n test_1
     */
    optparse_init(&options, argv);
    while ((opt = optparse(&options, "t:n:")) != -1) 
    {
        switch (opt) 
        {
        case 't':
            new_vm->os_type = vm_os_type(options.optarg);
            break;
        case 'n':
            strcpy(new_vm->name, options.optarg);
            break;
        case '?':
            rt_kprintf("[Error] %s: %s.\n", argv[0], options.errmsg);
            return RT_ERROR;
        }
    }

    /* Print remaining arguments. */
    while ((arg = optparse_arg(&options)))
        printf("%s\n", arg);

    return ret;
}

/*
 * setup VM:
 * -> -i  change VM index
 * -> -c  vcpu number
 * -> -m  memory size
 * -> -o  os image path (need file system support)
 * -> -r  ramdisk path
 * -> -d  dtb path (should parse dtb file before running)
 * ...
 */
rt_err_t setup_vm(int argc, char **argv)
{
    rt_err_t ret = RT_EOK;
    char *arg;
    int opt;
    struct optparse options;

    rt_uint16_t vm_idx;
    rt_uint8_t nr_vcpus;
    rt_uint64_t mem_size;

    void *entry_point;
    char *os_path;
    char *ramdisk_path;
    char *dtb_path;

    optparse_init(&options, argv);
    while ((opt = optparse(&options, "i:c:m:o:r:d:e:")) != -1) 
    {
        switch (opt)
        {
        case 'i':
            vm_idx = atoi(options.optarg);
            if (vm_idx >= MAX_VM_NUM)
            {
                rt_kprintf("[Error] Input VM index %d is out of scope.\n", vm_idx);
                return -RT_ERROR;
            }            
            break;
        case 'c':
            nr_vcpus = atoi(options.optarg);
            if (nr_vcpus == 0 || nr_vcpus > MAX_VCPU_NUM)
            {
                rt_kprintf("[Error] Input vCPU number %d is out of scope.\n", nr_vcpus);
                return -RT_ERROR;
            }
            break;
        case 'm':
            mem_size = atoi(options.optarg);
            if (mem_size == 0 || mem_size > HYP_MEM_SIZE)
            {
                rt_kprintf("[Error] Memory size %d is out of scope.\n", mem_size);
                return -RT_ERROR;
            }
            break;
        case 'o':
            os_path = options.optarg;
            /* these three file path are not check temporarily */
            break;
        case 'r':
            ramdisk_path = options.optarg;
            break;
        case 'd':
            dtb_path = options.optarg;
            break;
        case 'e':
            entry_point = options.optarg;
            break;
        case '?':
            rt_kprintf("[Error] %s: %s.\n", argv[0], options.errmsg);
            return RT_ERROR;
        }
    }

    /* Print remaining arguments. */
    while ((arg = optparse_arg(&options)))
        printf("%s\n", arg);

    /* All parameters are suitable, then setup corresponding VM */
    rt_hyp.curr_vm_idx = vm_idx;

    struct vm *set_vm = rt_hyp.vms[rt_hyp.curr_vm_idx];
    set_vm->nr_vcpus = nr_vcpus;
    set_vm->mm.mem_size = mem_size;
    set_vm->os_path = os_path;
    set_vm->ramdisk_path = ramdisk_path;
    set_vm->dtb_path = dtb_path;
    set_vm->entry_point = entry_point;

    return ret;
}

rt_err_t run_vm(int vm_idx)
{
    /* 
     * Check VM's environmental preparation, when VM is ready,
     * then changes VM status and schedules vCPUs.
     */
    RT_ASSERT((vm_idx < MAX_VM_NUM) && (vm_idx >= 0));

    rt_uint64_t ret = RT_EOK;
    struct vm *vm = rt_hyp.vms[vm_idx];

    if (vm)
    {
        switch (vm->status)
        {
        case VM_STATUS_OFFLINE:
            /* open this vm and schedule vcpus. */
            vm->status = VM_STATUS_ONLINE;
            vm_go(vm);
            break;
        case VM_STATUS_ONLINE:
            rt_kprintf("[Info] %d-th VM is running.\n", vm_idx);
            break;
        case VM_STATUS_SUSPEND:
            /* start schedule vcpus. */
            vm->status = VM_STATUS_ONLINE;
            vm_go(vm);
            break;
        case VM_STATUS_NEVER_RUN:
            /* 
             * Before it runs, we should allocate resource for it.
             * Now it has only memory for struct vm. 
             */
            ret = vm_init(vm);
            if (ret != RT_EOK)
                return ret;
            else
            {
                vm->status = VM_STATUS_ONLINE;
                vm_go(vm);
            }
            break;
        case VM_STATUS_UNKNOWN:
        default:
            rt_kprintf("[Error] %d-th VM's status unknown.\n", vm_idx);
            ret = -RT_ERROR;
            break;
        }
    }
    else
    {
        rt_kprintf("[Error] %d-th VM is not set.\n", vm_idx);
        ret = -RT_EINVAL;
    }

    return ret;
}

rt_err_t pause_vm(int vm_idx)
{
    /* 
     * Changes VM status and stops schedule vCPUs.
     */
    RT_ASSERT((vm_idx < MAX_VM_NUM) && (vm_idx >= 0));
    
    rt_uint64_t ret = RT_EOK;
    struct vm *vm = rt_hyp.vms[vm_idx];
    if (vm)
        vm_pause(vm);
    else
    {
        rt_kprintf("[Error] %d-th VM is not set.\n", vm_idx);
        ret = -RT_EINVAL;
    }

    return ret;
}

rt_err_t halt_vm(int vm_idx)
{
    /* 
     * Stop running VM and turn VM's status down. 
     */
    RT_ASSERT((vm_idx < MAX_VM_NUM) && (vm_idx >= 0));
    return RT_EOK;
}

rt_err_t delete_vm(rt_uint16_t vm_idx)
{
    /* 
     * Delete VM.
     */
    RT_ASSERT((vm_idx < MAX_VM_NUM) && (vm_idx >= 0));

    if (rt_hyp.vms[vm_idx])
    {
        /* halt the VM and free VM memory */
        rt_err_t ret = halt_vm(vm_idx);
        if (ret == 0)
        {
            struct vm *del_vm = rt_hyp.vms[vm_idx];
            rt_hyp.vms[vm_idx] = RT_NULL;
            rt_free(del_vm);    /* and free others resource. */
            rt_kprintf("[Info] Delete %d-th VM success.\n", vm_idx);
            return RT_EOK;
        }
        else
        {
            rt_kprintf("[Error] VM index %d halt failure.\n", vm_idx);
            return -RT_ERROR;
        }
    }
    else
    {
        rt_kprintf("[Error] %d-th VM is not set.\n", vm_idx);
        return -RT_EINVAL;
    }
}

#if defined(RT_USING_FINSH)
rt_inline void object_split(int len)
{
    while (len--) rt_kprintf("-");
}

/**
 * Virtual machine is not kernel object, 
 * so it's list_cmd can not put into cmd.c
 */
long list_vm(void)
{
    const char *item_title = "vm name";
    int maxlen = VM_NAME_SIZE;

    /*
     *  msh >list_vm
     *  vm name          vm_idx status       OS     vcpu  mem(M)
     *  ---------------- ------ -------- ---------- ---- -------
     *  linux_test_1        001 offline  Linux         4    1024
     *  Zephyr_test         002 never    Zephyr        1     128
     */
    rt_kprintf("%-*.s vm_idx status       OS     vcpu  mem(M)\n", 
            maxlen, item_title);
    object_split(maxlen);
    rt_kprintf(" ------ -------- ---------- ---- --------\n");

    for (rt_size_t i = 0; i < MAX_VM_NUM; i++)
    {
        struct vm* _vm = rt_hyp.vms[i];
        if (_vm)
        {
            rt_kprintf("%-*.*s %6.3d %-8.s %-10s %4.1d %8d\n",
                    maxlen, VM_NAME_SIZE, 
                    _vm->name,
                    _vm->vm_idx,
                    vm_status_str(_vm->status),
                    os_type_str(_vm->os_type),
                    _vm->nr_vcpus,
                    _vm->mm.mem_size);
        }
    }
    return 0;
}
MSH_CMD_EXPORT(list_vm, list all vm detail);

MSH_CMD_EXPORT(help_vm, print hypervisor help info);
MSH_CMD_EXPORT(create_vm, create new vm);
// MSH_CMD_EXPORT(setup_vm, set up vm);
// MSH_CMD_EXPORT(run_vm, run vm by index);
// MSH_CMD_EXPORT(pause_vm, pause vm by index);
// MSH_CMD_EXPORT(halt_vm, halt vm by index);
MSH_CMD_EXPORT(delete_vm, delete vm by index);
#endif /* RT_USING_FINSH */


// #endif  /* defined(RT_HYPERVISOR) */ 