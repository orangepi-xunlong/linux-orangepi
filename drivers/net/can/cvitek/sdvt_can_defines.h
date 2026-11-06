#ifndef SDVT_CAN_DEFINES_H
#define SDVT_CAN_DEFINES_H
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
// Date Created   : 02-06-2023

// Generator      : IIP Compiler Version 1.1
//0---------------------------------------------------------------------------------------------------

//0###################################################################################################
// #State machine defines
//0###################################################################################################
#define SDVT_CAN_IDLE                                              0x0       //  State for IDLE
#define SDVT_CAN_SOF                                               0x1       //  State for SOF
#define SDVT_CAN_ARB_FIELD                                         0x2       //  State for ARB_FIELD
#define SDVT_CAN_CTRL_FIELD                                        0x3       //  State for CTRL_FIELD
#define SDVT_CAN_DATA_FIELD                                        0x4       //  State for DATA_FIELD
#define SDVT_CAN_CRC_FIELD                                         0x5       //  State for CRC_FIELD
#define SDVT_CAN_CRC_DELIM                                         0x6       //  State for CRC_DELIM
#define SDVT_CAN_ACK                                               0x7       //  State for ACK
#define SDVT_CAN_ACK_DELIM                                         0x8       //  State for ACK_DELIM
#define SDVT_CAN_EOF                                               0x9       //  State for EOF
#define SDVT_CAN_ERR_FLAG                                          0xA       //  State for ERR_FLAG
#define SDVT_CAN_ERR_DELIM                                         0xB       //  State for ERR_DELIM
#define SDVT_CAN_OVER_LD_FLAG                                      0xC       //  State for OVER_LD_FLAG
#define SDVT_CAN_OVER_LD_DELIM                                     0xD       //  State for OVER_LD_DELIM

//0###################################################################################################
// #Bit timing State machine defines
//0###################################################################################################
#define SDVT_CAN_TIM_IDLE                                          0x0       //  State for TIM_IDLE
#define SDVT_CAN_TIM_LOW                                           0x1       //  State for TIM_LOW
#define SDVT_CAN_TIM_HIGH                                          0x2       //  State for TIM_HIGH

//0###################################################################################################
// #MODE register fields
//0###################################################################################################
#define SDVT_CAN_RESET_MODE                                        0         // Reset mode
#define SDVT_CAN_RECEIVE_DONE                                      1         // Reception done
#define SDVT_CAN_TRANS_DONE                                        2         // Transmit done
#define SDVT_CAN_ERROR_WARN                                        3         // Error warning
#define SDVT_CAN_OVER_RUN                                          4         // Over run
#define SDVT_CAN_CAN_FD                                            7         // CAN FD enable

//0###################################################################################################
// #COMMAND register fields
//0###################################################################################################
#define SDVT_CAN_TRANSMISSION_REQ                                  0         // Transmission request
#define SDVT_CAN_ABORT_REQ                                         1         // Abort request
#define SDVT_CAN_RELEASE_RX_BUF                                    2         // Release receive buffer
#define SDVT_CAN_CLEAR_DATA_OVERRUN                                3         // Clear data over run
#define SDVT_CAN_SELF_RECEPTION                                    4         // Self reception
#define SDVT_CAN_OVERLOAD_REQ                                      5         // Overload reception

//0###################################################################################################
// #CAN status register fields
//0###################################################################################################
#define SDVT_CAN_RX_BUFFER                                         0         // Receive buffer
#define SDVT_CAN_DATA_OVERRUN                                      1         // Data over run
#define SDVT_CAN_TX_BUFFER                                         2         // Transmit buffer
#define SDVT_CAN_TRAN_COMPLETE                                     3         // Transmit complete
#define SDVT_CAN_CAN_RX_STATUS                                     4         // Receive status
#define SDVT_CAN_TX_STATUS                                         5         // Transmit status
#define SDVT_CAN_ERR_STATUS                                        6         // Transmit status
#define SDVT_CAN_BUS_STATUS                                        7         // Bus status

