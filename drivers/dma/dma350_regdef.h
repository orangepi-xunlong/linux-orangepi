// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0
/*
 * Copyright (c) 2022 Arm Limited. All rights reserved.
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */
#ifndef __DMA350_REGDEF_H
#define __DMA350_REGDEF_H

/*DMA350 register define*/
/*DMACH<n> register summary*/

#define DMA350_REG_CMD 0x000 /* Channel DMA Command Register */
#define DMA350_REG_STATUS 0x004 /* Channel Status Register */
#define DMA350_REG_INTREN 0x008 /*Channel Interrupt Enable Register */
#define DMA350_REG_CTRL 0x00C /*Channel Control Register */
#define DMA350_REG_SRCADDR 0x010 /*Channel Source Address Register */
#define DMA350_REG_DESADDR 0x018 /*Channel Destination Address Register */
#define DMA350_REG_XSIZE 0x020 /*Channel X Dimension Size Register, Lower Bits [15:0] */
#define DMA350_REG_XSIZEHI 0x024 /*Channel X Dimension Size Register, Lower Bits [31:16] */
#define DMA350_REG_SRCTRANSCFG 0x028 /*Channel Source Transfer Configuration Register */
#define DMA350_REG_DESTRANSCFG 	0x02C /*Channel Destination Transfer Configuration Register */
#define DMA350_REG_XADDRINC 0x030 /*Channel X Dimension Address Increment Register */
#define DMA350_REG_YADDRSTRIDE 0x034 /*Channel Y Dimension Address Stride Register */
#define DMA350_REG_FILLVAL 0x038 /*Channel Fill Pattern Value Register */
#define DMA350_REG_YSIZE 0x03C /*Channel Y Dimensions Size Register */
#define DMA350_REG_TMPLTCFG 0x040 /*Channel Template Configuration Register */
#define DMA350_REG_SRCTMPLT 0x044 /*Channel Source Template Pattern Register */
#define DMA350_REG_DESTMPLT 0x048 /*Channel Destination Template Pattern Register */
#define DMA350_REG_SRCTRIGINCFG 0x04C /*Channel Source Trigger In Configuration Register */
#define DMA350_REG_DESTRIGINCFG 0x050 /*Channel Destination Trigger In Configuration Register */
#define DMA350_REG_LINKATTR 0x070 /* Channel Link Address Memory Attributes Register */
#define DMA350_REG_AUTOCFG  0x074 /*Channel Automatic Command Restart Configuration Register */
#define DMA350_REG_LINKADDR 0x078 /* Channel Link Address Register */
#define DMA350_REG_LINKADDRHI 0x07C /* Channel Link Address Register, High Bits [63:32] */
#define DMA350_REG_GPOREAD0 0x080 /* Channel GPO Read Value Register 0 */
#define DMA350_REG_WRKREGPTR 0x088 /* Channel Working Register Pointer Register */
#define DMA350_REG_WRKREGVAL 0x08C /* Channel - Working Register Value Register */
#define DMA350_REG_ERRINFO 0x090 /* Channel Error Information Register */
#define DMA350_REG_IIDR 0x0C8 /* Channel Implementation Identification Register */
#define DMA350_REG_AIDR 0x0CC /* Channel Architecture Identification Register */
#define DMA350_REG_ISSUECAP 0x0E8 /* Used for setting issuing capability threshold. */

#define DMA350_REG_CHINTRSTATUS0 0x200 /*Collated Non-Secure Channel Interrupt flags for channel 0 to channel 31*/
#define DMA350_NONSSEC_CTRL  0xC

/******************************************************************************/
/*                       Field Definitions of Registers                       */
/******************************************************************************/
/******************************************************************************/
/*                                   DMACH                                    */
/******************************************************************************/
/******************  Field definitions for CH_CMD register  *******************/
#define DMA350_CH_CMD_ENABLECMD_Pos                    (0U)
#define DMA350_CH_CMD_ENABLECMD_Msk     (0x1UL << DMA350_CH_CMD_ENABLECMD_Pos)
#define CMD_ENABLECMD_Msk (1)
#define DMA350_CH_CMD_ENABLECMD                      (CMD_ENABLECMD_Msk)                                  /*!< ENABLECMD bit Channel Enable. When set to '1', enables the channel to run its programmed task. When set to '1', it cannot be set back to zero, and this field will automatically clear to zero when a DMA process is completed. To force the DMA to stop prematurely, you must use CH_CMD.STOPCMD instead.*/

#define DMA350_CH_CMD_CLEARCMD_Pos                     (1U)
#define DMA350_CH_CMD_CLEARCMD_Msk                     (0x1UL << DMA350_CH_CMD_CLEARCMD_Pos)                        /*!< 0x00000002UL*/
#define DMA350_CH_CMD_CLEARCMD                         DMA350_CH_CMD_CLEARCMD_Msk                                   /*!< CLEARCMD bit DMA Clear command. When set to '1', it will remain high until all DMA channel registers and any internal queues and buffers are cleared, before returning to '0'. When set at the same time as ENABLECMD or while the DMA channel is already enabled, the clear will only occur after any ongoing DMA operation is either completed, stopped or disabled and the ENABLECMD bit is deasserted by the DMA.*/
#define DMA350_CH_CMD_DISABLECMD_Pos                   (2U)
#define DMA350_CH_CMD_DISABLECMD_Msk                   (0x1UL << DMA350_CH_CMD_DISABLECMD_Pos)                      /*!< 0x00000004UL*/
#define DMA350_CH_CMD_DISABLECMD                       DMA350_CH_CMD_DISABLECMD_Msk                                 /*!< DISABLECMD bit Disable DMA Operation at the end of the current DMA command operation. Once set to '1', this field will stay high and the current DMA command will be allowed to complete, but the DMA will not fetch the next linked command or will it auto-restart the DMA command even if they are set. Once the DMA has stopped, it will return to '0' and ENABLECMD is also cleared. When set at the same time as ENABLECMD or when the channel is not enabled then write to this register is ignored.*/
#define DMA350_CH_CMD_STOPCMD_Pos                      (3U)
#define DMA350_CH_CMD_STOPCMD_Msk                      (0x1UL << DMA350_CH_CMD_STOPCMD_Pos)                         /*!< 0x00000008UL*/
#define DMA350_CH_CMD_STOPCMD                          DMA350_CH_CMD_STOPCMD_Msk                                    /*!< STOPCMD bit Stop Current DMA Operation. Once set to '1', his will remain high until the DMA channel is stopped cleanly. Then this will return to '0' and ENABLECMD is also cleared. When set at the same time as ENABLECMD or when the channel is not enabled then write to this register is ignored. Note that each DMA channel can have other sources of a stop request and this field will not reflect the state of the other sources.*/
#define DMA350_CH_CMD_PAUSECMD_Pos                     (4U)
#define DMA350_CH_CMD_PAUSECMD_Msk                     (0x1UL << DMA350_CH_CMD_PAUSECMD_Pos)                        /*!< 0x00000010UL*/
#define DMA350_CH_CMD_PAUSECMD                         DMA350_CH_CMD_PAUSECMD_Msk                                   /*!< PAUSECMD bit Pause Current DMA Operation. Once set to '1' the status cannot change until the DMA operation reached the paused state indicated by the STAT_PAUSED and STAT_RESUMEWAIT bits. The bit can be set by SW by writing it to '1', the current active DMA operation will be paused as soon as possible, but the ENABLECMD bit will remain HIGH to show that the operation is still active.  Cleared automatically when STAT_RESUMEWAIT is set and the RESUMECMD bit is written to '1', meaning that the SW continues the operation of the channel. Note that each DMA channel can have other sources of a pause request and this field will not reflect the state of the other sources. When set at the same time as ENABLECMD or when the channel is not enabled then write to this register is ignored.*/
#define DMA350_CH_CMD_RESUMECMD_Pos                    (5U)
#define DMA350_CH_CMD_RESUMECMD_Msk                    (0x1UL << DMA350_CH_CMD_RESUMECMD_Pos)                       /*!< 0x00000020UL*/
#define DMA350_CH_CMD_RESUMECMD                        DMA350_CH_CMD_RESUMECMD_Msk                                  /*!< RESUMECMD bit Resume Current DMA Operation. Writing this bit to '1' means that the DMAC can continue the operation of a paused channel. Can be set to '1' when the PAUSECMD or a STAT_DONE assertion with DONEPAUSEEN set HIGH results in pausing the current DMA channel operation indicated by the STAT_PAUSED and STAT_RESUMEWAIT bits. Otherwise, writes to this bit are ignored.*/
#define DMA350_CH_CMD_SRCSWTRIGINREQ_Pos               (16U)
#define DMA350_CH_CMD_SRCSWTRIGINREQ_Msk               (0x1UL << DMA350_CH_CMD_SRCSWTRIGINREQ_Pos)                  /*!< 0x00010000UL*/
#define DMA350_CH_CMD_SRCSWTRIGINREQ                   DMA350_CH_CMD_SRCSWTRIGINREQ_Msk                             /*!< SRCSWTRIGINREQ bit Software Generated Source Trigger Input Request. Write to '1' to create a SW trigger request to the DMA with the specified type in the SRCSWTRIGINTYPE register. Once set to '1', this will remain high until the DMA accepted the trigger and will return this to '0'. It will also be cleared automatically if the current command is completed without expecting another trigger event. When the channel is not enabled, write to this register is ignored.*/
#define DMA350_CH_CMD_SRCSWTRIGINTYPE_Pos              (17U)
#define DMA350_CH_CMD_SRCSWTRIGINTYPE_Msk              (0x3UL << DMA350_CH_CMD_SRCSWTRIGINTYPE_Pos)                 /*!< 0x00060000UL*/
#define DMA350_CH_CMD_SRCSWTRIGINTYPE                  DMA350_CH_CMD_SRCSWTRIGINTYPE_Msk                            /*!< SRCSWTRIGINTYPE[ 1:0] bits Software Generated Source Trigger Input Request Type. Selects the trigger request type for the source trigger input when the SW triggers the SRCSWTRIGINREQ bit.
 - 00: Single request
 - 01: Last single request
 - 10: Block request
 - 11: Last block request
This field cannot be changed while the SRCSWTRIGINREQ bit is set.*/
#define DMA350_CH_CMD_SRCSWTRIGINTYPE_0                (0x1UL << DMA350_CH_CMD_SRCSWTRIGINTYPE_Pos)                 /*!< 0x00020000UL*/
#define DMA350_CH_CMD_SRCSWTRIGINTYPE_1                (0x2UL << DMA350_CH_CMD_SRCSWTRIGINTYPE_Pos)                 /*!< 0x00040000UL*/
#define DMA350_CH_CMD_DESSWTRIGINREQ_Pos               (20U)
#define DMA350_CH_CMD_DESSWTRIGINREQ_Msk               (0x1UL << DMA350_CH_CMD_DESSWTRIGINREQ_Pos)                  /*!< 0x00100000UL*/
#define DMA350_CH_CMD_DESSWTRIGINREQ                   DMA350_CH_CMD_DESSWTRIGINREQ_Msk                             /*!< DESSWTRIGINREQ bit Software Generated Destination Trigger Input Request. Write to '1' to create a SW trigger request to the DMA with the specified type in the DESSWTRIGINTYPE register. Once set to '1', this will remain high until the DMA is accepted the trigger and will return this to '0'. It will also be cleared automatically if the current command is completed without expecting another trigger event. When the channel is not enabled, write to this register is ignored.*/
#define DMA350_CH_CMD_DESSWTRIGINTYPE_Pos              (21U)
#define DMA350_CH_CMD_DESSWTRIGINTYPE_Msk              (0x3UL << DMA350_CH_CMD_DESSWTRIGINTYPE_Pos)                 /*!< 0x00600000UL*/
#define DMA350_CH_CMD_DESSWTRIGINTYPE                  DMA350_CH_CMD_DESSWTRIGINTYPE_Msk                            /*!< DESSWTRIGINTYPE[ 1:0] bits Software Generated Destination Trigger Input Request Type. Selects the trigger request type for the destination trigger input when the SW triggers the DESSWTRIGINREQ bit.
 - 00: Single request
 - 01: Last single request
 - 10: Block request
 - 11: Last block request
