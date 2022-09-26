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
#include <vdev.h>

#include "virt_arch.h"
#include "trap.h"

extern void rt_hw_trap_error(struct rt_hw_exp_stack *regs);

/*
 * According to fault_addr to find which device is mmaped
 * and return it's base address. Maybe vGIC.
 */
static void vdev_mmio_handler(struct rt_hw_exp_stack *regs, rt_uint64_t fa, 
                            rt_bool_t is_write, rt_uint32_t srt)
{
    vm_t vm = get_curr_vm();
    const struct os_desc *os = vm->os;

    /* find base address in virtual GIC */
    if ((fa >= os->arch.vgic.gicd_addr) 
     && (fa <  os->arch.vgic.gicd_addr + VGIC_GICD_SIZE))
    {
        if (is_write)
            vm->vgic->ops->write(regs, fa, srt, os->arch.vgic.gicd_addr);
        else
            vm->vgic->ops->read(regs, fa, srt, os->arch.vgic.gicd_addr);

        goto _mmio_over;
    }

    if ((fa >= os->arch.vgic.gicr_addr) 
     && (fa <  os->arch.vgic.gicr_addr + VGIC_GICR_SIZE))
    {
        if (is_write)
            vm->vgic->ops->write(regs, fa, srt, os->arch.vgic.gicr_addr);
        else
            vm->vgic->ops->read(regs, fa, srt, os->arch.vgic.gicr_addr);

        goto _mmio_over;
    }

    /* find base address in virtual device */
    struct rt_list_node *pos;
    rt_list_for_each(pos, &vm->dev_list)
    {
        vdev_t vdev = rt_list_entry(pos, struct vdev, node);
        for(rt_uint8_t i = 0; i < vdev->mmap_num; i++)
        {
            if ((fa >= vdev->region->vaddr_start) && (fa < vdev->region->vaddr_end))
            {
                // vdev->dev->ops->read();
                rt_kprintf("[Error] Unsupported Operations.\n");
                
                goto _mmio_over;
            }
        }
    }

_mmio_over:
    return;
}

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
    rt_uint8_t dfsc = esr & FSC_TYPE_MASK;
    rt_uint32_t isv_valid = esr & (1 << ESR_ISV_SHIFT);
    if ((dfsc == FSC_TRANS || dfsc == FSC_PERM) && (isv_valid == ESR_ISV_VALID))
    {
        /* Parse ESR_EL2 register */
        rt_bool_t is_write, far_valid;
        rt_uint32_t val, srt;
        rt_uint64_t fault_addr;

        srt = ESR_GET_SRT(esr);
        val = ESR_GET_FNV(esr);
        far_valid = (val == ESR_FNV_VALID) ? RT_TRUE : RT_FALSE;
        val = ESR_GET_WNR(esr);
        is_write = (val == ESR_WNR_WRITE) ? RT_TRUE : RT_FALSE;

        if (far_valid)
        {
            GET_SYS_REG(FAR_EL2, fault_addr);
        }
        else
            GET_SYS_REG(HPFAR_EL2, fault_addr);

        /* MMIO handler */
        vdev_mmio_handler(regs, fault_addr, is_write, srt);
    }
    else
    {
        rt_kprintf("[Error] Unsupported Data Abort Type or ISV invalid.\n");
        vcpu_fault(get_curr_vcpu());
    }
}RT_INSTALL_SYNC_DESC(ec_dabt_low, ec_dabt_low_handler, 4);

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
    rt_uint64_t reg_val, esr_val;
    struct rt_sync_desc *desc;

    GET_SYS_REG(ESR_EL2, esr_val);
    rt_uint32_t ec_type = GET_EC(esr_val);

    if (ec_type >= ESR_EC_MAX)
    {
        rt_kprintf("[Error] Unexpected ec_type:%d.\n", ec_type);
        return;
    }

    rt_kprintf("[Info]   ESR_EL2 = 0x%08x\n", esr_val);
    GET_SYS_REG(HCR_EL2, reg_val);
    rt_kprintf("[Info]   HCR_EL2 = 0x%08x\n", reg_val);
    GET_SYS_REG(FAR_EL2, reg_val);
    rt_kprintf("[Info]   FAR_EL2 = 0x%08x\n", reg_val);
    GET_SYS_REG(HPFAR_EL2, reg_val);
    rt_kprintf("[Info] HPFAR_EL2 = 0x%08x\n", reg_val);
    GET_SYS_REG(VTTBR_EL2, reg_val);
    rt_kprintf("[Info] VTTBR_EL2 = 0x%08x\n", reg_val);
    GET_SYS_REG(SPSR_EL2, reg_val);
    rt_kprintf("[Info]  SPSR_EL2 = 0x%08x\n", reg_val);    
    GET_SYS_REG(ELR_EL2, reg_val);
    rt_kprintf("[Info]   ELR_EL2 = 0x%08x\n", reg_val);

    desc = low_sync_table[ec_type];
    regs->pc += desc->pc_offset;
    desc->handler(regs, esr_val);
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