//0###################################################################################################
// #INTR status and enable register fields
//0###################################################################################################
#define SDVT_CAN_RECEPTION_DONE                                    0         // Reception done
#define SDVT_CAN_TRANSMIT_DONE                                     1         // Transmit done
#define SDVT_CAN_ERROR_WARN_INT                                    2         // Error warning
#define SDVT_CAN_DATA_OVERRUN_INT                                  3         // Data overrun
#define SDVT_CAN_REMOTE_FRAME                                      4         // Remote frame
#define SDVT_CAN_ERROR_PASSIVE                                     5         // Error passive
#define SDVT_CAN_ARB_LOST                                          6         // Arbitration lost
#define SDVT_CAN_BUS_ERR                                           7         // Bus erro

//0###################################################################################################
// #Bus timing 0 register fields
//0###################################################################################################
#define SDVT_CAN_BAUD_RATE_PRE                                     5:0       // Baud rate prescaler
#define SDVT_CAN_SJ_WIDTH                                          7:6       // Synchronization jump width

//0###################################################################################################
// #Bus timing 1 register fields
//0###################################################################################################
#define SDVT_CAN_TIME_SEG1                                         3:0       // Timer segment1
#define SDVT_CAN_TIME_SEG2                                         6:4       // Timer segment2
#define SDVT_CAN_SAMP_TYPE                                         7         // Sampling type

//0###################################################################################################
// #ARB lost capture register fields
//0###################################################################################################
#define SDVT_CAN_LOST_ARB                                          4:0       // Bit position of losing arbitration

//0###################################################################################################
// #Error code capture register fields
//0###################################################################################################
#define SDVT_CAN_ERR_SEGMENT                                       4:0       // Error segment
#define SDVT_CAN_DIRECTION                                         5         // Error direction
#define SDVT_CAN_ERROR_CODE                                        7:6       // Error code

//0###################################################################################################
// #Clock divider register fields
//0###################################################################################################
#define SDVT_CAN_CLK_DIV                                           2:0       // Clock divider
#define SDVT_CAN_CLK_OFF                                           3         // Clock off
#define SDVT_CAN_CAN_MODE                                          7         // CAN mode

//0###################################################################################################
// #Extended information register fields
//0###################################################################################################
#define SDVT_CAN_REMOTE_FRAME_IRQ                                  0         // Remote frame IRQ for basic mode
#define SDVT_CAN_REC_INCR_EN                                       1         // REC increment enable

//0###################################################################################################
// #DLC remote frame register fields
//0###################################################################################################
#define SDVT_CAN_AUTO_REMOTE                                       0         // Automatic remote mode
#define SDVT_CAN_DATA_LEN_CODE                                     4:1       // Data length code
#define SDVT_CAN_REMOTE_FRAME_VLD                                  5         // Remote frame valid

//0###################################################################################################
// #FD speed control register fields
//0###################################################################################################
#define SDVT_CAN_FD_SPEED_SWITCH                                   0         // Switch the speed
#define SDVT_CAN_FD_PRESCALER                                      7:1       // Prescaler value

//0###################################################################################################
// #Defines for HCI Command DWowrd
//0###################################################################################################
#define SDVT_CAN_CMD_DW0_OWN                                       0         // Descriptor ownership bit
#define SDVT_CAN_CMD_DW0_PRI                                       1         // Priority of command
#define SDVT_CAN_CMD_DW0_CMD                                       6:2       // Command to send
#define SDVT_CAN_CMD_DW0_SID                                       11:8      // Slave address or MID of command
#define SDVT_CAN_CMD_DW0_BC                                        15:12     // Byte count of command
#define SDVT_CAN_CMD_DW0_ADDR                                      31:16     // Register address of command

//0###################################################################################################
// #Defines for HCI status DWowrd
//0###################################################################################################
#define SDVT_CAN_STS_DW2_SUC                                       0         // Command status
#define SDVT_CAN_STS_DW2_SWE                                       1         // SOC write error
#define SDVT_CAN_STS_DW2_SRE                                       2         // SOC read error
#define SDVT_CAN_STS_DW2_NAK                                       3         // NACK error
#define SDVT_CAN_STS_DW2_ALR                                       4         // Arb retry abort limit reached
#define SDVT_CAN_STS_DW2_CRC                                       5         // CRC error
#define SDVT_CAN_STS_DW2_NRP                                       6         // No response error
#define SDVT_CAN_STS_DW2_FRE                                       7         // Form error
#define SDVT_CAN_STS_DW2_STE                                       8         // stuff error
#define SDVT_CAN_STS_DW2_BER                                       9         // Bit error
#define SDVT_CAN_STS_DW2_ODE                                       10        // Overload delimitter error

