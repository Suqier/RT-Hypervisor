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

#include "virt.h"

/* 
 * Controlling address translation stage 2 
 * 
 * Translation stage    : Non-secure EL1&0 stage 2
 * Controlled from      : Non-secure EL2 
 * Controlling registers: SCTLR_EL2.EE, HCR_EL2.VM
 *                        VTCR_EL2, VTTBR_EL2
 */

/* Attribute fields in stage 2 VMSAv8-64 Block and Page descriptors */
/* Upper attr */
#define S2_XN_SHIFT        (53)
#define S2_XN_EL0_EL1      (0b00 << S2_XN_SHIFT)
#define S2_XN_EL0          (0b01 << S2_XN_SHIFT)
#define S2_XN_NONE         (0b10 << S2_XN_SHIFT)
#define S2_XN_EL1          (0b11 << S2_XN_SHIFT)

#define S2_CONTIGUOUS      (1UL << 52)
#define S2_DBM             (1UL << 51)

/* Lower attr */
#define S2_NT              (1 << 16)
#define S2_AF              (1 << 10)

#define S2_SH_SHIFT        (8)
#define S2_SH_NON          (0b00 << S2_SH_SHIFT)
#define S2_SH_OUTER        (0b10 << S2_SH_SHIFT)
#define S2_SH_INNER        (0b11 << S2_SH_SHIFT)

#define S2_AP_SHIFT        (6)
#define S2_AP_NON	       (0b00 << S2_AP_SHIFT)
#define S2_AP_RO	       (0b01 << S2_AP_SHIFT)
#define S2_AP_WO		   (0b10 << S2_AP_SHIFT)
#define S2_AP_RW		   (0b11 << S2_AP_SHIFT)

#define S2_MEMATTR_SHIFT        (2)
#define S2_MEMATTR_DEV_nGnRnE	(0b0000 << S2_MEMATTR_SHIFT)
#define S2_MEMATTR_DEV_nGnRE	(0b0001 << S2_MEMATTR_SHIFT)
#define S2_MEMATTR_DEV_nGRE		(0b0010 << S2_MEMATTR_SHIFT)
#define S2_MEMATTR_DEV_GRE		(0b0011 << S2_MEMATTR_SHIFT)
#define S2_MEMATTR_NORMAL_WB    (0b1111 << S2_MEMATTR_SHIFT)
#define S2_MEMATTR_NORMAL_NC	(0b0101 << S2_MEMATTR_SHIFT)
#define S2_MEMATTR_NORMAL_WT	(0b1010 << S2_MEMATTR_SHIFT)

#define S2_BLOCK_NORMAL			(MMU_TYPE_BLOCK | S2_AF | S2_SH_INNER | S2_MEMATTR_NORMAL_WB)
#define S2_BLOCK_NORMAL_NC		(MMU_TYPE_BLOCK | S2_AF | S2_SH_INNER | S2_MEMATTR_NORMAL_NC)
#define S2_BLOCK_NORMAL_WT	    (MMU_TYPE_BLOCK | S2_AF | S2_SH_INNER | S2_MEMATTR_NORMAL_WT)
#define S2_BLOCK_DEVICE			(MMU_TYPE_BLOCK | S2_AF | S2_MEMATTR_DEV_nGnRnE | S2_XN_EL1)

#define S2_PAGE_NORMAL			(MMU_TYPE_PAGE | S2_AF | S2_SH_INNER | S2_MEMATTR_NORMAL_WB)
#define S2_PAGE_NORMAL_NC		(MMU_TYPE_PAGE | S2_AF | S2_SH_INNER | S2_MEMATTR_NORMAL_NC)
#define S2_PAGE_NORMAL_WT	    (MMU_TYPE_PAGE | S2_AF | S2_SH_INNER | S2_MEMATTR_NORMAL_WT)
#define S2_PAGE_DEVICE			(MMU_TYPE_PAGE | S2_AF | S2_MEMATTR_DEV_nGnRnE | S2_XN_EL1)

/* 
 * IPA supports 40 bits, and translation staring at level 1.
 * IA = [39:12]
 * level     bits     width
 *   1     [39:30]      10
 *   2     [29:21]      9
 *   3     [20:12]      9
 *  PAGE   [11: 0]      12
 */
