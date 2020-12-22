/*
 * sound\soc\codecs\tas5731.h
 * (C) Copyright 2010-2016
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * huangxin <huangxin@reuuimllatech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */
#ifndef _SNDTAS5731_H
#define _SNDTAS5731_H

#ifdef TAS5731
    #define TAS5731_DEG(format,args...)  printk("[AC100] "format,##args)
#else
    #define TAS5731_DEG(...)
#endif

#define Clock_ctl      0x00
#define Serial_data    0x03
#define Soft_mute      0x06
#define Master_vol     0x07
#define Channel1_vol   0x08
#define Channel2_vol   0x09
#define Channel3_vol   0x0a
#define Volume_ctl     0x0e
#define Osc_trim       0x1b

typedef unsigned char cfg_u8;

typedef union {
    struct {
        cfg_u8 offset;
        cfg_u8 value;
    };
    struct {
        cfg_u8 command;
        cfg_u8 param;
    };
} cfg_reg;

//set channel volume
typedef enum {
   CH_1  = 0,
   CH_2  = 1,
   CH_3  = 2,
} ch_num;

enum {
    TAS5711_SET_MASTER_VOLUME = 0,
    TAS5711_GET_MASTER_VOLUME = 1,
    TAS5711_SET_POWER          = 2,
    TAS5711_SET_MUTE           = 3,
    TAT5711_CMD_MAX_NUM        = 252,
};

//cmd
#define CFG_META_SWITCH (255)
#define CFG_META_DELAY  (254)
#define CFG_META_BURST  (253)
#endif