This field cannot be changed while the DESSWTRIGINREQ bit is set.*/
#define DMA350_CH_CMD_DESSWTRIGINTYPE_0                (0x1UL << DMA350_CH_CMD_DESSWTRIGINTYPE_Pos)                 /*!< 0x00200000UL*/
#define DMA350_CH_CMD_DESSWTRIGINTYPE_1                (0x2UL << DMA350_CH_CMD_DESSWTRIGINTYPE_Pos)                 /*!< 0x00400000UL*/
#define DMA350_CH_CMD_SWTRIGOUTACK_Pos                 (24U)
#define DMA350_CH_CMD_SWTRIGOUTACK_Msk                 (0x1UL << DMA350_CH_CMD_SWTRIGOUTACK_Pos)                    /*!< 0x01000000UL*/
#define DMA350_CH_CMD_SWTRIGOUTACK                     DMA350_CH_CMD_SWTRIGOUTACK_Msk                               /*!< SWTRIGOUTACK bit Software Generated Trigger Output Acknowledge. Write '1' to acknowledge a Trigger Output request from the DMA. Once set to '1', this will remain high until the DMA Trigger Output is raised (either on the trigger output signal or as an interrupt) and the acknowledge is accepted. When the channel is not enabled, write to this register is ignored.*/
/*****************  Field definitions for CH_STATUS register  *****************/
#define DMA350_CH_STATUS_INTR_DONE_Pos                 (0U)
#define DMA350_CH_STATUS_INTR_DONE_Msk                 (0x1UL << DMA350_CH_STATUS_INTR_DONE_Pos)                    /*!< 0x00000001UL*/
#define DMA350_CH_STATUS_INTR_DONE                     DMA350_CH_STATUS_INTR_DONE_Msk                               /*!< INTR_DONE bit Done Interrupt Flag. This interrupt will be set to HIGH if the INTREN_DONE is set and the STAT_DONE status flag gets raised. Automatically cleared when STAT_DONE is cleared.*/
#define DMA350_CH_STATUS_INTR_ERR_Pos                  (1U)
#define DMA350_CH_STATUS_INTR_ERR_Msk                  (0x1UL << DMA350_CH_STATUS_INTR_ERR_Pos)                     /*!< 0x00000002UL*/
#define DMA350_CH_STATUS_INTR_ERR                      DMA350_CH_STATUS_INTR_ERR_Msk                                /*!< INTR_ERR bit Error Interrupt Flag. This interrupt will be set to HIGH if the INTREN_ERR is set and the STAT_ERR status flag gets raised. Automatically cleared when STAT_ERR is cleared.*/
#define DMA350_CH_STATUS_INTR_DISABLED_Pos             (2U)
#define DMA350_CH_STATUS_INTR_DISABLED_Msk             (0x1UL << DMA350_CH_STATUS_INTR_DISABLED_Pos)                /*!< 0x00000004UL*/
#define DMA350_CH_STATUS_INTR_DISABLED                 DMA350_CH_STATUS_INTR_DISABLED_Msk                           /*!< INTR_DISABLED bit Disabled Interrupt Flag. This interrupt will be set to HIGH if the INTREN_DISABLED is set and the STAT_DISABLED flag gets raised. Automatically cleared when STAT_DISABLED is cleared.*/
#define DMA350_CH_STATUS_INTR_STOPPED_Pos              (3U)
#define DMA350_CH_STATUS_INTR_STOPPED_Msk              (0x1UL << DMA350_CH_STATUS_INTR_STOPPED_Pos)                 /*!< 0x00000008UL*/
#define DMA350_CH_STATUS_INTR_STOPPED                  DMA350_CH_STATUS_INTR_STOPPED_Msk                            /*!< INTR_STOPPED bit Stopped Interrupt Flag. This interrupt will be set to HIGH if the INTREN_STOPPED is set and the STAT_STOPPED flag gets raised. Automatically cleared when STAT_STOPPED is cleared.*/
#define DMA350_CH_STATUS_INTR_SRCTRIGINWAIT_Pos        (8U)
#define DMA350_CH_STATUS_INTR_SRCTRIGINWAIT_Msk        (0x1UL << DMA350_CH_STATUS_INTR_SRCTRIGINWAIT_Pos)           /*!< 0x00000100UL*/
#define DMA350_CH_STATUS_INTR_SRCTRIGINWAIT            DMA350_CH_STATUS_INTR_SRCTRIGINWAIT_Msk                      /*!< INTR_SRCTRIGINWAIT bit Channel is waiting for Source Trigger Interrupt Flag. This interrupt will be set to HIGH if the INTREN_SRCTRIGINWAIT is set and the STAT_SRCTRIGINWAIT status flag is asserted. Automatically cleared when STAT_SRCTRIGINWAIT is cleared.*/
#define DMA350_CH_STATUS_INTR_DESTRIGINWAIT_Pos        (9U)
#define DMA350_CH_STATUS_INTR_DESTRIGINWAIT_Msk        (0x1UL << DMA350_CH_STATUS_INTR_DESTRIGINWAIT_Pos)           /*!< 0x00000200UL*/
#define DMA350_CH_STATUS_INTR_DESTRIGINWAIT            DMA350_CH_STATUS_INTR_DESTRIGINWAIT_Msk                      /*!< INTR_DESTRIGINWAIT bit Channel is waiting for Destination Trigger Interrupt Flag. This interrupt will be set to HIGH if the INTREN_DESTRIGINWAIT is set and the STAT_DESTRIGINWAIT status flag is asserted. Automatically cleared when STAT_DESTRIGINWAIT is cleared.*/
#define DMA350_CH_STATUS_INTR_TRIGOUTACKWAIT_Pos       (10U)
#define DMA350_CH_STATUS_INTR_TRIGOUTACKWAIT_Msk       (0x1UL << DMA350_CH_STATUS_INTR_TRIGOUTACKWAIT_Pos)          /*!< 0x00000400UL*/
#define DMA350_CH_STATUS_INTR_TRIGOUTACKWAIT           DMA350_CH_STATUS_INTR_TRIGOUTACKWAIT_Msk                     /*!< INTR_TRIGOUTACKWAIT bit Channel is waiting for output Trigger Acknowledgement Interrupt Flag.  This interrupt will be set to HIGH if the INTREN_TRIGOUTACKWAIT is set and the STAT_TRIGOUTACKWAIT status flag is asserted. Automatically cleared when STAT_TRIGOUTACKWAIT is cleared.*/
#define DMA350_CH_STATUS_STAT_DONE_Pos                 (16U)
#define DMA350_CH_STATUS_STAT_DONE_Msk                 (0x1UL << DMA350_CH_STATUS_STAT_DONE_Pos)                    /*!< 0x00010000UL*/
#define DMA350_CH_STATUS_STAT_DONE                     DMA350_CH_STATUS_STAT_DONE_Msk                               /*!< STAT_DONE bit Done Status Flag. This flag will be set to HIGH when the DMA command reaches the state defined by the DONETYPE settings. When DONEPAUSEEN is set the DMA command operation is paused when this flag is asserted. Write '1' to this bit to clear it. Automatically cleared when the ENABLECMD is set.*/
#define DMA350_CH_STATUS_STAT_ERR_Pos                  (17U)
#define DMA350_CH_STATUS_STAT_ERR_Msk                  (0x1UL << DMA350_CH_STATUS_STAT_ERR_Pos)                     /*!< 0x00020000UL*/
#define DMA350_CH_STATUS_STAT_ERR                      DMA350_CH_STATUS_STAT_ERR_Msk                                /*!< STAT_ERR bit Error Status Flag. This flag will be set to HIGH if the DMA encounters an error during its operation. The details about the error event can be found in the ERRINFO register. Write '1' to this bit to clear it. When cleared, it also clears the ERRINFO register. Automatically cleared when the ENABLECMD is set.*/
#define DMA350_CH_STATUS_STAT_DISABLED_Pos             (18U)
#define DMA350_CH_STATUS_STAT_DISABLED_Msk             (0x1UL << DMA350_CH_STATUS_STAT_DISABLED_Pos)                /*!< 0x00040000UL*/
#define DMA350_CH_STATUS_STAT_DISABLED                 DMA350_CH_STATUS_STAT_DISABLED_Msk                           /*!< STAT_DISABLED bit Disabled Status Flag. This flag will be set to HIGH if the DMA channel is successfully disabled using the DISABLECMD command. Write '1' to this bit to clear it. Automatically cleared when the ENABLECMD is set.*/
#define DMA350_CH_STATUS_STAT_STOPPED_Pos              (19U)
#define DMA350_CH_STATUS_STAT_STOPPED_Msk              (0x1UL << DMA350_CH_STATUS_STAT_STOPPED_Pos)                 /*!< 0x00080000UL*/
#define DMA350_CH_STATUS_STAT_STOPPED                  DMA350_CH_STATUS_STAT_STOPPED_Msk                            /*!< STAT_STOPPED bit Stopped Status Flag. This flag will be set to HIGH if the DMA channel successfully reached the stopped state. The stop request can come from many internal or external sources. Write '1' to this bit to clear it. Automatically cleared when the ENABLECMD is set.*/
#define DMA350_CH_STATUS_STAT_PAUSED_Pos               (20U)
#define DMA350_CH_STATUS_STAT_PAUSED_Msk               (0x1UL << DMA350_CH_STATUS_STAT_PAUSED_Pos)                  /*!< 0x00100000UL*/
#define DMA350_CH_STATUS_STAT_PAUSED                   DMA350_CH_STATUS_STAT_PAUSED_Msk                             /*!< STAT_PAUSED bit Paused Status Flag. This flag will be set to HIGH if the DMA channel successfully paused the operation of the command. The pause request can come from many internal or external sources. When the request to pause is not asserted anymore the bit will be cleared automatically and the command operation can continue.*/
#define DMA350_CH_STATUS_STAT_RESUMEWAIT_Pos           (21U)
#define DMA350_CH_STATUS_STAT_RESUMEWAIT_Msk           (0x1UL << DMA350_CH_STATUS_STAT_RESUMEWAIT_Pos)              /*!< 0x00200000UL*/
#define DMA350_CH_STATUS_STAT_RESUMEWAIT               DMA350_CH_STATUS_STAT_RESUMEWAIT_Msk                         /*!< STAT_RESUMEWAIT bit Waiting for resume from software Flag. This flag indicates that the DMA channel successfully paused the operation of the command and needs SW acknowledgment to resume the operation. Will be set to HIGH if STAT_PAUSED is asserted and the PAUSECMD bit set in the command register or when the STAT_DONE is asserted and the DONEPAUSEEN bit is set. Cleared when the RESUMECMD bit is set in the command register.*/
#define DMA350_CH_STATUS_STAT_SRCTRIGINWAIT_Pos        (24U)
#define DMA350_CH_STATUS_STAT_SRCTRIGINWAIT_Msk        (0x1UL << DMA350_CH_STATUS_STAT_SRCTRIGINWAIT_Pos)           /*!< 0x01000000UL*/
#define DMA350_CH_STATUS_STAT_SRCTRIGINWAIT            DMA350_CH_STATUS_STAT_SRCTRIGINWAIT_Msk                      /*!< STAT_SRCTRIGINWAIT bit Channel is waiting for Source Trigger Status. This bit is set to HIGH when DMA channel starts waiting for source input trigger request. Automatically cleared when the source trigger request is received either from HW or SW source. */
#define DMA350_CH_STATUS_STAT_DESTRIGINWAIT_Pos        (25U)
#define DMA350_CH_STATUS_STAT_DESTRIGINWAIT_Msk        (0x1UL << DMA350_CH_STATUS_STAT_DESTRIGINWAIT_Pos)           /*!< 0x02000000UL*/
#define DMA350_CH_STATUS_STAT_DESTRIGINWAIT            DMA350_CH_STATUS_STAT_DESTRIGINWAIT_Msk                      /*!< STAT_DESTRIGINWAIT bit Channel is waiting for Destination Trigger Status. This bit is set to HIGH when DMA channel starts waiting for destination input trigger request. Automatically cleared when the destination trigger request is received either from HW or SW source.*/
#define DMA350_CH_STATUS_STAT_TRIGOUTACKWAIT_Pos       (26U)
#define DMA350_CH_STATUS_STAT_TRIGOUTACKWAIT_Msk       (0x1UL << DMA350_CH_STATUS_STAT_TRIGOUTACKWAIT_Pos)          /*!< 0x04000000UL*/
#define DMA350_CH_STATUS_STAT_TRIGOUTACKWAIT           DMA350_CH_STATUS_STAT_TRIGOUTACKWAIT_Msk                     /*!< STAT_TRIGOUTACKWAIT bit Channel is waiting for output Trigger Acknowledgement Status. This bit is set to HIGH when DMA channel starts waiting for output trigger acknowledgement. Automatically cleared when the output trigger acknowledgement is received either from HW or SW source.*/
/*****************  Field definitions for CH_INTREN register  *****************/
#define DMA350_CH_INTREN_INTREN_DONE_Pos               (0U)
#define DMA350_CH_INTREN_INTREN_DONE_Msk               (0x1UL << DMA350_CH_INTREN_INTREN_DONE_Pos)                  /*!< 0x00000001UL*/
#define DMA350_CH_INTREN_INTREN_DONE                   DMA350_CH_INTREN_INTREN_DONE_Msk                             /*!< INTREN_DONE bit Done Interrupt Enable. When set to HIGH, enables the INTR_DONE to be set and raise an interrupt when the STAT_DONE status flag is asserted. When set to LOW, it prevents INTR_DONE to be asserted. Currently pending interrupts are not affected by clearing this bit.*/
#define DMA350_CH_INTREN_INTREN_ERR_Pos                (1U)
#define DMA350_CH_INTREN_INTREN_ERR_Msk                (0x1UL << DMA350_CH_INTREN_INTREN_ERR_Pos)                   /*!< 0x00000002UL*/
#define DMA350_CH_INTREN_INTREN_ERR                    DMA350_CH_INTREN_INTREN_ERR_Msk                              /*!< INTREN_ERR bit Error Interrupt Enable. When set to HIGH, enables INTR_ERROR to be set and raise an interrupt when the STAT_ERR status flag is asserted. When set to LOW, it prevents INTR_ERR to be asserted. Currently pending interrupts are not affected by clearing this bit.*/
#define DMA350_CH_INTREN_INTREN_DISABLED_Pos           (2U)
#define DMA350_CH_INTREN_INTREN_DISABLED_Msk           (0x1UL << DMA350_CH_INTREN_INTREN_DISABLED_Pos)              /*!< 0x00000004UL*/
#define DMA350_CH_INTREN_INTREN_DISABLED               DMA350_CH_INTREN_INTREN_DISABLED_Msk                         /*!< INTREN_DISABLED bit Disabled Interrupt Enable. When set to HIGH, enables INTR_DISABLED to be set and raise an interrupt when STAT_DISABLED status flag is asserted. When set to LOW, it prevents INTR_DISABLED to be asserted. Currently pending interrupts are not affected by clearing this bit.*/
#define DMA350_CH_INTREN_INTREN_STOPPED_Pos            (3U)
#define DMA350_CH_INTREN_INTREN_STOPPED_Msk            (0x1UL << DMA350_CH_INTREN_INTREN_STOPPED_Pos)               /*!< 0x00000008UL*/
#define DMA350_CH_INTREN_INTREN_STOPPED                DMA350_CH_INTREN_INTREN_STOPPED_Msk                          /*!< INTREN_STOPPED bit Stopped Interrupt Enable. When set to HIGH, enables INTR_STOPPED to be set and raise an interrupt when STAT_STOPPED status flag is asserted. When set to LOW, it prevents INTR_STOPPED to be asserted. Currently pending interrupts are not affected by clearing this bit.*/
#define DMA350_CH_INTREN_INTREN_SRCTRIGINWAIT_Pos      (8U)
#define DMA350_CH_INTREN_INTREN_SRCTRIGINWAIT_Msk      (0x1UL << DMA350_CH_INTREN_INTREN_SRCTRIGINWAIT_Pos)         /*!< 0x00000100UL*/
#define DMA350_CH_INTREN_INTREN_SRCTRIGINWAIT          DMA350_CH_INTREN_INTREN_SRCTRIGINWAIT_Msk                    /*!< INTREN_SRCTRIGINWAIT bit Channel is waiting for Source Trigger Interrupt Enable. When set to HIGH, enables INTR_SRCTRIGINWAIT to be set and raise an interrupt when STAT_SRCTRIGINWAIT status flag is asserted. When set to LOW, it prevents INTR_SRCTRIGINWAIT to be asserted. Currently pending interrupts are not affected by clearing this bit.*/
#define DMA350_CH_INTREN_INTREN_DESTRIGINWAIT_Pos      (9U)
#define DMA350_CH_INTREN_INTREN_DESTRIGINWAIT_Msk      (0x1UL << DMA350_CH_INTREN_INTREN_DESTRIGINWAIT_Pos)         /*!< 0x00000200UL*/
#define DMA350_CH_INTREN_INTREN_DESTRIGINWAIT          DMA350_CH_INTREN_INTREN_DESTRIGINWAIT_Msk                    /*!< INTREN_DESTRIGINWAIT bit Channel is waiting for destination Trigger Interrupt Enable. When set to HIGH, enables INTR_DESTRIGINWAIT to be set and raise an interrupt when STAT_DESTRIGINWAIT status flag is asserted. When set to LOW, it prevents INTR_DESTRIGINWAIT to be asserted. Currently pending interrupts are not affected by clearing this bit.*/
#define DMA350_CH_INTREN_INTREN_TRIGOUTACKWAIT_Pos     (10U)
#define DMA350_CH_INTREN_INTREN_TRIGOUTACKWAIT_Msk     (0x1UL << DMA350_CH_INTREN_INTREN_TRIGOUTACKWAIT_Pos)        /*!< 0x00000400UL*/
#define DMA350_CH_INTREN_INTREN_TRIGOUTACKWAIT         DMA350_CH_INTREN_INTREN_TRIGOUTACKWAIT_Msk                   /*!< INTREN_TRIGOUTACKWAIT bit Channel is waiting for output Trigger Acknowledgement Interrupt Enable. When set to HIGH, enables INTR_TRIGOUTACKWAIT to be set and raise an interrupt when STAT_TRIGOUTACKWAIT status flag is asserted. When set to LOW, it prevents INTR_TRIGOUTACKWAIT to be asserted. Currently pending interrupts are not affected by clearing this bit.*/
/******************  Field definitions for CH_CTRL register  ******************/
#define DMA350_CH_CTRL_TRANSIZE_Pos                    (0U)
#define DMA350_CH_CTRL_TRANSIZE_Msk                    (0x7UL << DMA350_CH_CTRL_TRANSIZE_Pos)                       /*!< 0x00000007UL*/
#define DMA350_CH_CTRL_TRANSIZE                        DMA350_CH_CTRL_TRANSIZE_Msk                                  /*!< TRANSIZE[ 2:0] bits Transfer Entity Size. Size in bytes = 2^TRANSIZE.
 - 000: Byte
 - 001: Halfworld
 - 010: Word
 - 011: Doubleword
 - 100: 128bits
 - 101: 256bits
 - 110: 512bits
 - 111: 1024bits
