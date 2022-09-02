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

#include "stage2.h"
#include "virt_arch.h"

rt_bool_t arm_vhe_supported(void)
{
    rt_uint64_t val;
    GET_SYS_REG(ID_AA64MMFR1_EL1, val);
    val = (val >> ID_AA64MMFR1_VH_SHIFT) & ID_AA64MMFR1_MASK;
    return val == ID_AA64MMFR1_VHE;
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
    return val == ID_AA64PFR0_NSVE;
}

void __flush_all_tlb(void)
{
	__asm__ volatile (
		"dsb ish\n\r"
		"tlbi vmalls12e1is\n\r"
		"dsb ish\n\r"
		"isb\n\r"
	);
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

    vm->arch->vtcr_el2  = get_vtcr_el2();
    vm->arch->vttbr_el2 = ((rt_uint64_t)vm->mm->pgd_tbl & S2_VA_MASK) 
                        | ((rt_uint64_t)vm->vm_idx     << VMID_SHIFT);
}

/* Dump vCPU register info */
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

/* 
 * When vCPU sche in 
 */
static void hook_vcpu_load_regs(struct vcpu *vcpu)
{
    /* EL1 state restore */
    rt_kprintf("[Debug] %s, %d\n", __FUNCTION__, __LINE__);
    struct cpu_context *c = &vcpu->arch->vcpu_ctxt;

    /* context */
    SET_SYS_REG(TPIDR_EL0,   c->sys_regs[_TPIDR_EL0]);
    SET_SYS_REG(TPIDRRO_EL0, c->sys_regs[_TPIDRRO_EL0]);
    SET_SYS_REG(EL1_(CONTEXTIDR), c->sys_regs[_CONTEXTIDR_EL1]);
    
    SET_SYS_REG(CSSELR_EL1,  c->sys_regs[_CSSELR_EL1]);
    SET_SYS_REG(EL1_(CPACR), c->sys_regs[_CPACR_EL1]);
    
    /* MMU */
    SET_SYS_REG(EL1_(TTBR0), c->sys_regs[_TTBR0_EL1]);
    SET_SYS_REG(EL1_(TTBR1), c->sys_regs[_TTBR1_EL1]);
    SET_SYS_REG(EL1_(TCR),   c->sys_regs[_TCR_EL1]);
    SET_SYS_REG(EL1_(VBAR),  c->sys_regs[_VBAR_EL1]);

    /* Fault state */
    SET_SYS_REG(EL1_(ESR), c->sys_regs[_ESR_EL1]);
    SET_SYS_REG(EL1_(FAR), c->sys_regs[_FAR_EL1]);

    SET_SYS_REG(EL1_(AFSR0), c->sys_regs[_AFSR0_EL1]);
    SET_SYS_REG(EL1_(AFSR1), c->sys_regs[_AFSR1_EL1]);
    SET_SYS_REG(EL1_(MAIR),  c->sys_regs[_MAIR_EL1]);
    SET_SYS_REG(EL1_(AMAIR), c->sys_regs[_AMAIR_EL1]);

    /* Timer */
    SET_SYS_REG(EL1_(CNTKCTL), c->sys_regs[_CNTKCTL_EL1]);

    SET_SYS_REG(PAR_EL1,    c->sys_regs[_PAR_EL1]);
    SET_SYS_REG(SP_EL1,     c->sys_regs[_SP_EL1]);
    SET_SYS_REG(EL1_(ELR),  c->sys_regs[_ELR_EL1]);
    SET_SYS_REG(EL1_(SPSR), c->sys_regs[_SPSR_EL1]);

    /* P2524 - Traps to EL2 of EL0 and EL1 accesses to the ID registers */
    SET_SYS_REG(VMPIDR_EL2, c->sys_regs[_MPIDR_EL1]);
    SET_SYS_REG(VPIDR_EL2,  c->sys_regs[_MIDR_EL1]);

    SET_SYS_REG(EL1_(SCTLR), c->sys_regs[_SCTLR_EL1]);

    __ISB();
}

