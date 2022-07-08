/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-06-08     Suqier       first version
 */

// #if defined(RT_HYPERVISOR)
#include "trap.h"

static rt_uint64_t hvc_trap_call(rt_uint32_t fn, rt_uint64_t arg0, 
                             rt_uint64_t arg1, rt_uint64_t arg2)
{
    return arm_hvc_call(fn, arg0, arg1, arg2, 0, 0, 0, 0).x0;
}



/* 
 * for ESR_EC_UNKNOWN
 */
static rt_err_t ec_unknown_handler(regs *regs, rt_uint32_t esr, rt_uint32_t ec)
{
    return RT_EOK;
}RT_INSTALL_SYNC_DESC(ec_unknown, ec_unknown_handler, 4);

/*
 * for ESR_EC_WFX
 */
static rt_err_t ec_wfx_handler(regs *regs, rt_uint32_t esr, rt_uint32_t ec)
{
    /* vcpu turn to idle */
}RT_INSTALL_SYNC_DESC(ec_wfx, ec_wfx_handler, 4);

/*
 * for ESR_EC_HVC64
 */
static rt_err_t ec_hvc64_handler(regs *regs, rt_uint32_t esr, rt_uint32_t ec)
{
    
}RT_INSTALL_SYNC_DESC(ec_hvc64, ec_hvc64_handler, 0);

/* sync handler table */
static struct rt_sync_desc *low_sync_table[ESR_EC_MAX];

void rt_hw_handle_curr_sync(regs *regs)
{
    rt_kprintf("[Panic] From %s.\n", __func__);
    rt_hw_trap_error(regs);
}

void rt_hw_handle_low_sync(regs *regs)
{
    rt_uint32_t esr_val;
    struct rt_sync_desc *desc;

    GET_SYS_REG(ESR_EL2, esr_val);
    rt_uint32_t ec_type = GET_EC(esr_val);

    if (ec_type >= ESR_EC_MAX)
    {
        rt_kprintf("[Error] Unexpected ec_type %d.\n", ec_type);
        return;
    }

    desc = low_sync_table[ec_type];
    regs->pc += desc->pc_offset;
    desc->handler(regs, esr_val, ec_type);
}

/* handler sync exception from low level. */
void rt_hw_trap_sync(regs *regs)
{
    rt_uint64_t hcr_el2_val;
    GET_SYS_REG(HCR_EL2, hcr_el2_val);

    if (hcr_el2_val & HCR_TGE)  /* HCR_EL2.TGE = 1 -> Host App */
        rt_hw_handle_curr_sync(regs);
    else                        /* HCR_EL2.TGE = 0 -> Guest OS */
        rt_hw_handle_low_sync(regs);
}
// #endif  /* defined(RT_HYPERVISOR) */