/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-05-30     Suqier       first version
 */

#include "bitmap.h"
#include "hypervisor.h"
#include "switch.h"
#include "hyp_debug.h"
#include "hyp_fdt.h"

#include <vgic.h>

#define FDT_ADDR    0x46000000

#ifndef RT_USING_SMP
#define RT_CPUS_NR      1
extern int rt_hw_cpu_id(void);
#else
extern rt_uint64_t rt_cpu_mpidr_early[];
#endif /* RT_USING_SMP */

static struct rt_thread hyp_cpu_init[RT_CPUS_NR];
static rt_uint8_t hyp_stack[RT_CPUS_NR][2048];
static int parameter[RT_CPUS_NR];

extern const char* vm_stat_str[VM_STAT_UNKNOWN + 1];
extern const char* os_type_str[OS_TYPE_OTHER + 1];

struct hypervisor rt_hyp;

static rt_uint32_t fdt_get_cell(struct dtb_node *dtb_node, const char *property_name)
{
    rt_size_t property_size;
    rt_uint32_t *u32_ptr;

    u32_ptr = fdt_get_dtb_node_property(dtb_node, property_name, &property_size);
    return fdt_get_dtb_cell_value(u32_ptr);
}

rt_err_t vm_info_init(void)
{
    void *fdt;
    if ((fdt = fdt_load_from_memory((void *)FDT_ADDR, RT_FALSE)) != RT_NULL)
    {
        struct dtb_node *dtb_node_list = fdt_get_dtb_list(fdt);
        if (dtb_node_list != RT_NULL)
        {
            struct dtb_node *vm = fdt_get_dtb_node_by_path(dtb_node_list, "/vm");
            if (vm != RT_NULL)
            {
                struct dtb_node *vm_child = vm;

                /* traverse all VM */
                for_each_node_child(vm_child)
                {
                    rt_size_t p_size, index;
                    rt_uint32_t u32_val, *u32_ptr, vmid;

                    vmid = fdt_get_cell(vm_child, "vmid");
                    rt_hyp.vms[vmid].id = vmid;
                    struct vm_info *info = &rt_hyp.vms[vmid].info;

                    info->img_type = fdt_get_cell(vm_child, "type");
                    info->nr_vcpus = fdt_get_cell(vm_child, "vcpus");
                    
                    index = 0;
                    for_each_property_cell(vm_child, "vcpu_affinity", u32_val, u32_ptr, p_size)
                    {
                        info->affinity[index++] = u32_val;
                    }
                    
                    for_each_property_cell(vm_child, "image_address", u32_val, u32_ptr, p_size)
                    {
                        info->img_addr = u32_val;
                    }
                    
                    info->img_size = 0x20000;

                    for_each_property_cell(vm_child, "entry", u32_val, u32_ptr, p_size)
                    {
                        info->img_entry = u32_val;
                    }
                    
                    rt_strncpy(rt_hyp.vms[vmid].name, vm_child->name, RT_NAME_MAX);
                    rt_hyp.vms[vmid].status = VM_STAT_NEVER_RUN;

                    index = 0;
                    for_each_property_cell(vm_child, "memory", u32_val, u32_ptr, p_size)
                    {
                        index++;
                        if (index == 2)
                            info->va_addr = u32_val;

                        if (index == 4)
                            info->va_size = u32_val;
                    }

                    index = 0;
                    for_each_property_cell(vm_child, "phymem", u32_val, u32_ptr, p_size)
                    {
                        index++;
                        if (index == 2)
                            info->pa_addr = u32_val;

                        if (index == 4)
                            info->pa_size = u32_val;
                    }

                    struct dtb_node *vm_device = vm_child->child;
                    for_each_node_child(vm_device)
                    {
                        struct dtb_node *sub_node = vm_device;
                        char *dev_name = 
                        fdt_get_dtb_node_property(sub_node, "compatible", RT_NULL);
                        
                        if (rt_strcmp("arm,gicv3", dev_name) == 0)
                        {
                            info->maintenance_id = fdt_get_cell(sub_node, "maintenance_interrupts");
                            info->virq_num = fdt_get_cell(sub_node, "virq_num");
                            
                            index = 0;
                            for_each_property_cell(sub_node, "reg", u32_val, u32_ptr, p_size)
                            {
                                index++;
                                if (index == 1)
                                    info->gicd_addr = u32_val;

                                if (index == 3)
                                    info->gicr_addr = u32_val;
                            }
                        }

                        if (rt_strcmp("rt_thread,vm_console", dev_name) == 0)
                        {
                            info->dev_num = 1;
                            rt_size_t dev_index = 0;

                            for_each_property_cell(sub_node, "interrupts", u32_val, u32_ptr, p_size)
                            {
                                info->devs[dev_index].interrupts = u32_val;
                            }

                            index = 0;
                            for_each_property_cell(sub_node, "reg", u32_val, u32_ptr, p_size)
                            {
                                index++;
                                if (index == 1)
                                {
                                    info->devs[dev_index].va_addr = u32_val;
                                    info->devs[dev_index].pa_addr = u32_val;
                                }

                                if (index == 2)
                                    info->devs[dev_index].size = u32_val;
                            }
                        }
                    }

                    rt_hyp.total_vm++;
                }
            }
        }
        fdt_free_dtb_list(dtb_node_list);
        hyp_info("VM info load from device tree over");
    }

    return RT_EOK;
}

