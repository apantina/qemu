/*
 * QEMU model of the XPPU
 *
 * Copyright (c) 2014 - 2020 Xilinx Inc.
 *
 * Autogenerated by xregqemu.py 2019-12-05.
 * Written by Edgar E. Iglesias <edgar.iglesias@xilinx.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "hw/register.h"
#include "qemu/bitops.h"
#include "sysemu/dma.h"
#include "qapi/error.h"
#include "qemu/log.h"
#include "migration/vmstate.h"
#include "hw/qdev-properties.h"

#include "hw/fdt_generic_util.h"
#include "hw/misc/xlnx-xppu.h"

#define TYPE_XILINX_XPPU "xlnx.xppu"

#define XILINX_XPPU(obj) \
     OBJECT_CHECK(XPPU, (obj), TYPE_XILINX_XPPU)

/*
 * Register definitions shared between ZynqMP and Versal are in
 * the XPPU header file
 */
    FIELD(ERR_STATUS1, AXI_ADDR, 0, 20)
REG32(POISON, 0xc)
    FIELD(POISON, BASE, 0, 20)
REG32(M_APERTURE_32B, 0x40)
REG32(BASE_32B, 0x50)
REG32(RAM_ADJ, 0x1fc)
    FIELD(RAM_ADJ, MESSAGE_EMAS, 13, 1)
    FIELD(RAM_ADJ, MESSAGE_EMAW, 11, 2)
    FIELD(RAM_ADJ, MESSAGE_EMA, 8, 3)
    FIELD(RAM_ADJ, PERMISSION_EMAS, 5, 1)
    FIELD(RAM_ADJ, PERMISSION_EMAW, 3, 2)
    FIELD(RAM_ADJ, PERMISSION_EMA, 0, 3)

static void isr_postw(RegisterInfo *reg, uint64_t val64)
{
    XPPU *s = XILINX_XPPU(reg->opaque);
    isr_update_irq(s);
}

static uint64_t ien_prew(RegisterInfo *reg, uint64_t val64)
{
    XPPU *s = XILINX_XPPU(reg->opaque);
    uint32_t val = val64;

    s->regs[R_IMR] &= ~val;
    isr_update_irq(s);
    return 0;
}

static uint64_t ids_prew(RegisterInfo *reg, uint64_t val64)
{
    XPPU *s = XILINX_XPPU(reg->opaque);
    uint32_t val = val64;

    s->regs[R_IMR] |= val;
    isr_update_irq(s);
    return 0;
}

static void ctrl_postw(RegisterInfo *reg, uint64_t val64)
{
    XPPU *s = XILINX_XPPU(reg->opaque);
    update_mrs(s);
    check_mid_parities(s);
    isr_update_irq(s);
}

static void mid_postw(RegisterInfo *reg, uint64_t val64)
{
    XPPU *s = XILINX_XPPU(reg->opaque);
    check_mid_parity(s, val64);
    isr_update_irq(s);
}

