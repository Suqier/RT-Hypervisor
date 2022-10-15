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
#include "os.h"

extern struct hypervisor rt_hyp;

#define MAX_OS_NUM  3
extern const struct os_desc os_img[MAX_OS_NUM];

const static struct vdev_ops vc_ops = 
{
    .mmio = console_mmio_handler,
};

/*  
 * Get Guest OS device information from device tree.
 */
void get_vc_mmap_region(vdev_t vc, struct vm *vm)
{
    struct dev_info *dev    = os_img[vm->os_idx].devs.dev;

    vc->region->paddr_start = dev->paddr;
    vc->region->vaddr_start = dev->vaddr;
    vc->region->vaddr_end   = dev->vaddr + dev->size;
    vc->region->attr        = DEVICE_MEM;
}

static void vc_init(vdev_t vc, struct vm *vm)
{
    RT_ASSERT(vc && vm);

    vc->dev = (rt_device_t)rt_malloc(sizeof(struct rt_device));
    
    if (vc->dev == RT_NULL)
    {
        rt_kprintf("[Error] Alloc memory for vConsole failure\n");
        free(vc);
        return;
    }
    else
    {
        rt_size_t n = rt_strlen(RT_CONSOLE_DEVICE_NAME);
        rt_strncpy(vc->dev->parent.name, RT_CONSOLE_DEVICE_NAME, n);
    }

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
        rt_kputs("[Error] Alloc memory for vConsole failure\n");
        return -RT_ENOMEM;
    }

    vc_init(vc, vm);
    return RT_EOK;
}
 
static vdev_t get_curr_vc(void)
{
    if (rt_hyp.curr_vc_idx != MAX_VM_NUM)
    {
        struct rt_list_node *pos;

        rt_list_for_each(pos, &rt_hyp.vms[rt_hyp.curr_vc_idx]->dev_list)
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
    return (rt_hyp.curr_vc_idx == vm->id);
}

void vc_detach(vdev_t vc)
{
    if (vc->dev != RT_NULL)     /* Host console NOT using UART */
    {
        /* close device */
        vc->dev = RT_NULL;
        vc->is_open = RT_FALSE;
        rt_hyp.curr_vc_idx = MAX_VM_NUM;
    }
}

void vc_attach(struct vm *vm)
{
    vdev_t vc = get_curr_vc();
    
    if (vc == RT_NULL)  /* Host console using UART */
    {
        /* Close Host console */
        rt_kprintf("[Info] Close Host console, Change to %dth VM\n", vm->id);
        rt_console_close_device();
    }
    else    /* It's someone VM vConsole using UART */
    {
        vc_detach(vc);
    }

    rt_device_t new_dev = rt_device_find(RT_CONSOLE_DEVICE_NAME);
    rt_device_open(new_dev, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_STREAM);
    
    struct rt_list_node *pos;
    rt_list_for_each(pos, &vm->dev_list)
    {
        vdev_t vdev = rt_list_entry(pos, struct vdev, node);
        if (rt_strcmp(vdev->dev->parent.name, RT_CONSOLE_DEVICE_NAME) == 0)
        {
            vdev->dev = new_dev;
            finsh_set_device(vdev->dev->parent.name);
            vdev->is_open = RT_TRUE;
            rt_hyp.curr_vc_idx = vm->id;
            return;
        }
    }
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
        writel(*val, (volatile void *)(uart->hw_base + off));
    else    /* read */
        *val = readl((volatile void *)(uart->hw_base + off));
}

#if defined(RT_USING_FINSH)
rt_err_t attach_vm(int argc, char **argv)
{
    /* 
     * "i" for VM idx. for example: -i vm_idx
     * attach VM before you check VM using list_vm
     */
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
            if(vm_idx < 0 || vm_idx >= MAX_VM_NUM)
            {
                rt_kprintf("[Error] %d-th VM: Out of scope\n", vm_idx);
                return -RT_EINVAL;
            }

            if (rt_hyp.vms[vm_idx] == RT_NULL)
            {
                rt_kprintf("[Error] %d-th VM: Not use\n", vm_idx);
                return -RT_EINVAL;
            }
            
            vc_attach(rt_hyp.vms[vm_idx]);
            break;
        case '?':
            rt_kprintf("[Error] %s: %s.\n", argv[0], options.errmsg);
            return -RT_ERROR;
        }
    }
    
    return RT_EOK;
}
MSH_CMD_EXPORT(attach_vm, attach VM console);
#endif  /* RT_USING_FINSH */ 

/* detach should be writen into shell */
void detach_vm(void)
{
    vdev_t vc = get_curr_vc();
    
    if (vc == RT_NULL)      /* Host console using UART0 */
        return;
    else
        vc_detach(get_curr_vc());
}

// #endif  /* RT_USING_DEVICE && RT_USING_CONSOLE */ 