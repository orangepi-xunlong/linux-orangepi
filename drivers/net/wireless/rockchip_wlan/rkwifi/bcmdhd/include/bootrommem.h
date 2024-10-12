/*
 * bootrommem.h
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

/* This header defines structures shared between dongle and host */

#ifndef _BOOTROMMEM_H_
#define _BOOTROMMEM_H_

#include <typedefs.h>
#ifdef BCMDRIVER
#include <osl.h>
#else
#include <stddef.h>
#endif

#if defined(TEST_BOOTROM_BUILD)
typedef uint8 *bl_ptr_t;
#else
typedef uint32 bl_ptr_t;
#endif /* TEST_BOOROM_BUILD */

/* Memory layout
 * Addresses have to be uint32 instead of uint8* because the host and dongle
 * may have different pointer sizes.
 */
struct bl_mem_region {
	bl_ptr_t start;	/* start of region */
	bl_ptr_t end;	/* location after the region */
};
typedef struct bl_mem_region bl_mem_region_t;

#define BL_MEM_REGION_LEN(_r) ((_r)->end > (_r)->start ? \
	(_r)->end - (_r)->start : 0)

/* memory regions  - not all of these are included in signature */
struct bl_mem_info {
	bl_mem_region_t reset_vec;	/* read-only - jmp to BL */
	bl_mem_region_t int_vec;	/* copied from RAM, jmp here on success */
	/* ... */
	bl_mem_region_t rom;		/* Bootloader at ROM start */
	/* ... */
	/* start of RAM */
	bl_mem_region_t mmap;		/* struct/memory map written by host/dhd */
	bl_mem_region_t vstatus;	/* verification status */
	/* ... */
	bl_mem_region_t firmware;	/* firmware data and code */
	bl_mem_region_t signature;	/* signature and signing information */
	bl_mem_region_t heap;		/* region for heap allocations */
	/* ... */
	bl_mem_region_t stack;		/* stack allocations region */
	bl_mem_region_t prng;		/* PRNG data - may be 0 len */
	bl_mem_region_t nvram;		/* nvram data */
	/* end of RAM */
};
typedef struct bl_mem_info bl_mem_info_t;

typedef uint16 bl_state_t;
typedef uint32 bl_counter_t;

/* verification status - populated on return  */
struct bl_verif_status {
	bl_status_t		status;
	bl_state_t		state;
	uint16			alloc_fail_size;	/* Size of the failled alloc request */
	bl_counter_t		alloc_bytes;		/* currently allocated */
	bl_counter_t		max_alloc_bytes;	/* max ever  allocated */
	bl_counter_t		total_alloc_bytes;	/* running count of allocated */
	bl_counter_t		total_freed_bytes;	/* running count of freed */
	bl_counter_t		num_allocs;		/* number of allocations */
	bl_counter_t		max_allocs;		/* max number of allocations */
	bl_counter_t		max_alloc_size;		/* max allocation size */
	bl_counter_t		alloc_failures;		/* allocation failure count */
	/* add other information as necessary */
};
typedef struct bl_verif_status bl_verif_status_t;

#endif /* _BOOTROMMEM_H_ */
