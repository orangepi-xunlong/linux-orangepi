/* SPDX-License-Identifier: GPL-2.0 */
/*
 * regs-fbc-v2p0.h
 *
 * Copyright (C) 2023 KY Micro Limited
 */

#ifndef __REGS_FBC_V2P0_H__
#define __REGS_FBC_V2P0_H__

#define REG_FBC_TNRDEC_HL_ADDR    (0x800)
#define REG_FBC_TNRDEC_HH_ADDR    (0x804)
#define REG_FBC_TNRDEC_BBOX_X     (0x808)
#define REG_FBC_TNRDEC_BBOX_Y     (0x80C)
#define REG_FBC_TNRDEC_IMG_SIZE   (0x810)
#define REG_FBC_TNRDEC_SPLIT_MODE (0x814)
#define REG_FBC_TNRDEC_PERF_CTRL  (0x818)
#define REG_FBC_TNRDEC_CG_EN      (0x81C)
#define REG_FBC_TNRDEC_DMAC_CTRL  (0x820)
#define REG_FBC_TNRDEC_TRIG_CTRL  (0x824)
#define REG_FBC_TNRDEC_IRQ_MASK   (0x828)
#define REG_FBC_TNRDEC_IRQ_RAW    (0x82C)
#define REG_FBC_TNRDEC_IRQ_STATUS (0x830)

#define REG_FBC_TNRENC_HL_ADDR    (0xa00)
#define REG_FBC_TNRENC_HH_ADDR    (0xa04)
#define REG_FBC_TNRENC_PL_ADDR    (0xa08)
#define REG_FBC_TNRENC_PH_ADDR    (0xa0c)
#define REG_FBC_TNRENC_BBOX_X     (0xa10)
#define REG_FBC_TNRENC_BBOX_Y     (0xa14)
#define REG_FBC_TNRENC_Y_ADDR     (0xa18)
#define REG_FBC_TNRENC_PITCH_Y    (0xa1C)
#define REG_FBC_TNRENC_C_ADDR     (0xa20)
#define REG_FBC_TNRENC_PITCH_UV   (0xa24)
#define REG_FBC_TNRENC_Y_BUF_SIZE (0xa28)
#define REG_FBC_TNRENC_C_BUF_SIZE (0xa2C)
#define REG_FBC_TNRENC_SWAP_CTRL  (0xa30)
#define REG_FBC_TNRENC_IRQ_MASK   (0xa34)
#define REG_FBC_TNRENC_IRQ_RAW    (0xa38)
#define REG_FBC_TNRENC_IRQ_STATUS (0xa48)
#define REG_FBC_TNRENC_MODE_CTRL  (0xa40)
#define REG_FBC_TNRENC_DMAC_LENGTH (0xa44)

/* fbc irq status */
#define FIRQ_DEC_DMAC_ERR      (0x1 << 10)
#define FIRQ_DEC_RDMA_TIMEOUT  (0x1 << 9)
#define FIRQ_DEC_SLV_REQ_ERR   (0x1 << 8)
#define FIRQ_DEC_PAYLOAD_ERR   (0x1 << 7)
#define FIRQ_DEC_HDR_ERR       (0x1 << 6)
#define FIRQ_DEC_WLBUF_EOF     (0x1 << 5)
#define FIRQ_DEC_CORE_EOF      (0x1 << 4)
#define FIRQ_DEC_PLD_RDMA_EOF  (0x1 << 3)
#define FIRQ_DEC_HDR_RDMA_EOF  (0x1 << 2)
#define FIRQ_DEC_CFG_SWAPED    (0x1 << 1)
#define FIRQ_DEC_EOF           (0x1 << 0)

#define FIRQ_STAT_CFG_DONE (0x1 << 17)
#define FIRQ_STAT_ENC_EOF  (0x1 << 16)
#define FIRQ_STAT_ENC_ERR  (0xFFFF)

/* fbc irq mask */
#define FIRQ_MASK_DEC_ERR                                                      \
    (FIRQ_DEC_HDR_ERR | FIRQ_DEC_PAYLOAD_ERR | FIRQ_DEC_SLV_REQ_ERR |      \
     FIRQ_DEC_RDMA_TIMEOUT | FIRQ_DEC_DMAC_ERR)
#define FIRQ_MASK_ENC_ERR (FIRQ_STAT_ENC_ERR)

#define FIRQ_MASK_DEC_GEN (FIRQ_MASK_DEC_ERR)
#define FIRQ_MASK_ENC_GEN (FIRQ_MASK_ENC_ERR)

#endif
