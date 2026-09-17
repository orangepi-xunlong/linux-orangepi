/*
 * Firmware package functionality
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

#ifndef BCMDRIVER
#include <stdio.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 16, 0))
#include <stdarg.h>
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(5, 16, 0) */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#endif /* BCMDRIVER */

#include <typedefs.h>
#include <bcmutils.h>
#include <bcmendian.h>
#include <bcmstdlib_s.h>
#include <dhd.h>
#ifdef BCMDRIVER
#include <dhd_dbg.h>
#else
#include <errno.h>
#endif /* BCMDRIVER */
#include <fwpkg_utils.h>

#define FWPKG_UNIT_IDX(tag)	(tag-1)

const uint32 FWPKG_HDR_MGCW0 = 0xDEAD2BAD;
const uint32 FWPKG_HDR_MGCW1 = 0xFEE1DEAD;
const uint32 FWPKG_MAX_SUPPORTED_VER = 1;

#ifdef DHD_LINUX_STD_FW_API
#define FWPKG_ERR(msg)	DHD_ERROR(msg)
FWPKG_FILE *
fwpkg_request_firmware(FWPKG_FILE **fw, char *filename)
{
	if (dhd_os_get_img_fwreq(fw, filename) < 0)
		*fw = NULL;
	return *fw;
}

#define fwpkg_open(file, filename)	\
	fwpkg_request_firmware(file, filename)
#define fwpkg_seek(file, offset)	\
	BCME_OK
#define fwpkg_close(file)	\
	release_firmware(file)
#define fwpkg_getsize(file)	\
	((file)->size)
#else
#ifdef BCMDRIVER
#define FWPKG_ERR(msg)	DHD_ERROR(msg)
#define fwpkg_open(file, filename)	\
	dhd_os_open_image1(NULL, filename)
#define fwpkg_close(file)	\
	dhd_os_close_image1(NULL, (char *)file)
#define fwpkg_seek(file, offset)	\
	dhd_os_seek_file(file, offset)
#define fwpkg_read(buf, len, file)	\
	dhd_os_get_image_block(buf, len, file)
#define fwpkg_getsize(file)	\
	dhd_os_get_image_size(file)

#else
#if defined(_WIN32)
#define FWPKG_ERR(fmt, ...)
#else
#define FWPKG_ERR(fmt, args...)
#endif /* defined _WIN32 */
#define fwpkg_open(file, filename)	\
	(void *)fopen(filename, "rb")
#define fwpkg_close(file)	\
	fclose((FILE *)file)
#define fwpkg_seek(file, offset)	\
	fseek((FILE *)file, offset, SEEK_SET)
#define fwpkg_read(buf, len, file)	\
	app_fread(buf, len, file)
#define fwpkg_getsize(file)	\
	app_getsize(file)

#endif /* BCMDRIVER */
#endif /* DHD_LINUX_STD_FW_API */

#define UNIT_TYPE_IDX(type)	((type) - 1)

static int fwpkg_hdr_validation(fwpkg_hdr_t *hdr, uint32 pkg_len);
static bool fwpkg_parse_rtlvs(fwpkg_info_t *fwpkg, FWPKG_FILE *file, uint32 file_size);
static int fwpkg_parse(fwpkg_info_t *fwpkg, char *fname, FWPKG_FILE *file);
static int fwpkg_open_unit(fwpkg_info_t *fwpkg, char *fname,
	uint32 unit_type, FWPKG_FILE **fp);
static int fwpkg_open_file(fwpkg_info_t *fwpkg, char *fname, FWPKG_FILE **fp);
static uint32 fwpkg_get_unit_size(fwpkg_info_t *fwpkg, uint32 unit_type);
static int fwpkg_get_unit_block(FWPKG_FILE *fp, fwpkg_info_t *fwpkg, uint32 unit_type,
	char *buf, int size, int offset);
static int fwpkg_get_file_array(FWPKG_FILE *file, int offset, uint8 *buf, int len);

/* open file, if combined fw package parse common header,
 * parse each unit header, keep information
 */
static int
fwpkg_init(fwpkg_info_t *fwpkg, char *fname, FWPKG_FILE **fp)
{
	/* if status already set, no need to initialize */
	if (fwpkg->status) {
		return BCME_OK;
	}
	bzero(fwpkg, sizeof(*fwpkg));

	return fwpkg_parse(fwpkg, fname, *fp);
}