//0###################################################################################################
// #Defines for HCI read data status DWowrd
//0###################################################################################################
#define SDVT_CAN_STS_DW3_NRS                                       15:0      // No response status, each bit points to one byte
#define SDVT_CAN_STS_DW3_RPE                                       31:16     // Read parity error status, each bit points to one byte

//0###################################################################################################
// Defines for can
//0###################################################################################################
#define SDVT_CAN_CONTROL                                           0x0       // This register is control register
#define SDVT_CAN_EXT_CONTROL                                       0x1       // This register is extended control register
#define SDVT_CAN_FD_CONTROL                                        0x2       // The register is FD mode control register
#define SDVT_CAN_COMMAND                                           0x3       // This register is command register
#define SDVT_CAN_DLC_REMOTE_FRAME                                  0x4       // The register controls the DLC of remote frame response to send
#define SDVT_CAN_STATUS                                            0x5       // This register holds CAN status Register
#define SDVT_CAN_ACCEPTANCE_CODE0                                  0x6       // The holds the acceptance code 0 register
#define SDVT_CAN_ACCEPTANCE_CODE1                                  0x7       // The holds the acceptance code 1 register
#define SDVT_CAN_ACCEPTANCE_CODE2                                  0x8       // The holds the acceptance code 2 register
#define SDVT_CAN_ACCEPTANCE_CODE3                                  0x9       // The holds the acceptance code 3 register
#define SDVT_CAN_ACCEPTANCE_MASK0                                  0xA       // The holds the acceptance mask 0 register
#define SDVT_CAN_ACCEPTANCE_MASK1                                  0xB       // The holds the acceptance mask 1 register
#define SDVT_CAN_ACCEPTANCE_MASK2                                  0xC       // The holds the acceptance mask 2 register
#define SDVT_CAN_ACCEPTANCE_MASK3                                  0xD       // The holds the acceptance mask 3 register
#define SDVT_CAN_NOR_CLK_DIVIDER                                   0xE       // The register controls the normal mode clock divider
#define SDVT_CAN_NOR_BUS_TIMING0                                   0xF       // This register holds the normal bus timing 0
#define SDVT_CAN_NOR_BUS_TIMING1                                   0x10      // This register holds the normal bus timing 1
#define SDVT_CAN_FD_CLK_DIVIDER                                    0x11      // The register controls the FD mode clock divider
#define SDVT_CAN_FD_BUS_TIMING0                                    0x12      // This register holds the FD bus timing 0
#define SDVT_CAN_FD_BUS_TIMING1                                    0x13      // This register holds the FD bus timing 1
#define SDVT_CAN_ARB_ID0                                           0x14      // This register holds data frame Identifier bits 2:0 of ID, RTR, DLC for normal frame and frame format, RTR, DLC for extended frame
#define SDVT_CAN_ARB_ID1                                           0x15      // This register holds data frame Identifier bits 10:3 for normal frame and 28:21 for extended frame
#define SDVT_CAN_ARB_ID2                                           0x16      // This register holds data frame Identifier bits 20:13 for extended frame
#define SDVT_CAN_ARB_ID3                                           0x17      // This register holds data frame Identifier bits 12:5 for extended frame
#define SDVT_CAN_ARB_ID4                                           0x18      // This register holds data frame Identifier bits 4:0 for extended frame
#define SDVT_CAN_RSP_ARB_ID0                                       0x19      // This register holds remote frame response Identifier bits 10:3 for normal frame and 28:21 for extended frame
#define SDVT_CAN_RSP_ARB_ID1                                       0x1A      // This register holds remote frame response Identifier bits 2:0 for normal frame and 20:13 for extended frame
#define SDVT_CAN_RSP_ARB_ID2                                       0x1B      // This register holds remote frame response Identifier bits 12:5 for extended frame
#define SDVT_CAN_RSP_ARB_ID3                                       0x1C      // This register holds remote frame response Identifier bits 4:0 for extended frame
#define SDVT_CAN_TX_DATA_FIFO                                      0x1D      // This is transmit data FIFO, data in this is read by transmitter, written by Firmware
#define SDVT_CAN_RX_DATA_FIFO                                      0x1E      // This is receive data FIFO, data in this is written by receiver, read by Firmware
#define SDVT_CAN_RX_LEN_FIFO                                       0x1F      // This is receive length FIFO, data in this is written by receiver, read by Firmware
#define SDVT_CAN_TX_RSP_FIFO                                       0x20      // This is transmit response FIFO, data in this is read by transmitter, written by Firmware for remote frame request
#define SDVT_CAN_ARB_LOST_CAPTURE                                  0x21      // This register stores the bit at which aribitration was lost
#define SDVT_CAN_ERR_CODE_CAPTURE                                  0x22      // This register stores the error code of detected error
#define SDVT_CAN_ERR_WARN_LIMIT                                    0x23      // This register controls error limit
#define SDVT_CAN_RX_ERR_COUNTER                                    0x24      // This register holds receive error counter
#define SDVT_CAN_TX_ERR_COUNTER                                    0x25      // This register holds transmit error counter
#define SDVT_CAN_ERR_COUNTER_LOAD                                  0x26      // Backdoor load the transmit/receiver error counter
#define SDVT_CAN_ERR_DATA                                          0x27      // Backdoor error counter value to load
#define SDVT_CAN_FIFO_FLUSH                                        0x28      // Register to flush FIFO's
#define SDVT_CAN_TX_DATA_FIFO_THRESHOLD                            0x29      // Register to control threshold for interrupt for Transmit data FIFO
#define SDVT_CAN_TX_RSP_FIFO_THRESHOLD                             0x2A      // Register to control threshold for interrupt for Transmit response FIFO
#define SDVT_CAN_RX_DATA_FIFO_THRESHOLD                            0x2B      // Register to control threshold for interrupt for Receive data FIFO
#define SDVT_CAN_RX_LEN_FIFO_THRESHOLD                             0x2C      // Register to control threshold for interrupt for Receive length FIFO
#define SDVT_CAN_FIFO_STATUS                                       0x2D      // Register for FIFO status
#define SDVT_CAN_TX_DATA_FIFO_DEPTH                                0x2E      // Register for transmit data FIFO depth
#define SDVT_CAN_TX_DATA_FIFO_AVAIL                                0x2F      // Register for transmit data FIFO free space
#define SDVT_CAN_TX_RSP_FIFO_DEPTH                                 0x30      // Register for transmit respone FIFO depth
#define SDVT_CAN_TX_RSP_FIFO_AVAIL                                 0x31      // Register for transmit respone FIFO free space
#define SDVT_CAN_RX_DATA_FIFO_DEPTH                                0x32      // Register for Receive data FIFO depth
#define SDVT_CAN_RX_DATA_FIFO_AVAIL                                0x33      // Register for Receive data FIFO free space
#define SDVT_CAN_RX_LEN_FIFO_DEPTH                                 0x34      // Register for Receive length FIFO depth
#define SDVT_CAN_RX_LEN_FIFO_AVAIL                                 0x35      // Register for Receive length FIFO free space
#define SDVT_CAN_IRQ_ENABLE0                                       0x36      // Interrupt enable register 0
#define SDVT_CAN_IRQ_STATUS0                                       0x37      // Interrupt status register 0
#define SDVT_CAN_IRQ_ENABLE1                                       0x38      // Interrupt enable register 1 for transmit and receive frames
#define SDVT_CAN_IRQ_STATUS1                                       0x39      // Interrupt status register 1
#define SDVT_CAN_IRQ_ENABLE2                                       0x3A      // Interrupt enable register 2 for transmit data FIFO's
#define SDVT_CAN_IRQ_STATUS2                                       0x3B      // Interrupt status register 2
#define SDVT_CAN_IRQ_ENABLE3                                       0x3C      // Interrupt enable register 3 for transmit response FIFO's
#define SDVT_CAN_IRQ_STATUS3                                       0x3D      // Interrupt status register 3
#define SDVT_CAN_IRQ_ENABLE4                                       0x3E      // Interrupt enable register 4 for receive data FIFO's
#define SDVT_CAN_IRQ_STATUS4                                       0x3F      // Interrupt status register 4
#define SDVT_CAN_IRQ_ENABLE5                                       0x40      // Interrupt enable register 5 for receive length FIFO's
#define SDVT_CAN_IRQ_STATUS5                                       0x41      // Interrupt status register 5
#define SDVT_CAN_SOC_TIMEOUT                                       0x42      // SOC timeout register

