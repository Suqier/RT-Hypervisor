/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-03     Suqier       first version
 */

#ifndef __VCONSOLE_H__
#define __VCONSOLE_H__

// #if defined(RT_USING_DEVICE) && defined(RT_USING_CONSOLE)
#include <rtdef.h>
#include <armv8.h>

#include "vdev.h"
#include "trap.h"

/* vc = vConsole */
rt_err_t vc_create(struct vm *vm);

rt_bool_t is_vm_take_console(struct vm * vm);
void vc_detach(vdev_t vc);
void vc_attach(struct vm *vm);
void console_mmio_handler(gp_regs_t regs, access_info_t acc);

#if defined(RT_USING_FINSH)
rt_err_t attach_vm(int argc, char **argv);
#endif  /* RT_USING_FINSH */ 
void detach_vm(void);


// #endif  /* RT_USING_DEVICE && RT_USING_CONSOLE */ 

#endif  /* __VCONSOLE_H__ */ 