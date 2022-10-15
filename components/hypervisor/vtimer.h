/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-06     Suqier       first version
 */

#ifndef __VTIMER_H__
#define __VTIMER_H__

#include <rtdef.h>
#include <gtimer.h>

#include "vm.h"

/* set IQR number and default timer frequency in device tree */
#define PTIMER_PRIORITY	
#define PTIMER_IRQ_NUM  30
#define VTIMER_IRQ_NUM  27
#define PTIMER_FREQ		rt_hw_get_gtimer_frq()
#define VTIMER_FREQ		rt_hw_get_gtimer_frq()

#define CNTP_CTL_ISTATUS_SHIFT	(2)	/* Access to this field is RO. */
#define CNTP_CTL_ISTATUS_MASK	(1 << CNTP_CTL_ISTATUS_SHIFT)
#define CNTP_CTL_IMASK_SHIFT	(1)
#define CNTP_CTL_IMASK_MASK		(1 << CNTP_CTL_IMASK_SHIFT)
#define CNTP_CTL_ENABLE_SHIFT	(0)
#define CNTP_CTL_ENABLE_MASK	(1 << CNTP_CTL_ENABLE_SHIFT)

struct vtimer
{
    vcpu_t      vcpu;
    rt_timer_t  timer;
    rt_uint16_t vINIID;
    
    rt_uint32_t ctl;
	rt_uint32_t tval;
	rt_uint64_t freq;
};

struct vtimer_context {
	struct vtimer ptimer;
	struct vtimer vtimer;
	rt_uint64_t   offset;
};
typedef struct vtimer_context *vt_ctxt_t;

/* emulate EL1 pTimer and vTimer, then inject vIRQ */
void vtimer_ctxt_init(vt_ctxt_t vtimer_ctxt, vcpu_t vcpu);
rt_err_t vtimer_ctxt_create(vcpu_t vcpu);
void sysreg_vtimer_handler(struct rt_hw_exp_stack *regs, rt_uint64_t reg_name,
                        rt_bool_t is_write, rt_uint32_t srt);

/* hook function for context switch */
void hook_vtimer_context_save(vt_ctxt_t vtimer_ctxt, vcpu_t vcpu);
void hook_vtimer_context_restore(vt_ctxt_t vtimer_ctxt, vcpu_t vcpu);

/* emulate counter */


#endif  /* __VTIMER_H__ */