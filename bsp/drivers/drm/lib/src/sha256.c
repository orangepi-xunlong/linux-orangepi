/* SPDX-License-Identifier: GPL-2.0-or-later */
//------------------------------------------------------------------------------
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
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "sha256.h"

//------------------------------------------------------------------------------
//  sha256 constant array
//------------------------------------------------------------------------------
static const uint32_t K[64] = {
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

//------------------------------------------------------------------------------
//  These work together to produce the S0, S1, R0, R1 functions
//------------------------------------------------------------------------------
static inline uint32_t ror(uint32_t n_val, int k_val)
{
	return (n_val >> k_val) | (n_val << (32 - k_val));
}
#define Ch(x, y, z) (z ^ (x & (y ^ z)))
#define Maj(x, y, z) ((x & y) | (z & (x | y)))
#define S0(x)  (ror(x, 2) ^ ror(x, 13) ^ ror(x, 22))
#define S1(x)  (ror(x, 6) ^ ror(x, 11) ^ ror(x, 25))
#define R0(x)  (ror(x, 7) ^ ror(x, 18) ^ (x >> 3))
#define R1(x)  (ror(x, 17) ^ ror(x, 19) ^ (x >> 10))

//------------------------------------------------------------------------------
//  Function:   block_process
//  Process a block into the sha256 algorithm
//
//  Parameters:
//  s   - sha256 context
//  buf - buffer to process
//
//  Returns:
//  none
//------------------------------------------------------------------------------
static void block_process(sha256_t *s, const uint8_t *buf)
{
	uint32_t W[64], t1, t2, a_val, b_val, c_val, d_val, e, f, g, h;
	int i;

	for (i = 0; i < 16; i++) {
		W[i] = (uint32_t)buf[4 * i] << 24;
		W[i] |= (uint32_t)buf[4 * i + 1] << 16;
		W[i] |= (uint32_t)buf[4 * i + 2] << 8;
		W[i] |= buf[4 * i + 3];
	}
	for (; i < 64; i++)
		W[i] = R1(W[i - 2]) + W[i - 7] + R0(W[i - 15]) + W[i - 16];
	a_val = s->h[0];
	b_val = s->h[1];
	c_val = s->h[2];
	d_val = s->h[3];
	e = s->h[4];
	f = s->h[5];
	g = s->h[6];
	h = s->h[7];
	for (i = 0; i < 64; i++) {
		t1 = h + S1(e) + Ch(e, f, g) + K[i] + W[i];
		t2 = S0(a_val) + Maj(a_val, b_val, c_val);
		h = g;
		g = f;
		f = e;
		e = d_val + t1;
		d_val = c_val;
		c_val = b_val;
		b_val = a_val;
		a_val = t1 + t2;
	}
	s->h[0] += a_val;
	s->h[1] += b_val;
	s->h[2] += c_val;
	s->h[3] += d_val;
	s->h[4] += e;
	s->h[5] += f;
	s->h[6] += g;
	s->h[7] += h;
}

//------------------------------------------------------------------------------
//  Function:   block_pad
//  Pad block before executing sha256 algorithm
//
//  Parameters:
//  s   - sha256 context
//
//  Returns:
//  none
//  LDRA:  87 S - Use of pointer arithmetic
//------------------------------------------------------------------------------
static void block_pad(sha256_t *s)
{
	unsigned r = s->len % 64;

	s->buf[r++] = 0x80;
	if (r > 56) {
		/*LDRA_INSPECTED 87 S */
		memset(s->buf + r, 0, 64 - r);
		r = 0;
		block_process(s, s->buf);
	}
	/*LDRA_INSPECTED 87 S */
	memset(s->buf + r, 0, 56 - r);
	s->len *= 8;
	s->buf[56] = s->len >> 56;
	s->buf[57] = s->len >> 48;
	s->buf[58] = s->len >> 40;
	s->buf[59] = s->len >> 32;
	s->buf[60] = s->len >> 24;
	s->buf[61] = s->len >> 16;
	s->buf[62] = s->len >> 8;
	s->buf[63] = s->len;
	block_process(s, s->buf);
}

//------------------------------------------------------------------------------
//  Function:   sha256_init
//  Initialize sha256 context
//
//  Parameters:
//  ctx - sha256 context
//
//  Returns:
//  none
//------------------------------------------------------------------------------
void sha256_init(sha256_t *ctx)
{
	ctx->len = 0;
	ctx->h[0] = 0x6a09e667;
	ctx->h[1] = 0xbb67ae85;
	ctx->h[2] = 0x3c6ef372;
	ctx->h[3] = 0xa54ff53a;
	ctx->h[4] = 0x510e527f;
	ctx->h[5] = 0x9b05688c;
	ctx->h[6] = 0x1f83d9ab;
	ctx->h[7] = 0x5be0cd19;
}

//------------------------------------------------------------------------------
//  Function:   sha256_sum
//			  Calculate the sha256 over the block in the ctx
//
//  Parameters:
//	  ctx - sha256 context
//	  md  - output digetst
//
//  Returns:
//	  none
//------------------------------------------------------------------------------
void sha256_sum(sha256_t *ctx, uint8_t md[SHA256_DIGEST_LENGTH])
{
	int i;

	block_pad(ctx);
	for (i = 0; i < 8; i++) {
		md[4 * i]	   = ctx->h[i] >> 24;
		md[4 * i + 1]   = ctx->h[i] >> 16;
		md[4 * i + 2]   = ctx->h[i] >> 8;
		md[4 * i + 3]   = ctx->h[i];
	}
}

//------------------------------------------------------------------------------
//  Function:   sha256_update
//  Update the sha256 over the block m (length len)
//
//  Parameters:
//  ctx - sha256 context
//  m   - block pointer
//  len - block length
//
//  Returns:
//  none
//  LDRA:  87 S - Use of pointer arithmetic
//  LDRA:  53 S - Use of comma operator
//------------------------------------------------------------------------------
void sha256_update(sha256_t *ctx, const void *m, uint32_t len_val)
{
	uint32_t len = len_val;	 // temporary value to prevent 14 D Attempt to change parameter passed by value

	const uint8_t *p = m;
	unsigned r = ctx->len % 64;

	ctx->len += len;
	if (r) {
		if (len < 64 - r) {
			/*LDRA_INSPECTED 87 S */
			memcpy(ctx->buf + r, p, len);
			return;
		}
		/*LDRA_INSPECTED 87 S */
		memcpy(ctx->buf + r, p, 64 - r);
		len -= 64 - r;
		p += 64 - r;
		block_process(ctx, ctx->buf);
	}

	/*LDRA_INSPECTED 53 S */
	for (; len >= 64; len -= 64, p += 64)
		block_process(ctx, p);
	memcpy(ctx->buf, p, len);
}
