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

#ifndef _DW_CEC_H
#define _DW_CEC_H

/**
 * @desc: dw cec set logical address
 * @addr: logical address
 */
void dw_cec_set_logical_addr(u16 addr);

/**
 * @desc: dw cec receive message
 * @buf: cec message buffer for receive
 * @buf_size: cec message buffer size for receive
 * @return: actual receive message size. unit: byte
 */
int dw_cec_receive_msg(u8 *buf, u8 buf_size);

/**
 * @desc: dw cec send message
 * @buf: cec message buffer for send
 * @len: cec message size
 * @type: cec send wait bit times type.
 */
void dw_cec_send_msg(u8 *buf, u8 len, u8 type);

/**
 * @desc: dw cec set enable or disable
 * @enable: 1 - enable dw cec
 *          0 - disable dw cec
 */
int dw_cec_set_enable(u8 enable);

enum dw_cec_wait_bit_times_e {
	DW_CEC_WAIT_3BIT = 0,
	DW_CEC_WAIT_5BIT = 1,
	DW_CEC_WAIT_7BIT = 2,
	DW_CEC_WAIT_NULL = 3,
};

/*****************************************************************************
 *                                                                           *
 *                               CEC Registers                               *
 *                                                                           *
 *****************************************************************************/
/**
 * @desc: CEC Control Register
 * This register handles the main control of the CEC initiator
 * @bits: [0:0] CEC send
 *          - 1: Set by software to trigger CEC sending a frame as an initiator.
 *          - 0: Reset to 1'b0 by hardware when the CEC transmission is done.
 *        [2:1] Signal Free Times
 *          - 00: 3-bit periods. Previous attempt to send frame is unsuccessful
 *          - 01: 5-bit periods. New initiator wants to send a frame
 *          - 10: 7-bit periods. Present initiator wants to send another frame immediately after its previous frame
 *          - 11: lllegal value. If software writes this value, hardware sets the value to the defult 2'b01
 *        [3:3]
 *          - 1: Set by software to NACK the received broadcast message
 *          - 0: Reset by software to ACK the received broadcast message
 *        [4:4]
 *          - 1: CEC controller responds with a NACK to all messages and generates a wakeup status for opcode
 *          - 0: CEC controller responds the ACK to all message
 * */
#define CEC_CTRL        0x00007D00
#define CEC_CTRL_SEND_MASK          0x00000001
#define CEC_CTRL_FRAME_TYP_MASK     0x00000006
#define CEC_CTRL_BC_NACK_MASK       0x00000008
#define CEC_CTRL_STANDBY_MASK       0x00000010

/**
 * @desc: CEC Interrupt Mask Register
 * This read/write register masks/unmasks the interrupt events
 * @bits: [0:0] The current transmission is successful (for initiator only)
 *        [1:1] EOM is detected so that the received data is ready in the receiver data buffer (for follower only)
 *        [2:2] A frame is not acknowledged in a directly addressed message
 *        [3:3] The initiator losses the CEC line arbitration to a second initiator
 *        [4:4] An error is detected on a CEC line (for initiator only)
 *        [5:5] An error is notified by a follower
 *        [6:6] Follower wake-up signal mask
 * */
#define CEC_MASK        0x00007D02
#define CEC_MASK_DONE_MASK              0x00000001
#define CEC_MASK_EOM_MASK               0x00000002
#define CEC_MASK_NACK_MASK              0x00000004
#define CEC_MASK_ARB_LOST_MASK          0x00000008
#define CEC_MASK_ERROR_INITIATOR_MASK   0x00000010
#define CEC_MASK_ERROR_FLOW_MASK        0x00000020
#define CEC_MASK_WAKEUP_MASK            0x00000040

/**
 * @desc: CEC Logical Address Register Low
 * This register indicates the logical address(es) allocated to the CEC device
 * @bits: [0:0] Logical address 0 - Device TV
 *        [1:1] Logical address 1 - Recording Device 1
 *        [2:2] Logical address 2 - Recording Device 2
 *        [3:3] Logical address 3 - Tuner 1
 *        [4:4] Logical address 4 - Playback Device 1
 *        [5:5] Logical address 5 - Audio System
 *        [6:6] Logical address 6 - Tuner 2
 *        [7:7] Logical address 7 - Tuner 3
 */
#define CEC_ADDR_L      0x00007D05
#define CEC_ADDR_L_CEC_ADDR_L_0_MASK  0x00000001
#define CEC_ADDR_L_CEC_ADDR_L_1_MASK  0x00000002
#define CEC_ADDR_L_CEC_ADDR_L_2_MASK  0x00000004
#define CEC_ADDR_L_CEC_ADDR_L_3_MASK  0x00000008
#define CEC_ADDR_L_CEC_ADDR_L_4_MASK  0x00000010
#define CEC_ADDR_L_CEC_ADDR_L_5_MASK  0x00000020
#define CEC_ADDR_L_CEC_ADDR_L_6_MASK  0x00000040
#define CEC_ADDR_L_CEC_ADDR_L_7_MASK  0x00000080

