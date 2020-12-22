/*
 *  arch/arm/mach-sunxi/arisc/include/arisc_messages.h
 *
 * Copyright (c) 2012 Allwinner.
 * 2012-05-01 Written by sunny (sunny@allwinnertech.com).
 * 2012-10-01 Written by superm (superm@allwinnertech.com).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#ifndef __ARISC_MESSAGES_H__
#define __ARISC_MESSAGES_H__

#include <linux/arisc/arisc.h>

/* message states */
#define ARISC_MESSAGE_FREED         (0x0)   /* freed state       */
#define ARISC_MESSAGE_ALLOCATED     (0x1)   /* allocated state   */
#define ARISC_MESSAGE_INITIALIZED   (0x2)   /* initialized state */
#define ARISC_MESSAGE_RECEIVED      (0x3)   /* received state    */
#define ARISC_MESSAGE_PROCESSING    (0x4)   /* processing state  */
#define ARISC_MESSAGE_PROCESSED     (0x5)   /* processed state   */
#define ARISC_MESSAGE_FEEDBACKED    (0x6)   /* feedback state    */

/* call back struct */
typedef struct arisc_msg_cb
{
	arisc_cb_t   handler;
	void        *arg;
} arisc_msg_cb_t;

/*
 * the structure of message frame,
 * this structure will transfer between arisc and ac327.
 * sizeof(struct message) : 64Byte.
 */
typedef struct arisc_message
{
	volatile unsigned char           state;     /* identify the used status of message frame */
	volatile unsigned char           attr;      /* message attribute : SYN OR ASYN           */
	volatile unsigned char           type;      /* message type : DVFS_REQ                   */
	volatile unsigned char           result;    /* message process result                    */
	volatile struct arisc_message   *next;      /* pointer of next message frame             */
	volatile struct arisc_msg_cb         cb;        /* the callback function and arg of message  */
	volatile void                   *private;   /* message private data                      */
	volatile unsigned int                paras[11]; /* the parameters of message                 */
} arisc_message_t;

/* the base of messages */
#define ARISC_MESSAGE_BASE          (0x10)

/* standby commands */
#define ARISC_SSTANDBY_ENTER_REQ        (ARISC_MESSAGE_BASE + 0x00)  /* request to enter       (ac327 to arisc) */
#define ARISC_SSTANDBY_RESTORE_NOTIFY   (ARISC_MESSAGE_BASE + 0x01)  /* restore finished       (ac327 to arisc) */
#define ARISC_NSTANDBY_ENTER_REQ        (ARISC_MESSAGE_BASE + 0x02)  /* request to enter       (ac327 to arisc) */
#define ARISC_NSTANDBY_WAKEUP_NOTIFY    (ARISC_MESSAGE_BASE + 0x03)  /* wakeup notify          (arisc to ac327) */
#define ARISC_NSTANDBY_RESTORE_REQ      (ARISC_MESSAGE_BASE + 0x04)  /* request to restore     (ac327 to arisc) */
#define ARISC_NSTANDBY_RESTORE_COMPLETE (ARISC_MESSAGE_BASE + 0x05)  /* arisc restore complete (arisc to ac327) */
#define ARISC_ESSTANDBY_ENTER_REQ       (ARISC_MESSAGE_BASE + 0x06)  /* request to enter       (ac327 to arisc) */
#define ARISC_TSTANDBY_ENTER_REQ        (ARISC_MESSAGE_BASE + 0x07)  /* request to enter       (ac327 to arisc) */
#define ARISC_TSTANDBY_RESTORE_NOTIFY   (ARISC_MESSAGE_BASE + 0x08)  /* restore finished       (ac327 to arisc) */
#define ARISC_FAKE_POWER_OFF_REQ        (ARISC_MESSAGE_BASE + 0x09)  /* request to enter       (ac327 to arisc) */
#define ARISC_CPUIDLE_ENTER_REQ         (ARISC_MESSAGE_BASE + 0x0a)  /* request to enter       (ac327 to arisc) */
#define ARISC_STANDBY_INFO_REQ          (ARISC_MESSAGE_BASE + 0x10)  /* request sst info       (ac327 to arisc) */
#define ARISC_CPUIDLE_CFG_REQ           (ARISC_MESSAGE_BASE + 0x11)  /* request to config      (ac327 to arisc) */

/* dvfs commands */
#define ARISC_CPUX_DVFS_REQ              (ARISC_MESSAGE_BASE + 0x20)  /* request dvfs           (ac327 to arisc) */
#define ARISC_CPUX_DVFS_CFG_VF_REQ       (ARISC_MESSAGE_BASE + 0x21)  /* request config dvfs v-f table(ac327 to arisc) */
#define ARISC_CPUX_DVFS_CFG_REQ          (ARISC_MESSAGE_BASE + 0x22)  /* request config info.  (ac327 to arisc) */

