/*
 * The confidential and proprietary information contained in this file may
 * only be used by a person authorised under and to the extent permitted
 * by a subsisting licensing agreement from Arm Technology (China) Co., Ltd.
 *
 *            (C) COPYRIGHT 2021-2021 Arm Technology (China) Co., Ltd.
 *                ALL RIGHTS RESERVED
 *
 * This entire notice must be reproduced on all copies of this file
 * and copies of this file may only be made by a person if such person is
 * permitted to do so under the terms of a subsisting license agreement
 * from Arm Technology (China) Co., Ltd.
 * 
 * SPDX-License-Identifier: GPL-2.0-only
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * 
 */

#ifndef MVX_LOG_RAM_H
#define MVX_LOG_RAM_H

/******************************************************************************
 * Includes
 ******************************************************************************/

#ifndef __KERNEL__
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#else
#include <linux/types.h>
#include <linux/time.h>
#endif

/******************************************************************************
 * Defines
 ******************************************************************************/

/**
 * Magic word "MVXL" that prefix all messages.
 *
 * Messages are stored in native byte order. The magic word can be used to
 * detect if the log has been stored in the same byte order as the application
 * unpacking the log is using.
 */
#define MVX_LOG_MAGIC                   0x4d56584c

/**
 * The maximum message length.
 */
#define MVX_LOG_MESSAGE_LENGTH_MAX      4096

/******************************************************************************
 * Types
 ******************************************************************************/

/**
 * enum mvx_log_ioctl - IOCTL commands.
 * @MVX_LOG_IOCTL_CLEAR:	Clear the log.
 */
enum mvx_log_ioctl {
	MVX_LOG_IOCTL_CLEAR
};

/**
 * enum mvx_log_type - Message type. The definitions are assigned values that
 *		       are not allowed to change.
 */
enum mvx_log_type {
	MVX_LOG_TYPE_TEXT      = 0,
	MVX_LOG_TYPE_FWIF      = 1,
	MVX_LOG_TYPE_FW_BINARY = 2,
	MVX_LOG_TYPE_MAX
};

/**
 * struct mvx_log_timeval - Portable time value format.
 * @sec:		Seconds since 1970-01-01, Unix time epoch.
 * @nsec:		Nano seconds.
 */
struct mvx_log_timeval {
	uint64_t sec;
	uint64_t nsec;
}
__attribute__((packed));

/**
 * struct mvx_log_header - Common header for all messages stored in RAM buffer.
 * @magic:		Magic word.
 * @length:		Length of message, excluding this header.
 * @type:		Message type.
 * @severity:		Message severity.
 * @timestamp:		Time stamp.
 */
struct mvx_log_header {
	uint32_t magic;
	uint16_t length;
	uint8_t type;
	uint8_t severity;
	struct mvx_log_timeval timestamp;
}
__attribute__((packed));

/******************************************************************************
 * Text message
 ******************************************************************************/

/**
 * struct mvx_log_text - ASCII text message.
 * @message[0]:		ASCII text message.
 *
 * The message shall be header.length long and should end with a standard ASCII
 * character. The parser of the log will add new line and null terminate
 * the string.
 */
struct mvx_log_text {
	char message[0];
}
__attribute__((packed));

/******************************************************************************
 * Firmware interface
 ******************************************************************************/

/**
 * enum mvx_log_fwif_channel - Firmware interface message types.
 */
enum mvx_log_fwif_channel {
	MVX_LOG_FWIF_CHANNEL_MESSAGE,
	MVX_LOG_FWIF_CHANNEL_INPUT_BUFFER,
	MVX_LOG_FWIF_CHANNEL_OUTPUT_BUFFER,
	MVX_LOG_FWIF_CHANNEL_RPC
};

/**
 * enum mvx_log_fwif_direction - Firmware interface message types.
 */
enum mvx_log_fwif_direction {
	MVX_LOG_FWIF_DIRECTION_HOST_TO_FIRMWARE,
	MVX_LOG_FWIF_DIRECTION_FIRMWARE_TO_HOST
};

/**
 * enum mvx_log_fwif_code - Special message codes for message types not defined
 *			    by the firmware interface.
 */
enum mvx_log_fwif_code {
	MVX_LOG_FWIF_CODE_STAT = 16000
};

/**
 * struct mvx_log_fwif - Firmware interface header type.
 * @version_minor:		Protocol version.
 * @version_major:		Protocol version.
 * @channel:			@see enum mvx_log_fwif_channel.
 * @direction:			@see enum mvx_log_fwif_direction.
 * @session:			Session id.
 * @data[0]:			Data following the firmware interface message
 *				header.
 */
struct mvx_log_fwif {
	uint8_t version_minor;
	uint8_t version_major;
	uint8_t channel;
	uint8_t direction;
	uint64_t session;
	uint8_t data[0];
}
__attribute__((packed));

/**
 * struct mvx_log_fwif_stat - Firmware interface statistics.
 * @handle:		Buffer handle.
 * @queued:		Number of buffers currently queued to the firmware.
 */
struct mvx_log_fwif_stat {
	uint64_t handle;
	uint32_t queued;
}
__attribute__((packed));

/******************************************************************************
 * Firmware binary header
 ******************************************************************************/

/**
 * struct mvx_log_fw_binary - Firmware binary header.
 * @session:		Session id.
 * @data[0]:		Firmware binary, byte 0..length.
 *
 * The first ~100 bytes of the firmware binary contain information describing
 * the codec.
 */
struct mvx_log_fw_binary {
	uint64_t session;
	uint8_t data[0];
};

#endif /* MVX_LOG_RAM_H */