static const RegisterAccessInfo xppu_regs_info[] = {
    {   .name = "CTRL",  .addr = A_CTRL,
        .rsvd = 0xfffffff8,
        .ro = 0xfffffff8,
        .post_write = ctrl_postw,
    },{ .name = "ERR_STATUS1",  .addr = A_ERR_STATUS1,
        .rsvd = 0xfff00000,
        .ro = 0xffffffff,
    },{ .name = "ERR_STATUS2",  .addr = A_ERR_STATUS2,
        .rsvd = 0xfffffc00,
        .ro = 0xffffffff,
    },{ .name = "POISON",  .addr = A_POISON,
        .reset = 0xff9c0,
        .rsvd = 0xfff00000,
        .ro = 0xffffffff,
    },{ .name = "ISR",  .addr = A_ISR,
        .rsvd = 0xffffff10,
        .ro = 0xffffff10,
        .w1c = 0xef,
        .post_write = isr_postw,
    },{ .name = "IMR",  .addr = A_IMR,
        .reset = 0xef,
        .rsvd = 0xffffff10,
        .ro = 0xffffffff,
    },{ .name = "IEN",  .addr = A_IEN,
        .rsvd = 0xffffff10,
        .ro = 0xffffff10,
        .pre_write = ien_prew,
    },{ .name = "IDS",  .addr = A_IDS,
        .rsvd = 0xffffff10,
        .ro = 0xffffff10,
        .pre_write = ids_prew,
    },{ .name = "M_MASTER_IDS",  .addr = A_M_MASTER_IDS,
        .reset = 0x14,
        .ro = 0xffffffff,
    },{ .name = "M_APERTURE_32B",  .addr = A_M_APERTURE_32B,
        .reset = 0x80,
        .ro = 0xffffffff,
    },{ .name = "M_APERTURE_64KB",  .addr = A_M_APERTURE_64KB,
        .reset = 0x100,
        .ro = 0xffffffff,
    },{ .name = "M_APERTURE_1MB",  .addr = A_M_APERTURE_1MB,
        .reset = 0x10,
        .ro = 0xffffffff,
    },{ .name = "M_APERTURE_512MB",  .addr = A_M_APERTURE_512MB,
        .reset = 0x1,
        .ro = 0xffffffff,
    },{ .name = "BASE_32B",  .addr = A_BASE_32B,
        .reset = 0xff990000,
        .ro = 0xffffffff,
    },{ .name = "BASE_64KB",  .addr = A_BASE_64KB,
        .reset = 0xff000000,
        .ro = 0xffffffff,
    },{ .name = "BASE_1MB",  .addr = A_BASE_1MB,
        .reset = 0xfe000000,
        .ro = 0xffffffff,
    },{ .name = "BASE_512MB",  .addr = A_BASE_512MB,
        .reset = 0xc0000000,
        .ro = 0xffffffff,
    },{ .name = "MASTER_ID00",  .addr = A_MASTER_ID00,
        .reset = 0x83ff0040,
        .rsvd = 0x3c00fc00,
        .ro = 0x3c00fc00,
        .post_write = mid_postw,
    },{ .name = "MASTER_ID01",  .addr = A_MASTER_ID01,
        .reset = 0x3f00000,
        .rsvd = 0x3c00fc00,
        .ro = 0x3c00fc00,
        .post_write = mid_postw,
    },{ .name = "MASTER_ID02",  .addr = A_MASTER_ID02,
        .reset = 0x83f00010,
        .rsvd = 0x3c00fc00,
        .ro = 0x3c00fc00,
        .post_write = mid_postw,
    },{ .name = "MASTER_ID03",  .addr = A_MASTER_ID03,
        .reset = 0x83c00080,
        .rsvd = 0x3c00fc00,
        .ro = 0x3c00fc00,
        .post_write = mid_postw,
    },{ .name = "MASTER_ID04",  .addr = A_MASTER_ID04,
        .reset = 0x83c30080,
        .rsvd = 0x3c00fc00,
        .ro = 0x3c00fc00,
    },{ .name = "MASTER_ID05",  .addr = A_MASTER_ID05,
        .reset = 0x3c30081,
        .rsvd = 0x3c00fc00,
        .ro = 0x3c00fc00,
    },{ .name = "MASTER_ID06",  .addr = A_MASTER_ID06,
        .reset = 0x3c30082,
        .rsvd = 0x3c00fc00,
        .ro = 0x3c00fc00,
    },{ .name = "MASTER_ID07",  .addr = A_MASTER_ID07,
        .reset = 0x83c30083,
        .rsvd = 0x3c00fc00,
        .ro = 0x3c00fc00,
    },{ .name = "MASTER_ID08",  .addr = A_MASTER_ID08,
        .rsvd = 0x3c00fc00,
        .ro = 0x3c00fc00,
    },{ .name = "MASTER_ID09",  .addr = A_MASTER_ID09,
        .rsvd = 0x3c00fc00,
        .ro = 0x3c00fc00,
    },{ .name = "MASTER_ID10",  .addr = A_MASTER_ID10,
        .rsvd = 0x3c00fc00,
        .ro = 0x3c00fc00,
    },{ .name = "MASTER_ID11",  .addr = A_MASTER_ID11,
        .rsvd = 0x3c00fc00,
        .ro = 0x3c00fc00,
    },{ .name = "MASTER_ID12",  .addr = A_MASTER_ID12,
        .rsvd = 0x3c00fc00,
        .ro = 0x3c00fc00,
    },{ .name = "MASTER_ID13",  .addr = A_MASTER_ID13,
        .rsvd = 0x3c00fc00,
        .ro = 0x3c00fc00,
    },{ .name = "MASTER_ID14",  .addr = A_MASTER_ID14,
        .rsvd = 0x3c00fc00,
        .ro = 0x3c00fc00,
    },{ .name = "MASTER_ID15",  .addr = A_MASTER_ID15,
        .rsvd = 0x3c00fc00,
        .ro = 0x3c00fc00,
    },{ .name = "MASTER_ID16",  .addr = A_MASTER_ID16,
        .rsvd = 0x3c00fc00,
        .ro = 0x3c00fc00,
    },{ .name = "MASTER_ID17",  .addr = A_MASTER_ID17,
        .rsvd = 0x3c00fc00,
        .ro = 0x3c00fc00,
    },{ .name = "MASTER_ID18",  .addr = A_MASTER_ID18,
        .rsvd = 0x3c00fc00,
        .ro = 0x3c00fc00,
    },{ .name = "MASTER_ID19",  .addr = A_MASTER_ID19,
        .rsvd = 0x3c00fc00,
        .ro = 0x3c00fc00,
    },{ .name = "RAM_ADJ",  .addr = A_RAM_ADJ,
        .reset = 0xb0b,
        .rsvd = 0xffffc0c0,
        .ro = 0xffffc0c0,
    }
};

