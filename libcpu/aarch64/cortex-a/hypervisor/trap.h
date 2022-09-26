/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-04     Suqier       the first version
 */

#ifndef __TRAP_H__
#define __TRAP_H__

#include <rtdef.h>
#include <rtthread.h>
#include <smccc.h>

#include <armv8.h>
#include "lib_helpers.h"

#ifdef RT_HYPERVISOR

#ifdef RT_USING_NVHE
#include "nvhe/nvhe.h"
#else
#include "vhe/vhe.h"
#endif

/* ESR value */
#define EC_SHIFT            (26)
#define EC_MASK             (0b111111)
#define GET_EC(esr)         (((esr) >> EC_SHIFT) & EC_MASK)

/* ESR_ELx exception type */
#define ESR_EC_UNKNOWN      (0b000000)
#define ESR_EC_WFX          (0b000001)
#define ESR_EC_SIMD_FP      (0b000111)
#define ESR_EC_IES          (0b001110)
#define ESR_EC_SVC64        (0b010101)
#define ESR_EC_HVC64        (0b010110)
#define ESR_EC_SMC64        (0b010111)
#define ESR_EC_SYS64        (0b011000)
#define ESR_EC_SVE          (0b011001)
#define ESR_EC_IABT_LOW	    (0b100000)
#define ESR_EC_IABT_CUR	    (0b100001)
#define ESR_EC_PC_ALIGN	    (0b100010)
#define ESR_EC_DABT_LOW	    (0b100100)
#define ESR_EC_DABT_CUR	    (0b100101)
#define ESR_EC_SP_ALIGN	    (0b100110)
#define ESR_EC_TFP64	    (0b101100)
#define ESR_EC_SERROR       (0b101111)
#define ESR_EC_MAX		    (0b111111)

/*
 * SMC Function Identifier
 * SMC64: Arm Architecture Calls (0xC0000000-0xC000FFFF)
 */
#define HVC_FN64_BASE           (0xC0000000)
#define HVC_FN64(n)             (HVC_FN64_BASE + (n))
#define HVC_FN64_END            (0xC000FFFF)

#define HVC_FN64_GET_VERSION    HVC_FN64(1)
#define HVC_FN64_CREATE_VM      HVC_FN64(2)
#define HVC_FN64_FREE_VM        HVC_FN64(3)
#define HVC_FN64_MMAP_VM_MEM    HVC_FN64(4)

typedef rt_uint64_t (*hvc_trap_handle)(rt_uint32_t fn, rt_uint64_t arg0, 
                                    rt_uint64_t arg1, rt_uint64_t arg2);

/* 
 * ISS for instruction/data abort from low level 
 */
#define ESR_ISV_SHIFT   (24)
#define ESR_ISV_VALID   (1 << ESR_ISV_SHIFT)

#define ESR_SAS_SHIFT   (22)
#define ESR_SAS_BYTE    (0x00 << ESR_SAS_SHIFT)
#define ESR_SAS_HFWD    (0x01 << ESR_SAS_SHIFT)
#define ESR_SAS_WORD    (0x10 << ESR_SAS_SHIFT)
#define ESR_SAS_DBWD    (0x11 << ESR_SAS_SHIFT)

#define ESR_SRT_SHIFT   (16)
#define ESR_SRT_MASK    (0x1F0000)
#define ESR_GET_SRT(e)  ((e & ESR_SRT_MASK) >> ESR_SRT_SHIFT)

#define ESR_FNV_SHIFT   (10)
#define ESR_FNV_VALID   (0)
#define ESR_FNV_MASK    (1 << ESR_FNV_SHIFT)
#define ESR_GET_FNV(e)  ((e & ESR_FNV_MASK) >> ESR_FNV_SHIFT)

#define ESR_WNR_SHIFT   (6)
#define ESR_WNR_WRITE   (1)     /* NOT write then read */
#define ESR_WNR_MASK    (1 << ESR_WNR_SHIFT)
#define ESR_GET_WNR(e)  ((e & ESR_WNR_MASK) >> ESR_WNR_SHIFT)

#define FSC_MASK        (0x3F)
#define FSC_TYPE_MASK   (0x3C)
#define FSC_ADDR        (0x00)  /* Address size fault */
#define FSC_TRANS       (0x04)  /* Translation fault  */
#define FSC_ACCESS      (0x08)  /* Access flag fault  */
#define FSC_PERM        (0x0C)  /* Permission fault   */
#define FSC_SEA         (0x10)  /* Synchronous External abort */

/* 
 * Sync excepition handler definition 
 */
typedef void (*rt_sync_handler_t)(struct rt_hw_exp_stack *regs, rt_uint32_t esr);

struct rt_sync_desc
{
    rt_sync_handler_t   handler;
    rt_uint8_t          pc_offset;
};

#define RT_INSTALL_SYNC_DESC(name, h, offset) \
	RT_USED static struct rt_sync_desc        \
    __sync_##name RT_SECTION("SyncDesc")=     \
    {	                                      \
        .handler = h,                         \
		.pc_offset = offset,                  \
	};




void rt_hw_handle_curr_sync(struct rt_hw_exp_stack *regs);
void rt_hw_handle_low_sync(struct rt_hw_exp_stack *regs);
void rt_hw_trap_sync(struct rt_hw_exp_stack *regs);

#endif  /* RT_HYPERVISOR */

#endif  /* __TRAP_H__ */