/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-01     Suqier       first version
 */

#ifndef __MM_H__
#define __MM_H__

// #if defined(RT_HYPERVISOR) 

#include <rthw.h>
#include <mmu.h>

typedef rt_uint64_t pgd_t;
typedef rt_uint64_t pud_t;
typedef rt_uint64_t pmd_t;
typedef rt_uint64_t pte_t;

struct mem_block
{
    rt_uint32_t bfn;
    struct mem_block *next;
};

struct vm_area
{
    struct mem_desc vm_area_desc;
	struct mem_block *mb_head;

    struct rt_list_node *node;
    struct mm_struct *vm_mm;    /* this area belongs to */
};

struct mm_struct
{
    rt_uint64_t mem_size;
    rt_uint64_t mem_used;

    pgd_t *pgd_tbl;

// #ifdef RT_USING_SMP
    rt_hw_spinlock_t lock;
// #endif
    
    rt_list_t vm_area_used;
};

struct vm_area *vm_area_init(struct mm_struct *mm, 
                             rt_uint64_t start, rt_uint64_t end);
rt_err_t vm_mm_struct_init(struct mm_struct *mm);

// #endif  /* defined(RT_HYPERVISOR) */ 

#endif  /* __MM_H__ */