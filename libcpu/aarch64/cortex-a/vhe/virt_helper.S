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

/*
 * void __guest_enter(struct cpu_context *vcpu_ctxt, struct cpu_context *host_ctxt);
 * x0: vcpu_ctxt
 * x1: host_ctxt
 */
.global __guest_enter
__guest_enter:
    /* save host gp_regs */
    STP     X0,  X1,  [X1, #0]
    STP     X2,  X3,  [X1, #16]
    STP     X4,  X5,  [X1, #32]
    STP     X6,  X7,  [X1, #48]
    STP     X8,  X9,  [X1, #64]
    STP     X10, X11, [X1, #80]
    STP     X12, X13, [X1, #96]
    STP     X14, X15, [X1, #112]
    STP     X16, X17, [X1, #128]
    STP     X18, X19, [X1, #144]
    STP     X20, X21, [X1, #160]
    STP     X22, X23, [X1, #176]
    STP     X24, X25, [X1, #192]
    STP     X26, X27, [X1, #208]
    STP     X28, X29, [X1, #224]
    STR     X30, [X1, #240]

    ADD     X1,  X1,  #248

    MRS     X27, FPCR
    STR     X27, [X1, #28]
    MRS     X28, FPSR
    STR     X28, [X1, #24]

    /* save host OS sp_el0 */
    MOV     X29, SP
    STR     X29, [X1, #0]

    /* 
     * TBD
     * check ISR_EL1 [sche in OR trap in]
     */

    /* load vCPU gp_regs */
    ADD     X0,  X0,  #248

    LDR     X29, [x0, #0]
    MOV     SP,  X29

    LDR     X28, [x0, #24]
    MSR     FPSR, X28
    LDR     X27, [x0, #28]
    MSR     FPCR, X27

    SUB     X0,  X0,  #248

    LDR     X30, [X0, #240]
    LDP     X28, X29, [X0, #224]
    LDP     X26, X27, [X0, #208]
    LDP     X24, X25, [X0, #192]
    LDP     X22, X23, [X0, #176]
    LDP     X20, X21, [X0, #160]
    LDP     X18, X19, [X0, #144]
    LDP     X16, X17, [X0, #128]
    LDP     X14, X15, [X0, #112]
    LDP     X12, X13, [X0, #96]
    LDP     X10, X11, [X0, #80]
    LDP     X8,  X9,  [X0, #64]
    LDP     X6,  X7,  [X0, #48]
    LDP     X4,  X5,  [X0, #32]
    LDP     X2,  X3,  [X0, #16]
    LDP     X0,  X1,  [X0, #0]

    /* ERET [switch to vCPU / Guest */
    ERET
