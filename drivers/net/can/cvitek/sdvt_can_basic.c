// SPDX-License-Identifier: GPL-2.0-only
#ifndef SDVT_CAN_BASIC_C
#define SDVT_CAN_BASIC_C

//                           SmartDV Technologies Proprietary
//            Copyright 2007-2023 SmartDV Technologies India Private Limited
//
//                               CONFIDENTIAL INFORMATION
//
//                                  All rights reserved
//
//             The use, modification, or duplication of this product is protected
//             according to SmartDV Technologies's licensing agreement.
//
//             This Intellectual property contains confidential and proprietary
//             information which are the properties of SmartDV Technologies.
//
//             Unauthorized use, disclosure, duplication, or reproduction are prohibited.

// Date Created   : 02-06-2023
// Description    : This is simple CAN IP driver
// Generator      : IIP Compiler Version 1.1

#define STANDARD_DATA_FRAME 0         // Standard data frame
#define EXTENDED_DATA_FRAME 1         // Extended data frame
#define STANDARD_REMOTE_FRAME 2         // Standard remote frame
#define EXTENDED_REMOTE_FRAME 3         // Extended remote frame

#include "sdvt_can_defines.h"
#include "sdvt_can_basic.h"
#include "cvitek_can.h"
#include <linux/types.h>

// init_chip :This method is used to initialize the chip
//           p_config_st :Pointer to config object

void sdvt_init_chip(struct sdvt_can_classdev *cdev, struct sdvt_can_config *p_config_st)
{
	u8 m_data_8b ; // Variable to store data
	// sc_wait_clocks(100);
	// Set normal mode clock divider
	cdev->ops->write_reg(cdev, SDVT_CAN_NOR_CLK_DIVIDER, p_config_st->cfg_nor_clk_divider_8b);

	// Reset the core
	m_data_8b = 1;
	cdev->ops->write_reg(cdev, SDVT_CAN_CONTROL, m_data_8b);

	// Set  normal mode  bus timing register 1
	m_data_8b = 0;
	m_data_8b = m_data_8b | (p_config_st->cfg_nor_baud_prescaler_6b << 0);
	m_data_8b = m_data_8b | (p_config_st->cfg_nor_sync_jump_width_2b << 6);
	cdev->ops->write_reg(cdev, SDVT_CAN_NOR_BUS_TIMING0, m_data_8b);

	// Set normal mode bus timing register 1
	m_data_8b = 0;
	m_data_8b = m_data_8b | (p_config_st->cfg_nor_time_segment1_3b << 0);
	m_data_8b = m_data_8b | (p_config_st->cfg_nor_time_segment2_4b << 4);
	m_data_8b = m_data_8b | (p_config_st->cfg_nor_time_triple_sample_b << 7);
	cdev->ops->write_reg(cdev, SDVT_CAN_NOR_BUS_TIMING1, m_data_8b);

	// Set FD mode clock divider
	cdev->ops->write_reg(cdev, SDVT_CAN_FD_CLK_DIVIDER, p_config_st->cfg_fd_clk_divider_8b);

	// Set FD control register
	m_data_8b = 0;
	m_data_8b = p_config_st->cfg_fd_brs_b;
	cdev->ops->write_reg(cdev, SDVT_CAN_FD_CONTROL, m_data_8b);

	// Set FD mode  bus timing register 1
	m_data_8b = 0;
	m_data_8b = m_data_8b | (p_config_st->cfg_fd_baud_prescaler_6b << 0);
	m_data_8b = m_data_8b | (p_config_st->cfg_fd_sync_jump_width_2b << 6);
	cdev->ops->write_reg(cdev, SDVT_CAN_FD_BUS_TIMING0, m_data_8b);

	// Set FD mode bus timing register 1
	m_data_8b = 0;
	m_data_8b = m_data_8b | (p_config_st->cfg_fd_time_segment1_4b << 0);
	m_data_8b = m_data_8b | (p_config_st->cfg_fd_time_segment2_3b << 4);
	m_data_8b = m_data_8b | (p_config_st->cfg_fd_time_triple_sample_b << 7);
	cdev->ops->write_reg(cdev, SDVT_CAN_FD_BUS_TIMING1, m_data_8b);

	// Set Acceptance Code and Acceptance Mask registers
	cdev->ops->write_reg(cdev, SDVT_CAN_ACCEPTANCE_CODE0, p_config_st->cfg_acceptance_code0_8b);
	cdev->ops->write_reg(cdev, SDVT_CAN_ACCEPTANCE_CODE1, p_config_st->cfg_acceptance_code1_8b);
	cdev->ops->write_reg(cdev, SDVT_CAN_ACCEPTANCE_CODE2, p_config_st->cfg_acceptance_code2_8b);
	cdev->ops->write_reg(cdev, SDVT_CAN_ACCEPTANCE_CODE3, p_config_st->cfg_acceptance_code3_8b);
	cdev->ops->write_reg(cdev, SDVT_CAN_ACCEPTANCE_MASK0, p_config_st->cfg_acceptance_mask0_8b);
	cdev->ops->write_reg(cdev, SDVT_CAN_ACCEPTANCE_MASK1, p_config_st->cfg_acceptance_mask1_8b);
	cdev->ops->write_reg(cdev, SDVT_CAN_ACCEPTANCE_MASK2, p_config_st->cfg_acceptance_mask2_8b);
	cdev->ops->write_reg(cdev, SDVT_CAN_ACCEPTANCE_MASK3, p_config_st->cfg_acceptance_mask3_8b);

	// set the core to Operating mode
	m_data_8b = 0;
	cdev->ops->write_reg(cdev, SDVT_CAN_CONTROL, m_data_8b);

	if (p_config_st->cfg_ext_frame_mode_b == SDVT_CAN_STANDARD_FRAME)
		m_data_8b = m_data_8b | (SDVT_CAN_STANDARD_FRAME << 2);  // Standard frame format
	else
		m_data_8b = m_data_8b | (SDVT_CAN_EXTENDED_FRAME << 2); // Extended frame format
	 // Operate in Normal CAN specs mode
	if (p_config_st->cfg_can_mode_2b == SDVT_CAN_CLASSIC_MODE)
		m_data_8b = m_data_8b | (SDVT_CAN_CLASSIC_MODE << 3);
	else
		// Operate in CAN FD mode
		m_data_8b = m_data_8b | (SDVT_CAN_FD_MODE << 3);

	// Write control register
	cdev->ops->write_reg(cdev, SDVT_CAN_CONTROL, m_data_8b);
	m_data_8b = 0;
	m_data_8b = m_data_8b |
		 (p_config_st->cfg_ext_ctrl_listen_b  << SDVT_CAN_EXT_CTRL_LISTEN);
	m_data_8b = m_data_8b |
		 (p_config_st->cfg_ext_ctrl_self_test_b  << SDVT_CAN_EXT_CTRL_SELF_TEST);
	m_data_8b = m_data_8b |
		 (p_config_st->cfg_ext_ctrl_acc_filter_b << SDVT_CAN_EXT_CTRL_ACC_FILTER);
	m_data_8b = m_data_8b |
		 (p_config_st->cfg_ext_ctrl_rec_incr_b << SDVT_CAN_EXT_CTRL_REC_INCR);
	m_data_8b = m_data_8b |
		 (p_config_st->cfg_ext_ctrl_remote_en_b << SDVT_CAN_EXT_CTRL_REMOTE_EN);
	m_data_8b = m_data_8b |
		 (p_config_st->cfg_ext_ctrl_id_rx_b << SDVT_CAN_EXT_ID_RX);

	// Write control register
	cdev->ops->write_reg(cdev, SDVT_CAN_EXT_CONTROL, m_data_8b);

	// Disable IRQ
	cdev->ops->write_reg(cdev, SDVT_CAN_IRQ_ENABLE0, 0x0);
	cdev->ops->write_reg(cdev, SDVT_CAN_IRQ_ENABLE1, 0x0);
	cdev->ops->write_reg(cdev, SDVT_CAN_IRQ_ENABLE1, 0x0);
	cdev->ops->write_reg(cdev, SDVT_CAN_IRQ_ENABLE1, 0x0);

	// clear intr register
	cdev->ops->write_reg(cdev, SDVT_CAN_IRQ_STATUS0, 0xFF);
	cdev->ops->write_reg(cdev, SDVT_CAN_IRQ_STATUS1, 0xFF);
	dev_info(cdev->dev, "INFO : init_chip() :: CAN DUT CONFIGURATION COMPLETED\n");

	// flush the RX data fifo
	m_data_8b = 0x1 << SDVT_CAN_RX_DATA_FIFO_FLUSH;
	cdev->ops->write_reg(cdev, SDVT_CAN_FIFO_FLUSH, m_data_8b);
	mdelay(1);
	m_data_8b = 0;
	cdev->ops->write_reg(cdev, SDVT_CAN_FIFO_FLUSH, m_data_8b);
}