int
fwpkg_open_firmware_img(FWPKG_FILE **fp, fwpkg_info_t *fwpkg,
	uint32 unit_type, char *fname, const char *caller)
{
	int ret = BCME_ERROR;
	int file_size, unit_size = 0;

	if (fwpkg == NULL || fname == NULL) {
		FWPKG_ERR(("%s: missing argument\n", __FUNCTION__));
		return BCME_ERROR;
	}

	if (unit_type <= FWPKG_TAG_ZERO || unit_type >= FWPKG_TAG_LAST)
		fwpkg->status = FWPKG_SINGLE_FLG;

	if (fwpkg->status)
		ret = fwpkg_open_unit(fwpkg, fname, unit_type, fp);
	else {
		ret = fwpkg_open_file(fwpkg, fname, fp);
		if (!ret)
			ret = fwpkg_init(fwpkg, fname, fp);
	}
	if (ret)
		goto err;

	file_size = fwpkg_getsize(*fp);
	unit_size = fwpkg_get_unit_size(fwpkg, unit_type);
	if (IS_FWPKG_COMBND(fwpkg))
		FWPKG_ERR(("%s(%s): %s (COMBINED image %d bytes, unit %d bytes)\n",
			__FUNCTION__, caller, fname, file_size, unit_size));
	else
		FWPKG_ERR(("%s(%s): %s (SINGLE image %d bytes)\n",
			__FUNCTION__, caller, fname, file_size));

err:
	return unit_size;
}

void
fwpkg_close_firmware_img(FWPKG_FILE *fp)
{
	if (fp)
		return fwpkg_close(fp);
}

/* This function is one difference with dhd_os_get_image_block() if DHD_LINUX_STD_FW_API NOT defined,
  * dhd_os_get_image_block() will move the fp->f_pos += rdlen after calling,
  * but this function will move back to original fp->f_pos, so please assign right offset for the read.
 */
int
fwpkg_get_firmware_img_block(FWPKG_FILE *fp, fwpkg_info_t *fwpkg,
	uint32 unit_type, char *buf, int size, int offset)
{
	if (fwpkg == NULL) {
		FWPKG_ERR(("%s: missing argument\n", __FUNCTION__));
		return 0;
	}

	return fwpkg_get_unit_block(fp, fwpkg, unit_type, buf, size, offset);
}

bool
fwpkg_unit_inside(fwpkg_info_t *fwpkg, uint32 unit_type)
{
	fwpkg_unit_t *fw_unit = NULL;

	if (fwpkg == NULL) {
		FWPKG_ERR(("%s: missing argument\n", __FUNCTION__));
		return FALSE;
	}

	if (IS_FWPKG_COMBND(fwpkg)) {
		fw_unit = &fwpkg->units[FWPKG_UNIT_IDX(unit_type)];
		if (fw_unit->type == unit_type)
			return TRUE;
	}

	return FALSE;
}

#ifndef BCMDRIVER
static int
app_getsize(FILE *file)
{
	int len = 0;

	fseek(file, 0, SEEK_END);
	len = (int)ftell(file);
	fseek(file, 0, SEEK_SET);

	return len;
}

static int
app_fread(char *buf, int len, void *file)
{
	int status;

	status = fread(buf, sizeof(uint8), len, (FILE *)file);
	if (status < len) {
		status = -EINVAL;
	}

	return status;
}
#endif /* !BCMDRIVER */

/* check package header information */
static int
fwpkg_hdr_validation(fwpkg_hdr_t *hdr, uint32 pkg_len)
{
	int ret = BCME_ERROR;
	uint32 len = 0;

	/* check megic word0 */
	len += sizeof(hdr->magic_word0);
	if (hdr->magic_word0 != FWPKG_HDR_MGCW0) {
		ret = BCME_UNSUPPORTED;
		goto done;
	}
	/* check megic word1 */
	len += sizeof(hdr->magic_word1);
	if (hdr->magic_word1 != FWPKG_HDR_MGCW1) {
		goto done;
	}
	/* check length */
	len += sizeof(hdr->length);
	if (hdr->length != (pkg_len - len)) {
		goto done;
	}
	/* check version */
	if (hdr->version > FWPKG_MAX_SUPPORTED_VER) {
		goto done;
	}

	ret = BCME_OK;
done:
	return ret;
}

