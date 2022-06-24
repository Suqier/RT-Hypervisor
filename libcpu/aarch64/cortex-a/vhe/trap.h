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

// #if defined(RT_HYPERVISOR)

#define EC_SHIFT    (26)
#define EC_MASK     (0b111111)
#define GET_EC(esr) (((esr) >> EC_SHIFT) & EC_MASK)

/* ESR_ELx exception type */
#define ESR_EC_UNKNOWN  (0b000000)
#define ESR_EC_WFX      (0b000001)
#define ESR_EC_SIMD_FP  (0b000111)
#define ESR_EC_IES      (0b001110)
#define ESR_EC_SVC64    (0b010101)
#define ESR_EC_HVC64    (0b010110)
#define ESR_EC_SMC64    (0b010111)
#define ESR_EC_SYS64    (0b011000)
#define ESR_EC_SVE      (0b011001)
#define ESR_EC_IABT_LOW	(0b100000)
#define ESR_EC_IABT_CUR	(0b100001)
#define ESR_EC_PC_ALIGN	(0b100010)
#define ESR_EC_DABT_LOW	(0b100100)
#define ESR_EC_DABT_CUR	(0b100101)
#define ESR_EC_SP_ALIGN	(0b100110)
#define ESR_EC_TFP64	(0b101100)
#define ESR_EC_SERROR   (0b101111)
#define ESR_EC_MAX		(0b111111)

/* 
 * Sync excepition handler definition 
 */
typedef void (*rt_sync_handler_t)(rt_uint32_t esr, rt_uint32_t ec_type);

struct rt_sync_desc
{
    rt_sync_handler_t handler;
    rt_uint8_t        pc_offset;
};

// #define RT_INSTALL_SYNC_DESC(name, h, offset) \
// 	static struct rt_sync_desc name =  \
//     {	                        \
//         .handler = h,           \
// 		.pc_offset = offset,    \
// 	}(__attribute__(__used)) 

void rt_hw_trap_sync(void);
// #endif  /* defined(RT_HYPERVISOR) */

#endif  /* __TRAP_H__ */