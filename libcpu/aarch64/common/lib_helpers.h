/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-06     Suqier       first version
 */

#ifndef __LIB_HELPERS_H__
#define __LIB_HELPERS_H__

#include <rtdef.h>

// #ifdef __ASSEMBLY__

#define GET_SYS_REG(reg, out) __asm__ volatile ("mrs %0, " reg:"=r"(out)::"memory");
#define SET_SYS_REG(reg, in)  __asm__ volatile ("msr " reg ", %0"::"r"(in):"memory");

/* AArch64 common register. */
#define CNTFRQ_EL0		"S3_3_C14_C0_0"
#define	CNTV_CTL_EL0	"S3_3_C14_C3_1"
#define	CNTV_CVAL_EL0	"S3_3_C14_C3_2"
#define	CNTVCT_EL0		"S3_3_C14_C0_2"
#define TPIDR_EL0		"S3_3_C13_C0_2"
#define	TPIDRRO_EL0		"S3_3_C13_C0_3"

#define	CLIDR_EL1		"S3_1_C0_C0_1"
#define	CSSELR_EL1		"S3_2_C0_C0_0"
#define	CCSIDR_EL1		"S3_1_C0_C0_0"
#define ID_AA64PFR0_EL1	"S3_0_C0_C4_0"
#define ID_AA64PFR1_EL1	"S3_0_C0_C4_1"
#define ID_AA64MMFR0_EL1	"S3_0_C0_C7_0"
#define ID_AA64MMFR1_EL1    "S3_0_C0_C7_1"
#define ID_AA64MMFR2_EL1    "S3_0_C0_C7_2"
#define	DAIF			"S3_3_C4_C2_1"
#define TPIDR_EL1		"S3_0_C13_C0_4"
#define CPACR_EL1		"S3_0_C1_C0_2"
#define ESR_EL1			"S3_0_C5_C2_0"
#define AFSR0_EL1		"S3_0_C5_C1_0"
#define AFSR1_EL1		"S3_0_C5_C1_1"
#define FAR_EL1			"S3_0_C6_C0_0"
#define VBAR_EL1		"S3_0_C12_C0_0"
#define CONTEXTIDR_EL1	"S3_0_C13_C0_1"
#define AMAIR_EL1		"S3_0_C10_C3_0"
#define CNTKCTL_EL1		"S3_0_C14_C1_0"
#define PAR_EL1			"S3_0_C7_C4_0"
#define TFSR_EL1		"S3_0_C5_C6_0"
#define TFSRE0_EL1		"S3_0_C5_C6_1"
#define SP_EL1			"S3_4_C4_C1_0"
#define ELR_EL1			"S3_0_C4_C0_1"
#define ELR_EL2			"S3_4_C4_C0_1"
#define SPSR_EL1		"S3_0_C4_C0_0"
#define SPSR_EL2		"S3_4_C4_C0_0"

#define	CNTHCTL_EL2		"S3_4_C14_C1_0"
#define	CNTHP_CTL_EL2	"S3_4_C14_C2_1"
#define	CNTHPS_CTL_EL2	"S3_4_C14_C5_1"
#define	CNTVOFF_EL2		"S3_4_C14_C0_3"
#define TPIDR_EL2		"S3_4_C13_C0_2"

#define MIDR_EL1		"S3_0_C0_C0_0"
#define MPIDR_EL1		"S3_0_C0_C0_5"
#define VPIDR_EL2		"S3_4_C0_C0_0"
#define VMPIDR_EL2		"S3_4_C0_C0_5"
#define HCR_EL2			"S3_4_C1_C1_0"
#define VTCR_EL2		"S3_4_C2_C1_2"
#define VTTBR_EL2		"S3_4_C2_C1_0"
#define VBAR_EL2		"S3_4_C12_C0_0"

#define	SCTLR_EL1		"S3_0_C1_C0_0"
#define	SCTLR_EL2		"S3_4_C1_C0_0"
#define	CPTR_EL2		"S3_4_C1_C1_2"
#define	ESR_EL2			"S3_4_C5_C2_0"
#define	FAR_EL2			"S3_4_C6_C0_0"
#define	HPFAR_EL2		"S3_4_C6_C0_4"

#define MAIR_EL1        "S3_0_C10_C2_0"
#define MAIR_EL2        "S3_4_C10_C2_0"

#define TTBR0_EL1       "S3_0_C2_C0_0"
#define TTBR1_EL1       "S3_0_C2_C0_1"
#define TCR_EL1         "S3_0_C2_C0_2"

