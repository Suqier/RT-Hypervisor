/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-19     Suqier       the first version
 */

#include <rtthread.h>
#include "sort.h"

/* 
 * output: from small to big
 * 0 0 0 1 1 1 2 2 2 3 3 3 4 4 4 5 5 5 6 6 6 7 7 7 8 8 8 9 9 9 
 */
void max_heapify(rt_uint32_t arr[], rt_size_t start, rt_size_t end) 
{
    rt_size_t dad = start;
    rt_size_t son = dad * 2 + 1;
    while (son <= end)
    {   
        if (son + 1 <= end && arr[son] < arr[son + 1]) 
            son++;
        
        if (arr[dad] > arr[son]) 
            return;
        else
        { 
            rt_swap_uint32(&arr[dad], &arr[son]);
            dad = son;
            son = dad * 2 + 1;
        }
    }
}

void max_heap_create(rt_uint32_t arr[], rt_size_t start, rt_size_t end)
{
	for(rt_int16_t i = (end - start)/2; i >= 0; i--)
		max_heapify(arr, start, end);
}

/* output: from big to small */
void min_heapify(rt_uint32_t arr[], rt_size_t start, rt_size_t end) 
{
    rt_size_t dad = start;
    rt_size_t son = dad * 2 + 1;
    while (son <= end)
    {   
        if (son + 1 <= end && arr[son] > arr[son + 1]) 
            son++;
        
        if (arr[dad] < arr[son]) 
            return;
        else
        { 
            rt_swap_uint32(&arr[dad], &arr[son]);
            dad = son;
            son = dad * 2 + 1;
        }
    }
}

void min_heap_create(rt_uint32_t arr[], rt_size_t start, rt_size_t end)
{
	for(rt_int16_t i = (end - start) / 2; i >= 0; i--)
		min_heapify(arr, start, end);
}

void heap_sort(rt_uint32_t arr[], rt_size_t len, rt_bool_t big) 
{
    heapify handler;
    if (big)
        handler = max_heapify;
    else
        handler = min_heapify;

    for (rt_int16_t i = len / 2 - 1; i >= 0; i--)
        handler(arr, i, len - 1);
    for (rt_int16_t i = len - 1; i > 0; i--) 
    {
        rt_swap_uint32(&arr[0], &arr[i]);
        handler(arr, 0, i - 1);
    }
}