Note that DATA_WIDTH limits this field. Address will be aligned to TRANSIZE by the DMAC by ignoring the lower bits.*/
#define DMA350_CH_CTRL_TRANSIZE_0                      (0x1UL << DMA350_CH_CTRL_TRANSIZE_Pos)                       /*!< 0x00000001UL*/
#define DMA350_CH_CTRL_TRANSIZE_1                      (0x2UL << DMA350_CH_CTRL_TRANSIZE_Pos)                       /*!< 0x00000002UL*/
#define DMA350_CH_CTRL_TRANSIZE_2                      (0x4UL << DMA350_CH_CTRL_TRANSIZE_Pos)                       /*!< 0x00000004UL*/
#define DMA350_CH_CTRL_CHPRIO_Pos                      (4U)
#define DMA350_CH_CTRL_CHPRIO_Msk                      (0xFUL << DMA350_CH_CTRL_CHPRIO_Pos)                         /*!< 0x000000F0UL*/
#define DMA350_CH_CTRL_CHPRIO                          DMA350_CH_CTRL_CHPRIO_Msk                                    /*!< CHPRIO[ 3:0] bits Channel Priority.
 - 0: Lowest Priority
 - 15: Highest Priority*/
#define DMA350_CH_CTRL_CHPRIO_0                        (0x1UL << DMA350_CH_CTRL_CHPRIO_Pos)                         /*!< 0x00000010UL*/
#define DMA350_CH_CTRL_CHPRIO_1                        (0x2UL << DMA350_CH_CTRL_CHPRIO_Pos)                         /*!< 0x00000020UL*/
#define DMA350_CH_CTRL_CHPRIO_2                        (0x4UL << DMA350_CH_CTRL_CHPRIO_Pos)                         /*!< 0x00000040UL*/
#define DMA350_CH_CTRL_CHPRIO_3                        (0x8UL << DMA350_CH_CTRL_CHPRIO_Pos)                         /*!< 0x00000080UL*/
#define DMA350_CH_CTRL_XTYPE_Pos                       (9U)
#define DMA350_CH_CTRL_XTYPE_Msk                       (0x7UL << DMA350_CH_CTRL_XTYPE_Pos)                          /*!< 0x00000E00UL*/
#define DMA350_CH_CTRL_XTYPE                           DMA350_CH_CTRL_XTYPE_Msk                                     /*!< XTYPE[ 2:0] bits Operation type for X direction:
 - 000: "disable" - No data transfer will take place for this command. This mode can be used to create empty commands that wait for an event or set GPOs.
 - 001: "continue" - Copy data in a continuous manner from source to the destination. For 1D operations it is expected that SRCXSIZE is equal to DESXSIZE, other combinations result in UNPREDICTABLE behavior. For 2D operations, this mode can be used for simple 2D to 2D copy but it also allows the reshaping of the data like 1D to 2D or 2D to 1D conversions. If the DESXSIZE is smaller than SRCXSIZE then the read data from the current source line goes to the next destination line. If SRCXSIZE is smaller than DESXSIZE then the reads start on the next line and data is written to the remainder of the current destination line. Note: For 1D to 2D the SRCYSIZE for 2D to 1D conversion the DESYSIZE needs to be set to 1 when using this mode.
 - 010: "wrap" - Wrap source data within a destination line when the end of the source line is reached. Read starts again from the beginning of the source line and copied to the remainder of the destination line. If the DESXSIZE is smaller than SRCXSIZE then the behavior is UNPREDICTABLE. Not supported when HAS_WRAP is 0 in HW
 - 011: "fill" - Fill the remainder of the destination line with FILLVAL when the end of the source line is reached. If the DESXSIZE is smaller than SRCXSIZE then the behavior is UNPREDICTABLE. Not supported when HAS_WRAP is 0 in HW.
 - Others: Reserved.*/
#define DMA350_CH_CTRL_XTYPE_0                         (0x1UL << DMA350_CH_CTRL_XTYPE_Pos)                          /*!< 0x00000200UL*/
#define DMA350_CH_CTRL_XTYPE_1                         (0x2UL << DMA350_CH_CTRL_XTYPE_Pos)                          /*!< 0x00000400UL*/
#define DMA350_CH_CTRL_XTYPE_2                         (0x4UL << DMA350_CH_CTRL_XTYPE_Pos)                          /*!< 0x00000800UL*/
#define DMA350_CH_CTRL_YTYPE_Pos                       (12U)
#define DMA350_CH_CTRL_YTYPE_Msk                       (0x7UL << DMA350_CH_CTRL_YTYPE_Pos)                          /*!< 0x00007000UL*/
#define DMA350_CH_CTRL_YTYPE                           DMA350_CH_CTRL_YTYPE_Msk                                     /*!< YTYPE[ 2:0] bits Operation type for Y direction:
 - 000: "disable" - Only do 1D transfers. When HAS_2D is 0, meaning 2D capability is not supported in HW, the YTYPE is always "000".
 - 001: "continue" - Copy 2D data in a continuous manner from source area to the destination area by using the YSIZE registers. The copy stops when the source runs out of data or the destination runs out of space. Not supported when HAS_2D is 0 in HW.
 - 010: "wrap" - Wrap the 2D source area within the destination 2D area by starting to copy data from the beginning of the first source line to the remaining space in the destination area. If the destination area is smaller than the source area then the behavior is UNPREDICTABLE. Not supported when HAS_WRAP or HAS_2D is 0 in HW.
 - 011:  Fill the remainder of the destination area with FILLVAL when the source area runs out of data. If the destination area is smaller than the source area then the behavior is UNPREDICTABLE. Not supported when HAS_WRAP or HAS_2D is 0 in HW
 - Others: Reserved*/
#define DMA350_CH_CTRL_YTYPE_0                         (0x1UL << DMA350_CH_CTRL_YTYPE_Pos)                          /*!< 0x00001000UL*/
#define DMA350_CH_CTRL_YTYPE_1                         (0x2UL << DMA350_CH_CTRL_YTYPE_Pos)                          /*!< 0x00002000UL*/
#define DMA350_CH_CTRL_YTYPE_2                         (0x4UL << DMA350_CH_CTRL_YTYPE_Pos)                          /*!< 0x00004000UL*/
#define DMA350_CH_CTRL_REGRELOADTYPE_Pos               (18U)
#define DMA350_CH_CTRL_REGRELOADTYPE_Msk               (0x7UL << DMA350_CH_CTRL_REGRELOADTYPE_Pos)                  /*!< 0x001C0000UL*/
#define DMA350_CH_CTRL_REGRELOADTYPE                   DMA350_CH_CTRL_REGRELOADTYPE_Msk                             /*!< REGRELOADTYPE[ 2:0] bits Automatic register reload type. Defines how the DMA command reloads initial values at the end of a DMA command before autorestarting, ending or linking to a new DMA command:
 - 000: Reload Disabled.
 - 001: Reload source and destination size registers only.
 - 011: Reload source address only and all source and destination size registers.
 - 101: Reload destination address only and all source and destination size registers.
 - 111: Reload source and destination address and all source and destination size registers.
 - Others: Reserved.
NOTE: When CLEARCMD is set, the reloaded registers will also be cleared.*/
#define DMA350_CH_CTRL_REGRELOADTYPE_0                 (0x1UL << DMA350_CH_CTRL_REGRELOADTYPE_Pos)                  /*!< 0x00040000UL*/
#define DMA350_CH_CTRL_REGRELOADTYPE_1                 (0x2UL << DMA350_CH_CTRL_REGRELOADTYPE_Pos)                  /*!< 0x00080000UL*/
#define DMA350_CH_CTRL_REGRELOADTYPE_2                 (0x4UL << DMA350_CH_CTRL_REGRELOADTYPE_Pos)                  /*!< 0x00100000UL*/
#define DMA350_CH_CTRL_DONETYPE_Pos                    (21U)
#define DMA350_CH_CTRL_DONETYPE_Msk                    (0x7UL << DMA350_CH_CTRL_DONETYPE_Pos)                       /*!< 0x00E00000UL*/
#define DMA350_CH_CTRL_DONETYPE                        DMA350_CH_CTRL_DONETYPE_Msk                                  /*!< DONETYPE[ 2:0] bits Done type selection. This field defines when the STAT_DONE status flag is asserted during the command operation.
 - 000: STAT_DONE flag is not asserted for this command.
 - 001: End of a command, before jumping to the next linked command. (default)
 - 011: End of an autorestart cycle, before starting the next cycle.
 - Others : Reserved.*/
#define DMA350_CH_CTRL_DONETYPE_0                      (0x1UL << DMA350_CH_CTRL_DONETYPE_Pos)                       /*!< 0x00200000UL*/
#define DMA350_CH_CTRL_DONETYPE_1                      (0x2UL << DMA350_CH_CTRL_DONETYPE_Pos)                       /*!< 0x00400000UL*/
#define DMA350_CH_CTRL_DONETYPE_2                      (0x4UL << DMA350_CH_CTRL_DONETYPE_Pos)                       /*!< 0x00800000UL*/
#define DMA350_CH_CTRL_DONEPAUSEEN_Pos                 (24U)
#define DMA350_CH_CTRL_DONEPAUSEEN_Msk                 (0x1UL << DMA350_CH_CTRL_DONEPAUSEEN_Pos)                    /*!< 0x01000000UL*/
#define DMA350_CH_CTRL_DONEPAUSEEN                     DMA350_CH_CTRL_DONEPAUSEEN_Msk                               /*!< DONEPAUSEEN bit Done pause enable. When set to HIGH the assertion of the STAT_DONE flag results in an automatic pause request for the current DMA operation. When the paused state is reached the STAT_RESUMEWAIT flag is also set. When set to LOW the assertion of the STAT_DONE does not pause the progress of the command and the next operation of the channel will be started immediately after the STAT_DONE flag is set.*/
#define DMA350_CH_CTRL_USESRCTRIGIN_Pos                (25U)
#define DMA350_CH_CTRL_USESRCTRIGIN_Msk                (0x1UL << DMA350_CH_CTRL_USESRCTRIGIN_Pos)                   /*!< 0x02000000UL*/
#define DMA350_CH_CTRL_USESRCTRIGIN                    DMA350_CH_CTRL_USESRCTRIGIN_Msk                              /*!< USESRCTRIGIN bit Enable Source Trigger Input use for this command.
 - 0: disable
 - 1: enable*/
#define DMA350_CH_CTRL_USEDESTRIGIN_Pos                (26U)
#define DMA350_CH_CTRL_USEDESTRIGIN_Msk                (0x1UL << DMA350_CH_CTRL_USEDESTRIGIN_Pos)                   /*!< 0x04000000UL*/
#define DMA350_CH_CTRL_USEDESTRIGIN                    DMA350_CH_CTRL_USEDESTRIGIN_Msk                              /*!< USEDESTRIGIN bit Enable Destination Trigger Input use for this command.
 - 0: disable
 - 1: enable*/
#define DMA350_CH_CTRL_USETRIGOUT_Pos                  (27U)
#define DMA350_CH_CTRL_USETRIGOUT_Msk                  (0x1UL << DMA350_CH_CTRL_USETRIGOUT_Pos)                     /*!< 0x08000000UL*/
#define DMA350_CH_CTRL_USETRIGOUT                      DMA350_CH_CTRL_USETRIGOUT_Msk                                /*!< USETRIGOUT bit Enable Trigger Output use for this command.
 - 0: disable
 - 1: enable*/
