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

#if defined(RT_HYPERVISOR)

rt_bool_t arm_vhe_supported(void);
rt_int8_t arm_vmid_bits(void);
rt_bool_t arm_sve_supported(void);

/* HCR_EL2 Registers bits */
#define HCR_TLOR    (1UL << 35)
#define HCR_E2H     (1UL << 34)
#define HCR_TRVW    (1UL << 30)
#define HCR_HCD     (1UL << 29)
#define HCR_TGE     (1UL << 27)
#define HCR_TVM     (1UL << 26)
#define	HCR_TSW		(1UL << 22)
#define HCR_TACR	(1UL << 21)
#define	HCR_TIDCP	(1UL << 20)
#define HCR_TSC     (1UL << 19)
#define HCR_TWE     (1UL << 14)
#define HCR_TWI     (1UL << 13)
#define	HCR_BSU		(3UL << 10)
#define	HCR_BSU_IS	(1UL << 10)
#define	HCR_FB		(1UL << 9)
#define HCR_VSE     (1UL << 8)
#define HCR_VI      (1UL << 7)
#define HCR_VF      (1UL << 6)
#define HCR_AMO     (1UL << 5)
#define HCR_IMO     (1UL << 4)
#define HCR_FMO     (1UL << 3)
#define HCR_PTW     (1UL << 2)
#define HCR_VM      (1UL << 0)

/*
 * The bits we set in HCR:
 * RW:		64bit by default, can be overridden for 32bit VMs
 * TACR:	Trap ACTLR
 * TSC:		Trap SMC
 * TSW:		Trap cache operations by set/way
 * TWE:		Trap WFE
 * TWI:		Trap WFI
 * TIDCP:	Trap L2CTLR/L2ECTLR
 * BSU_IS:	Upgrade barriers to the inner shareable domain
 * FB:		Force broadcast of all maintenance operations
 * AMO:		Override CPSR.A and enable signaling with VA
 * IMO:		Override CPSR.I and enable signaling with VI
 * FMO:		Override CPSR.F and enable signaling with VF
 * SWIO:	Turn set/way invalidates into set/way clean+invalidate
 * PTW:		Take a stage2 fault if a stage1 walk steps in device memory

#define HCR_GUEST_FLAGS (HCR_TSC | HCR_TSW | HCR_TWE | HCR_TWI | HCR_VM | \
			             HCR_BSU_IS | HCR_FB | HCR_TACR | \
			             HCR_AMO | HCR_SWIO | HCR_TIDCP | HCR_RW | \
			             HCR_FMO | HCR_IMO | HCR_PTW)
 */
#define HCR_GUEST_FLAGS (HCR_TSC | HCR_IMO | HCR_FMO | HCR_VM | HCR_RW)

#define HCR_VIRT_EXCP_MASK (HCR_VSE | HCR_VI | HCR_VF)
#define HCR_HOST_VHE_FLAGS (HCR_RW | HCR_TGE | HCR_E2H)

/* CPTR_EL2 Registers bits */
#define CPTR_EL2_TCPAC  	(1 << 31)
#define CPTR_EL2_TAM    	(1 << 30)
#define CPTR_EL2_TTA    	(1 << 28)
#define CPTR_EL2_FPEN   	((1 << 20) | (1 << 21))
#define CPTR_EL2_ZEN    	((1 << 16) | (1 << 17))
#define CPTR_EL2_DEFAULT    0x000032ff

/* 
 * VTCR_EL2 Registers bits 
 * The control register for stage 2 of the EL1&0 translation regime.
 */
#define VTCR_EL2_RES1       (1UL << 31)

#define VTCR_EL2_16_VMID    (1UL << 19)
#define VTCR_EL2_8_VMID     (0UL << 19)

#define VTCR_EL2_PS_40_BIT  (0b010 << 16)

#define VTCR_EL2_TG0_4KB	(0 << 14)
#define VTCR_EL2_TG0_64KB	(1 << 14)
#define VTCR_EL2_TG0_16KB	(2 << 14)

#define	VTCR_EL2_SH0_NON	(0 << 12)
#define	VTCR_EL2_SH0_OUTER	(2 << 12)
#define	VTCR_EL2_SH0_INNER	(3 << 12)

#define	VTCR_EL2_ORGN0_NC		(0 << 10)
#define	VTCR_EL2_ORGN0_WBWA		(1 << 10)
#define	VTCR_EL2_ORGN0_WT		(2 << 10)
#define	VTCR_EL2_ORGN0_WBNWA	(3 << 10)

#define	VTCR_EL2_IRGN0_NC		(0 << 8)
#define	VTCR_EL2_IRGN0_WBWA		(1 << 8)
#define	VTCR_EL2_IRGN0_WT		(2 << 8)
#define	VTCR_EL2_IRGN0_WBNWA	(3 << 8)

#define VTCR_EL2_SL0_4KB_LEVEL1	(0b01 << 6)

#define VTCR_EL2_T0SZ(x)	TCR_T0SZ(x)
#define VTCR_EL2_T0SZ_MASK	(0b111111)

#define VTCR_EL2_COMMON_BITS	(VTCR_EL2_SH0_INNER  | VTCR_EL2_ORGN0_WBWA | \
                                 VTCR_EL2_IRGN0_WBWA | VTCR_EL2_RES1)

#endif /* defined(RT_HYPERVISOR) */

#endif /* __VHE_H__ */