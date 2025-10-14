/* SPDX-License-Identifier: GPL-2.0*/
/*
 * display port hdcp driver for the cix ec
 *
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#ifndef CIX_HDCP_IOCTL_CMD_H_
#define CIX_HDCP_IOCTL_CMD_H_

#include <linux/types.h>
#include <asm/ioctl.h>

typedef struct {
	unsigned char rtx[8]; //  random TX value
	unsigned char rrx[8]; //  random RX value
	unsigned char RxInfo[2]; //  receiver information
	unsigned char RxCaps[3]; //  receiver capabilities
	unsigned char TxCaps[3]; //  transmitter capabilities
	unsigned char certrx[522]; //  Receiver certificate
	unsigned char km[16]; //  device master key km (128-bits)
	unsigned char
		dkey0[16]; //  dkey0 is used to build kd, kd is used to calculate H
	unsigned char
		dkey1[16]; //  dkey1 is used to build kd, kd is used to calculate H
	unsigned char dkey2[16];
	unsigned char kd[32]; //  kd is used to calculate H
	unsigned char Ekpub_km[128];
	unsigned char Ekh_km[16];
	unsigned char m[16];
	unsigned char H_val[32];
	unsigned char H_prime[32];
	unsigned char rn[8];
	unsigned char L[32];
	unsigned char L_prime[32];
	unsigned char ks[16];
	unsigned char Edkey_ks[16];
	unsigned char riv[8];
	unsigned char seq_num_V[3];
	unsigned char seq_num_M[3];
	unsigned char V_val[32];
	unsigned char V_prime[16];
	unsigned char M_prime[32];
	unsigned char RcvID_List[155];
	unsigned char k[2];
	unsigned char StreamID_Type[126];
	unsigned char RxStatus;
} hdcp2_tx_data_t;

typedef struct {
	unsigned char rtx[8]; // random TX value
	unsigned char rrx[8]; //  random RX value
	unsigned char km[16]; //  device master key km (128-bits)
	unsigned char dkey0[16]; // dkey0 is used to build kd
	unsigned char dkey1[16]; // dkey1 is used to build kd
	unsigned char kd[32]; //  kd is used to calculate H
} hdcp2_kd_t;

typedef struct {
	unsigned char rn[8];
	unsigned char km[16]; //  device master key km (128-bits)
	unsigned char dkey2[16];
} hdcp2_dkey2_t;

typedef struct {
	unsigned char riv[8];
	unsigned char ks[16];
} hdcp2_cipher_data_t;

//------------------------------------------------------------------------------
//  Aux transaction callback
//------------------------------------------------------------------------------
typedef struct dptx_aux_trxn_s
	dptx_aux_trxn_t; //  Aux Transaction forward reference
typedef void (*dptx_aux_cb_t)(dptx_aux_trxn_t *trxn); //  callback

//------------------------------------------------------------------------------
//  aux transmitter status
//------------------------------------------------------------------------------
typedef enum {
	dptx_aux_status_ok, // status is good, operation success
	dptx_aux_status_error, // general error condition
	dptx_aux_status_ct // status count (must be last)
} dptx_aux_status_t;

//------------------------------------------------------------------------------
//  DisplayPort transmitter aux transaction structure.
//  This structure is linked to the dptx_aux_trxn_t typedef.
//------------------------------------------------------------------------------
struct dptx_aux_trxn_s {
	uint32_t cmd; //  aux-bus command
	uint32_t addr; //  native: aux-bus address / i2c: i2c-address
	uint32_t ct; //  data count
	uint8_t *data;
	void *user_data; //  handle data
	dptx_aux_cb_t cb;
	dptx_aux_status_t status; //  transaction status
};

#define CIX_HDCP_IOCTL_BASE 'H'
#define CIX_HDCP_IO(nr) _IO(CIX_HDCP_IOCTL_BASE, nr)
#define CIX_HDCP_IOR(nr, type) _IOR(CIX_HDCP_IOCTL_BASE, nr, type)
#define CIX_HDCP_IOW(nr, type) _IOW(CIX_HDCP_IOCTL_BASE, nr, type)
#define CIX_HDCP_IOWR(nr, type) _IOWR(CIX_HDCP_IOCTL_BASE, nr, type)

#define HDCP2_IOCTL_RXSTATE CIX_HDCP_IOR(0x00, uint)
#define HDCP2_IOCTL_TIMER_START CIX_HDCP_IOW(0x01, uint)
#define HDCP2_IOCTL_TIMER_STOP CIX_HDCP_IO(0x02)
#define HDCP2_IOCTL_DPCD_ACCESS CIX_HDCP_IOWR(0x03, dptx_aux_trxn_t)
#define HDCP2_IOCTL_HWINIT CIX_HDCP_IO(0x04)
#define HDCP2_IOCTL_CIPHER_ENABLE CIX_HDCP_IOW(0x05, hdcp2_cipher_data_t)
#define HDCP2_IOCTL_CIPHER_DISABLE CIX_HDCP_IO(0x06)
#define HDCP2_IOCTL_GET_KD CIX_HDCP_IOWR(0x07, hdcp2_kd_t)
#define HDCP2_IOCTL_GET_DKEY2 CIX_HDCP_IOWR(0x08, hdcp2_dkey2_t)

#endif // CIX_HDCP_IOCTL_CMD_H_
