/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-08-30     Suqier       the first version
 */

#include <rtthread.h>
#include "bitmap.h"

/* 
 * bit operation 
 */
rt_uint64_t bit_get(rt_uint64_t word, rt_uint64_t off)
{
    return word & (1UL << off);
}

rt_uint64_t bit_set(rt_uint64_t word, rt_uint64_t off)
{
    return word |= (1UL << off);
}

rt_uint64_t bit_clear(rt_uint64_t word, rt_uint64_t off)
{
    return word &= ~(1UL << off);
}

rt_uint64_t bit_extract(rt_uint64_t word, rt_uint64_t off, rt_uint64_t len)
{
    return (word >> off) & BITMASK(0, len);
}

rt_uint64_t bit_insert(rt_uint64_t word, rt_uint64_t val, rt_uint64_t off,
                                  rt_uint64_t len)
{
    return (~BITMASK(off, len) & word) | ((BITMASK(0, len) & val) << off);
}

/* 
 * bitmap operation 
 */
void bitmap_init(rt_uint32_t *bitmap)
{
	/* 
	 * For re-use the code of function __rt_ffs(), 
	 * we define 1 to be idle and 0 to be occupied. 
	 */
    rt_memset(bitmap, 0xFF, sizeof(rt_uint32_t));
}

void bitmap_set_bit(rt_uint32_t *bitmap, int index)
{
	*bitmap &= ~(0x1 << index);
}

void bitmap_clr_bit(rt_uint32_t *bitmap, int index)
{
	*bitmap |= 0x1 << index;
}

int bitmap_get_bit(rt_uint32_t *bitmap, int index)
{
	/* If index bit idle than return 0, otherwise return 1. */
	return (*bitmap & (0x1 << index)) == (0x1 << index) ? 0 : 1;
}

/* find next idle? */
int bitmap_find_next(rt_uint32_t *bitmap)
{
	return __rt_ffs(*bitmap) - 1;
}