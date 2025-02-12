/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __CONFIG_H__
#define __CONFIG_H__

#if defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(WIN32) || defined(__MINGW32__)
#	define PLATFORM_WIN32
#elif defined(linux) || defined(__linux) || defined(ANDROID)
#	define PLATFORM_LINUX
#else
#	define PLATFORM_NON_OS
#endif

#if defined(CNM_FPGA_PLATFORM) || defined(CNM_SIM_PLATFORM)
#ifdef ANDROID
#else
#endif
#endif

#define API_VERSION 0x124

#endif /* __CONFIG_H__ */
