/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-21     Suqier       first version
 */

#include "virt.h"

static void __flush_all_tlb(void)
{
	asm volatile(
		"dsb sy;"
		"tlbi vmalls12e1is;"
		"dsb sy;"
		"isb;"
		: : : "memory"
	);
}

void flush_guest_all_tlb(struct mm_struct *mm)
{
    struct vm* vm = rt_container_of(mm, struct vm, mm);
    rt_uint64_t vttbr = (*(mm->pgd_tbl) & VA_MASK) 
                      | (rt_uint64_t)vm->vm_idx << VMID_SHIFT;
    
    rt_hw_spin_lock(&mm->lock);
    SET_SYS_REG(VTTBR_EL2, vttbr);
    __flush_all_tlb();
    rt_hw_spin_unlock(&mm->lock);
}