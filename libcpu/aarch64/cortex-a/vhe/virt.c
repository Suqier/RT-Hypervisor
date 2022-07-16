/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-21     Suqier       first version
 */

#include <cpuport.h>

#include "virt.h"

static struct cpu_context host_ctxt;

void flush_guest_all_tlb(struct vm *vm)
{
    struct mm_struct *mm = vm->mm;

    rt_uint64_t vttbr;
    vttbr = (*(mm->pgd_tbl) & VA_MASK) | (rt_uint64_t)vm->vm_idx << VMID_SHIFT;
    
    rt_hw_spin_lock(&mm->lock);
    SET_SYS_REG(VTTBR_EL2, vttbr);
    __flush_all_tlb();
    rt_hw_spin_unlock(&mm->lock);
}

rt_inline void hook_vcpu_adjust_pc(struct vcpu *vcpu)
{
    rt_kprintf("[Debug] %s, %d\n", __FUNCTION__, __LINE__);

    if (vcpu->flag & VCPU_INCREMENT_PC)
    {
        vcpu->arch->vcpu_ctxt.regs.pc += 4;
        vcpu->flag &= ~VCPU_INCREMENT_PC;
    }
}

rt_inline rt_uint64_t get_vtcr_el2(void)
{
	rt_uint64_t vtcr_val = 0UL;

    /* IPA support 40bits */
	vtcr_val |= VTCR_EL2_T0SZ(40);

    /* 4kb start at level1 */
	vtcr_val |= VTCR_EL2_SL0_4KB_LEVEL1;	

    /* Normal memory, Inner WBWA */
	vtcr_val |= VTCR_EL2_IRGN0_WBWA;

    /* Normal memory, Outer WBWA */
	vtcr_val |= VTCR_EL2_ORGN0_WBWA;
	
    /* Inner Shareable */
    vtcr_val |= VTCR_EL2_SH0_INNER;

	/* TG0 4K */
	vtcr_val |= VTCR_EL2_TG0_4KB;

	/* rk3568 just support pysical size 64GB */
	vtcr_val |= VTCR_EL2_PS_64GB;

	/* vmid -- 8bit */
	vtcr_val |= VTCR_EL2_VS_8BIT;

	return vtcr_val;
}

void hook_vcpu_state_init(struct vcpu *vcpu)
{
    struct vm *vm = vcpu->vm;
    struct cpu_context *c = &vcpu->arch->vcpu_ctxt;
    
    GET_SYS_REG(HCR_EL2, vcpu->arch->hcr_el2);
    vcpu->arch->hcr_el2 |= (HCR_GUEST_FLAGS & 0x00000000ffffffff) | HCR_INT_OVERRIDE;

    c->sys_regs[_MPIDR_EL1] = vcpu->vcpu_id;  /* set this value to VMPIDR_EL2 */
    c->sys_regs[_MIDR_EL1]  = 0x410FC050;     /* set this value to VMIDR_EL2 */
    c->sys_regs[_CPACR_EL1] = CPACR_EL1_FPEN;
    c->sys_regs[_TTBR0_EL1] = 0UL;
    c->sys_regs[_TTBR1_EL1] = 0UL;
    c->sys_regs[_TCR_EL1]   = 0UL;
    c->sys_regs[_MAIR_EL1]  = 0UL;
    c->sys_regs[_AMAIR_EL1] = 0UL;
    c->sys_regs[_PAR_EL1]   = 0UL;

    c->regs.pstate = SPSR_EL1H | DAIF_FIQ | DAIF_IRQ | DAIF_ABT;
    c->regs.pc = vm->os->entry_point;

    vm->arch->vtcr_el2  = get_vtcr_el2();
    vm->arch->vttbr_el2 = *vm->mm->pgd_tbl;
}

static void vcpu_load_user_state_regs(struct vcpu *vcpu)
{
    rt_kprintf("[Debug] %s, %d\n", __FUNCTION__, __LINE__);
    struct cpu_context *c = &vcpu->arch->vcpu_ctxt;

    /* user state */
    SET_SYS_REG(TPIDR_EL0, c->sys_regs[_TPIDR_EL0]);
    SET_SYS_REG(TPIDRRO_EL0, c->sys_regs[_TPIDRRO_EL0]);
}

static void vcpu_save_user_state_regs(struct vcpu *vcpu)
{

    rt_kprintf("[Debug] %s, %d\n", __FUNCTION__, __LINE__);
    struct cpu_context *c = &vcpu->arch->vcpu_ctxt;

    /* user state */
    GET_SYS_REG(TPIDR_EL0, c->sys_regs[_TPIDR_EL0]);
    GET_SYS_REG(TPIDRRO_EL0, c->sys_regs[_TPIDRRO_EL0]);
}