int rt_hyp_init(void)
{
    rt_hyp.total_vm = 0;

    rt_hyp.phy_mem_size = HYP_MEM_SIZE;
    rt_hyp.phy_mem_used = 0;

    rt_hyp.curr_vm = 0;

    rt_memset((void *)&rt_hyp.vms, 0, MAX_VM_NUM * sizeof(struct vm));
    vm_info_init();
    hyp_info("Support %d VMs max, Now load %d VMs", MAX_VM_NUM, rt_hyp.total_vm);

#ifdef RT_USING_SMP
    rt_hw_spin_lock_init(&rt_hyp.hyp_lock);
#endif
    
    for (rt_size_t i = 0; i < RT_CPUS_NR; i++)
        rt_hyp.arch.cpu_hyp_enabled[i] = RT_FALSE;

    rt_hyp.arch.hyp_init_ok = RT_FALSE;

    rt_hyp.curr_vc = MAX_VM_NUM;    /* MAX_VM_NUM == Host using UART */

    hyp_info("rt_hyp init OK");
    return RT_EOK;
}
INIT_APP_EXPORT(rt_hyp_init);

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
    rt_hyp.arch.ipa_size = parange <= t0sz ? parange : t0sz;
}

static void __set_vmid_bits(void)
{
    rt_hyp.arch.vmid_bits = arm_vmid_bits();
    hyp_info("VM id support %d bits", rt_hyp.arch.vmid_bits);
}

/* per cpu should run this function to enable hyp mode. */
static void __cpu_hyp_enable_entry(void* parameter)
{
    int *i = parameter;

    /* Judge whether currentEL is EL2 or not */
    rt_ubase_t currEL = rt_hw_get_current_el();
    if (currEL != 2)
    {
        hyp_err("%dth CPU currentEL is %d", *i, currEL);
        return;
    }

    rt_hyp.arch.cpu_hyp_enabled[*i] = RT_TRUE;
    return;
}

