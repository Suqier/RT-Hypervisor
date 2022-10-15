/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-06-18     Suqier       first version
 */

#include <rtconfig.h>
#include <cpuport.h>
#include <bitmap.h>

#include "gicv3.h"
#include "vgic.h"
#include "vm.h"
#include "os.h"

#define MAX_OS_NUM  3

extern const struct os_desc os_img[MAX_OS_NUM];

const static struct vgic_ops vgic_ops = 
{
    .emulate = vgic_emulate,
    .update  = vgic_update,
    .inject  = vgic_inject,
};

/* For vGIC create & init */
vgic_t vgic_create(void)
{
    vgic_t v = (vgic_t)rt_malloc(sizeof(struct vgic));
    vgicd_t gicd = (vgicd_t)rt_malloc(sizeof(struct vgicd));

    if (v == RT_NULL || gicd == RT_NULL)
    {
        rt_free(v);
        rt_free(gicd);
        rt_kputs("[Error] Alloc meomry for vGIC failure.\n");
        return (vgic_t)RT_NULL;
    }
    else
        v->gicd = gicd;

    return v;
}

void vgicd_init(struct vm *vm, vgicd_t gicd)
{
    RT_ASSERT(gicd);
    rt_uint32_t hw_base = platform_get_gic_dist_base();

    /* get from device tree */ 
    gicd->virq_num = vm->os->arch.vgic.virq_num;
    for (rt_size_t i = 0; i < gicd->virq_num - VIRQ_PRIV_NUM; i++)
    {
        gicd->virqs[i].vINIID = i + VIRQ_PRIV_NUM;
        gicd->virqs[i].pINTID = 0;
        gicd->virqs[i].vcpu   = RT_NULL;
        gicd->virqs[i].aff    = ~MPIDR_AFF_MASK;
        gicd->virqs[i].state  = VIRQ_STATUS_INACTIVE;
        gicd->virqs[i].prio   = GIC_LOWEST_PRIO;
        gicd->virqs[i].cfg    = 0;
        gicd->virqs[i].in_lr  = RT_FALSE;
        gicd->virqs[i].enable = RT_FALSE;
        gicd->virqs[i].hw     = RT_FALSE;
    }
    
    rt_uint8_t it_line_num = (gicd->virq_num + 1) / 32 - 1;
    gicd->CTLR = 0;
    gicd->IIDR = (rt_uint32_t)GIC_DIST_IIDR(hw_base);
    gicd->TYPE = ((it_line_num << GICD_TYPE_IT_OFF) & GICD_TYPE_IT_MASK) |
    (((vm->nr_vcpus - 1) << GICD_TYPE_CPU_OFF) & GICD_TYPE_CPU_MASK) |
    (((10 - 1) << GICD_TYPE_IDBITS_OFF) & GICD_TYPE_IDBITS_MASK);
}

void vgicr_init(vgicr_t gicr, struct vcpu *vcpu)
{
    RT_ASSERT(gicr);
    rt_uint32_t hw_base = platform_get_gic_redist_base();

    gicr->CTLR = 0;
    gicr->IIDR = (rt_uint32_t)GIC_RDIST_IIDR(hw_base);
    gicr->TYPE = vcpu->id << GICR_TYPE_PN_OFF | 
    (vcpu->arch->vcpu_ctxt.sys_regs[_MPIDR_EL1] & MPIDR_AFF_MSK) << GICR_TYPE_AFF_OFF |
    !!(vcpu->id == vcpu->vm->nr_vcpus - 1) << GICR_TYPE_LAST_OFF;

    /* get from device tree */ 
    bitmap_init(&gicr->lr_bitmap);
    rt_memset((void *)&gicr->lr_list, 0, GIC_LR_LIST_NUM * sizeof(struct lr_reg));

    for (rt_size_t i = 0; i < VIRQ_PRIV_NUM; i++)
    {
        gicr->virqs[i].vINIID = i;
        gicr->virqs[i].pINTID = 0;
        gicr->virqs[i].vcpu   = vcpu;
        gicr->virqs[i].aff    = vcpu->id;
        gicr->virqs[i].state  = VIRQ_STATUS_INACTIVE;
        gicr->virqs[i].prio   = GIC_LOWEST_PRIO;
        gicr->virqs[i].cfg    = 0;
        gicr->virqs[i].in_lr  = RT_FALSE;
        gicr->virqs[i].enable = RT_FALSE;
        gicr->virqs[i].hw     = RT_FALSE;
    }

    /* For SGIs, this field always indicates edge-triggered. */
    for (rt_size_t i = 0; i < VIRQ_SGI_NUM; i++)
        gicr->virqs[i].cfg = 0b10;
}

