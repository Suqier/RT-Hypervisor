/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-05-30     Suqier       first version
 */

#include "hypervisor.h"

#ifndef RT_USING_SMP
#define RT_CPUS_NR      1
extern int rt_hw_cpu_id(void);
#else
extern rt_uint64_t rt_cpu_mpidr_early[];
#endif /* RT_USING_SMP */

static struct rt_thread hyp_cpu_init[RT_CPUS_NR];
static rt_uint8_t hyp_stack[RT_CPUS_NR][2048];
static int parameter[RT_CPUS_NR];

#ifdef RT_HYPERVISOR

extern const struct os_desc os_support[MAX_OS_TYPE];
extern const char* vm_status_str[VM_STATUS_UNKNOWN + 1];
extern const char* os_type_str[OS_TYPE_OTHER + 1];

static struct hypervisor rt_hyp;

int rt_hyp_init(void)
{
    rt_hyp.total_vm = 0;

    rt_hyp.phy_mem_size = HYP_MEM_SIZE;
    rt_hyp.phy_mem_used = 0;

    rt_hyp.next_vm_idx = 0;
    rt_hyp.curr_vm_idx = MAX_VM_NUM;
    rt_kprintf("[Info] Support %d VMs max.\n", MAX_VM_NUM);

#ifdef RT_USING_SMP
    rt_hw_spin_lock_init(&rt_hyp.hyp_lock);
#endif
    
    for (rt_size_t i = 0; i < RT_CPUS_NR; i++)
        rt_hyp.arch.cpu_hyp_enabled[i] = RT_FALSE;

    rt_hyp.arch.hyp_init_ok = RT_FALSE;

    rt_kprintf("[Info] RT_H: rt_hyp init OK.\n");
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
    rt_kprintf("[Info] VM id support %d bits.\n", rt_hyp.arch.vmid_bits);
}

/* per cpu should run this function to enable hyp mode. */
static void __cpu_hyp_enable_entry(void* parameter)
{
    int *i = parameter;

    /* Judge whether currentEL is EL2 or not */
    rt_ubase_t currEL = rt_hw_get_current_el();
    if (currEL != 2)
    {
        rt_kprintf("[Error] %dth CPU currentEL is %d.\n", *i, currEL);
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
        rt_thread_init(&hyp_cpu_init[i], 
                    hyp_thread_name,
                    __cpu_hyp_enable_entry, 
                    (void *)&parameter[i], 
                    &hyp_stack[i],
                    2048, 
                    FINSH_THREAD_PRIORITY - 1, 
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
        cpu_hyp &= rt_hyp.arch.cpu_hyp_enabled[i];

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
    rt_kprintf("[Info] Start RT-Hypervisor init\n");

    rt_err_t ret;
    ret = __hyp_arch_init();
    if (ret != RT_EOK)
        return ret;

    rt_kprintf("[Info] RT-Hypervisor init over\n");
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
    rt_uint16_t vm_idx, os_type;
    struct vm *new_vm;
    struct mm_struct *mm;
    char *arg;
    int opt;
    struct optparse options;

    /* First time to create vm need to init all hyp system. */
    if (!rt_hyp.arch.hyp_init_ok)
    {
        ret = rt_hypervisor_init();
        if (ret != RT_EOK)
            return ret;
        else
            rt_hyp.arch.hyp_init_ok = RT_TRUE;
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
    mm = (struct mm_struct*)rt_malloc(sizeof(struct mm_struct));
    if (!new_vm || !mm)
    {
        rt_kprintf("[Error] Allocate memory for new vm failure.\n");
        rt_free(new_vm);
        rt_free(mm);
        return -RT_ENOMEM;
    }
    else
    {
        new_vm->mm = mm;
        mm->vm = new_vm;
    }

    /* 
     * "i" for os type and "n" for vm name. for example:
     * -i os_img_type -n test_1
     */
    optparse_init(&options, argv);
    while ((opt = optparse(&options, "i:n:")) != -1) 
    {
        switch (opt) 
        {
        case 'i':
            os_type = strtol((const char *)options.optarg, NULL, 10);
            if(os_type < 0 || os_type >= MAX_OS_TYPE)
            {
                rt_kprintf("[Error] OS_type %d is out of scope\n", os_type);
                return -RT_EINVAL;
            }
            new_vm->os = &os_support[os_type];
            break;
        case 'n':
            strncpy(new_vm->name, options.optarg, VM_NAME_SIZE);
            break;
        case '?':
            rt_kprintf("[Error] %s: %s.\n", argv[0], options.errmsg);
            return -RT_ERROR;
        }
    }

    /* Print remaining arguments. */
    while ((arg = optparse_arg(&options)))
        printf("%s\n", arg);

    vm_config_init(new_vm, vm_idx);

#ifdef RT_USING_SMP
    rt_hw_spin_lock(&rt_hyp.hyp_lock);
#endif

    rt_hyp.vms[vm_idx] = new_vm;
    rt_hyp.curr_vm_idx = vm_idx;
    rt_hyp.total_vm++;

#ifdef RT_USING_SMP
    rt_hw_spin_unlock(&rt_hyp.hyp_lock);
#endif

    return ret;
}

void pick_vm(int argc, char **argv)
{
    int opt;
    rt_uint16_t vm_idx;
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
                rt_kprintf("[Error] VM id %d is out of scope\n", vm_idx);
                return;
            }
#ifdef RT_USING_SMP
            rt_hw_spin_lock(&rt_hyp.hyp_lock);
#endif
            rt_hyp.curr_vm_idx = vm_idx;
#ifdef RT_USING_SMP
            rt_hw_spin_unlock(&rt_hyp.hyp_lock);
#endif 
            break;
        case '?':
            rt_kprintf("[Error] %s: %s.\n", argv[0], options.errmsg);
            return;
        }
    }

#ifdef RT_USING_SMP
    rt_hw_spin_lock(&rt_hyp.hyp_lock);
#endif

    if (rt_hyp.vms[vm_idx])
        rt_hyp.curr_vm_idx = vm_idx;
    else
        rt_kprintf("[Error] VM id %d is out of scope\n", vm_idx);

#ifdef RT_USING_SMP
    rt_hw_spin_unlock(&rt_hyp.hyp_lock);
#endif
}

