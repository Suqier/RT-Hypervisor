/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-19     Suqier       the first version
 */

#ifndef __SORT_H__
#define __SORT_H__

#include <rtthread.h>

#define HEAP_SORT_BIG_FIRST     0   /* output: from big to small (9 -> 0)*/
#define HEAP_SORT_SMALL_FIRST   1   /* output: from small to big (0 -> 9) */


typedef void (*heapify)(rt_uint32_t arr[], rt_size_t start, rt_size_t end);

rt_inline void rt_swap_uint32(rt_uint32_t *a, rt_uint32_t *b)
{
    rt_uint32_t temp = *b;
    *b = *a;
    *a = temp;
}

/* output: from small to big (0 -> 9) */
void max_heapify(rt_uint32_t arr[], rt_size_t start, rt_size_t end);
void max_heap_create(rt_uint32_t arr[], rt_size_t start, rt_size_t end);

/* output: from big to small (9 -> 0)*/
void min_heapify(rt_uint32_t arr[], rt_size_t start, rt_size_t end);
void min_heap_create(rt_uint32_t arr[], rt_size_t start, rt_size_t end);

void heap_sort(rt_uint32_t arr[], rt_size_t len, rt_bool_t big);

#endif  /* __SORT_H__ */