// send_command :This method is used for sending command
// p_cmd_st :Pointer to command object
// p_config_st :Pointer to config object

void send_command(struct sdvt_can_classdev *cdev,
		  struct sdvt_can_command *p_cmd_st,
		  struct sdvt_can_config *p_config_st)
{
	u8   m_data_8b   ; // Variable to store data
	u8   m_data1_8b  ; // Variable to store data
	u8   m_bytes_8b  ; // Byte count
	int  m_idx_i     ; // Iterative variable
	u8   m_ext_b     ; // Extended frame
	u8   m_rtr_b     ; // Remote frame

	if (p_cmd_st->command_8b == STANDARD_DATA_FRAME) {
		m_ext_b = 0; // This is extended data frame
		m_rtr_b = 0;
	} else if (p_cmd_st->command_8b == EXTENDED_DATA_FRAME) {
		m_ext_b = 1; // This is standard remote frame
		m_rtr_b = 0;
	} else if (p_cmd_st->command_8b == STANDARD_REMOTE_FRAME) {
		m_ext_b = 0; // This is extended remote frame
		m_rtr_b = 1;
	} else if (p_cmd_st->command_8b == EXTENDED_REMOTE_FRAME) {
		m_ext_b = 1;
		m_rtr_b = 1;
	}

  // Program standard frame ID registers
	if (m_ext_b == 0) {
		if (m_rtr_b == 0)
			m_data_8b = p_cmd_st->dlc_4b; // Identifier[2:0], rtr, length
		else
			m_data_8b = 0;

		m_data_8b = m_data_8b | (m_rtr_b << 4);
		m_data1_8b = p_cmd_st->ident_32b & 7;
		m_data_8b = m_data_8b | (m_data1_8b << 5);
		cdev->ops->write_reg(cdev, SDVT_CAN_ARB_ID0, m_data_8b);

		m_data_8b = p_cmd_st->ident_32b >> 3; // Program the identifier[10:3]
		// Program extended frame ID registers
		cdev->ops->write_reg(cdev, SDVT_CAN_ARB_ID1, m_data_8b);
	} else {
		if (m_rtr_b == 0) {
			m_data_8b = p_cmd_st->dlc_4b;
			// Frame Format = 3'b100, RTR, DLC
			cdev->ops->write_reg(cdev, SDVT_CAN_ARB_ID0, m_data_8b);
		} else {
			m_data_8b = 0;
			cdev->ops->write_reg(cdev, SDVT_CAN_ARB_ID0, m_data_8b);
		}
		m_data_8b = m_data_8b | (m_rtr_b << 6);
		m_data_8b = m_data_8b | (1 << 7);
		cdev->ops->write_reg(cdev, SDVT_CAN_ARB_ID0, m_data_8b);

		// Program the identifier[28:21]
		m_data_8b = p_cmd_st->ident_32b >> 21 & 0xFF;
		cdev->ops->write_reg(cdev, SDVT_CAN_ARB_ID1, m_data_8b);

		// Program the identifier[20:13]
		m_data_8b = p_cmd_st->ident_32b >> 13 & 0xFF;
		cdev->ops->write_reg(cdev, SDVT_CAN_ARB_ID2, m_data_8b);

		// Program the identifier[12:5]
		m_data_8b = p_cmd_st->ident_32b >> 5 & 0xFF;
		cdev->ops->write_reg(cdev, SDVT_CAN_ARB_ID3, m_data_8b);

		// Program the identifier[4:0]
		m_data_8b = p_cmd_st->ident_32b & 0x1F;
		cdev->ops->write_reg(cdev, SDVT_CAN_ARB_ID4, m_data_8b);
		// dev_info(cdev->dev, "INFO : %s() :: Extended frame\n", __func__);
	}