static rt_err_t check_vm_idx(void)
{
    rt_uint64_t vm_idx = rt_hyp.curr_vm_idx;
    rt_uint64_t ret = RT_EOK;

    if (vm_idx < 0 || vm_idx >= MAX_VM_NUM)
    {
        rt_kprintf("[Error] %dth VM is not exist\n", vm_idx);
        ret = -RT_EINVAL;
    }

    return ret;
}

static void print_vm(void)
{
    rt_uint64_t ret = RT_EOK;
    ret = check_vm_idx();
    if (ret)
        return;

    rt_uint64_t vm_idx = rt_hyp.curr_vm_idx;
    struct vm *vm = rt_hyp.vms[vm_idx];
    rt_kprintf("[Debug] vm     = 0x%x\n", vm);
    rt_kprintf("[Debug] vm->os = 0x%x\n", vm->os);
    rt_kprintf("[Debug] vm->os->entry_point = 0x%x\n", vm->os->entry_point);
    rt_kprintf("[Debug] vm->mm = 0x%x\n", vm->mm);
}

rt_err_t run_vm(void)
{
    /* 
     * Check VM's environmental preparation, when VM is ready,
     * then changes VM status and schedules vCPUs.
     */
    rt_uint64_t ret = RT_EOK;
    ret = check_vm_idx();
    if (ret)
        return ret;

    rt_uint64_t vm_idx = rt_hyp.curr_vm_idx;
    struct vm *vm = rt_hyp.vms[vm_idx];
    if (vm)
    {
        switch (vm->status)
        {
        case VM_STATUS_OFFLINE:
            /* open this vm and schedule vcpus. */
            vm->status = VM_STATUS_ONLINE;
            rt_kprintf("[Info] Open %dth VM\n", vm_idx);
            go_vm(vm);
            break;
        case VM_STATUS_ONLINE:
            rt_kprintf("[Info] %dth VM is running\n", vm_idx);
            break;
        case VM_STATUS_SUSPEND:
            /* start schedule vcpus. */
            vm->status = VM_STATUS_ONLINE;
            rt_kprintf("[Info] Continue %dth VM\n", vm_idx);
            go_vm(vm);
            break;
        case VM_STATUS_NEVER_RUN:
            /* 
             * Before it runs, we should allocate resource for it.
             * Now it has only memory for struct vm. 
             */
            ret = vm_init(vm);
            if (ret != RT_EOK)
            {
                rt_kprintf("[Error] Init %dth VM failure\n", vm_idx);
                return ret;
            }
            else
            {
                vm->status = VM_STATUS_ONLINE;
                rt_kprintf("[Info] Run %dth VM first time\n", vm_idx);
                go_vm(vm);
            }
            break;
        case VM_STATUS_UNKNOWN:
        default:
            rt_kprintf("[Error] %dth VM's status unknown.\n", vm_idx);
            ret = -RT_ERROR;
            break;
        }
    }
    else
    {
        rt_kprintf("[Error] %dth VM is not set.\n", vm_idx);
        ret = -RT_EINVAL;
    }

    return ret;
}

rt_err_t pause_vm(void)
{
    /* 
     * Changes VM status and stops schedule vCPUs.
     */
    rt_uint64_t ret = RT_EOK;
    ret = check_vm_idx();
    if (ret)
        return ret;

    rt_uint64_t vm_idx = rt_hyp.curr_vm_idx;
    struct vm *vm = rt_hyp.vms[vm_idx];
    if (vm)
        suspend_vm(vm);
    else
    {
        rt_kprintf("[Error] %dth VM is not set.\n", vm_idx);
        ret = -RT_EINVAL;
    }

    return ret;
}

