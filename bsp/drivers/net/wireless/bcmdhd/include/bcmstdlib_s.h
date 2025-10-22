/*
 * Broadcom Secure Standard Library.
 *
 * Copyright (C) 2024 Synaptics Incorporated. All rights reserved.
 *
 * This software is licensed to you under the terms of the
 * GNU General Public License version 2 (the "GPL") with Broadcom special exception.
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS," AND SYNAPTICS
 * EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES, INCLUDING ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY INTELLECTUAL PROPERTY RIGHTS.
 * IN NO EVENT SHALL SYNAPTICS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, PUNITIVE, OR CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED
 * AND BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF COMPETENT JURISDICTION
 * DOES NOT PERMIT THE DISCLAIMER OF DIRECT DAMAGES OR ANY OTHER DAMAGES,
 * SYNAPTICS' TOTAL CUMULATIVE LIABILITY TO ANY PARTY SHALL NOT
 * EXCEED ONE HUNDRED U.S. DOLLARS
 *
 * Copyright (C) 2024, Broadcom.
 *
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 *
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 *
 *
 * <<Broadcom-WL-IPTag/Dual:>>
 */

#ifndef	_bcmstdlib_s_h_
#define	_bcmstdlib_s_h_

#ifndef BWL_NO_INTERNAL_STDLIB_S_SUPPORT
#if !defined(__STDC_WANT_SECURE_LIB__) && !(defined(__STDC_LIB_EXT1__) && \
	defined(__STDC_WANT_LIB_EXT1__))
extern int memmove_s(void *dest, size_t destsz, const void *src, size_t n);
extern int memcpy_s(void *dest, size_t destsz, const void *src, size_t n);
extern int memset_s(void *dest, size_t destsz, int c, size_t n);
#endif /* !__STDC_WANT_SECURE_LIB__ && !(__STDC_LIB_EXT1__ && __STDC_WANT_LIB_EXT1__) */
#if !defined(FREEBSD) && !defined(MACOSX) && !defined(BCM_USE_PLATFORM_STRLCPY)
extern size_t strlcpy(char *dest, const char *src, size_t size);
#endif /* !defined(FREEBSD) && !defined(MACOSX) && !defined(BCM_USE_PLATFORM_STRLCPY) */
extern size_t strlcat_s(char *dest, const char *src, size_t size);

/* Remap xxx_s() APIs to use compiler builtin functions for C standard library functions.
 * The intent is to identify buffer overflow at compile-time for the safe stdlib APIs when
 * the user-specified destination buffer-size is incorrect.
 *
 * This is only intended as a compile-time test, and should be used by compile-only targets.
 */
#if defined(BCM_STDLIB_S_BUILTINS_TEST)
#define memmove_s(dest, destsz, src, n) ((void)(destsz), (int)__builtin_memmove((dest), (src), (n)))
#define memcpy_s(dest, destsz, src, n)  ((void)(destsz), (int)__builtin_memcpy((dest), (src), (n)))
#define memset_s(dest, destsz, c, n)    ((void)(destsz), (int)__builtin_memset((dest), (c), (n)))
#define strlcpy(dest, src, size)        ((void)(size), (size_t)__builtin_strcpy((dest), (src)))
#define strlcat_s(dest, src, size)      ((void)(size), (size_t)__builtin_strcat((dest), (src)))
#endif /* BCM_STDLIB_S_BUILTINS_TEST */

#endif /* !BWL_NO_INTERNAL_STDLIB_S_SUPPORT */
#endif /* _bcmstdlib_s_h_ */