static void xppu_reset(DeviceState *dev)
{
    XPPU *s = XILINX_XPPU(dev);
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(s->regs_info); ++i) {
        register_reset(&s->regs_info[i]);
    }
    update_mrs(s);
    isr_update_irq(s);
}

static void xppu_ap_access(void *opaque, hwaddr addr, uint64_t *value, bool rw,
                           unsigned size, MemTxAttrs attr)
{
    XPPUAperture *ap = opaque;
    XPPU *s = ap->parent;
    uint32_t ram_offset;
    uint32_t apl;
    bool valid;
    bool isr_free;
    bool xppu_enabled = ARRAY_FIELD_EX32(s->regs, CTRL, ENABLE);

    assert(xppu_enabled);

    addr += ap->base;

    /* If any of bits ISR[7:1] are set, we cant store new faults.  */
    isr_free = (s->regs[R_ISR] & 0xf6) == 0;

    ram_offset = addr & ap->extract_mask;
    ram_offset >>= ap->extract_shift;

    ram_offset += ap->ram_base;
    apl = s->perm_ram[ram_offset];
    valid = xppu_ap_check(s, attr, rw, apl);

    if (!valid) {
        if (isr_free) {
            ARRAY_FIELD_DP32(s->regs, ISR, APER_PERM, true);
            ARRAY_FIELD_DP32(s->regs, ERR_STATUS1, AXI_ADDR, addr >> 12);
            ARRAY_FIELD_DP32(s->regs, ERR_STATUS2, AXI_ID, attr.requester_id);
        }

        /* Poison the transaction.
         *
         * Bits 11:0  remain untouched.
         * Bits 31:12 are taken from the POISONBASE register
         * Bits 48:32 are zeroed.
         */
        addr &= (1 << 12) - 1;
        addr |= ARRAY_FIELD_EX32(s->regs, POISON, BASE) << 12;
        isr_update_irq(s);
    }

    /* The access is accepted, let it through.  */
    *value = cpu_to_le64(*value);
    address_space_rw(&s->as, addr, attr, (uint8_t *)value, size, rw);
    *value = le64_to_cpu(*value);
}

static MemTxResult xppu_ap_read(void *opaque, hwaddr addr, uint64_t *value,
                                unsigned size, MemTxAttrs attr)
{
    xppu_ap_access(opaque, addr, value, false, size, attr);

    return MEMTX_OK;
}

static MemTxResult xppu_ap_write(void *opaque, hwaddr addr, uint64_t value,
                                 unsigned size, MemTxAttrs attr)
{
    xppu_ap_access(opaque, addr, &value, true, size, attr);

    return MEMTX_OK;
}

