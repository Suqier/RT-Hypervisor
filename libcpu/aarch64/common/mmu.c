/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-11-28     GuEe-GUI     first version
 * 2022-06-03     Suqier       add comment for memory attribute
 *                             modify rt_hw_mmu_init() for RT_HYPERVISOR
 */

#include <rtthread.h>
#include <rthw.h>

#include <cpuport.h>
#include <lib_helpers.h>

#include "mmu.h"

/* only map 4G io/memory */
static volatile unsigned long MMUTable[512] __attribute__((aligned(4096)));
static volatile struct
{
    unsigned long entry[512];
} MMUPage[MMU_TBL_PAGE_NR_MAX] __attribute__((aligned(4096)));

static unsigned long _kernel_free_page(void)
{
    static unsigned long i = 0;

    if (i >= MMU_TBL_PAGE_NR_MAX)
    {
        return RT_NULL;
    }

    ++i;

    return (unsigned long)&MMUPage[i - 1].entry;
}

static int _kernel_map_2M(unsigned long *tbl, unsigned long va, unsigned long pa, unsigned long attr)
{
    int level;
    unsigned long *cur_lv_tbl = tbl;
    unsigned long page;
    unsigned long off;
    int level_shift = MMU_ADDRESS_BITS;

    if (va & ARCH_SECTION_MASK)
    {
        return MMU_MAP_ERROR_VANOTALIGN;
    }
    if (pa & ARCH_SECTION_MASK)
    {
        return MMU_MAP_ERROR_PANOTALIGN;
    }

    for (level = 0; level < MMU_TBL_BLOCK_2M_LEVEL; ++level)
    {
        off = (va >> level_shift);
        off &= MMU_LEVEL_MASK;

        if (!(cur_lv_tbl[off] & MMU_TYPE_USED))
        {
            page = _kernel_free_page();

            if (!page)
            {
                return MMU_MAP_ERROR_NOPAGE;
            }

            rt_memset((char *)page, 0, ARCH_PAGE_SIZE);
            rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH, (void *)page, ARCH_PAGE_SIZE);
            cur_lv_tbl[off] = page | MMU_TYPE_TABLE;
            rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH, cur_lv_tbl + off, sizeof(void *));
        }
        else
        {
            page = cur_lv_tbl[off];
            page &= MMU_ADDRESS_MASK;
        }

        page = cur_lv_tbl[off];
        if ((page & MMU_TYPE_MASK) == MMU_TYPE_BLOCK)
        {
            /* is block! error! */
            return MMU_MAP_ERROR_CONFLICT;
        }

        /* next level */
        cur_lv_tbl = (unsigned long *)(page & MMU_ADDRESS_MASK);
        level_shift -= MMU_LEVEL_SHIFT;
    }

    attr &= MMU_ATTRIB_MASK;
    pa |= (attr | MMU_TYPE_BLOCK);
    off = (va >> ARCH_SECTION_SHIFT);
    off &= MMU_LEVEL_MASK;
    cur_lv_tbl[off] = pa;
    rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH, cur_lv_tbl + off, sizeof(void *));

    return 0;
}

int rt_hw_mmu_setmtt(unsigned long vaddr_start, unsigned long vaddr_end,
        unsigned long paddr_start, unsigned long attr)
{
    int ret = -1;
    int i;
    unsigned long count;
    unsigned long map_attr = MMU_MAP_CUSTOM(MMU_AP_KAUN, attr);

    if (vaddr_start > vaddr_end)
    {
        goto end;
    }
    if (vaddr_start % ARCH_SECTION_SIZE)
    {
        vaddr_start = (vaddr_start / ARCH_SECTION_SIZE) * ARCH_SECTION_SIZE;
    }
    if (paddr_start % ARCH_SECTION_SIZE)
    {
        paddr_start = (paddr_start / ARCH_SECTION_SIZE) * ARCH_SECTION_SIZE;
    }
    if (vaddr_end % ARCH_SECTION_SIZE)
    {
        vaddr_end = (vaddr_end / ARCH_SECTION_SIZE + 1) * ARCH_SECTION_SIZE;
    }

    count = (vaddr_end - vaddr_start) >> ARCH_SECTION_SHIFT;

    for (i = 0; i < count; i++)
    {
        ret = _kernel_map_2M((void *)MMUTable, vaddr_start, paddr_start, map_attr);
        vaddr_start += ARCH_SECTION_SIZE;
        paddr_start += ARCH_SECTION_SIZE;

        if (ret != 0)
        {
            goto end;
        }
    }

end:
    return ret;
}

