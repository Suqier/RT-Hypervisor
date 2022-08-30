/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-19     Suqier       first version
 */

#include "rtconfig.h"
#include "stage2.h"

/* 
 * VM Page Table 
 * In menuconfig, we set the default value of MAX_VM_NUM 
 * to decrease VM managment cost.
 */
struct S2_MMUTable
{
    /* Concatened 2 pages for level 1 and malloc should align 8KB. */
    unsigned long table_entry[1024];
} __attribute__((aligned(8192)));
static volatile struct S2_MMUTable S2_MMUTable_Group[MAX_VM_NUM];

/* 
 * VM MMU Page 
 * Each virtual machine uses MMU_TBL_PAGE_NR_MAX S2_MMUPage,
 * for a total of MAX_VM_NUM virtual machines.
 */
struct S2_MMUPage
{
    unsigned long page_entry[512];
} __attribute__((aligned(4096)));
static volatile struct S2_MMUPage S2_MMUPage_Group[MAX_VM_NUM][MMU_TBL_PAGE_NR_MAX];
static volatile unsigned long page_i[MAX_VM_NUM] = {0};

/* 
 * Using these clear function when delete VM 
 */
void clear_s2_mmu_table(int vm_idx)
{
    RT_ASSERT(vm_idx >= 0 && vm_idx < MAX_VM_NUM);
    rt_memset((void *)&S2_MMUTable_Group[vm_idx], 0, sizeof(struct S2_MMUTable));
}

void clear_s2_mmu_page_group(int vm_idx)
{
    RT_ASSERT(vm_idx >= 0 && vm_idx < MAX_VM_NUM);
    page_i[vm_idx] = 0;
    rt_memset((void *)S2_MMUPage_Group[vm_idx], 0, 
                sizeof(struct S2_MMUPage) * MMU_TBL_PAGE_NR_MAX);
}

/* 
 * If it can use this vm_idx, then means that this vm_idx is new. 
 */
void *alloc_vm_pgd(int vm_idx)
{
    RT_ASSERT(vm_idx >= 0 && vm_idx < MAX_VM_NUM);
    return (void *)&S2_MMUTable_Group[vm_idx];
}

static unsigned long _kernel_free_s2_page(int vm_idx)
{
    RT_ASSERT(vm_idx >= 0 && vm_idx < MAX_VM_NUM);
    
    if (page_i[vm_idx] >= MMU_TBL_PAGE_NR_MAX)
    {
        return RT_NULL;
    }

    ++page_i[vm_idx];

    return (unsigned long)&S2_MMUPage_Group[vm_idx][page_i[vm_idx] - 1].page_entry;
}

rt_inline void s2_clear_pgd(pgd_t *pgd_ptr) { WRITE_ONCE(*pgd_ptr, 0); }
rt_inline void s2_clear_pud(pud_t *pud_ptr) { WRITE_ONCE(*pud_ptr, 0); }
rt_inline void s2_clear_pmd(pmd_t *pmd_ptr) { WRITE_ONCE(*pmd_ptr, 0); }
rt_inline void s2_clear_pte(pte_t *pte_ptr) { WRITE_ONCE(*pte_ptr, 0); }

rt_inline void s2_set_pgd(pgd_t *pgd_ptr, pgd_t v) { WRITE_ONCE(*pgd_ptr, v); }
rt_inline void s2_set_pud(pud_t *pud_ptr, pud_t v) { WRITE_ONCE(*pud_ptr, v); }
rt_inline void s2_set_pmd(pmd_t *pmd_ptr, pmd_t v) { WRITE_ONCE(*pmd_ptr, v); }
rt_inline void s2_set_pte(pte_t *pte_ptr, pte_t v) { WRITE_ONCE(*pte_ptr, v); }

/* 
 * Map
 */
static rt_bool_t s2_map_pmd_block(pmd_t *pmd_ptr, struct mem_desc *desc)
{
    if ((*pmd_ptr))
        return RT_FALSE;

    if ((desc->attr & MMU_TYPE_MASK) != MMU_TYPE_BLOCK)
        return RT_FALSE;

    if (!IS_2M_ALIGN(desc->vaddr_start) 
     || !IS_2M_ALIGN(desc->vaddr_end - desc->vaddr_start))
        return RT_FALSE;
    
    return RT_TRUE;
}