static void vgic_info_init(struct vgic_info *info, rt_uint64_t os_idx)
{
    info->gicd_addr = os_img[os_idx].arch.vgic.gicd_addr;
    info->gicr_addr = os_img[os_idx].arch.vgic.gicr_addr;
}

void vgic_init(struct vm *vm)
{
    RT_ASSERT(vm);
    vgic_t v = vm->vgic;
    rt_uint64_t os_idx = vm->os_idx;

    vgicd_t gicd = (vgicd_t)rt_malloc(sizeof(struct vgicd));
    RT_ASSERT(gicd);
    vm->vgic->gicd = gicd;

    vgicd_init(vm, v->gicd);
    for (rt_size_t i = 0; i < vm->nr_vcpus; i++)
    {
        if (vm->vcpus[i])
        {
            vgicr_t gicr = (vgicr_t)rt_malloc(sizeof(struct vgicr));
            RT_ASSERT(gicr);
            vm->vgic->gicr[i] = gicr;
            vgicr_init(v->gicr[i], vm->vcpus[i]);
        }
        else
            break;
    }

    vgic_info_init(&v->info, os_idx);
    rt_memset((void *)&v->ctxt, 0, sizeof(struct vgic_context));
    bitmap_init(&v->spi_bitmap);
    v->ops = &vgic_ops;

    GET_SYS_REG(ICH_VTR_EL2, v->ctxt.ich_vtr_el2);
    v->ctxt.ich_vtr_el2 &= ICH_VTR_EL2_MASK;
    v->ctxt.nr_lr = (v->ctxt.ich_vtr_el2 & LR_REGS_MASK) + 1;
    v->ctxt.nr_pr = ((v->ctxt.ich_vtr_el2 >> PRI_BITS_SHIFT) & PRI_BITS_MASK) + 1;

    v->ctxt.icc_sre_el1  = ICC_SRE_VAL;
    // v->ctxt.icc_ctlr_el1 = ICC_CTLR_EOI;
	v->ctxt.ich_vmcr_el2 = (GROUP1_INT << ICH_VMCR_VENG_OFF) | (ICH_VMPR_VAL << ICH_VMPR_OFF);
	v->ctxt.ich_hcr_el2  = ICH_HCR_EN;
}

void vgic_free(vgic_t v)
{
    v->ops = RT_NULL;
    rt_free(v);
    v = RT_NULL;
}

/* 
 * hook_ Prefix function for switching.
 * 
 * In vGIC, These registers need to save:
 * - ICH_LR<n>_EL2
 * - ICH_APxRx_EL2
 * - ICC_SRE_EL1
 * - ICH_VMCR_EL2
 * - ICH_HCR_EL2
 */
/* for save process */
static void read_lr(struct vgic_context *c, int lr_idx, struct lr_reg *lr)
{
    RT_ASSERT(lr_idx < c->nr_lr);

    switch (lr_idx)
    {
    case 15: GET_GICV3_REG(ICH_LR15_EL2, *lr); break;
    case 14: GET_GICV3_REG(ICH_LR14_EL2, *lr); break;
    case 13: GET_GICV3_REG(ICH_LR13_EL2, *lr); break;
    case 12: GET_GICV3_REG(ICH_LR12_EL2, *lr); break;
    case 11: GET_GICV3_REG(ICH_LR11_EL2, *lr); break;
    case 10: GET_GICV3_REG(ICH_LR10_EL2, *lr); break;
    case  9: GET_GICV3_REG(ICH_LR9_EL2, *lr);  break;
    case  8: GET_GICV3_REG(ICH_LR8_EL2, *lr);  break;
    case  7: GET_GICV3_REG(ICH_LR7_EL2, *lr);  break;
    case  6: GET_GICV3_REG(ICH_LR6_EL2, *lr);  break;
    case  5: GET_GICV3_REG(ICH_LR5_EL2, *lr);  break;
    case  4: GET_GICV3_REG(ICH_LR4_EL2, *lr);  break;
    case  3: GET_GICV3_REG(ICH_LR3_EL2, *lr);  break;
    case  2: GET_GICV3_REG(ICH_LR2_EL2, *lr);  break;
    case  1: GET_GICV3_REG(ICH_LR1_EL2, *lr);  break;
    case  0: GET_GICV3_REG(ICH_LR0_EL2, *lr);  break;
    default: break;
    }
}

static void vgic_context_save_lr(struct vgic_context *c, rt_uint32_t nr_lr)
{
    for (rt_int8_t i = nr_lr - 1; i >= 0; i--)
        read_lr(c, i, &c->ich_lr_el2[i]);
}

