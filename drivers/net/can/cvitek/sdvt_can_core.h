#ifndef SDVT_CAN_CORE_H
#define SDVT_CAN_CORE_H
//0---------------------------------------------------------------------------------------------------
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
//0---------------------------------------------------------------------------------------------------
// Defines for all CSR in IIP
//0-------------------------------------------------------------------------------
#define CONTROL 0x0       // This register is control register
#define EXT_CONTROL 0x1       // This register is extended control register
#define FD_CONTROL 0x2       // The register is FD mode control register
#define COMMAND 0x3       // This register is command register
#define DLC_REMOTE_FRAME 0x4       // The register controls the DLC of remote frame response to send
#define STATUS 0x5       // This register holds CAN status Register  
#define ACCEPTANCE_CODE0 0x6       // The holds the acceptance code 0 register
#define ACCEPTANCE_CODE1 0x7       // The holds the acceptance code 1 register
#define ACCEPTANCE_CODE2 0x8       // The holds the acceptance code 2 register
#define ACCEPTANCE_CODE3 0x9       // The holds the acceptance code 3 register
#define ACCEPTANCE_MASK0 0xA       // The holds the acceptance mask 0 register
#define ACCEPTANCE_MASK1 0xB       // The holds the acceptance mask 1 register
#define ACCEPTANCE_MASK2 0xC       // The holds the acceptance mask 2 register
#define ACCEPTANCE_MASK3 0xD       // The holds the acceptance mask 3 register
#define NOR_CLK_DIVIDER 0xE       // The register controls the normal mode clock divider
#define NOR_BUS_TIMING0 0xF       // This register holds the normal bus timing 0
#define NOR_BUS_TIMING1 0x10      // This register holds the normal bus timing 1
#define FD_CLK_DIVIDER 0x11      // The register controls the FD mode clock divider
#define FD_BUS_TIMING0 0x12      // This register holds the FD bus timing 0
#define FD_BUS_TIMING1 0x13      // This register holds the FD bus timing 1
#define ARB_ID0 0x14      // This register holds data frame Identifier bits 2:0 of ID, RTR, DLC for normal frame and frame format, RTR, DLC for extended frame
#define ARB_ID1 0x15      // This register holds data frame Identifier bits 10:3 for normal frame and 28:21 for extended frame
#define ARB_ID2 0x16      // This register holds data frame Identifier bits 20:13 for extended frame
#define ARB_ID3 0x17      // This register holds data frame Identifier bits 12:5 for extended frame
#define ARB_ID4 0x18      // This register holds data frame Identifier bits 4:0 for extended frame
#define RSP_ARB_ID0 0x19      // This register holds remote frame response Identifier bits 10:3 for normal frame and 28:21 for extended frame
#define RSP_ARB_ID1 0x1A      // This register holds remote frame response Identifier bits 2:0 for normal frame and 20:13 for extended frame
#define RSP_ARB_ID2 0x1B      // This register holds remote frame response Identifier bits 12:5 for extended frame
#define RSP_ARB_ID3 0x1C      // This register holds remote frame response Identifier bits 4:0 for extended frame
#define TX_DATA_FIFO 0x1D      // This is transmit data FIFO, data in this is read by transmitter, written by Firmware
#define RX_DATA_FIFO 0x1E      // This is receive data FIFO, data in this is written by receiver, read by Firmware
#define RX_LEN_FIFO 0x1F      // This is receive length FIFO, data in this is written by receiver, read by Firmware
#define TX_RSP_FIFO 0x20      // This is transmit response FIFO, data in this is read by transmitter, written by Firmware for remote frame request
#define ARB_LOST_CAPTURE 0x21      // This register stores the bit at which aribitration was lost
#define ERR_CODE_CAPTURE 0x22      // This register stores the error code of detected error
#define ERR_WARN_LIMIT 0x23      // This register controls error limit
#define RX_ERR_COUNTER 0x24      // This register holds receive error counter
#define TX_ERR_COUNTER 0x25      // This register holds transmit error counter
#define ERR_COUNTER_LOAD 0x26      // Backdoor load the transmit/receiver error counter
#define ERR_DATA 0x27      // Backdoor error counter value to load
#define FIFO_FLUSH 0x28      // Register to flush FIFO's
#define TX_DATA_FIFO_THRESHOLD 0x29      // Register to control threshold for interrupt for Transmit data FIFO
#define TX_RSP_FIFO_THRESHOLD 0x2A      // Register to control threshold for interrupt for Transmit response FIFO
#define RX_DATA_FIFO_THRESHOLD 0x2B      // Register to control threshold for interrupt for Receive data FIFO
#define RX_LEN_FIFO_THRESHOLD 0x2C      // Register to control threshold for interrupt for Receive length FIFO
#define FIFO_STATUS 0x2D      // Register for FIFO status
#define TX_DATA_FIFO_DEPTH 0x2E      // Register for transmit data FIFO depth
#define TX_DATA_FIFO_AVAIL 0x2F      // Register for transmit data FIFO free space
#define TX_RSP_FIFO_DEPTH 0x30      // Register for transmit respone FIFO depth
#define TX_RSP_FIFO_AVAIL 0x31      // Register for transmit respone FIFO free space
#define RX_DATA_FIFO_DEPTH 0x32      // Register for Receive data FIFO depth
#define RX_DATA_FIFO_AVAIL 0x33      // Register for Receive data FIFO free space
#define RX_LEN_FIFO_DEPTH 0x34      // Register for Receive length FIFO depth
#define RX_LEN_FIFO_AVAIL 0x35      // Register for Receive length FIFO free space
#define IRQ_ENABLE0 0x36      // Interrupt enable register 0
#define IRQ_STATUS0 0x37      // Interrupt status register 0
#define IRQ_ENABLE1 0x38      // Interrupt enable register 1 for transmit and receive frames
#define IRQ_STATUS1 0x39      // Interrupt status register 1
#define IRQ_ENABLE2 0x3A      // Interrupt enable register 2 for transmit data FIFO's
#define IRQ_STATUS2 0x3B      // Interrupt status register 2
#define IRQ_ENABLE3 0x3C      // Interrupt enable register 3 for transmit response FIFO's
#define IRQ_STATUS3 0x3D      // Interrupt status register 3
#define IRQ_ENABLE4 0x3E      // Interrupt enable register 4 for receive data FIFO's
#define IRQ_STATUS4 0x3F      // Interrupt status register 4
#define IRQ_ENABLE5 0x40      // Interrupt enable register 5 for receive length FIFO's
#define IRQ_STATUS5 0x41      // Interrupt status register 5
#define SOC_TIMEOUT 0x42      // SOC timeout register



#endif
