/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-05-30     Suqier       first version
 */

#ifndef __VM_ARCH_H__
#define __VM_ARCH_H__

#include <rtdef.h>
#include <rtconfig.h>

// #if defined(RT_HYPERVISOR) 

/**
 *  All vcpu architecture related content is in this file.
 *  It will be used by hypervisor.
 */

struct user_pt_regs
{
    rt_uint64_t regs[31];
    rt_uint64_t sp;
    rt_uint64_t pc;
    rt_uint64_t pstate;
};

/*
 * 0 is reserved as an invalid value.
 * Order should be kept in sync with the save/restore code.
 */
enum vcpu_sysreg 
{
	__INVALID_SYSREG__,
	_MPIDR_EL1,	    /* MultiProcessor Affinity Register */
	_CSSELR_EL1,	    /* Cache Size Selection Register */
	_SCTLR_EL1,	    /* System Control Register */
	_ACTLR_EL1,	    /* Auxiliary Control Register */
	_CPACR_EL1,	    /* Coprocessor Access Control */
	_TTBR0_EL1,	    /* Translation Table Base Register 0 */
	_TTBR1_EL1,	    /* Translation Table Base Register 1 */
	_TCR_EL1,	    /* Translation Control Register */
	_ESR_EL1,	    /* Exception Syndrome Register */
	_AFSR0_EL1,	    /* Auxiliary Fault Status Register 0 */
	_AFSR1_EL1,	    /* Auxiliary Fault Status Register 1 */
	_FAR_EL1,	    /* Fault Address Register */
	_MAIR_EL1,	    /* Memory Attribute Indirection Register */
	_VBAR_EL1,	    /* Vector Base Address Register */
	_CONTEXTIDR_EL1,	/* Context ID Register */
	_TPIDR_EL0,	    /* Thread ID, User R/W */
	_TPIDRRO_EL0,	/* Thread ID, User R/O */
	_TPIDR_EL1,	    /* Thread ID, Privileged */
	_AMAIR_EL1,	    /* Aux Memory Attribute Indirection Register */
	_CNTKCTL_EL1,	/* Timer Control Register (EL1) */
	_PAR_EL1,	    /* Physical Address Register */
	_MDSCR_EL1,	    /* Monitor Debug System Control Register */
	_MDCCINT_EL1,	/* Monitor Debug Comms Channel Interrupt Enable Reg */
	_DISR_EL1,	    /* Deferred Interrupt Status Register */

	/* Performance Monitors Registers */
	_PMCR_EL0,	    /* Control Register */
	_PMSELR_EL0,	/* Event Counter Selection Register */
	_PMEVCNTR0_EL0,	/* Event Counter Register (0-30) */
	_PMEVCNTR30_EL0 = _PMEVCNTR0_EL0 + 30,
	_PMCCNTR_EL0,	/* Cycle Counter Register */
	_PMEVTYPER0_EL0,/* Event Type Register (0-30) */
	_PMEVTYPER30_EL0 = _PMEVTYPER0_EL0 + 30,
	_PMCCFILTR_EL0,	/* Cycle Count Filter Register */
	_PMCNTENSET_EL0,/* Count Enable Set Register */
	_PMINTENSET_EL1,/* Interrupt Enable Set Register */
	_PMOVSSET_EL0,	/* Overflow Flag Status Set Register */
	_PMSWINC_EL0,	/* Software Increment Register */
	_PMUSERENR_EL0,	/* User Enable Register */

	/* 32bit specific registers. Keep them at the end of the range */
	_DACR32_EL2,	/* Domain Access Control Register */
	_IFSR32_EL2,	/* Instruction Fault Status Register */
	_FPEXC32_EL2,	/* Floating-Point Exception Control Register */
	_DBGVCR32_EL2,	/* Debug Vector Catch Register */

	NR_SYS_REGS	    /* Nothing after this line! */
};

struct vcpu_context 
{
    struct user_pt_regs regs;   /* sp = sp_el0 */

    rt_uint64_t sys_regs[NR_SYS_REGS];

    struct vcpu *vcpu;
};

struct vcpu_arch
{
    struct vcpu_context ctxt;

    /* Values of trap registers for the guest. */
	rt_uint64_t hcr_el2;
	rt_uint64_t mdcr_el2;
	rt_uint64_t cptr_el2;
};

struct vm_arch
{
	/* VTCR_EL2 value for this VM */
	rt_uint64_t vtcr_el2;

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

// #endif  /* RT_HYPERVISOR */

#endif  /* __VM_ARCH_H__ */