static void vgic_context_save_arp(struct vgic_context *c, rt_uint32_t nr_pr)
{
    switch (nr_pr)
    {
	case 7:
        SET_GICV3_REG(ICH_AP1R2_EL2, c->ich_ap1r_el2);
	case 6:
        SET_GICV3_REG(ICH_AP1R1_EL2, c->ich_ap1r_el2);
	case 5:
        SET_GICV3_REG(ICH_AP1R0_EL2, c->ich_ap1r_el2);
		break;
    default:
        break;
    }
}

void hook_vgic_context_save(vgic_t v)
{
    struct vgic_context *c = &v->ctxt;

    vgic_context_save_lr(c, c->nr_lr);
    vgic_context_save_arp(c, c->nr_pr);
    GET_GICV3_REG(ICC_SRE_EL1, c->icc_sre_el1);
    GET_GICV3_REG(ICH_VMCR_EL2, c->ich_vmcr_el2);
    GET_GICV3_REG(ICH_HCR_EL2, c->ich_hcr_el2);
    // GET_GICV3_REG(ICC_CTLR_EL1, c->icc_ctlr_el1);
}

/* for restore process */
static void write_lr(struct vgic_context *c, int lr_idx, struct lr_reg *lr)
{
    RT_ASSERT(lr_idx < c->nr_lr);
    switch (lr_idx)
    {
    case 15: SET_GICV3_REG(ICH_LR15_EL2, *lr); break;
    case 14: SET_GICV3_REG(ICH_LR14_EL2, *lr); break;
    case 13: SET_GICV3_REG(ICH_LR13_EL2, *lr); break;
    case 12: SET_GICV3_REG(ICH_LR12_EL2, *lr); break;
    case 11: SET_GICV3_REG(ICH_LR11_EL2, *lr); break;
    case 10: SET_GICV3_REG(ICH_LR10_EL2, *lr); break;
    case  9: SET_GICV3_REG(ICH_LR9_EL2, *lr);  break;
    case  8: SET_GICV3_REG(ICH_LR8_EL2, *lr);  break;
    case  7: SET_GICV3_REG(ICH_LR7_EL2, *lr);  break;
    case  6: SET_GICV3_REG(ICH_LR6_EL2, *lr);  break;
    case  5: SET_GICV3_REG(ICH_LR5_EL2, *lr);  break;
    case  4: SET_GICV3_REG(ICH_LR4_EL2, *lr);  break;
    case  3: SET_GICV3_REG(ICH_LR3_EL2, *lr);  break;
    case  2: SET_GICV3_REG(ICH_LR2_EL2, *lr);  break;
    case  1: SET_GICV3_REG(ICH_LR1_EL2, *lr);  break;
    case  0: SET_GICV3_REG(ICH_LR0_EL2, *lr);  break;
    default: break;
    }
}

static void vgic_context_restore_lr(struct vgic_context *c, rt_uint32_t nr_lr)
{
    for (rt_int8_t i = nr_lr - 1; i >= 0; i--)
        write_lr(c, i, &c->ich_lr_el2[i]);
    __ISB();
}

static void vgic_context_restore_arp(struct vgic_context *c, rt_uint32_t nr_pr)
{
    switch (nr_pr)
    {
	case 7:
        SET_SYS_REG(ICH_AP1R2_EL2, c->ich_ap1r_el2);
	case 6:
        SET_SYS_REG(ICH_AP1R1_EL2, c->ich_ap1r_el2);
	case 5:
        SET_SYS_REG(ICH_AP1R0_EL2, c->ich_ap1r_el2);
		break;
    default:
        break;
    }
}

void hook_vgic_context_restore(vgic_t v)
{
    struct vgic_context *c = &v->ctxt;

    vgic_context_restore_lr(c, c->nr_lr);
    vgic_context_restore_arp(c, c->nr_pr);
    SET_GICV3_REG(ICC_SRE_EL1 , c->icc_sre_el1);
    SET_GICV3_REG(ICH_VMCR_EL2, c->ich_vmcr_el2);
    SET_GICV3_REG(ICH_HCR_EL2 , c->ich_hcr_el2);
    // SET_GICV3_REG(ICC_CTLR_EL1, c->icc_ctlr_el1);
    __ISB();
}

/*  
 * For vGIC MMIO read - vGIC emulation
 */
