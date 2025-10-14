//------------------------------------------------------------------------------
//  COPYRIGHT (c) 2018-2022 TRILINEAR TECHNOLOGIES, INC.
//  CONFIDENTIAL AND PROPRIETARY
//
//  THE SOURCE CODE CONTAINED HEREIN IS PROVIDED ON AN "AS IS" BASIS.
//  TRILINEAR TECHNOLOGIES, INC. DISCLAIMS ANY AND ALL WARRANTIES,
//  WHETHER EXPRESS, IMPLIED, OR STATUTORY, INCLUDING ANY IMPLIED
//  WARRANTIES OF MERCHANTABILITY OR OF FITNESS FOR A PARTICULAR PURPOSE.
//  IN NO EVENT SHALL TRILINEAR TECHNOLOGIES, INC. BE LIABLE FOR ANY
//  INCIDENTAL, PUNITIVE, OR CONSEQUENTIAL DAMAGES OF ANY KIND WHATSOEVER
//  ARISING FROM THE USE OF THIS SOURCE CODE.
//
//  THIS DISCLAIMER OF WARRANTY EXTENDS TO THE USER OF THIS SOURCE CODE
//  AND USER'S CUSTOMERS, EMPLOYEES, AGENTS, TRANSFEREES, SUCCESSORS,
//  AND ASSIGNS.
//
//  THIS IS NOT A GRANT OF PATENT RIGHTS
//------------------------------------------------------------------------------
#ifndef __DPTX_AUX_REG_H__
#define __DPTX_AUX_REG_H__

//------------------------------------------------------------------------------
//  TR_DPTX_AUX_COMMAND (offset 0x100)
//
//  Initiates an AUX channel command of the specified length when written.
//  This register is written last as part of the AUX channel request set up process.
//  When written, the internal state machines will begin transmitting the request to the sink device.
//  The AUX_ADDRESS and, when applicable, the AUX_WRITE_FIFO must be set up before writing this register.
//
//  13 - AUX_PHY_WAKE:
//      Set to a 1 to send the AUX_PHY_WAKE signal for embedded DisplayPort operations.
//      This signal is sent as a standalone operation and is not sent as a part of a larger AUX transaction.
//
//  12 - ADDRESS_ONLY:
//      Set to a 1 to initiate an address only request.
//
//  11:8 - COMMAND:
//      AUX Channel Command. The field is equivalent to the commands defined in
//      section 2.4.1 of the DisplayPort specification.
//      0x8 = AUX Write
//      0x9 = AUX Read
//      0x0 = I2C over AUX Write
//      0x4 = I2C over AUX Write, Middle of Transaction bit set
//      0x1 = I2C over AUX Read
//      0x5 = I2C over AUX Read, Middle of Transaction bit set
//      0x2 = I2C over AUX Write Status
//
//  3:0 – BYTE_COUNT:
//      Specifies the number of bytes to transfer with the current command.
//      The range of the register is 0 to 15 indicating between 1 and 16 bytes of data.
//------------------------------------------------------------------------------
#define TR_DPTX_AUX_CMD_PHY_WAKE (0x01ul << 13)
#define TR_DPTX_AUX_CMD_ADDRESS_ONLY (0x01ul << 12)

#define TR_DPTX_AUX_CMD_MASK 0x0f00ul
#define TR_DPTX_AUX_CMD_READ 0x0900ul
#define TR_DPTX_AUX_CMD_WRITE 0x0800ul

#define TR_DPTX_AUX_CMD_I2C_WRITE 0x0000ul
#define TR_DPTX_AUX_CMD_I2C_READ 0x0100ul
#define TR_DPTX_AUX_CMD_I2C_WRITE_STATUS 0x0200ul
#define TR_DPTX_AUX_CMD_I2C_READ_MOT 0x0500ul
#define TR_DPTX_AUX_CMD_I2C_WRITE_MOT 0x0400ul
#define TR_DPTX_AUX_CMD_I2C_WRITE_STATUS_MOT 0x0600ul

#define TR_DPTX_AUX_BYTE_CT_MASK 0x000ful

//------------------------------------------------------------------------------
//  TR_DPTX_AUX_REPLY_CODE (offset 0x138)
//  Contains the reply code received from the most recent AUX Channel request.
//  This code is the two, two-bit fields received by the reply controller from the sink device.
//  The value of the code maps to the DisplayPort specification section 2.4.1.2 which
//  is repeated below for clarity.
//
//  1:0 – AUX channel native reply codes received from the sink device.
//  0x0 = Native AUX ACK
//  0x1 = Native AUX NACK
//  0x2 = Native AUX Defer
//
//  3:2 – AUX channel I2C reply codes received from the sink device.
//  0x0 = I2C over AUX ACK
//  0x4 = I2C over AUX NACK
//  0x8 = I2C over AUX Defer
//------------------------------------------------------------------------------
#define TR_DPTX_AUX_REPLY_NATIVE_MASK 0x03ul
#define TR_DPTX_AUX_REPLY_ACK 0x00ul
#define TR_DPTX_AUX_REPLY_NACK 0x01ul
#define TR_DPTX_AUX_REPLY_DEFER 0x02ul

#define TR_DPTX_AUX_REPLY_I2C_MASK 0x0cul
#define TR_DPTX_AUX_REPLY_I2C_ACK 0x00ul
#define TR_DPTX_AUX_REPLY_I2C_NACK 0x04ul
#define TR_DPTX_AUX_REPLY_I2C_DEFER 0x08ul

#define TR_DPTX_AUX_REPLY_MASK 0x0ful

//------------------------------------------------------------------------------
//  TR_DPTX_AUX_STATUS (offset 0x14c)
//
//  This register contains the status of the internal AUX channel controllers.
//  The progress of request and reply transactions are monitored and reply transactions
//  are checked for errors. These bits are always valid:
//
//  3 – REPLY_ERROR: When set to a 1, the AUX reply logic has detected an error in
//      the reply to the most recent AUX transaction. Errors are detected when the
//      precharge and sync phases of the reply last more than 38 cycles instead of the maximum 32.
//      This condition typically indicates noise on the AUX channel data signals.
//
//  2 – REQUEST_IN_PROGRESS: The AUX transaction request controller sets this bit to a 1
//      while actively transmitting a request on the AUX channel. The bit is set to 0 when
//      the AUX transaction request controller is idle.
//
//  1 – REPLY_IN_PROGRESS: The AUX reply detection logic sets this bit to a 1 while
//      receiving a reply on the AUX channel. The bit is 0 otherwise.
//
//  0 – REPLY_RECEIVED: This bit is set to 0 when the AUX request controller begins
//      sending bits on the AUX serial bus. The AUX reply controller sets this bit
//      to 1 when a complete and valid reply transaction has been received.
//      This bit is cleared when a request transaction has been initiated by the request controller.
//------------------------------------------------------------------------------
#define TR_DPTX_AUX_STATUS_REPLY_ERROR (1ul << 3)
#define TR_DPTX_AUX_STATUS_REQUEST_IN_PROGRESS (1ul << 2)
#define TR_DPTX_AUX_STATUS_REPLY_IN_PROGRESS (1ul << 1)
#define TR_DPTX_AUX_STATUS_REPLY_RECEIVED (1ul << 0)

#endif