static void activate_trap(struct vcpu *vcpu)
{
    rt_kprintf("[Debug] %s, %d\n", __FUNCTION__, __LINE__);
    SET_SYS_REG(HCR_EL2, vcpu->arch->hcr_el2);
    __ISB();

    rt_uint64_t val = 0UL;
    val &= ~CPACR_EL1_ZEN;
    val |= CPACR_EL1_TTA | CPACR_EL1_FPEN | CPTR_EL2_TAM;  /* ! */
    /* With VHE (HCR.E2H == 1), accesses to CPACR_EL1 are routed to CPTR_EL2 */
    SET_SYS_REG(CPACR_EL1, val);

    extern void *system_vectors;    /* TBD */
    SET_SYS_REG(VBAR_EL1, &system_vectors);
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

/*
 * When vCPU sche out 
 */
static void hook_vcpu_save_regs(struct vcpu *vcpu)
{
    /* EL1 state restore */
    rt_kprintf("[Debug] %s, %d\n", __FUNCTION__, __LINE__);
    struct cpu_context *c = &vcpu->arch->vcpu_ctxt;

    /* context */
    GET_SYS_REG(TPIDR_EL0,   c->sys_regs[_TPIDR_EL0]);
    GET_SYS_REG(TPIDRRO_EL0, c->sys_regs[_TPIDRRO_EL0]);
    GET_SYS_REG(EL1_(CONTEXTIDR), c->sys_regs[_CONTEXTIDR_EL1]);
    
    GET_SYS_REG(CSSELR_EL1,  c->sys_regs[_CSSELR_EL1]);
    GET_SYS_REG(EL1_(CPACR), c->sys_regs[_CPACR_EL1]);
    
    /* MMU */
    GET_SYS_REG(EL1_(TTBR0), c->sys_regs[_TTBR0_EL1]);
    GET_SYS_REG(EL1_(TTBR1), c->sys_regs[_TTBR1_EL1]);
    GET_SYS_REG(EL1_(TCR),   c->sys_regs[_TCR_EL1]);
    GET_SYS_REG(EL1_(VBAR),  c->sys_regs[_VBAR_EL1]);

    /* Fault state */
    GET_SYS_REG(EL1_(ESR), c->sys_regs[_ESR_EL1]);
    GET_SYS_REG(EL1_(FAR), c->sys_regs[_FAR_EL1]);

    GET_SYS_REG(EL1_(AFSR0), c->sys_regs[_AFSR0_EL1]);
    GET_SYS_REG(EL1_(AFSR1), c->sys_regs[_AFSR1_EL1]);
    GET_SYS_REG(EL1_(MAIR),  c->sys_regs[_MAIR_EL1]);
    GET_SYS_REG(EL1_(AMAIR), c->sys_regs[_AMAIR_EL1]);

    /* Timer */
    GET_SYS_REG(EL1_(CNTKCTL), c->sys_regs[_CNTKCTL_EL1]);

    GET_SYS_REG(PAR_EL1,    c->sys_regs[_PAR_EL1]);
    GET_SYS_REG(SP_EL1,     c->sys_regs[_SP_EL1]);
    GET_SYS_REG(EL1_(ELR),  c->sys_regs[_ELR_EL1]);
    GET_SYS_REG(EL1_(SPSR), c->sys_regs[_SPSR_EL1]);

    /* P2524 - Traps to EL2 of EL0 and EL1 accesses to the ID registers */
    GET_SYS_REG(VMPIDR_EL2, c->sys_regs[_MPIDR_EL1]);
    GET_SYS_REG(VPIDR_EL2,  c->sys_regs[_MIDR_EL1]);

    GET_SYS_REG(EL1_(SCTLR), c->sys_regs[_SCTLR_EL1]);

    __ISB();
}

static void deactivate_trap(struct vcpu *vcpu)
{
    rt_kprintf("[Debug] %s, %d\n", __FUNCTION__, __LINE__);
    SET_SYS_REG(HCR_EL2, HCR_HOST_VHE_FLAGS);
    SET_SYS_REG(CPACR_EL1, CPACR_EL1_DEFAULT);
}

static void save_stage2_setting(struct vcpu *vcpu)
{
    rt_kprintf("[Debug] %s, %d\n", __FUNCTION__, __LINE__);
    GET_SYS_REG(VTCR_EL2,  vcpu->vm->arch->vtcr_el2);
    GET_SYS_REG(VTTBR_EL2, vcpu->vm->arch->vttbr_el2);
    __ISB();
}

/*
 * Before host schedule into guest vCPU, we need prepare the runtime env, 
 * especially EL1 registers.
 */
void host_to_guest_arch_handler(struct vcpu *vcpu)
{
    rt_kprintf("[Debug] %s, %d\n", __FUNCTION__, __LINE__);

    /* EL1 regs & */
    hook_vcpu_load_regs(vcpu);
    activate_trap(vcpu);
    load_stage2_setting(vcpu);  // interrupts disabled ?
}

/*
 * Before guest vCPU schedule out to host, we need save the runtime env 
 * for next time restore, especially EL1 registers.
 */
void guest_to_host_arch_handler(struct vcpu *vcpu)
{
    rt_kprintf("[Debug] %s, %d\n", __FUNCTION__, __LINE__);

    save_stage2_setting(vcpu);
    deactivate_trap(vcpu);
    hook_vcpu_save_regs(vcpu);
}

/* 
 * From vCPU_1 to vCPU_2 in same vm, most of runtime env need not to change.
 * Just GP_REGs need to change by RESTORE_CONTEXT & SAVE_CONTEXT.
 */
void vcpu_to_vcpu_arch_handler(struct vcpu *from, struct vcpu *to)
{
    /*  */
}

/*
 * From vCPU_1 to vCPU_2 in different vm.
 */
void guest_to_guest_arch_handler(struct vcpu *from, struct vcpu *to)
{
    /* save guest_1 runtime env */

    /* resotre guest_2 runtime env */

}
