/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-19     Suqier       first version
 */

#include "stage2.h"

rt_inline void s2_clear_pgd(pgd_t *pgd_ptr)
{
    WRITE_ONCE(*pgd_ptr, 0);
}

rt_inline void s2_clear_pud(pud_t *pud_ptr)
{
    WRITE_ONCE(*pud_ptr, 0);
}

rt_inline void s2_clear_pmd(pmd_t *pmd_ptr)
{
    WRITE_ONCE(*pmd_ptr, 0);
}

rt_inline void s2_clear_pte(pte_t *pte_ptr)
{
    WRITE_ONCE(*pte_ptr, 0);
}

rt_inline void s2_set_pgd(pte_t *pgd_ptr, pgd_t pgd_val)
{
    WRITE_ONCE(*pgd_ptr, pgd_val);
}

rt_inline void s2_set_pud(pud_t *pud_ptr, pud_t pud_val)
{
    WRITE_ONCE(*pud_ptr, pud_val);
}

rt_inline void s2_set_pmd(pmd_t *pmd_ptr, pmd_t pmd_val)
{
    WRITE_ONCE(*pmd_ptr, pmd_val);
}

rt_inline void s2_set_pte(pte_t *pte_ptr, pte_t pte_val)
{
    WRITE_ONCE(*pte_ptr, pte_val);
}

/* 
 * s2_map_pmd_huge
 * check weather mmap type is memory block
 */
static rt_err_t s2_map_pmd_huge(pmd_t *pmd_ptr, struct mem_desc *desc)
{
    if ((*pmd_ptr))
        return -RT_ERROR;

    if ((desc->attr & MMU_TYPE_MASK) != MMU_TYPE_BLOCK)
        return -RT_ERROR;

    if (!IS_BLOCK_ALIGN(desc->vaddr_start) 
     || !IS_BLOCK_ALIGN(desc->vaddr_end - desc->vaddr_start))
        return -RT_ERROR;
    
    return RT_EOK;
}

static rt_err_t s2_map_pud_huge(pud_t *pud_ptr, struct mem_desc *desc)
{
    if ((*pud_ptr))
        return -RT_ERROR;

    if ((desc->attr & MMU_TYPE_MASK) != MMU_TYPE_BLOCK)
        return -RT_ERROR;

    if (!IS_PUD_ALIGN(desc->vaddr_start) 
     || !IS_PUD_ALIGN(desc->vaddr_end - desc->vaddr_start))
        return -RT_ERROR;
    
    return RT_EOK;
}

static void s2_map_pte(pte_t *pte_tbl, struct mem_desc *desc)
{
    pte_t *pte_ptr;
    rt_uint64_t pte_attr;

    pte_ptr = S2_PTE_OFFSET(pte_tbl, desc->vaddr_start);
    pte_attr = desc->attr;  /* S2_PAGE_NORMAL, S2_PAGE_DEVICE and so on.*/

    do
    {
        if (!(*pte_ptr))
        {
            s2_set_pte(pte_ptr, desc->paddr_start | pte_attr);
            desc->vaddr_start += RT_MM_PAGE_SIZE;
            desc->paddr_start += RT_MM_PAGE_SIZE;
        }

        pte_ptr++;
    } while (desc->vaddr_start != desc->vaddr_end);
}

static rt_err_t s2_map_pmd(pmd_t *pmd_tbl, struct mem_desc *desc)
{
    pmd_t *pmd_ptr;
    pte_t *pte_tbl;

    rt_uint64_t next, size;

    pmd_ptr = S2_PMD_OFFSET(pmd_tbl, desc->vaddr_start);
    do
    {
        next = (desc->vaddr_start + S2_PMD_SIZE) & S2_PMD_MASK;
        if (next > desc->vaddr_end)
            next = desc->vaddr_end;

        size = next - desc->vaddr_start;

        if (s2_map_pmd_huge(pmd_ptr, desc) == RT_EOK)
        {
            rt_uint64_t pmd_val = desc->attr | (desc->paddr_start & S2_PMD_MASK);
            rt_kprintf("[Info]  desc->attr = 0x%16.16p\n", desc->attr);
            rt_kprintf("[Info] S2 map 2M pa&attr=0x%016x at pmd_ptr=0x%016x\n", pmd_val, pmd_ptr);
            s2_set_pmd(pmd_ptr, pmd_val);
        }
        else
        {
            if (*pmd_ptr)
            {
                /* get next level pte */
                pte_tbl = (pte_t *)(*pmd_ptr & TABLE_ADDR_MASK);
            }
            else
            {
                /* populate first pte */
                pte_tbl = (pte_t *)rt_malloc_align(RT_MM_PAGE_SIZE, RT_MM_PAGE_SIZE);
                if (pte_tbl == RT_NULL)
                    return -RT_ENOMEM;

                rt_memset(pte_tbl, 0, RT_MM_PAGE_SIZE);
                rt_uint64_t pte_val = MMU_TYPE_TABLE | ((rt_uint64_t)pte_tbl & TABLE_ADDR_MASK);
                rt_kprintf("[Info] pte&attr = 0x%016x\n", pte_val);
                s2_set_pmd(pmd_ptr, pte_val);
            }

            s2_map_pte(pte_tbl, desc);
        }
    } while (pmd_ptr++, 
             desc->paddr_start += size, 
             desc->vaddr_start  = next, 
             desc->vaddr_start != desc->vaddr_end);

    return RT_EOK;
}