#define SCTLR_EL12		"S3_5_C1_C0_0"
#define CPACR_EL12		"S3_5_C1_C0_2"
#define TTBR0_EL12      "S3_5_C2_C0_0"
#define TTBR1_EL12      "S3_5_C2_C0_1"
#define TCR_EL12        "S3_5_C2_C0_2"
#define AFSR0_EL12		"S3_5_C5_C1_0"
#define AFSR1_EL12		"S3_5_C5_C1_1"
#define	ESR_EL12		"S3_5_C5_C2_0"
#define	FAR_EL12		"S3_5_C6_C0_0"
#define MAIR_EL12		"S3_5_C10_C2_0"
#define AMAIR_EL12		"S3_5_C10_C3_0"
#define VBAR_EL12		"S3_5_C12_C0_0"
#define CONTEXTIDR_EL12	"S3_5_C13_C0_1"
#define CNTKCTL_EL12	"S3_5_C14_C1_0"
#define SPSR_EL12		"S3_5_C4_C0_0"
#define ELR_EL12		"S3_5_C4_C0_1"

#define DAIFSET_FIQ		(1 << 0)
#define DAIFSET_IRQ		(1 << 1)
#define DAIFSET_ABT		(1 << 2)
#define DAIFSET_DBG		(1 << 3)

#define DAIFCLR_FIQ		(1 << 0)
#define DAIFCLR_IRQ		(1 << 1)
#define DAIFCLR_ABT		(1 << 2)
#define DAIFCLR_DBG		(1 << 3)

#define DAIF_FIQ		(1 << 6)
#define DAIF_IRQ		(1 << 7)
#define DAIF_ABT		(1 << 8)
#define DAIF_DBG		(1 << 9)

#define SPSR_DAIF_SHIFT	(6)
#define SPSR_DAIF_MASK	(0xf << SPSR_DAIF_SHIFT)

#define SPSR_EL0T		(0b0000)
#define SPSR_EL1T		(0b0100)
#define SPSR_EL1H		(0b0101)
#define SPSR_EL2T		(0b1000)
#define SPSR_EL2H		(0b1001)
#define SPSR_MASK		(0b1111)

#define SCTLR_RES1		((1 << 29) | (1 << 28) | (1 << 23) | \
				 		 (1 << 22) | (1 << 18) | (1 << 16) | \
				 		 (1 << 11) | (1 << 5)  | (1 << 4))
#define SCTLR_EL3_RES1  SCTLR_RES1
#define SCTLR_EL2_RES1  SCTLR_RES1
#define SCTLR_EL1_RES1	((1 << 29) | (1 << 28) | (1 << 23) | \
                         (1 << 22) | (1 << 20) | (1 << 11))

#define SCTLR_M		(1 << 0)
#define SCTLR_A		(1 << 1)
#define SCTLR_C		(1 << 2)
#define SCTLR_SA	(1 << 3)
#define SCTLR_SA0	(1 << 4)
#define SCTLR_SP    (SCTLR_SA | SCTLR_SA0)
#define SCTLR_I		(1 << 12)
#define SCTLR_EIS	(1 << 22)
#define SCTLR_SPA	(1 << 23)

#define SCR_NS		(1 << 0)
#define SCR_IRQ		(1 << 1)
#define SCR_FIQ		(1 << 2)
#define SCR_EA		(1 << 3)
#define SCR_RES1    ((1 << 4) | (1 << 5))
#define SCR_SMD		(1 << 7)
#define SCR_HCE		(1 << 8)
#define SCR_RW		(1 << 10)
#define SCR_ST		(1 << 11)

#define HCR_RW      (1 << 31)
#define HCR_SWIO    (1 << 1)

#define CPACR_EL1_TTA		(1 << 28)
#define CPACR_EL1_FPEN      ((1 << 20) | (1 << 21))
#define	CPACR_EL1_ZEN		((1 << 16) | (1 << 17))
#define CPACR_EL1_DEFAULT	(CPACR_EL1_FPEN | CPACR_EL1_ZEN)

/* 
 * TCR_ELx Registers bits. 
 * 
 * It can be used in TCR_EL1 or 
 * TCR_EL2 When VHE is implemented and HCR_EL2.E2H == 1. 
 */
#define TCR_T0SZ_OFFSET		0
#define TCR_T1SZ_OFFSET		16
#define TCR_T0SZ(x)		    ((64UL - (x)) << TCR_T0SZ_OFFSET)
#define TCR_T1SZ(x)		    ((64UL - (x)) << TCR_T1SZ_OFFSET)

#define TCR_RES0            ((1 << 6) | (1UL << 35) | (0xFUL << 60))
#define TCR_EPD0            (1 << 7)
#define TCR_EPD1            (1 << 23)

