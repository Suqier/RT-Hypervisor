/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-06-08     Suqier       first version
 */

// #if defined(RT_HYPERVISOR)
#include <rtthread.h>

#include "trap.h"
#include "lib_helpers.h"

void ec_unknown_handler(rt_uint32_t esr, rt_uint32_t ec_type)
{

}

void ec_wfx_handler(rt_uint32_t esr, rt_uint32_t ec_type)
{
    
}


// RT_INSTALL_SYNC_DESC(esr_ec_unknown, ec_unknown_handler, 4);
// RT_INSTALL_SYNC_DESC(esr_ec_wfx, ec_wfx_handler, 4);


static struct rt_sync_desc sync_table[ESR_EC_MAX];


/* handler sync exception from current level. */
void rt_hw_trap_sync(void)
{
    rt_uint32_t esr_val;
    GET_SYS_REG(ESR_EL2, esr_val);
    rt_uint32_t ec_type = GET_EC(esr_val);
    
    if (ec_type >= ESR_EC_MAX)
    {
        rt_kprintf("[Error] Unexpected ec_type %d.\n", ec_type);
        return;
    }
    
    /* TBD */
    rt_kprintf("[TBD] rt_hw_trap_sync() %d.\n", ec_type);
}
// #endif  /* defined(RT_HYPERVISOR) */