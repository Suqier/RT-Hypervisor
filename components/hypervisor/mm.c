/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-21     Suqier       first version
 */

#include "mm.h"

extern void *alloc_guest_pgd(void);

struct vm_area *vm_area_init(struct mm_struct *mm, 
                             rt_uint64_t start, rt_uint64_t end)
{
    struct vm_area *va = (struct vm_area *)rt_malloc(sizeof(struct vm_area));
    if (!va)
    {
        rt_kprintf("[Error] Allocate memory for vm_area failure.\n");
        return RT_NULL;
    }
    
    va->vm_area_desc.vaddr_start = start;
    va->vm_area_desc.vaddr_end = end;
    va->vm_area_desc.attr = 0;
    va->vm_mm = mm;

    return va;
}

rt_err_t vm_mm_struct_init(struct mm_struct *mm)
{
    mm->pgd_tbl = RT_NULL;
    rt_list_init(&mm->vm_area_used);
    rt_hw_spin_lock_init(&mm->lock);

    mm->pgd_tbl = (pud_t *)alloc_guest_pgd();
    if (!mm->pgd_tbl)
    {
        rt_kprintf("[Error] Allocate memory for VM pgd table.\n");
        return -RT_ENOMEM;
    }
    
    rt_uint64_t va_start = 0UL, va_end = (1UL << 40);
    struct vm_area *va = vm_area_init(mm, va_start, va_end);
    if (!va)
        return -RT_ENOMEM;
    else
        rt_list_insert_after(&mm->vm_area_used, va->node);
    
    return RT_EOK;
}



