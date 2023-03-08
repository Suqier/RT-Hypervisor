/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2011-12-20     GuEe-GUI     first version
 */

#include <rtthread.h>
#include <rthw.h>
#include <gtimer.h>
#include <cpuport.h>

#define EL1_PHY_TIMER_IRQ_NUM 30
#define EL2_PHY_TIMER_IRQ_NUM 26

static volatile rt_uint64_t timer_step;

static void rt_hw_timer_isr(int vector, void *parameter)
{
    rt_hw_set_gtimer_val(timer_step);
    rt_tick_increase();
}

void rt_hw_gtimer_init(void)
{
#ifdef RT_HYPERVISOR
    rt_hw_interrupt_install(EL2_PHY_TIMER_IRQ_NUM, rt_hw_timer_isr, RT_NULL, "tick");
#else
    rt_hw_interrupt_install(EL1_PHY_TIMER_IRQ_NUM, rt_hw_timer_isr, RT_NULL, "tick");
#endif
    __ISB();
    timer_step = rt_hw_get_gtimer_frq();
    __DSB();
    timer_step /= RT_TICK_PER_SECOND;
    rt_hw_gtimer_local_enable();
}

void rt_hw_gtimer_local_enable(void)
{
    rt_hw_gtimer_disable();
    rt_hw_set_gtimer_val(timer_step);

#ifdef RT_HYPERVISOR
    rt_hw_interrupt_umask(EL2_PHY_TIMER_IRQ_NUM);
#else
    rt_hw_interrupt_umask(EL1_PHY_TIMER_IRQ_NUM);
#endif
    
    rt_hw_gtimer_enable();
}

void rt_hw_gtimer_local_disable(void)
{
    rt_hw_gtimer_disable();

#ifdef RT_HYPERVISOR
    rt_hw_interrupt_mask(EL2_PHY_TIMER_IRQ_NUM);
#else
    rt_hw_interrupt_mask(EL1_PHY_TIMER_IRQ_NUM);
#endif
}
