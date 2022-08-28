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
#include "stage2.h"

rt_bool_t arm_vhe_supported(void)
{
    rt_uint64_t val;
    GET_SYS_REG(ID_AA64MMFR1_EL1, val);
    val = (val >> ID_AA64MMFR1_VH_SHIFT) & ID_AA64MMFR1_MASK;
    if (val == ID_AA64MMFR1_VHE)
        return RT_TRUE;
    else
        return RT_FALSE;
}

rt_int8_t arm_vmid_bits(void)
{
    rt_int8_t vmid_bits = 8;

    rt_uint64_t val;
    GET_SYS_REG(ID_AA64MMFR1_EL1, val);
    val = (val >> ID_AA64MMFR1_VH_SHIFT) & ID_AA64MMFR1_MASK;
    if (val == ID_AA64MMFR1_VMID_16BIT)
        vmid_bits = 16;

    return vmid_bits;
}

rt_bool_t arm_sve_supported(void)
{
    rt_uint64_t val;
    GET_SYS_REG(ID_AA64PFR0_EL1, val);
    val = (val >> ID_AA64PFR0_SVE_SHIFT) & ID_AA64PFR0_MASK;
    if (val == ID_AA64PFR0_NSVE)
        return RT_FALSE;
    else
        return RT_TRUE;
}

void flush_vm_all_tlb(struct vm *vm)
{
    struct mm_struct *mm = vm->mm;
    rt_uint64_t vttbr = ((rt_uint64_t)mm->pgd_tbl & S2_VA_MASK) 
                      | ((rt_uint64_t)vm->vm_idx << VMID_SHIFT);

    rt_uint64_t old_vttbr; 
    GET_SYS_REG(VTTBR_EL2, old_vttbr);

    if (old_vttbr != vttbr)
        SET_SYS_REG(VTTBR_EL2, vttbr);

    __flush_all_tlb();

    if (old_vttbr != vttbr)
        SET_SYS_REG(VTTBR_EL2, old_vttbr);
}

rt_inline rt_uint64_t get_vtcr_el2(void)
{
	rt_uint64_t vtcr_val = 0UL;
	vtcr_val |= (VTCR_EL2_T0SZ(40)   | VTCR_EL2_SL0_4KB_LEVEL1 | VTCR_EL2_IRGN0_WBWA 
             |   VTCR_EL2_ORGN0_WBWA | VTCR_EL2_SH0_INNER      | VTCR_EL2_TG0_4KB 
             |   VTCR_EL2_PS_40_BIT  | VTCR_EL2_8_VMID);         
	return vtcr_val;
}

void hook_vcpu_state_init(struct vcpu *vcpu)
{
    struct vm *vm = vcpu->vm;
    struct cpu_context *c = &vcpu->arch->vcpu_ctxt;
    
    vcpu->arch->hcr_el2 = HCR_GUEST_FLAGS;
    rt_kprintf("[Info] vcpu->arch->hcr_el2 = 0x%16.16p\n", vcpu->arch->hcr_el2);

    c->sys_regs[_MPIDR_EL1]   = vcpu->vcpu_id;  /* set this value to VMPIDR_EL2 */
    c->sys_regs[_MIDR_EL1]    = 0x410FC050;     /* set this value to VMIDR_EL2 */
    c->sys_regs[_CPACR_EL1]   = CPACR_EL1_FPEN;
    c->sys_regs[_TTBR0_EL1]   = 0UL;
    c->sys_regs[_TTBR1_EL1]   = 0UL;
    c->sys_regs[_TCR_EL1]     = 0UL;
    c->sys_regs[_MAIR_EL1]    = 0UL;
    c->sys_regs[_AMAIR_EL1]   = 0UL;
    c->sys_regs[_PAR_EL1]     = 0UL;
    c->sys_regs[_CNTKCTL_EL1] = 0UL;
    c->sys_regs[_CNTVOFF_EL2] = 0UL;
    c->sys_regs[_SCTLR_EL1] = 0x00C50078;

    c->regs.spsr = SPSR_EL1H | DAIF_FIQ | DAIF_IRQ | DAIF_ABT | DAIF_DBG;
    c->regs.pc = vm->os->entry_point;
    rt_kprintf("[Debug] c->regs.spsr = 0x%08x\n", c->regs.spsr);
    rt_kprintf("[Debug] c->regs.pc   = 0x%08x\n", c->regs.pc);

    vm->arch->vtcr_el2  = get_vtcr_el2();
    vm->arch->vttbr_el2 = ((rt_uint64_t)vm->mm->pgd_tbl & S2_VA_MASK) 
                        | ((rt_uint64_t)vm->vm_idx     << VMID_SHIFT);
}