static rt_err_t s2_map_pud(pud_t *pud_tbl, struct mem_desc *desc)
{
    pud_t *pud_ptr;
    pmd_t *pmd_tbl;

    rt_uint64_t next, size;

    pud_ptr = S2_PUD_OFFSET(pud_tbl, desc->vaddr_start);
    do
    {
        next = (desc->vaddr_start + S2_PUD_SIZE) & S2_PUD_MASK;
        if (next > desc->vaddr_end)
            next = desc->vaddr_end;

        size = next - desc->vaddr_start;

        /* map 1G memory */
        if (s2_map_pud_huge(pud_ptr, desc) == RT_EOK)
        {
            rt_kprintf("[Info] S2 map 1G at pud_ptr=0x%08x, pa=0x%08x\n", 
                        pud_ptr, desc->attr | (desc->paddr_start & S2_VA_MASK));
            s2_set_pud(pud_ptr, desc->attr | (desc->paddr_start & S2_VA_MASK));
        }
        else
        {
            if (*pud_ptr)
            {
                /* get next level pmd */
                pmd_tbl = (pmd_t *)(*pud_ptr & TABLE_ADDR_MASK);
            }
            else
            {
                /* 
                 * populate first pmd 
                 * page table type must align to RT_MM_PAGE_SIZE[4096]
                 */
                pmd_tbl = (pmd_t *)rt_malloc_align(RT_MM_PAGE_SIZE, RT_MM_PAGE_SIZE);
                if (pmd_tbl == RT_NULL)
                    return -RT_ENOMEM;
                rt_memset(pmd_tbl, 0, RT_MM_PAGE_SIZE);
                rt_uint64_t pud_val = MMU_TYPE_TABLE | ((rt_uint64_t)pmd_tbl & TABLE_ADDR_MASK);
                rt_kprintf("[Info] map pud&attr=0x%016x at pud_ptr=0x%016x\n", pud_val, pud_ptr);
                s2_set_pud(pud_ptr, pud_val);
            }

            s2_map_pmd(pmd_tbl, desc);
        }
    } while (pud_ptr++, 
             desc->paddr_start += size, 
             desc->vaddr_start  = next, 
             desc->vaddr_start != desc->vaddr_end);

    return RT_EOK;    
}

static void s2_unmap_pte(pte_t *pte_tbl, rt_ubase_t va, rt_ubase_t va_end)
{
    pte_t *pte_ptr = S2_PTE_OFFSET(pte_tbl, va);

    do
    {   
        pte_t *pte_tmp = pte_ptr;
        pte_ptr++;

        if (*pte_tmp)
        {
            s2_clear_pte(pte_tmp);
            rt_free(pte_tmp);
        }
    } while (va += RT_MM_PAGE_SIZE, va != va_end);
}

static void s2_unmap_pmd(pmd_t *pmd_tbl, rt_ubase_t va, rt_ubase_t va_end)
{
    pmd_t *pmd_ptr = S2_PMD_OFFSET(pmd_tbl, va);
    pte_t *pte_tbl = RT_NULL;
    rt_uint64_t next;

    do
    {
        next = (va + S2_PMD_SIZE) & S2_PMD_MASK;
        if (next > va_end)
            next = va_end;

        if (*pmd_ptr)
        {
            if ((*pmd_ptr & MMU_TYPE_MASK) == MMU_TYPE_BLOCK)
            {
                s2_clear_pmd(pmd_ptr);
                rt_free(pmd_ptr);
            }
            else
            {
                pte_tbl = (pte_t *)(*pmd_ptr & TABLE_ADDR_MASK);
                s2_unmap_pte(pte_tbl, va, next);
                if (((va & ~S2_PMD_MASK) == 0) && ((va_end - va) == S2_PMD_SIZE))
                {
                    s2_clear_pmd(pmd_ptr);
                    rt_free(pmd_ptr);
                    rt_free(pte_tbl);
                }
            }
        }
    } while (pmd_ptr++, va = next, va != va_end);
}

