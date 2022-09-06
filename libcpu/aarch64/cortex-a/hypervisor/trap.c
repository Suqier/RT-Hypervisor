/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-06-08     Suqier       first version
 */

#include <vm.h>
#include "trap.h"

extern void rt_hw_trap_error(struct rt_hw_exp_stack *regs);

static rt_uint64_t hvc_trap_call(rt_uint32_t fn, rt_uint64_t arg0, 
                             rt_uint64_t arg1, rt_uint64_t arg2)
{
    return 0;
    return arm_hvc_call(fn, arg0, arg1, arg2, 0, 0, 0, 0).x0;
}

/* for ESR_EC_UNKNOWN */
void ec_unknown_handler(struct rt_hw_exp_stack *regs, rt_uint32_t esr)
{
    rt_kprintf("[Debug] %s, %d\n", __FUNCTION__, __LINE__);
}RT_INSTALL_SYNC_DESC(ec_unknown, ec_unknown_handler, 4);

/* for ESR_EC_WFX */
void ec_wfx_handler(struct rt_hw_exp_stack *regs, rt_uint32_t esr)
{
    /* vcpu turn to idle */
    rt_kprintf("[Debug] %s, %d\n", __FUNCTION__, __LINE__);
}RT_INSTALL_SYNC_DESC(ec_wfx, ec_wfx_handler, 4);

/* for ESR_EC_HVC64 */
void ec_hvc64_handler(struct rt_hw_exp_stack *regs, rt_uint32_t esr)
{
    rt_kprintf("[Debug] %s, %d\n", __FUNCTION__, __LINE__);
}RT_INSTALL_SYNC_DESC(ec_hvc64, ec_hvc64_handler, 0);

/* for ESR_EC_SYS64 */
void ec_sys64_handler(struct rt_hw_exp_stack *regs, rt_uint32_t esr)
{
    rt_kprintf("[Debug] %s, %d\n", __FUNCTION__, __LINE__);
}RT_INSTALL_SYNC_DESC(ec_sys64, ec_sys64_handler, 4);

/* for ESR_EC_IABT_LOW */
void ec_iabt_low_handler(struct rt_hw_exp_stack *regs, rt_uint32_t esr)
{
    /* 
     * Re-try for ARM64_ERRATUM_1530923 
     * We think there is no instruction abort normally. If occurs, it means that
     * SPECULATIVE_AT happened and allocate error TLB, than re-write PC and SPSR. 
     */
    rt_kprintf("[Info] %s, %d. Re-try for ARM64_ERRATUM_1530923.\n", 
                __FUNCTION__, __LINE__);
    while (1) {}
    
}RT_INSTALL_SYNC_DESC(ec_iabt_low, ec_iabt_low_handler, 0);

/* for ESR_EC_DABT_LOW */
void ec_dabt_low_handler(struct rt_hw_exp_stack *regs, rt_uint32_t esr)
{
    rt_kprintf("[Info] %s, %d.\n", __FUNCTION__, __LINE__);
    rt_uint8_t dfsc = esr & ESR_FSC_TYPE;
    if (dfsc == ESR_FSC_TRANS || dfsc == ESR_FSC_PERM)
    {
        /* MMIO */
        
    }
    else
    {
        rt_kprintf("[Error] Unsupported Data Abort Type.\n");
        vcpu_fault(get_curr_vcpu());
    }
}RT_INSTALL_SYNC_DESC(ec_dabt_low, ec_dabt_low_handler, 0);

/* sync handler table */
static struct rt_sync_desc *low_sync_table[] = 
{
    [0 ... ESR_EC_MAX] = &__sync_ec_unknown,
    [ESR_EC_UNKNOWN]   = &__sync_ec_unknown,
    [ESR_EC_WFX]       = &__sync_ec_wfx,
    [ESR_EC_HVC64]     = &__sync_ec_hvc64,
    [ESR_EC_SYS64]     = &__sync_ec_sys64,
    [ESR_EC_IABT_LOW]  = &__sync_ec_iabt_low,
    [ESR_EC_DABT_LOW]  = &__sync_ec_dabt_low,
};

void rt_hw_handle_curr_sync(struct rt_hw_exp_stack *regs)
{
    rt_kprintf("[Panic] %s, %d\n", __FUNCTION__, __LINE__);
    rt_hw_trap_error(regs);
}

void rt_hw_handle_low_sync(struct rt_hw_exp_stack *regs)
{
    rt_kprintf("[Debug] %s, %d\n", __FUNCTION__, __LINE__);
    rt_uint64_t esr_val;
    struct rt_sync_desc *desc;

    GET_SYS_REG(ESR_EL2, esr_val);
    rt_uint32_t ec_type = GET_EC(esr_val);

    if (ec_type >= ESR_EC_MAX)
    {
        rt_kprintf("[Error] Unexpected ec_type:%d.\n", ec_type);
        return;
    }

    rt_kprintf("[Info]   ESR_EL2 = 0x%08x\n", esr_val);
    GET_SYS_REG(HCR_EL2, esr_val);
    rt_kprintf("[Info]   HCR_EL2 = 0x%08x\n", esr_val);
    GET_SYS_REG(FAR_EL2, esr_val);
    rt_kprintf("[Info]   FAR_EL2 = 0x%08x\n", esr_val);
    GET_SYS_REG(VTTBR_EL2, esr_val);
    rt_kprintf("[Info] VTTBR_EL2 = 0x%08x\n", esr_val);
    GET_SYS_REG(SPSR_EL2, esr_val);
    rt_kprintf("[Info]  SPSR_EL2 = 0x%08x\n", esr_val);    
    GET_SYS_REG(ELR_EL2, esr_val);
    rt_kprintf("[Info]   ELR_EL2 = 0x%08x\n", esr_val);

    desc = low_sync_table[ec_type];
    regs->pc += desc->pc_offset;
    desc->handler(regs, esr_val);
}

/* handler sync exception from low level. */
void rt_hw_trap_sync(struct rt_hw_exp_stack *regs)
{
    rt_kprintf("[Debug] %s, %d\n", __FUNCTION__, __LINE__);   
    rt_uint64_t hcr_el2_val;
    GET_SYS_REG(HCR_EL2, hcr_el2_val);

    if (hcr_el2_val & HCR_TGE)  /* HCR_EL2.TGE = 1 -> Host App */
        rt_hw_handle_curr_sync(regs);
    else                        /* HCR_EL2.TGE = 0 -> Guest OS */
        rt_hw_handle_low_sync(regs);
}