static void vcpu_save_user_state_regs(struct vcpu *vcpu)
{
    rt_kprintf("[Debug] %s, %d\n", __FUNCTION__, __LINE__);
    struct cpu_context *c = &vcpu->arch->vcpu_ctxt;

    /* user state */
    GET_SYS_REG(TPIDR_EL0, c->sys_regs[_TPIDR_EL0]);
    GET_SYS_REG(TPIDRRO_EL0, c->sys_regs[_TPIDRRO_EL0]);
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
    /* EL1 state restore */
    rt_kprintf("[Debug] %s, %d\n", __FUNCTION__, __LINE__);
    struct cpu_context *c = &vcpu->arch->vcpu_ctxt;

    /* context */
    SET_SYS_REG(TPIDR_EL0, c->sys_regs[_TPIDR_EL0]);
    SET_SYS_REG(TPIDRRO_EL0, c->sys_regs[_TPIDRRO_EL0]);
    SET_SYS_REG(CONTEXTIDR_EL12, c->sys_regs[_CONTEXTIDR_EL1]);
    
    SET_SYS_REG(CSSELR_EL1, c->sys_regs[_CSSELR_EL1]);
    SET_SYS_REG(CPACR_EL12, c->sys_regs[_CPACR_EL1]);
    
    /* MMU */
    SET_SYS_REG(TTBR0_EL12, c->sys_regs[_TTBR0_EL1]);
    SET_SYS_REG(TTBR1_EL12, c->sys_regs[_TTBR1_EL1]);
    SET_SYS_REG(TCR_EL12, c->sys_regs[_TCR_EL1]);
    SET_SYS_REG(VBAR_EL12, c->sys_regs[_VBAR_EL1]);

    /* Fault state */
    SET_SYS_REG(ESR_EL12, c->sys_regs[_ESR_EL1]);
    SET_SYS_REG(FAR_EL12, c->sys_regs[_FAR_EL1]);

    SET_SYS_REG(AFSR0_EL12, c->sys_regs[_AFSR0_EL1]);
    SET_SYS_REG(AFSR1_EL12, c->sys_regs[_AFSR1_EL1]);
    SET_SYS_REG(MAIR_EL12, c->sys_regs[_MAIR_EL1]);
    SET_SYS_REG(AMAIR_EL12, c->sys_regs[_AMAIR_EL1]);

    /* Timer */
    SET_SYS_REG(CNTKCTL_EL12, c->sys_regs[_CNTKCTL_EL1]);

    SET_SYS_REG(PAR_EL1, c->sys_regs[_PAR_EL1]);
    SET_SYS_REG(SP_EL1, c->sys_regs[_SP_EL1]);
    SET_SYS_REG(ELR_EL12, c->sys_regs[_ELR_EL1]);
    SET_SYS_REG(SPSR_EL12, c->sys_regs[_SPSR_EL1]);

    /* P2524 - Traps to EL2 of EL0 and EL1 accesses to the ID registers */
    SET_SYS_REG(VMPIDR_EL2, c->sys_regs[_MPIDR_EL1]);
    SET_SYS_REG(VPIDR_EL2, c->sys_regs[_MIDR_EL1]);

#ifdef ARM64_ERRATUM_1530923
    __ISB();
    /* P2791 - System and Special-purpose register aliasing */
    SET_SYS_REG(SCTLR_EL12, c->sys_regs[_SCTLR_EL1]);
#else
    SET_SYS_REG(SCTLR_EL12, c->sys_regs[_SCTLR_EL1]);
#endif  /* ARM64_ERRATUM_1530923 */

    __ISB();
}

/* When vCPU sche out */
static void hook_vcpu_save_regs(struct vcpu *vcpu)
{
    rt_kprintf("[Debug] %s, %d\n", __FUNCTION__, __LINE__);

    vcpu_save_user_state_regs(vcpu);
    vcpu_save_el1_state_regs(vcpu);
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
    __ISB();

    rt_uint64_t val = 0UL;
    val |= CPACR_EL1_TTA;
    val &= ~CPACR_EL1_ZEN;
    val |= CPACR_EL1_FPEN;  /* ! */
    val |= CPTR_EL2_TAM;
    /* With VHE (HCR.E2H == 1), accesses to CPACR_EL1 are routed to CPTR_EL2 */
    SET_SYS_REG(CPACR_EL1, val);

    extern void *system_vectors;    /* TBD */
    SET_SYS_REG(VBAR_EL1, &system_vectors);
}

static void deactivate_trap(struct vcpu *vcpu)
{
    rt_kprintf("[Debug] %s, %d\n", __FUNCTION__, __LINE__);

    SET_SYS_REG(HCR_EL2, HCR_HOST_VHE_FLAGS);
    SET_SYS_REG(CPACR_EL1, CPACR_EL1_DEFAULT);
}

static void load_stage2_setting(struct vcpu *vcpu)
{
    rt_kprintf("[Debug] %s, %d\n", __FUNCTION__, __LINE__);
    __ISB();
    SET_SYS_REG(VTCR_EL2,  vcpu->vm->arch->vtcr_el2);
    SET_SYS_REG(VTTBR_EL2, vcpu->vm->arch->vttbr_el2);
    __ISB();
    flush_vm_all_tlb(vcpu->vm);
}

/* When vCPU sche in */
void vcpu_sche_in(struct vcpu *vcpu)
{
    rt_kprintf("[Debug] %s, %d\n", __FUNCTION__, __LINE__);
    hook_vcpu_load_regs(vcpu);

    /* activate trap */
    activate_trap(vcpu);
    
    /* load this VM's stage2 setting */
    load_stage2_setting(vcpu);  // interrupts disabled ?

    rt_kprintf("[Debug] __vcpu_entry\n");
    __vcpu_entry((void *)(&vcpu->arch->vcpu_ctxt.regs));
}

/* When vCPU sche out */
void vcpu_sche_out(struct vcpu* vcpu)
{
    rt_kprintf("[Debug] %s, %d\n", __FUNCTION__, __LINE__);

    hook_vcpu_save_regs(vcpu);

    deactivate_trap(vcpu);
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
    rt_kprintf("_SP_EL1: 0x%16lx         _ELR_EL1: 0x%16lx\n", 
            c->sys_regs[_SP_EL1], c->sys_regs[_ELR_EL1]);
    rt_kprintf("_MPIDR_EL1: 0x%16lx      _MIDR_EL1: 0x%16lx\n", 
            c->sys_regs[_MPIDR_EL1], c->sys_regs[_MIDR_EL1]);
    rt_kprintf("_SPSR_EL1: 0x%16lx\n", c->sys_regs[_SPSR_EL1]);
}
