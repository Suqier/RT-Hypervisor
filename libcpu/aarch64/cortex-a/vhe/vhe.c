/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-05-28     Suqier       first version
 */

// #if defined(RT_HYPERVISOR) && defined(ARCH_FEAT_VHE)

#include "vhe.h"

rt_bool_t arm_vhe_supported(void)
{
    rt_uint64_t val;
    GET_SYS_REG(ID_AA64MMFR1_EL1, val);
    val = (val >> ID_AA64MMFR1_VH_SHIFT) & ID_AA64MMFR1_MASK;
    if (val == ID_AA64MMFR1_VHE)
        return RT_TRUE;
    else
        return RT_FALSE;
}

rt_int8_t arm_vmid_bits(void)
{
    rt_int8_t vmid_bits = 8;

    rt_uint64_t val;
    GET_SYS_REG(ID_AA64MMFR1_EL1, val);
    val = (val >> ID_AA64MMFR1_VH_SHIFT) & ID_AA64MMFR1_MASK;
    if (val == ID_AA64MMFR1_VMID_16BIT)
        vmid_bits = 16;

    return vmid_bits;
}

rt_bool_t arm_sve_supported(void)
{
    rt_uint64_t val;
    GET_SYS_REG(ID_AA64PFR0_EL1, val);
    val = (val >> ID_AA64PFR0_SVE_SHIFT) & ID_AA64PFR0_MASK;
    if (val == ID_AA64PFR0_NSVE)
        return RT_FALSE;
    else
        return RT_TRUE;
}


// #endif /* defined(RT_HYPERVISOR) && defined(ARCH_FEAT_VHE) */