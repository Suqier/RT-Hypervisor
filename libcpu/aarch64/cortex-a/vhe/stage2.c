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

void *alloc_vm_pgd(void)
{
    /* concatened 2 page for level 1 and malloc should align 8KB */
    void *vm_pgd = rt_malloc_align(S2_PAGETABLE_SIZE, S2_PAGETABLE_SIZE);
    
    if (vm_pgd == RT_NULL)
        rt_kprintf("[Error] Allocate VM PGD table failure.\n");
    else
    {
        rt_memset(vm_pgd, 0, S2_PAGETABLE_SIZE);
        rt_kprintf("[Info] PGD: 0x%08x-0x%08x\n", vm_pgd, vm_pgd + S2_PAGETABLE_SIZE);
    }
    
    return vm_pgd;
}

rt_inline void s2_clear_l0(l0_t *l0_ptr)
{
    WRITE_ONCE(*l0_ptr, 0);
}

rt_inline void s2_clear_l1(l1_t *l1_ptr)
{
    WRITE_ONCE(*l1_ptr, 0);
}

rt_inline void s2_clear_l2(l2_t *l2_ptr)
{
    WRITE_ONCE(*l2_ptr, 0);
}

rt_inline void s2_clear_l3(l3_t *l3_ptr)
{
    WRITE_ONCE(*l3_ptr, 0);
}

rt_inline void s2_set_l0(l3_t *l0_ptr, l0_t l0_val)
{
    WRITE_ONCE(*l0_ptr, l0_val);
}

rt_inline void s2_set_l1(l1_t *l1_ptr, l1_t l1_val)
{
    WRITE_ONCE(*l1_ptr, l1_val);
}

rt_inline void s2_set_l2(l2_t *l2_ptr, l2_t l2_val)
{
    WRITE_ONCE(*l2_ptr, l2_val);
}

rt_inline void s2_set_l3(l3_t *l3_ptr, l3_t l3_val)
{
    WRITE_ONCE(*l3_ptr, l3_val);
}

/* 
 * s2_map_l2_block
 * check weather mmap type is memory block
 */
static rt_bool_t s2_map_l2_block(l2_t *l2_ptr, struct mem_desc *desc)
{
    if ((*l2_ptr))
        return RT_FALSE;

    if ((desc->attr & MMU_TYPE_MASK) != MMU_TYPE_BLOCK)
        return RT_FALSE;

    if (!IS_2M_ALIGN(desc->vaddr_start) 
     || !IS_2M_ALIGN(desc->vaddr_end - desc->vaddr_start))
        return RT_FALSE;
    
    return RT_TRUE;
}

static rt_bool_t s2_map_l1_block(l1_t *l1_ptr, struct mem_desc *desc)
{
    if ((*l1_ptr))
        return RT_FALSE;

    if ((desc->attr & MMU_TYPE_MASK) != MMU_TYPE_BLOCK)
        return RT_FALSE;

    if (!IS_1G_ALIGN(desc->vaddr_start) 
     || !IS_1G_ALIGN(desc->vaddr_end - desc->vaddr_start))
        return RT_FALSE;
    
    return RT_TRUE;
}

static void s2_map_l3(l3_t *l3_tbl, struct mem_desc *desc)
{
    l3_t *l3_ptr;
    rt_uint64_t l3_attr;

    l3_ptr = S2_L3_OFFSET(l3_tbl, desc->vaddr_start);
    l3_attr = desc->attr;  /* S2_PAGE_NORMAL, S2_PAGE_DEVICE and so on.*/

    do
    {
        if (!(*l3_ptr))
        {
            s2_set_l3(l3_ptr, desc->paddr_start | l3_attr);
            desc->vaddr_start += RT_MM_PAGE_SIZE;
            desc->paddr_start += RT_MM_PAGE_SIZE;
        }

        l3_ptr++;
    } while (desc->vaddr_start != desc->vaddr_end);
}

