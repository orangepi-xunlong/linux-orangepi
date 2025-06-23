// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
/*
 * Copyright (c) 2019 Synopsys, Inc. and/or its affiliates.
 *
 * Author: Vitor Soares <vitor.soares@synopsys.com>
 */

#ifndef _UAPI_LINUX_I3C_DEVICE_H
#define _UAPI_LINUX_I3C_DEVICE_H

#include <linux/types.h>

/**
 * enum i3c_error_code - I3C error codes
 *
 * These are the standard error codes as defined by the I3C specification.
 * When -EIO is returned by the i3c_device_do_priv_xfers() or
 * i3c_device_send_hdr_cmds() one can check the error code in
 * &struct_i3c_priv_xfer.err or &struct i3c_hdr_cmd.err to get a better idea of
 * what went wrong.
 *
 * @I3C_ERROR_UNKNOWN: unknown error, usually means the error is not I3C
 *		       related
 * @I3C_ERROR_M0: M0 error
 * @I3C_ERROR_M1: M1 error
 * @I3C_ERROR_M2: M2 error
 */
enum i3c_error_code {
	I3C_ERROR_UNKNOWN = 0,
	I3C_ERROR_M0 = 1,
	I3C_ERROR_M1,
	I3C_ERROR_M2,
};

/**
 * enum i3c_hdr_mode - HDR mode ids
 * @I3C_HDR_DDR: DDR mode
 * @I3C_HDR_TSP: TSP mode
 * @I3C_HDR_TSL: TSL mode
 */
enum i3c_hdr_mode {
	I3C_HDR_DDR,
	I3C_HDR_TSP,
	I3C_HDR_TSL,
};

/**
 * struct i3c_priv_xfer - I3C SDR private transfer
 * @rnw: encodes the transfer direction. true for a read, false for a write
 * @len: transfer length in bytes of the transfer
 * @data: input/output buffer
 * @data.in: input buffer. Must point to a DMA-able buffer
 * @data.out: output buffer. Must point to a DMA-able buffer
 * @err: I3C error code
 */
struct i3c_priv_xfer {
	u8 rnw;
	u16 len;
	union {
		void *in;
		const void *out;
	} data;
	enum i3c_error_code err;
};

#endif /* _UAPI_LINUX_I3C_DEVICE_H */
