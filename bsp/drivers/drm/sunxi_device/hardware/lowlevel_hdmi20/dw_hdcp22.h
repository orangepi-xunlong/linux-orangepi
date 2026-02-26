/* SPDX-License-Identifier: GPL-2.0-or-later */
/*******************************************************************************
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 ******************************************************************************/
#ifndef _DW_HDCP22_H
#define _DW_HDCP22_H

#include <linux/workqueue.h>

/*****************************************************************************
 *                                                                           *
 *                               DW HDCP2.x Function                         *
 *                                                                           *
 *****************************************************************************/
/**
 * @desc: dw hdcp config data pth
 * @path: 0 - hdcp14 path
 *        1 - hdcp22 path
 */
void dw_hdcp2x_ovr_set_path(u8 ovr_val, u8 ovr_en);
/**
 * @desc: dw hdcp get data path
 * @return: 0 - hdcp1x path
 *          1 - hdcp2x path
 */
u8 dw_hdcp2x_get_path(void);
/**
 * @desc: dw hdcp2x get encry state
 * @return: refer to enum dw_hdcp_state_e
 */
int dw_hdcp2x_get_encrypt_state(void);
/**
 * @desc: dw hdcp2x enable
 * @return: 0 - success
 *         -1 - failed
 */
int dw_hdcp2x_enable(void);
/**
 * @desc: dw hdcp2x disable
 * @return: 0 - success
 *         -1 - failed
 */
int dw_hdcp2x_disable(void);
/**
 * @desc: dw hdcp2x firmware loading state;
 * @return: 1 - loaded
 *          0 - not loaded
 */
int dw_hdcp2x_firmware_state(void);
/**
 * @desc: dw hdcp2x firmware update dma address
 * @data: save firmware buffer
 * @size: firmware size. unit: byte
 * @return: 0 - success
 *         -1 - failed
 */
int dw_hdcp2x_firmware_update(const u8 *data, size_t size);
/**
 * @desc: dw hdcp2x init
 * @return: 0 - success
 *         -1 - failed
 */
int dw_hdcp2x_init(void);
/**
 * @desc: dw hdcp2x exit
 * @return: 0 - success
 *         -1 - failed
 */
void dw_hdcp2x_exit(void);
/**
 * @desc: dw hdcp2x info dump
 * @buf: dump info buffer
 * @return: dump info byte count
 */
ssize_t dw_hdcp2x_dump(char *buf);

/*****************************************************************************
 *                                                                           *
 *                               HDCP22 Registers                            *
 *                                                                           *
 *****************************************************************************/

/**
 * @desc: HDCP2.2 Identification Register
 * @bits: [1:1] Indicates that External HDCP2.2 interface is present
 *        [2:2] Indicates that External HDCP2.2 interface is present and 3rd party
 *              HDCP2.2 module is connected to this interface
 */
#define HDCP22REG_ID            0x00007900
#define HDCP22REG_EXTERNALIF_MASK               0x00000002
#define HDCP22REG_3RDPARTY_MASK                 0x00000004

/**
 * @desc: HDCP2.2 Control Register
 * @bits: [0:0] HDCP22 switch lock
 *        [1:1] HDCP22 versus 1.4 switch override enable
 *        [2:2] HDCP22 versus 1.4 switch override value
 *        [4:4] HPD override enable
 *        [5:5] HPD override value
 */
#define HDCP22REG_CTRL          0x00007904
#define HDCP22REG_CTRL_HDCP22_SWITCH_LCK_MASK   0x00000001
#define HDCP22REG_CTRL_HDCP22_OVR_EN_MASK       0x00000002
#define HDCP22REG_CTRL_HDCP22_OVR_VAL_MASK      0x00000004
#define HDCP22REG_CTRL_HPD_OVR_EN_MASK          0x00000010
#define HDCP22REG_CTRL_HPD_OVR_VAL_MASK         0x00000020

/**
 * @desc: HDCP2.2 Control Register
 * @bits: [0:0] HDCP2.2 versus 1.4 avmute override enable
 *        [1:1] HDCP AVMUTE overrit value
 *        [3:3] HDCP2.2 versus 1.4 color depth override enable
 *        [7:4] HDCP color depth override value
 */
#define HDCP22REG_CTRL1         0x00007905
#define HDCP22REG_CTRL1_HDCP22_AVMUTE_OVR_EN_MASK   0x00000001
#define HDCP22REG_CTRL1_HDCP22_AVMUTE_OVR_VAL_MASK  0x00000002
#define HDCP22REG_CTRL1_HDCP22_CD_OVR_EN_MASK       0x00000008
#define HDCP22REG_CTRL1_HDCP22_CD_OVR_VAL_MASK      0x000000F0