	m_bytes_8b = p_cmd_st->data_len_code_4b;

    // Generate data only in non remote frames mode
	if (m_rtr_b == 0) {
	// Randomize the data, and store in STD_TX_DATA fifo
		for (m_idx_i = 0; m_idx_i < m_bytes_8b; m_idx_i += 1) {
			m_data_8b = p_cmd_st->data_2d_8b[m_idx_i];
			cdev->ops->write_reg(cdev, SDVT_CAN_TX_DATA_FIFO, m_data_8b);
		}
	}

	// Write the COMMAND_REGISTER to enable tranmission
	  m_data_8b    = 1;
	  m_data_8b    = m_data_8b | (m_ext_b  << 1);
	  cdev->ops->write_reg(cdev, SDVT_CAN_COMMAND, m_data_8b);

	  // Enable bit 0 (Info empty register)
	  m_data_8b = 0;
	  cdev->ops->write_reg(cdev, SDVT_CAN_IRQ_ENABLE0, m_data_8b);

	  // Enable bit 0 (Transmit Done)
	  m_data_8b = cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_ENABLE1) | 0x1;
	  cdev->ops->write_reg(cdev, SDVT_CAN_IRQ_ENABLE1, m_data_8b);

	// #Core drives response frame
	if (p_config_st->cfg_remote_resp_en_b == 1) {
		// #Randomize the data, and store in STD_TX_DATA fifo
		for (m_idx_i = 0; m_idx_i < p_cmd_st->dlc_4b; m_idx_i += 1)
			cdev->ops->write_reg(cdev,
					      SDVT_CAN_TX_RSP_FIFO,
					      p_cmd_st->data_2d_8b[m_idx_i]);
		m_data_8b  = m_data_8b | (p_cmd_st->dlc_4b << 0);
		m_data_8b  = m_data_8b | (p_config_st->cfg_ext_frame_mode_b << 4);
		m_data_8b  = m_data_8b | (p_cmd_st->remote_resp_en_b << 5);
		cdev->ops->write_reg(cdev, SDVT_CAN_DLC_REMOTE_FRAME, m_data_8b);
	}
}

// receive_frame :This method is used for receiving frame, this method is called by IRQ handler
//                when frame is received

//              p_cmd_st :Pointer to command object

u8 receive_frame(struct sdvt_can_classdev *cdev,
		   struct sdvt_can_command *p_cmd_st,
		   struct sdvt_can_config *p_config_st)
{
	u8   m_data_8b; // Command control register
	u8   m_len_8b = 0; // Command control register
	u32  m_can_id[5];
	u32  can_index;
	int  m_idx_i; // Iterative variable

	// Read the length from RX LENGTH FIFO
	for (m_idx_i = 0; m_idx_i < 2; m_idx_i += 1) {
		m_data_8b         = cdev->ops->read_reg(cdev, SDVT_CAN_RX_LEN_FIFO);
		p_cmd_st->rx_len_2d_8b[m_idx_i] = m_data_8b;
		m_len_8b = m_data_8b;
	}

	if (p_cmd_st->rx_len_2d_8b[0] > 5) {
		dev_err(cdev->dev, "ERR : %s() :: id len = %d\n", __func__, p_cmd_st->rx_len_2d_8b[0]);
		return 1;
	}

	if (p_config_st->cfg_ext_frame_mode_b == SDVT_CAN_STANDARD_FRAME) {
		for (m_idx_i = 0; m_idx_i < p_cmd_st->rx_len_2d_8b[0]; m_idx_i++)
			m_can_id[m_idx_i] = cdev->ops->read_reg(cdev, SDVT_CAN_RX_DATA_FIFO);
			//flush invalid data:0xa 0xc2

		 p_cmd_st->ident_32b = (m_can_id[0] & 0x7ff) << 3 | (m_can_id[1] & 0x7ff) >> 5;
	} else {
		for (m_idx_i = 0; m_idx_i < p_cmd_st->rx_len_2d_8b[0]; m_idx_i++)
			m_can_id[m_idx_i] = cdev->ops->read_reg(cdev, SDVT_CAN_RX_DATA_FIFO);

		if (p_cmd_st->rx_len_2d_8b[0] == 3) {
			p_cmd_st->ident_32b = (((m_can_id[1] & 0x01) << 10) |
					      ((m_can_id[2] & 0xe0)  << 2) |
					      ((m_can_id[1] & 0xfe) >> 1));
		} else if (p_cmd_st->rx_len_2d_8b[0] == 0x05) {
			can_index = ((m_can_id[1] << 24) & 0xFFFFFFFF) |
			((m_can_id[2] << 16) & 0xFFFFFFFF) |
			((m_can_id[3] << 8) & 0xFFFFFFFF) |
			(m_can_id[4] & 0xFFFFFFFF);
			p_cmd_st->ident_32b = ((can_index >> 3) & 0x1FFFFFFF) | CAN_EFF_FLAG;
		}
	}

