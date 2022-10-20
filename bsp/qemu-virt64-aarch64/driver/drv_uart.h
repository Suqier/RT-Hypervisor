/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2017-5-30      Bernard      the first version
 */

#ifndef DRV_UART_H__
#define DRV_UART_H__

#include <rtdef.h>

struct hw_uart_device
{
    rt_ubase_t hw_base;
    rt_uint32_t irqno;
};

unsigned int readl(volatile void *addr);
void writel(unsigned int v, volatile void *addr);

int rt_hw_uart_init(void);

#endif /* DRV_UART_H__ */
