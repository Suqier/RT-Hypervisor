/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2017-5-30      Bernard      the first version
 */

#include <rtthread.h>
#include <armv8.h>
#include <lib_helpers.h>

int main(int argc, char** argv)
{
    rt_kprintf("Hi, this is RT-Thread!!\n");
    
    rt_ubase_t currEL = rt_hw_get_current_el();
    rt_kprintf("Now is at EL%d\n", currEL);

    GET_SYS_REG(ID_AA64MMFR2_EL1, currEL);
    rt_kprintf("ID_AA64MMFR2_EL1 is 0x%16.16p\n", currEL);

    return 0;
}
