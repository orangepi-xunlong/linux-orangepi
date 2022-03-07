/*
 * Kernel based bootsplash.
 *
 * (File format)
 *
 * Authors:
 * Max Staudt <mstaudt@suse.de>
 *
 * SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
 */

#ifndef __BOOTSPLASH_FILE_H
#define __BOOTSPLASH_FILE_H


#define BOOTSPLASH_VERSION 55561


#include <linux/kernel.h>
#include <linux/types.h>


/*
 * On-disk types
 *
 * A splash file consists of:
 *  - One single 'struct splash_file_header'
 *  - An array of 'struct splash_pic_header'
 *  - An array of raw data blocks, each padded to 16 bytes and
 *    preceded by a 'struct splash_blob_header'
 *
 * A single-frame splash may look like this:
 *
 * +--------------------+
 * |                    |
 * | splash_file_header |
 * |  -> num_blobs = 1  |
 * |  -> num_pics = 1   |
 * |                    |
 * +--------------------+
 * |                    |
 * | splash_pic_header  |
 * |                    |
 * +--------------------+
 * |                    |
 * | splash_blob_header |
 * |  -> type = 0       |
 * |  -> picture_id = 0 |
 * |                    |
 * | (raw RGB data)     |
 * | (pad to 16 bytes)  |
 * |                    |
 * +--------------------+
 *
 * All multi-byte values are stored on disk in the native format
 * expected by the system the file will be used on.
 */
#define BOOTSPLASH_MAGIC_BE "Linux bootsplash"
#define BOOTSPLASH_MAGIC_LE "hsalpstoob xuniL"

struct splash_file_header {
	uint8_t  id[16]; /* "Linux bootsplash" (no trailing NUL) */

	/* Splash file format version to avoid clashes */
	uint16_t version;

	/* The background color */
	uint8_t bg_red;
	uint8_t bg_green;
	uint8_t bg_blue;
	uint8_t bg_reserved;

	/*
	 * Number of pic/blobs so we can allocate memory for internal
	 * structures ahead of time when reading the file
	 */
	uint16_t num_blobs;
	uint8_t num_pics;

	uint8_t unused_1;

	/*
	 * Milliseconds to wait before painting the next frame in
	 * an animation.
	 * This is actually a minimum, as the system is allowed to
	 * stall for longer between frames.
	 */
	uint16_t frame_ms;

	uint8_t padding[100];
} __attribute__((__packed__));


struct splash_pic_header {
	uint16_t width;
	uint16_t height;

	/*
	 * Number of data packages associated with this picture.
	 * Currently, the only use for more than 1 is for animations.
	 */
	uint8_t num_blobs;

	/*
	 * Corner to move the picture to / from.
	 *  0x00 - Top left
	 *  0x01 - Top
	 *  0x02 - Top right
	 *  0x03 - Right
	 *  0x04 - Bottom right
	 *  0x05 - Bottom
	 *  0x06 - Bottom left
	 *  0x07 - Left
	 *
	 * Flags:
	 *  0x10 - Calculate offset from the corner towards the center,
	 *         rather than from the center towards the corner
	 */
	uint8_t position;

	/*
	 * Pixel offset from the selected position.
	 * Example: If the picture is in the top right corner, it will
	 *          be placed position_offset pixels from the top and
	 *          position_offset pixels from the right margin.
	 */
	uint16_t position_offset;

	/*
	 * Animation type.
	 *  0 - off
	 *  1 - forward loop
	 */
	uint8_t anim_type;

	/*
	 * Animation loop point.
	 * Actual meaning depends on animation type:
	 * Type 0 - Unused
	 *      1 - Frame at which to restart the forward loop
	 *          (allowing for "intro" frames)
	 */
	uint8_t anim_loop;

	uint8_t padding[22];
} __attribute__((__packed__));


struct splash_blob_header {
	/* Length of the data block in bytes. */
	uint32_t length;

	/*
	 * Type of the contents.
	 *  0 - Raw RGB data.
	 */
	uint16_t type;

	/*
	 * Picture this blob is associated with.
	 * Blobs will be added to a picture in the order they are
	 * found in the file.
	 */
	uint8_t picture_id;

	uint8_t padding[9];
} __attribute__((__packed__));




/*
 * Enums for on-disk types
 */
enum splash_position {
	SPLASH_CORNER_TOP_LEFT = 0,
	SPLASH_CORNER_TOP = 1,
	SPLASH_CORNER_TOP_RIGHT = 2,
	SPLASH_CORNER_RIGHT = 3,
	SPLASH_CORNER_BOTTOM_RIGHT = 4,
	SPLASH_CORNER_BOTTOM = 5,
	SPLASH_CORNER_BOTTOM_LEFT = 6,
	SPLASH_CORNER_LEFT = 7,
	SPLASH_POS_FLAG_CORNER = 0x10,
};

enum splash_anim_type {
	SPLASH_ANIM_NONE = 0,
	SPLASH_ANIM_LOOP_FORWARD = 1,
};

#endif