static void vgic_gicd_write_isenabler(rt_uint64_t off, rt_uint64_t *val)
{
    vgic_t v = get_curr_vm()->vgic; 
    off = (off - GICD_ISEN_OFF) / 4;
    int irq = 32 * off;

    for (rt_size_t i = 0; i < 32; i++, irq++)
    {
        if (*val & 0b1)  /* write 0 to this has no effect */
        {
            virq_t virq = &v->gicd->virqs[irq - VIRQ_PRIV_NUM];
            virq->enable = RT_TRUE;
            v->ops->update(get_curr_vcpu(), virq, UPDATE_ISEN);
        }
        *val = *val >> 1;
    }
}

static void vgic_gicd_write_icenabler(rt_uint64_t off, rt_uint64_t *val)
{
    vgic_t v = get_curr_vm()->vgic; 
    off = (off - GICD_ICEN_OFF) / 4;
    int irq = 32 * off;

    for (rt_size_t i = 0; i < 32; i++, irq++)
    {
        if (*val & 0b1)  /* write 0 to this has no effect */
        {
            virq_t virq = &v->gicd->virqs[irq - VIRQ_PRIV_NUM];
            virq->enable = RT_FALSE;
            v->ops->update(get_curr_vcpu(), virq, UPDATE_ICEN);
        }

        *val = *val >> 1;
    }
}

static void vgic_gicd_write_ispend(rt_uint64_t off, rt_uint64_t *val)
{
    vgic_t v = get_curr_vm()->vgic; 
    off = (off - GICD_ISPND_OFF) / 4;
    int irq = 32 * off;

    for (rt_size_t i = 0; i < 32; i++, irq++)
    {
        if ((*val & 0b1) && (irq >= 32))  /* write 0 to this has no effect */
        {
            virq_t virq = &v->gicd->virqs[irq - VIRQ_PRIV_NUM];
            if (virq->state == VIRQ_STATUS_INACTIVE)
            {
                virq->state =  VIRQ_STATUS_PENDING;
                v->ops->update(get_curr_vcpu(), virq, UPDATE_ISPND);
            }
            
            if (virq->state == VIRQ_STATUS_ACTIVE)
            {
                virq->state =  VIRQ_STATUS_PENDING_ACTIVE;
                v->ops->update(get_curr_vcpu(), virq, UPDATE_ISPND);
            }
        }

        *val = *val >> 1;
    }
}

static void vgic_gicd_write_icpend(rt_uint64_t off, rt_uint64_t *val)
{
    vgic_t v = get_curr_vm()->vgic;
    off = (off - GICD_ICPND_OFF) / 4;
    int irq = 32 * off;

    for (rt_size_t i = 0; i < 32; i++, irq++)
    {
        if ((*val & 0b1) && (irq >= 32))  /* write 0 to this has no effect */
        {
            virq_t virq = &v->gicd->virqs[irq - VIRQ_PRIV_NUM];
            if (virq->state == VIRQ_STATUS_PENDING)
            {
                virq->state =  VIRQ_STATUS_INACTIVE;
                v->ops->update(get_curr_vcpu(), virq, UPDATE_ICPND);
            }
            
            if (virq->state == VIRQ_STATUS_PENDING_ACTIVE)
            {
                virq->state =  VIRQ_STATUS_ACTIVE;
                v->ops->update(get_curr_vcpu(), virq, UPDATE_ICPND);
            }
        }

        *val = *val >> 1;
    }
}

static void vgic_gicd_write_priority(rt_uint64_t off, rt_uint64_t *val)
{
    vgic_t v = get_curr_vm()->vgic; 
    off = (off - GICD_PRIO_OFF) / 4;
    int irq = 4 * off;

    for (rt_size_t i = 0; i < 4; i++, irq++)
    {
        virq_t virq = &v->gicd->virqs[irq - VIRQ_PRIV_NUM];
        virq->prio = *val & 0xFFUL;
        v->ops->update(get_curr_vcpu(), virq, UPDATE_PRIO);
        *val = *val >> 8;
    }
}

static void vgic_gicd_write_icfgr(rt_uint64_t off, rt_uint64_t *val)
{
    off = (off - GICD_ICFGR_OFF) / 4;
    int irq = 16 * off;
    vgic_t v = get_curr_vm()->vgic; 

    for (rt_size_t i = 0; i < 16; i++, irq++)
    {
        virq_t virq = &v->gicd->virqs[irq - VIRQ_PRIV_NUM];
        virq->cfg = *val & 0b11;
        v->ops->update(get_curr_vcpu(), virq, UPDATE_ICFGR);
        *val = *val >> 2;
    }
}

