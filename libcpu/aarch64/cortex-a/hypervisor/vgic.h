/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-18     Suqier       first version
 */

#ifndef __VGIC_H__
#define __VGIC_H__

#include <rtdef.h>
#include <rthw.h>
#include <armv8.h>
#include <bitmap.h>
#include <vdev.h>

#if defined(BSP_USING_GIC) && defined(BSP_USING_GICV3)
#include <gicv3.h>
#include "trap.h"

#define MAX_VCPU_NUM    4      /* per vm */

#define VIRQ_SGI_NUM    16     /*  0 ~ 15 */
#define VIRQ_PPI_NUM    16     /* 16 ~ 31 */
#define VIRQ_PRIV_NUM   (VIRQ_SGI_NUM + VIRQ_PPI_NUM)
#define MPIDR_AFF_MASK  0xFF   /* affinity 0 only */
#define MAX_LR_REGS     16
#define GIC_LR_LIST_NUM 64

#define VGIC_GICD_SIZE  0x10000     /* 64KB */
#define VGIC_GICR_SIZE  0x20000     /* 64KB * 2 */

#define LR_NO_USE       -1
#define LR_REGS_MASK    0x1F
#define PRI_BITS_MASK   0x7
#define PRI_BITS_SHIFT  29

#define ICC_SRE_VAL         0x7
#define ICC_CTLR_EOI        (0x1 << 1)

#define GROUP0_INT          0b0
#define GROUP1_INT          0b1
#define ICH_VMCR_VENG_OFF   1
#define ICH_VMPR_OFF        24
#define ICH_VMPR_VAL        0xFF
#define GIC_LOWEST_PRIO     0xFF
#define ICH_HCR_EN          (0b1 << 0)
#define ICH_HCR_NPIE        (0b1 << 3)

#define ICC_SRE_EL1     "S3_0_C12_C12_5"

#define ICH_AP0R0_EL2   "S3_4_C12_C8_0"
#define ICH_AP0R1_EL2   "S3_4_C12_C8_1"
#define ICH_AP0R2_EL2   "S3_4_C12_C8_2"
#define ICH_AP0R3_EL2   "S3_4_C12_C8_3"

#define ICH_AP1R0_EL2   "S3_4_C12_C9_0"
#define ICH_AP1R1_EL2   "S3_4_C12_C9_1"
#define ICH_AP1R2_EL2   "S3_4_C12_C9_2"
#define ICH_AP1R3_EL2   "S3_4_C12_C9_3"

/* LR */
#define ICH_LR0_EL2     "S3_4_C12_C12_0"
#define ICH_LR1_EL2     "S3_4_C12_C12_1"
#define ICH_LR2_EL2     "S3_4_C12_C12_2"
#define ICH_LR3_EL2     "S3_4_C12_C12_3"
#define ICH_LR4_EL2     "S3_4_C12_C12_4"
#define ICH_LR5_EL2     "S3_4_C12_C12_5"
#define ICH_LR6_EL2     "S3_4_C12_C12_6"
#define ICH_LR7_EL2     "S3_4_C12_C12_7"
#define ICH_LR8_EL2     "S3_4_C12_C13_0"
#define ICH_LR9_EL2     "S3_4_C12_C13_1"
#define ICH_LR10_EL2    "S3_4_C12_C13_2"
#define ICH_LR11_EL2    "S3_4_C12_C13_3"
#define ICH_LR12_EL2    "S3_4_C12_C13_4"
#define ICH_LR13_EL2    "S3_4_C12_C13_5"
#define ICH_LR14_EL2    "S3_4_C12_C13_6"
#define ICH_LR15_EL2    "S3_4_C12_C13_7"

#define ICH_HCR_EL2     "S3_4_C12_C11_0"
#define ICH_VTR_EL2     "S3_4_C12_C11_1"
#define ICH_MISR_EL2    "S3_4_C12_C11_2"
#define ICH_EISR_EL2    "S3_4_C12_C11_3"
#define ICH_ELRSR_EL2   "S3_4_C12_C11_5"
#define ICH_VMCR_EL2    "S3_4_C12_C11_7"

