/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-05-28     Suqier       first version
 */

#ifndef __VHE_H__
#define __VHE_H__

#include <armv8.h>
#include <lib_helpers.h>

// #if defined(RT_HYPERVISOR) && defined(ARCH_FEAT_VHE)

rt_bool_t arm_vhe_supported(void);
rt_int8_t arm_vmid_bits(void);
rt_bool_t arm_sve_supported(void);

/* HCR_EL2 Registers bits */
#define HCR_E2H     (1UL << 34)
#define HCR_TRVW    (1UL << 30)
#define HCR_HCD     (1UL << 29)
#define HCR_TGE     (1UL << 27)
#define HCR_TVM     (1UL << 26)
#define	HCR_TSW		(1UL << 22)
#define HCR_TAC		(1UL << 21)
#define	HCR_TIDCP	(1UL << 20)
#define HCR_TSC     (1UL << 19)
#define HCR_TWE     (1UL << 14)
#define HCR_TWI     (1UL << 13)
#define	HCR_BSU		(3UL << 10)
#define	HCR_FB		(1UL << 9)
#define HCR_VSE     (1UL << 8)
#define HCR_VI      (1UL << 7)
#define HCR_VF      (1UL << 6)
#define HCR_AMO     (1UL << 5)
#define HCR_IMO     (1UL << 4)
#define HCR_FMO     (1UL << 3)
#define HCR_VM      (1UL << 0)

#define HCR_GUEST_FLAGS		(HCR_TSC | HCR_TSW | HCR_TWE | HCR_TWI |\
							 HCR_VM  | HCR_TVM | HCR_BSU | HCR_FB  | \
							 HCR_TAC | HCR_AMO | HCR_SWIO| HCR_TIDCP | HCR_RW)
#define HCR_VIRT_EXCP_MASK 	(HCR_VSE | HCR_VI  | HCR_VF)
#define HCR_INT_OVERRIDE   	(HCR_FMO | HCR_IMO)
#define HCR_HOST_VHE_FLAGS	(HCR_RW  | HCR_TGE | HCR_E2H)

/* CPTR_EL2 Registers bits */
#define CPTR_EL2_ZEN_VHE	((1 << 16) | (1 << 17))
#define CPTR_EL2_FPEN_VHE	((1 << 20) | (1 << 21))
#define CPTR_EL2_TTA_VHE	(1 << 28)
#define CPTR_EL2_TAM_VHE	(1 << 30)
#define CPTR_EL2_TCPAC_VHE	(1 << 31)

/* 
 * VTCR_EL2 Registers bits 
 * The control register for stage 2 of the EL1&0 translation regime.
 * 
 * If using rk3568, it's FEAT as follows:
 * ID_AA64MMFR0_EL1 = 0x101122
 * ID_AA64MMFR1_EL1 = 0x10212122
 * ID_AA64MMFR2_EL1 = 0x1011
 */
/* The size offset of the memory region addressed by VTTBR_EL2.*/
#define VTCR_EL2_T0SZ(x)	TCR_T0SZ(x)	
#define VTCR_EL2_T0SZ_MASK	(0b111111)

#define VTCR_EL2_RES0		((1UL << 20) | (3UL << 22) | (0x3FFFFFFFUL << 34))
#define VTCR_EL2_RES1		(1UL << 31)

#define VTCR_EL2_SL0_SHIFT	6

#define	VTCR_EL2_IRGN0_SHIFT	8
#define	VTCR_EL2_IRGN0_NC		(0 << VTCR_EL2_IRGN0_SHIFT)
#define	VTCR_EL2_IRGN0_WBWA		(1 << VTCR_EL2_IRGN0_SHIFT)
#define	VTCR_EL2_IRGN0_WT		(2 << VTCR_EL2_IRGN0_SHIFT)
#define	VTCR_EL2_IRGN0_WBNWA	(3 << VTCR_EL2_IRGN0_SHIFT)

#define	VTCR_EL2_ORGN0_SHIFT	10
#define	VTCR_EL2_ORGN0_NC		(0 << VTCR_EL2_ORGN0_SHIFT)
#define	VTCR_EL2_ORGN0_WBWA		(1 << VTCR_EL2_ORGN0_SHIFT)
#define	VTCR_EL2_ORGN0_WT		(2 << VTCR_EL2_ORGN0_SHIFT)
#define	VTCR_EL2_ORGN0_WBNWA	(3 << VTCR_EL2_ORGN0_SHIFT)

#define	VTCR_EL2_SH0_SHIFT	12
#define	VTCR_EL2_SH0_NON	(0 << VTCR_EL2_SH0_SHIFT)
#define	VTCR_EL2_SH0_OUTER	(2 << VTCR_EL2_SH0_SHIFT)
#define	VTCR_EL2_SH0_INNER	(3 << VTCR_EL2_SH0_SHIFT)

#define VTCR_EL2_TG0_SHIFT	14
#define VTCR_EL2_TG0_4KB	(0 << VTCR_EL2_TG0_SHIFT)
#define VTCR_EL2_TG0_64KB	(1 << VTCR_EL2_TG0_SHIFT)
#define VTCR_EL2_TG0_16KB	(2 << VTCR_EL2_TG0_SHIFT)

#define VTCR_EL2_PS_SHIFT	16
#define VTCR_EL2_PS_64GB	(1 << VTCR_EL2_PS_SHIFT)

#define VTCR_EL2_VS_SHIFT	19
#define VTCR_EL2_VS_8BIT	(0 << VTCR_EL2_VS_SHIFT)
#define VTCR_EL2_VS_16BIT	(1 << VTCR_EL2_VS_SHIFT)

/* Not use temporarily */
#define VTCR_EL2_HA			(1 << 21)
#define VTCR_EL2_HD			(1 << 22)

// #endif /* defined(RT_HYPERVISOR) && defined(ARCH_FEAT_VHE) */

#endif /* __VHE_H__ */