#define DMA350_CH_CTRL_USEGPO_Pos                      (28U)
#define DMA350_CH_CTRL_USEGPO_Msk                      (0x1UL << DMA350_CH_CTRL_USEGPO_Pos)                         /*!< 0x10000000UL*/
#define DMA350_CH_CTRL_USEGPO                          DMA350_CH_CTRL_USEGPO_Msk                                    /*!< USEGPO bit Enable GPO use for this command.
 - 0: disable
 - 1: enable*/
#define DMA350_CH_CTRL_USESTREAM_Pos                   (29U)
#define DMA350_CH_CTRL_USESTREAM_Msk                   (0x1UL << DMA350_CH_CTRL_USESTREAM_Pos)                      /*!< 0x20000000UL*/
#define DMA350_CH_CTRL_USESTREAM                       DMA350_CH_CTRL_USESTREAM_Msk                                 /*!< USESTREAM bit Enable Stream Interface use for this command.
 - 0: disable
 - 1: enable*/
/****************  Field definitions for CH_SRCADDR register  *****************/
#define DMA350_CH_SRCADDR_SRCADDR_Pos                  (0U)
#define DMA350_CH_SRCADDR_SRCADDR_Msk                  (0xFFFFFFFFUL << DMA350_CH_SRCADDR_SRCADDR_Pos)              /*!< 0xFFFFFFFFUL*/
#define DMA350_CH_SRCADDR_SRCADDR                      DMA350_CH_SRCADDR_SRCADDR_Msk                                /*!< SRCADDR[31:0] bits Source Address [31:0].*/
/***************  Field definitions for CH_SRCADDRHI register  ****************/
#define DMA350_CH_SRCADDRHI_SRCADDRHI_Pos              (0U)
#define DMA350_CH_SRCADDRHI_SRCADDRHI_Msk              (0xFFFFFFFFUL << DMA350_CH_SRCADDRHI_SRCADDRHI_Pos)          /*!< 0xFFFFFFFFUL*/
#define DMA350_CH_SRCADDRHI_SRCADDRHI                  DMA350_CH_SRCADDRHI_SRCADDRHI_Msk                            /*!< SRCADDRHI[31:0] bits Source Address [63:32]. Allows 64-bit addressing but the system might need less address bits defined by ADDR_WIDTH. The not implemented bits remain reserved.*/
/****************  Field definitions for CH_DESADDR register  *****************/
#define DMA350_CH_DESADDR_DESADDR_Pos                  (0U)
#define DMA350_CH_DESADDR_DESADDR_Msk                  (0xFFFFFFFFUL << DMA350_CH_DESADDR_DESADDR_Pos)              /*!< 0xFFFFFFFFUL*/
#define DMA350_CH_DESADDR_DESADDR                      DMA350_CH_DESADDR_DESADDR_Msk                                /*!< DESADDR[31:0] bits Destination Address[31:0]*/
/***************  Field definitions for CH_DESADDRHI register  ****************/
#define DMA350_CH_DESADDRHI_DESADDRHI_Pos              (0U)
#define DMA350_CH_DESADDRHI_DESADDRHI_Msk              (0xFFFFFFFFUL << DMA350_CH_DESADDRHI_DESADDRHI_Pos)          /*!< 0xFFFFFFFFUL*/
#define DMA350_CH_DESADDRHI_DESADDRHI                  DMA350_CH_DESADDRHI_DESADDRHI_Msk                            /*!< DESADDRHI[31:0] bits Destination Address[63:32]. Allows 64-bit addressing but the system might need less address bits defined by ADDR_WIDTH. The not implemented bits remain reserved.*/
/*****************  Field definitions for CH_XSIZE register  ******************/
#define DMA350_CH_XSIZE_SRCXSIZE_Pos                   (0U)
#define DMA350_CH_XSIZE_SRCXSIZE_Msk                   (0xFFFFUL << DMA350_CH_XSIZE_SRCXSIZE_Pos)                   /*!< 0x0000FFFFUL*/
#define DMA350_CH_XSIZE_SRCXSIZE                       DMA350_CH_XSIZE_SRCXSIZE_Msk                                 /*!< SRCXSIZE[15:0] bits Source Number of Transfers in the X Dimension lower bits [15:0]. This register along with SRCXSIZEHI defines the source data block size of the DMA operation for any 1D operation, and defines the X dimension of the 2D source block for 2D operation.*/
#define DMA350_CH_XSIZE_DESXSIZE_Pos                   (16U)
#define DMA350_CH_XSIZE_DESXSIZE_Msk                   (0xFFFFUL << DMA350_CH_XSIZE_DESXSIZE_Pos)                   /*!< 0xFFFF0000UL*/
#define DMA350_CH_XSIZE_DESXSIZE                       DMA350_CH_XSIZE_DESXSIZE_Msk                                 /*!< DESXSIZE[15:0] bits Destination Number of Transfers in the X Dimension lower bits [15:0]. This register along with DESXSIZEHI defines the destination data block size of the DMA operation for or any 1D operation, and defines the X dimension of the 2D destination block for a 2D operation. HAS_WRAP or HAS_STREAM configuration needs to be set to allow writes to this register, otherwise it is read-only and writing to SRCXSIZE will also update the value of this register.*/
/****************  Field definitions for CH_XSIZEHI register  *****************/
#define DMA350_CH_XSIZEHI_SRCXSIZEHI_Pos               (0U)
#define DMA350_CH_XSIZEHI_SRCXSIZEHI_Msk               (0xFFFFUL << DMA350_CH_XSIZEHI_SRCXSIZEHI_Pos)               /*!< 0x0000FFFFUL*/
#define DMA350_CH_XSIZEHI_SRCXSIZEHI                   DMA350_CH_XSIZEHI_SRCXSIZEHI_Msk                             /*!< SRCXSIZEHI[15:0] bits Source Number of Transfers in the X Dimension high bits [31:16]. This register along with SRCXSIZE defines the source data block size of the DMA operation for any 1D operation, and defines the X dimension of the 2D source block for 2D operation.*/
#define DMA350_CH_XSIZEHI_DESXSIZEHI_Pos               (16U)
#define DMA350_CH_XSIZEHI_DESXSIZEHI_Msk               (0xFFFFUL << DMA350_CH_XSIZEHI_DESXSIZEHI_Pos)               /*!< 0xFFFF0000UL*/
#define DMA350_CH_XSIZEHI_DESXSIZEHI                   DMA350_CH_XSIZEHI_DESXSIZEHI_Msk                             /*!< DESXSIZEHI[15:0] bits Destination Number of Transfers in the X Dimension high bits [31:16]. This register along with DESXSIZE defines the destination data block size of the DMA operation for or any 1D operation, and defines the X dimension of the 2D destination block for a 2D operation. HAS_WRAP or HAS_STREAM configuration needs to be set to allow writes to this register, otherwise it is read-only and writing to SRCXSIZEHI will also update the value of this register.*/
/**************  Field definitions for CH_SRCTRANSCFG register  ***************/
#define DMA350_CH_SRCTRANSCFG_SRCMEMATTRLO_Pos         (0U)
#define DMA350_CH_SRCTRANSCFG_SRCMEMATTRLO_Msk         (0xFUL << DMA350_CH_SRCTRANSCFG_SRCMEMATTRLO_Pos)            /*!< 0x0000000FUL*/
#define DMA350_CH_SRCTRANSCFG_SRCMEMATTRLO             DMA350_CH_SRCTRANSCFG_SRCMEMATTRLO_Msk                       /*!< SRCMEMATTRLO[ 3:0] bits Source Transfer Memory Attribute field [3:0].
When SRCMEMATTRHI is Device type (0000) then this field means:
 - 0000: Device-nGnRnE
 - 0100: Device-nGnRE
 - 1000: Device-nGRE
 - 1100: Device-GRE
 - Others: Invalid resulting in UNPREDICTABLE behavior
When SRCMEMATTRHI is Normal memory type (other than 0000) then this field means:
 - 0000: Reserved
 - 0001: Normal memory, Inner Write allocate, Inner Write-through transient
 - 0010: Normal memory, Inner Read allocate, Inner Write-through transient
 - 0011: Normal memory, Inner Read/Write allocate, Inner Write-through transient
 - 0100: Normal memory, Inner non-cacheable
 - 0101: Normal memory, Inner Write allocate, Inner Write-back transient
 - 0110: Normal memory, Inner Read allocate, Inner Write-back transient
 - 0111: Normal memory, Inner Read/Write allocate, Inner Write-back transient
 - 1000: Normal memory, Inner Write-through non-transient
 - 1001: Normal memory, Inner Write allocate, Inner Write-through non-transient
 - 1010: Normal memory, Inner Read allocate, Inner Write-through non-transient
 - 1011: Normal memory, Inner Read/Write allocate, Inner Write-through non-transient
 - 1100: Normal memory, Inner Write-back non-transient
 - 1101: Normal memory, Inner Write allocate, Inner Write-back non-transient
 - 1110: Normal memory, Inner Read allocate, Inner Write-back non-transient
 - 1111: Normal memory, Inner Read/Write allocate, Inner Write-back non-transient*/
#define DMA350_CH_SRCTRANSCFG_SRCMEMATTRLO_0           (0x1UL << DMA350_CH_SRCTRANSCFG_SRCMEMATTRLO_Pos)            /*!< 0x00000001UL*/
#define DMA350_CH_SRCTRANSCFG_SRCMEMATTRLO_1           (0x2UL << DMA350_CH_SRCTRANSCFG_SRCMEMATTRLO_Pos)            /*!< 0x00000002UL*/
#define DMA350_CH_SRCTRANSCFG_SRCMEMATTRLO_2           (0x4UL << DMA350_CH_SRCTRANSCFG_SRCMEMATTRLO_Pos)            /*!< 0x00000004UL*/
#define DMA350_CH_SRCTRANSCFG_SRCMEMATTRLO_3           (0x8UL << DMA350_CH_SRCTRANSCFG_SRCMEMATTRLO_Pos)            /*!< 0x00000008UL*/
#define DMA350_CH_SRCTRANSCFG_SRCMEMATTRHI_Pos         (4U)
#define DMA350_CH_SRCTRANSCFG_SRCMEMATTRHI_Msk         (0xFUL << DMA350_CH_SRCTRANSCFG_SRCMEMATTRHI_Pos)            /*!< 0x000000F0UL*/
#define DMA350_CH_SRCTRANSCFG_SRCMEMATTRHI             DMA350_CH_SRCTRANSCFG_SRCMEMATTRHI_Msk                       /*!< SRCMEMATTRHI[ 3:0] bits Source Transfer Memory Attribute field [7:4].
 - 0000: Device memory
 - 0001: Normal memory, Outer Write allocate, Outer Write-through transient
 - 0010: Normal memory, Outer Read allocate, Outer Write-through transient
 - 0011: Normal memory, Outer Read/Write allocate, Outer Write-through transient
 - 0100: Normal memory, Outer non-cacheable
 - 0101: Normal memory, Outer Write allocate, Outer Write-back transient
 - 0110: Normal memory, Outer Read allocate, Outer Write-back transient
 - 0111: Normal memory, Outer Read/Write allocate, Outer Write-back transient
 - 1000: Normal memory, Outer Write-through non-transient
 - 1001: Normal memory, Outer Write allocate, Outer Write-through non-transient
 - 1010: Normal memory, Outer Read allocate, Outer Write-through non-transient
 - 1011: Normal memory, Outer Read/Write allocate, Outer Write-through non-transient
 - 1100: Normal memory, Outer Write-back non-transient
 - 1101: Normal memory, Outer Write allocate, Outer Write-back non-transient
 - 1110: Normal memory, Outer Read allocate, Outer Write-back non-transient
 - 1111: Normal memory, Outer Read/Write allocate, Outer Write-back non-transient*/
#define DMA350_CH_SRCTRANSCFG_SRCMEMATTRHI_0           (0x1UL << DMA350_CH_SRCTRANSCFG_SRCMEMATTRHI_Pos)            /*!< 0x00000010UL*/
#define DMA350_CH_SRCTRANSCFG_SRCMEMATTRHI_1           (0x2UL << DMA350_CH_SRCTRANSCFG_SRCMEMATTRHI_Pos)            /*!< 0x00000020UL*/
#define DMA350_CH_SRCTRANSCFG_SRCMEMATTRHI_2           (0x4UL << DMA350_CH_SRCTRANSCFG_SRCMEMATTRHI_Pos)            /*!< 0x00000040UL*/
#define DMA350_CH_SRCTRANSCFG_SRCMEMATTRHI_3           (0x8UL << DMA350_CH_SRCTRANSCFG_SRCMEMATTRHI_Pos)            /*!< 0x00000080UL*/
#define DMA350_CH_SRCTRANSCFG_SRCSHAREATTR_Pos         (8U)
#define DMA350_CH_SRCTRANSCFG_SRCSHAREATTR_Msk         (0x3UL << DMA350_CH_SRCTRANSCFG_SRCSHAREATTR_Pos)            /*!< 0x00000300UL*/
#define DMA350_CH_SRCTRANSCFG_SRCSHAREATTR             DMA350_CH_SRCTRANSCFG_SRCSHAREATTR_Msk                       /*!< SRCSHAREATTR[ 1:0] bits Source Transfer Shareability Attribute.
 - 00: Non-shareable
 - 01: Reserved
 - 10: Outer shareable
 - 11: Inner shareable */
#define DMA350_CH_SRCTRANSCFG_SRCSHAREATTR_0           (0x1UL << DMA350_CH_SRCTRANSCFG_SRCSHAREATTR_Pos)            /*!< 0x00000100UL*/
#define DMA350_CH_SRCTRANSCFG_SRCSHAREATTR_1           (0x2UL << DMA350_CH_SRCTRANSCFG_SRCSHAREATTR_Pos)            /*!< 0x00000200UL*/
#define DMA350_CH_SRCTRANSCFG_SRCNONSECATTR_Pos        (10U)
#define DMA350_CH_SRCTRANSCFG_SRCNONSECATTR_Msk        (0x1UL << DMA350_CH_SRCTRANSCFG_SRCNONSECATTR_Pos)           /*!< 0x00000400UL*/
#define DMA350_CH_SRCTRANSCFG_SRCNONSECATTR            DMA350_CH_SRCTRANSCFG_SRCNONSECATTR_Msk                      /*!< SRCNONSECATTR bit Source Transfer Non-secure Attribute.
 - 0: Secure
 - 1: Non-secure
 When a channel is Non-secure this bit is tied to 1. */