//0###################################################################################################
// Defines for subfields of CSR CONTROL
//0###################################################################################################
#define SDVT_CAN_CTRL_RESET                                        0         // Reset request
#define SDVT_CAN_CTRL_CLK_OFF                                      1         // Clock off control
#define SDVT_CAN_CTRL_FRAME_MODE                                   2         // Controls the frame mode
#define SDVT_CAN_CTRL_MODE                                         4:3       // Normal CAN or CAN FD Mode

//0###################################################################################################
// Defines for subfields of CSR EXT_CONTROL
//0###################################################################################################
#define SDVT_CAN_EXT_CTRL_LISTEN                                   0         // Listen only mode
#define SDVT_CAN_EXT_CTRL_SELF_TEST                                1         // Self test mode
#define SDVT_CAN_EXT_CTRL_ACC_FILTER                               2         // Acceptance filter mode
#define SDVT_CAN_EXT_CTRL_REC_INCR                                 3         // Increment receiver error counter
#define SDVT_CAN_EXT_CTRL_REMOTE_EN                                4         // Enable receiving and processing remote frames, this should be set to 0 for CAN FD mode
#define SDVT_CAN_EXT_ID_RX                                         5         // Extended Identifier in Receiver side

//0###################################################################################################
// Defines for subfields of CSR FD_CONTROL
//0###################################################################################################
#define SDVT_CAN_FD_BRS                                            0         // BRS mode enable or disable, 0 means disabled, 1 means enabled

