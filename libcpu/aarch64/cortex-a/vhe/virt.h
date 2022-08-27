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

#include "armv8.h"
#include "lib_helpers.h"
#include "vhe.h"
#include "vm.h"

#if defined(RT_HYPERVISOR)

#ifndef RT_USING_SMP
#define RT_CPUS_NR      1
extern int rt_hw_cpu_id(void);
#else
extern rt_uint64_t rt_cpu_mpidr_early[];
#endif /* RT_USING_SMP */

#define VMID_SHIFT  (48)
#define VA_MASK     (0x0000fffffffff000UL)

enum vcpu_sysreg 
{
	__INVALID_SYSREG__,
	/* common state|sp */

	/* user state      */
	_TPIDR_EL0,
	_TPIDRRO_EL0,

	/* el1 state       */
	_CSSELR_EL1,
	_SCTLR_EL1,
	_CPACR_EL1,
	_TTBR0_EL1,
	_TTBR1_EL1,
	_TCR_EL1,
	_ESR_EL1,
	_AFSR0_EL1,
	_AFSR1_EL1,
	_FAR_EL1,
	_MAIR_EL1,
	_VBAR_EL1,
	_CONTEXTIDR_EL1,
	_AMAIR_EL1,
	_CNTKCTL_EL1,
	_CNTVOFF_EL2,
	_PAR_EL1,
	_SP_EL1,
	_ELR_EL1,
	_SPSR_EL1,
	_MPIDR_EL1,
	_MIDR_EL1,
	
	/* el2 return state | handle PC and PSTATE */

	NR_SYS_REGS	    /* Nothing after this line! */
};

struct cpu_context
{
	struct rt_hw_exp_stack regs;
    rt_uint64_t sys_regs[NR_SYS_REGS];

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

	/* Interrupt controller */
	// struct vgic_dist vgic;

	/* Mandated version of PSCI */
	rt_uint32_t psci_version;
};

struct hyp_arch
{
	rt_uint32_t ipa_size;
	rt_uint8_t vmid_bits;

	rt_bool_t cpu_hyp_enabled[RT_CPUS_NR];
	rt_bool_t hyp_init_ok;
};

struct vm;
struct vcpu;

void __flush_all_tlb(void);
void flush_vm_all_tlb(struct vm *vm);

void hook_vcpu_state_init(struct vcpu *vcpu);

void __vcpu_entry(void* regs);

void vcpu_sche_in(struct vcpu *vcpu);
void vcpu_sche_out(struct vcpu* vcpu);

void hook_vcpu_dump_regs(struct vcpu *vcpu);

#endif  /* RT_HYPERVISOR */
#endif  /* __VIRT_H__ */