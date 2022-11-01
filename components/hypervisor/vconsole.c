/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-03     Suqier       first version
 */

#include <rtthread.h>
#include <rtconfig.h>

// #if defined(RT_USING_DEVICE) && defined(RT_USING_CONSOLE)
#include "drv_uart.h"
#include "vconsole.h"
#include "hypervisor.h"
#include "vgic.h"
#include "hyp_debug.h"

extern struct hypervisor rt_hyp;

const static struct vdev_ops vc_ops = 
{
    .mmio = console_mmio_handler,
};

/*  
 * Get Guest OS device information from device tree.
 */
void get_vc_mmap_region(vdev_t vc, struct vm *vm)
{
    struct dev_info *dev    = &vm->info.devs[0];    /* @todo */

    vc->region->paddr_start = dev->pa_addr;
    vc->region->vaddr_start = dev->va_addr;
    vc->region->vaddr_end   = dev->va_addr + dev->size;
    vc->region->attr        = DEVICE_MEM;
}

static void vc_init(vdev_t vc, struct vm *vm)
{
    RT_ASSERT(vc && vm);
    
    /* memory leak */
    vc->dev = rt_device_find(RT_CONSOLE_DEVICE_NAME);
    vc->vm = vm;
    vc->mmap_num = 1;
    get_vc_mmap_region(vc, vm);
    rt_list_insert_after(&vm->dev_list, &vc->node);
    vc->is_open = RT_FALSE;
    vc->ops = &vc_ops;
}

rt_err_t vc_create(struct vm *vm)
{
    vdev_t vc = (vdev_t)rt_malloc(sizeof(struct vdev));

    if (vc == RT_NULL)
    {
        hyp_err("Allocate memory for vConsole failure");
        return -RT_ENOMEM;
    }

    vc_init(vc, vm);
    return RT_EOK;
}
 
static vdev_t get_curr_vc(void)
{
    if (rt_hyp.curr_vc != MAX_VM_NUM)
    {
        struct rt_list_node *pos;

        rt_list_for_each(pos, &rt_hyp.vms[rt_hyp.curr_vc].dev_list)
        {
            vdev_t vdev = rt_list_entry(pos, struct vdev, node);

            if (rt_strcmp(vdev->dev->parent.name, RT_CONSOLE_DEVICE_NAME) == 0)
                return vdev;
        }
    }

    return RT_NULL; /* Host console using UART */
}

/* Justice whether this VM take console now */
rt_bool_t is_vm_take_console(struct vm *vm)
{
    return (rt_hyp.curr_vc == vm->id);
}

void vc_detach(vdev_t vc)
{
    if (vc->dev != RT_NULL)     /* Host console NOT using UART */
    {
        /* close device */
        finsh_delete_device();  /* clear finsh device and reset flag */
        rt_device_close(vc->dev);
        vc->is_open = RT_FALSE;
        rt_hyp.curr_vc = MAX_VM_NUM;
        vgic_virq_umount(vc->vm, PL011_UART0_IRQNUM);
    }

    /* attach Host console */
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
    finsh_set_device(RT_CONSOLE_DEVICE_NAME);
    rt_kprintf("\n[Info] Quit %dth VM, back to Host\n", vc->vm->id);
}

void vc_attach(struct vm *vm)
{
    vdev_t vc = get_curr_vc();
    
    if (vc == RT_NULL)  /* Host console using UART */
    {
        /* Close Host console */
        hyp_info("Close Host console, Change to %dth VM", vm->id);
        rt_console_close_device();
    }
    else    /* It's someone VM vConsole using UART */
        vc_detach(vc);

    struct rt_list_node *pos;
    rt_list_for_each(pos, &vm->dev_list)
    {
        vdev_t vdev = rt_list_entry(pos, struct vdev, node);
        if (rt_strcmp(vdev->dev->parent.name, RT_CONSOLE_DEVICE_NAME) == 0)
        {
            vdev->is_open = RT_TRUE;
            rt_hyp.curr_vc = vm->id;

            /* Allocate device to VM then mount vIRQ */
            vgic_virq_mount(vm, PL011_UART0_IRQNUM);
            return;
        }
    }

    hyp_err("%dth VM: no device as %s", vm->id, RT_CONSOLE_DEVICE_NAME);
    return;
}

void console_mmio_handler(gp_regs_t regs, access_info_t acc)
{
    vdev_t vc = get_curr_vc();
    if (vc == RT_NULL)      /* Host console using UART0 */
        return;

    rt_uint64_t off = acc.addr - vc->region->vaddr_start;
    unsigned long long *val = regs_xn(regs, acc.srt);
    struct hw_uart_device *uart = (struct hw_uart_device *)vc->dev->user_data;

    if (acc.is_write)
    {
        if (*val == 0x2)    /* ctrl + B to quit VM*/
            vc_detach(vc);
        else
            writel(*val, (volatile void *)(uart->hw_base + off));
    }
    else    /* read */
        *val = readl((volatile void *)(uart->hw_base + off));
}

#if defined(RT_USING_FINSH)
rt_err_t attach_vm(void)
{
    rt_uint16_t vm_idx = rt_hyp.curr_vm;
    vc_attach(&rt_hyp.vms[vm_idx]);
    return RT_EOK;
}
MSH_CMD_EXPORT(attach_vm, attach VM console);
#endif  /* RT_USING_FINSH */

// #endif  /* RT_USING_DEVICE && RT_USING_CONSOLE */ 