static vgic_gicd_write_t vgic_gicd_write_handlers[UPDATE_MAX] =
{
    [UPDATE_ISEN]  = &vgic_gicd_write_isenabler,
    [UPDATE_ICEN]  = &vgic_gicd_write_icenabler,
    [UPDATE_ISPND] = &vgic_gicd_write_ispend,
    [UPDATE_ICPND] = &vgic_gicd_write_icpend,
    [UPDATE_PRIO]  = &vgic_gicd_write_priority,
    [UPDATE_ICFGR] = &vgic_gicd_write_icfgr,
};

static void vgic_gicd_write_emulate(rt_uint64_t off, rt_uint64_t *val)
{
    vgic_t v = get_curr_vm()->vgic;
    rt_uint8_t update_id = UPDATE_MAX;

    switch (off)
    {
    case GICD_CTLR_OFF:
        *val = v->gicd->CTLR;
        break;
    case GICD_ISEN_OFF...GICD_ISEN_END:
        update_id = UPDATE_ISEN;
        break;
    case GICD_ICEN_OFF...GICD_ICEN_END:
        update_id = UPDATE_ICEN;
        break;
    case GICD_ISPND_OFF...GICD_ISPND_END:
        update_id = UPDATE_ISPND;
        break;
    case GICD_ICPND_OFF...GICD_ICPND_END:
        update_id = UPDATE_ICPND;
        break;
    case GICD_PRIO_OFF...GICD_PRIO_END:
        update_id = UPDATE_PRIO;
        break;
    case GICD_ICFGR_OFF...GICD_ICFGR_END:
        update_id = UPDATE_ICFGR;
        break;

    default:
        break;
    }

    if (update_id < UPDATE_MAX)
    {
        vgic_gicd_write_t handler = vgic_gicd_write_handlers[update_id];
        handler(off, val);
    }
}

static void vgic_gicr_rd_emulate(rt_uint64_t off, access_info_t acc, rt_uint64_t *val)
{
    vgic_t v = get_curr_vm()->vgic;
    rt_uint32_t vcpu_id = get_curr_vcpu()->id;

    switch (off)
    {
    case GICR_CTLR_OFF:
    {
        if (acc.is_write)
            v->gicr[vcpu_id]->CTLR = *val;
        else
            *val = v->gicr[vcpu_id]->CTLR;
    } break;
    case GICR_IIDR_OFF:
        if (!acc.is_write)
            *val = v->gicr[vcpu_id]->IIDR;
        break;
    case GICR_TYPE_OFF:
        if (!acc.is_write)
            *val = v->gicr[vcpu_id]->TYPE;
        break;

    default:
       *val = 0;
        break;
    }
}

static void vgic_gicr_sgi_write_isenabler(rt_uint64_t off, rt_uint64_t *val)
{
    vgic_t v = get_curr_vm()->vgic; 
    rt_uint32_t vcpu_id = get_curr_vcpu()->id;

    for (rt_size_t i = 0; i < 32; i++)
    {
        virq_t virq = &v->gicr[vcpu_id]->virqs[i];
        if (*val & 0b1)
        {
            virq->enable = RT_TRUE;
            v->ops->update(get_curr_vcpu(), virq, UPDATE_ISEN);
        }

        *val = *val >> 1;
    }
}

static void vgic_gicr_sgi_write_icenabler(rt_uint64_t off, rt_uint64_t *val)
{
    vgic_t v = get_curr_vm()->vgic; 
    rt_uint32_t vcpu_id = get_curr_vcpu()->id;

    for (rt_size_t i = 0; i < 32; i++)
    {
        virq_t virq = &v->gicr[vcpu_id]->virqs[i];
        if (*val & 0b1)
        {
            virq->enable = RT_FALSE;
            v->ops->update(get_curr_vcpu(), virq, UPDATE_ICEN);
        }
        
        *val = *val >> 1;
    }
}

static void vgic_gicr_sgi_write_ispend(rt_uint64_t off, rt_uint64_t *val)
{
    vgic_t v = get_curr_vm()->vgic; 
    rt_uint32_t vcpu_id = get_curr_vcpu()->id;

    for (rt_size_t i = 0; i < 32; i++)
    {
        if (*val & 0b1)
        {
            virq_t virq = &v->gicr[vcpu_id]->virqs[i];

            if (virq->state == VIRQ_STATUS_INACTIVE)
            {
                virq->state =  VIRQ_STATUS_PENDING;
                v->ops->update(get_curr_vcpu(), virq, UPDATE_ISPND);
            }
            
            if (virq->state == VIRQ_STATUS_ACTIVE)
            {
                virq->state =  VIRQ_STATUS_PENDING_ACTIVE;
                v->ops->update(get_curr_vcpu(), virq, UPDATE_ISPND);
            }
        }

        *val = *val >> 1;
    }
}