/* parse rtlvs in combined fw package */
static bool
fwpkg_parse_rtlvs(fwpkg_info_t *fwpkg, FWPKG_FILE *file, uint32 file_size)
{
	bool ret = FALSE;
	const uint32 l_len = sizeof(uint32);		/* len of rTLV's field length */
	const uint32 t_len = sizeof(uint32);		/* len of rTLV's field type */
	uint32 unit_size = 0, unit_type = 0;
	uint32 left_size = file_size - sizeof(fwpkg_hdr_t);

	while (left_size) {
		/* remove length of rTLV's fields type, length */
		left_size -= (t_len + l_len);
		if (fwpkg_get_file_array(file, left_size, (char *)&unit_size, l_len) < 0) {
			FWPKG_ERR(("fwpkg_parse_rtlvs: can't read rtlv data len\n"));
			goto done;
		}
		if (fwpkg_get_file_array(file, left_size + l_len, (char *)&unit_type, t_len) < 0) {
			FWPKG_ERR(("fwpkg_parse_rtlvs: can't read rtlv data type\n"));
			goto done;
		}
		if ((unit_type == FWPKG_TAG_ZERO) || (unit_type >= FWPKG_TAG_LAST)) {
			FWPKG_ERR(("fwpkg_parse_rtlvs: unsupported data type(%d)\n",
				unit_type));
			goto done;
		}
		fwpkg->units[UNIT_TYPE_IDX(unit_type)].type = unit_type;
		fwpkg->units[UNIT_TYPE_IDX(unit_type)].size = unit_size;
		left_size -= unit_size;
		fwpkg->units[UNIT_TYPE_IDX(unit_type)].offset = left_size;

		FWPKG_ERR(("fwpkg_parse_rtlvs: type %d, len 0x%08X (%d bytes), off %d\n",
			unit_type, unit_size, unit_size, left_size));
	}

	ret = TRUE;
done:
	return ret;
}

/* parse file if is combined fw package */
static int
fwpkg_parse(fwpkg_info_t *fwpkg, char *fname, FWPKG_FILE *file)
{
	int ret = BCME_ERROR;
	int file_size = 0;
	fwpkg_hdr_t hdr = {0};
	int offset;

	file_size = fwpkg_getsize(file);
	if (!file_size) {
		FWPKG_ERR(("fwpkg_parse: get file size fails\n"));
		goto done;
	}
	
	/* seek to the last sizeof(fwpkg_hdr_t) bytes in the file */
	offset = file_size - sizeof(fwpkg_hdr_t);
	/* read the last sizeof(fwpkg_hdr_t) bytes of the file to a buffer */
	if (fwpkg_get_file_array(file, offset, (char *)&hdr, sizeof(fwpkg_hdr_t)) < 0) {
		FWPKG_ERR(("fwpkg_parse: can't read from the pkg header offset\n"));
		goto done;
	}

	/* if combined firmware package validates it's common header
	 * otherwise return BCME_UNSUPPORTED as it may be
	 * another type of firmware binary
	 */
	ret = fwpkg_hdr_validation(&hdr, file_size);
	if (ret == BCME_ERROR) {
		FWPKG_ERR(("fwpkg_parse: can't parse pkg header\n"));
		goto done;
	}
	/* parse rTLVs only in case of combined firmware package */
	if (ret == BCME_OK) {
		fwpkg->status = FWPKG_COMBND_FLG;
		if (fwpkg_parse_rtlvs(fwpkg, file, file_size) == FALSE) {
			FWPKG_ERR(("fwpkg_parse: can't parse rtlvs\n"));
			ret = BCME_ERROR;
			goto done;
		}
	} else {
		fwpkg->status = FWPKG_SINGLE_FLG;
		ret = BCME_OK;
	}

	fwpkg->file_size = file_size;
done:
	return ret;
}

/*
 * opens the package file and seek to requested unit.
 * in case of single binary file just open the file.
 */
static int
fwpkg_open_unit(fwpkg_info_t *fwpkg, char *fname, uint32 unit_type, FWPKG_FILE **fp)
{
	int ret = BCME_OK;
	fwpkg_unit_t *fw_unit = NULL;

	if (IS_FWPKG_COMBND(fwpkg)) {
		fw_unit = &fwpkg->units[FWPKG_UNIT_IDX(unit_type)];
		if (fw_unit->type != unit_type)
			return BCME_ERROR;
	}

	*fp = fwpkg_open(fp, fname);
	if (*fp == NULL) {
		ret = BCME_ERROR;
		goto done;
	}

	/* successfully opened file while status is FWPKG_SINGLE_FLG
	 * means, this is single binary file format.
	 */
	if (IS_FWPKG_SINGLE(fwpkg)) {
		fwpkg->file_size = fwpkg_getsize(*fp);
		goto done;
	}

	fw_unit = &fwpkg->units[FWPKG_UNIT_IDX(unit_type)];

	/* seek to the last sizeof(fwpkg_hdr_t) bytes in the file */
	if (fwpkg_seek(*fp, fw_unit->offset) != BCME_OK) {
		FWPKG_ERR(("fwpkg_open_unit: can't get to the pkg header offset\n"));
		ret = BCME_ERROR;
		goto done;
	}

done:
	return ret;
}

