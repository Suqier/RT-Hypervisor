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
 * sizeof(unsigned char) = 1
 */
void bitmap_init(unsigned char *bitmap, int size)
{
    rt_memset(bitmap, 0, sizeof(unsigned char) * size);
}

void bitmap_set_bit(unsigned char *bitmap, int size, int index)
{
    int add = index / sizeof(unsigned char *), pos = index % sizeof(unsigned char *);
	RT_ASSERT(add < size);

	bitmap += add;
	*bitmap |= one[pos];
}

void bitmap_clr_bit(unsigned char *bitmap, int size, int index)
{
    int add = index / sizeof(unsigned char *), pos = index % sizeof(unsigned char *);
	RT_ASSERT(add < size);

	bitmap += add;
	*bitmap &= zero[pos];
}

int bitmap_get_bit(unsigned char* bitmap, int size, int index)
{
	int add = index / sizeof(unsigned char *), pos = index % sizeof(unsigned char *);
	RT_ASSERT(add < size);

	bitmap += add;
	return (*bitmap & one[pos]) == one[pos];
}