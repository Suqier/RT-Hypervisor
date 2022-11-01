/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-06-08     Suqier       first version
 */

#include <bitmap.h>
#include <os.h>
#include <vm.h>
#include <vdev.h>
#include <vtimer.h>

#include "virt_arch.h"
#include "trap.h"
#include "hyp_debug.h"

extern void rt_hw_trap_error(struct rt_hw_exp_stack *regs);

/* for ESR_EC_UNKNOWN */
void ec_unknown_handler(struct rt_hw_exp_stack *regs, rt_uint32_t esr)
{
    hyp_debug("ec_unknown_handler tid->name = %s", rt_thread_self()->name);
    while (1) {}
}RT_INSTALL_SYNC_DESC(ec_unknown, ec_unknown_handler, 4);

/* for ESR_EC_WFX */
void ec_wfx_handler(struct rt_hw_exp_stack *regs, rt_uint32_t esr)
{
    vcpu_suspend(get_curr_vcpu());
    hyp_debug("tid->name = %s, curr_el = %d", 
        rt_thread_self()->name, rt_hw_get_current_el());
}RT_INSTALL_SYNC_DESC(ec_wfx, ec_wfx_handler, 4);

/* for ESR_EC_HVC64 */
void ec_hvc64_handler(struct rt_hw_exp_stack *regs, rt_uint32_t esr)
{
    hyp_debug("%s, %d", __FUNCTION__, __LINE__);
    while (1) {}
}RT_INSTALL_SYNC_DESC(ec_hvc64, ec_hvc64_handler, 0);

/* 
 * for ESR_EC_SYS64
 * - access EL1 timer register 
 */
void ec_sys64_handler(struct rt_hw_exp_stack *regs, rt_uint32_t esr)
{
    /* Just emulate a little part of system registers */
    rt_bool_t is_write;
    rt_uint32_t val, srt;
    rt_uint64_t reg_name;

    val = esr & ESR_DIRECTION_MASK;
    is_write = (val == ESR_DIRECTION_READ) ? RT_FALSE : RT_TRUE;
    srt = (esr & ESR_RT_MASK) >> ESR_RT_SHIFT;
    reg_name = esr & ESR_SYSREG_MASK;

    switch (reg_name)
    {
    /* timer sysreg handler */
    case ESR_SYSREG_CNTPCT_EL0:
    case ESR_SYSREG_CNTP_TVAL_EL0:
    case ESR_SYSREG_CNTP_CTL_EL0:
    case ESR_SYSREG_CNTP_CVAL_EL0:
        sysreg_vtimer_handler(regs, reg_name, is_write, srt);
        break;
    
    default:
        hyp_err("Unsupported system register access");
        break;
    }
}RT_INSTALL_SYNC_DESC(ec_sys64, ec_sys64_handler, 4);

/* for ESR_EC_IABT_LOW */
void ec_iabt_low_handler(gp_regs_t regs, rt_uint32_t esr)
{
    hyp_debug("ec_iabt_low_handler tid->name = %s", rt_thread_self()->name);
    while(1) {}
}RT_INSTALL_SYNC_DESC(ec_iabt_low, ec_iabt_low_handler, 0);

/* for ESR_EC_DABT_LOW */
rt_inline rt_bool_t is_fa_in(rt_uint64_t fa, rt_uint64_t base, rt_uint64_t size)
{
    return ((fa >= base) && (fa < base + size));
}

rt_inline rt_bool_t is_access_vgicd(rt_uint64_t fa)
{
    struct vm_info *info = &get_curr_vm()->info;
    rt_uint64_t d = info->gicd_addr;
    return is_fa_in(fa, d, VGIC_GICD_SIZE);
}

rt_inline rt_bool_t is_access_vgicr(rt_uint64_t fa)
{
    struct vm_info *info = &get_curr_vm()->info;
    rt_uint64_t r = info->gicr_addr;
    return is_fa_in(fa, r, VGIC_GICR_SIZE);
}