//0###################################################################################################
// Defines for subfields of CSR COMMAND
//0###################################################################################################
#define SDVT_CAN_CMD_REQ                                           0         // Transmission Request
#define SDVT_CAN_CMD_EXT_ID                                        1         // Send extended frame
#define SDVT_CAN_CMD_REMOTE_RDY                                    2         // Remote frame data is set by FW, and Hardware can transmit response
#define SDVT_CAN_CMD_ABORT                                         3         // Abort Transmission
#define SDVT_CAN_CMD_REL_RX_BUF                                    4         // Release Receive Buffer
#define SDVT_CAN_CMD_CLEAR_OVERRUN                                 5         // Clear Data Overrun
#define SDVT_CAN_CMD_REQ_OVERLOAD                                  6         // Request overload frame
#define SDVT_CAN_CMD_REQ_SLEEP                                     7         // Go To Sleep

//0###################################################################################################
// Defines for subfields of CSR DLC_REMOTE_FRAME
//0###################################################################################################
#define SDVT_CAN_REMOTE_DLC                                        3:0       // DLC value of remote frame response
#define SDVT_CAN_REMOTE_EXT_ID                                     4         // Use extended ID for remote frame response
#define SDVT_CAN_REMOTE_RSP_VALID                                  5         // Remote frame response is ready

//0###################################################################################################
// Defines for subfields of CSR STATUS
//0###################################################################################################
#define SDVT_CAN_STATUS_RX_BUF                                     0         // CAN status to indicate receive buffer status
#define SDVT_CAN_STATUS_OVERRUN                                    1         // CAN status to indicate Data Overrun Status
#define SDVT_CAN_STATUS_TX_BUF                                     2         // CAN status to indicate transmit buffer status
#define SDVT_CAN_STATUS_TX_DONE                                    3         // CAN status to indicate transmission complete status
#define SDVT_CAN_STATUS_RX                                         4         // CAN status to indicate receive status
#define SDVT_CAN_STATUS_TX                                         5         // CAN status to indicate transmit status
#define SDVT_CAN_STATUS_ERR                                        6         // CAN status to indicate error status
#define SDVT_CAN_STATUS_BUS_OFF                                    7         // CAN status to indicate bus off