static void vgic_gicr_sgi_write_icpend(rt_uint64_t off, rt_uint64_t *val)
{
    vgic_t v = get_curr_vm()->vgic; 
    rt_uint32_t vcpu_id = get_curr_vcpu()->id;

    for (rt_size_t i = 0; i < 32; i++)
    {
        if (*val & 0b1)
        {
            virq_t virq = &v->gicr[vcpu_id]->virqs[i];

            if (virq->state == VIRQ_STATUS_PENDING)
            {
                virq->state =  VIRQ_STATUS_INACTIVE;
                v->ops->update(get_curr_vcpu(), virq, UPDATE_ICPND);
            }
            
            if (virq->state == VIRQ_STATUS_PENDING_ACTIVE)
            {
                virq->state =  VIRQ_STATUS_ACTIVE;
                v->ops->update(get_curr_vcpu(), virq, UPDATE_ICPND);
            }
        }

        *val = *val >> 1;
    }
}

static void vgic_gicr_sgi_write_priority(rt_uint64_t off, rt_uint64_t *val)
{
    off = (off - GICR_PRIO_OFF) / 4;
    int irq = 4 * off;
    vgic_t v = get_curr_vm()->vgic; 
    rt_uint32_t vcpu_id = get_curr_vcpu()->id;

    for (rt_size_t i = 0; i < 4; i++, irq++)
    {
        virq_t virq = &v->gicr[vcpu_id]->virqs[i];
        virq->prio = *val & 0xFFUL;
        v->ops->update(get_curr_vcpu(), virq, UPDATE_PRIO);
        *val = *val >> 8;
    }
}

static void vgic_gicr_sgi_write_icfgr(rt_uint64_t off, rt_uint64_t *val)
{
    off = (off - GICR_ICFGR0_OFF) / 4;
    int irq = 16 * off;
    vgic_t v = get_curr_vm()->vgic; 
    rt_uint32_t vcpu_id = get_curr_vcpu()->id;

    for (rt_size_t i = 0; i < 16; i++, irq++)
    {
        virq_t virq = &v->gicr[vcpu_id]->virqs[i];
        virq->cfg = *val & 0b11;
        v->ops->update(get_curr_vcpu(), virq, UPDATE_ICFGR);
        *val = *val >> 2;
    }
}

static vgic_gicr_sgi_write_t vgic_gicr_sgi_write_handler[UPDATE_MAX] =
{
    [UPDATE_ISEN]  = &vgic_gicr_sgi_write_isenabler,
    [UPDATE_ICEN]  = &vgic_gicr_sgi_write_icenabler,
    [UPDATE_ISPND] = &vgic_gicr_sgi_write_ispend,
    [UPDATE_ICPND] = &vgic_gicr_sgi_write_icpend,
    [UPDATE_PRIO]  = &vgic_gicr_sgi_write_priority,
    [UPDATE_ICFGR] = &vgic_gicr_sgi_write_icfgr,
};

static void vgic_gicr_sgi_emulate(rt_uint64_t off, access_info_t acc, rt_uint64_t *val)
{
    rt_uint8_t update_id = UPDATE_MAX;

    if (acc.is_write)
    {
        switch (off)
        {
        case GICR_ISEN0_OFF:
            update_id = UPDATE_ISEN;
            break;
        case GICR_ICEN0_OFF:
            update_id = UPDATE_ICEN;
            break;
        case GICR_ISPND0_OFF:
            update_id = UPDATE_ISPND;
            break;
        case GICR_ICPND0_OFF:
            update_id = UPDATE_ICPND;
            break;
        case GICR_PRIO_OFF...GICR_PRIO_END:
            update_id = UPDATE_PRIO;
            break;
        case GICR_ICFGR0_OFF...GICR_ICFGR1_OFF:
            update_id = UPDATE_ICFGR;
            break;

        default:
            break;
        }
    }

    if (update_id < UPDATE_MAX)
    {
        vgic_gicr_sgi_write_t handler = vgic_gicr_sgi_write_handler[update_id];
        handler(off, val);
    }
}

