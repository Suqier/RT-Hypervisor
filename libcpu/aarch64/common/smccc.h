/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-09-09     GuEe-GUI     The first version
 */

#include <stdint.h>

/*
 * DEN0028B_SMC_Calling_Convention
 * Table 2-1 Bit usage within the SMC and HVC Function Identifier
 */
#define SMC_HVC_FC_MASK         (0x80000000)
#define SMC_HVC_32_64_MASK      (0x40000000)
#define SMC_HVC_SC_MASK         (0x3F000000)
#define SMC_HVC_MBZ_MASK        (0x00FF0000)
#define SMC_HVC_FUNC_MASK       (0x0000FFFF)

#define SMC_HVC_SC_ARCH			(0x00000000)
#define SMC_HVC_SC_CPU			(0x01000000)
#define SMC_HVC_SC_SIP			(0x02000000)
#define SMC_HVC_SC_OEM			(0x03000000)
#define SMC_HVC_SC_STD_SMC		(0x04000000)
#define SMC_HVC_SC_STD_HVC		(0x05000000)
#define SMC_HVC_SC_VENDOR_HVC	(0x06000000)

/*
 * The ARM SMCCC v1.0 calling convention provides the following guarantees about registers:
 *  Register     Modified    Return State
 *  X0...X3      Yes         Result values
 *  X4...X17     Yes         Unpredictable
 *  X18...X30    No          Preserved
 *  SP_EL0       No          Preserved
 *  SP_ELx       No          Preserved
 */

struct arm_smccc_ret
{
    /* x0 ~ x3: SMC and HVC result registers */
    uint64_t x0;    /* Function Identifier */
    uint64_t x1;    /* Parameter registers */
    uint64_t x2;    /* Parameter registers */
    uint64_t x3;    /* Parameter registers */
    uint64_t x6;    /* Parameter register: Optional Session ID register */
};

struct arm_smccc_ret arm_smc_call(uint32_t w0, uint64_t x1, uint64_t x2, uint64_t x3, uint64_t x4, uint64_t x5, uint64_t x6, uint32_t w7);
struct arm_smccc_ret arm_hvc_call(uint32_t w0, uint64_t x1, uint64_t x2, uint64_t x3, uint64_t x4, uint64_t x5, uint64_t x6, uint32_t w7);