/* pmu commands */
#define ARISC_AXP_INT_COMING_NOTIFY      (ARISC_MESSAGE_BASE + 0x40)  /* interrupt coming notify(arisc to ac327) */
#define ARISC_AXP_DISABLE_IRQ            (ARISC_MESSAGE_BASE + 0x41)  /* disable axp irq of arisc                */
#define ARISC_AXP_ENABLE_IRQ             (ARISC_MESSAGE_BASE + 0x42)  /* enable axp irq of arisc                 */
#define ARISC_AXP_GET_CHIP_ID            (ARISC_MESSAGE_BASE + 0x43)  /* axp get chip id                         */
#define ARISC_AXP_SET_PARAS              (ARISC_MESSAGE_BASE + 0x44)  /* config axp parameters (ac327 to arisc)  */
#define ARISC_SET_PMU_VOLT               (ARISC_MESSAGE_BASE + 0x45)  /* set pmu volt (ac327 to arisc)           */
#define ARISC_GET_PMU_VOLT               (ARISC_MESSAGE_BASE + 0x46)  /* get pmu volt (ac327 to arisc)           */
#define ARISC_SET_LED_BLN                (ARISC_MESSAGE_BASE + 0x47)  /* set led bln (ac327 to arisc)            */
#define ARISC_AXP_REBOOT                 (ARISC_MESSAGE_BASE + 0x48)  /* reboot system for no pmu protocols      */

/* set arisc debug commands */
#define ARISC_SET_DEBUG_LEVEL            (ARISC_MESSAGE_BASE + 0x50)  /* set arisc debug level  (ac327 to arisc)     */
#define ARISC_MESSAGE_LOOPBACK           (ARISC_MESSAGE_BASE + 0x51)  /* loopback message  (ac327 to arisc)          */
#define ARISC_SET_UART_BAUDRATE          (ARISC_MESSAGE_BASE + 0x52)  /* set uart baudrate (ac327 to arisc)          */
#define ARISC_SET_DRAM_PARAS             (ARISC_MESSAGE_BASE + 0x53)  /* config dram parameter (ac327 to arisc)      */
#define ARISC_SET_DEBUG_DRAM_CRC_PARAS   (ARISC_MESSAGE_BASE + 0x54)  /* config dram crc parameters (ac327 to arisc) */
#define ARISC_SET_IR_PARAS               (ARISC_MESSAGE_BASE + 0x55)  /* config ir parameter (ac327 to arisc)        */
#define ARISC_REPORT_ERR_INFO            (ARISC_MESSAGE_BASE + 0x56)  /* report arisc error info (arisc to ac327)    */

/* audio commands */
#define ARISC_AUDIO_START                (ARISC_MESSAGE_BASE + 0x60)  /* audio start play/capture(ac327 to arisc) */
#define ARISC_AUDIO_STOP                 (ARISC_MESSAGE_BASE + 0x61)  /* audio stop  play/capture(ac327 to arisc) */
#define ARISC_AUDIO_SET_BUF_PER_PARAS    (ARISC_MESSAGE_BASE + 0x62)  /* set audio buffer and peroid paras(ac327 to arisc) */
#define ARISC_AUDIO_GET_POSITION         (ARISC_MESSAGE_BASE + 0x63)  /* get audio buffer position(ac327 to arisc) */
#define ARISC_AUDIO_SET_TDM_PARAS        (ARISC_MESSAGE_BASE + 0x64)  /* set audio tdm parameters(ac327 to arisc) */
#define ARISC_AUDIO_PERDONE_NOTIFY       (ARISC_MESSAGE_BASE + 0x65)  /* audio period done notify(arisc to ac327) */
#define ARISC_AUDIO_ADD_PERIOD           (ARISC_MESSAGE_BASE + 0x66)  /* audio period done notify(arisc to ac327) */

#if defined CONFIG_ARCH_SUN8IW1P1
/* p2wi commands */
#define ARISC_P2WI_READ_BLOCK_DATA       (ARISC_MESSAGE_BASE + 0x70)  /* p2wi read block data        (ac327 to arisc) */
#define ARISC_P2WI_WRITE_BLOCK_DATA      (ARISC_MESSAGE_BASE + 0x71)  /* p2wi write block data       (ac327 to arisc) */
#define ARISC_P2WI_BITS_OPS_SYNC         (ARISC_MESSAGE_BASE + 0x72)  /* p2wi clear bits sync        (ac327 to arisc) */
#elif (defined CONFIG_ARCH_SUN8IW3P1) || (defined CONFIG_ARCH_SUN8IW5P1) || (defined CONFIG_ARCH_SUN8IW6P1) || \
      (defined CONFIG_ARCH_SUN8IW7P1) || (defined CONFIG_ARCH_SUN8IW9P1) || (defined CONFIG_ARCH_SUN9IW1P1)
/* rsb commands */
#define ARISC_RSB_READ_BLOCK_DATA        (ARISC_MESSAGE_BASE + 0x70)  /* rsb read block data        (ac327 to arisc) */
#define ARISC_RSB_WRITE_BLOCK_DATA       (ARISC_MESSAGE_BASE + 0x71)  /* rsb write block data       (ac327 to arisc) */
#define ARISC_RSB_BITS_OPS_SYNC          (ARISC_MESSAGE_BASE + 0x72)  /* rsb clear bits sync        (ac327 to arisc) */
#define ARISC_RSB_SET_INTERFACE_MODE     (ARISC_MESSAGE_BASE + 0x73)  /* rsb set interface mode     (ac327 to arisc) */
#define ARISC_RSB_SET_RTSADDR            (ARISC_MESSAGE_BASE + 0x74)  /* rsb set runtime slave addr (ac327 to arisc) */
#endif

/* arisc initialize state notify commands */
#define ARISC_STARTUP_NOTIFY             (ARISC_MESSAGE_BASE + 0x80)  /* arisc init state notify(arisc to ac327) */

#endif  /* __ARISC_MESSAGES_H */