void vgic_emulate(gp_regs_t regs, access_info_t acc, rt_bool_t gicd)
{
    vgic_t v = get_curr_vm()->vgic;
    unsigned long long *val = regs_xn(regs, acc.srt);

    if (gicd)
    {
        rt_uint64_t off = acc.addr - v->info.gicd_addr;
        
        if (acc.is_write)
            vgic_gicd_write_emulate(off, (rt_uint64_t *)val);
        else    /* read */
        {   
            switch (off)
            {
            case GICD_CTLR_OFF:
                *val = v->gicd->CTLR;
                break;
            case GICD_TYPE_OFF:
                *val = v->gicd->TYPE;
                break;
            case GICD_IIDR_OFF:
                *val = v->gicd->IIDR;
                break;

            default:
                *val = 0;
                break;
            }
        }
    }
    else    /* gicr */
    {
        rt_uint64_t off = acc.addr - v->info.gicr_addr;
        if (off < GICR_SGI_OFF)
            vgic_gicr_rd_emulate(off, acc, (rt_uint64_t *)val);
        else
            vgic_gicr_sgi_emulate(off - GICR_SGI_OFF, acc, (rt_uint64_t *)val);
    }
}

/*
 * vGIC Update
 */
static void vgic_update_isenabler(struct vcpu *vcpu, virq_t virq)
{
    arm_gic_umask(0, virq->pINTID);
}

static void vgic_update_icenabler(struct vcpu *vcpu, virq_t virq)
{
    arm_gic_mask(0, virq->pINTID);
}

static void vgic_update_ispend(struct vcpu *vcpu, virq_t virq)
{
    arm_gic_set_pending_irq(0, virq->pINTID);
}

static void vgic_update_icpend(struct vcpu *vcpu, virq_t virq)
{
    arm_gic_clear_pending_irq(0, virq->pINTID);
}

static void vgic_update_priority(struct vcpu *vcpu, virq_t virq)
{
    arm_gic_set_priority(0, virq->pINTID, virq->prio);
}

static void vgic_update_icfgr(struct vcpu *vcpu, virq_t virq)
{
    arm_gic_set_configuration(0, virq->pINTID, virq->cfg);
}

static vgic_update_t vgic_update_handlers[UPDATE_MAX] =
{
    [UPDATE_ISEN]  = &vgic_update_isenabler,
    [UPDATE_ICEN]  = &vgic_update_icenabler,
    [UPDATE_ISPND] = &vgic_update_ispend,
    [UPDATE_ICPND] = &vgic_update_icpend,
    [UPDATE_PRIO]  = &vgic_update_priority,
    [UPDATE_ICFGR] = &vgic_update_icfgr,
};

void vgic_update(struct vcpu *vcpu, virq_t virq, rt_uint8_t update_id)
{
    if (virq->hw)
    {
        vgic_update_t handler = vgic_update_handlers[update_id];
        handler(vcpu, virq);
    }
}

/*
 * vGIC Inject vIRQ
 */
static rt_uint64_t vgic_get_hcr(void)
{
    rt_uint64_t val;
    GET_GICV3_REG(ICH_HCR_EL2, val);
    return val;
}

static void vgic_set_hcr(rt_uint64_t val)
{
    SET_GICV3_REG(ICH_HCR_EL2, val);
}

void vgic_virq_register(struct vm *vm)
{
    /* Associated Physical Interrupts */
    const struct devs_info *devs = &os_img[vm->os_idx].devs;

    for (rt_size_t i = 0; i < devs->num; i++)
    {
        rt_uint8_t int_num = devs->dev[i].interrupt_num;
        
        for (rt_size_t j = 0; j < int_num; j++)
        {
            rt_uint64_t virq_id = *devs->dev[i].interrupts;

            if (virq_id < VIRQ_PRIV_NUM)   /* SGI + PPI */
                continue;
            else    /* SPI */
            {
                virq_t virq = &vm->vgic->gicd->virqs[virq_id];
                virq->hw = RT_TRUE;
            }
        }
    }
}

static rt_size_t vgic_find_lr(struct vcpu *vcpu)
{
    rt_size_t idle_lr_idx = LR_NO_USE;
    rt_uint64_t idle_lr;
    GET_GICV3_REG(ICH_ELRSR_EL2, idle_lr);

    for (rt_size_t i = 0; i < vcpu->vm->vgic->ctxt.nr_lr; i++) 
    {
        if (bit_get(idle_lr, i))
        {
            idle_lr_idx = i;
            continue;
        }
    }

    return idle_lr_idx;
}

/*
 * Pick one vIRQ which can be exchanged out from LR list now,
 * return it's index in LR list.
 */