//0###################################################################################################
// Defines for subfields of CSR NOR_BUS_TIMING0
//0###################################################################################################
#define SDVT_CAN_BAUD_PRESCALER                                    5:0       // The field controls the baud rate prescaler (2*(value + 1))
#define SDVT_CAN_SYNC_JUMP_WIDTH                                   7:6       // The field controls the Synchronization Jump Width  (value + 1)

//0###################################################################################################
// Defines for subfields of CSR NOR_BUS_TIMING1
//0###################################################################################################
#define SDVT_CAN_TIME_SEGMENT1                                     3:0       // The field controls the timing segement 1 (TSEG1) (value + 1)
#define SDVT_CAN_TIME_SEGMENT2                                     6:4       // The field controls the timing segement 2 (TSEG2) (value + 1)
#define SDVT_CAN_TIME_TRIPLE_SAMPLE                                7         // The field controls the timing triple sample

//0###################################################################################################
// Defines for subfields of CSR ERR_CODE_CAPTURE
//0###################################################################################################
#define SDVT_CAN_ERR_CODE_SEG                                      4:0       // Error capture code segment(Segment0,1,2,3,4)
#define SDVT_CAN_ERR_CODE_DIR                                      5         // Error capture code direction
#define SDVT_CAN_ERR_CODE_TYPE                                     7:6       // Error capture code type

//0###################################################################################################
// Defines for subfields of CSR ERR_COUNTER_LOAD
//0###################################################################################################
#define SDVT_CAN_TX_LOAD_ERR                                       0         // Load transmitter error counter
#define SDVT_CAN_RX_LOAD_ERR                                       1         // Load receiver error counter

//0###################################################################################################
// Defines for subfields of CSR FIFO_FLUSH
//0###################################################################################################
#define SDVT_CAN_TX_DATA_FIFO_FLUSH                                0         // Transmit Data FIFO flush
#define SDVT_CAN_RX_DATA_FIFO_FLUSH                                1         // Receive Data FIFO flush
#define SDVT_CAN_RX_LEN_FIFO_FLUSH                                 2         // Receive Length FIFO flush
#define SDVT_CAN_TX_RSP_FIFO_FLUSH                                 3         // Transmit response FIFO flush

//0###################################################################################################
// Defines for subfields of CSR FIFO_STATUS
//0###################################################################################################
#define SDVT_CAN_TX_DFIFO_FULL                                     0         // Transmit Data FIFO full
#define SDVT_CAN_TX_DFIFO_EMPTY                                    1         // Transmit Data FIFO empty
#define SDVT_CAN_RX_DFIFO_FULL                                     2         // Receive Data FIFO full
#define SDVT_CAN_RX_DFIFO_EMPTY                                    3         // Receive Data FIFO empty
#define SDVT_CAN_RX_LFIFO_FULL                                     4         // Receive Length FIFO full
#define SDVT_CAN_RX_LFIFO_EMPTY                                    5         // Receive Length FIFO empty
#define SDVT_CAN_TX_RFIFO_FULL                                     6         // Transmit Response FIFO full
#define SDVT_CAN_TX_RFIFO_EMPTY                                    7         // Transmit Response FIFO empty

//0###################################################################################################
// Defines for subfields of CSR IRQ_ENABLE0
//0###################################################################################################
#define SDVT_CAN_IRQ_INFO_EMPTY                                    0         // Irq info empty register
#define SDVT_CAN_IRQ_TRANSMIT_BUFFER_STATUS                        1         // Irq transmit buffer status register
#define SDVT_CAN_IRQ_ERROR_STATUS                                  2         // Irq error status register
#define SDVT_CAN_IRQ_DATA_OVER_RUN                                 3         // Irq data over run register
#define SDVT_CAN_IRQ_REMOTE_FRAME                                  4         // Irq remote frame register
#define SDVT_CAN_IRQ_NODE_ERROR_PASSIVE                            5         // Irq node error passive register
#define SDVT_CAN_IRQ_ARBITRATION_LOST                              6         // Irq arbitration last register
#define SDVT_CAN_IRQ_BUS_ERR                                       7         // Irq Bus error register