rt_err_t halt_vm(void)
{
    /* 
     * Stop running VM and turn VM's status down. 
     */
    rt_uint64_t ret = RT_EOK;
    ret = check_vm_idx();
    if (ret)
        return ret;

    rt_uint64_t vm_idx = rt_hyp.curr_vm_idx;
    struct vm* vm = rt_hyp.vms[vm_idx];
    shutdown_vm(vm);

    return RT_EOK;
}

rt_err_t delete_vm(void)
{
    rt_uint64_t ret = RT_EOK;
    ret = check_vm_idx();
    if (ret)
        return ret;

    rt_uint64_t vm_idx = rt_hyp.curr_vm_idx;
    if (rt_hyp.vms[vm_idx])
    {
        /* halt the VM and free VM memory */
        rt_err_t ret = halt_vm();
        if (ret == 0)
        {
            struct vm *del_vm = rt_hyp.vms[vm_idx];
            rt_hyp.vms[vm_idx] = RT_NULL;
            free_vm(del_vm);
            rt_kprintf("[Info] Delete %dth VM success.\n", vm_idx);
            return RT_EOK;
        }
        else
        {
            rt_kprintf("[Error] %dth VM halt failure.\n", vm_idx);
            return -RT_ERROR;
        }
    }
    else
    {
        rt_kprintf("[Error] %dth VM is not set.\n", vm_idx);
        return -RT_EINVAL;
    }
}

#if defined(RT_USING_FINSH)
rt_inline void object_split(int len)
{
    while (len--) rt_kprintf("-");
}

void list_os_img(void)
{
    /*
     *  msh >list_os_img
     *  OS type   OS id vcpu mem(M)
     *  --------- ----- ---- ------
     *  RT-Thread     0    1     64
     *  zephyr        1    2     64
     */
    const char *item_title = "OS type";
    int maxlen = VM_NAME_SIZE;
    rt_kprintf("%-*.s OS id vcpu mem(M)\n", maxlen, item_title);
    object_split(maxlen);
    rt_kprintf(" ----- ---- ------\n");

    for (rt_size_t i = 0; i < MAX_OS_TYPE; i++)
    {
        const struct os_desc *os = &os_support[i];
        if (os)
        {
            rt_kprintf("%-*.*s %5d %4.1d %6d\n", maxlen, VM_NAME_SIZE, 
                    os_type_str[os->os_type], i, os->nr_vcpus, os->mm_size);
        }
    }
}

void list_vm(void)
{
    const char *item_title = "vm name";
    int maxlen = VM_NAME_SIZE;
    char *fmt;

    /*
     *  msh >list_vm
     *  vm name          vm_idx status       OS     vcpu  mem(M)
     *  ---------------- ------ -------- ---------- ---- -------
     *  linux_test_1        001 offline  Linux         4      64
     *  Zephyr_test         002 never    Zephyr        1      64
     */
    rt_kprintf("%-*.s vm_idx status   OS type    vcpu  mem(M)\n", 
            maxlen, item_title);
    object_split(maxlen);
    rt_kprintf(" ------ -------- ---------- ---- --------\n");

    for (rt_size_t i = 0; i < MAX_VM_NUM; i++)
    {
        struct vm* vm = rt_hyp.vms[i];
        if (vm)
        {
            if (i == rt_hyp.curr_vm_idx)
                fmt = "\033[34m%-*.*s %6.3d %-8.s %-10s %4.1d %8d\n\033[0m";
            else
                fmt = "%-*.*s %6.3d %-8.s %-10s %4.1d %8d\n";
            
            rt_kprintf(fmt, maxlen, VM_NAME_SIZE, vm->name, vm->vm_idx,
                    vm_status_str[vm->status], os_type_str[vm->os->os_type],
                    vm->os->nr_vcpus, vm->mm->mem_size);
        }
    }
}
void print_el(void)
{
    rt_ubase_t currEL = rt_hw_get_current_el();
    rt_kprintf("Now is at EL%d\n", currEL);
}
MSH_CMD_EXPORT(print_el, print current EL);

MSH_CMD_EXPORT(list_os_img, list all os support);
MSH_CMD_EXPORT(list_vm, list all vm detail);
MSH_CMD_EXPORT(help_vm, print hypervisor help info);
MSH_CMD_EXPORT(create_vm, create new vm);
MSH_CMD_EXPORT(pick_vm, change current picking vm);
MSH_CMD_EXPORT(print_vm, for debug);
MSH_CMD_EXPORT(run_vm, run vm by index);
MSH_CMD_EXPORT(pause_vm, pause vm by index);
MSH_CMD_EXPORT(halt_vm, halt vm by index);
MSH_CMD_EXPORT(delete_vm, delete vm by index);
#endif /* RT_USING_FINSH */


#endif /* RT_HYPERVISOR */ 