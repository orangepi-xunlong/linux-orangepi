/*
 * Firmware package defines
 *
 * Copyright (C) 2026 Synaptics Incorporated. All rights reserved.
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
 * Copyright (C) 2026, Broadcom.
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

#ifndef _fwpkg_utils_h_
#define _fwpkg_utils_h_
#include <linux/firmware.h>

enum {
	FWPKG_TAG_ZERO	= 0,
	FWPKG_TAG_FW	= 1,
	FWPKG_TAG_SIG	= 2,
	FWPKG_TAG_INFO	= 3,
#ifdef SHOW_LOGTRACE
	FWPKG_TAG_LOGSTRS	= 4,
	FWPKG_TAG_RAM_MAP	= 5,
	FWPKG_TAG_ROM	= 6,
	FWPKG_TAG_ROM_MAP	= 7,
#endif /* SHOW_LOGTRACE */
	FWPKG_TAG_LAST
};
#define NBR_OF_FWPKG_UNITS	(FWPKG_TAG_LAST-1)

/* internal firmware package header */
typedef struct fwpkg_hdr {
	uint32 options;
	uint32 version;		/* fw package version */
	uint32 length;		/* length of data after the length field */
	uint32 magic_word0;	/* hardcoded value */
	uint32 magic_word1;	/* hardcoded value */
} fwpkg_hdr_t;

/* internal firmware package unit header */
typedef struct fwpkg_unit
{
	uint32	offset;		/* offset to the data in the file */
	uint32	size;		/* the data size */
	uint32  type;
} fwpkg_unit_t;

#define FWPKG_SINGLE_FLG	1U
#define FWPKG_COMBND_FLG	2U

typedef struct fwpkg_info
{
	fwpkg_unit_t units[NBR_OF_FWPKG_UNITS];
	uint32 file_size;
	uint8 status;
	uint8 pad[3];
} fwpkg_info_t;

#define IS_FWPKG_SINGLE(fwpkg)	\
	((fwpkg->status == FWPKG_SINGLE_FLG) ? TRUE : FALSE)
#define IS_FWPKG_COMBND(fwpkg)	\
	((fwpkg->status == FWPKG_COMBND_FLG) ? TRUE : FALSE)

#ifdef DHD_LINUX_STD_FW_API
#define FWPKG_FILE	const struct firmware
#else
#ifdef BCMDRIVER
#define FWPKG_FILE	struct file
#else
#define FWPKG_FILE	FILE
#endif /* BCMDRIVER */
#endif

int fwpkg_open_firmware_img(FWPKG_FILE **fp, fwpkg_info_t *fwpkg,
	uint32 unit_type, char *fname, const char *caller);
void fwpkg_close_firmware_img(FWPKG_FILE *fp);
int fwpkg_get_firmware_img_block(FWPKG_FILE *fp, fwpkg_info_t *fwpkg,
	uint32 unit_type, char *buf, int size, int offset);
bool fwpkg_unit_inside(fwpkg_info_t *fwpkg, uint32 unit_type);

#endif /* _fwpkg_utils_h_ */
