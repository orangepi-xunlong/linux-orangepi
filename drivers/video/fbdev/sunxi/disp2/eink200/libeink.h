/*
 * libeink.h
 *
 * Copyright (c) 2007-2021 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef _LIBEINK_H
#define _LIBEINK_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * E Ink - pixel process library
 *
 * Copyright (C) 2020 E Ink Holdings Inc.
 *
 */


/**
 * awf:
 *     The input awff file for reference
 * return: 0 , Success
 *         others , Fail
 */
int EInk_Init(char *awf);

/**
 * image:
 *     The Y8 image to be processed, the content may be modifed.
 *
 * previous:
 *     The previous Y8 image, which's content won't be modified.
 *
 * width, height:
 *     The dimension of both images.
 *     Assume the stride == width.
 *
 * Note:
 *     Do not apply other image prcossing methods after eink_process().
 *
 */
void eink_process(uint8_t *image, uint8_t *previous, uint32_t width,
		  uint32_t height);
/**
 * NEON version of eink_process
 *
 * The caller should do add some code like following sample:
 * -------------------------------------
 *    #include <asm/neon.h>
 *
 *    kernel_neon_begin();
 *    eink_process_index_neon(index, width, height);
 *    kernel_neon_end();
 * -------------------------------------
 *
 * Refer to Documentation/arm/kernel_mode_neon.txt
 */
void eink_process_neon(uint8_t *image, uint8_t *previous, uint32_t width,
		       uint32_t height);

/**
 * index:
 *     Combined image, previous image (bit 5~9) and new image (bit 0~4).
 *     The new image (bit 0~4) may be modified.
 *
 * width, height:
 *     The dimension of both images.
 *     Assume the stride == width.
 *
 */
void eink_process_index(uint16_t *index, uint32_t width, uint32_t height);

/**
 * NEON version of eink_process_index
 *
 * The caller should do add some code like following sample:
 * -------------------------------------
 *    #include <asm/neon.h>
 *
 *    kernel_neon_begin();
 *    eink_process_index_neon(index, width, height);
 *    kernel_neon_end();
 * -------------------------------------
 *
 * Refer to Documentation/arm/kernel_mode_neon.txt
 */
void eink_process_index_neon(uint16_t *index, uint32_t width, uint32_t height);

/**
 * For color sense
 *
 * This MUST cooperate with specific waveform.
 */
void eink_process_index_color_neon(uint8_t *color, uint16_t *index,
				   uint32_t width, uint32_t height);

/**
 *
 * return:
 *     The build date in decimal number, Ex: 20200225 => Feb 25, 2020.
 */
uint32_t eink_get_version(void);

#ifdef __cplusplus
}
#endif

#endif /*End of file*/
