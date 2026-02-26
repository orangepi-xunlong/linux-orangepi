/*
 * spinand_ecc_op.c for  SUNXI NAND .
 *
 * Copyright (C) 2016 Allwinner.
 *
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include "spinand_ecc_op.h"

#ifdef	DEBUG
#ifndef debug
#define debug(fmt, args...)	NAND_Print (fmt, ##args)
#endif
#else
#ifndef debug
#define debug(fmt, args...)
#endif
#endif	/* DEBUG */

extern void NAND_Memcpy(void *pAddr_dst, void *pAddr_src, unsigned int len);
extern int NAND_Print(const char *fmt, ...);

#define DUMP_SPARE 0

#if DUMP_SPARE
void hexdump(const char *buf, int len)
{
	int i;
	for (i = 0; i < len;) {
		NAND_Print("0x%02x ", (int)(buf[i]));
		if (++i % 16 == 0)
			NAND_Print("\n");
	}
	NAND_Print("\n");
}
#endif

static inline __s32 general_check_ecc(__u8 ecc, __u8 limit_from, __u8 limit_to,
		__u8 err_from, unsigned char err_to)
{
	if (ecc < limit_from) {
		return NAND_OP_TRUE;
	} else if (ecc >= limit_from && ecc <= limit_to) {
		debug("ecc limit 0x%x\n", ecc);
		return ECC_LIMIT;
	} else if (ecc >= err_from && ecc <= err_to) {
		NAND_Print("ecc error 0x%x\n", ecc);
		return ERR_ECC;
	} else {
		NAND_Print("unknown ecc value 0x%x\n", ecc);
		return ERR_ECC;
	}
}

static __s32 check_ecc_bit2_limit1_err2_limit3(__u8 ecc)
{
	if (ecc == 0) {
		return NAND_OP_TRUE;
	} else if (ecc == 1 || ecc == 3) {
		debug("ecc limit 0x%x\n", ecc);
		return ECC_LIMIT;
	} else {
		NAND_Print("ecc error 0x%x\n", ecc);
		return ERR_ECC;
	}
}

static __s32 check_ecc_bit3_limit5_err2(__u8 ecc)
{
	if (ecc <= 1) {
		return NAND_OP_TRUE;
	} else if (ecc == 3 || ecc == 5) {
		debug("ecc limit 0x%x\n", ecc);
		return ECC_LIMIT;
	} else {
		NAND_Print("ecc error 0x%x\n", ecc);
		return ERR_ECC;
	}
}

static __s32 check_ecc_bit4_limit5_7_err8_limit12(__u8 ecc)
{
	if (ecc <= 4) {
		return NAND_OP_TRUE;
	} else if (((ecc >= 5) && (ecc <= 7)) || (ecc >= 12)) {
		debug("ecc limit 0x%x\n", ecc);
		return ECC_LIMIT;
	} else {
		debug("ecc error 0x%x\n", ecc);
		return ERR_ECC;
	}
}
static __s32 check_ecc_bit4_limit5_7_err8(__u8 ecc)
{
	if (ecc <= 4) {
		return NAND_OP_TRUE;
	} else if (((ecc >= 5) && (ecc <= 7))) {
		debug("ecc limit 0x%x\n", ecc);
		return ECC_LIMIT;
	} else {
		debug("ecc error 0x%x\n", ecc);
		return ERR_ECC;
	}
}


static __s32 check_ecc_bit4_limit5_8_err15(__u8 ecc)
{
	if (ecc <= 4) {
		return NAND_OP_TRUE;
	} else if (((ecc >= 5) && (ecc <= 8))) {
		debug("ecc limit 0x%x\n", ecc);
		return ECC_LIMIT;
	} else {
		debug("ecc error 0x%x\n", ecc);
		return ERR_ECC;
	}
}
static __s32 check_ecc_bit3_limit3_limit5_err2(__u8 ecc)
{
	if (ecc <= 1) {
		return NAND_OP_TRUE;
	} else if (ecc == 3 || ecc == 5) {
		debug("ecc limit 0x%x\n", ecc);
		return ECC_LIMIT;
	} else {
		debug("ecc error 0x%x\n", ecc);
		return ERR_ECC;
	}
}

static __s32 check_ecc_bit2_limit3_err2(__u8 ecc)
{
	if (ecc <= 1) {
		return NAND_OP_TRUE;
	} else if (ecc == 3) {
		debug("ecc limit 0x%x\n", ecc);
		return ECC_LIMIT;
	} else {
		debug("ecc error 0x%x\n", ecc);
		return ERR_ECC;
	}
}

