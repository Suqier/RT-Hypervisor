/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-19     Suqier       first version
 */

#ifndef __STAGE2_H__
#define __STAGE2_H__

#include <rtdef.h>
#include <rtthread.h>

#include "lib_helpers.h"
#include "virt.h"

/* 
 * Controlling address translation stage 2 
 * 
 * Translation stage    : Non-secure EL1&0 stage 2
 * Controlled from      : Non-secure EL2 
 * Controlling registers: SCTLR_EL2.EE, HCR_EL2.VM
 *                        VTCR_EL2, VTTBR_EL2
 */

/* 
 * VTCR_EL2 Registers bits 
 * The control register for stage 2 of the EL1&0 translation regime.
 * Stage 2 translation is not only for VHE. NVHE is also need this.
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

/* P2751-D5.3.3 Attribute fields in stage 2 VMSAv8-64 Block and Page descriptors */
/* Upper attr [63:51] */
/* FEAT_XNX support */
#define S2_XN_EL01  (0b00UL << 53)   
#define S2_XN_EL0   (0b01UL << 53)
#define S2_XN_NONE  (0b10UL << 53)
#define S2_XN_EL1   (0b11UL << 53)

/* Lower attr [11: 2] */
#define S2_AF       (1 << 10)
#define S2_SH_IS    (0b11 << 8)
#define S2_AP_RO	(0b01 << 6)
#define S2_AP_RW	(0b11 << 6)

#define S2_MEMATTR_DEV_nGnRnE	(0b0000 << 2)
#define S2_MEMATTR_DEV_nGnRE	(0b0001 << 2)
#define S2_MEMATTR_DEV_nGRE		(0b0010 << 2)
#define S2_MEMATTR_DEV_GRE		(0b0011 << 2)
#define S2_MEMATTR_NORMAL_WB    (0b1111 << 2)
#define S2_MEMATTR_NORMAL_NC	(0b0101 << 2)
#define S2_MEMATTR_NORMAL_WT	(0b1010 << 2)

#define S2_BLOCK_NORMAL     (MMU_TYPE_BLOCK | S2_AF | S2_SH_IS | S2_AP_RW | S2_MEMATTR_NORMAL_WB | S2_XN_EL01)
#define S2_BLOCK_NORMAL_NC	(MMU_TYPE_BLOCK | S2_AF | S2_SH_IS | S2_AP_RW | S2_MEMATTR_NORMAL_NC | S2_XN_EL01)
#define S2_BLOCK_NORMAL_WT	(MMU_TYPE_BLOCK | S2_AF | S2_SH_IS | S2_AP_RW | S2_MEMATTR_NORMAL_WT | S2_XN_EL01)
#define S2_BLOCK_DEVICE	    (MMU_TYPE_BLOCK | S2_AF | S2_MEMATTR_DEV_nGnRnE | S2_XN_EL1)

#define S2_PAGE_NORMAL      (MMU_TYPE_PAGE | S2_AF | S2_SH_IS | S2_AP_RW | S2_MEMATTR_NORMAL_WB | S2_XN_EL01)
#define S2_PAGE_NORMAL_NC	(MMU_TYPE_PAGE | S2_AF | S2_SH_IS | S2_AP_RW | S2_MEMATTR_NORMAL_NC | S2_XN_EL01)
#define S2_PAGE_NORMAL_WT	(MMU_TYPE_PAGE | S2_AF | S2_SH_IS | S2_AP_RW | S2_MEMATTR_NORMAL_WT | S2_XN_EL01)
#define S2_PAGE_DEVICE	    (MMU_TYPE_PAGE | S2_AF | S2_MEMATTR_DEV_nGnRnE | S2_XN_EL1)

/* 
 * IPA supports 40 bits, and translation staring at level 1.
 * IA = [39:12]
 * level     bits     width
 *   1     [39:30]      10
 *   2     [29:21]      9
 *   3     [20:12]      9
 *  PAGE   [11: 0]      12
 */
#define S2_VA_MASK      	(0x0000fffffffff000UL)

#define S2_VA_SHIFT         (48)
#define S2_VA_SIZE          (1UL << S2_VA_SHIFT)
#define S2_IPA_SHIFT        (40)
#define S2_IPA_SIZE         (1UL << S2_IPA_SHIFT)
#define S2_PA_SHIFT         (40)
#define S2_PA_SIZE          (1UL << S2_PA_SHIFT)

#define S2_L1_SHIFT			(30)
#define S2_L1_SIZE			(1UL << S2_L1_SHIFT)
#define S2_L1_MASK			(~(S2_L1_SIZE - 1))
#define S2_L2_SHIFT			(21)
#define S2_L2_SIZE			(1UL << S2_L2_SHIFT)
#define S2_L2_MASK			(~(S2_L2_SIZE - 1))
#define S2_L3_SHIFT			(12)
#define S2_L3_SIZE			(1UL << S2_L3_SHIFT)
#define S2_L3_MASK			(~(S2_L3_SIZE - 1))

#define S2_PAGETABLE_SIZE	RT_MM_PAGE_SIZE * 2  /* two pages concatenated */
#define S2_L1_NUM	    	S2_PAGETABLE_SIZE / (sizeof(rt_uint64_t))
#define S2_L2_NUM		    RT_MM_PAGE_SIZE   / (sizeof(rt_uint64_t))
#define S2_L3_NUM			RT_MM_PAGE_SIZE   / (sizeof(rt_uint64_t))

#define IS_1G_ALIGN(x)      (!((rt_uint64_t)(x) & (S2_L1_SIZE - 1)))
#define IS_2M_ALIGN(x)	    (!((rt_uint64_t)(x) & (S2_L2_SIZE - 1)))
#define IS_4K_ALIGN(x)	    (!((rt_uint64_t)(x) & (S2_L3_SIZE - 1)))

#define S2_L1_IDX(va)      (((va) >> S2_L1_SHIFT) & (S2_L1_NUM - 1))
#define S2_L2_IDX(va)      (((va) >> S2_L2_SHIFT) & (S2_L2_NUM - 1))
#define S2_L3_IDX(va)      (((va) >> S2_L3_SHIFT) & (S2_L3_NUM - 1)) /* [20:12] */

#define S2_L1_OFFSET(l1_tbl, va)   ((l1_t *)(l1_tbl) + S2_L1_IDX((rt_uint64_t)va))
#define S2_L2_OFFSET(l2_tbl, va)   ((l2_t *)(l2_tbl) + S2_L2_IDX((rt_uint64_t)va))
#define S2_L3_OFFSET(l3_tbl, va)   ((l3_t *)(l3_tbl) + S2_L3_IDX((rt_uint64_t)va))

#define TABLE_ADDR_MASK    (0xFFFFFFFFF000UL)  /* [47:12] */
#define L1_BLOCK_OA_MASK   (0xFFFFC0000000UL)  /* [47:30] */
#define L2_BLOCK_OA_MASK   (0xFFFFFFF00000UL)  /* [47:21] */

#define WRITE_ONCE(x, val)    *(volatile typeof(x) *)&(x) = (val);

struct mm_struct;
struct mem_desc;

void *alloc_vm_pgd(void);
rt_err_t s2_map(struct mm_struct *mm, struct mem_desc *desc);
rt_err_t s2_unmap(struct mm_struct *mm, rt_ubase_t va, rt_ubase_t va_end);
rt_err_t s2_translate(struct mm_struct *mm, rt_uint64_t va, rt_ubase_t *pa);

#endif  /* __STAGE2_H__ */