	// Read the data from RX DATA FIFO based on length

	if (m_len_8b > 64) {
		dev_err(cdev->dev, "ERR : %s() :: data len = %d\n", __func__, m_len_8b);
		m_len_8b = 64;
	}

	for (m_idx_i = 0; m_idx_i < m_len_8b; m_idx_i += 1) {
		m_data_8b = cdev->ops->read_reg(cdev, SDVT_CAN_RX_DATA_FIFO);
		p_cmd_st->rx_data_2d_8b[m_idx_i] = m_data_8b;
	}
	return 0;
}

u8 receive_remote_frame(struct sdvt_can_classdev *cdev,
			  struct sdvt_can_command *p_cmd_st,
			  struct sdvt_can_config *p_config_st)
{
	u8   m_data_8b; // Command control register
	u8   m_len_8b = 0; // Command control register
	u32  m_can_id[5];
	u32  can_index;
	int  m_idx_i; // Iterative variable
	// Read the length from RX LENGTH FIFO
	for (m_idx_i = 0; m_idx_i < 2; m_idx_i += 1) {
		m_data_8b         = cdev->ops->read_reg(cdev, SDVT_CAN_RX_LEN_FIFO);
		p_cmd_st->rx_len_2d_8b[m_idx_i] = m_data_8b;
		m_len_8b = m_data_8b;
	}

	if (p_cmd_st->rx_len_2d_8b[0] > 5) {
		dev_err(cdev->dev, "ERR : %s() :: id len = %d\n", __func__, p_cmd_st->rx_len_2d_8b[0]);
		return 1;
	}
	if (p_config_st->cfg_ext_frame_mode_b == SDVT_CAN_STANDARD_FRAME) {
		for (m_idx_i = 0; m_idx_i < p_cmd_st->rx_len_2d_8b[0]; m_idx_i++)
			m_can_id[m_idx_i] = cdev->ops->read_reg(cdev, SDVT_CAN_RX_DATA_FIFO);
			//flush invalid data:0xa 0xc2

		p_cmd_st->ident_32b = ((m_can_id[0] & 0x7ff) << 3 | (m_can_id[1] & 0x7ff) >> 5);
	} else {
		for (m_idx_i = 0; m_idx_i < p_cmd_st->rx_len_2d_8b[0]; m_idx_i++)
			m_can_id[m_idx_i] = cdev->ops->read_reg(cdev, SDVT_CAN_RX_DATA_FIFO);

		if (p_cmd_st->rx_len_2d_8b[0] == 0x03) {
			p_cmd_st->ident_32b = (((m_can_id[1] & 0x01) << 10) |
					      ((m_can_id[2] & 0xe0)  << 2) |
					      ((m_can_id[1] & 0xfe) >> 1));
		} else if (p_cmd_st->rx_len_2d_8b[0] == 0x05) {
			can_index = ((m_can_id[1] << 24) & 0xFFFFFFFF) |
				    ((m_can_id[2] << 16) & 0xFFFFFFFF) |
				    ((m_can_id[3] << 8) & 0xFFFFFFFF) |
				    (m_can_id[4] & 0xFFFFFFFF);
			p_cmd_st->ident_32b = ((can_index >> 3) & 0x1FFFFFFF) | CAN_EFF_FLAG;
		}
	}
	return 0;
}

// detect_irq_status :This method is used to handle the interrupts

//              p_cmd_st :Pointer to command object

void detect_irq_status(struct sdvt_can_classdev *cdev, struct sdvt_can_command *p_cmd_st)
{
	u8 m_rd_status_2b ; // Status 1 reg

	p_cmd_st->irq_status0_8b = 0;
	p_cmd_st->irq_status1_8b = 0;
	p_cmd_st->irq_status4_8b = 0;
	p_cmd_st->irq_status5_8b = 0;

	m_rd_status_2b = cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_STATUS0);
	if (m_rd_status_2b) {
		cdev->ops->write_reg(cdev, SDVT_CAN_IRQ_STATUS0, m_rd_status_2b);
		p_cmd_st->irq_status0_8b = m_rd_status_2b;
	}

	m_rd_status_2b = cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_STATUS1);
	if (m_rd_status_2b) {
		cdev->ops->write_reg(cdev, SDVT_CAN_IRQ_STATUS1, m_rd_status_2b);
		p_cmd_st->irq_status1_8b = m_rd_status_2b;
	}
	m_rd_status_2b = cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_STATUS4);
	cdev->ops->write_reg(cdev, SDVT_CAN_IRQ_STATUS4, m_rd_status_2b);
	p_cmd_st->irq_status4_8b = m_rd_status_2b;

	m_rd_status_2b = cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_STATUS5);
	cdev->ops->write_reg(cdev, SDVT_CAN_IRQ_STATUS5, m_rd_status_2b);
	p_cmd_st->irq_status5_8b = m_rd_status_2b;

}

// mask_irq :This method is used to mask the IRQ
// p_config_st :Pointer to config object
// p_mask_8b :Interrupts to mask