static rt_err_t s2_unmap_pud(struct mm_struct *mm, rt_ubase_t va, rt_ubase_t va_end)
{
    pud_t *pud_ptr = S2_PUD_OFFSET(mm->pgd_tbl, va);
    pmd_t *pmd_tbl;
    rt_uint64_t next;

    do
    {
        next = (va + S2_PUD_SIZE) & S2_PUD_MASK;
        if (next > va_end)
            next = va_end;

        if (*pud_ptr)
        {
            pmd_tbl = (pmd_t *)(*pud_ptr & TABLE_ADDR_MASK);
            s2_unmap_pmd(pmd_tbl, va, next);
            if (((va & ~S2_PUD_MASK) == 0) && ((va_end - va) == S2_PUD_SIZE))
            {
                s2_clear_pud(pud_ptr);
                rt_free(pud_ptr);
                rt_free(pmd_tbl);
            }
        }
    } while (pud_ptr++, va = next, va != va_end);

    flush_vm_all_tlb(mm->vm);
    return RT_EOK;
}

rt_err_t stage2_translate(struct mm_struct *mm, rt_uint64_t va, rt_ubase_t *pa)
{
    rt_uint64_t pte_offset = va & ~S2_PTE_MASK; /* [47:12] */
    rt_uint64_t pmd_offset = va & ~S2_PMD_MASK; /* [47:21] */
    rt_uint64_t phy_addr = 0;

    pud_t *pud_ptr = RT_NULL;
    pud_t *pmd_ptr = RT_NULL;
    pud_t *pte_ptr = RT_NULL;

    pud_t pud_val = (rt_uint64_t)mm->pgd_tbl & S2_VA_MASK;
    pud_ptr = S2_PUD_OFFSET(pud_val, va);
    if (!pud_ptr)
        return -RT_ERROR;

    pmd_ptr = S2_PMD_OFFSET((pmd_t *)(*pud_ptr & S2_VA_MASK), va);
    if (!pmd_ptr)
        return -RT_ERROR;

    /* 2M huge page */
    if ((*pmd_ptr & MMU_TYPE_MASK) == MMU_TYPE_BLOCK)
    {
		*pa = (*pmd_ptr & S2_VA_MASK) + pmd_offset;
    	return RT_EOK;
    }

    pte_ptr = S2_PTE_OFFSET((pte_t *)(*pte_ptr & S2_VA_MASK), va);
    phy_addr = *pte_ptr & S2_VA_MASK;
    if (phy_addr == 0)
        return -RT_ERROR;
    
    *pa = phy_addr + pte_offset;
    return RT_EOK;
}

void *alloc_vm_pgd(void)
{
    void *vm_pgd = rt_malloc_align(S2_PAGETABLE_SIZE, S2_PAGETABLE_SIZE);
    if (vm_pgd == RT_NULL)
    {
        rt_kprintf("[Error] Allocate VM PGD table failure.\n");
        return RT_NULL;
    }
    else
    {
        rt_memset(vm_pgd, 0, S2_PAGETABLE_SIZE);
        rt_kprintf("[Info] PGD: 0x%08x-0x%08x\n", vm_pgd, vm_pgd + S2_PAGETABLE_SIZE);
        return vm_pgd;
    }
}

rt_err_t stage2_map(struct mm_struct *mm, struct mem_desc *desc)
{
    if (desc->vaddr_start == desc->vaddr_end)
        return -RT_EINVAL;

    RT_ASSERT(desc->paddr_start < S2_PA_SIZE);
    RT_ASSERT((desc->vaddr_start < S2_IPA_SIZE) && (desc->vaddr_end < S2_IPA_SIZE));
    RT_ASSERT(IS_BLOCK_ALIGN(desc->vaddr_start) && IS_BLOCK_ALIGN(desc->vaddr_end));

    pud_t pud_val = (rt_uint64_t)mm->pgd_tbl & S2_VA_MASK;
    return s2_map_pud((pud_t *)pud_val, desc);
}

rt_err_t stage2_unmap(struct mm_struct *mm, rt_ubase_t va, rt_ubase_t va_end)
{
    if (va == va_end)
        return -RT_EINVAL;

	RT_ASSERT((va < S2_IPA_SIZE) && (va_end <= S2_IPA_SIZE));
	return s2_unmap_pud(mm, va, va_end);
}