#define TCR_IRGN0_SHIFT		8
#define TCR_IRGN0_NC		(0 << TCR_IRGN0_SHIFT)
#define TCR_IRGN0_WBWA		(1 << TCR_IRGN0_SHIFT)
#define TCR_IRGN0_WT		(2 << TCR_IRGN0_SHIFT)
#define TCR_IRGN0_WBNWA		(3 << TCR_IRGN0_SHIFT)

#define TCR_ORGN0_SHIFT		10
#define TCR_ORGN0_NC		(0 << TCR_ORGN0_SHIFT)
#define TCR_ORGN0_WBWA		(1 << TCR_ORGN0_SHIFT)
#define TCR_ORGN0_WT		(2 << TCR_ORGN0_SHIFT)
#define TCR_ORGN0_WBNWA		(3 << TCR_ORGN0_SHIFT)

#define TCR_SH0_SHIFT		12
#define TCR_SH0_NON		    (0 << TCR_SH0_SHIFT)
#define TCR_SH0_OUTER		(2 << TCR_SH0_SHIFT)
#define TCR_SH0_INNER		(3 << TCR_SH0_SHIFT)

#define TCR_TG0_SHIFT       14
#define TCR_TG0_4KB         (0 << TCR_TG0_SHIFT)
#define TCR_TG0_64KB        (1 << TCR_TG0_SHIFT)
#define TCR_TG0_16KB        (2 << TCR_TG0_SHIFT)

#define TCR_IRGN1_SHIFT		24
#define TCR_IRGN1_NC		(0 << TCR_IRGN1_SHIFT)
#define TCR_IRGN1_WBWA		(1 << TCR_IRGN1_SHIFT)
#define TCR_IRGN1_WT		(2 << TCR_IRGN1_SHIFT)
#define TCR_IRGN1_WBNWA		(3 << TCR_IRGN1_SHIFT)

#define TCR_ORGN1_SHIFT		26
#define TCR_ORGN1_NC		(0 << TCR_ORGN1_SHIFT)
#define TCR_ORGN1_WBWA		(1 << TCR_ORGN1_SHIFT)
#define TCR_ORGN1_WT		(2 << TCR_ORGN1_SHIFT)
#define TCR_ORGN1_WBNWA		(3 << TCR_ORGN1_SHIFT)

#define TCR_SH1_SHIFT		28
#define TCR_SH1_NON		    (0 << TCR_SH1_SHIFT)
#define TCR_SH1_OUTER		(2 << TCR_SH1_SHIFT)
#define TCR_SH1_INNER		(3 << TCR_SH1_SHIFT)

#define TCR_TG1_SHIFT       30
#define TCR_TG1_4KB         (0 << TCR_TG1_SHIFT)
#define TCR_TG1_64KB        (1 << TCR_TG1_SHIFT)
#define TCR_TG1_16KB        (2 << TCR_TG1_SHIFT)

#define TCR_IPS_SHIFT       32
#define TCR_IPS_64GB        (1UL << TCR_IPS_SHIFT)

#define TCR_A1			(1UL << 22)
#define TCR_ASID16		(1UL << 36)
#define TCR_TBI0		(1UL << 37)
#define TCR_TBI1		(1UL << 38)
#define TCR_HA			(1UL << 39)		/* FEAT_HAFDBS */

/* ID_AA64PFR0_EL1 */
#define ID_AA64PFR0_SVE_SHIFT	32
#define ID_AA64PFR0_NSVE		(0b0000)
#define ID_AA64PFR0_MASK		0xF


/* ID_AA64MMFR0_EL1 */
#define ID_AA64MMFR0_TGRAN4_2_SHIFT		40
#define ID_AA64MMFR0_TGRAN64_2_SHIFT	36
#define ID_AA64MMFR0_TGRAN16_2_SHIFT	32
#define ID_AA64MMFR0_TGRAN4_SHIFT		28
#define ID_AA64MMFR0_TGRAN64_SHIFT		24
#define ID_AA64MMFR0_TGRAN16_SHIFT		20
#define ID_AA64MMFR0_MASK		0xF

/* ID_AA64MMFR1_EL1 */
#define ID_AA64MMFR1_VH_SHIFT	8
#define ID_AA64MMFR1_VHE		(0b0001)
#define ID_AA64MMFR1_VMID_SHIFT	4
#define ID_AA64MMFR1_VMID_16BIT	(0b0001)
#define ID_AA64MMFR1_MASK		0xF



// #endif  /* __ASSEMBLY__ */

#endif  /* __LIB_HELPERS_H__ */