/* GICD */
#define GICD_CTLR_OFF    0x0000
#define GICD_TYPE_OFF    0x0004
#define GICD_TYPE_IT_OFF      0
#define GICD_TYPE_IT_LEN      5
#define GICD_TYPE_IT_MASK     BITMASK(GICD_TYPE_IT_OFF, GICD_TYPE_IT_LEN)
#define GICD_TYPE_CPU_OFF     5
#define GICD_TYPE_CPU_LEN     3
#define GICD_TYPE_CPU_MASK    BITMASK(GICD_TYPE_CPU_OFF, GICD_TYPE_CPU_LEN)
#define GICD_TYPE_IDBITS_OFF 19
#define GICD_TYPE_IDBITS_LEN  5
#define GICD_TYPE_IDBITS_MASK BITMASK(GICD_TYPE_IDBITS_OFF, GICD_TYPE_IDBITS_LEN)
#define GICD_IIDR_OFF    0x0008

#define GICD_IGRP_OFF    0x0080
#define GICD_IGRP_END    0x00FC

#define GICD_ISEN_OFF    0x0100
#define GICD_ISEN_END    0x017C
#define GICD_ICEN_OFF    0x0180
#define GICD_ICEN_END    0x01FC

#define GICD_ISPND_OFF   0x0200
#define GICD_ISPND_END   0x027C
#define GICD_ICPND_OFF   0x0280
#define GICD_ICPND_END   0x02FC

#define GICD_PRIO_OFF     0x0400
#define GICD_PRIO_END     0x07F8

#define GICD_ICFGR_OFF   0x0C00
#define GICD_ICFGR_END   0x0CFC

/* GICR */
/* rd_base */
#define GICR_CTLR_OFF    0x0000
#define GICR_IIDR_OFF    0x0004
#define GICR_TYPE_OFF    0x0008
#define GICR_TYPE_PN_OFF      8
#define GICR_TYPE_AFF_OFF    32
#define GICR_TYPE_LAST_OFF    4
#define GICR_STAT_OFF    0x0010
#define GICR_WAKE_OFF    0x0014
#define MPIDR_AFF_MSK    0xFFFF

/* sgi_base */
#define GICR_SGI_OFF     0x10000  
#define GICR_IGRP0_OFF   0x0080
#define GICR_ISEN0_OFF   0x0100
#define GICR_ICEN0_OFF   0x0180
#define GICR_ISPND0_OFF  0x0200
#define GICR_ICPND0_OFF  0x0280
#define GICR_ISACT_OFF   0x0300
#define GICR_ICACT_OFF   0x0380
#define GICR_PRIO_OFF    0x0400
#define GICR_PRIO_END    0x041C
#define GICR_ICFGR0_OFF  0x0C00
#define GICR_ICFGR1_OFF  0x0C04
#define GICR_IG_MOD0_OFF 0x0D00

/* ICH_x */
#define ICH_VTR_EL2_MASK 0xFFFFFFFF

enum
{
    UPDATE_ISEN = 0,
    UPDATE_ICEN,
    UPDATE_ISPND,
    UPDATE_ICPND,
    UPDATE_PRIO,
    UPDATE_ICFGR,
    UPDATE_MAX,
};


/* vIRQ status */
enum
{
    VIRQ_STATUS_INACTIVE = 0,
    VIRQ_STATUS_PENDING,
    VIRQ_STATUS_ACTIVE,
    VIRQ_STATUS_PENDING_ACTIVE,
};

struct lr_reg 
{
    rt_uint64_t vINTID : 32;  
    rt_uint64_t pINTID : 13;  
    rt_uint64_t res0   :  3;  
    rt_uint64_t prio   :  8;
    rt_uint64_t res1   :  4;  
    rt_uint64_t group  :  1;
    rt_uint64_t hw     :  1;  
    rt_uint64_t state  :  2;  
}; 

/* 
 * Describe vGIC's virtual interrupt
 */
struct virq
{
    rt_uint16_t vINIID; /* 0~127 */
    rt_uint16_t pINTID;

    struct vcpu *vcpu;
    rt_uint64_t aff;    /* affinity for SPI; cpu_id for SGI/PPI */

