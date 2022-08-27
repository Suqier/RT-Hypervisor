/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-05-28     Suqier       first version
 */

#ifndef __VHE_H__
#define __VHE_H__

#include <armv8.h>
#include <lib_helpers.h>

#ifdef RT_HYPERVISOR

/*
 * The bits we set in HCR:
 * RW:		64bit by default, can be overridden for 32bit VMs
 * TACR:	Trap ACTLR
 * TSC:		Trap SMC
 * TSW:		Trap cache operations by set/way
 * TWE:		Trap WFE
 * TWI:		Trap WFI
 * TIDCP:	Trap L2CTLR/L2ECTLR
 * BSU_IS:	Upgrade barriers to the inner shareable domain
 * FB:		Force broadcast of all maintenance operations
 * AMO:		Override CPSR.A and enable signaling with VA
 * IMO:		Override CPSR.I and enable signaling with VI
 * FMO:		Override CPSR.F and enable signaling with VF
 * SWIO:	Turn set/way invalidates into set/way clean+invalidate
 * PTW:		Take a stage2 fault if a stage1 walk steps in device memory

#define HCR_GUEST_FLAGS (HCR_TSC | HCR_TSW | HCR_TWE | HCR_TWI | HCR_VM | \
			             HCR_BSU_IS | HCR_FB | HCR_TACR | \
			             HCR_AMO | HCR_SWIO | HCR_TIDCP | HCR_RW | \
			             HCR_FMO | HCR_IMO | HCR_PTW)
 */
#define HCR_GUEST_FLAGS 	(HCR_TSC | HCR_IMO | HCR_FMO | HCR_VM | HCR_RW)
#define HCR_VIRT_EXCP_MASK 	(HCR_VSE | HCR_VI  | HCR_VF)
#define HCR_HOST_VHE_FLAGS 	(HCR_RW  | HCR_TGE | HCR_E2H)

#endif /* RT_HYPERVISOR */

#endif /* __VHE_H__ */