void sdvt_mask_irq(struct sdvt_can_classdev *cdev,
		   struct sdvt_can_config *p_config_st,
		   u32 p_addr, u8 p_mask_8b)
{
	u8 m_new_mask_8b ; // New register mask

	switch (p_addr) {
	case SDVT_CAN_IRQ_ENABLE0:
		m_new_mask_8b = cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_ENABLE0) & ~p_mask_8b;
		p_config_st->irq_enable0_8b = m_new_mask_8b;
		cdev->ops->write_reg(cdev, SDVT_CAN_IRQ_ENABLE0, m_new_mask_8b);
		break;
	case SDVT_CAN_IRQ_ENABLE1:
		m_new_mask_8b = cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_ENABLE1) & ~p_mask_8b;
		p_config_st->irq_enable1_8b = m_new_mask_8b;
		cdev->ops->write_reg(cdev, SDVT_CAN_IRQ_ENABLE1, m_new_mask_8b);
		break;
	case SDVT_CAN_IRQ_ENABLE2:
		m_new_mask_8b = cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_ENABLE2) & ~p_mask_8b;
		p_config_st->irq_enable2_8b = m_new_mask_8b;
		cdev->ops->write_reg(cdev, SDVT_CAN_IRQ_ENABLE2, m_new_mask_8b);
		break;
	case SDVT_CAN_IRQ_ENABLE3:
		m_new_mask_8b = cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_ENABLE3) & ~p_mask_8b;
		p_config_st->irq_enable3_8b = m_new_mask_8b;
		cdev->ops->write_reg(cdev, SDVT_CAN_IRQ_ENABLE3, m_new_mask_8b);
		break;
	case SDVT_CAN_IRQ_ENABLE4:
		m_new_mask_8b = cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_ENABLE4) & ~p_mask_8b;
		p_config_st->irq_enable4_8b = m_new_mask_8b;
		cdev->ops->write_reg(cdev, SDVT_CAN_IRQ_ENABLE4, m_new_mask_8b);
		break;
	case SDVT_CAN_IRQ_ENABLE5:
		m_new_mask_8b = cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_ENABLE5) & ~p_mask_8b;
		p_config_st->irq_enable5_8b = m_new_mask_8b;
		cdev->ops->write_reg(cdev, SDVT_CAN_IRQ_ENABLE5, m_new_mask_8b);
		break;
	default:
		break;
	}
}

// unmask_irq :This method is used to unmask the IRQ
// p_config_st :Pointer to config object
// p_mask_8b :Interrupts to mask

void sdvt_unmask_irq(struct sdvt_can_classdev *cdev,
		     struct sdvt_can_config *p_config_st,
		     u32 p_addr, u8 p_mask_8b)
{
	u8 m_new_mask_8b ; // New register mask

	switch (p_addr) {
	case SDVT_CAN_IRQ_ENABLE0:
		m_new_mask_8b = cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_ENABLE0) | p_mask_8b;
		p_config_st->irq_enable0_8b = m_new_mask_8b;
		cdev->ops->write_reg(cdev, SDVT_CAN_IRQ_ENABLE0, m_new_mask_8b);
		break;
	case SDVT_CAN_IRQ_ENABLE1:
		m_new_mask_8b = cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_ENABLE1) | p_mask_8b;
		p_config_st->irq_enable1_8b = m_new_mask_8b;
		cdev->ops->write_reg(cdev, SDVT_CAN_IRQ_ENABLE1, m_new_mask_8b);
		break;
	case SDVT_CAN_IRQ_ENABLE2:
		m_new_mask_8b = cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_ENABLE2) | p_mask_8b;
		p_config_st->irq_enable2_8b = m_new_mask_8b;
		cdev->ops->write_reg(cdev, SDVT_CAN_IRQ_ENABLE2, m_new_mask_8b);
		break;
	case SDVT_CAN_IRQ_ENABLE3:
		m_new_mask_8b = cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_ENABLE3) | p_mask_8b;
		p_config_st->irq_enable3_8b = m_new_mask_8b;
		cdev->ops->write_reg(cdev, SDVT_CAN_IRQ_ENABLE3, m_new_mask_8b);
		break;
	case SDVT_CAN_IRQ_ENABLE4:
		m_new_mask_8b = cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_ENABLE4) | p_mask_8b;
		p_config_st->irq_enable4_8b = m_new_mask_8b;
		cdev->ops->write_reg(cdev, SDVT_CAN_IRQ_ENABLE4, m_new_mask_8b);
		break;
	case SDVT_CAN_IRQ_ENABLE5:
		m_new_mask_8b = cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_ENABLE5) | p_mask_8b;
		p_config_st->irq_enable5_8b = m_new_mask_8b;
		cdev->ops->write_reg(cdev, SDVT_CAN_IRQ_ENABLE5, m_new_mask_8b);
		break;
	default:
		break;
	}
}

// wait_tx_done :This method is used for waiting for completion of request
// parameter :No parameter

int32_t wait_tx_done(struct sdvt_can_classdev *cdev)
{
	u8 can_tx_status = 0; // Command control register
	u32 timeout = CAN_TXRX_TIMEOUT;

	// m_data_8b = cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_STATUS1);
	// m_data_8b = m_data_8b & (1 << SDVT_CAN_IRQ_TX_DONE);

	while (can_tx_status == 0 && timeout > 0) {
		udelay(10);
		can_tx_status = cdev->ops->read_reg(cdev, SDVT_CAN_STATUS);
		can_tx_status = can_tx_status & (0x1 << SDVT_CAN_STATUS_TX_DONE);
		timeout--;
	}
	if (timeout <= 0) {
		dev_err(cdev->dev, "%s:tx busy\n", __func__);
		return -1;
	}
	return 0;
}
void abort_tx(struct sdvt_can_classdev *cdev)
{
	u8 command = 1; // Command control register

	dev_err(cdev->dev, "%s:abort tx\n", __func__);
	command = cdev->ops->read_reg(cdev, SDVT_CAN_COMMAND);
	cdev->ops->write_reg(cdev, SDVT_CAN_COMMAND, command | (0x1 << SDVT_CAN_CMD_ABORT));
	cdev->ops->write_reg(cdev, SDVT_CAN_COMMAND, command);
}