    // spinlock

    rt_uint8_t state;
    rt_uint8_t prio;
    rt_uint8_t cfg;
    rt_uint8_t lr;

    rt_bool_t in_lr;
    rt_bool_t enable;
    rt_bool_t hw;
};
typedef struct virq *virq_t;

/*
 * Record all SPI interrupt for a VM. 
 * Create and init when create VM.
 */
struct vgicd
{
    rt_uint32_t CTLR;
    rt_uint32_t TYPE;
    rt_uint32_t IIDR;

    struct virq virqs[128];  /* virq array */
    rt_size_t virq_num;
    // spinlock
};
typedef struct vgicd *vgicd_t;

/*
 * Record all SGI/PPI interrupt for a vCPU.
 * Create and init when create vcpu.
 */
struct vgicr
{
    rt_uint32_t CTLR;
    rt_uint32_t IIDR;
    rt_uint32_t TYPE;

    rt_uint32_t lr_bitmap;  /* this bitmap will record idle LR REG in lr_list */
    struct lr_reg lr_list[GIC_LR_LIST_NUM];   /* record vIRQ written into LR */
    struct virq virqs[VIRQ_PRIV_NUM];
    // spinlock
};
typedef struct vgicr *vgicr_t;

struct vgic_context
{
    /* info need to save/restore */
    struct lr_reg ich_lr_el2[MAX_LR_REGS];
    rt_uint32_t ich_ap1r_el2;
	rt_uint32_t icc_sre_el1;
	rt_uint32_t icc_ctlr_el1;
	rt_uint32_t ich_vmcr_el2;
	rt_uint32_t ich_hcr_el2;

    /* vGIC attr info */
	rt_uint32_t ich_vtr_el2;
	rt_uint32_t nr_lr;
	rt_uint32_t nr_pr;
};

struct vgic_info
{
    rt_uint64_t gicd_addr;
    rt_uint64_t gicr_addr;

    rt_uint64_t maintenance_id;
    rt_uint64_t virq_num;
};

struct vgic
{
    struct vgic_info info;

    vgicd_t gicd;
    vgicr_t gicr[MAX_VCPU_NUM];
    rt_uint32_t spi_bitmap;

    struct vgic_context ctxt;

    const struct vgic_ops *ops;
};
typedef struct vgic *vgic_t;

struct vgic_ops
{
    void (*emulate)(gp_regs_t regs, access_info_t acc, rt_bool_t gicd);
    void (*update)(struct vcpu *vcpu, virq_t virq, rt_uint8_t update_id);
    void (*inject)(struct vcpu *vcpu, virq_t virq);
};

typedef void (*vgic_update_t)(struct vcpu *vcpu, virq_t virq);
typedef void (*vgic_gicd_write_t)(rt_uint64_t off, rt_uint64_t *val);
typedef void (*vgic_gicr_sgi_write_t)(rt_uint64_t off, rt_uint64_t *val);

rt_inline rt_bool_t is_virq_hw(virq_t virq)
{   return (virq->hw && (virq->vINIID < VIRQ_PRIV_NUM));    }

rt_inline rt_bool_t is_virq_priv(virq_t virq)
{   return (virq->vINIID < VIRQ_PRIV_NUM);  }

vgic_t vgic_create(void);
void vgicd_init(struct vm *vm, vgicd_t gicd);  /* using when vgic init in init VM */
void vgicr_init(vgicr_t gicr, struct vcpu *vcpu);  /* using when create vcpu */
void vgic_init (struct vm *vm);
void vgic_free (vgic_t v);

void hook_vgic_context_save(vgic_t v);
void hook_vgic_context_restore(vgic_t v);

/* vIRQ Operations */
void vgic_emulate(gp_regs_t regs, access_info_t acc, rt_bool_t gicd);
void vgic_update(struct vcpu *vcpu, virq_t virq, rt_uint8_t update_id);
void vgic_inject(struct vcpu *vcpu, virq_t virq);
#endif  /* BSP_USING_GIC && BSP_USING_GICV3 */

#endif  /* __VGIC_H__ */