static void vcpu_load_el1_state_regs(struct vcpu *vcpu)
{
    rt_kprintf("[Debug] %s, %d\n", __FUNCTION__, __LINE__);
    struct cpu_context *c = &vcpu->arch->vcpu_ctxt;

    /* el1 state  */
    rt_kprintf("[Debug] Here 1\n");
    SET_SYS_REG(CSSELR_EL1, c->sys_regs[_CSSELR_EL1]);
    rt_kprintf("[Debug] Here 1\n");
    SET_SYS_REG(SCTLR_EL1, c->sys_regs[_SCTLR_EL1]);
    rt_kprintf("[Debug] Here 1\n");
    SET_SYS_REG(CPACR_EL1, c->sys_regs[_CPACR_EL1]);
    SET_SYS_REG(TTBR0_EL1, c->sys_regs[_TTBR0_EL1]);
    SET_SYS_REG(TTBR1_EL1, c->sys_regs[_TTBR1_EL1]);
    rt_kprintf("[Debug] Here 1\n");
    SET_SYS_REG(TCR_EL1, c->sys_regs[_TCR_EL1]);
    SET_SYS_REG(ESR_EL1, c->sys_regs[_ESR_EL1]);
    SET_SYS_REG(AFSR0_EL1, c->sys_regs[_AFSR0_EL1]);
    SET_SYS_REG(AFSR1_EL1, c->sys_regs[_AFSR1_EL1]);
    rt_kprintf("[Debug] Here 1\n");
    SET_SYS_REG(FAR_EL1, c->sys_regs[_FAR_EL1]);
    SET_SYS_REG(MAIR_EL1, c->sys_regs[_MAIR_EL1]);
    SET_SYS_REG(VBAR_EL1, c->sys_regs[_VBAR_EL1]);
    rt_kprintf("[Debug] Here 1\n");
    SET_SYS_REG(CONTEXTIDR_EL1, c->sys_regs[_CONTEXTIDR_EL1]);
    SET_SYS_REG(AMAIR_EL1, c->sys_regs[_AMAIR_EL1]);
    SET_SYS_REG(CNTKCTL_EL1, c->sys_regs[_CNTKCTL_EL1]);
    SET_SYS_REG(PAR_EL1, c->sys_regs[_PAR_EL1]);
    rt_kprintf("[Debug] Here 1\n");
    SET_SYS_REG(TFSR_EL1, c->sys_regs[_TFSR_EL1]);
    SET_SYS_REG(TFSRE0_EL1, c->sys_regs[_TFSRE0_EL1]);
    SET_SYS_REG(SP_EL1, c->sys_regs[_SP_EL1]);
    SET_SYS_REG(ELR_EL1, c->sys_regs[_ELR_EL1]);
    SET_SYS_REG(SPSR_EL1, c->sys_regs[_SPSR_EL1]);
    rt_kprintf("[Debug] %s, %d\n", __FUNCTION__, __LINE__);

    /* P2524 - Traps to EL2 of EL0 and EL1 accesses to the ID registers */
    SET_SYS_REG(VMPIDR_EL2, c->sys_regs[_MPIDR_EL1]);
    SET_SYS_REG(VPIDR_EL2, c->sys_regs[_MIDR_EL1]);
}