static rt_err_t s2_map_l2(l2_t *l2_tbl, struct mem_desc *desc)
{
    l2_t *l2_ptr;
    l3_t *l3_tbl;
    rt_uint64_t next, size;

    l2_ptr = S2_L2_OFFSET(l2_tbl, desc->vaddr_start);
    do
    {
        next = (desc->vaddr_start + S2_L2_SIZE) & S2_L2_MASK;
        if (next > desc->vaddr_end)
            next = desc->vaddr_end;
        size = next - desc->vaddr_start;

        if (s2_map_l2_block(l2_ptr, desc))
        {
            /* map 2M memory */
            l2_t block_val = desc->attr | (desc->paddr_start & L2_BLOCK_OA_MASK);
            rt_kprintf("[Info] S2 map 2M pa&attr=0x%08x at l2_ptr=0x%08x\n", block_val, l2_ptr);
            s2_set_l2(l2_ptr, block_val);
        }
        else
        {
            if (*l2_ptr)
                /* get next level l3 */
                l3_tbl = (l3_t *)(*l2_ptr & TABLE_ADDR_MASK);
            else
            {
                /* 
                 * populate first l3 
                 * page table type must align to RT_MM_PAGE_SIZE[4096]
                 */
                l3_tbl = (l3_t *)rt_malloc_align(RT_MM_PAGE_SIZE, RT_MM_PAGE_SIZE);
                if (l3_tbl == RT_NULL)
                    return -RT_ENOMEM;

                rt_memset(l3_tbl, 0, RT_MM_PAGE_SIZE);
                l3_tbl = (l3_t *)(MMU_TYPE_TABLE | ((rt_uint64_t)l3_tbl & TABLE_ADDR_MASK));
                rt_kprintf("[Info] map l3_tbl&attr=0x%016x at l2_ptr=0x%016x\n", l3_tbl, l2_ptr);
                s2_set_l2(l2_ptr, (l2_t)l3_tbl);
            }

            s2_map_l3(l3_tbl, desc);
        }
    } while (l2_ptr++, 
             desc->paddr_start += size, 
             desc->vaddr_start  = next, 
             desc->vaddr_start != desc->vaddr_end);

    return RT_EOK;
}

static rt_err_t s2_map_l1(l1_t *l1_tbl, struct mem_desc *desc)
{
    l1_t *l1_ptr;
    l2_t *l2_tbl;
    rt_uint64_t next, size;

    l1_ptr = S2_L1_OFFSET(l1_tbl, desc->vaddr_start);
    do
    {
        next = (desc->vaddr_start + S2_L1_SIZE) & S2_L1_MASK;
        if (next > desc->vaddr_end)
            next = desc->vaddr_end;
        size = next - desc->vaddr_start;

        if (s2_map_l1_block(l1_ptr, desc))
        {
            /* map 1G memory */
            l1_t block_val = desc->attr | (desc->paddr_start & L1_BLOCK_OA_MASK);
            rt_kprintf("[Info] S2 map 1G pa&attr=0x%016x at l1_ptr=0x%08x\n", block_val, l1_ptr);
            s2_set_l1(l1_ptr, block_val);
        }
        else
        {
            if (*l1_ptr)
                /* get next level l2 */
                l2_tbl = (l2_t *)(*l1_ptr & TABLE_ADDR_MASK);
            else
            {
                /* 
                 * populate first l2
                 * page table type must align to RT_MM_PAGE_SIZE[4096]
                 */
                l2_tbl = (l2_t *)rt_malloc_align(RT_MM_PAGE_SIZE, RT_MM_PAGE_SIZE);
                if (l2_tbl == RT_NULL)
                    return -RT_ENOMEM;

                rt_memset(l2_tbl, 0, RT_MM_PAGE_SIZE);
                l2_tbl = (l2_t *)(MMU_TYPE_TABLE | ((rt_uint64_t)l2_tbl & TABLE_ADDR_MASK));
                rt_kprintf("[Info] set l2_tbl&attr=0x%08x at l1_ptr=0x%08x\n", l2_tbl, l1_ptr);
                s2_set_l1(l1_ptr, (l1_t)l2_tbl);
            }
        
            l2_t l2_val = (l2_t)l2_tbl & TABLE_ADDR_MASK;   /* [47:12] */
            s2_map_l2((l2_t *)l2_val, desc);
        }
    } while (l1_ptr++, 
             desc->paddr_start += size, 
             desc->vaddr_start  = next, 
             desc->vaddr_start != desc->vaddr_end);

    return RT_EOK;    
}

rt_err_t s2_map(struct mm_struct *mm, struct mem_desc *desc)
{
    if (desc->vaddr_start == desc->vaddr_end)
        return -RT_EINVAL;

    RT_ASSERT(desc->paddr_start < S2_PA_SIZE);
    RT_ASSERT((desc->vaddr_start < S2_IPA_SIZE) && (desc->vaddr_end < S2_IPA_SIZE));
    RT_ASSERT(IS_2M_ALIGN(desc->vaddr_start) && IS_2M_ALIGN(desc->vaddr_end));

    l1_t l1_val = (l1_t)mm->pgd_tbl & TABLE_ADDR_MASK;   /* [47:12] */
    return s2_map_l1((l1_t *)l1_val, desc);
}