static const MemoryRegionOps xppu_ap_ops = {
    .read_with_attrs = xppu_ap_read,
    .write_with_attrs = xppu_ap_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

static XPPU *xppu_from_mr(void *mr_accessor)
{
    RegisterInfoArray *reg_array = mr_accessor;
    Object *obj;

    assert(reg_array != NULL);

    obj = reg_array->mem.owner;
    assert(obj);

    return XILINX_XPPU(obj);
}

static MemTxResult xppu_read(void *opaque, hwaddr addr, uint64_t *value,
                             unsigned size, MemTxAttrs attr)
{
    XPPU *s = xppu_from_mr(opaque);

    if (!attr.secure) {
        return MEMTX_ERROR;
    }

    return xppu_read_common(opaque, s, addr, value, size, attr);
}

static MemTxResult xppu_write(void *opaque, hwaddr addr, uint64_t value,
                       unsigned size, MemTxAttrs attr)
{
    XPPU *s = xppu_from_mr(opaque);

    return xppu_write_common(opaque, s, addr, value, size, attr);
}

static MemTxResult xppu_perm_ram_read(void *opaque, hwaddr addr, uint64_t *val,
                             unsigned size, MemTxAttrs attr)
{
    XPPU *s = XILINX_XPPU(opaque);

    if (!attr.secure) {
        return MEMTX_ERROR;
    }

    return xppu_perm_ram_read_common(s, addr, val, size, attr);
}

static MemTxResult xppu_perm_ram_write(void *opaque, hwaddr addr, uint64_t val,
                              unsigned size, MemTxAttrs attr)
{
    XPPU *s = XILINX_XPPU(opaque);

    return xppu_perm_ram_write_common(s, addr, val, size, attr);
}

static const MemoryRegionOps xppu_ops = {
    .read_with_attrs = xppu_read,
    .write_with_attrs = xppu_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

static const MemoryRegionOps xppu_perm_ram_ops = {
    .read_with_attrs = xppu_perm_ram_read,
    .write_with_attrs = xppu_perm_ram_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

static void xppu_realize(DeviceState *dev, Error **errp)
{
    XPPU *s = XILINX_XPPU(dev);

    address_space_init(&s->as, s->mr ? s->mr : get_system_memory(),
                       object_get_canonical_path(OBJECT(dev)));
}

static void xppu_init(Object *obj)
{
    XPPU *s = XILINX_XPPU(obj);

    s->num_ap = 4;
    s->ap = g_new(XPPUAperture, s->num_ap);

    xppu_init_common(s, obj, TYPE_XILINX_XPPU, &xppu_ops, &xppu_perm_ram_ops,
                     xppu_regs_info, ARRAY_SIZE(xppu_regs_info));
}

static bool xppu_parse_reg(FDTGenericMMap *obj, FDTGenericRegPropInfo reg,
                           Error **errp)
{
    XPPU *s = XILINX_XPPU(obj);
    XPPUApertureInfo ap_info;

    static const XPPUGranule granules[] = {
        GRANULE_32B,
        GRANULE_64K,
        GRANULE_1M,
        GRANULE_512M
    };
    static const uint64_t bases[] = {
        0xff990000,
        0xff000000,
        0xfe000000,
        0xc0000000,
    };
    static const uint64_t masks[] = {
        0x7f << 5,  /* 32B, bits 11:05.  */
        0xff << 16, /* 64K, bits 23:16.  */
        0x0f << 20, /* 1MB, bits 23:20.  */
        0, /* No extraction.  */
    };
    static const unsigned int shifts[] = {
        5,  /* 32B, bits 11:05.  */
        16, /* 64K, bits 23:16.  */
        20, /* 1MB, bits 23:20.  */
        0, /* No extraction.  */
    };
    static const uint32_t ram_bases[] = {
        0x100,
        0x0,
        0x180,
        0x190,
    };

    ap_info.masks = masks;
    ap_info.shifts = shifts;
    ap_info.ram_bases = ram_bases;
    ap_info.granules = granules;
    ap_info.bases = bases;

    return xppu_parse_reg_common(s, TYPE_XILINX_XPPU, reg, obj, &ap_info,
                                 &xppu_ap_ops, errp);
}

static const VMStateDescription vmstate_xppu = {
    .name = TYPE_XILINX_XPPU,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(regs, XPPU, XPPU_R_MAX),
        VMSTATE_END_OF_LIST(),
    }
};

static void xppu_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    FDTGenericMMapClass *fmc = FDT_GENERIC_MMAP_CLASS(klass);

    dc->reset = xppu_reset;
    dc->realize = xppu_realize;
    dc->vmsd = &vmstate_xppu;
    fmc->parse_reg = xppu_parse_reg;
}

static const TypeInfo xppu_info = {
    .name          = TYPE_XILINX_XPPU,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(XPPU),
    .class_init    = xppu_class_init,
    .instance_init = xppu_init,
    .interfaces    = (InterfaceInfo[]) {
        { TYPE_FDT_GENERIC_MMAP },
        { },
    },

};

static void xppu_register_types(void)
{
    type_register_static(&xppu_info);
}

type_init(xppu_register_types)