static void vcpu_save_el1_state_regs(struct vcpu *vcpu)
{
    struct cpu_context *c = &vcpu->arch->vcpu_ctxt;

    /* el1 state  */
    GET_SYS_REG(CSSELR_EL1, c->sys_regs[_CSSELR_EL1]);
    GET_SYS_REG(SCTLR_EL1, c->sys_regs[_SCTLR_EL1]);
    GET_SYS_REG(CPACR_EL1, c->sys_regs[_CPACR_EL1]);
    GET_SYS_REG(TTBR0_EL1, c->sys_regs[_TTBR0_EL1]);
    GET_SYS_REG(TTBR1_EL1, c->sys_regs[_TTBR1_EL1]);
    GET_SYS_REG(TCR_EL1, c->sys_regs[_TCR_EL1]);
    GET_SYS_REG(ESR_EL1, c->sys_regs[_ESR_EL1]);
    GET_SYS_REG(AFSR0_EL1, c->sys_regs[_AFSR0_EL1]);
    GET_SYS_REG(AFSR1_EL1, c->sys_regs[_AFSR1_EL1]);
    GET_SYS_REG(FAR_EL1, c->sys_regs[_FAR_EL1]);
    GET_SYS_REG(MAIR_EL1, c->sys_regs[_MAIR_EL1]);
    GET_SYS_REG(VBAR_EL1, c->sys_regs[_VBAR_EL1]);
    GET_SYS_REG(CONTEXTIDR_EL1, c->sys_regs[_CONTEXTIDR_EL1]);
    GET_SYS_REG(AMAIR_EL1, c->sys_regs[_AMAIR_EL1]);
    GET_SYS_REG(CNTKCTL_EL1, c->sys_regs[_CNTKCTL_EL1]);
    GET_SYS_REG(PAR_EL1, c->sys_regs[_PAR_EL1]);
    GET_SYS_REG(TFSR_EL1, c->sys_regs[_TFSR_EL1]);
    GET_SYS_REG(TFSRE0_EL1, c->sys_regs[_TFSRE0_EL1]);
    GET_SYS_REG(SP_EL1, c->sys_regs[_SP_EL1]);
    GET_SYS_REG(ELR_EL1, c->sys_regs[_ELR_EL1]);
    GET_SYS_REG(SPSR_EL1, c->sys_regs[_SPSR_EL1]);

    /* P2524 - Traps to EL2 of EL0 and EL1 accesses to the ID registers */
    GET_SYS_REG(VMPIDR_EL2, c->sys_regs[_MPIDR_EL1]);
    GET_SYS_REG(VPIDR_EL2, c->sys_regs[_MIDR_EL1]);
}

/* When vCPU sche in */
static void hook_vcpu_load_regs(struct vcpu *vcpu)
{
    rt_kprintf("[Debug] %s, %d\n", __FUNCTION__, __LINE__);
    
    vcpu_load_user_state_regs(vcpu);
    vcpu_load_el1_state_regs(vcpu);
}

/* When vCPU sche out */
static void hook_vcpu_save_regs(struct vcpu *vcpu)
{
    vcpu_save_user_state_regs(vcpu);
    vcpu_save_el1_state_regs(vcpu);
}

/* When vCPU trap in: EL1 -> EL2 */
static void vcpu_load_el2_return_state(struct vcpu *vcpu)
{
    rt_kprintf("[Debug] %s, %d\n", __FUNCTION__, __LINE__);
    struct cpu_context *c = &vcpu->arch->vcpu_ctxt;

    /* el2 return state | handle PC and PSTATE */
    SET_SYS_REG(SPSR_EL2, c->regs.pstate);
    SET_SYS_REG(ELR_EL2, c->regs.pc);
    rt_kprintf("[Debug] c->regs.pc = 0x%08x\n", c->regs.pc);
}

/* When vCPU trap out: EL2 -> EL1 */
static void vcpu_save_el2_return_state(struct vcpu *vcpu)
{
    struct cpu_context *c = &vcpu->arch->vcpu_ctxt;

    /* el2 return state | handle PC and PSTATE */
    GET_SYS_REG(SPSR_EL2, c->regs.pstate);
    GET_SYS_REG(ELR_EL2, c->regs.pc);
}

static void activate_trap(struct vcpu *vcpu)
{
    rt_kprintf("[Debug] %s, %d\n", __FUNCTION__, __LINE__);
    /*
     * HCR_EL2_TVM
     * CPACR_EL1_TTA
     * ~(CPACR_EL1_ZEN_EL0EN | CPACR_EL1_ZEN_EL1EN)
     * CPTR_EL2_TAM
     * VBAR_EL1
     */
    vcpu->arch->hcr_el2 |= HCR_TVM;
    SET_SYS_REG(HCR_EL2, vcpu->arch->hcr_el2);

    rt_uint64_t val = 0UL;
    val |= CPACR_EL1_TTA;
    val &= ~CPACR_EL1_ZEN;
    val |= CPACR_EL1_FPEN;  /* ! */
    val |= CPTR_EL2_TAM_VHE;
    /* With VHE (HCR.E2H == 1), accesses to CPACR_EL1 are routed to CPTR_EL2 */
    SET_SYS_REG(CPACR_EL1, val);

    extern void *system_vectors;    /* TBD */
    SET_SYS_REG(VBAR_EL1, &system_vectors);
}

static void deactivate_trap(struct vcpu *vcpu)
{
    rt_kprintf("[DEBUG] deactivate_trap\n");
}

/* vCPU trap in */
void vcpu_trap_in(struct vcpu *vcpu)
{
    rt_kprintf("[DEBUG] vcpu_trap_in\n");
}

