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

#include <rthw.h>
#include <mmu.h>
#include <stage2.h>

#define DEFAULT_CPU_STACK_SIZE  (0x8000)
#define MEM_BLOCK_SIZE          (0x200000)
#define MEM_BLOCK_SHIFT         (21)

#define VM_DEV_nGnRnE           (0b0001)
#define VM_NORMAL_WB            (0b0010)
#define VM_NORMAL_NC            (0b0100)
#define VM_NORMAL_WT            (0b1000)
#define VM_MEMATTR_MASK         (0b1111)

#define VM_RWX_SHIFT            (4)
#define VM_READ                 (0b0001 << VM_RWX_SHIFT)
#define VM_WRITE                (0b0010 << VM_RWX_SHIFT)
#define VM_EXEC                 (0b0100 << VM_RWX_SHIFT)
#define VM_RWX_MASK             (0b0111 << VM_RWX_SHIFT)

#define VM_MAP_SHIFT            (8)
#define VM_BLOCK_2M             (0b0001 << VM_MAP_SHIFT)
#define VM_DEV_MAP              (0b0010 << VM_MAP_SHIFT)
#define VM_PFN_MAP		        (0b0100 << VM_MAP_SHIFT)
#define VM_MAP_MASK             (0b0111 << VM_MAP_SHIFT)

#define VM_SHARED_SHIFT         (12)
#define VM_SHARED               (0b0001 << VM_SHARED_SHIFT)
#define VM_HOST                 (0b0010 << VM_SHARED_SHIFT)
#define VM_GUEST                (0b0100 << VM_SHARED_SHIFT)
#define VM_SHARED_MASK          (0b0111 << VM_SHARED_SHIFT)

#define VM_MAP_TYPE_SHIFT       (16)
#define VM_MAP_BK   	        (0b0001 << VM_MAP_TYPE_SHIFT)	/* mapped as block */
#define VM_MAP_PT   	        (0b0010 << VM_MAP_TYPE_SHIFT)	/* mapped as pass though, PFN_MAP */
#define VM_MAP_TYPE_MASK	    (0b0011 << VM_MAP_TYPE_SHIFT)

#define VM_RO                   (VM_READ)
#define VM_WO                   (VM_WRITE)
#define VM_RW                   (VM_READ | VM_WRITE)
#define VM_RWX                  (VM_READ | VM_WRITE | VM_EXEC)

#define VM_NORMAL               (VM_NORMAL_WB)
#define VM_IO                   (VM_DEV_nGnRnE | VM_DEV_MAP)
#define VM_DMA			        (VM_NORMAL_NC)
#define VM_HUGE		            (VM_BLOCK_2M)

#define VM_HOST_NORMAL		    (VM_HOST | VM_PFN_MAP | VM_NORMAL)
#define VM_HOST_NORMAL_NC	    (VM_HOST | VM_PFN_MAP | VM_NORMAL_NC)
#define VM_HOST_IO		        (VM_HOST | VM_IO)

#define VM_GUEST_NORMAL		    (VM_GUEST | VM_NORMAL | VM_MAP_BK | VM_RWX)

/* passthough device for guest VM */
#define VM_GUEST_IO		        (VM_GUEST | VM_IO | VM_MAP_PT | VM_DEV_MAP)

/* virtual device created by host for guest VM, memory R/W will trapped */
#define VM_GUEST_VDEV		    (VM_GUEST)

typedef rt_uint64_t pgd_t;
typedef rt_uint64_t pud_t;
typedef rt_uint64_t pmd_t;
typedef rt_uint64_t pte_t;

#define BYTE(n)   ((n) << 20)
#define MB(n)     ((n) >> 20)

struct mem_block
{
    void *ptr;  /* pointer to vitrual memory allocated from Host OS */
    struct mem_block *next;
};
typedef struct mem_block mem_block_t;

struct vm_area
{
    struct mem_desc desc;
	mem_block_t *mb_head;
    rt_uint64_t flag;

    rt_list_t node;
    struct mm_struct *mm;    /* this area belongs to */
};
typedef struct vm_area *vm_area_t;

struct mm_struct
{
    rt_uint64_t mem_size;
    rt_uint64_t mem_used;

    pud_t *pgd_tbl;     /* start from level 1 */

#ifdef RT_USING_SMP
    rt_hw_spinlock_t lock;
#endif
    
    rt_list_t vm_area_used;
    struct vm *vm;  /* this mm_struct belongs to. */
};

struct vm_area *vm_area_init(struct mm_struct *mm, rt_uint64_t start, rt_uint64_t end);
rt_err_t vm_mm_struct_init(struct mm_struct *mm);
mem_block_t *alloc_mem_block(void);
rt_err_t alloc_vm_memory(struct mm_struct *mm);
rt_err_t map_vm_memory(struct mm_struct *mm);
rt_err_t vm_memory_init(struct mm_struct *mm);

#endif  /* __MM_H__ */