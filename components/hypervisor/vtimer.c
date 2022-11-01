/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-06     Suqier       first version
 */

#include <cpuport.h>

#include "vgic.h"
#include "vtimer.h"
#include "trap.h"
#include "hyp_debug.h"

void vtimer_ctxt_init(vt_ctxt_t vtimer_ctxt, vcpu_t vcpu)
{
    RT_ASSERT(vtimer_ctxt);
    RT_ASSERT(vcpu);

    vtimer_ctxt->offset = 0UL;

    /* ptimer init */
    vtimer_ctxt->ptimer.vcpu   = vcpu;
    vtimer_ctxt->ptimer.vINIID = PTIMER_IRQ_NUM; 
    vtimer_ctxt->ptimer.ctl    = 0;
    vtimer_ctxt->ptimer.tval   = 0;
    vtimer_ctxt->ptimer.freq   = PTIMER_FREQ;

    /* vtimer init */
    vtimer_ctxt->vtimer.vcpu   = vcpu;
    vtimer_ctxt->vtimer.vINIID = VTIMER_IRQ_NUM;
    vtimer_ctxt->vtimer.ctl    = 0;
    vtimer_ctxt->vtimer.tval   = 0;
    vtimer_ctxt->vtimer.freq   = VTIMER_FREQ;
}

void ptimer_timeout_function(void *parameter)
{
    /* inject vIRQ function */
    hyp_debug("%s, %d", __FUNCTION__, __LINE__);

    vt_ctxt_t vtc = (vt_ctxt_t)parameter;

    vcpu_t vcpu = vtc->ptimer.vcpu;
    virq_t virq = &vcpu->vm->vgic.gicr[vcpu->id]->virqs[vtc->ptimer.vINIID];

    vcpu->vm->vgic.ops->inject(vcpu, virq);
}
 
void vtimer_timeout_function(void *parameter) {}

rt_err_t vtimer_ctxt_create(vcpu_t vcpu)
{
    vt_ctxt_t vt_ctxt = (vt_ctxt_t)rt_malloc(sizeof(struct vtimer_context));

    if (vt_ctxt == RT_NULL)
    {
        rt_free(vt_ctxt);
        hyp_err("Create vTimer failure for vcpu-%d", vcpu->id);
        return -RT_ENOMEM;
    }
    else
    {
        rt_timer_t ptimer = rt_timer_create("ptimer", ptimer_timeout_function, 
                            (void *)vt_ctxt, PTIMER_FREQ, 
                            RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
        rt_timer_t vtimer = rt_timer_create("ptimer", vtimer_timeout_function, 
                            (void *)vt_ctxt, PTIMER_FREQ, 
                            RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);

        if (!ptimer || !vtimer)
            return -RT_ENOMEM;
        
        vt_ctxt->ptimer.timer = ptimer;
        vt_ctxt->vtimer.timer = vtimer;
        
        vcpu->vtc = vt_ctxt;
        vtimer_ctxt_init(vt_ctxt, vcpu);
    }

    return RT_EOK;
}

static void vtimer_handler_cntp_ctl(rt_bool_t is_write, rt_uint64_t *reg_val)
{
    vcpu_t vcpu = get_curr_vcpu();
    struct vtimer *timer = &vcpu->vtc->ptimer;
    
    if (is_write)
    {
        /* write into it will change timer setting */
        rt_uint32_t v = (rt_uint32_t)*reg_val;

        v &= ~CNTP_CTL_ISTATUS_MASK;            
        if (v & CNTP_CTL_ENABLE_MASK)
			v |= timer->ctl & CNTP_CTL_ISTATUS_MASK;
		timer->ctl = v;

        if ((timer->ctl & CNTP_CTL_ENABLE_MASK) && !(timer->ctl & CNTP_CTL_IMASK_MASK))
            rt_timer_start(timer->timer);
		else
			rt_timer_stop(timer->timer);
    }
    else    /* read */
    {
        *reg_val = timer->ctl;
    }
}

static void vtimer_handler_cntp_tval(rt_bool_t is_write, rt_uint64_t *reg_val)
{
    vcpu_t vcpu = get_curr_vcpu();
    struct vtimer *timer = &vcpu->vtc->ptimer;

    if (is_write)
    {
        /* write into it will change timer setting */
        timer->ctl &= ~CNTP_CTL_ISTATUS_MASK;
        timer->tval = *reg_val;
        rt_timer_control(timer->timer, RT_TIMER_CTRL_SET_TIME, (void *)reg_val);
    }
    else    /* read */
    {
        *reg_val = timer->tval;
    }
}

void sysreg_vtimer_handler(struct rt_hw_exp_stack *regs, rt_uint64_t reg_name,
                        rt_bool_t is_write, rt_uint32_t srt)
{
    rt_uint64_t *reg_val = (rt_uint64_t *)(is_write ? regs_xn(regs, srt) : 0);

    switch (reg_name)
    {
    case ESR_SYSREG_CNTP_CTL_EL0:
        vtimer_handler_cntp_ctl(is_write, reg_val);
        break;
    case ESR_SYSREG_CNTP_TVAL_EL0:
        vtimer_handler_cntp_tval(is_write, reg_val);
        break;
    
    default:
        break;
    }
}

/* 
 * hook for switch 
 * 
 * Main task for vtimer is to save/restore vtimer_context.
 * In vtimer_context, the value of vcpu/timer/vINTID/freq will not change,
 * So, we just need to save/restore ctl/tval value.
 */
void hook_vtimer_context_save(vt_ctxt_t vtimer_ctxt, vcpu_t vcpu)
{
    struct vtimer *timer = &vtimer_ctxt->ptimer;

    /* 
     * save vtimer context(only ptimer now) for exit Guest OS, 
     * record current wall time(sys tick).
     */
    GET_SYS_REG(CNTP_CTL_EL0, timer->ctl);
    GET_SYS_REG(CNTP_TVAL_EL0, timer->tval);
	SET_SYS_REG(CNTP_CTL_EL0, 0);
    __ISB();

    vtimer_ctxt->offset = rt_hw_get_cntpct_val();
}

void hook_vtimer_context_restore(vt_ctxt_t vtimer_ctxt, vcpu_t vcpu)
{
    struct vtimer *timer = &vtimer_ctxt->ptimer;

    /* 
     * restore vtimer context(only ptimer now) for enter Guest OS,
     * using offset to amend VM timer.
     */
    rt_uint64_t now = rt_hw_get_cntpct_val();
    vtimer_ctxt->offset = now - vtimer_ctxt->offset;
    timer->tval += vtimer_ctxt->offset;

    SET_SYS_REG(CNTP_CTL_EL0, timer->ctl);
    SET_SYS_REG(CNTP_TVAL_EL0, timer->tval);
    __ISB();
}
