/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-21     Suqier       first version
 */

#include <rtdef.h>

#include "vm.h"
#include "mm.h"
#include "hyp_debug.h"

extern void *alloc_vm_pgd(rt_uint8_t vm_idx);

struct vm_area *vm_area_init(struct mm_struct *mm, rt_uint64_t start, rt_uint64_t end)
{
    struct vm_area *va = (struct vm_area *)rt_malloc(sizeof(struct vm_area));
    if (va == RT_NULL)
    {
        hyp_err("Allocate memory for vm_area failure");
        return RT_NULL;
    }

    va->desc.vaddr_start = start;
    va->desc.vaddr_end = end;
    va->desc.attr = 0UL;    /* for stage 2 translate */
    va->flag = 0UL;         /* for programmer manage */
    va->mm = mm;

    return va;
}

rt_err_t vm_mm_struct_init(struct mm_struct *mm)
{
    mm->pgd_tbl = RT_NULL;
    rt_list_init(&(mm->vm_area_used));

#ifdef RT_USING_SMP
    rt_hw_spin_lock_init(&mm->lock);
#endif
    vm_t vm = mm->vm;
    mm->pgd_tbl = (pud_t *)alloc_vm_pgd(vm->id);
    if (mm->pgd_tbl == RT_NULL)
        return -RT_ENOMEM;
    else
        mm->pgd_tbl = (pud_t *)(MMU_TYPE_TABLE 
                    | ((rt_uint64_t)mm->pgd_tbl & TABLE_ADDR_MASK));
    hyp_debug("mm->pgd_tbl&attr=0x%08x", mm->pgd_tbl);
    /* 
     * @TODO
     * We adjust ipa_start and ipa_end by getting OS img information.
     * Currently only supports a memory case.
     */
    rt_uint64_t ipa_start = vm->info.va_addr;
    rt_uint64_t ipa_end = ipa_start + vm->info.va_size;
    struct vm_area *va = vm_area_init(mm, ipa_start, ipa_end);
    if (!va)
        return -RT_ENOMEM;
    else
        rt_list_insert_after(&(mm->vm_area_used), &(va->node));

    hyp_info("%dth VM: Init mm_struct success", vm->id);
    return RT_EOK;
}

mem_block_t *alloc_mem_block(void)
{
    mem_block_t *mb = (mem_block_t *)rt_malloc(sizeof(mem_block_t));
    
    /* return phy addr. It must align. */
    mb->ptr = rt_malloc_align(MEM_BLOCK_SIZE, MEM_BLOCK_SIZE);  
    if (mb->ptr == RT_NULL)
    {
        hyp_err("Allocate mem_block failure");
        return RT_NULL;
    }
    mb->next = RT_NULL;
    return mb;
}

rt_err_t alloc_vm_memory(struct mm_struct *mm)
{
    vm_t vm = mm->vm;
    vm_area_t vma = rt_list_entry(mm->vm_area_used.next, struct vm_area, node);
    rt_uint64_t va_start = vma->desc.vaddr_start;
    mem_block_t *mb;

    rt_uint64_t start = RT_ALIGN_DOWN(va_start, MEM_BLOCK_SIZE);
    if (start != va_start)
    {
		hyp_err("MEM 0x%08x not mem_block aligned", va_start);
		return -RT_EINVAL;
    }

    vma->mb_head = RT_NULL;
    vma->flag |= VM_MAP_BK;
    rt_size_t count = mm->mem_size >> MEM_BLOCK_SHIFT;

    /* Allocate virtual memory from Host OS. Still not map memory yet. */
    for (rt_size_t i = 0; i < count; i++)
    {
        mb = alloc_mem_block();     /* mem_block size: 2M */
        if (mb == RT_NULL)
            return -RT_ENOMEM;
        
        mb->next = vma->mb_head;    /* head insert */
        vma->mb_head = mb;
        mm->mem_used += MEM_BLOCK_SIZE;
    }

    hyp_info("%dth VM: Alloc %d MB memory", vm->id, MB(mm->mem_used));
    
    return RT_EOK;
}

rt_err_t create_vm_mmap(struct mm_struct *mm, struct mem_desc *desc)
{
    rt_uint64_t mmap_size;
    rt_err_t ret;

#ifdef RT_USING_SMP
    rt_hw_spin_lock(&mm->lock);
#endif

    desc->vaddr_end = RT_ALIGN(desc->vaddr_end, MEM_BLOCK_SIZE);
    desc->vaddr_start = RT_ALIGN_DOWN(desc->vaddr_start, MEM_BLOCK_SIZE);
    mmap_size = desc->vaddr_end - desc->vaddr_start;
    if (mmap_size == 0)
    {
        hyp_err("Memory map size = 0");
        return -RT_EINVAL;    
    }

    /* map memory: build stage 2 page table and translate GPA to HPA */
    ret = s2_map(mm, desc);
    /*
     * @TODO 
     * if (ret == RT_EOK)
     * ret = iommu_iotlb_flush_all(vm);
     */

#ifdef RT_USING_SMP
    rt_hw_spin_unlock(&mm->lock);
#endif

    return ret;
}

/* 
 * According VM's memory type flag, map vm_area separately. 
 */
static rt_err_t map_vma_bk(struct mm_struct *mm, struct vm_area *vma)
{
    /* map normal memory, usually a large contiguous segment of memory */
    rt_err_t ret;
    mem_block_t *mb;
    struct mem_desc desc;

    mb = vma->mb_head;
    desc.vaddr_start = vma->desc.vaddr_start;
    desc.vaddr_end = desc.vaddr_start + MEM_BLOCK_SIZE;

    while (mb)
    {
        desc.paddr_start = (rt_uint64_t)mb->ptr;
        desc.attr = vma->desc.attr | S2_BLOCK_NORMAL;

        ret = create_vm_mmap(mm, &desc);
        if (ret)
            return ret;

        mb = mb->next;
        desc.vaddr_end += MEM_BLOCK_SIZE;
    }

    return RT_EOK;
}

static rt_err_t map_vma_pt(struct mm_struct *mm, struct vm_area *vma)
{
    /* IO memory pass through */
    vma->desc.attr |= S2_BLOCK_DEVICE;
    return create_vm_mmap(mm, &vma->desc);
}

/*
 * In mm_struct, we use rt_list_t vm_area_used to manage vm_area, 
 * but we currently only supports a memory case.
 * So there is only 1 normal memory segment 
 * and many device memory segment in this rt_list.
 */
rt_err_t map_vm_memory(struct mm_struct *mm)
{
    struct rt_list_node *pos;
    rt_err_t ret = RT_EOK;

    rt_list_for_each(pos, &mm->vm_area_used)
    {
        struct vm_area *vma = rt_list_entry(pos, struct vm_area, node);

        switch (vma->flag & VM_MAP_TYPE_MASK)
        {
        case VM_MAP_BK: /* normal memory */
            ret = map_vma_bk(mm, vma);
            break;

        case VM_MAP_PT: /* IO memory */
            vma->desc.paddr_start = vma->desc.vaddr_start;
            ret = map_vma_pt(mm, vma);
            break;

        default:
            vma->desc.paddr_start = 0;
            ret = map_vma_pt(mm, vma); 
            break;
        }
    }

    return ret;
}

rt_err_t vm_memory_init(struct mm_struct *mm)
{
    rt_err_t ret;

    /* allocate memory */
    ret = alloc_vm_memory(mm);
    if (ret)
        return ret;

    /* memory map */
    ret = map_vm_memory(mm);
    if (ret)
        return ret;
    
    return RT_EOK;
}
