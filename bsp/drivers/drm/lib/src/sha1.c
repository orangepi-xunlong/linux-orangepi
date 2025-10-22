
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 *  sha1.c
 *
 *  Description:
 *	  This file implements the Secure Hashing Algorithm 1 as
 *	  defined in FIPS PUB 180-1 published April 17, 1995.
 *
 *	  The SHA-1, produces a 160-bit message digest for a given
 *	  data stream.  It should take about 2**n steps to find a
 *	  message with the same digest as a given message and
 *	  2**(n/2) to find any two messages with the same digest,
 *	  when n is the digest size in bits.  Therefore, this
 *	  algorithm can serve as a means of providing a
 *	  "fingerprint" for a message.
 *
 *  Portability Issues:
 *	  SHA-1 is defined in terms of 32-bit "words".  This code
 *	  uses <stdint.h> (included via "sha1.h" to define 32 and 8
 *	  bit unsigned integer types.  If your C compiler does not
 *	  support 32 bit unsigned integers, this code is not
 *	  appropriate.
 *
 *  Caveats:
 *	  SHA-1 is designed to work with messages less than 2^64 bits
 *	  long.  Although SHA-1 allows a message digest to be generated
 *	  for messages of any number of bits less than 2^64, this
 *	  implementation only works with messages with a length that is
 *	  a multiple of the size of an 8-bit character.
 *
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 * Author: huangyongxing <huangyongxing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include "sha1.h"

/*
 *  Define the SHA1 circular left shift macro
 */
#define SHA1CircularShift(bits, word) \
				(((word) << (bits)) | ((word) >> (32 - (bits))))

/* Local Function Prototyptes */
void SHA1PadMessage(SHA1Context *);
void SHA1ProcessMessageBlock(SHA1Context *);

/*
 *  Function: SHA1Reset
 *
 *  Description:
 *	  This function will initialize the SHA1Context in preparation
 *	  for computing a new SHA1 message digest.
 *
 *  Parameters:
 *	  context - [in/out] The context to reset.
 *
 *  Returns:
 *	  Error Code.
 *
 */
int SHA1Reset(SHA1Context *context)
{
	if (!context) {
		return shaNull;
	}

	context->Length_Low = 0;
	context->Length_High = 0;
	context->Message_Block_Index = 0;

	context->Intermediate_Hash[0] = 0x67452301;
	context->Intermediate_Hash[1] = 0xEFCDAB89;
	context->Intermediate_Hash[2] = 0x98BADCFE;
	context->Intermediate_Hash[3] = 0x10325476;
	context->Intermediate_Hash[4] = 0xC3D2E1F0;

	context->Computed = 0;
	context->Corrupted = 0;

	return shaSuccess;
}

/*
 *  Function: SHA1Result
 *
 *  Description:
 *  This function will return the 160-bit message digest into the
 *  Message_Digest array  provided by the caller.
 *  NOTE: The first octet of hash is stored in the 0th element,
 *	the last octet of hash in the 19th element.
 *
 *  Parameters:
 *  context - [in/out] The context to use to calculate the SHA-1 hash.
 *  Message_Digest - [out] Where the digest is returned.
 *
 *  Returns:
 *  Error Code.
 *
 */
int SHA1Result(SHA1Context *context,
		uint8_t Message_Digest[SHA1HashSize])
{
	int i;

	if ((!context) || (!Message_Digest)) {
		return shaNull;
	}

	if (context->Corrupted) {
		return context->Corrupted;
	}

	if (!context->Computed) {
		SHA1PadMessage(context);
		for (i = 0; i < 64; ++i) {
			/* message may be sensitive, clear it out */
			context->Message_Block[i] = 0;
		}
		context->Length_Low = 0;	/* and clear length */
		context->Length_High = 0;
		context->Computed = 1;
	}

	for (i = 0; i < SHA1HashSize; ++i) {
		Message_Digest[i] = context->Intermediate_Hash[i >> 2]
							>> 8 * (3 - (i & 0x03));
	}

	return shaSuccess;
}

/*
 *  Function: SHA1Input
 *
 *  Description:
 *  This function accepts an array of octets as the next portion
 *  of the message.
 *
 *  Parameters:
 *  context - [in/out] The SHA context to update
 *  message_array - [in] An array of characters representing the next portion of the message.
 *  length - [in] The length of the message in message_array
 *
 *  Returns:
 *  Error Code.
 *
 */
int SHA1Input(SHA1Context *context, const uint8_t  *message_array, unsigned length_val)
{
	unsigned length = length_val;	 // temporary value to prevent 14 D Attempt to change parameter passed by value

	if (!length) {
		return shaSuccess;
	}

	if ((!context) || (!message_array)) {
		return shaNull;
	}

	if (context->Computed) {
		context->Corrupted = shaStateError;
		return shaStateError;
	}

	if (context->Corrupted) {
		return context->Corrupted;
	}
	while ((length--) && (!context->Corrupted)) {
		context->Message_Block[context->Message_Block_Index++] =
		(*message_array & 0xFF);

		context->Length_Low += 8;
		if (context->Length_Low == 0) {
			context->Length_High++;
			if (context->Length_High == 0) {
					/* Message is too long */
			 context->Corrupted = 1;
			}
		}

		if (context->Message_Block_Index == 64) {
			SHA1ProcessMessageBlock(context);
		}

		message_array++;
	}

	return shaSuccess;
}

/*
 *  Function: SHA1ProcessMessageBlock
 *
 *  Description:
 *  This function will process the next 512 bits of the message
 *  stored in the Message_Block array.
 *
 *  Parameters:
 *  None.
 *
 *  Returns:
 *  Nothing.
 *
 *  Comments:
 *
 *  Many of the variable names in this code, especially the
 *  single character names, were used because those were the
 *  names used in the publication.
 *
 *
 */
void SHA1ProcessMessageBlock(SHA1Context *context)
{
	const uint32_t K[] = {  /* Constants defined in SHA-1   */
		0x5A827999,
		0x6ED9EBA1,
		0x8F1BBCDC,
		0xCA62C1D6
	};
	int t;	/* Loop counter			 */
	uint32_t temp; /* Temporary word value	 */
	uint32_t Word_Sequence[80]; /* Word sequence			*/
	uint32_t Abuff, Bbuff, Cbuff, Dbuff, Ebuff; /* Word buffers			 */

	/*
	 *  Initialize the first 16 words in the array W
	 */
	for (t = 0; t < 16; t++) {
		Word_Sequence[t] = context->Message_Block[t * 4] << 24;
		Word_Sequence[t] |= context->Message_Block[t * 4 + 1] << 16;
		Word_Sequence[t] |= context->Message_Block[t * 4 + 2] << 8;
		Word_Sequence[t] |= context->Message_Block[t * 4 + 3];
	}

	for (t = 16; t < 80; t++) {
		Word_Sequence[t] = SHA1CircularShift(1, Word_Sequence[t - 3] ^ Word_Sequence[t - 8] ^ Word_Sequence[t - 14] ^ Word_Sequence[t - 16]);
	}

	Abuff = context->Intermediate_Hash[0];
	Bbuff = context->Intermediate_Hash[1];
	Cbuff = context->Intermediate_Hash[2];
	Dbuff = context->Intermediate_Hash[3];
	Ebuff = context->Intermediate_Hash[4];

	for (t = 0; t < 20; t++) {
		temp =  SHA1CircularShift(5, Abuff) +
				((Bbuff & Cbuff) | ((~Bbuff) & Dbuff)) + Ebuff + Word_Sequence[t] + K[0];
		Ebuff = Dbuff;
		Dbuff = Cbuff;
		Cbuff = SHA1CircularShift(30, Bbuff);

		Bbuff = Abuff;
		Abuff = temp;
	}

	for (t = 20; t < 40; t++) {
		temp = SHA1CircularShift(5, Abuff) + (Bbuff ^ Cbuff ^ Dbuff) + Ebuff + Word_Sequence[t] + K[1];
		Ebuff = Dbuff;
		Dbuff = Cbuff;
		Cbuff = SHA1CircularShift(30, Bbuff);
		Bbuff = Abuff;
		Abuff = temp;
	}

	for (t = 40; t < 60; t++) {
		temp = SHA1CircularShift(5, Abuff) +
			   ((Bbuff & Cbuff) | (Bbuff & Dbuff) | (Cbuff & Dbuff)) + Ebuff + Word_Sequence[t] + K[2];
		Ebuff = Dbuff;
		Dbuff = Cbuff;
		Cbuff = SHA1CircularShift(30, Bbuff);
		Bbuff = Abuff;
		Abuff = temp;
	}

	for (t = 60; t < 80; t++) {
		temp = SHA1CircularShift(5, Abuff) + (Bbuff ^ Cbuff ^ Dbuff) + Ebuff + Word_Sequence[t] + K[3];
		Ebuff = Dbuff;
		Dbuff = Cbuff;
		Cbuff = SHA1CircularShift(30, Bbuff);
		Bbuff = Abuff;
		Abuff = temp;
	}

	context->Intermediate_Hash[0] += Abuff;
	context->Intermediate_Hash[1] += Bbuff;
	context->Intermediate_Hash[2] += Cbuff;
	context->Intermediate_Hash[3] += Dbuff;
	context->Intermediate_Hash[4] += Ebuff;

	context->Message_Block_Index = 0;
}

/*
 *  Function: SHA1PadMessage
 *
 *  Description:
 *  According to the standard, the message must be padded to an even
 *  512 bits.  The first padding bit must be a '1'.  The last 64
 *  bits represent the length of the original message.  All bits in
 *  between should be 0.  This function will pad the message
 *  according to those rules by filling the Message_Block array
 *  accordingly.  It will also call the ProcessMessageBlock function
 *  provided appropriately.  When it returns, it can be assumed that
 *  the message digest has been computed.
 *
 *  Parameters:
 *  context - [in/out] The context to pad
 *  ProcessMessageBlock - [in] The appropriate SHA*ProcessMessageBlock function
 *
 *  Returns:
 *  Nothing.
 *
 */
void SHA1PadMessage(SHA1Context *context)
{
	/*
	 *  Check to see if the current message block is too small to hold
	 *  the initial padding bits and length.  If so, we will pad the
	 *  block, process it, and then continue padding into a second
	 *  block.
	 */
	if (context->Message_Block_Index > 55) {
		context->Message_Block[context->Message_Block_Index++] = 0x80;
		while (context->Message_Block_Index < 64) {
			context->Message_Block[context->Message_Block_Index++] = 0;
		}

		SHA1ProcessMessageBlock(context);

		while (context->Message_Block_Index < 56) {
			context->Message_Block[context->Message_Block_Index++] = 0;
		}
	} else {
		context->Message_Block[context->Message_Block_Index++] = 0x80;
		while (context->Message_Block_Index < 56) {

			context->Message_Block[context->Message_Block_Index++] = 0;
		}
	}

	/*
	 *  Store the message length as the last 8 octets
	 */
	context->Message_Block[56] = context->Length_High >> 24;
	context->Message_Block[57] = context->Length_High >> 16;
	context->Message_Block[58] = context->Length_High >> 8;
	context->Message_Block[59] = context->Length_High;
	context->Message_Block[60] = context->Length_Low >> 24;
	context->Message_Block[61] = context->Length_Low >> 16;
	context->Message_Block[62] = context->Length_Low >> 8;
	context->Message_Block[63] = context->Length_Low;

	SHA1ProcessMessageBlock(context);
}
