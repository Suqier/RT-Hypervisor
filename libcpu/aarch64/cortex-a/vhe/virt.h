/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-21     Suqier       first version
 */

#ifndef __VIRT_H__
#define __VIRT_H__

#include "lib_helpers.h"
#include "vm.h"

#define VMID_SHIFT  (48)
#define VA_MASK     (0x0000fffffffff000UL)

void flush_guest_all_tlb(struct mm_struct *mm);

#endif  /* __VIRT_H__ */