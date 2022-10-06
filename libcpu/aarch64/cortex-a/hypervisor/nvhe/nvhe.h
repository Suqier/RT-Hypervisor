/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-08-27     Suqier       first version
 */

#ifndef __NVHE_H__
#define __NVHE_H__

#if defined(RT_HYPERVISOR) && defined(RT_USING_NVHE)
#include <lib_helpers.h>

/* nVHE: HCR_EL2.E2H = 0 && HCR_EL2.TGE = 1(RT-Thread is running at EL2). */
/*
 * The bits we set in HCR:
 * RW:		64bit by default, can be overridden for 32bit VMs
 * VM:      enable stage 2 translation
 * TSC:		Trap SMC
 * IMO:		Override CPSR.I and enable signaling with VI
 * FMO:		Override CPSR.F and enable signaling with VF
 */
#define HCR_GUEST_FLAGS 	(HCR_TSC | HCR_TWI | HCR_VM | HCR_RW)
#define HCR_GUEST_XMO_FLAGS (HCR_AMO | HCR_IMO | HCR_FMO)
#define HCR_VIRT_EXCP_MASK 	(HCR_VSE | HCR_VI  | HCR_VF)
#define HCR_HOST_NVHE_FLAGS (HCR_RW  | HCR_TGE)

#endif /* RT_HYPERVISOR && RT_USING_NVHE */

#endif /* __NVHE_H__ */