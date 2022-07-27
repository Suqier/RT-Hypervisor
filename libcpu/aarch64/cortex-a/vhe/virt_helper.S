/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-07-09     Suqier       first version
 */

/*
 * void __flush_all_tlb(void);
 */
.globl __flush_all_tlb
__flush_all_tlb:
    DSB     SY
    TLBI    VMALLS12E1IS
    DSB     SY
    ISB
