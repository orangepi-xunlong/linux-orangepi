#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include "cvitek_can.h"

#ifndef SDVT_CAN_BASIC_H
#define SDVT_CAN_BASIC_H

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
// Description    : This is C header file for sdvt_can IP core
// Generator      : IIP Compiler Version 1.1


#define STANDARD_DATA_FRAME                                        0         // Standard data frame
#define EXTENDED_DATA_FRAME                                        1         // Extended data frame
#define STANDARD_REMOTE_FRAME                                      2         // Standard remote frame
#define EXTENDED_REMOTE_FRAME                                      3         // Extended remote frame

// write_byte_reg :#This is for register byte write
//            p_addr_32b :#Register address to write
//             p_data_8b :#Register data to write
extern void write_byte_reg (uint32_t p_addr_32b,u8 p_data_8b);

// write_reg :#This is for register write
//            p_addr_32b :#Register address to write
//             p_data_8b :#Register data to write
extern void write_reg (uint32_t p_addr_32b,u8 p_data_8b);



// read_reg :#This is for register read
//               returns :#Return the read data
//            p_addr_32b :#Register address to read

extern u8  read_reg (uint32_t p_addr_32b);

// sdvt_init_chip :#This method is used to initialize the chip
//           p_config_st :#Pointer to config object
void sdvt_init_chip (struct sdvt_can_classdev *cdev, struct sdvt_can_config* p_config_st);

// send_command :#This method is used for sending command
//              p_cmd_st :# Pointer to command object
//           p_config_st :# Pointer to config object
void send_command ( struct sdvt_can_classdev *cdev,struct sdvt_can_command* p_cmd_st,struct sdvt_can_config* p_config_st);

// receive_frame :#This method is used for receiving frame, this method is called by IRQ handler
//                when frame is received
//              p_cmd_st :# Pointer to command object

u8 receive_frame(struct sdvt_can_classdev *cdev,
				 struct sdvt_can_command *p_cmd_st,
				 struct sdvt_can_config *p_config_st);

u8 receive_remote_frame(struct sdvt_can_classdev *cdev,
						struct sdvt_can_command *p_cmd_st,
						struct sdvt_can_config *p_config_st);

// detect_irq_status :# This method is used to handle the interrupts
//              p_cmd_st :# Pointer to command object

void detect_irq_status(struct sdvt_can_classdev *cdev, struct sdvt_can_command *p_cmd_st);



// mask_irq :# This method is used to mask the IRQ
//           p_config_st :# Pointer to config object
//             p_mask_8b :# Interrupts to mask

void sdvt_mask_irq (struct sdvt_can_classdev *cdev,struct sdvt_can_config *p_config_st,uint32_t p_addr,u8 p_mask_8b);



// unmask_irq :# This method is used to unmask the IRQ
//           p_config_st :# Pointer to config object
//             p_mask_8b :# Interrupts to mask
void sdvt_unmask_irq (struct sdvt_can_classdev *cdev,struct sdvt_can_config *p_config_st,uint32_t p_addr,u8 p_mask_8b);



// wait_tx_done :#This method is used for waiting for completion of request
//             parameter :No parameter

int32_t wait_tx_done(struct sdvt_can_classdev *cdev);

// abort_tx :#This method is used for abort_tx
//             parameter :No parameter

void abort_tx(struct sdvt_can_classdev *cdev);

// wait_rx_valid :#This method is used for waiting for completion of request
//             parameter :No parameter

int32_t wait_rx_valid(struct sdvt_can_classdev *cdev);


// print_config :# This method is used to print all registers
//           parameter :No parameter

void print_register(struct sdvt_can_classdev *cdev);



// print_config :# This method is used to print all config registers
//           p_config_st :# Pointer to config object

void print_config(struct sdvt_can_classdev *cdev, struct sdvt_can_config *p_config_st);



// print_command :# This method is used to print command being sent

//              p_cmd_st :#Pointer to command object

void print_command(struct sdvt_can_classdev *cdev, struct sdvt_can_command *p_cmd_st);

#define CAN_TXRX_TIMEOUT 200
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

#endif