/**
 * @desc: CEC Logical Address Register High
 * This register indicates the logical address(es) allocated to the CEC device
 * @bits: [0:0] Logical address 8 - Playback Device 2
 *        [1:1] Logical address 9 - Playback Device 3
 *        [2:2] Logical address 10 - Tuner 4
 *        [3:3] Logical address 11 - Playback Device 3
 *        [4:4] Logical address 12 - Reserved
 *        [5:5] Logical address 13 - Reserved
 *        [6:6] Logical address 14 - Free use
 *        [7:7] Logical address 15 - Unregistered (as initiator address), Broadcast (as destination address)
 */
#define CEC_ADDR_H      0x00007D06
#define CEC_ADDR_H_CEC_ADDR_H_0_MASK  0x00000001
#define CEC_ADDR_H_CEC_ADDR_H_1_MASK  0x00000002
#define CEC_ADDR_H_CEC_ADDR_H_2_MASK  0x00000004
#define CEC_ADDR_H_CEC_ADDR_H_3_MASK  0x00000008
#define CEC_ADDR_H_CEC_ADDR_H_4_MASK  0x00000010
#define CEC_ADDR_H_CEC_ADDR_H_5_MASK  0x00000020
#define CEC_ADDR_H_CEC_ADDR_H_6_MASK  0x00000040
#define CEC_ADDR_H_CEC_ADDR_H_7_MASK  0x00000080

/**
 * @desc: CEC TX Frame Size Register
 * This register indicates the size of the frame in bytes (including header and data blocks),
 * which are available in the transmitter data buffer
 * @bits: [4:0] CEC Transmitter Counter register
 *          - 0: No data needs to be transmitted
 *          - 1: Frame size is 1 byte
 *          - ...
 *          - 16: Frame size is 16 byte
 */
#define CEC_TX_CNT      0x00007D07
#define CEC_TX_CNT_CEC_TX_CNT_MASK  0x0000001F

/**
 * @desc: CEC RX Frame Size Register
 * This register indicates the size of the frame in bytes (including header and data blocks),
 * which are available in the receiver data buffer
 * @bits: [4:0] CEC Receiver Counter register
 *          - 0: No data received
 *          - 1: 1-byte data is received
 *          - ...
 *          - 16: 16-byte data is received
 * */
#define CEC_RX_CNT      0x00007D08
#define CEC_RX_CNT_CEC_RX_CNT_MASK  0x0000001F

/**
 * @desc: CEC TX Data Register Array
 * Address offset: i = 0 to 15
 * These registers (8 bits each) are the buffers used for storing the data waiting
 * for transmission (including header and data blocks)
 * @bits: [7:0] Data Byte[i]
 */
#define CEC_TX_DATA     0x00007D10

/**
 * @desc: CEC RX Data Register Array
 * Address offset: i =0 to 15
 * These registers (8 bit each) are the buffers used for storing the received data
 * @bits: [7:0] Data Byte[i]
 */
#define CEC_RX_DATA     0x00007D20

/**
 * @desc: CEC Buffer Lock Register
 * @bits: [0:0] When a frame is received, this bit would be active
 */
#define CEC_LOCK        0x00007D30
#define CEC_LOCK_LOCKED_BUFFER_MASK  0x00000001

/**
 * @desc: CEC Wake-up Control Register
 * @bits: [0:0] OPCODE 0x04 wake up enable
 *        [1:1] OPCODE 0x0D wake up enable
 *        [2:2] OPCODE 0x41 wake up enable
 *        [3:3] OPCODE 0x42 wake up enable
 *        [4:4] OPCODE 0x44 wake up enable
 *        [5:5] OPCODE 0x70 wake up enable
 *        [6:6] OPCODE 0x82 wake up enable
 *        [7:7] OPCODE 0x86 wake up enable
 */
#define CEC_WAKEUPCTRL  0x00007D31
#define CEC_WAKEUPCTRL_OPCODE0X04EN_MASK  0x00000001
#define CEC_WAKEUPCTRL_OPCODE0X0DEN_MASK  0x00000002
#define CEC_WAKEUPCTRL_OPCODE0X41EN_MASK  0x00000004
#define CEC_WAKEUPCTRL_OPCODE0X42EN_MASK  0x00000008
#define CEC_WAKEUPCTRL_OPCODE0X44EN_MASK  0x00000010
#define CEC_WAKEUPCTRL_OPCODE0X70EN_MASK  0x00000020
#define CEC_WAKEUPCTRL_OPCODE0X82EN_MASK  0x00000040
#define CEC_WAKEUPCTRL_OPCODE0X86EN_MASK  0x00000080

#endif /* _DW_CEC_H */
