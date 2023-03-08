/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-06     Suqier       first version
 */

#include <stdio.h>
#include <string.h>
#include <lib_helpers.h>
#include <cpuport.h>

#include "vgic.h"
#include "vtimer.h"
#include "trap.h"
#include "hyp_debug.h"
#include "vm.h"

void vtimer_ctxt_init(vt_ctxt_t virt_timer_ctxt, struct vcpu *vcpu)
{
    RT_ASSERT(vcpu && virt_timer_ctxt);
    virt_timer_ctxt->offset = 0UL;

    /* phy_timer init */
    virt_timer_ctxt->phy_timer.vcpu    = vcpu;
    virt_timer_ctxt->phy_timer.vINIID  = PTIMER_IRQ_NUM; 
    virt_timer_ctxt->phy_timer.ctl     = 0;
    virt_timer_ctxt->phy_timer.cval    = 0;
    virt_timer_ctxt->phy_timer.freq    = PTIMER_FREQ;

    /* virt_timer init */
    virt_timer_ctxt->virt_timer.vcpu   = vcpu;
    virt_timer_ctxt->virt_timer.vINIID = VTIMER_IRQ_NUM;
    virt_timer_ctxt->virt_timer.ctl    = 0;
    virt_timer_ctxt->virt_timer.cval   = 0;
    virt_timer_ctxt->virt_timer.freq   = VTIMER_FREQ;
}

void phy_timer_timeout_func(void *parameter)
{
    /* inject vIRQ function */
    vt_ctxt_t vtc = (vt_ctxt_t)parameter;
    vtc->phy_timer.ctl |= CNTP_CTL_ISTATUS_MASK;
    vtc->phy_timer.cval = 0UL;

    if (!(vtc->phy_timer.ctl & CNTP_CTL_IMASK_MASK))
    {
        struct vcpu *vcpu = vtc->phy_timer.vcpu;
        virq_t virq = &vcpu->vm->vgic.gicr[vcpu->id].virqs[vtc->phy_timer.vINIID];
        vcpu->vm->vgic.ops->inject(vcpu, virq);
    }
}
 
void virt_timer_timeout_func(void *parameter) 
{
    hyp_debug("%s, %d", __FUNCTION__, __LINE__);
}

void vtimer_init(struct vcpu *vcpu)
{
    vtimer_ctxt_init(&vcpu->vtc, vcpu);

    char name[VM_NAME_SIZE];
    rt_memset(name, 0, VM_NAME_SIZE);
	sprintf(name, "m%d_c%d_phy" , vcpu->vm->id, vcpu->id);
    rt_timer_init(&(vcpu->vtc.phy_timer.timer),  name, phy_timer_timeout_func,
                (void *)&(vcpu->vtc), PTIMER_FREQ / RT_TICK_PER_SECOND / 1000, 
                RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);

	sprintf(name, "m%d_c%d_virt", vcpu->vm->id, vcpu->id);
    rt_timer_init(&(vcpu->vtc.virt_timer.timer), name, virt_timer_timeout_func,
                (void *)&(vcpu->vtc), PTIMER_FREQ / RT_TICK_PER_SECOND / 1000, 
                RT_TIMER_FLAG_ONE_SHOT | RT_TIMER_FLAG_SOFT_TIMER);
}

/*
 * Register handler
 */
static void vtimer_handler_cntp_ctl(rt_bool_t is_write, rt_uint64_t *reg_val)
{
    struct vcpu   *vcpu  = get_curr_vcpu();
    struct vtimer *timer = &vcpu->vtc.phy_timer;
    
    if (is_write)
    {
        /* write into it will change timer setting */
        rt_uint32_t v = (rt_uint32_t)*reg_val;

        v &= ~CNTP_CTL_ISTATUS_MASK;            
        if (v & CNTP_CTL_ENABLE_MASK)
			v |= timer->ctl & CNTP_CTL_ISTATUS_MASK;
		timer->ctl = v;

        if ((timer->ctl & CNTP_CTL_ENABLE_MASK) 
        && !(timer->ctl & CNTP_CTL_IMASK_MASK))
            rt_timer_start(&timer->timer);
		else
			rt_timer_stop(&timer->timer);
    }
    else    /* read */
    {
        *reg_val = timer->ctl;
    }
}