// wait_rx_valid :This method is used for waiting for completion of request
// parameter :No parameter

int32_t wait_rx_valid(struct sdvt_can_classdev *cdev)
{
	u8  m_data_8b ; // Command control register
	u32 timeout = CAN_TXRX_TIMEOUT;

	m_data_8b = cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_STATUS1);
	m_data_8b = m_data_8b & (1 << SDVT_CAN_IRQ_RX_DATA_FRAME);

	// Command is pending
	while (m_data_8b == 0 && --timeout) {
		m_data_8b   = cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_STATUS1);
		m_data_8b   = m_data_8b & (1 << SDVT_CAN_IRQ_RX_DATA_FRAME);
		mdelay(1);
	}

	if (timeout <= 0) {
		dev_info(cdev->dev, "rx timeout!\n");
		return -1;
	}

	return 0;
}

void print_register(struct sdvt_can_classdev *cdev)
{
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_CONTROL:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_CONTROL));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_EXT_CONTROL:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_EXT_CONTROL));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_FD_CONTROL:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_FD_CONTROL));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_COMMAND:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_COMMAND));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_DLC_REMOTE_FRAME:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_DLC_REMOTE_FRAME));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_STATUS:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_STATUS));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_ACCEPTANCE_CODE0:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_ACCEPTANCE_CODE0));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_ACCEPTANCE_CODE1:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_ACCEPTANCE_CODE1));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_ACCEPTANCE_CODE2:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_ACCEPTANCE_CODE2));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_ACCEPTANCE_CODE3:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_ACCEPTANCE_CODE3));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_ACCEPTANCE_MASK0:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_ACCEPTANCE_MASK0));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_ACCEPTANCE_MASK1:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_ACCEPTANCE_MASK1));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_ACCEPTANCE_MASK2:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_ACCEPTANCE_MASK2));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_ACCEPTANCE_MASK3:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_ACCEPTANCE_MASK3));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_NOR_CLK_DIVIDER:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_NOR_CLK_DIVIDER));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_NOR_BUS_TIMING0:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_NOR_BUS_TIMING0));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_NOR_BUS_TIMING1:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_NOR_BUS_TIMING1));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_FD_CLK_DIVIDER:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_FD_CLK_DIVIDER));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_FD_BUS_TIMING0:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_FD_BUS_TIMING0));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_FD_BUS_TIMING1:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_FD_BUS_TIMING1));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_ARB_ID0:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_ARB_ID0));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_ARB_ID1:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_ARB_ID1));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_ARB_ID2:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_ARB_ID2));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_ARB_ID3:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_ARB_ID3));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_ARB_ID4:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_ARB_ID4));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_RSP_ARB_ID0:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_RSP_ARB_ID0));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_RSP_ARB_ID1:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_RSP_ARB_ID1));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_RSP_ARB_ID2:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_RSP_ARB_ID2));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_RSP_ARB_ID3:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_RSP_ARB_ID3));
	// dev_info(cdev->dev,"INFO : print_register() ::SDVT_CAN_TX_DATA_FIFO:: 0x%x\n",
	//	 cdev->ops->read_reg(cdev,SDVT_CAN_TX_DATA_FIFO));
	// dev_info(cdev->dev,"INFO : print_register() ::SDVT_CAN_RX_DATA_FIFO:: 0x%x\n",
	//	 cdev->ops->read_reg(cdev,SDVT_CAN_RX_DATA_FIFO));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_RX_LEN_FIFO:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_RX_LEN_FIFO));
	// dev_info(cdev->dev,"INFO : print_register() ::SDVT_CAN_TX_RSP_FIFO:: 0x%x\n",
	//	 cdev->ops->read_reg(cdev,SDVT_CAN_TX_RSP_FIFO));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_ARB_LOST_CAPTURE:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_ARB_LOST_CAPTURE));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_ERR_CODE_CAPTURE:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_ERR_CODE_CAPTURE));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_ERR_WARN_LIMIT:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_ERR_WARN_LIMIT));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_RX_ERR_COUNTER:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_RX_ERR_COUNTER));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_TX_ERR_COUNTER:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_TX_ERR_COUNTER));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_ERR_COUNTER_LOAD:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_ERR_COUNTER_LOAD));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_ERR_DATA:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_ERR_DATA));
	// dev_info(cdev->dev,"INFO : print_register() ::SDVT_CAN_FIFO_FLUSH:: 0x%x\n",
	//	 cdev->ops->read_reg(cdev,SDVT_CAN_FIFO_FLUSH));
	// dev_info(cdev->dev,"INFO : print_register() ::SDVT_CAN_TX_DATA_FIFO_THRESHOLD:: 0x%x\n",
	//	 cdev->ops->read_reg(cdev,SDVT_CAN_TX_DATA_FIFO_THRESHOLD));
	// dev_info(cdev->dev,"INFO : print_register() ::SDVT_CAN_TX_RSP_FIFO_THRESHOLD:: 0x%x\n",
	//	 cdev->ops->read_reg(cdev,SDVT_CAN_TX_RSP_FIFO_THRESHOLD));
	// dev_info(cdev->dev,"INFO : print_register() ::SDVT_CAN_RX_DATA_FIFO_THRESHOLD:: 0x%x\n",
	//   cdev->ops->read_reg(cdev,SDVT_CAN_RX_DATA_FIFO_THRESHOLD));
	// dev_info(cdev->dev,"INFO : print_register() ::SDVT_CAN_RX_LEN_FIFO_THRESHOLD:: 0x%x\n",
	//	 cdev->ops->read_reg(cdev,SDVT_CAN_RX_LEN_FIFO_THRESHOLD));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_FIFO_STATUS:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_FIFO_STATUS));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_TX_DATA_FIFO_DEPTH:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_TX_DATA_FIFO_DEPTH));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_TX_DATA_FIFO_AVAIL:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_TX_DATA_FIFO_AVAIL));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_TX_RSP_FIFO_DEPTH:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_TX_RSP_FIFO_DEPTH));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_TX_RSP_FIFO_AVAIL:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_TX_RSP_FIFO_AVAIL));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_RX_DATA_FIFO_DEPTH:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_RX_DATA_FIFO_DEPTH));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_RX_DATA_FIFO_AVAIL:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_RX_DATA_FIFO_AVAIL));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_RX_LEN_FIFO_DEPTH:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_RX_LEN_FIFO_DEPTH));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_RX_LEN_FIFO_AVAIL:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_RX_LEN_FIFO_AVAIL));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_IRQ_ENABLE0:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_ENABLE0));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_IRQ_STATUS0:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_STATUS0));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_IRQ_ENABLE1:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_ENABLE1));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_IRQ_STATUS1:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_STATUS1));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_IRQ_ENABLE2:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_ENABLE2));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_IRQ_STATUS2:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_STATUS2));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_IRQ_ENABLE3:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_ENABLE3));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_IRQ_STATUS3:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_STATUS3));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_IRQ_ENABLE4:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_ENABLE4));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_IRQ_STATUS4:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_STATUS4));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_IRQ_ENABLE5:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_ENABLE5));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_IRQ_STATUS5:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_IRQ_STATUS5));
	dev_info(cdev->dev, "INFO : %s() ::SDVT_CAN_SOC_TIMEOUT:: 0x%x\n", __func__,
		 cdev->ops->read_reg(cdev, SDVT_CAN_SOC_TIMEOUT));
}