/**
 * @desc: HDCP2.2 Status Register
 * @bits: [0:0] HDCP2.2 HPD external interface status after lock
 *          - 0: sink not detected(HPD line clear)
 *          - 1: sink detected(HPD line set)
 *        [1:1] HDCP2.2 AVMUTE external interface status
 *          - 0: external HDCP used AVMUTE is clear
 *          - 1: external HDCP ACMUTE is set
 *        [2:2] HDCP2.2 versus 1.4 switch status after lock
 *          - 0: HDCP1.4 selected
 *          - 1: HDCP2.2 selected
 *        [3:3] Value of HDCP2.2 ist_hdcp_decrypted line. Provided for debug only
 */
#define HDCP22REG_STS           0x00007908
#define HDCP22REG_STS_HDMI_HPD                      0x00000001
#define HDCP22REG_STS_HDCP_AVMUTE                   0x00000002
#define HDCP22REG_STS_HDCP22_SWITCH                 0x00000004
#define HDCP22REG_STS_HDCP_DECRYPTED                0x00000008

/**
 * @desc: HDCP2.2 Status Mask Register
 * @bits: [0:0] Active high interrupt mask to HDCP2.2 capable rise interrupt status
 *        [1:1] Active high interrupt mask to HDCP2.2 not capable rise interrupt status
 *        [2:2] Active high interrupt mask to HDCP2.2 authentication lose interrupt status
 *        [3:3] Active high interrupt mask to HDCP2.2 authentication interrupt status
 *        [4:4] Active high interrupt mask to HDCP2.2 authentication fail interrupt status
 *        [5:5] Active high interrupt mask to HDCP2.2 decrypted value change interrupt status
 */
#define HDCP22REG_MASK          0x0000790C
#define HDCP22REG_MASK_CAPABLE_MASK             0x00000001
#define HDCP22REG_MASK_NOT_CAPABLE_MASK         0x00000002
#define HDCP22REG_MASK_AUTHEN_LOST_MASK         0x00000004
#define HDCP22REG_MASK_AUTHEN_MASK              0x00000008
#define HDCP22REG_MASK_AUTHEN_FAIL_MASK         0x00000010
#define HDCP22REG_MASK_DECRYP_CHG_MASK          0x00000020

/**
 * @desc: HDCP2.2 Status Register
 * @bits: [0:0] HDCP2.2 capable rise interrupt status sticky bit
 *        [1:1] HDCP2.2 not capable rise interrupt status sticky bit
 *        [2:2] HDCP2.2 authentication lost interrupt status sticky bit
 *        [3:3] HDCP2.2 authenticatio interrupt status sticky bit
 *        [4:4] HDCP2.2 authenticatio ifail nterrupt status sticky bit
 *        [5:5] HDCP2.2 decrypted value change interrupt status sticky bit
 */
#define HDCP22REG_STAT          0x0000790D
#define HDCP22REG_STAT_CAPABLE_MASK             0x00000001
#define HDCP22REG_STAT_NOT_CAPABLE_MASK         0x00000002
#define HDCP22REG_STAT_AUTHEN_LOST_MASK         0x00000004
#define HDCP22REG_STAT_AUTHEN_MASK              0x00000008
#define HDCP22REG_STAT_AUTHEN_FAIL_MASK         0x00000010
#define HDCP22REG_STAT_DECRYP_CHG_MASK          0x00000020

/**
 * @desc: HDCP2.2 Interrupt Mute Vector
 * @bits: [0:0] Active high interrupt mute to HDCP2.2 capable rise interrupt status
 *        [1:1] Active high interrupt mute to HDCP2.2 not capable rise interrupt status
 *        [2:2] Active high interrupt mute to HDCP2.2 authentication lose interrupt status
 *        [3:3] Active high interrupt mute to HDCP2.2 authentication interrupt status
 *        [4:4] Active high interrupt mute to HDCP2.2 authentication fail interrupt status
 *        [5:5] Active high interrupt mute to HDCP2.2 decrypted value change interrupt status
 */
#define HDCP22REG_MUTE          0x0000790E
#define HDCP22REG_MUTE_CAPABLE_MASK             0x00000001
#define HDCP22REG_MUTE_NOT_CAPABLE_MASK         0x00000002
#define HDCP22REG_MUTE_AUTHEN_LOST_MASK         0x00000004
#define HDCP22REG_MUTE_AUTHEN_MASK              0x00000008
#define HDCP22REG_MUTE_AUTHEN_FAIL_MASK         0x00000010
#define HDCP22REG_MUTE_DECRYP_CHG_MASK          0x00000020

#endif /* _DW_HDCP22_H */