static void vdev_mmio_handler(gp_regs_t regs, access_info_t acc)
{
    vm_t vm = get_curr_vm();

    /* find base address in virtual device */
    struct rt_list_node *pos;
    rt_list_for_each(pos, &vm->dev_list)
    {
        vdev_t vdev = rt_list_entry(pos, struct vdev, node);
        
        if (vdev->is_open)
        {
            for(rt_uint8_t i = 0; i < vdev->mmap_num; i++)
            {
                if ((acc.addr >= vdev->region->vaddr_start)
                &&  (acc.addr <  vdev->region->vaddr_end))
                {
                    if (vdev->ops->mmio)
                        vdev->ops->mmio(regs, acc);

                    return;
                }
            }
        }
        else
            return;
    }
}

void ec_dabt_low_handler(gp_regs_t regs, rt_uint32_t esr)
{
    rt_uint8_t dfsc = esr & FSC_TYPE_MASK;
    rt_bool_t is_isv_valid = !!bit_get(esr, ESR_ISV_SHIFT);
    if ((dfsc == FSC_TRANS || dfsc == FSC_PERM) && is_isv_valid)
    {
        /* Parse ESR_EL2 register */
        rt_bool_t is_write, far_valid;
        rt_uint32_t val, srt;
        rt_uint64_t fault_addr;

        srt = ESR_GET_SRT(esr);
        val = ESR_GET_WNR(esr);
        is_write = (val == ESR_WNR_WRITE) ? RT_TRUE : RT_FALSE;
        
        val = ESR_GET_FNV(esr);
        far_valid = (val == ESR_FNV_VALID) ? RT_TRUE : RT_FALSE;
        if (far_valid)
        {
            GET_SYS_REG(FAR_EL2, fault_addr);
        }
        else
            GET_SYS_REG(HPFAR_EL2, fault_addr);

        access_info_t acc = 
        {
            .addr     = fault_addr,
            .srt      = srt,
            .is_write = is_write,
        };

        /* MMIO handler */
        if(is_access_vgicd(acc.addr))
            get_curr_vm()->vgic->ops->emulate(regs, acc, RT_TRUE);
        else if (is_access_vgicr(acc.addr))
            get_curr_vm()->vgic->ops->emulate(regs, acc, RT_FALSE);
        else
            vdev_mmio_handler(regs, acc);
    }
    else
    {
        rt_uint64_t fault_addr;
        GET_SYS_REG(FAR_EL2, fault_addr);
        hyp_err("  FAR_EL2 = 0x%016x", fault_addr);
        GET_SYS_REG(HPFAR_EL2, fault_addr);
        hyp_err("HPFAR_EL2 = 0x%016x", fault_addr);

        hyp_err("Unsupported Type or ISV invalid, esr = 0x%08x", esr);
        vcpu_fault(get_curr_vcpu());
    }
}RT_INSTALL_SYNC_DESC(ec_dabt_low, ec_dabt_low_handler, 4);

/* Sync Handler Table */
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
    hyp_err("%s, %d", __FUNCTION__, __LINE__);
    rt_hw_trap_error(regs);
}

void rt_hw_handle_low_sync(struct rt_hw_exp_stack *regs)
{
    rt_uint64_t esr_val;

    GET_SYS_REG(ESR_EL2, esr_val);
    rt_uint32_t ec_type = GET_EC(esr_val);

    if (ec_type >= ESR_EC_MAX)
    {
        hyp_err("Unexpected ec_type: %d", ec_type);
        return;
    }

    struct rt_sync_desc *desc = low_sync_table[ec_type];
    if (desc)
    {
        regs->pc += desc->pc_offset;
        desc->handler(regs, esr_val);
    }
}

/* handler sync exception from low level. */
void rt_hw_trap_sync(struct rt_hw_exp_stack *regs)
{
    rt_uint64_t hcr_el2_val;
    GET_SYS_REG(HCR_EL2, hcr_el2_val);

    if (hcr_el2_val & HCR_TGE)  /* HCR_EL2.TGE = 1 -> Host App */
        rt_hw_handle_curr_sync(regs);
    else                        /* HCR_EL2.TGE = 0 -> Guest OS */
        rt_hw_handle_low_sync(regs);
}