/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-21     Suqier       first version
 */

#ifndef __VIRT_H__
#define __VIRT_H__

#include <armv8.h>
#include <lib_helpers.h>

#include "vgic.h"

#ifndef RT_USING_SMP
#define RT_CPUS_NR      1
extern int rt_hw_cpu_id(void);
#else
extern rt_uint64_t rt_cpu_mpidr_early[];
#endif /* RT_USING_SMP */

#ifdef	RT_USING_NVHE
#include "nvhe/nvhe.h"
#define EL1_(REG)	REG##_EL1	/* P2791 - System and Special-purpose register aliasing */
#else
#include "vhe/vhe.h"
#define EL1_(REG)	REG##_EL12
#endif

#define VMID_SHIFT  (48)
#define VA_MASK     (0x0000FFFFFFFFF000UL)

struct vm;
struct vcpu;

enum vcpu_sysreg 
{
	__INVALID_VCPU_SYSREG__,
	/* common state|sp */

	/* context */
	_TPIDR_EL0,
	_TPIDRRO_EL0,
	_CONTEXTIDR_EL1,

	/* el1 state       */
	_CSSELR_EL1,
	_SCTLR_EL1,
	_CPACR_EL1,

	/* MMU */
	_TTBR0_EL1,
	_TTBR1_EL1,
	_TCR_EL1,
	_VBAR_EL1,

	/* Fault state */	
	_ESR_EL1,
	_FAR_EL1,

	_AFSR0_EL1,
	_AFSR1_EL1,
	_MAIR_EL1,
	_AMAIR_EL1,
	
	/* Timer */
	_CNTKCTL_EL1,
	_CNTVOFF_EL2,
	
	_PAR_EL1,
	_SP_EL1,
	_ELR_EL1,
	_SPSR_EL1,

	_MPIDR_EL1,
	_MIDR_EL1,
	
	/* el2 return state | handle PC and PSTATE */

	NR_VCPU_SYS_REGS	    /* Nothing after this line! */
};

struct cpu_context
{
	struct rt_hw_exp_stack regs;
    rt_uint64_t sys_regs[NR_VCPU_SYS_REGS];

	/* interrupts & timer virtualization | TBD */

    struct vcpu *vcpu;
};

struct vcpu_arch
{
    struct cpu_context vcpu_ctxt;

    /* Values of trap registers for the guest */
	rt_uint64_t hcr_el2;
	rt_uint64_t cptr_el2;
	rt_uint64_t mdcr_el2;
	rt_uint32_t fpexc32_el2;

	/* Values of trap registers for the host before guest entry. */
	rt_uint64_t mdcr_el2_host;
};

struct vm_arch
{
	/* VTCR_EL2 value for this VM */
	rt_uint64_t vtcr_el2;
	rt_uint64_t vttbr_el2;
};

struct arch_info
{
	/* for vGIC */
	struct vgic_info vgic;

	/* for Generic_Timer */

	/* for SMMU */
};

struct hyp_arch
{
	rt_uint32_t ipa_size;
	rt_uint8_t vmid_bits;

	rt_bool_t cpu_hyp_enabled[RT_CPUS_NR];
	rt_bool_t hyp_init_ok;
};

rt_bool_t arm_vhe_supported(void);
rt_int8_t arm_vmid_bits(void);
rt_bool_t arm_sve_supported(void);

void __flush_all_tlb(void);
void flush_vm_all_tlb(struct vm *vm);

void vcpu_state_init(struct vcpu *vcpu);
void vcpu_regs_dump(struct vcpu *vcpu);

/* Different type of switch handler interface in arch. */
void host_to_guest_arch_handler(struct vcpu *vcpu);
void guest_to_host_arch_handler(struct vcpu *vcpu);
void vcpu_to_vcpu_arch_handler(struct vcpu *from, struct vcpu *to);
void guest_to_guest_arch_handler(struct vcpu *from, struct vcpu *to);

#endif  /* __VIRT_H__ */