__s32 spinand_check_ecc(enum ecc_type type, __u8 status)
{
	__u8 ecc;

	switch (type) {
	case BIT3_LIMIT3_TO_6_ERR7:
		ecc = status & 0x07;
		return general_check_ecc(ecc, 3, 6, 7, 7);
	case BIT2_LIMIT1_ERR2:
		ecc = status & 0x03;
		return general_check_ecc(ecc, 1, 1, 2, 2);
	case BIT2_LIMIT1_ERR2_LIMIT3:
		ecc = status & 0x03;
		return check_ecc_bit2_limit1_err2_limit3(ecc);
	case BIT4_LIMIT3_TO_4_ERR15:
		ecc = status & 0x0f;
		return general_check_ecc(ecc, 3, 4, 15, 15);
	case BIT3_LIMIT3_TO_4_ERR7:
		ecc = status & 0x07;
		return general_check_ecc(ecc, 3, 4, 7, 7);
	case BIT3_LIMIT2_TO_6_ERR7:
		ecc = status & 0x07;
		return general_check_ecc(ecc, 2, 6, 7, 7);
	case BIT3_LIMIT5_ERR2:
		ecc = status & 0x07;
		return check_ecc_bit3_limit5_err2(ecc);
	case BIT4_LIMIT5_TO_7_ERR8_LIMIT_12:
		ecc = status & 0x0f;
		return check_ecc_bit4_limit5_7_err8_limit12(ecc);
	case BIT4_LIMIT5_TO_7_ERR8:
		ecc = status & 0x0f;
		return check_ecc_bit4_limit5_7_err8(ecc);
	case BIT2_LIMIT3_ERR2:
		ecc = status & 0x03;
		return check_ecc_bit2_limit3_err2(ecc);
	case BIT3_LIMIT3_LIMIT_5_ERR2:
		ecc = status & 0x07;
		return check_ecc_bit3_limit3_limit5_err2(ecc);
	case BIT4_LIMIT5_TO_8_ERR15:
		ecc = status & 0x0f;
		return check_ecc_bit4_limit5_8_err15(ecc);
	default:
		return NAND_OP_FALSE;
	}
}

static __s32 __copy_to_spare(__u8 *to, __u8 *from, __u32 cnt, __u32 spare_size,
		__u32 offset, __u32 datalen)
{
	__u32 i;
#if DUMP_SPARE
	__u32 len = cnt;
#endif

	/* bad block mark byte */
	to[0] = from[0];

	for (i = 0; cnt > 0; i++) {
		/*
		 * only the last time, num maybe not datalen, that's why
		 * memcpy offset add datalen rather than num
		 */
		__u32 num = cnt > datalen ? datalen : cnt;
		NAND_Memcpy(to + offset + spare_size * i, from + datalen * i, num);
		cnt -= num;
	}
#if DUMP_SPARE
	NAND_Print("Write: from: ram buf\n");
	hexdump((const char *)from, len);
	NAND_Print("Write: to: phy spare\n");
	hexdump((const char *)to, 64);
#endif
	return NAND_OP_TRUE;
}

static __s32 __copy_from_spare(__u8 *to, __u8 *from, __u32 cnt, __u32 spare_size,
		__u32 offset, __u32 datalen)
{
	__u32 i;
#if DUMP_SPARE
	__u32 len = cnt;
#endif

	for (i = 0; cnt > 0; i++) {
		/*
		 * only the last time, num maybe not datalen, that's why
		 * memcpy offset add datalen rather than num
		 */
		__u32 num = cnt > datalen ? datalen : cnt;
		NAND_Memcpy(to + datalen * i, from + offset + spare_size * i, num);
		cnt -= num;
	}
#if DUMP_SPARE
	NAND_Print("Read: from: phy spare\n");
	hexdump((const char *)from, 64);
	NAND_Print("Read: to: ram buf\n");
	hexdump((const char *)to, len);
#endif
	return NAND_OP_TRUE;
}

__s32 spinand_copy_to_spare(__u32 type, __u8 *to, __u8 *from, __u32 cnt)
{
	switch (type) {
	case SIZE16_OFF0_LEN16:
		return __copy_to_spare(to, from, cnt, 16, 0, 16);
	case SIZE16_OFF4_LEN12:
		return __copy_to_spare(to, from, cnt, 16, 4, 12);
	case SIZE16_OFF8_LEN16:
		return __copy_to_spare(to, from, cnt, 16, 8, 16);
	case SIZE16_OFF4_LEN4_OFF8:
		return __copy_to_spare(to, from, cnt, 16, 4, 4);
	case SIZE16_OFF4_LEN8_OFF4:
		return __copy_to_spare(to, from, cnt, 16, 4, 8);
	case SIZE64_OFF32_LEN32:
		return __copy_to_spare(to, from, cnt, 64, 32, 32);
	case SIZE64_OFF8_LEN40_OFF16:
		return __copy_to_spare(to, from, cnt, 64, 8, 40);
	case SIZE256_OFF64_LEN64:
		return __copy_to_spare(to, from, cnt, 256, 64, 64);
	default:
		return NAND_OP_FALSE;
	}
}

__s32 spinand_copy_from_spare(__u32 type, __u8 *to, __u8 *from, __u32 cnt)
{
	switch (type) {
	case SIZE16_OFF0_LEN16:
		return __copy_from_spare(to, from, cnt, 16, 0, 16);
	case SIZE16_OFF4_LEN12:
		return __copy_from_spare(to, from, cnt, 16, 4, 12);
	case SIZE16_OFF8_LEN16:
		return __copy_from_spare(to, from, cnt, 16, 8, 16);
	case SIZE16_OFF4_LEN4_OFF8:
		return __copy_from_spare(to, from, cnt, 16, 4, 4);
	case SIZE16_OFF4_LEN8_OFF4:
		return __copy_from_spare(to, from, cnt, 16, 4, 8);
	case SIZE64_OFF32_LEN32:
		return __copy_from_spare(to, from, cnt, 64, 32, 32);
	case SIZE64_OFF8_LEN40_OFF16:
		return __copy_from_spare(to, from, cnt, 64, 8, 40);
	case SIZE256_OFF64_LEN64:
		return __copy_from_spare(to, from, cnt, 256, 64, 64);
	default:
		return NAND_OP_FALSE;
	}
}

__u32 spinand_get_spare_size(__u32 type)
{
	switch (type) {
	case SIZE256_OFF64_LEN64:
		return 256;
	default:
		return 64;
	}
}
