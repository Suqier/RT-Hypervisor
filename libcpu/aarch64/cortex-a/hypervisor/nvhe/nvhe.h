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
#define HCR_GUEST_FLAGS 	(HCR_TSC | HCR_IMO | HCR_FMO | HCR_VM | HCR_RW)
#define HCR_VIRT_EXCP_MASK 	(HCR_VSE | HCR_VI  | HCR_VF)
#define HCR_HOST_VHE_FLAGS 	(HCR_RW  | HCR_TGE)

#endif /* __VHE_H__ */