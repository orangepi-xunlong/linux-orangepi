/* SPDX-License-Identifier: GPL-2.0 */
/* CAN bus driver for Bosch M_CAN controller
 * Copyright (C) 2018 Texas Instruments Incorporated - http://www.ti.com/
 */

#ifndef _CAN_SDVT_CAN_H_
#define _CAN_SDVT_CAN_H_

#include <linux/can/core.h>
#include <linux/can/led.h>
#include <linux/completion.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/freezer.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/pm_runtime.h>
#include <linux/iopoll.h>
#include <linux/can/dev.h>
#include <linux/pinctrl/consumer.h>

#define SD1_D1_PINMUX 0x28104b40
#define PULL_DOWN_EN 0x8
/* CAN STD mode */
#define CAN_CTRLMODE_STD 0x100

struct sdvt_can_classdev;
struct sdvt_can_command;
struct sdvt_can_config;
struct sdvt_can_ops {
	u32 (*read_reg)(struct sdvt_can_classdev *cdev, int reg);
	int (*write_reg)(struct sdvt_can_classdev *cdev, int reg, int val);
};

struct sdvt_can_plat_priv {
	void __iomem *base;
	void __iomem *mem_base;
};

struct sdvt_can_command {
	u8 command_8b          ; // Command type
	u32 ident_32b     ; // Identifier
	u8 dlc_4b              ; // DLC field value
	u8 data_2d_8b[64]     ; // Data bytes
	u8 tb_tx_2d_8b[64]    ; // Register model
	u8 rx_data_2d_8b[64]  ; // Received data
	u8 rx_len_2d_8b[64]   ; // Received length
	u8 dut_rsp_2d_8b[64]  ; // Response Data bytes
	u8 irq_status0_8b      ; // IRQ status 0
	u8 irq_status1_8b      ; // IRQ status 1
	u8 irq_status2_8b      ; // IRQ status 2
	u8 irq_status3_8b      ; // IRQ status 3
	u8 irq_status4_8b      ; // IRQ status 3
	u8 irq_status5_8b      ; // IRQ status 3
	u8 data_len_code_4b    ; // data length
	u8 remote_resp_en_b    ; // Remote enable mode
	u8 tx_done;              // tx done
};

