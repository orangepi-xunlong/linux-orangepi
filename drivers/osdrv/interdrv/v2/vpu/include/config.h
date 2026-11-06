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

#ifndef __CONFIG_H__
#define __CONFIG_H__

#if defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(WIN32) || defined(__MINGW32__)
#	define PLATFORM_WIN32
#elif defined(linux) || defined(__linux) || defined(ANDROID)
#	define PLATFORM_LINUX
#elif defined(unix) || defined(__unix)
#   define PLATFORM_QNX
#else
#	define PLATFORM_NON_OS
#endif

#if defined(_MSC_VER)
#	include <windows.h>
#	define inline _inline
#elif defined(__GNUC__)
#elif defined(__ARMCC__)
#else
#  error "Unknown compiler."
#endif

#define API_VERSION_MAJOR       5
#define API_VERSION_MINOR       6
#define API_VERSION_PATCH       9
#define API_VERSION             ((API_VERSION_MAJOR<<16) | (API_VERSION_MINOR<<8) | API_VERSION_PATCH)

#if defined(PLATFORM_NON_OS) || defined(ANDROID) || defined(MFHMFT_EXPORTS) || defined(PLATFORM_QNX) || defined(_MSC_VER)
//#define SUPPORT_FFMPEG_DEMUX
#else
//#define SUPPORT_FFMPEG_DEMUX
#endif

//------------------------------------------------------------------------------
// COMMON
//------------------------------------------------------------------------------
#if defined(linux) || defined(__linux) || defined(ANDROID)
#define SUPPORT_INTERRUPT
#endif


#if defined(CNM_FPGA_VU440_INTERFACE) || defined(CNM_FPGA_USB_INTERFACE) || defined(CNM_FPGA_VU19P_INTERFACE)
#define SUPPORT_ENC_YUV_SAVE_AND_WRITE
#endif


//#define SUPPORT_ENC_BIG_ENDIAN_YUV

// do not define BIT_CODE_FILE_PATH in case of multiple product support. because wave410 and coda980 has different firmware binary format.

#define CODA960_BITCODE_PATH                "coda960.out"
#define CODA980_BITCODE_PATH                "coda980.out"
#define WAVE637_BITCODE_PATH                "seurat.bin"
#define WAVE627_BITCODE_PATH                "seurat.bin"
#define WAVE521C_DUAL_BITCODE_PATH          "chagall_dual.bin"
#define WAVE521E1_BITCODE_PATH              "degas_e1.bin"
#define WAVE537_BITCODE_PATH                "vincent_dual.bin"
#define WAVE521_BITCODE_PATH                "chagall.bin"
#define WAVE517_BITCODE_PATH                "vincent.bin"
#define WAVE511_BITCODE_PATH                "chagall.bin"


//==============================================================================
//
// Product Configuration
//
//==============================================================================

//------------------------------------------------------------------------------
// OMX
//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
// CODA980
//------------------------------------------------------------------------------
#define CLIP_PIC_DELTA_QP




#undef SUPPORT_GDB

#if (defined(WAVE627) || defined(WAVE637)) || defined(WAVE624) || defined(WAVE667) || defined(WAVE677)
//------------------------------------------------------------------------------
// WAVE6
//------------------------------------------------------------------------------
//#define SUPPORT_READ_BITSTREAM_IN_ENCODER



#if defined(WAVE624)
#endif
#if defined(WAVE677)
#endif
#if defined(WAVE667)
#endif
#endif /* WAVE6 */

//------------------------------------------------------------------------------
// WAVE521C
//------------------------------------------------------------------------------
//#define SUPPORT_SOURCE_RELEASE_INTERRUPT
//#define SUPPORT_READ_BITSTREAM_IN_ENCODER





//------------------------------------------------------------------------------
// WAVE517
//------------------------------------------------------------------------------



//#define SUPPORT_SW_UART
//#define SUPPORT_SW_UART_V2	// WAVE511 or WAVE521C or WAVE6
// #define SUPPORT_SW_UART_ON_NONOS
#ifdef SUPPORT_SW_UART_ON_NONOS
    #if (!defined(SUPPORT_SW_UART) && !defined(SUPPORT_SW_UART_V2))
        #error "SUPPORT_SW_UART_ON_NONOS define needs (#if defined(SUPPORT_SW_UART_V2) || defined(SUPPORT_SW_UART))"
    #endif
#endif





#endif /* __CONFIG_H__ */