#define DMA350_CH_SRCTRANSCFG_SRCPRIVATTR_Pos          (11U)
#define DMA350_CH_SRCTRANSCFG_SRCPRIVATTR_Msk          (0x1UL << DMA350_CH_SRCTRANSCFG_SRCPRIVATTR_Pos)             /*!< 0x00000800UL*/
#define DMA350_CH_SRCTRANSCFG_SRCPRIVATTR              DMA350_CH_SRCTRANSCFG_SRCPRIVATTR_Msk                        /*!< SRCPRIVATTR bit Source Transfer Privilege Attribute.
 - 0: Unprivileged
 - 1: Privileged
 When a channel is unprivileged this bit is tied to 0.*/
#define DMA350_CH_SRCTRANSCFG_SRCMAXBURSTLEN_Pos       (16U)
#define DMA350_CH_SRCTRANSCFG_SRCMAXBURSTLEN_Msk       (0xFUL << DMA350_CH_SRCTRANSCFG_SRCMAXBURSTLEN_Pos)          /*!< 0x000F0000UL*/
#define DMA350_CH_SRCTRANSCFG_SRCMAXBURSTLEN           DMA350_CH_SRCTRANSCFG_SRCMAXBURSTLEN_Msk                     /*!< SRCMAXBURSTLEN[ 3:0] bits Source Max Burst Length. Hint for the DMA on what is the maximum allowed burst size it can use for read transfers. The maximum number of beats sent by the DMA for a read burst is equal to SRCMAXBURSTLEN + 1. Default value is 16 beats, which allows the DMA to set all burst sizes. Note: Limited by the DATA_BUFF_SIZE so larger settings may not always result in larger bursts.*/
#define DMA350_CH_SRCTRANSCFG_SRCMAXBURSTLEN_0         (0x1UL << DMA350_CH_SRCTRANSCFG_SRCMAXBURSTLEN_Pos)          /*!< 0x00010000UL*/
#define DMA350_CH_SRCTRANSCFG_SRCMAXBURSTLEN_1         (0x2UL << DMA350_CH_SRCTRANSCFG_SRCMAXBURSTLEN_Pos)          /*!< 0x00020000UL*/
#define DMA350_CH_SRCTRANSCFG_SRCMAXBURSTLEN_2         (0x4UL << DMA350_CH_SRCTRANSCFG_SRCMAXBURSTLEN_Pos)          /*!< 0x00040000UL*/
#define DMA350_CH_SRCTRANSCFG_SRCMAXBURSTLEN_3         (0x8UL << DMA350_CH_SRCTRANSCFG_SRCMAXBURSTLEN_Pos)          /*!< 0x00080000UL*/
/**************  Field definitions for CH_DESTRANSCFG register  ***************/
#define DMA350_CH_DESTRANSCFG_DESMEMATTRLO_Pos         (0U)
#define DMA350_CH_DESTRANSCFG_DESMEMATTRLO_Msk         (0xFUL << DMA350_CH_DESTRANSCFG_DESMEMATTRLO_Pos)            /*!< 0x0000000FUL*/
#define DMA350_CH_DESTRANSCFG_DESMEMATTRLO             DMA350_CH_DESTRANSCFG_DESMEMATTRLO_Msk                       /*!< DESMEMATTRLO[ 3:0] bits Destination Transfer Memory Attribute field [3:0].
When DESMEMATTRHI is Device type (0000) then this field means:
 - 0000: Device-nGnRnE
 - 0100: Device-nGnRE
 - 1000: Device-nGRE
 - 1100: Device-GRE
 - Others: Invalid resulting in UNPREDICTABLE behavior
When DESMEMATTRHI is Normal Memory type (other than 0000) then this field means:
 - 0000: Reserved
 - 0001: Normal memory, Inner Write allocate, Inner Write-through transient
 - 0010: Normal memory, Inner Read allocate, Inner Write-through transient
 - 0011: Normal memory, Inner Read/Write allocate, Inner Write-through transient
 - 0100: Normal memory, Inner non-cacheable
 - 0101: Normal memory, Inner Write allocate, Inner Write-back transient
 - 0110: Normal memory, Inner Read allocate, Inner Write-back transient
 - 0111: Normal memory, Inner Read/Write allocate, Inner Write-back transient
 - 1000: Normal memory, Inner Write-through non-transient
 - 1001: Normal memory, Inner Write allocate, Inner Write-through non-transient
 - 1010: Normal memory, Inner Read allocate, Inner Write-through non-transient
 - 1011: Normal memory, Inner Read/Write allocate, Inner Write-through non-transient
 - 1100: Normal memory, Inner Write-back non-transient
 - 1101: Normal memory, Inner Write allocate, Inner Write-back non-transient
 - 1110: Normal memory, Inner Read allocate, Inner Write-back non-transient
 - 1111: Normal memory, Inner Read/Write allocate, Inner Write-back non-transient*/
#define DMA350_CH_DESTRANSCFG_DESMEMATTRLO_0           (0x1UL << DMA350_CH_DESTRANSCFG_DESMEMATTRLO_Pos)            /*!< 0x00000001UL*/
#define DMA350_CH_DESTRANSCFG_DESMEMATTRLO_1           (0x2UL << DMA350_CH_DESTRANSCFG_DESMEMATTRLO_Pos)            /*!< 0x00000002UL*/
#define DMA350_CH_DESTRANSCFG_DESMEMATTRLO_2           (0x4UL << DMA350_CH_DESTRANSCFG_DESMEMATTRLO_Pos)            /*!< 0x00000004UL*/
#define DMA350_CH_DESTRANSCFG_DESMEMATTRLO_3           (0x8UL << DMA350_CH_DESTRANSCFG_DESMEMATTRLO_Pos)            /*!< 0x00000008UL*/
#define DMA350_CH_DESTRANSCFG_DESMEMATTRHI_Pos         (4U)
#define DMA350_CH_DESTRANSCFG_DESMEMATTRHI_Msk         (0xFUL << DMA350_CH_DESTRANSCFG_DESMEMATTRHI_Pos)            /*!< 0x000000F0UL*/
#define DMA350_CH_DESTRANSCFG_DESMEMATTRHI             DMA350_CH_DESTRANSCFG_DESMEMATTRHI_Msk                       /*!< DESMEMATTRHI[ 3:0] bits Destination Transfer Memory Attribute field [7:4].
 - 0000: Device memory
 - 0001: Normal memory, Outer Write allocate, Outer Write-through transient
 - 0010: Normal memory, Outer Read allocate, Outer Write-through transient
 - 0011: Normal memory, Outer Read/Write allocate, Outer Write-through transient
 - 0100: Normal memory, Outer non-cacheable
 - 0101: Normal memory, Outer Write allocate, Outer Write-back transient
 - 0110: Normal memory, Outer Read allocate, Outer Write-back transient
 - 0111: Normal memory, Outer Read/Write allocate, Outer Write-back transient
 - 1000: Normal memory, Outer Write-through non-transient
 - 1001: Normal memory, Outer Write allocate, Outer Write-through non-transient
 - 1010: Normal memory, Outer Read allocate, Outer Write-through non-transient
 - 1011: Normal memory, Outer Read/Write allocate, Outer Write-through non-transient
 - 1100: Normal memory, Outer Write-back non-transient
 - 1101: Normal memory, Outer Write allocate, Outer Write-back non-transient
 - 1110: Normal memory, Outer Read allocate, Outer Write-back non-transient
 - 1111: Normal memory, Outer Read/Write allocate, Outer Write-back non-transient*/
#define DMA350_CH_DESTRANSCFG_DESMEMATTRHI_0           (0x1UL << DMA350_CH_DESTRANSCFG_DESMEMATTRHI_Pos)            /*!< 0x00000010UL*/
#define DMA350_CH_DESTRANSCFG_DESMEMATTRHI_1           (0x2UL << DMA350_CH_DESTRANSCFG_DESMEMATTRHI_Pos)            /*!< 0x00000020UL*/
#define DMA350_CH_DESTRANSCFG_DESMEMATTRHI_2           (0x4UL << DMA350_CH_DESTRANSCFG_DESMEMATTRHI_Pos)            /*!< 0x00000040UL*/
#define DMA350_CH_DESTRANSCFG_DESMEMATTRHI_3           (0x8UL << DMA350_CH_DESTRANSCFG_DESMEMATTRHI_Pos)            /*!< 0x00000080UL*/
#define DMA350_CH_DESTRANSCFG_DESSHAREATTR_Pos         (8U)
#define DMA350_CH_DESTRANSCFG_DESSHAREATTR_Msk         (0x3UL << DMA350_CH_DESTRANSCFG_DESSHAREATTR_Pos)            /*!< 0x00000300UL*/
#define DMA350_CH_DESTRANSCFG_DESSHAREATTR             DMA350_CH_DESTRANSCFG_DESSHAREATTR_Msk                       /*!< DESSHAREATTR[ 1:0] bits Destination Transfer Shareability Attribute.
 - 00: Non-shareable
 - 01: Reserved
 - 10: Outer shareable
 - 11: Inner shareable*/
#define DMA350_CH_DESTRANSCFG_DESSHAREATTR_0           (0x1UL << DMA350_CH_DESTRANSCFG_DESSHAREATTR_Pos)            /*!< 0x00000100UL*/
#define DMA350_CH_DESTRANSCFG_DESSHAREATTR_1           (0x2UL << DMA350_CH_DESTRANSCFG_DESSHAREATTR_Pos)            /*!< 0x00000200UL*/
#define DMA350_CH_DESTRANSCFG_DESNONSECATTR_Pos        (10U)
#define DMA350_CH_DESTRANSCFG_DESNONSECATTR_Msk        (0x1UL << DMA350_CH_DESTRANSCFG_DESNONSECATTR_Pos)           /*!< 0x00000400UL*/
#define DMA350_CH_DESTRANSCFG_DESNONSECATTR            DMA350_CH_DESTRANSCFG_DESNONSECATTR_Msk                      /*!< DESNONSECATTR bit Destination Transfer Non-secure Attribute.
 - 0: Secure
 - 1: Non-secure
 When a channel is Non-secure this bit is tied to 1. */
#define DMA350_CH_DESTRANSCFG_DESPRIVATTR_Pos          (11U)
#define DMA350_CH_DESTRANSCFG_DESPRIVATTR_Msk          (0x1UL << DMA350_CH_DESTRANSCFG_DESPRIVATTR_Pos)             /*!< 0x00000800UL*/
#define DMA350_CH_DESTRANSCFG_DESPRIVATTR              DMA350_CH_DESTRANSCFG_DESPRIVATTR_Msk                        /*!< DESPRIVATTR bit Destination Transfer Privilege Attribute.
 - 0: Unprivileged
 - 1: Privileged
 When a channel is unprivileged this bit is tied to 0.*/