//0###################################################################################################
// Defines for subfields of CSR IRQ_ENABLE1
//0###################################################################################################
#define SDVT_CAN_IRQ_TX_DONE                                       0         // Transmitter is done transmitting
#define SDVT_CAN_IRQ_RX_DATA_FRAME                                 1         // Receiver has recieved data frame which is not remote frame

//0###################################################################################################
// Defines for subfields of CSR IRQ_ENABLE2
//0###################################################################################################
#define SDVT_CAN_IRQ_TX_DATA_FIFO_TT                               0         // Transmit Data FIFO threshold reached on read
#define SDVT_CAN_IRQ_TX_DATA_FIFO_UR                               1         // Transmit Data FIFO underrun
#define SDVT_CAN_IRQ_TX_DATA_FIFO_OR                               2         // Transmit Data FIFO overrun
#define SDVT_CAN_IRQ_TX_DATA_FIFO_FULL                             3         // Transmit Data FIFO full
#define SDVT_CAN_IRQ_TX_DATA_FIFO_EMPTY                            4         // Transmit Data FIFO empty

//0###################################################################################################
// Defines for subfields of CSR IRQ_ENABLE3
//0###################################################################################################
#define SDVT_CAN_IRQ_TX_RSP_FIFO_TT                                0         // Transmit Response FIFO threshold reached on read
#define SDVT_CAN_IRQ_TX_RSP_FIFO_UR                                1         // Transmit Response FIFO underrun
#define SDVT_CAN_IRQ_TX_RSP_FIFO_OR                                2         // Transmit Response FIFO overrun
#define SDVT_CAN_IRQ_TX_RSP_FIFO_FULL                              3         // Transmit Response FIFO full
#define SDVT_CAN_IRQ_TX_RSP_FIFO_EMPTY                             4         // Transmit Response FIFO empty

//0###################################################################################################
// Defines for subfields of CSR IRQ_ENABLE4
//0###################################################################################################
#define SDVT_CAN_IRQ_RX_DATA_FIFO_TT                               0         // Receive Data FIFO threshold reached on write
#define SDVT_CAN_IRQ_RX_DATA_FIFO_UR                               1         // Receive Data FIFO underrun
#define SDVT_CAN_IRQ_RX_DATA_FIFO_OR                               2         // Receive Data FIFO overrun
#define SDVT_CAN_IRQ_RX_DATA_FIFO_FULL                             3         // Receive Data FIFO full
#define SDVT_CAN_IRQ_RX_DATA_FIFO_EMPTY                            4         // Receive Data FIFO empty

//0###################################################################################################
// Defines for subfields of CSR IRQ_ENABLE5
//0###################################################################################################
#define SDVT_CAN_IRQ_RX_LEN_FIFO_TT                                0         // Receive Length FIFO threshold reached on write
#define SDVT_CAN_IRQ_RX_LEN_FIFO_UR                                1         // Receive Length FIFO underrun
#define SDVT_CAN_IRQ_RX_LEN_FIFO_OR                                2         // Receive Length FIFO overrun
#define SDVT_CAN_IRQ_RX_LEN_FIFO_FULL                              3         // Receive Length FIFO full
#define SDVT_CAN_IRQ_RX_LEN_FIFO_EMPTY                             4         // Receive Length FIFO empty

//0###################################################################################################
// Defines for enum for csr CONTROL field CTRL_FRAME_MODE of can
//0###################################################################################################
#define SDVT_CAN_STANDARD_FRAME                                    0         // CAN in standard frame mode
#define SDVT_CAN_EXTENDED_FRAME                                    1         // CAN in extended frame mode