#define S2_START_LEVEL          (1)
#define S2_PAGE_TABLE_LEVELS    (3)

#define S2_VA_SHIFT             (48)
#define S2_VA_SIZE              (1UL << S2_VA_SHIFT)
#define S2_VA_MASK      		(0x0000fffffffff000UL)
#define S2_IPA_SHIFT            (40)
#define S2_IPA_SIZE             (1UL << S2_IPA_SHIFT)
#define S2_PA_SHIFT             (36)
#define S2_PA_SIZE              (1UL << S2_PA_SHIFT)

#define S2_PGD_SHIFT			(39)
#define S2_PGD_SIZE			    (1UL << S2_PGD_SHIFT)
#define S2_PGD_MASK			    (~(S2_PGD_SIZE - 1))

#define S2_PUD_SHIFT			(30)
#define S2_PUD_SIZE			    (1UL << S2_PUD_SHIFT)
#define S2_PUD_MASK			    (~(S2_PUD_SIZE - 1))

#define S2_PMD_SHIFT			(21)
#define S2_PMD_SIZE			    (1UL << S2_PMD_SHIFT)
#define S2_PMD_MASK			    (~(S2_PMD_SIZE - 1))

#define S2_PTE_SHIFT			(12)
#define S2_PTE_SIZE			    (1UL << S2_PTE_SHIFT)
#define S2_PTE_MASK			    (~(S2_PTE_SIZE - 1))

#define S2_PAGETABLE_SIZE		RT_MM_PAGE_SIZE * 2  /* two pages concatenated */

#define S2_PGD_NUM  			RT_MM_PAGE_SIZE / (sizeof(rt_uint64_t))
#define S2_PUD_NUM	    		S2_PAGETABLE_SIZE / (sizeof(rt_uint64_t))
#define S2_PMD_NUM		    	RT_MM_PAGE_SIZE / (sizeof(rt_uint64_t))
#define S2_PTE_NUM			    RT_MM_PAGE_SIZE / (sizeof(rt_uint64_t))

#define IS_PUD_ALIGN(va)    (!((rt_uint64_t)(va) & (S2_PUD_SIZE - 1)))
#define IS_BLOCK_ALIGN(x)	(!((rt_uint64_t)(x) & (0x1fffffUL)))
#define IS_PAGE_ALIGN(x)	(!((rt_uint64_t)(x) & (RT_MM_PAGE_SIZE - 1)))

#define S2_PGD_IDX(va)      (((va) >> S2_PGD_SHIFT) & (S2_PGD_NUM - 1))
#define S2_PUD_IDX(va)      (((va) >> S2_PUD_SHIFT) & (S2_PUD_NUM - 1))
#define S2_PMD_IDX(va)      (((va) >> S2_PMD_SHIFT) & (S2_PMD_NUM - 1))
#define S2_PTE_IDX(va)      (((va) >> S2_PTE_SHIFT) & (S2_PTE_NUM - 1)) /* [20:12] */

#define S2_PGD_OFFSET(pgd_ptr, va)   ((pgd_t *)(pgd_ptr) + S2_PGD_IDX((rt_uint64_t)va))
#define S2_PUD_OFFSET(pud_ptr, va)   ((pud_t *)(pud_ptr) + S2_PUD_IDX((rt_uint64_t)va))
#define S2_PMD_OFFSET(pmd_ptr, va)   ((pmd_t *)(pmd_ptr) + S2_PMD_IDX((rt_uint64_t)va))
#define S2_PTE_OFFSET(pte_ptr, va)   ((pte_t *)(pte_ptr) + S2_PTE_IDX((rt_uint64_t)va))

#define NEXT_LEVEL_ADDR_MASK    (0xffffffff0000UL)

#define WRITE_ONCE(x, val)    *(volatile typeof(x) *)&(x) = (val);

void *alloc_guest_pgd(void);
rt_err_t stage2_translate(struct mm_struct *mm, rt_uint64_t va, rt_ubase_t *pa);
rt_err_t stage2_map(struct mm_struct *mm, struct mem_desc *desc);
rt_err_t stage2_unmap(struct mm_struct *mm, rt_ubase_t va, rt_ubase_t va_end);


#endif  /* __STAGE2_H__ */