static void __init_cpu(void)
{
    char hyp_thread_name[RT_NAME_MAX];

    for (rt_size_t i = 0; i < RT_CPUS_NR; i++)
    {
        parameter[i] = i;
        rt_sprintf(hyp_thread_name, "hyp%d", i);
        rt_thread_init(&hyp_cpu_init[i], hyp_thread_name, __cpu_hyp_enable_entry, 
                    (void *)&parameter[i], &hyp_stack[i], 2048, 
                    FINSH_THREAD_PRIORITY - 1, THREAD_TIMESLICE);
        rt_thread_control(&hyp_cpu_init[i], RT_THREAD_CTRL_BIND_CPU, (void *)i);
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
        cpu_hyp &= rt_hyp.arch.cpu_hyp_enabled[i];

    if (cpu_hyp)
        hyp_info("All CPU hyp enabled");
    else
    {
        // hyp_err("CPU hyp disable");
        return -RT_ERROR;
    }
    
    /* init vtimer */

    return ret;
}

static rt_err_t __hyp_arch_init(void)
{
    rt_err_t ret;
    
    /* Software part init: arch feature detect and rt_hyp set */
    __set_ipa_size();
    __set_vmid_bits();
    rt_init_s2_mmu_table();

    /* Hardware part init: such as cpu, gic, timer and so on */
    ret = __init_subsystems();

    return ret;
}

int rt_hypervisor_init(void)
{
    rt_scheduler_sethook(switch_hook);

    int ret = __hyp_arch_init();
    if (ret != RT_EOK)
        return ret;

    hyp_info("RT-Hypervisor init over");
    return ret;
}
INIT_APP_EXPORT(rt_hypervisor_init);

#if defined(RT_USING_FINSH)
void help_vm(void)
{
    rt_kprintf("RT-Hypervisor shell command:\n");    
    rt_kprintf("%2s- %s\n", "help_vm", "print hypervisor related command.");
    rt_kprintf("%2s- %s\n", "list_vm", "list all vm details.");
    rt_kprintf("%2s- %s\n", "create_vm", "create new vm.");
}

rt_inline void object_split(int len)
{
    while (len--) rt_kprintf("-");
}

void list_vm(void)
{
    const char *item_title = "vm name";
    int maxlen = VM_NAME_SIZE;
    char *fmt;

    /*
     *  msh >list_vm
     *  vm name           vm id status       OS     vcpu  mem(M)
     *  ---------------- ------ -------- ---------- ---- -------
     *  linux_test_1        001 offline  Linux         4      64
     *  Zephyr_test         002 never    Zephyr        1      64
     */
    rt_kprintf("%-*.s  vm id status   OS type    vcpu mem(M)\n", 
            maxlen, item_title);
    object_split(maxlen);
    rt_kprintf(" ------ -------- ---------- ---- ------\n");

    for (rt_size_t i = 0; i < MAX_VM_NUM; i++)
    {
        vm_t vm = &rt_hyp.vms[i];
        if (vm->status != VM_STAT_IDLE)
        {
            if (i == rt_hyp.curr_vm)
                fmt = "\033[34m%-*.*s %6.3d %-8.s %-10s %4.1d %6d\n\033[0m";
            else
                fmt = "%-*.*s %6.3d %-8.s %-10s %4.1d %6d\n";
            
            rt_kprintf(fmt, maxlen, VM_NAME_SIZE, vm->name, vm->id,
                    vm_stat_str[vm->status], os_type_str[vm->info.img_type],
                    vm->info.nr_vcpus, MB(vm->info.va_size));
        }
    }
}

void pick_vm(int argc, char **argv)
{
    int opt;
    rt_uint8_t vm_idx;
    struct optparse options;

    optparse_init(&options, argv);
    while ((opt = optparse(&options, "i:")) != -1) 
    {
        switch (opt) 
        {
        case 'i':
            vm_idx = strtol((const char *)options.optarg, NULL, 10);
            if (vm_idx < 0 || vm_idx >= MAX_VM_NUM)
            {
                hyp_err("%dth VM: Out of scope", vm_idx);
                return;
            }
#ifdef RT_USING_SMP
            rt_hw_spin_lock(&rt_hyp.hyp_lock);
#endif
            rt_hyp.curr_vm = vm_idx;
#ifdef RT_USING_SMP
            rt_hw_spin_unlock(&rt_hyp.hyp_lock);
#endif 
            list_vm();
            break;
        case '?':
            hyp_err("%s: %s", argv[0], options.errmsg);
            return;
        }
    }
}

rt_err_t run_vm(void)
{
    /* 
     * Check VM's environmental preparation, when VM is ready,
     * then changes VM status and schedules vCPUs.
     */
    rt_uint64_t ret = RT_EOK;
    rt_uint64_t vm_idx = rt_hyp.curr_vm;
    vm_t vm = &rt_hyp.vms[vm_idx];
    if (vm)
    {
        switch (vm->status)
        {
        case VM_STAT_OFFLINE:
            /* open this vm and schedule vcpus. */
            vm->status = VM_STAT_ONLINE;
            hyp_info("%dth VM: Open", vm_idx);
            vm_go(vm);
            break;
        case VM_STAT_ONLINE:
            hyp_warn("%dth VM: Running", vm_idx);
            break;
        case VM_STAT_SUSPEND:
            /* start schedule vcpus. */
            vm->status = VM_STAT_ONLINE;
            hyp_info("%dth VM: Continue", vm_idx);
            vm_go(vm);
            break;
        case VM_STAT_NEVER_RUN:
            /* 
             * Before it runs, we should allocate resource for it.
             * Now it has only memory for struct vm. 
             */
            ret = vm_init(vm);
            if (ret != RT_EOK)
            {
                hyp_err("%dth VM: Init failure", vm_idx);
                return ret;
            }
            else
            {
                vm->status = VM_STAT_ONLINE;
                hyp_info("%dth VM: Run first time", vm_idx);
                vm_go(vm);
            }
            break;
        case VM_STAT_UNKNOWN:
        default:
            hyp_err("%dth VM: Status unknown", vm_idx);
            ret = -RT_ERROR;
            break;
        }
    }
    else
    {
        hyp_err("%dth VM: Not use", vm_idx);
        ret = -RT_EINVAL;
    }

    return ret;
}

rt_err_t pause_vm(void)
{
    /* Changes VM status and stops schedule vCPUs */
    rt_uint64_t ret = RT_EOK;
    rt_uint64_t vm_idx = rt_hyp.curr_vm;
    vm_t vm = &rt_hyp.vms[vm_idx];
    if (vm->status != VM_STAT_IDLE)
    {
        vm->status = VM_STAT_SUSPEND;
        vm_suspend(vm);
    }
    else
    {
        hyp_err("%dth VM: Not set", vm_idx);
        ret = -RT_EINVAL;
    }

    return ret;
}

rt_err_t halt_vm(void)
{
    /* Stop running VM and turn VM's status down */
    rt_uint64_t ret = RT_EOK;
    rt_uint64_t vm_idx = rt_hyp.curr_vm;
    vm_t vm = &rt_hyp.vms[vm_idx];
    if (vm->status != VM_STAT_IDLE)
    {
        vm->status = VM_STAT_OFFLINE;
        vm_shutdown(vm);
    }
    else
    {
        hyp_err("%dth VM: Not set", vm_idx);
        ret = -RT_EINVAL;
    }

    return ret;
}

void print_el(void)
{
    rt_ubase_t currEL = rt_hw_get_current_el();
    hyp_debug("Now is at EL%d", currEL);
}

void print_virq(int vm_idx)
{
    if (rt_hyp.vms[vm_idx].vgic.gicd)
    {
        rt_kprintf("__ %dth VM __\nvINTID = %d\npINTID = %d\nenable = %d\nhw     = %d\n\n",
            vm_idx,
            rt_hyp.vms[vm_idx].vgic.gicd->virqs[1].vINIID,
            rt_hyp.vms[vm_idx].vgic.gicd->virqs[1].pINTID,
            rt_hyp.vms[vm_idx].vgic.gicd->virqs[1].enable,
            rt_hyp.vms[vm_idx].vgic.gicd->virqs[1].hw);
    }
}

void dump_virq(void)
{
    print_virq(0);
    print_virq(1);
    print_virq(2);
    print_virq(3);
}

MSH_CMD_EXPORT(print_el, print current EL);
MSH_CMD_EXPORT(list_vm, list all vm detail);
MSH_CMD_EXPORT(help_vm, print hypervisor help info);
MSH_CMD_EXPORT(pick_vm, change current picking vm);
MSH_CMD_EXPORT(run_vm, run vm by index);
MSH_CMD_EXPORT(pause_vm, pause vm by index);
MSH_CMD_EXPORT(halt_vm, halt vm by index);
MSH_CMD_EXPORT(dump_virq, for test);
MSH_CMD_EXPORT(vm_info_init, for test);
#endif /* RT_USING_FINSH */