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
