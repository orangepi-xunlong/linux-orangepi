//-----------------------------------------------------------------------------
// COPYRIGHT (C) 2020   CHIPS&MEDIA INC. ALL RIGHTS RESERVED
//
// This file is distributed under BSD 3 clause and LGPL2.1 (dual license)
// SPDX License Identifier: BSD-3-Clause
// SPDX License Identifier: LGPL-2.1-only
//
// The entire notice above must be reproduced on all authorized copies.
//
// Description  :
//-----------------------------------------------------------------------------
#ifndef _JPU_TYPES_H_
#define _JPU_TYPES_H_

#if 0
#include <stdint.h>
/**
* @brief    This type is an 8-bit unsigned integral type, which is used for declaring pixel data.
*/
typedef uint8_t             Uint8;

/**
* @brief    This type is a 16-bit unsigned integral type.
*/
typedef uint16_t            Uint16;

/**
* @brief    This type is a 32-bit unsigned integral type, which is used for declaring variables with wide ranges and no signs such as size of buffer.
*/
typedef uint32_t            Uint32;

/**
* @brief    This type is a 64-bit unsigned integral type, which is used for declaring variables with wide ranges and no signs such as size of buffer.
*/
typedef uint64_t            Uint64;

/**
* @brief    This type is an 8-bit signed integral type.
*/
typedef int8_t              Int8;

/**
* @brief    This type is a 16-bit signed integral type.
*/
typedef int16_t             Int16;

/**
* @brief    This type is a 32-bit signed integral type.
*/
typedef int32_t             Int32;

/**
* @brief    This type is a 64-bit signed integral type.
*/
typedef int64_t             Int64;

/**
* @brief
This is a type for representing physical addresses which is recognizable by the JPU. In general,
the JPU hardware does not know about virtual address space which is set and handled by host
processor. All these virtual addresses are translated into physical addresses by Memory Management
Unit. All data buffer addresses such as stream buffer, frame buffer, should be given to
the JPU as an address on physical address space.
*/
typedef uint32_t            PhysicalAddress;

/**
* @brief This type is an 8-bit unsigned character type.
*/
typedef unsigned char       BYTE;

/**
* @brief This type is an 8-bit boolean data type to indicate TRUE or FALSE.
*/
typedef int32_t             BOOL;

#else
typedef unsigned int    UINT;
typedef unsigned char   Uint8;
typedef unsigned int    Uint32;
typedef unsigned short  Uint16;
typedef char            Int8;
typedef int             Int32;
typedef short           Int16;
#if defined(_MSC_VER)
typedef unsigned _int64 Uint64;
typedef _int64 Int64;
#else
typedef unsigned long long Uint64;
typedef long long Int64;
#endif
typedef Uint64 PhysicalAddress;
typedef unsigned char   BYTE;
typedef int BOOL;

#endif



#ifndef NULL
#define NULL    0
#endif

#ifndef TRUE
#define TRUE                        1
#endif /* TRUE */

#define STATIC              static

#ifndef FALSE
#define FALSE                       0
#endif /* FALSE */

#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(P)          \
    /*lint -save -e527 -e530 */ \
{ \
    (P) = (P); \
} \
    /*lint -restore */
#endif

#endif    /* _JPU_TYPES_H_ */
