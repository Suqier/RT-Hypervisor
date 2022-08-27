/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Date           Author       Notes
 * 2020-06-02     Suqier       the first version
 */

#include <rtdef.h>
#include <cpuport.h>

extern unsigned char __bss_start;
extern unsigned char __bss_end;

#if defined(RT_HYPERVISOR)
#include "armv8.h"
#include "lib_helpers.h"

#ifdef	RT_USING_VHE
#include "vhe.h"
#else
#include "nvhe.h"
#endif

void rt_hypervisor_el2_setup(void)
{
	rt_uint64_t val;
	GET_SYS_REG(MIDR_EL1, val);
	SET_SYS_REG(VPIDR_EL2, val);
	
	GET_SYS_REG(MPIDR_EL1, val);
	SET_SYS_REG(VMPIDR_EL2, val);

	GET_SYS_REG(SCTLR_EL2, val);
	val |= (SCTLR_EL2_RES1 | SCTLR_I | SCTLR_EIS | SCTLR_SPA);
	val &= ~(SCTLR_SP | SCTLR_A);
	SET_SYS_REG(SCTLR_EL2, val);

	GET_SYS_REG(HCR_EL2, val);
	val |= HCR_HOST_VHE_FLAGS;
	SET_SYS_REG(HCR_EL2, val);

	val = 0UL;
	val &= ~(CPTR_EL2_TAM);
	val |= CPTR_EL2_FPEN;
	SET_SYS_REG(CPTR_EL2, val);

	__DSB();
}
#endif /* RT_HYPERVISOR */

void rt_arm64_bss_zero(void)
{
	unsigned char *p = &__bss_start;
	unsigned char *end = &__bss_end;

	while (p < end)
		*p++ = 0U;
}