void rt_hw_init_mmu_table(struct mem_desc *mdesc, rt_size_t desc_nr)
{
    rt_memset((void *)MMUTable, 0, sizeof(MMUTable));
    rt_memset((void *)MMUPage, 0, sizeof(MMUPage));

    /* set page table */
    for (; desc_nr > 0; --desc_nr)
    {
        rt_hw_mmu_setmtt(mdesc->vaddr_start, mdesc->vaddr_end, mdesc->paddr_start, mdesc->attr);
        ++mdesc;
    }

    rt_hw_dcache_flush_range((unsigned long)MMUTable, sizeof(MMUTable));
}

void rt_hw_mmu_tlb_invalidate(void)
{
    __asm__ volatile (
        "tlbi vmalle1\n\r"
        "dsb sy\n\r"
        "isb sy\n\r"
        "ic ialluis\n\r"
        "dsb sy\n\r"
        "isb sy");
}

/*
 * When FEAT_VHE is implemented, and HCR_EL2.E2H is set to 1, when executing at 
 * EL2, some EL1 System register access instructions are redefined to access the
 * equivalent EL2 register.
 * 
 * These register includes:
 * SCTLR_EL1 / CPACR_EL1 / TRFCR_EL1 / TTBR0_EL1 / TTBR1_EL1 / TCR_EL1
 * AFSR0_EL1 / AFSR1_EL1 / ESR_EL1   / FAR_EL1   / MAIR_EL1  / AMAIR_EL1
 * VBAR_EL1  /     CONTEXTIDR_EL1    /      CNTKCTL_EL1      / CNTP_TVAL_EL0
 * CNTP_CTL_EL0 / CNTP_CVAL_EL0 / CNTV_TVAL_EL0 / CNTV_CTL_EL0 / CNTV_CVAL_EL0
 * SPSR_EL1 / ELR_EL1
 */
void rt_hw_mmu_init(void)
{
    /**
     *                 Attr2    Attr1    Attr0
     * 0x00447fUL = 0b00000000 01000100 01111111
     * Attr2: Device-nGnRnE memory
     * Attr1: Normal memory, Outer Non-cacheable, 
     *        Inner Non-cacheable
     * Attr0: Normal memory, Outer Write-Back Transient, 
     *        Inner Write-Back Non-transient
     * 
     * If use RT_HYPERVISOR, write mair_el2 / tcr_el2 / 
     * sctlr_el2 / ttbr0_el2 and set MMU at EL2 actually.
     */ 
    rt_uint64_t reg_val = 0x00447fUL;
    SET_SYS_REG(MAIR_EL1, reg_val);     
    rt_hw_isb();

    reg_val = 0UL;
    reg_val &= ~( TCR_RES0 
                | TCR_EPD0  /* Perform translation table walks using ttbr0_el2. */
                | TCR_A1    /* ttbr0_el2.ASID defines the ASID.  */
                | TCR_EPD1  /* Perform translation table walks using ttbr1_el2. */
                | TCR_TBI0  /* Top Byte used in the address calculation. */
                | TCR_TBI1 );
    reg_val |= (TCR_T0SZ(48)  | TCR_IRGN0_WBNWA | TCR_ORGN0_WBNWA
              | TCR_SH0_OUTER | TCR_TG0_4KB
              | TCR_T1SZ(48)  | TCR_IRGN1_WBNWA | TCR_ORGN1_WBNWA
              | TCR_SH0_OUTER | TCR_TG1_4KB     
              | TCR_IPS_64GB  | TCR_ASID16);

    SET_SYS_REG(TCR_EL1, reg_val);
    rt_hw_isb();

    GET_SYS_REG(SCTLR_EL1, reg_val);
    reg_val |= 1 << 2;  /* enable dcache */
    reg_val |= 1 << 0;  /* enable mmu */

    __asm__ volatile (
        "msr ttbr0_el1, %0\n\r"
        "msr sctlr_el1, %1\n\r"
        "dsb sy\n\r"
        "isb sy\n\r"
        ::"r"(MMUTable), "r"(reg_val) :"memory");

    rt_hw_mmu_tlb_invalidate();
}

int rt_hw_mmu_map(unsigned long addr, unsigned long size, unsigned long attr)
{
    int ret;
    rt_base_t level;

    level = rt_hw_interrupt_disable();
    ret = rt_hw_mmu_setmtt(addr, addr + size, addr, attr);

    rt_hw_interrupt_enable(level);

    return ret;
}
