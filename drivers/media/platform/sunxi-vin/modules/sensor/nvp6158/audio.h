/*
 * A V4L2 driver for nvp6158c yuv cameras.
 *
 * Copyright (c) 2019 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zheng Zequn<zequnzheng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _AUDIO_H_
#define _AUDIO_H_

/********************************************************************
 *  define and enum
 ********************************************************************/
#define AIG_DEF   0x08
#define AOG_DEF   0x08

/********************************************************************
 *  structure
 ********************************************************************/

/********************************************************************
 *  external api
 ********************************************************************/
extern void audio_init(unsigned char recmaster, unsigned char pbmaster, unsigned char ch_num, unsigned char samplerate, unsigned char bits);
extern void nvp6168_audio_init(unsigned char recmaster, unsigned char pbmaster, unsigned char ch_num, unsigned char samplerate, unsigned char bits);
extern void audio_powerdown(unsigned char chip);
/* Add for Raptor4 */
void audio_in_type_set(int type);
int audio_in_type_get(void);
void audio_sample_rate_set(int sample);
int audio_sample_rate_get(void);
void audio_re_initialize(int devnum);
void audio_set_aoc_format(decoder_dev_ch_info_s *decoder_info);

#endif	/* End of _AUDIO_H_ */

/********************************************************************
 *  End of file
 ********************************************************************/

