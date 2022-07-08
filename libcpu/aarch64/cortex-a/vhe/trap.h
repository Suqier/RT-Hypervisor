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

#include "vhe.h"
#include "vm_arch.h"
#include "lib_helpers.h"

// #if defined(RT_HYPERVISOR)

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
typedef struct rt_hw_exp_stack regs;

/* 
 * Sync excepition handler definition 
 */
typedef void (*rt_sync_handler_t)(regs *regs, rt_uint32_t esr, rt_uint32_t ec);

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

void rt_hw_handle_curr_sync(regs *regs);
void rt_hw_handle_low_sync(regs *regs);
void rt_hw_trap_sync(regs *regs);
// #endif  /* defined(RT_HYPERVISOR) */

#endif  /* __TRAP_H__ */