#define DMA350_CH_DESTRANSCFG_DESMAXBURSTLEN_Pos       (16U)
#define DMA350_CH_DESTRANSCFG_DESMAXBURSTLEN_Msk       (0xFUL << DMA350_CH_DESTRANSCFG_DESMAXBURSTLEN_Pos)          /*!< 0x000F0000UL*/
#define DMA350_CH_DESTRANSCFG_DESMAXBURSTLEN           DMA350_CH_DESTRANSCFG_DESMAXBURSTLEN_Msk                     /*!< DESMAXBURSTLEN[ 3:0] bits Destination Max Burst Length. Hint for the DMA on what is the maximum allowed burst size it can use for write transfers. The maximum number of beats sent by the DMA for a write burst is equal to DESMAXBURSTLEN + 1. Default value is 16 beats, which allows the DMA to set all burst sizes. Note: Limited by the DATA_BUFF_SIZE so larger settings may not always result in larger bursts.*/
#define DMA350_CH_DESTRANSCFG_DESMAXBURSTLEN_0         (0x1UL << DMA350_CH_DESTRANSCFG_DESMAXBURSTLEN_Pos)          /*!< 0x00010000UL*/
#define DMA350_CH_DESTRANSCFG_DESMAXBURSTLEN_1         (0x2UL << DMA350_CH_DESTRANSCFG_DESMAXBURSTLEN_Pos)          /*!< 0x00020000UL*/
#define DMA350_CH_DESTRANSCFG_DESMAXBURSTLEN_2         (0x4UL << DMA350_CH_DESTRANSCFG_DESMAXBURSTLEN_Pos)          /*!< 0x00040000UL*/
#define DMA350_CH_DESTRANSCFG_DESMAXBURSTLEN_3         (0x8UL << DMA350_CH_DESTRANSCFG_DESMAXBURSTLEN_Pos)          /*!< 0x00080000UL*/
/****************  Field definitions for CH_XADDRINC register  ****************/
#define DMA350_CH_XADDRINC_SRCXADDRINC_Pos             (0U)
#define DMA350_CH_XADDRINC_SRCXADDRINC_Msk             (0xFFFFUL << DMA350_CH_XADDRINC_SRCXADDRINC_Pos)             /*!< 0x0000FFFFUL*/
#define DMA350_CH_XADDRINC_SRCXADDRINC                 DMA350_CH_XADDRINC_SRCXADDRINC_Msk                           /*!< SRCXADDRINC[15:0] bits Source X dimension Address Increment. This value is used as the increment between each TRANSIZE transfer. When a single bit is used then only 0 and 1 can be set. For wider increment registers, two's complement used with a range between -32768 to 32767 when the counter is 16-bits wide. The width of the register is indicated by the INC_WIDTH parameter. SRCADDR_next = SRCADDR + 2^TRANSIZE * SRCXADDRINC*/
#define DMA350_CH_XADDRINC_DESXADDRINC_Pos             (16U)
#define DMA350_CH_XADDRINC_DESXADDRINC_Msk             (0xFFFFUL << DMA350_CH_XADDRINC_DESXADDRINC_Pos)             /*!< 0xFFFF0000UL*/
#define DMA350_CH_XADDRINC_DESXADDRINC                 DMA350_CH_XADDRINC_DESXADDRINC_Msk                           /*!< DESXADDRINC[15:0] bits Destination X dimension Address Increment. This value is used as the increment between each TRANSIZE transfer. When a single bit is used then only 0 and 1 can be set. For wider increment registers, two's complement used with a range between -32768 to 32767 when the counter is 16-bits wide. The width of the register is indicated by the INC_WIDTH parameter. DESADDR_next = DESADDR + 2^TRANSIZE * DESXADDRINC*/
/**************  Field definitions for CH_YADDRSTRIDE register  ***************/
#define DMA350_CH_YADDRSTRIDE_SRCYADDRSTRIDE_Pos       (0U)
#define DMA350_CH_YADDRSTRIDE_SRCYADDRSTRIDE_Msk       (0xFFFFUL << DMA350_CH_YADDRSTRIDE_SRCYADDRSTRIDE_Pos)       /*!< 0x0000FFFFUL*/
#define DMA350_CH_YADDRSTRIDE_SRCYADDRSTRIDE           DMA350_CH_YADDRSTRIDE_SRCYADDRSTRIDE_Msk                     /*!< SRCYADDRSTRIDE[15:0] bits Source Address Stride between lines. Calculated in TRANSIZE aligned steps. This value is used to increment the SRCADDR after completing the transfer of a source line. SRCADDR_next_line_base = SRCADDR_line_base + 2^TRANSIZE * SRCYADDRSTRIDE. Two's complement used with a range between -32768 to 32767. When set to 0 the SRCADDR is not incremented after completing one line. Not present when HAS_2D is 0.*/
#define DMA350_CH_YADDRSTRIDE_DESYADDRSTRIDE_Pos       (16U)
#define DMA350_CH_YADDRSTRIDE_DESYADDRSTRIDE_Msk       (0xFFFFUL << DMA350_CH_YADDRSTRIDE_DESYADDRSTRIDE_Pos)       /*!< 0xFFFF0000UL*/
#define DMA350_CH_YADDRSTRIDE_DESYADDRSTRIDE           DMA350_CH_YADDRSTRIDE_DESYADDRSTRIDE_Msk                     /*!< DESYADDRSTRIDE[15:0] bits Destination Address Stride between lines. Calculated in TRANSIZE aligned steps. This value is used to increment the DESADDR after completing the transfer of a destination line. DESADDR_next_line_base = DESADDR_line_base + 2^TRANSIZE * DESYADDRSTRIDE. Two's complement used with a range between -32768 to 32767. When set to 0 the DESADDR is not incremented after completing one line. Not present when HAS_2D is 0.*/
/****************  Field definitions for CH_FILLVAL register  *****************/
#define DMA350_CH_FILLVAL_FILLVAL_Pos                  (0U)
#define DMA350_CH_FILLVAL_FILLVAL_Msk                  (0xFFFFFFFFUL << DMA350_CH_FILLVAL_FILLVAL_Pos)              /*!< 0xFFFFFFFFUL*/
#define DMA350_CH_FILLVAL_FILLVAL                      DMA350_CH_FILLVAL_FILLVAL_Msk                                /*!< FILLVAL[31:0] bits Fill pattern value. When XTYPE or YTYPE is set to fill mode, then this register value is used on the write data bus when the command starts to fill the memory area. The TRANSIZE defines the width of the FILLVAL used for the command. For byte transfers the FILLVAL[7:0] is used, other bits are ignored. For halfword transfers the FILLVAL[15:0] is used, other bits are ignored. For 64-bit and wider transfers the FILLVAL[31:0] pattern is repeated on the full width of the data bus. Not present when HAS_WRAP is 0.*/
/*****************  Field definitions for CH_YSIZE register  ******************/
#define DMA350_CH_YSIZE_SRCYSIZE_Pos                   (0U)
#define DMA350_CH_YSIZE_SRCYSIZE_Msk                   (0xFFFFUL << DMA350_CH_YSIZE_SRCYSIZE_Pos)                   /*!< 0x0000FFFFUL*/
#define DMA350_CH_YSIZE_SRCYSIZE                       DMA350_CH_YSIZE_SRCYSIZE_Msk                                 /*!< SRCYSIZE[15:0] bits Source Y dimension or number of lines. Not present when HAS_2D is 0.*/
#define DMA350_CH_YSIZE_DESYSIZE_Pos                   (16U)
#define DMA350_CH_YSIZE_DESYSIZE_Msk                   (0xFFFFUL << DMA350_CH_YSIZE_DESYSIZE_Pos)                   /*!< 0xFFFF0000UL*/
#define DMA350_CH_YSIZE_DESYSIZE                       DMA350_CH_YSIZE_DESYSIZE_Msk                                 /*!< DESYSIZE[15:0] bits Destination Y dimension or number of lines. Not present when HAS_2D is 0. HAS_WRAP or HAS_STREAM configuration needs to be set to allow writes to this register, otherwise it is read-only.*/
/****************  Field definitions for CH_TMPLTCFG register  ****************/
#define DMA350_CH_TMPLTCFG_SRCTMPLTSIZE_Pos            (8U)
#define DMA350_CH_TMPLTCFG_SRCTMPLTSIZE_Msk            (0x1FUL << DMA350_CH_TMPLTCFG_SRCTMPLTSIZE_Pos)              /*!< 0x00001F00UL*/
#define DMA350_CH_TMPLTCFG_SRCTMPLTSIZE                DMA350_CH_TMPLTCFG_SRCTMPLTSIZE_Msk                          /*!< SRCTMPLTSIZE[ 4:0] bits Source Template Size in number of transfers plus one.
 - 0: Source template is disabled.
 - 1 to 31: Bits SRCTMPLT[SRCTMPLTSIZE:0] is used as the source template. Not present when HAS_TMPLT is 0.*/
#define DMA350_CH_TMPLTCFG_SRCTMPLTSIZE_0              (0x1UL << DMA350_CH_TMPLTCFG_SRCTMPLTSIZE_Pos)               /*!< 0x00000100UL*/
#define DMA350_CH_TMPLTCFG_SRCTMPLTSIZE_1              (0x2UL << DMA350_CH_TMPLTCFG_SRCTMPLTSIZE_Pos)               /*!< 0x00000200UL*/
#define DMA350_CH_TMPLTCFG_SRCTMPLTSIZE_2              (0x4UL << DMA350_CH_TMPLTCFG_SRCTMPLTSIZE_Pos)               /*!< 0x00000400UL*/
#define DMA350_CH_TMPLTCFG_SRCTMPLTSIZE_3              (0x8UL << DMA350_CH_TMPLTCFG_SRCTMPLTSIZE_Pos)               /*!< 0x00000800UL*/
#define DMA350_CH_TMPLTCFG_SRCTMPLTSIZE_4              (0x10UL << DMA350_CH_TMPLTCFG_SRCTMPLTSIZE_Pos)              /*!< 0x00001000UL*/
#define DMA350_CH_TMPLTCFG_DESTMPLTSIZE_Pos            (16U)
#define DMA350_CH_TMPLTCFG_DESTMPLTSIZE_Msk            (0x1FUL << DMA350_CH_TMPLTCFG_DESTMPLTSIZE_Pos)              /*!< 0x001F0000UL*/
#define DMA350_CH_TMPLTCFG_DESTMPLTSIZE                DMA350_CH_TMPLTCFG_DESTMPLTSIZE_Msk                          /*!< DESTMPLTSIZE[ 4:0] bits Destination Template Size in number of transfers plus one.
 - 0: Destination template is disabled.
 - 1 to 31: DESTMPLT[DESTMPLTSIZE:0] is used as the destination template. Not present when HAS_TMPLT is 0.*/
#define DMA350_CH_TMPLTCFG_DESTMPLTSIZE_0              (0x1UL << DMA350_CH_TMPLTCFG_DESTMPLTSIZE_Pos)               /*!< 0x00010000UL*/
#define DMA350_CH_TMPLTCFG_DESTMPLTSIZE_1              (0x2UL << DMA350_CH_TMPLTCFG_DESTMPLTSIZE_Pos)               /*!< 0x00020000UL*/
#define DMA350_CH_TMPLTCFG_DESTMPLTSIZE_2              (0x4UL << DMA350_CH_TMPLTCFG_DESTMPLTSIZE_Pos)               /*!< 0x00040000UL*/
#define DMA350_CH_TMPLTCFG_DESTMPLTSIZE_3              (0x8UL << DMA350_CH_TMPLTCFG_DESTMPLTSIZE_Pos)               /*!< 0x00080000UL*/
#define DMA350_CH_TMPLTCFG_DESTMPLTSIZE_4              (0x10UL << DMA350_CH_TMPLTCFG_DESTMPLTSIZE_Pos)              /*!< 0x00100000UL*/
/****************  Field definitions for CH_SRCTMPLT register  ****************/
#define DMA350_CH_SRCTMPLT_SRCTMPLTLSB_Pos             (0U)
#define DMA350_CH_SRCTMPLT_SRCTMPLTLSB_Msk             (0x1UL << DMA350_CH_SRCTMPLT_SRCTMPLTLSB_Pos)                /*!< 0x00000001UL*/
#define DMA350_CH_SRCTMPLT_SRCTMPLTLSB                 DMA350_CH_SRCTMPLT_SRCTMPLTLSB_Msk                           /*!< SRCTMPLTLSB bit Source Packing Template Least Significant Bit. This bit of the template is read only and always set to 1 as template patterns can only start from the base address of the transfer. Not present when HAS_TMPLT is 0.*/
#define DMA350_CH_SRCTMPLT_SRCTMPLT_Pos                (1U)
#define DMA350_CH_SRCTMPLT_SRCTMPLT_Msk                (0x7FFFFFFFUL << DMA350_CH_SRCTMPLT_SRCTMPLT_Pos)            /*!< 0xFFFFFFFEUL*/
#define DMA350_CH_SRCTMPLT_SRCTMPLT                    DMA350_CH_SRCTMPLT_SRCTMPLT_Msk                              /*!< SRCTMPLT[30:0] bits Source Packing Template. Bit[0] is read only and always set to 1 as template patterns can only start from the base address of the transfer. Not present when HAS_TMPLT is 0.*/
/****************  Field definitions for CH_DESTMPLT register  ****************/
#define DMA350_CH_DESTMPLT_DESTMPLTLSB_Pos             (0U)
#define DMA350_CH_DESTMPLT_DESTMPLTLSB_Msk             (0x1UL << DMA350_CH_DESTMPLT_DESTMPLTLSB_Pos)                /*!< 0x00000001UL*/
#define DMA350_CH_DESTMPLT_DESTMPLTLSB                 DMA350_CH_DESTMPLT_DESTMPLTLSB_Msk                           /*!< DESTMPLTLSB bit Destination Packing Template Least Significant Bit. This bit of the template is read only and always set to 1 as template patterns can only start from the base address of the transfer. Not present when HAS_TMPLT is 0.*/
#define DMA350_CH_DESTMPLT_DESTMPLT_Pos                (1U)
#define DMA350_CH_DESTMPLT_DESTMPLT_Msk                (0x7FFFFFFFUL << DMA350_CH_DESTMPLT_DESTMPLT_Pos)            /*!< 0xFFFFFFFEUL*/
#define DMA350_CH_DESTMPLT_DESTMPLT                    DMA350_CH_DESTMPLT_DESTMPLT_Msk                              /*!< DESTMPLT[30:0] bits Destination Packing Template.  Bit[0] is read only and always set to 1 as template patterns can only start from the base address of the transfer. Not present when HAS_TMPLT is 0.*/
/**************  Field definitions for CH_SRCTRIGINCFG register  **************/
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINSEL_Pos        (0U)
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINSEL_Msk        (0xFFUL << DMA350_CH_SRCTRIGINCFG_SRCTRIGINSEL_Pos)          /*!< 0x000000FFUL*/
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINSEL            DMA350_CH_SRCTRIGINCFG_SRCTRIGINSEL_Msk                      /*!< SRCTRIGINSEL[ 7:0] bits Source Trigger Input Select*/
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINSEL_0          (0x1UL << DMA350_CH_SRCTRIGINCFG_SRCTRIGINSEL_Pos)           /*!< 0x00000001UL*/
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINSEL_1          (0x2UL << DMA350_CH_SRCTRIGINCFG_SRCTRIGINSEL_Pos)           /*!< 0x00000002UL*/
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINSEL_2          (0x4UL << DMA350_CH_SRCTRIGINCFG_SRCTRIGINSEL_Pos)           /*!< 0x00000004UL*/
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINSEL_3          (0x8UL << DMA350_CH_SRCTRIGINCFG_SRCTRIGINSEL_Pos)           /*!< 0x00000008UL*/
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINSEL_4          (0x10UL << DMA350_CH_SRCTRIGINCFG_SRCTRIGINSEL_Pos)          /*!< 0x00000010UL*/
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINSEL_5          (0x20UL << DMA350_CH_SRCTRIGINCFG_SRCTRIGINSEL_Pos)          /*!< 0x00000020UL*/
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINSEL_6          (0x40UL << DMA350_CH_SRCTRIGINCFG_SRCTRIGINSEL_Pos)          /*!< 0x00000040UL*/
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINSEL_7          (0x80UL << DMA350_CH_SRCTRIGINCFG_SRCTRIGINSEL_Pos)          /*!< 0x00000080UL*/
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINTYPE_Pos       (8U)
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINTYPE_Msk       (0x3UL << DMA350_CH_SRCTRIGINCFG_SRCTRIGINTYPE_Pos)          /*!< 0x00000300UL*/
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINTYPE           DMA350_CH_SRCTRIGINCFG_SRCTRIGINTYPE_Msk                     /*!< SRCTRIGINTYPE[ 1:0] bits Source Trigger Input Type:
 - 00: Software only Trigger Request. SRCTRIGINSEL is ignored.
 - 01: Reserved
 - 10: HW Trigger Request. Only allowed when HAS_TRIGIN is enabled. SRCTRIGINSEL selects between external trigger inputs if HAS_TRIGSEL is enabled.
 - 11: Internal Trigger Request. Only allowed when HAS_TRIGSEL is enabled and the DMAC has multiple channels, otherwise treated as HW Trigger Request. SRCTRIGINSEL selects between DMA channels.
Note: SW triggers are also available when HW or Internal types are selected, but is is not recommended and caution must be taken when the these modes are combined.*/
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINTYPE_0         (0x1UL << DMA350_CH_SRCTRIGINCFG_SRCTRIGINTYPE_Pos)          /*!< 0x00000100UL*/
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINTYPE_1         (0x2UL << DMA350_CH_SRCTRIGINCFG_SRCTRIGINTYPE_Pos)          /*!< 0x00000200UL*/
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINMODE_Pos       (10U)
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINMODE_Msk       (0x3UL << DMA350_CH_SRCTRIGINCFG_SRCTRIGINMODE_Pos)          /*!< 0x00000C00UL*/
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINMODE           DMA350_CH_SRCTRIGINCFG_SRCTRIGINMODE_Msk                     /*!< SRCTRIGINMODE[ 1:0] bits Source Trigger Input Mode:
 - 00: Command
 - 01: Reserved
 - 10: DMA driven Flow control. Only allowed when HAS_TRIGIN is enabled.
 - 11: Peripheral driven Flow control. Only allowed when HAS_TRIGIN is enabled.
Note: This field is ignored for Internal triggers as they only support Command triggers.*/
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINMODE_0         (0x1UL << DMA350_CH_SRCTRIGINCFG_SRCTRIGINMODE_Pos)          /*!< 0x00000400UL*/
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINMODE_1         (0x2UL << DMA350_CH_SRCTRIGINCFG_SRCTRIGINMODE_Pos)          /*!< 0x00000800UL*/
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINBLKSIZE_Pos    (16U)
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINBLKSIZE_Msk    (0xFFUL << DMA350_CH_SRCTRIGINCFG_SRCTRIGINBLKSIZE_Pos)      /*!< 0x00FF0000UL*/
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINBLKSIZE        DMA350_CH_SRCTRIGINCFG_SRCTRIGINBLKSIZE_Msk                  /*!< SRCTRIGINBLKSIZE[ 7:0] bits Source Trigger Input Default Transfer Size. Defined transfer size per trigger + 1.*/
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINBLKSIZE_0      (0x1UL << DMA350_CH_SRCTRIGINCFG_SRCTRIGINBLKSIZE_Pos)       /*!< 0x00010000UL*/
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINBLKSIZE_1      (0x2UL << DMA350_CH_SRCTRIGINCFG_SRCTRIGINBLKSIZE_Pos)       /*!< 0x00020000UL*/
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINBLKSIZE_2      (0x4UL << DMA350_CH_SRCTRIGINCFG_SRCTRIGINBLKSIZE_Pos)       /*!< 0x00040000UL*/
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINBLKSIZE_3      (0x8UL << DMA350_CH_SRCTRIGINCFG_SRCTRIGINBLKSIZE_Pos)       /*!< 0x00080000UL*/
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINBLKSIZE_4      (0x10UL << DMA350_CH_SRCTRIGINCFG_SRCTRIGINBLKSIZE_Pos)      /*!< 0x00100000UL*/
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINBLKSIZE_5      (0x20UL << DMA350_CH_SRCTRIGINCFG_SRCTRIGINBLKSIZE_Pos)      /*!< 0x00200000UL*/
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINBLKSIZE_6      (0x40UL << DMA350_CH_SRCTRIGINCFG_SRCTRIGINBLKSIZE_Pos)      /*!< 0x00400000UL*/
#define DMA350_CH_SRCTRIGINCFG_SRCTRIGINBLKSIZE_7      (0x80UL << DMA350_CH_SRCTRIGINCFG_SRCTRIGINBLKSIZE_Pos)      /*!< 0x00800000UL*/
/**************  Field definitions for CH_DESTRIGINCFG register  **************/
#define DMA350_CH_DESTRIGINCFG_DESTRIGINSEL_Pos        (0U)
#define DMA350_CH_DESTRIGINCFG_DESTRIGINSEL_Msk        (0xFFUL << DMA350_CH_DESTRIGINCFG_DESTRIGINSEL_Pos)          /*!< 0x000000FFUL*/
#define DMA350_CH_DESTRIGINCFG_DESTRIGINSEL            DMA350_CH_DESTRIGINCFG_DESTRIGINSEL_Msk                      /*!< DESTRIGINSEL[ 7:0] bits Destination Trigger Input Select*/
#define DMA350_CH_DESTRIGINCFG_DESTRIGINSEL_0          (0x1UL << DMA350_CH_DESTRIGINCFG_DESTRIGINSEL_Pos)           /*!< 0x00000001UL*/
#define DMA350_CH_DESTRIGINCFG_DESTRIGINSEL_1          (0x2UL << DMA350_CH_DESTRIGINCFG_DESTRIGINSEL_Pos)           /*!< 0x00000002UL*/
#define DMA350_CH_DESTRIGINCFG_DESTRIGINSEL_2          (0x4UL << DMA350_CH_DESTRIGINCFG_DESTRIGINSEL_Pos)           /*!< 0x00000004UL*/
#define DMA350_CH_DESTRIGINCFG_DESTRIGINSEL_3          (0x8UL << DMA350_CH_DESTRIGINCFG_DESTRIGINSEL_Pos)           /*!< 0x00000008UL*/
#define DMA350_CH_DESTRIGINCFG_DESTRIGINSEL_4          (0x10UL << DMA350_CH_DESTRIGINCFG_DESTRIGINSEL_Pos)          /*!< 0x00000010UL*/
#define DMA350_CH_DESTRIGINCFG_DESTRIGINSEL_5          (0x20UL << DMA350_CH_DESTRIGINCFG_DESTRIGINSEL_Pos)          /*!< 0x00000020UL*/
#define DMA350_CH_DESTRIGINCFG_DESTRIGINSEL_6          (0x40UL << DMA350_CH_DESTRIGINCFG_DESTRIGINSEL_Pos)          /*!< 0x00000040UL*/
#define DMA350_CH_DESTRIGINCFG_DESTRIGINSEL_7          (0x80UL << DMA350_CH_DESTRIGINCFG_DESTRIGINSEL_Pos)          /*!< 0x00000080UL*/
#define DMA350_CH_DESTRIGINCFG_DESTRIGINTYPE_Pos       (8U)
#define DMA350_CH_DESTRIGINCFG_DESTRIGINTYPE_Msk       (0x3UL << DMA350_CH_DESTRIGINCFG_DESTRIGINTYPE_Pos)          /*!< 0x00000300UL*/
#define DMA350_CH_DESTRIGINCFG_DESTRIGINTYPE           DMA350_CH_DESTRIGINCFG_DESTRIGINTYPE_Msk                     /*!< DESTRIGINTYPE[ 1:0] bits Destination Trigger Input Type:
 - 00: Software only Trigger Request. DESTRIGINSEL is ignored.
 - 01: Reserved
 - 10: HW Trigger Request. Only allowed when HAS_TRIGIN is enabled. DESTRIGINSEL selects between external trigger inputs if HAS_TRIGSEL is enabled.
 - 11: Internal Trigger Request. Only allowed when HAS_TRIGSEL is enabled and the DMAC has multiple channels, otherwise treated as HW Trigger Request. DESTRIGINSEL selects between DMA channels.
