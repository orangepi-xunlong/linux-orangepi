/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 *  sha1.h
 *
 *  Description:
 *      This is the header file for code which implements the Secure
 *      Hashing Algorithm 1 as defined in FIPS PUB 180-1 published
 *      April 17, 1995.
 *
 *      Many of the variable names in this code, especially the
 *      single character names, were used because those were the names
 *      used in the publication.
 *
 *      Please read the file sha1.c for more information.
 *
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 * Author: huangyongxing <huangyongxing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#ifndef __SHA1_H__
#define __SHA1_H__
//#include <stdint.h>
#include <linux/types.h>
#define SHA1HashSize 20

enum {
    shaSuccess = 0,
    shaNull,            /* Null pointer parameter */
    shaInputTooLong,    /* input data too long */
    shaStateError       /* called Input after Result */
};

/*
 *  This structure will hold context information for the SHA-1
 *  hashing operation
 */
typedef struct SHA1Context {
    uint32_t Intermediate_Hash[SHA1HashSize / 4];                               // Message Digest
    uint32_t Length_Low;                                                        // Message length in bits
    uint32_t Length_High;                                                       // Message length in bits
    short int Message_Block_Index;                                          // Index into message block array
    uint8_t Message_Block[64];                                                  // 512-bit message blocks
    int Computed;                                                               // Is the digest computed?
    int Corrupted;                                                              // Is the message digest corrupted?
} SHA1Context;

int SHA1Reset(SHA1Context *);
int SHA1Input(SHA1Context *, const uint8_t *,  unsigned int);
int SHA1Result(SHA1Context *, uint8_t Message_Digest[SHA1HashSize]);

#endif
