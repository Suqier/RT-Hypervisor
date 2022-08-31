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
	return (*bitmap & (0x1 << index)) == (0x1 << index) ? 0 : 1;
}

int bitmap_find_next(rt_uint32_t *bitmap)
{
	return __rt_ffs(*bitmap) - 1;
}