void print_config(struct sdvt_can_classdev *cdev, struct sdvt_can_config *p_config_st)
{
	dev_info(cdev->dev, "INFO : %s() :: ARB_ID0              :: 0x%x\n",
		 __func__, p_config_st->std_arb_id0_8b);
	dev_info(cdev->dev, "INFO : %s() :: ARB_ID1              :: 0x%x\n",
		 __func__, p_config_st->std_arb_id1_8b);
	dev_info(cdev->dev, "INFO : %s() :: ARB_ID2              :: 0x%x\n",
		 __func__, p_config_st->std_arb_id2_8b);
	dev_info(cdev->dev, "INFO : %s() :: ARB_ID3              :: 0x%x\n",
		 __func__, p_config_st->std_arb_id3_8b);
	dev_info(cdev->dev, "INFO : %s() :: ARB_ID4              :: 0x%x\n",
		 __func__, p_config_st->std_arb_id4_8b);
	dev_info(cdev->dev, "INFO : %s() :: CAN_MODE             :: 0x%x\n",
		 __func__, p_config_st->cfg_can_mode_2b);
	dev_info(cdev->dev, "INFO : %s() :: EXT_FRAME            :: 0x%x\n",
		 __func__, p_config_st->cfg_ext_frame_mode_b);
	dev_info(cdev->dev, "INFO : %s() :: FD_BRS               :: 0x%x\n",
		 __func__, p_config_st->cfg_fd_brs_b);
	dev_info(cdev->dev, "INFO : %s() :: REMOTE_RESP_ENABLE   :: 0x%x\n",
		 __func__, p_config_st->cfg_remote_resp_en_b);
	dev_info(cdev->dev, "INFO : %s() :: ACCEPTANCE_CODE0     :: 0x%x\n",
		 __func__, p_config_st->cfg_acceptance_code0_8b);
	dev_info(cdev->dev, "INFO : %s() :: ACCEPTANCE_CODE1     :: 0x%x\n",
		 __func__, p_config_st->cfg_acceptance_code1_8b);
	dev_info(cdev->dev, "INFO : %s() :: ACCEPTANCE_CODE2     :: 0x%x\n",
		 __func__, p_config_st->cfg_acceptance_code2_8b);
	dev_info(cdev->dev, "INFO : %s() :: ACCEPTANCE_CODE3     :: 0x%x\n",
		 __func__, p_config_st->cfg_acceptance_code3_8b);
	dev_info(cdev->dev, "INFO : %s() :: ACCEPTANCE_MASK0     :: 0x%x\n",
		 __func__, p_config_st->cfg_acceptance_mask0_8b);
	dev_info(cdev->dev, "INFO : %s() :: ACCEPTANCE_MASK1     :: 0x%x\n",
		 __func__, p_config_st->cfg_acceptance_mask1_8b);
	dev_info(cdev->dev, "INFO : %s() :: ACCEPTANCE_MASK2     :: 0x%x\n",
		 __func__, p_config_st->cfg_acceptance_mask2_8b);
	dev_info(cdev->dev, "INFO : %s() :: ACCEPTANCE_MASK3     :: 0x%x\n",
		 __func__, p_config_st->cfg_acceptance_mask3_8b);
	dev_info(cdev->dev, "INFO : %s() :: NOR_CLK_DIV          :: 0x%x\n",
		 __func__, p_config_st->cfg_nor_clk_divider_8b);
	dev_info(cdev->dev, "INFO : %s() :: NOR_BAUD_PRE         :: 0x%x\n",
		 __func__, p_config_st->cfg_nor_baud_prescaler_6b);
	dev_info(cdev->dev, "INFO : %s() :: NOR_SYNC_JUMP_WIDTH  :: 0x%x\n",
		 __func__, p_config_st->cfg_nor_sync_jump_width_2b);
	dev_info(cdev->dev, "INFO : %s() :: NOR_TIM_SEG1         :: 0x%x\n",
		 __func__, p_config_st->cfg_nor_time_segment1_3b);
	dev_info(cdev->dev, "INFO : %s() :: NOR_TIM_SEG1         :: 0x%x\n",
		 __func__, p_config_st->cfg_nor_time_segment2_4b);
	dev_info(cdev->dev, "INFO : %s() :: NOR_TRIPLE_SAMP      :: 0x%x\n",
		 __func__, p_config_st->cfg_nor_time_triple_sample_b);
	dev_info(cdev->dev, "INFO : %s() :: FD_CLK_DIV           :: 0x%x\n",
		 __func__, p_config_st->cfg_fd_clk_divider_8b);
	dev_info(cdev->dev, "INFO : %s() :: FD_BAUD_PRE          :: 0x%x\n",
		 __func__, p_config_st->cfg_fd_baud_prescaler_6b);
	dev_info(cdev->dev, "INFO : %s() :: FD_SYNC_JUMP_WIDTH   :: 0x%x\n",
		 __func__, p_config_st->cfg_fd_sync_jump_width_2b);
	dev_info(cdev->dev, "INFO : %s() :: FD_TIM_SEG1          :: 0x%x\n",
		 __func__, p_config_st->cfg_fd_time_segment1_4b);
	dev_info(cdev->dev, "INFO : %s() :: FD_TIM_SEG1          :: 0x%x\n",
		 __func__, p_config_st->cfg_fd_time_segment2_3b);
	dev_info(cdev->dev, "INFO : %s() :: FD_TRIPLE_SAMP       :: 0x%x\n",
		 __func__, p_config_st->cfg_fd_time_triple_sample_b);
	dev_info(cdev->dev, "INFO : %s() :: BUS_TIMING0          :: 0x%x\n",
		 __func__, p_config_st->bus_timing0_8b);
	dev_info(cdev->dev, "INFO : %s() :: BUS_TIMING1          :: 0x%x\n",
		 __func__, p_config_st->bus_timing1_8b);
	dev_info(cdev->dev, "INFO : %s() :: EXT_CTRL_LISTEN      :: 0x%x\n",
		 __func__, p_config_st->cfg_ext_ctrl_listen_b);
	dev_info(cdev->dev, "INFO : %s() :: EXT_CTRL_SELF_TEST   :: 0x%x\n",
		 __func__, p_config_st->cfg_ext_ctrl_self_test_b);
	dev_info(cdev->dev, "INFO : %s() :: EXT_CTRL_ACC_FILTER  :: 0x%x\n",
		 __func__, p_config_st->cfg_ext_ctrl_acc_filter_b);
	dev_info(cdev->dev, "INFO : %s() :: EXT_CTRL_REC_INCR    :: 0x%x\n",
		 __func__, p_config_st->cfg_ext_ctrl_rec_incr_b);
	dev_info(cdev->dev, "INFO : %s() :: EXT_CTRL_REMOTE_EN   :: 0x%x\n",
		 __func__, p_config_st->cfg_ext_ctrl_remote_en_b);
	dev_info(cdev->dev, "INFO : %s() :: EXT_CTRL_ID_RX       :: 0x%x\n",
		 __func__, p_config_st->cfg_ext_ctrl_id_rx_b);
	dev_info(cdev->dev, "INFO : %s() :: IRQ_ENABLE0          :: 0x%x\n",
		 __func__, p_config_st->irq_enable0_8b);
	dev_info(cdev->dev, "INFO : %s() :: IRQ_ENABLE1          :: 0x%x\n",
		 __func__, p_config_st->irq_enable1_8b);
	dev_info(cdev->dev, "INFO : %s() :: IRQ_ENABLE2          :: 0x%x\n",
		 __func__, p_config_st->irq_enable2_8b);
	dev_info(cdev->dev, "INFO : %s() :: IRQ_ENABLE3          :: 0x%x\n",
		 __func__, p_config_st->irq_enable3_8b);
}