static int
fwpkg_open_file(fwpkg_info_t *fwpkg, char *fname, FWPKG_FILE **fp)
{
	int ret = BCME_OK;
	int file_size = 0;

	*fp = fwpkg_open(fp, fname);
	if (*fp == NULL) {
		return BCME_ERROR;
	}

	file_size = fwpkg_getsize(*fp);
	if (!file_size) {
		FWPKG_ERR(("%s: get file size fails\n", __FUNCTION__));
		return BCME_ERROR;
	}

	return ret;
}

static uint32
fwpkg_get_unit_size(fwpkg_info_t *fwpkg, uint32 unit_type)
{
	fwpkg_unit_t *fw_unit = NULL;
	uint32 size;

	if (IS_FWPKG_SINGLE(fwpkg)) {
		size = fwpkg->file_size;
		goto done;
	}

	fw_unit = &fwpkg->units[FWPKG_UNIT_IDX(unit_type)];
	size = fw_unit->size;

done:
	return size;
}

#ifdef DHD_LINUX_STD_FW_API
static uint32
fwpkg_get_unit_offset(fwpkg_info_t *fwpkg, uint32 unit_type)
{
	fwpkg_unit_t *fw_unit = NULL;
	uint32 offset;

	if (IS_FWPKG_SINGLE(fwpkg)) {
		offset = 0;
		goto done;
	}

	fw_unit = &fwpkg->units[FWPKG_UNIT_IDX(unit_type)];
	offset = fw_unit->offset;

done:
	return offset;
}
#endif /* DHD_LINUX_STD_FW_API */

static int
fwpkg_get_unit_block(FWPKG_FILE *fp, fwpkg_info_t *fwpkg, uint32 unit_type,
	char *buf, int size, int offset)
{
#ifdef DHD_LINUX_STD_FW_API
	int unit_offset;
#endif /* DHD_LINUX_STD_FW_API */
	int seek_len = 0;
	int rdlen, remain, ret = 0;

	if (!fp) {
		return BCME_NOTFOUND;
	}

	remain = fwpkg_get_unit_size(fwpkg, unit_type) - offset;
	rdlen = MIN(size, remain);

	if (offset < 0 || size <= 0 || rdlen <= 0) {
		FWPKG_ERR(("%s: remain %d, offset %u, size %d\n",
			__FUNCTION__, remain, offset, size));
		return -EIO;
	}

	if (fwpkg_seek(fp, offset) != BCME_OK) {
		ret = BCME_ERROR;
		goto exit;
	}
	seek_len = offset;

#ifdef DHD_LINUX_STD_FW_API
	unit_offset = fwpkg_get_unit_offset(fwpkg, unit_type);
	memcpy(buf, fp->data + (unit_offset + offset), rdlen);
#else
	/* offset to the unit after file open in fwpkg_open_unit(), so no need to offset to the unit again */
	rdlen = fwpkg_read(buf, size, fp);
	if (rdlen < 0) {
		ret = -EIO;
		goto exit;
	}
#endif /* DHD_LINUX_STD_FW_API */
	ret = rdlen;
	seek_len += rdlen;

exit:
	/* offset the pointer back to original place */
	if (fwpkg_seek(fp, -seek_len) != BCME_OK) {
		ret = BCME_ERROR;
	}

	return ret;
}

static int
fwpkg_get_file_array(FWPKG_FILE *file, int offset, uint8 *buf, int len)
{
	int file_size, rdlen = 0, seek_len = 0, ret = BCME_OK;

	if (!file) {
		return BCME_NOTFOUND;
	}

	file_size = fwpkg_getsize(file);

	if (offset < 0 || file_size < (offset + len)) {
		FWPKG_ERR(("%s: invalid file_size %u, offset %u, len %d\n",
			__FUNCTION__, file_size, offset, len));
		return BCME_BUFTOOSHORT;
	}

	/* offset the pointer for next read */
	if (fwpkg_seek(file, offset) != BCME_OK) {
		ret = BCME_ERROR;
		goto exit;
	}
	seek_len = offset;

	memset(buf, 0, len);
#ifdef DHD_LINUX_STD_FW_API
	memcpy(buf, file->data + offset, len);
	rdlen = len;
#else
	rdlen = fwpkg_read(buf, len, file);
	if (rdlen < 0) {
		ret = -EIO;
		goto exit;
	}
#endif /* DHD_LINUX_STD_FW_API */
	seek_len += rdlen;

exit:
	/* offset the pointer back to original place */
	if (fwpkg_seek(file, -seek_len) != BCME_OK) {
		ret = BCME_ERROR;
	}

	return ret;
}