static rt_size_t vgic_pick_lr(struct vcpu *vcpu, virq_t virq)
{
    struct lr_reg lr;
    rt_size_t   idle_lr_idx   =  0;
    rt_uint16_t min_prio_pend =  0, min_prio_act =  0;
    rt_uint16_t pend_found    =  0, act_found    =  0;
    rt_int16_t  pend_ind      = -1, act_ind      = -1;

    for (; idle_lr_idx < vcpu->vm->vgic->ctxt.nr_lr; idle_lr_idx++)
    {
        read_lr(&vcpu->vm->vgic->ctxt, idle_lr_idx, &lr);

        if (lr.vINTID && lr.state == VIRQ_STATUS_INACTIVE)
            return idle_lr_idx;

        if (lr.vINTID && lr.state == VIRQ_STATUS_ACTIVE) 
        {
            if (lr.prio > min_prio_act) 
            {
                min_prio_act = lr.prio;
                act_ind = idle_lr_idx;
            }
            pend_found++;
        } 
        
        if (lr.vINTID && lr.state == VIRQ_STATUS_PENDING) 
        {
            if (lr.prio > min_prio_pend)
            {
                min_prio_pend = lr.prio;
                pend_ind = idle_lr_idx;
            }
            act_found++;
        }
    }

    if (pend_found > 1)
        idle_lr_idx = pend_ind;
    else
        idle_lr_idx = act_ind;

    return idle_lr_idx;
}

static rt_int8_t vgic_is_lr_in_list(struct vcpu *vcpu, struct lr_reg *lr)
{
    vgicr_t gicr = vcpu->vm->vgic->gicr[vcpu->id];
    for (rt_int8_t i = 0; i < 32; i++)
    {
        int bit = bitmap_get_bit(&gicr->lr_bitmap, i);
        if (bit)
        {
            struct lr_reg lr_in_list = gicr->lr_list[i];
            if (lr_in_list.vINTID == lr->vINTID)
                return i;
        }
    }
    return -1;
}

/*
 * According LR index return from vgic_pick_lr(), 
 * remove corresponding vIRQ int the LR list and save it into memory.
 */
static void vgic_remove_lr(struct vcpu *vcpu, rt_size_t idle_lr_idx)
{
    struct lr_reg lr;
    read_lr(&vcpu->vm->vgic->ctxt, idle_lr_idx, &lr);
    write_lr(&vcpu->vm->vgic->ctxt, idle_lr_idx, (struct lr_reg *)(0));

    rt_size_t virq_id = lr.vINTID;
    vgicr_t gicr = vcpu->vm->vgic->gicr[vcpu->id];
    int n = bitmap_find_next(&gicr->lr_bitmap);
    bitmap_set_bit(&gicr->lr_bitmap, n);
    gicr->lr_list[n] = lr;
    gicr->virqs[virq_id].in_lr = RT_FALSE;

    if (lr.state != VIRQ_STATUS_INACTIVE)  /* set maintenance interrupt */
        vgic_set_hcr(vgic_get_hcr() | ICH_HCR_NPIE);
}

/* 
 * Add a vIRQ into LR list. 
 * In this function, we can say idle_lr_idx is OK.
 */
static void vgic_add_lr(struct vcpu *vcpu, virq_t virq, rt_size_t idle_lr_idx)
{
    if (virq->in_lr || !virq->enable)
        return;

    struct lr_reg lr;
    lr.group  = GROUP1_INT;
    lr.hw     = virq->hw;
    lr.vINTID = virq->vINIID;
    lr.prio   = virq->prio;
    lr.state  = virq->state;
    if (lr.hw)
        lr.pINTID = virq->pINTID;

    write_lr(&vcpu->vm->vgic->ctxt, idle_lr_idx, &lr);

    vgicr_t gicr = vcpu->vm->vgic->gicr[vcpu->id];
    int i = vgic_is_lr_in_list(vcpu, &lr);
    if (i > 0)
    {
        /* remove it from LR list */
        rt_memset((void *)&gicr->lr_list[i], 0, sizeof(struct lr_reg));
    }
    gicr->virqs[lr.vINTID].in_lr = RT_TRUE;
    gicr->virqs[lr.vINTID].lr    = idle_lr_idx;
}

void vgic_inject(struct vcpu *vcpu, virq_t virq)
{
    /* 
     * Host find a idle LR register and write into. 
     * If there's no idle LR register, call maintenance interrupt.
     * If there's no pending interrupt in LR register, call maintenance interrupt.
     */
    rt_size_t idle_lr_idx = vgic_find_lr(vcpu);
    
    if(idle_lr_idx == LR_NO_USE)
    {
        idle_lr_idx = vgic_pick_lr(vcpu, virq);
        vgic_remove_lr(vcpu, idle_lr_idx);
    }

    if (idle_lr_idx != LR_NO_USE)
    {
        vgic_add_lr(vcpu, virq, idle_lr_idx);
        vcpu_go(vcpu);      /* kick vcpu to handler this vIRQ */
    }
    else
    {
        /* set maintenance interrupt */
        vgic_set_hcr(vgic_get_hcr() | ICH_HCR_NPIE);
    }
}