Note: SW triggers are also available when HW or Internal types are selected, but is is not recommended and caution must be taken when the these modes are combined.*/
#define DMA350_CH_DESTRIGINCFG_DESTRIGINTYPE_0         (0x1UL << DMA350_CH_DESTRIGINCFG_DESTRIGINTYPE_Pos)          /*!< 0x00000100UL*/
#define DMA350_CH_DESTRIGINCFG_DESTRIGINTYPE_1         (0x2UL << DMA350_CH_DESTRIGINCFG_DESTRIGINTYPE_Pos)          /*!< 0x00000200UL*/
#define DMA350_CH_DESTRIGINCFG_DESTRIGINMODE_Pos       (10U)
#define DMA350_CH_DESTRIGINCFG_DESTRIGINMODE_Msk       (0x3UL << DMA350_CH_DESTRIGINCFG_DESTRIGINMODE_Pos)          /*!< 0x00000C00UL*/
#define DMA350_CH_DESTRIGINCFG_DESTRIGINMODE           DMA350_CH_DESTRIGINCFG_DESTRIGINMODE_Msk                     /*!< DESTRIGINMODE[ 1:0] bits Destination Trigger Input Mode:
 - 00: Command
 - 01: Reserved
 - 10: DMA driven Flow control. Only allowed when HAS_TRIGIN is enabled.
 - 11: Peripheral driven Flow control. Only allowed when HAS_TRIGIN is enabled.
Note: This field is ignored for Internal triggers as they only support Command triggers.*/
#define DMA350_CH_DESTRIGINCFG_DESTRIGINMODE_0         (0x1UL << DMA350_CH_DESTRIGINCFG_DESTRIGINMODE_Pos)          /*!< 0x00000400UL*/
#define DMA350_CH_DESTRIGINCFG_DESTRIGINMODE_1         (0x2UL << DMA350_CH_DESTRIGINCFG_DESTRIGINMODE_Pos)          /*!< 0x00000800UL*/
#define DMA350_CH_DESTRIGINCFG_DESTRIGINBLKSIZE_Pos    (16U)
#define DMA350_CH_DESTRIGINCFG_DESTRIGINBLKSIZE_Msk    (0xFFUL << DMA350_CH_DESTRIGINCFG_DESTRIGINBLKSIZE_Pos)      /*!< 0x00FF0000UL*/
#define DMA350_CH_DESTRIGINCFG_DESTRIGINBLKSIZE        DMA350_CH_DESTRIGINCFG_DESTRIGINBLKSIZE_Msk                  /*!< DESTRIGINBLKSIZE[ 7:0] bits Destination Trigger Input Default Transfer Size. Defined transfer size per trigger + 1.*/
#define DMA350_CH_DESTRIGINCFG_DESTRIGINBLKSIZE_0      (0x1UL << DMA350_CH_DESTRIGINCFG_DESTRIGINBLKSIZE_Pos)       /*!< 0x00010000UL*/
#define DMA350_CH_DESTRIGINCFG_DESTRIGINBLKSIZE_1      (0x2UL << DMA350_CH_DESTRIGINCFG_DESTRIGINBLKSIZE_Pos)       /*!< 0x00020000UL*/
#define DMA350_CH_DESTRIGINCFG_DESTRIGINBLKSIZE_2      (0x4UL << DMA350_CH_DESTRIGINCFG_DESTRIGINBLKSIZE_Pos)       /*!< 0x00040000UL*/
#define DMA350_CH_DESTRIGINCFG_DESTRIGINBLKSIZE_3      (0x8UL << DMA350_CH_DESTRIGINCFG_DESTRIGINBLKSIZE_Pos)       /*!< 0x00080000UL*/
#define DMA350_CH_DESTRIGINCFG_DESTRIGINBLKSIZE_4      (0x10UL << DMA350_CH_DESTRIGINCFG_DESTRIGINBLKSIZE_Pos)      /*!< 0x00100000UL*/
#define DMA350_CH_DESTRIGINCFG_DESTRIGINBLKSIZE_5      (0x20UL << DMA350_CH_DESTRIGINCFG_DESTRIGINBLKSIZE_Pos)      /*!< 0x00200000UL*/
#define DMA350_CH_DESTRIGINCFG_DESTRIGINBLKSIZE_6      (0x40UL << DMA350_CH_DESTRIGINCFG_DESTRIGINBLKSIZE_Pos)      /*!< 0x00400000UL*/
#define DMA350_CH_DESTRIGINCFG_DESTRIGINBLKSIZE_7      (0x80UL << DMA350_CH_DESTRIGINCFG_DESTRIGINBLKSIZE_Pos)      /*!< 0x00800000UL*/
/***************  Field definitions for CH_TRIGOUTCFG register  ***************/
#define DMA350_CH_TRIGOUTCFG_TRIGOUTSEL_Pos            (0U)
#define DMA350_CH_TRIGOUTCFG_TRIGOUTSEL_Msk            (0x3FUL << DMA350_CH_TRIGOUTCFG_TRIGOUTSEL_Pos)              /*!< 0x0000003FUL*/
#define DMA350_CH_TRIGOUTCFG_TRIGOUTSEL                DMA350_CH_TRIGOUTCFG_TRIGOUTSEL_Msk                          /*!< TRIGOUTSEL[ 5:0] bits Trigger Output Select*/
#define DMA350_CH_TRIGOUTCFG_TRIGOUTSEL_0              (0x1UL << DMA350_CH_TRIGOUTCFG_TRIGOUTSEL_Pos)               /*!< 0x00000001UL*/
#define DMA350_CH_TRIGOUTCFG_TRIGOUTSEL_1              (0x2UL << DMA350_CH_TRIGOUTCFG_TRIGOUTSEL_Pos)               /*!< 0x00000002UL*/
#define DMA350_CH_TRIGOUTCFG_TRIGOUTSEL_2              (0x4UL << DMA350_CH_TRIGOUTCFG_TRIGOUTSEL_Pos)               /*!< 0x00000004UL*/
#define DMA350_CH_TRIGOUTCFG_TRIGOUTSEL_3              (0x8UL << DMA350_CH_TRIGOUTCFG_TRIGOUTSEL_Pos)               /*!< 0x00000008UL*/
#define DMA350_CH_TRIGOUTCFG_TRIGOUTSEL_4              (0x10UL << DMA350_CH_TRIGOUTCFG_TRIGOUTSEL_Pos)              /*!< 0x00000010UL*/
#define DMA350_CH_TRIGOUTCFG_TRIGOUTSEL_5              (0x20UL << DMA350_CH_TRIGOUTCFG_TRIGOUTSEL_Pos)              /*!< 0x00000020UL*/
#define DMA350_CH_TRIGOUTCFG_TRIGOUTTYPE_Pos           (8U)
#define DMA350_CH_TRIGOUTCFG_TRIGOUTTYPE_Msk           (0x3UL << DMA350_CH_TRIGOUTCFG_TRIGOUTTYPE_Pos)              /*!< 0x00000300UL*/
#define DMA350_CH_TRIGOUTCFG_TRIGOUTTYPE               DMA350_CH_TRIGOUTCFG_TRIGOUTTYPE_Msk                         /*!< TRIGOUTTYPE[ 1:0] bits Trigger Output Type
 - 00: Software only Trigger Acknowledgement.
 - 01: Reserved
 - 10: HW Trigger Acknowledgement. Only allowed when HAS_TRIGOUT is enabled.
 - 11: Internal Trigger Acknowledgement. Only allowed when HAS_TRIGSEL is enabled and the DMAC has multiple channels, otherwise treated as HW Trigger Acknowledgement.*/
