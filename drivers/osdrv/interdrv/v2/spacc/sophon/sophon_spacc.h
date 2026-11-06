/* SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef __SOPHON_SPACC_H__
#define __SOPHON_SPACC_H__
#define OPTEE_SMC_CALL_CV_BASE64 0x0300000D
#define OPTEE_SMC_CALL_CV_SPACC_EXEC 0x0300000A

struct cvi_spacc_base64 {
	u32 customer_code;
	u32 action; // 0: Decode, 1: Encode
};

struct cvi_spacc_base64_inner {
	u64 src;
	u64 dst;
	u64 len;
	u32 customer_code;
	u32 action; // 0: Decode, 1: Encode
};
enum algo {
	ALGO_BYPASS = 8,
	ALGO_AES = 9,
	ALGO_DES = 10,
	ALGO_SM4 = 11,
	ALGO_BASE64 = 13
};

enum mode { AES_ECB = 0, AES_CBC = 1, AES_CTR = 2, DES_DES = 3, DES_TDES = 4 };

enum key_mode { AES_128BIT = 4, AES_192BIT = 2, AES_256BIT = 1 };

enum action { DECRYPT = 0, ENCRYPTION = 1 };
enum otp { USE_DMA_KEY = 0, USE_OTP_KEY = 1 };
typedef struct _spacc_exec_config {
	enum algo algo;
	enum mode mode;
	enum key_mode key_mode;
	uintptr_t key;
	uintptr_t iv;
	enum action action;
	enum otp otp;
} spacc_exec_config;

#define IOCTL_SPACC_BASE 'S'
#define IOCTL_SPACC_CREATE_MEMPOOL _IOW(IOCTL_SPACC_BASE, 1, unsigned int)
#define IOCTL_SPACC_GET_MEMPOOL_SIZE _IOR(IOCTL_SPACC_BASE, 2, unsigned int)
#define IOCTL_SPACC_BASE 'S'
#define IOCTL_SPACC_CREATE_POOL _IOW(IOCTL_SPACC_BASE, 1, unsigned int)
#define IOCTL_SPACC_GET_POOL_SIZE _IOR(IOCTL_SPACC_BASE, 2, unsigned int)

#define IOCTL_SPACC_BASE64 _IOW(IOCTL_SPACC_BASE, 5, struct cvi_spacc_base64)
#define IOCTL_SPACC_BASE64_INNER                                               \
	_IOW(IOCTL_SPACC_BASE, 6, struct cvi_spacc_base64_inner)
#define IOCTL_SPACC_SHA256_ACTION _IO(IOCTL_SPACC_BASE, 5)
#define IOCTL_SPACC_SHA1_ACTION _IO(IOCTL_SPACC_BASE, 6)
#define IOCTL_SPACC_AES_ACTION _IOW(IOCTL_SPACC_BASE, 8, spacc_exec_config)
#define IOCTL_SPACC_SM4_ACTION _IOW(IOCTL_SPACC_BASE, 9, spacc_sm4_config_s)
#define IOCTL_SPACC_DES_ACTION _IOW(IOCTL_SPACC_BASE, 10, spacc_des_config_s)
#define IOCTL_SPACC_TDES_ACTION _IOW(IOCTL_SPACC_BASE, 11, spacc_tdes_config_s)

#endif