//0###################################################################################################
// Defines for enum for csr CONTROL field CTRL_MODE of can
//0###################################################################################################
#define SDVT_CAN_CLASSIC_MODE                                      0         // CAN in Classic mode
#define SDVT_CAN_FD_MODE                                           1         // CAN in FD mode
#define SDVT_CAN_AUTO_MODE                                         2         // CAN in Auto mode, Transmitter and receiver in Normal Mode,  will detect CAN FD frames in receive path automatically and switch to FD mode. Same way, will detect normal mode and switch to normal mode

//0###################################################################################################
// Defines for enum for csr EXT_CONTROL field EXT_CTRL_ACC_FILTER of can
//0###################################################################################################
#define SDVT_CAN_DUAL_FILTER                                       0         // Acceptance filter mode is dual filter mode
#define SDVT_CAN_SINGLE_FILTER                                     1         // Acceptance filter mode is single filter mode

//0###################################################################################################
// Defines for enum for csr ERR_CODE_CAPTURE field ERR_CODE_SEG of can
//0###################################################################################################
#define SDVT_CAN_SOF_CODE                                          0x03      // Error capture at SOF
#define SDVT_CAN_ID28_ID21_CODE                                    0x02      // Error capture at ID.28 to ID.21
#define SDVT_CAN_ID20_ID18_CODE                                    0x06      // Error capture at ID.20 to ID.18
#define SDVT_CAN_BIT_SRTR                                          0x04      // Error capture at Bit SRTR
#define SDVT_CAN_BIT_IDE                                           0x05      // Error capture at Bit IDE
#define SDVT_CAN_ID17_ID13_CODE                                    0x07      // Error capture at ID.17 to ID.13
#define SDVT_CAN_ID12_ID5_CODE                                     0x0F      // Error capture at ID.12 to ID.5
#define SDVT_CAN_ID4_ID0_CODE                                      0x0E      // Error capture at ID.4 to ID.0
#define SDVT_CAN_BIT_RTR                                           0x0C      // Error capture at Bit RTR
#define SDVT_CAN_RESER_BIT1                                        0x0D      // Error capture at Reserved bit 1
#define SDVT_CAN_RESER_BIT0                                        0x09      // Error capture at Reserved bit 0
#define SDVT_CAN_DATA_LENGTH_CODE                                  0x0B      // Error capture at data length code
#define SDVT_CAN_DATA_FIELDS                                       0x0A      // Error capture at data field
#define SDVT_CAN_CRC_SEQ                                           0x08      // Error capture at CRC sequence
#define SDVT_CAN_CRC_DEL                                           0x18      // Error capture at CRC delimiter
#define SDVT_CAN_ACK_SLOT                                          0x19      // Error capture at Acknowledge slot
#define SDVT_CAN_ACK_DEL                                           0x1B      // Error capture at Acknowledge delimiter
#define SDVT_CAN_EOF_FRAME                                         0x1A      // Error capture at End of frame
#define SDVT_CAN_INTERMISSION                                      0x12      // Error capture at Intermission
#define SDVT_CAN_ACT_ERR_FLAG                                      0x11      // Error capture at Active error flag
#define SDVT_CAN_PASS_ERR_FLAG                                     0x16      // Error capture at Passive error flag
#define SDVT_CAN_TOLE_DOMI_BIT                                     0x13      // Error capture at Tolerate dominant bits
#define SDVT_CAN_ERR_DELI                                          0x17      // Error capture at Error delimiter
#define SDVT_CAN_OVERLOAD_FLAG                                     0x1C      // Error capture at Overload flag

//0###################################################################################################
// Defines for enum for csr ERR_CODE_CAPTURE field ERR_CODE_DIR of can
//0###################################################################################################
#define SDVT_CAN_TX_DIR                                            0         // Transmit direction
#define SDVT_CAN_RX_DIR                                            1         // Receive direction

//0###################################################################################################
// Defines for enum for csr ERR_CODE_CAPTURE field ERR_CODE_TYPE of can
//0###################################################################################################
#define SDVT_CAN_BIT_ERR                                           0         // Bit error
#define SDVT_CAN_FORM_ERR                                          1         // Form error
#define SDVT_CAN_STUFF_ERR                                         2         // Stuff error
#define SDVT_CAN_OTHER_ERR                                         3         // Other error


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
#endif