#define DMA350_CH_TRIGOUTCFG_TRIGOUTTYPE_0             (0x1UL << DMA350_CH_TRIGOUTCFG_TRIGOUTTYPE_Pos)              /*!< 0x00000100UL*/
#define DMA350_CH_TRIGOUTCFG_TRIGOUTTYPE_1             (0x2UL << DMA350_CH_TRIGOUTCFG_TRIGOUTTYPE_Pos)              /*!< 0x00000200UL*/
/*****************  Field definitions for CH_GPOEN0 register  *****************/
#define DMA350_CH_GPOEN0_GPOEN0_Pos                    (0U)
#define DMA350_CH_GPOEN0_GPOEN0_Msk                    (0xFFFFFFFFUL << DMA350_CH_GPOEN0_GPOEN0_Pos)                /*!< 0xFFFFFFFFUL*/
#define DMA350_CH_GPOEN0_GPOEN0                        DMA350_CH_GPOEN0_GPOEN0_Msk                                  /*!< GPOEN0[31:0] bits Channel General Purpose Output (GPO) bit 0 to 31 enable mask. If bit n is '1', then GPO[n] is selected for driving by GPOVAL0[n]. If bit 'n' is '0', then GPO[n] keeps its previous value. Only [GPO_WIDTH-1:0] are implemented. All unimplemented bits are RAZWI.*/
/****************  Field definitions for CH_GPOVAL0 register  *****************/
#define DMA350_CH_GPOVAL0_GPOVAL0_Pos                  (0U)
#define DMA350_CH_GPOVAL0_GPOVAL0_Msk                  (0xFFFFFFFFUL << DMA350_CH_GPOVAL0_GPOVAL0_Pos)              /*!< 0xFFFFFFFFUL*/
#define DMA350_CH_GPOVAL0_GPOVAL0                      DMA350_CH_GPOVAL0_GPOVAL0_Msk                                /*!< GPOVAL0[31:0] bits General Purpose Output Value GPO[31:0]. Write to set output value. The actual value on the GPO port will become active when the command is enabled. Read returns the register value which might be different from the actual GPO port status. Only [GPO_WIDTH-1:0] are implemented. All unimplemented bits are RAZWI.*/
/**************  Field definitions for CH_STREAMINTCFG register  **************/
#define DMA350_CH_STREAMINTCFG_STREAMTYPE_Pos          (9U)
#define DMA350_CH_STREAMINTCFG_STREAMTYPE_Msk          (0x3UL << DMA350_CH_STREAMINTCFG_STREAMTYPE_Pos)             /*!< 0x00000600UL*/
#define DMA350_CH_STREAMINTCFG_STREAMTYPE              DMA350_CH_STREAMINTCFG_STREAMTYPE_Msk                        /*!< STREAMTYPE[ 1:0] bits Stream Interface operation Type
 - 00: Stream in and out used.
 - 01: Stream out only
 - 10: Stream in only
 - 11: Reserved*/
#define DMA350_CH_STREAMINTCFG_STREAMTYPE_0            (0x1UL << DMA350_CH_STREAMINTCFG_STREAMTYPE_Pos)             /*!< 0x00000200UL*/
#define DMA350_CH_STREAMINTCFG_STREAMTYPE_1            (0x2UL << DMA350_CH_STREAMINTCFG_STREAMTYPE_Pos)             /*!< 0x00000400UL*/
/****************  Field definitions for CH_LINKATTR register  ****************/
#define DMA350_CH_LINKATTR_LINKMEMATTRLO_Pos           (0U)
#define DMA350_CH_LINKATTR_LINKMEMATTRLO_Msk           (0xFUL << DMA350_CH_LINKATTR_LINKMEMATTRLO_Pos)              /*!< 0x0000000FUL*/
#define DMA350_CH_LINKATTR_LINKMEMATTRLO               DMA350_CH_LINKATTR_LINKMEMATTRLO_Msk                         /*!< LINKMEMATTRLO[ 3:0] bits Link Address Read Transfer Memory Attribute field [3:0].
When LINKMEMATTRHI is Device type (0000) then this field means:
 - 0000: Device-nGnRnE
 - 0100: Device-nGnRE
 - 1000: Device-nGRE
 - 1100: Device-GRE
 - Others: Invalid resulting in UNPREDICTABLE behavior
When LINKMEMATTRHI is Normal memory type (other than 0000) then this field means:
 - 0000: Reserved
 - 0001: Normal memory, Inner Write allocate, Inner Write-through transient
 - 0010: Normal memory, Inner Read allocate, Inner Write-through transient
 - 0011: Normal memory, Inner Read/Write allocate, Inner Write-through transient
 - 0100: Normal memory, Inner non-cacheable
 - 0101: Normal memory, Inner Write allocate, Inner Write-back transient
 - 0110: Normal memory, Inner Read allocate, Inner Write-back transient
 - 0111: Normal memory, Inner Read/Write allocate, Inner Write-back transient
 - 1000: Normal memory, Inner Write-through non-transient
 - 1001: Normal memory, Inner Write allocate, Inner Write-through non-transient
 - 1010: Normal memory, Inner Read allocate, Inner Write-through non-transient
 - 1011: Normal memory, Inner Read/Write allocate, Inner Write-through non-transient
 - 1100: Normal memory, Inner Write-back non-transient
 - 1101: Normal memory, Inner Write allocate, Inner Write-back non-transient
 - 1110: Normal memory, Inner Read allocate, Inner Write-back non-transient
 - 1111: Normal memory, Inner Read/Write allocate, Inner Write-back non-transient*/
#define DMA350_CH_LINKATTR_LINKMEMATTRLO_0             (0x1UL << DMA350_CH_LINKATTR_LINKMEMATTRLO_Pos)              /*!< 0x00000001UL*/
#define DMA350_CH_LINKATTR_LINKMEMATTRLO_1             (0x2UL << DMA350_CH_LINKATTR_LINKMEMATTRLO_Pos)              /*!< 0x00000002UL*/
#define DMA350_CH_LINKATTR_LINKMEMATTRLO_2             (0x4UL << DMA350_CH_LINKATTR_LINKMEMATTRLO_Pos)              /*!< 0x00000004UL*/
#define DMA350_CH_LINKATTR_LINKMEMATTRLO_3             (0x8UL << DMA350_CH_LINKATTR_LINKMEMATTRLO_Pos)              /*!< 0x00000008UL*/
#define DMA350_CH_LINKATTR_LINKMEMATTRHI_Pos           (4U)
#define DMA350_CH_LINKATTR_LINKMEMATTRHI_Msk           (0xFUL << DMA350_CH_LINKATTR_LINKMEMATTRHI_Pos)              /*!< 0x000000F0UL*/
#define DMA350_CH_LINKATTR_LINKMEMATTRHI               DMA350_CH_LINKATTR_LINKMEMATTRHI_Msk                         /*!< LINKMEMATTRHI[ 3:0] bits Link Address Read Transfer Memory Attribute field [7:4].
 - 0000: Device memory
 - 0001: Normal memory, Outer Write allocate, Outer Write-through transient
 - 0010: Normal memory, Outer Read allocate, Outer Write-through transient
 - 0011: Normal memory, Outer Read/Write allocate, Outer Write-through transient
 - 0100: Normal memory, Outer non-cacheable
 - 0101: Normal memory, Outer Write allocate, Outer Write-back transient
 - 0110: Normal memory, Outer Read allocate, Outer Write-back transient
 - 0111: Normal memory, Outer Read/Write allocate, Outer Write-back transient
 - 1000: Normal memory, Outer Write-through non-transient
 - 1001: Normal memory, Outer Write allocate, Outer Write-through non-transient
 - 1010: Normal memory, Outer Read allocate, Outer Write-through non-transient
 - 1011: Normal memory, Outer Read/Write allocate, Outer Write-through non-transient
 - 1100: Normal memory, Outer Write-back non-transient
 - 1101: Normal memory, Outer Write allocate, Outer Write-back non-transient
 - 1110: Normal memory, Outer Read allocate, Outer Write-back non-transient
 - 1111: Normal memory, Outer Read/Write allocate, Outer Write-back non-transient*/
#define DMA350_CH_LINKATTR_LINKMEMATTRHI_0             (0x1UL << DMA350_CH_LINKATTR_LINKMEMATTRHI_Pos)              /*!< 0x00000010UL*/
#define DMA350_CH_LINKATTR_LINKMEMATTRHI_1             (0x2UL << DMA350_CH_LINKATTR_LINKMEMATTRHI_Pos)              /*!< 0x00000020UL*/
#define DMA350_CH_LINKATTR_LINKMEMATTRHI_2             (0x4UL << DMA350_CH_LINKATTR_LINKMEMATTRHI_Pos)              /*!< 0x00000040UL*/
#define DMA350_CH_LINKATTR_LINKMEMATTRHI_3             (0x8UL << DMA350_CH_LINKATTR_LINKMEMATTRHI_Pos)              /*!< 0x00000080UL*/
#define DMA350_CH_LINKATTR_LINKSHAREATTR_Pos           (8U)
#define DMA350_CH_LINKATTR_LINKSHAREATTR_Msk           (0x3UL << DMA350_CH_LINKATTR_LINKSHAREATTR_Pos)              /*!< 0x00000300UL*/
#define DMA350_CH_LINKATTR_LINKSHAREATTR               DMA350_CH_LINKATTR_LINKSHAREATTR_Msk                         /*!< LINKSHAREATTR[ 1:0] bits Link Address Transfer Shareability Attribute.
 - 00: Non-shareable
 - 01: Reserved
 - 10: Outer shareable
 - 11: Inner shareable */
#define DMA350_CH_LINKATTR_LINKSHAREATTR_0             (0x1UL << DMA350_CH_LINKATTR_LINKSHAREATTR_Pos)              /*!< 0x00000100UL*/
#define DMA350_CH_LINKATTR_LINKSHAREATTR_1             (0x2UL << DMA350_CH_LINKATTR_LINKSHAREATTR_Pos)              /*!< 0x00000200UL*/
/****************  Field definitions for CH_AUTOCFG register  *****************/
#define DMA350_CH_AUTOCFG_CMDRESTARTCNT_Pos            (0U)
#define DMA350_CH_AUTOCFG_CMDRESTARTCNT_Msk            (0xFFFFUL << DMA350_CH_AUTOCFG_CMDRESTARTCNT_Pos)            /*!< 0x0000FFFFUL*/
#define DMA350_CH_AUTOCFG_CMDRESTARTCNT                DMA350_CH_AUTOCFG_CMDRESTARTCNT_Msk                          /*!< CMDRESTARTCNT[15:0] bits Automatic Command Restart Counter. Defines the number of times automatic restarting will occur at end of DMA command. Auto restarting will occur after the command is completed, including output triggering if enabled and autoreloading the registers, but it will only perfrom a link to the next command when CMDRESTARTCNT == 0. When CMDRESTARTCNT and CMDRESTARTINF are both set to '0', autorestart is disabled.*/
#define DMA350_CH_AUTOCFG_CMDRESTARTINFEN_Pos          (16U)
#define DMA350_CH_AUTOCFG_CMDRESTARTINFEN_Msk          (0x1UL << DMA350_CH_AUTOCFG_CMDRESTARTINFEN_Pos)             /*!< 0x00010000UL*/
#define DMA350_CH_AUTOCFG_CMDRESTARTINFEN              DMA350_CH_AUTOCFG_CMDRESTARTINFEN_Msk                        /*!< CMDRESTARTINFEN bit Enable Infinite Automatic Command Restart. When set, CMDRESTARTCNT is ignored and the command is always restarted after it is completed, including output triggering if enabled and autoreloading the registers but it will not perform a link to the next command. This means that the infinite loop of automatic restarts can only be broken by DISABLECMD or STOPCMD. When CMDRESTARTINFEN is set to '0', then the autorestarting of a command depends on CMDRESTARTCNT and when that counter is set to 0 the autorestarting is finished. In this case the next linked command is read or the command is complete.*/
/****************  Field definitions for CH_LINKADDR register  ****************/
#define DMA350_CH_LINKADDR_LINKADDREN_Pos              (0U)
#define DMA350_CH_LINKADDR_LINKADDREN_Msk              (0x1UL << DMA350_CH_LINKADDR_LINKADDREN_Pos)                 /*!< 0x00000001UL*/
#define DMA350_CH_LINKADDR_LINKADDREN                  DMA350_CH_LINKADDR_LINKADDREN_Msk                            /*!< LINKADDREN bit Enable Link Address. When set to '1', the DMAC fetches the next command defined by LINKADDR. When set to '0' the DMAC will return to idle at the end of the current command.
NOTE: the linked command fetched by the DMAC needs to clear this field to mark the end of the command chain. Otherwise it may result in an infinite loop of the same command.
*/
#define DMA350_CH_LINKADDR_LINKADDR_Pos                (2U)
#define DMA350_CH_LINKADDR_LINKADDR_Msk                (0x3FFFFFFFUL << DMA350_CH_LINKADDR_LINKADDR_Pos)            /*!< 0xFFFFFFFCUL*/
#define DMA350_CH_LINKADDR_LINKADDR                    DMA350_CH_LINKADDR_LINKADDR_Msk                              /*!< LINKADDR[29:0] bits Link Address Pointer [31:2]. The DMAC fetches the next command from this address if LINKADDREN is set.
NOTE: Commands are fetched with the security and privilege attribute of the channel and cannot be adjusted for the command link reads.
*/
/***************  Field definitions for CH_LINKADDRHI register  ***************/
#define DMA350_CH_LINKADDRHI_LINKADDRHI_Pos            (0U)
#define DMA350_CH_LINKADDRHI_LINKADDRHI_Msk            (0xFFFFFFFFUL << DMA350_CH_LINKADDRHI_LINKADDRHI_Pos)        /*!< 0xFFFFFFFFUL*/
#define DMA350_CH_LINKADDRHI_LINKADDRHI                DMA350_CH_LINKADDRHI_LINKADDRHI_Msk                          /*!< LINKADDRHI[31:0] bits Link Address Pointer [63:32]. Allows 64-bit addressing but the system might need less address bits. Limited by ADDR_WIDTH and the not implemented bits remain reserved.*/

/***************  Field definitions for Non-Secure Control register  ***************/
#define INTREN_ANYCHINTR_ENABLE                     (1U)


#endif /* __DMA350_REGDEF_H */