static void vtimer_handler_cntp_tval(rt_bool_t is_write, rt_uint64_t *reg_val)
{
    struct vcpu   *vcpu  = get_curr_vcpu();
    struct vtimer *timer = &vcpu->vtc.phy_timer;

    if (is_write)
    {
        timer->cval = rt_hw_get_cntpct_val() + *reg_val;

        if (timer->ctl & CNTP_CTL_ENABLE_MASK)
        {
            timer->ctl &= ~CNTP_CTL_ISTATUS_MASK;
            rt_timer_control(&timer->timer, 
                        RT_TIMER_CTRL_SET_TIME, (void *)reg_val);
        }
    }
    else    /* read */
    {
        rt_uint64_t now  = rt_hw_get_cntpct_val() - vcpu->vtc.offset;
        rt_uint64_t tval = (timer->cval - vcpu->vtc.offset - now) & 0xFFFFFFFF;
        *reg_val = tval;
    }
}

static void vtimer_handler_cntp_cval(rt_bool_t is_write, rt_uint64_t *reg_val)
{
    struct vcpu   *vcpu  = get_curr_vcpu();
    struct vtimer *timer = &vcpu->vtc.phy_timer;

    if (is_write)
    {
        /* write into it will change timer setting */
        timer->cval = *reg_val + vcpu->vtc.offset;
        if (timer->ctl & CNTP_CTL_ENABLE_MASK)
        {
            timer->ctl &= ~CNTP_CTL_ISTATUS_MASK;
            rt_timer_control(&timer->timer, 
                        RT_TIMER_CTRL_SET_TIME, (void *)timer->cval);
        }
    }
    else    /* read */
    {
        *reg_val = timer->cval - vcpu->vtc.offset;
    }
}

void sysreg_vtimer_handler(struct rt_hw_exp_stack *regs, access_info_t acc)
{
    rt_uint64_t *reg_val =
                (rt_uint64_t *)(acc.is_write ? regs_xn(regs, acc.srt) : 0);

    switch (acc.addr)
    {
        /* Guest OS access physical timer related registers */
    case ESR_SYSREG_CNTP_CTL_EL0:
        vtimer_handler_cntp_ctl(acc.is_write, reg_val);
        break;
    case ESR_SYSREG_CNTP_TVAL_EL0:
        vtimer_handler_cntp_tval(acc.is_write, reg_val);
        break;
    case ESR_SYSREG_CNTP_CVAL_EL0:
        vtimer_handler_cntp_cval(acc.is_write, reg_val);
        break;

    default:
        break;
    }
}

void vtimer_free(struct vcpu *vcpu)
{
    RT_ASSERT(vcpu);
    vt_ctxt_t virt_timer_ctxt = &vcpu->vtc;
    rt_timer_stop(&virt_timer_ctxt->phy_timer.timer);
    rt_timer_stop(&virt_timer_ctxt->virt_timer.timer);
}

/* 
 * hook for switch 
 * 
 * Main task for virt_timer is to save/restore virt_timer_context.
 * In virt_timer_context, the value of vcpu/timer/vINTID/freq will not change,
 * So, we just need to save/restore ctl/tval value.
 */
void hook_vtimer_context_save(vt_ctxt_t virt_timer_ctxt, struct vcpu *vcpu)
{
    struct vtimer *timer = &virt_timer_ctxt->virt_timer;

    GET_SYS_REG(CNTV_CTL_EL0,  timer->ctl);
    GET_SYS_REG(CNTV_CVAL_EL0, timer->cval);
	SET_SYS_REG(CNTV_CTL_EL0,  0);
    __ISB();

	if ((timer->ctl & CNTP_CTL_ENABLE_MASK) && !(timer->ctl & CNTP_CTL_IMASK_MASK)) 
    {
        rt_timer_control(&timer->timer, 
                        RT_TIMER_CTRL_SET_TIME, 
                        (void *)(timer->cval + virt_timer_ctxt->offset));
	}
}

void hook_vtimer_context_restore(vt_ctxt_t virt_timer_ctxt, struct vcpu *vcpu)
{
    struct vtimer *timer = &virt_timer_ctxt->virt_timer;
    rt_uint64_t   offset =  virt_timer_ctxt->offset;
    
    rt_timer_stop(&timer->timer);

	SET_SYS_REG(CNTVOFF_EL2,   offset);
    SET_SYS_REG(CNTV_CTL_EL0,  timer->ctl);
    SET_SYS_REG(CNTV_CVAL_EL0, timer->cval);
    __ISB();
}