static rt_bool_t s2_map_pud_block(pud_t *pud_ptr, struct mem_desc *desc)
{
    if ((*pud_ptr))
        return RT_FALSE;

    if ((desc->attr & MMU_TYPE_MASK) != MMU_TYPE_BLOCK)
        return RT_FALSE;

    if (!IS_1G_ALIGN(desc->vaddr_start) 
     || !IS_1G_ALIGN(desc->vaddr_end - desc->vaddr_start))
        return RT_FALSE;
    
    return RT_TRUE;
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

static rt_err_t s2_map_pmd(pmd_t *pmd_tbl, struct mem_desc *desc, int vm_idx)
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

        if (s2_map_pmd_block(pmd_ptr, desc))
        {
            /* map 2M memory */
            pmd_t block_val = desc->attr | (desc->paddr_start & L2_BLOCK_OA_MASK);
            rt_kprintf("[Info] S2 map 2M pa&attr=0x%08x at pmd_ptr=0x%08x\n", block_val, pmd_ptr);
            s2_set_pmd(pmd_ptr, block_val);
        }
        else
        {
            if (*pmd_ptr)
                /* get next level pte */
                pte_tbl = (pte_t *)(*pmd_ptr & TABLE_ADDR_MASK);
            else
            {
                /* 
                 * populate first pte
                 * page table type must align to RT_MM_PAGE_SIZE[4096]
                 */
                pte_tbl = (pte_t *)_kernel_free_s2_page(vm_idx);
                if (pte_tbl == RT_NULL)
                    return -RT_ENOMEM;

                rt_memset(pte_tbl, 0, RT_MM_PAGE_SIZE);
                pte_tbl = (pte_t *)(MMU_TYPE_TABLE | ((rt_uint64_t)pte_tbl & TABLE_ADDR_MASK));
                rt_kprintf("[Info] map pte_tbl&attr=0x%016x at pmd_ptr=0x%016x\n", pte_tbl, pmd_ptr);
                s2_set_pmd(pmd_ptr, (pmd_t)pte_tbl);
            }

            s2_map_pte(pte_tbl, desc);
        }
    } while (pmd_ptr++, 
             desc->paddr_start += size, 
             desc->vaddr_start  = next, 
             desc->vaddr_start != desc->vaddr_end);

    return RT_EOK;
}

static rt_err_t s2_map_pud(pud_t *pud_tbl, struct mem_desc *desc, int vm_idx)
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

        if (s2_map_pud_block(pud_ptr, desc))
        {
            /* map 1G memory */
            pud_t block_val = desc->attr | (desc->paddr_start & L1_BLOCK_OA_MASK);
            rt_kprintf("[Info] S2 map 1G pa&attr=0x%016x at pud_ptr=0x%08x\n", block_val, pud_ptr);
            s2_set_pud(pud_ptr, block_val);
        }
        else
        {
            if (*pud_ptr)
                /* get next level pmd */
                pmd_tbl = (pmd_t *)(*pud_ptr & TABLE_ADDR_MASK);
            else
            {
                /* 
                 * populate first pmd
                 * page table type must align to RT_MM_PAGE_SIZE[4096]
                 */
                pmd_tbl = (pmd_t *)_kernel_free_s2_page(vm_idx);
                if (pmd_tbl == RT_NULL)
                    return -RT_ENOMEM;

                rt_memset(pmd_tbl, 0, RT_MM_PAGE_SIZE);
                pmd_tbl = (pmd_t *)(MMU_TYPE_TABLE | ((rt_uint64_t)pmd_tbl & TABLE_ADDR_MASK));
                rt_kprintf("[Info] set pmd_tbl&attr=0x%08x at pud_ptr=0x%08x\n", pmd_tbl, pud_ptr);
                s2_set_pud(pud_ptr, (pud_t)pmd_tbl);
            }
        
            pmd_t pmd_val = (pmd_t)pmd_tbl & TABLE_ADDR_MASK;   /* [47:12] */
            s2_map_pmd((pmd_t *)pmd_val, desc, vm_idx);
        }
    } while (pud_ptr++, 
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

    int vm_idx = mm->vm->vm_idx;
    pud_t pud_val = (pud_t)mm->pgd_tbl & TABLE_ADDR_MASK;   /* [47:12] */
    return s2_map_pud((pud_t *)pud_val, desc, vm_idx);
}

/* 
 * Unmap
 */
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
            rt_free(pte_tmp);   /* This memory gets from heap. */
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
                    rt_free(pmd_ptr);   /* This memory gets from heap. */ 
                    rt_free(pte_tbl);   /* This page table gets from S2_MMUPage_Group. */
                }
            }
        }
    } while (pmd_ptr++, va = next, va != va_end);
}

static void s2_unmap_pud(struct mm_struct *mm, rt_ubase_t va, rt_ubase_t va_end)
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
}

rt_err_t s2_unmap(struct mm_struct *mm, rt_ubase_t va, rt_ubase_t va_end)
{
    if (va == va_end)
        return -RT_EINVAL;

	RT_ASSERT((va < S2_IPA_SIZE) && (va_end <= S2_IPA_SIZE));
	s2_unmap_pud(mm, va, va_end);
    return RT_EOK;
}

/*
 * Translation
 */
rt_err_t s2_translate(struct mm_struct *mm, rt_uint64_t va, rt_ubase_t *pa)
{
    rt_uint64_t pte_offset = va & ~TABLE_ADDR_MASK;  /* [47:12] */
    rt_uint64_t pmd_offset = va & ~L2_BLOCK_OA_MASK; /* [47:21] */
    rt_uint64_t phy_addr = 0UL;

    pud_t *pud_ptr = RT_NULL;
    pmd_t *pmd_ptr = RT_NULL;
    pte_t *pte_ptr = RT_NULL;

    pud_t pud_val = (rt_uint64_t)mm->pgd_tbl & S2_VA_MASK;
    pud_ptr = S2_PUD_OFFSET(pud_val, va);
    if (!pud_ptr)
        return -RT_ERROR;

    pmd_ptr = S2_PMD_OFFSET((pmd_t *)(*pud_ptr & S2_VA_MASK), va);
    if (!pmd_ptr)
        return -RT_ERROR;

    /* 2M mem block */
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