/* vCPU trap out */
void vcpu_trap_out(struct vcpu *vcpu)
{
    rt_kprintf("[DEBUG] vcpu_trap_out\n");
}

static void load_stage2_setting(struct vcpu *vcpu)
{
    SET_SYS_REG(VTCR_EL2, vcpu->vm->arch->vtcr_el2);
    SET_SYS_REG(VTTBR_EL2, vcpu->vm->arch->vttbr_el2);
    __ISB();
}

/* When vCPU sche in */
void vm_entry(struct vcpu *vcpu)
{
    rt_kprintf("[Debug] %s, %d\n", __FUNCTION__, __LINE__);
    /* save host context [but save nothing yet] */

    /* load this VM's stage2 setting */
    load_stage2_setting(vcpu);

    /* activate trap */
    activate_trap(vcpu);

    /* before enter VM and run vCPU, check whether PC need adjust */
    hook_vcpu_adjust_pc(vcpu);

    /* load vCPU sys regs [PC & PSTATE] */
    vcpu_load_el2_return_state(vcpu);

    // hook_vcpu_load_regs(vcpu);

    /* __guest_enter */
    rt_kprintf("[DEBUG] __guest_enter\n");
    __guest_enter(&vcpu->arch->vcpu_ctxt, &host_ctxt);
}

/* When vCPU sche out */
void vm_exit(struct vcpu *vcpu)
{
    rt_kprintf("[DEBUG] vm_exit\n");
}

/* Debug dump */
void hook_vcpu_dump_regs(struct vcpu *vcpu)
{
    struct cpu_context *c = &vcpu->arch->vcpu_ctxt;
    
    rt_kprintf("Dump vCPU Sys Regs:\n");
    rt_kprintf("_TPIDRRO_EL0: 0x%16lx    _TPIDR_EL0: 0x%16lx\n", 
            c->sys_regs[_TPIDRRO_EL0], c->sys_regs[_TPIDR_EL0]);
    rt_kprintf("_CSSELR_EL1: 0x%16lx     _SCTLR_EL1: 0x%16lx\n", 
            c->sys_regs[_CSSELR_EL1], c->sys_regs[_SCTLR_EL1]);
    rt_kprintf("_CPACR_EL1: 0x%16lx      _TCR_EL1: 0x%16lx\n", 
            c->sys_regs[_CPACR_EL1], c->sys_regs[_TCR_EL1]);
    rt_kprintf("_TTBR0_EL1: 0x%16lx      _TTBR1_EL1: 0x%16lx\n", 
            c->sys_regs[_TTBR0_EL1], c->sys_regs[_TTBR1_EL1]);
    rt_kprintf("_ESR_EL1: 0x%16lx        _FAR_EL1: 0x%16lx\n", 
            c->sys_regs[_ESR_EL1], c->sys_regs[_FAR_EL1]);
    rt_kprintf("_AFSR0_EL1: 0x%16lx      _AFSR1_EL1: 0x%16lx\n", 
            c->sys_regs[_AFSR0_EL1], c->sys_regs[_AFSR1_EL1]);
    rt_kprintf("_MAIR_EL1: 0x%16lx       _AMAIR_EL1: 0x%16lx\n", 
            c->sys_regs[_MAIR_EL1], c->sys_regs[_AMAIR_EL1]);
    rt_kprintf("_CONTEXTIDR_EL1: 0x%16lx _VBAR_EL1: 0x%16lx\n", 
            c->sys_regs[_CONTEXTIDR_EL1], c->sys_regs[_VBAR_EL1]);
    rt_kprintf("_CNTKCTL_EL1: 0x%16lx    _PAR_EL1: 0x%16lx\n", 
            c->sys_regs[_CNTKCTL_EL1], c->sys_regs[_PAR_EL1]);
    rt_kprintf("_TFSR_EL1: 0x%16lx       _TFSRE0_EL1: 0x%16lx\n", 
            c->sys_regs[_TFSR_EL1], c->sys_regs[_TFSRE0_EL1]);
    rt_kprintf("_SP_EL1: 0x%16lx         _ELR_EL1: 0x%16lx\n", 
            c->sys_regs[_SP_EL1], c->sys_regs[_ELR_EL1]);
    rt_kprintf("_MPIDR_EL1: 0x%16lx      _MIDR_EL1: 0x%16lx\n", 
            c->sys_regs[_MPIDR_EL1], c->sys_regs[_MIDR_EL1]);
    rt_kprintf("_SPSR_EL1: 0x%16lx\n", c->sys_regs[_SPSR_EL1]);
}