// print_command :This method is used to print command being sent
// p_cmd_st :Pointer to command object

void print_command(struct sdvt_can_classdev *cdev, struct sdvt_can_command *p_cmd_st)
{
	dev_info(cdev->dev, "INFO : %s() :: COMMAND              :: 0x%x\n",
		 __func__, p_cmd_st->command_8b);
	dev_info(cdev->dev, "INFO : %s() :: DLC                  :: 0x%x\n",
		 __func__, p_cmd_st->dlc_4b);
	dev_info(cdev->dev, "INFO : %s() :: DATA                 :: 0x%x\n",
		 __func__, p_cmd_st->data_2d_8b[0]);
	dev_info(cdev->dev, "INFO : %s() :: IDENTIFIER           :: 0x%x\n",
		 __func__, p_cmd_st->ident_32b);
	dev_info(cdev->dev, "INFO : %s() :: IRQ STATUS1          :: 0x%x\n",
		 __func__, p_cmd_st->irq_status1_8b);
}

//                           SmartDV Technologies Proprietary
//            Copyright 2007-2023 SmartDV Technologies India Private Limited
//
//                              CONFIDENTIAL INFORMATION
//
//                                 All rights reserved
//
//            The use, modification, or duplication of this product is protected
//            according to SmartDV Technologies's licensing agreement.
//
//            This Intellectual property contains confidential and proprietary
//            information which are the properties of SmartDV Technologies.
//
//            Unauthorized use, disclosure, duplication, or reproduction are prohibited.

#endif