static void s2_unmap_l3(l3_t *l3_tbl, rt_ubase_t va, rt_ubase_t va_end)
{
    l3_t *l3_ptr = S2_L3_OFFSET(l3_tbl, va);

    do
    {   
        l3_t *l3_tmp = l3_ptr;
        l3_ptr++;

        if (*l3_tmp)
        {
            s2_clear_l3(l3_tmp);
            rt_free(l3_tmp);
        }
    } while (va += RT_MM_PAGE_SIZE, va != va_end);
}

static void s2_unmap_l2(l2_t *l2_tbl, rt_ubase_t va, rt_ubase_t va_end)
{
    l2_t *l2_ptr = S2_L2_OFFSET(l2_tbl, va);
    l3_t *l3_tbl = RT_NULL;
    rt_uint64_t next;

    do
    {
        next = (va + S2_L2_SIZE) & S2_L2_MASK;
        if (next > va_end)
            next = va_end;

        if (*l2_ptr)
        {
            if ((*l2_ptr & MMU_TYPE_MASK) == MMU_TYPE_BLOCK)
            {
                s2_clear_l2(l2_ptr);
                rt_free(l2_ptr);
            }
            else
            {
                l3_tbl = (l3_t *)(*l2_ptr & TABLE_ADDR_MASK);
                s2_unmap_l3(l3_tbl, va, next);
                if (((va & ~S2_L2_MASK) == 0) && ((va_end - va) == S2_L2_SIZE))
                {
                    s2_clear_l2(l2_ptr);
                    rt_free(l2_ptr);
                    rt_free(l3_tbl);
                }
            }
        }
    } while (l2_ptr++, va = next, va != va_end);
}

static void s2_unmap_l1(struct mm_struct *mm, rt_ubase_t va, rt_ubase_t va_end)
{
    l1_t *l1_ptr = S2_L1_OFFSET(mm->pgd_tbl, va);
    l2_t *l2_tbl;
    rt_uint64_t next;

    do
    {
        next = (va + S2_L1_SIZE) & S2_L1_MASK;
        if (next > va_end)
            next = va_end;

        if (*l1_ptr)
        {
            l2_tbl = (l2_t *)(*l1_ptr & TABLE_ADDR_MASK);
            s2_unmap_l2(l2_tbl, va, next);
            if (((va & ~S2_L1_MASK) == 0) && ((va_end - va) == S2_L1_SIZE))
            {
                s2_clear_l1(l1_ptr);
                rt_free(l1_ptr);
                rt_free(l2_tbl);
            }
        }
    } while (l1_ptr++, va = next, va != va_end);

    flush_vm_all_tlb(mm->vm);
}

rt_err_t s2_unmap(struct mm_struct *mm, rt_ubase_t va, rt_ubase_t va_end)
{
    if (va == va_end)
        return -RT_EINVAL;

	RT_ASSERT((va < S2_IPA_SIZE) && (va_end <= S2_IPA_SIZE));
	s2_unmap_l1(mm, va, va_end);
    return RT_EOK;
}

rt_err_t s2_translate(struct mm_struct *mm, rt_uint64_t va, rt_ubase_t *pa)
{
    rt_uint64_t l3_offset = va & ~TABLE_ADDR_MASK;  /* [47:12] */
    rt_uint64_t l2_offset = va & ~L2_BLOCK_OA_MASK; /* [47:21] */
    rt_uint64_t phy_addr = 0UL;

    l1_t *l1_ptr = RT_NULL;
    l2_t *l2_ptr = RT_NULL;
    l3_t *l3_ptr = RT_NULL;

    l1_t l1_val = (rt_uint64_t)mm->pgd_tbl & S2_VA_MASK;
    l1_ptr = S2_L1_OFFSET(l1_val, va);
    if (!l1_ptr)
        return -RT_ERROR;

    l2_ptr = S2_L2_OFFSET((l2_t *)(*l1_ptr & S2_VA_MASK), va);
    if (!l2_ptr)
        return -RT_ERROR;

    /* 2M mem block */
    if ((*l2_ptr & MMU_TYPE_MASK) == MMU_TYPE_BLOCK)
    {
		*pa = (*l2_ptr & S2_VA_MASK) + l2_offset;
    	return RT_EOK;
    }

    l3_ptr = S2_L3_OFFSET((l3_t *)(*l3_ptr & S2_VA_MASK), va);
    phy_addr = *l3_ptr & S2_VA_MASK;
    if (phy_addr == 0)
        return -RT_ERROR;
    
    *pa = phy_addr + l3_offset;
    return RT_EOK;
}