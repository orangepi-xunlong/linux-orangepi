/* SPDX-License-Identifier: GPL-2.0-or-later */
//------------------------------------------------------------------------------
//
//  COPYRIGHT (c) 2018-2022 TRILINEAR TECHNOLOGIES, INC.
//  CONFIDENTIAL AND PROPRIETARY
//
//  THE SOURCE CODE CONTAINED HEREIN IS PROVIDED ON AN "AS IS" BASIS.
//  TRILINEAR TECHNOLOGIES, INC. DISCLAIMS ANY AND ALL WARRANTIES,
//  WHETHER EXPRESS, IMPLIED, OR STATUTORY, INCLUDING ANY IMPLIED
//  WARRANTIES OF MERCHANTABILITY OR OF FITNESS FOR A PARTICULAR PURPOSE.
//  IN NO EVENT SHALL TRILINEAR TECHNOLOGIES, INC. BE LIABLE FOR ANY
//  INCIDENTAL, PUNITIVE, OR CONSEQUENTIAL DAMAGES OF ANY KIND WHATSOEVER
//  ARISING FROM THE USE OF THIS SOURCE CODE.
//
//  THIS DISCLAIMER OF WARRANTY EXTENDS TO THE USER OF THIS SOURCE CODE
//  AND USER'S CUSTOMERS, EMPLOYEES, AGENTS, TRANSFEREES, SUCCESSORS,
//  AND ASSIGNS.
//
//  THIS IS NOT A GRANT OF PATENT RIGHTS
//------------------------------------------------------------------------------
//
//  Original License:
//  public domain sha256 implementation based on fips180-3
//
//------------------------------------------------------------------------------
#ifndef __SHA256_H__
#define __SHA256_H__
#define SHA256_DIGEST_LENGTH    32

typedef struct {
    uint64_t len;               /* processed message length */
    uint32_t h[8];              /* hash state */
    uint8_t buf[64];            /* message block buffer */
} sha256_t;

void sha256_init(sha256_t *ctx);
void sha256_update(sha256_t *ctx, const void *m, uint32_t len);
void sha256_sum(sha256_t *ctx, uint8_t md[SHA256_DIGEST_LENGTH]);

#endif
