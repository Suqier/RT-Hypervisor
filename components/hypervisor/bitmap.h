/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-08-30     Suqier       the first version
 */

#ifndef __BITMAP_H__
#define __BITMAP_H__

#include <rtthread.h>

#define BITMASK(OFF, LEN) ((((1ULL<<((LEN)-1))<<1)-1)<<(OFF))

rt_uint64_t bit_get(rt_uint64_t word, rt_uint64_t off);
rt_uint64_t bit_set(rt_uint64_t word, rt_uint64_t off);
rt_uint64_t bit_clear(rt_uint64_t word, rt_uint64_t off);
rt_uint64_t bit_extract(rt_uint64_t word, rt_uint64_t off, rt_uint64_t len);
rt_uint64_t bit_insert(rt_uint64_t word, rt_uint64_t val, rt_uint64_t off,
                                  rt_uint64_t len);
                                  
/*
 * This bitmap file is using to managament VM index and VM's page tables.
 * The default value of VM index (MAX_VM_NUM) is 8  
 * and for VM's page tables (MMU_TBL_PAGE_NR_MAX) is 32.
 */
void bitmap_init(rt_uint32_t *bitmap);
void bitmap_set_bit(rt_uint32_t *bitmap, int index);
void bitmap_clr_bit(rt_uint32_t *bitmap, int index);
int bitmap_get_bit(rt_uint32_t *bitmap, int index);
int bitmap_find_next(rt_uint32_t *bitmap);

#endif  /* __BITMAP_H__ */