struct sdvt_can_config {
	u8 cfg_can_mode_2b              ; // Can mode
	u8 cfg_ext_frame_mode_b         ; // extended_mode variable
	u8 cfg_remote_resp_en_b         ; // Remove enable mode
	u8 cfg_ext_ctrl_listen_b        ; // DUT Extended Control - Listen only mode
	u8 cfg_ext_ctrl_self_test_b     ; // DUT Extended Control - Self test mode
	u8 cfg_ext_ctrl_acc_filter_b    ; // Extended Acceptance filter mode. 0 means dual filter and 1 single filter
	u8 cfg_ext_ctrl_rec_incr_b      ; // DUT Extended Control - Increment receiver error counter
	u8 cfg_ext_ctrl_remote_en_b     ; // DUT Extended Control - Enable receiving and processing remote frames
	u8 cfg_ext_ctrl_id_rx_b         ; // Extended Identifier  - Receiver side
	u8 cfg_ext_mode_en_b            ; // DUT Extended Control - Enable receiving and processing extebded mode
	u8 cfg_acceptance_code0_8b      ; // Acceptance code0
	u8 cfg_acceptance_code1_8b      ; // Acceptance code1
	u8 cfg_acceptance_code2_8b      ; // Acceptance code2
	u8 cfg_acceptance_code3_8b      ; // Acceptance code3
	u8 cfg_acceptance_mask0_8b      ; // Acceptance mask0
	u8 cfg_acceptance_mask1_8b      ; // Acceptance mask1
	u8 cfg_acceptance_mask2_8b      ; // Acceptance mask2
	u8 cfg_acceptance_mask3_8b      ; // Acceptance mask3
	u8 cfg_nor_clk_divider_8b       ; // The controls the normal mode clock divider
	u8 cfg_nor_baud_prescaler_6b    ; // The controls the normal mode baud prescaler
	u8 cfg_nor_sync_jump_width_2b   ; // The controls the normal mode Sync jump width
	u8 cfg_nor_time_segment1_3b     ; // The controls the normal mode timing segment 1
	u8 cfg_nor_time_segment2_4b     ; // The controls the normal mode timing segment 2
	u8 cfg_nor_time_triple_sample_b ; // The controls the normal mode timing triple sample
	u8 cfg_fd_clk_divider_8b        ; // The controls the fd mode clock divider
	u8 cfg_fd_baud_prescaler_6b     ; // The controls the fd mode baud prescaler
	u8 cfg_fd_sync_jump_width_2b    ; // The controls the fd mode Sync jump width
	u8 cfg_fd_time_segment1_4b      ; // The controls the fd mode timing segment 1
	u8 cfg_fd_time_segment2_3b      ; // The controls the fd mode timing segment 2
	u8 cfg_fd_time_triple_sample_b  ; // The controls the fd mode timing triple sample
	u8 irq_enable0_8b               ; // IRQ enable 0
	u8 irq_enable1_8b               ; // IRQ enable 1
	u8 irq_enable2_8b               ; // IRQ enable 2
	u8 irq_enable3_8b               ; // IRQ enable 3
	u8 irq_enable4_8b               ; // IRQ enable 4
	u8 irq_enable5_8b               ; // IRQ enable 5
	u8 tx_b                         ; // Tx operation
	u8 cfg_fd_brs_b                 ; // CAN FD - Enable BRS
	u8 mode_reset_8b                ; // mode reset
	u8 irq_enable_8b                ; // Irq enable
	u8 can_mode_8b                  ; // can mode register
	u8 std_arb_id0_8b               ; // 0]
	u8 std_arb_id1_8b               ; // 3] and normal frame and 28:21 for extended frame
	u8 std_arb_id2_8b               ; // Arbitration identifier bits 20:13 for extended frame
	u8 std_arb_id3_8b               ; // Arbitration identifier bits 12:5 for extended frame
	u8 std_arb_id4_8b               ; // Arbitration identifier bits 12:5 for extended frame
	u8 rsp_arb_id0_8b               ; // Remote frame response Identifier bits 10:3 for nor and 28:21 for ex
	u8 rsp_arb_id1_8b               ; // Remote frame response Identifier bits 2:0 for normal and 20:13 for extended
	u8 rsp_arb_id2_8b               ; // This register holds remote frame response Identifier bits 12:5 for extended
	u8 rsp_arb_id3_8b               ; // This register holds remote frame response Identifier bits 4:0 for extended
	u8 acceptance_code_8b           ; // Acceptance code
	u8 acceptance_mask_8b           ; // Acceptance mask
	u8 bus_timing0_8b               ; // Bus timing register 0
	u8 bus_timing1_8b               ; // Bus timing register 1
};
struct sdvt_can_classdev {
	struct can_priv can;
	struct napi_struct napi;
	struct net_device *net;
	struct device *dev;
	struct clk *cclk;

	struct workqueue_struct *tx_wq;
	struct work_struct tx_work;
	struct sk_buff *tx_skb;

	struct can_bittiming_const *bit_timing;
	struct can_bittiming_const *data_timing;

	struct sdvt_can_ops *ops;

	struct sdvt_can_command cmd_o;
	struct sdvt_can_config cfg_o;

	struct sdvt_can_plat_priv *device_data;

	int version;
	int freq;
	u32 irqstatus;

	int pm_clock_support;
	int is_peripheral;
};

struct sdvt_can_classdev *sdvt_can_class_allocate_dev(struct device *dev);
void sdvt_can_class_free_dev(struct net_device *net);
int sdvt_can_class_register(struct sdvt_can_classdev *cdev);
void sdvt_can_class_unregister(struct sdvt_can_classdev *cdev);
int sdvt_can_class_get_clocks(struct sdvt_can_classdev *cdev);
void sdvt_can_init_ram(struct sdvt_can_classdev *priv);
void sdvt_can_config_endisable(struct sdvt_can_classdev *priv, bool enable);

int sdvt_can_class_suspend(struct device *dev);
int sdvt_can_class_resume(struct device *dev);
#endif	/* _CAN_M_H_ */
