
/*
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
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

#define _NCO_C_

#include "../../physic_common/nand_common_interface.h"
#include "../nand_physic_inc.h"
#include <linux/sched.h>

extern void ndfc_dma_config_start(struct _nand_controller_info *nctri, u8 rw,
				  void *buff_addr, u32 len);
extern s32 ndfc_wait_dma_end(struct _nand_controller_info *nctri, u8 rw,
			     void *buff_addr, u32 len);

const u32 default_random_seed = 0x4a80;
const u32 random_seed[128] = {
    // 0        1      2       3        4      5        6       7       8 9
    0x2b75, 0x0bd0, 0x5ca3, 0x62d1, 0x1c93, 0x07e9, 0x2162, 0x3a72, 0x0d67,
    0x67f9, 0x1be7, 0x077d, 0x032f, 0x0dac, 0x2716, 0x2436, 0x7922, 0x1510,
    0x3860, 0x5287, 0x480f, 0x4252, 0x1789, 0x5a2d, 0x2a49, 0x5e10, 0x437f,
    0x4b4e, 0x2f45, 0x216e, 0x5cb7, 0x7130, 0x2a3f, 0x60e4, 0x4dc9, 0x0ef0,
    0x0f52, 0x1bb9, 0x6211, 0x7a56, 0x226d, 0x4ea7, 0x6f36, 0x3692, 0x38bf,
    0x0c62, 0x05eb, 0x4c55, 0x60f4, 0x728c, 0x3b6f, 0x2037, 0x7f69, 0x0936,
    0x651a, 0x4ceb, 0x6218, 0x79f3, 0x383f, 0x18d9, 0x4f05, 0x5c82, 0x2912,
    0x6f17, 0x6856, 0x5938, 0x1007, 0x61ab, 0x3e7f, 0x57c2, 0x542f, 0x4f62,
    0x7454, 0x2eac, 0x7739, 0x42d4, 0x2f90, 0x435a, 0x2e52, 0x2064, 0x637c,
    0x66ad, 0x2c90, 0x0bad, 0x759c, 0x0029, 0x0986, 0x7126, 0x1ca7, 0x1605,
    0x386a, 0x27f5, 0x1380, 0x6d75, 0x24c3, 0x0f8e, 0x2b7a, 0x1418, 0x1fd1,
    0x7dc1, 0x2d8e, 0x43af, 0x2267, 0x7da3, 0x4e3d, 0x1338, 0x50db, 0x454d,
    0x764d, 0x40a3, 0x42e6, 0x262b, 0x2d2e, 0x1aea, 0x2e17, 0x173d, 0x3a6e,
    0x71bf, 0x25f9, 0x0a5d, 0x7c57, 0x0fbe, 0x46ce, 0x4939, 0x6b17, 0x37bb,
    0x3e91, 0x76db};
const u32 random_seed_new[67] = {
    // 0              1           2           3           4          5
    // 6           7           8           9           10          11
    // 12          13          14          15
    0xab11cc9b, 0x2e1da61b, 0xf8372be7, 0xf2cdf49f, 0xd3d22f7f, 0x5be09d1c,
    0xf98e137e, 0x6f670345, 0xd91b5d72, 0xedec5375, 0x8a50e1d2, 0x167637db,
    0x3988baed, 0x5075b36e, 0x27ea9ab7, 0x49d5a266, 0x948e296e, 0xbfd518f3,
    0xdf01ec4a, 0x686e1bb1, 0xfa7ee2c9, 0x7397c076, 0xd275da40, 0xcf428cef,
    0xe1ab245b, 0x57e26a5e, 0x2ac04837, 0x99a115a6, 0xd3c92d47, 0x196d8a26,
    0x9a1a1aa9, 0xb8259e70, 0x547badc1, 0xd3ed8ca8, 0xa34a4e94, 0xe8bc32f0,
    0x3063210f, 0x4a5808d8, 0x94772728, 0x7d832e2b, 0xbe1626a2, 0xf046bea7,
    0x8c451487, 0xe8bcd15f, 0x555fdde8, 0x86006513, 0x4df771ed, 0xe391fbee,
    0xdbc1fd98, 0x164cfd33, 0x2eae0831, 0x57077929, 0x72cfee1f, 0x7b2a7fcc,
    0x39d47304, 0xafa49758, 0xc9c5e5a3, 0x45a5fa49, 0xe544e572, 0x7e8f78a6,
    0x42074860, 0x6a183bbc, 0x4d797600, 0x03be9b49, 0xb80dfafb, 0xf2dad720,
    0x9aaaa2c7};

const u8 rand_factor_1st_spare_data[][128] = {
    {0xf5, 0xf1, 0xa4, 0x74, 0xd2, 0x8b, 0x85, 0xb6, 0x5c, 0xd6, 0x7d, 0x69,
     0x75, 0x81, 0xf3, 0x82, 0x87, 0x2a, 0x49, 0x25, 0xa8, 0xa9, 0x3e, 0xd9,
     0xbf, 0xec, 0x2e, 0x34, 0x85, 0x00, 0x2a, 0x07, 0x5d, 0x92, 0x8f, 0x55,
     0xba, 0x82, 0x2e, 0x4e, 0xeb, 0x0c, 0x44, 0x5d, 0x1a, 0xc6, 0x79, 0xb2,
     0x9f, 0x92, 0x27, 0x24, 0xe3, 0xc1, 0xa5, 0x0c, 0xe8, 0x10, 0x76, 0xc5,
     0xe3, 0x7f, 0x69, 0x9f, 0x65, 0xfd, 0x9a, 0x79, 0x95, 0xef, 0x81, 0xcc,
     0x16, 0xdb, 0x14, 0x07, 0x91, 0x76, 0x53, 0x9e, 0x9f, 0xb7, 0xfb, 0x94,
     0x93, 0xdd, 0xbb, 0xc8, 0x47, 0x0e, 0x0e, 0x33, 0x9e, 0x40, 0x8d, 0x68,
     0xf1, 0xf5, 0xcf, 0x21, 0xeb, 0x79, 0xac, 0x4d, 0x2a, 0xe2, 0x28, 0xac,
     0x6e, 0x96, 0x50, 0x79, 0x9c, 0xe1, 0x26, 0xc7, 0x3e, 0x6f, 0x05, 0x01,
     0x5b, 0x7e, 0x2b, 0xa5, 0xf9, 0x59, 0x10, 0xcd},
    {0xac, 0xb5, 0x63, 0x52, 0xab, 0x77, 0xf2, 0xb2, 0x6a, 0xf0, 0xf5, 0xe7,
     0xd3, 0x9e, 0x0c, 0xb9, 0x67, 0x84, 0xec, 0x99, 0x56, 0xc0, 0x86, 0xfb,
     0x4d, 0x55, 0xe7, 0x73, 0xd1, 0x4d, 0xa3, 0x7e, 0xc4, 0xeb, 0x52, 0xc2,
     0xd3, 0xfd, 0xaa, 0xba, 0x75, 0x45, 0x68, 0x9b, 0xd8, 0xcc, 0x83, 0xb7,
     0x41, 0x8e, 0x6b, 0x56, 0x01, 0x87, 0xc4, 0x13, 0x13, 0x59, 0x88, 0xe0,
     0x54, 0x5b, 0xa2, 0x50, 0xf6, 0xb6, 0xbe, 0x85, 0xe2, 0x14, 0x8a, 0x1b,
     0xcb, 0x64, 0x05, 0x4f, 0x85, 0xb5, 0x68, 0x8d, 0x25, 0x38, 0x64, 0x30,
     0x46, 0xed, 0x29, 0x0b, 0x3f, 0xc9, 0x8c, 0x79, 0xbc, 0x58, 0xb9, 0x3e,
     0xca, 0xf1, 0x57, 0x91, 0x64, 0xb5, 0x15, 0xd8, 0xdf, 0xc7, 0x65, 0x6e,
     0x99, 0xeb, 0x04, 0x81, 0x60, 0x86, 0xc6, 0x42, 0xa7, 0x48, 0x87, 0x96,
     0x14, 0xc0, 0x06, 0xd7, 0xd3, 0xd6, 0x44, 0xbc},
    {0x47, 0x84, 0x3b, 0xa7, 0x1d, 0x27, 0xe3, 0xf6, 0xb9, 0x5e, 0xe1, 0x6e,
     0x67, 0xe0, 0x45, 0xa1, 0x22, 0x1f, 0x36, 0xdb, 0xfe, 0x7e, 0x90, 0x1a,
     0xb0, 0x8d, 0x5c, 0x57, 0xa3, 0xc0, 0x5f, 0x82, 0x39, 0x2d, 0xe4, 0xbf,
     0x33, 0xa1, 0x9c, 0xb4, 0x8f, 0xc5, 0x33, 0x79, 0x0b, 0x52, 0x62, 0x35,
     0xa8, 0xed, 0x5a, 0x9b, 0x89, 0x10, 0x7b, 0x45, 0x0e, 0xcc, 0x26, 0x53,
     0x49, 0x60, 0xae, 0x68, 0xab, 0xc1, 0xeb, 0xe2, 0xef, 0x4c, 0xe0, 0x15,
     0x4e, 0x5b, 0xcf, 0x42, 0xac, 0xe6, 0x3d, 0xa8, 0xa8, 0x76, 0x43, 0x6f,
     0xed, 0x99, 0xb3, 0x16, 0x72, 0xc4, 0x04, 0xd5, 0x68, 0x30, 0xa5, 0xae,
     0xc4, 0x87, 0x14, 0xd8, 0x4f, 0xe2, 0xbd, 0x35, 0x5f, 0x09, 0xde, 0xfd,
     0xec, 0x2e, 0x3c, 0xe2, 0x69, 0xc8, 0x9a, 0xd2, 0x50, 0x2c, 0x43, 0x80,
     0x3b, 0x20, 0x9f, 0x3b, 0x02, 0xba, 0x0c, 0x55},
    {0x3d, 0x77, 0x69, 0x7d, 0xbf, 0x66, 0x05, 0xf5, 0xef, 0xc4, 0x87, 0xca,
     0x1d, 0x68, 0xc5, 0xb2, 0xaa, 0x23, 0xcd, 0x2a, 0xbe, 0xd0, 0x62, 0xc3,
     0x35, 0xff, 0x4a, 0x65, 0x1c, 0x35, 0x39, 0xa0, 0x93, 0x8f, 0x3d, 0x11,
     0x1d, 0x81, 0x7f, 0x73, 0x67, 0xf3, 0x6e, 0xab, 0x1a, 0xd5, 0xe1, 0xb6,
     0x30, 0xa4, 0xaf, 0x7e, 0xc0, 0x62, 0x13, 0xcd, 0x8d, 0x3a, 0xe6, 0x08,
     0xff, 0x3b, 0xf9, 0x3c, 0x06, 0xb6, 0x30, 0xe3, 0x09, 0x0f, 0x67, 0xcb,
     0xd7, 0x6b, 0x43, 0xb4, 0x63, 0xf7, 0xee, 0x65, 0x1b, 0x92, 0x6b, 0x54,
     0xf2, 0x8d, 0x5e, 0x87, 0x90, 0x56, 0x65, 0xe2, 0x71, 0x3a, 0xb2, 0x90,
     0x57, 0x04, 0x3e, 0x6c, 0x6b, 0xf7, 0xcf, 0x9a, 0x18, 0x92, 0xab, 0xec,
     0x6a, 0xcf, 0x03, 0xe0, 0xe8, 0x62, 0xd2, 0xb1, 0x7a, 0x36, 0x22, 0x6e,
     0x4f, 0x50, 0x42, 0x1e, 0xdd, 0xde, 0x33, 0xb1},
    {0xf2, 0x23, 0xd3, 0xba, 0x49, 0x9a, 0x89, 0x86, 0x32, 0x38, 0x08, 0xac,
     0xea, 0x48, 0xf3, 0xf8, 0x99, 0x48, 0xd6, 0xdb, 0xc0, 0x20, 0xec, 0x4b,
     0xb4, 0x25, 0xb9, 0xfe, 0x79, 0x90, 0xf8, 0x61, 0x52, 0x5d, 0x8b, 0xb0,
     0xd5, 0xb8, 0x29, 0x37, 0x24, 0x13, 0x95, 0x62, 0x87, 0xfd, 0xe9, 0x97,
     0x7e, 0x4d, 0x7b, 0xeb, 0x66, 0x8c, 0x63, 0xf3, 0xc4, 0xd5, 0x9a, 0x3d,
     0x76, 0x68, 0xbc, 0x2e, 0xff, 0xd0, 0x4f, 0x09, 0x8c, 0x75, 0x08, 0x4f,
     0x74, 0x7b, 0x14, 0x31, 0x3d, 0x0a, 0x91, 0xbe, 0x3e, 0xa6, 0x71, 0x2c,
     0xcd, 0xaa, 0xf5, 0x4e, 0x25, 0xd3, 0xc3, 0xdf, 0xee, 0x94, 0xfb, 0x7c,
     0x13, 0x62, 0x8f, 0x5a, 0x74, 0x09, 0x31, 0x97, 0x38, 0x86, 0x18, 0x41,
     0xcd, 0x5c, 0x51, 0x49, 0x2e, 0xd6, 0xeb, 0x9d, 0xbc, 0x9d, 0xb1, 0xe0,
     0x53, 0x18, 0xe8, 0x93, 0xc1, 0xf3, 0x45, 0xff},
    {0x91, 0x66, 0x6e, 0xa1, 0xb0, 0xaa, 0xc3, 0xc7, 0xcc, 0x53, 0x62, 0x57,
     0x89, 0x2e, 0x13, 0x75, 0xbf, 0x19, 0xd5, 0x5f, 0x70, 0x5c, 0x29, 0x11,
     0x17, 0x80, 0xf7, 0xab, 0xc9, 0x17, 0x12, 0xb8, 0xed, 0xa4, 0x51, 0x0c,
     0xc9, 0x60, 0xe0, 0x65, 0x2a, 0x05, 0xec, 0xff, 0x4b, 0x9f, 0x88, 0x36,
     0x94, 0xbb, 0x3c, 0x60, 0xd0, 0x29, 0x4d, 0x15, 0x65, 0xd3, 0xca, 0xc6,
     0xc0, 0x13, 0x42, 0x91, 0x42, 0x76, 0x54, 0x89, 0x06, 0xc4, 0x2a, 0x17,
     0x5e, 0x6f, 0x31, 0xb7, 0xe9, 0xc6, 0x8c, 0xab, 0x8b, 0xed, 0xef, 0x3f,
     0x85, 0xe5, 0xf8, 0xe2, 0xac, 0x7e, 0x6b, 0x09, 0xa4, 0x13, 0x35, 0x6c,
     0x7e, 0x83, 0x50, 0xad, 0x2f, 0x86, 0x94, 0x2b, 0x0a, 0xed, 0x7f, 0x8d,
     0xef, 0x54, 0xc1, 0x88, 0xce, 0xa9, 0x1d, 0xb4, 0x23, 0xd6, 0xd9, 0x2c,
     0x74, 0x3c, 0x31, 0x48, 0x99, 0x18, 0xd5, 0x34},
    {0x85, 0x99, 0xdd, 0xb3, 0x36, 0xeb, 0x26, 0x22, 0x15, 0x52, 0x86, 0x3d,
     0x8f, 0xb6, 0x05, 0x82, 0x2a, 0xf6, 0x9e, 0x1b, 0x50, 0x18, 0x8d, 0xf7,
     0x37, 0x1b, 0x32, 0x00, 0xe2, 0x2c, 0xc2, 0x28, 0xfd, 0x39, 0xa7, 0x74,
     0x9f, 0x72, 0x1e, 0xd6, 0x9b, 0xcd, 0x6f, 0x69, 0x22, 0x01, 0x4e, 0xee,
     0x20, 0x75, 0x23, 0x4f, 0x2a, 0xa5, 0xe9, 0x85, 0x93, 0x1f, 0xeb, 0x91,
     0x26, 0x6e, 0xf1, 0xdc, 0xc0, 0xdc, 0x34, 0xc6, 0xe5, 0x27, 0x86, 0x74,
     0xa7, 0x63, 0xcf, 0x54, 0xd1, 0x87, 0x6c, 0x30, 0x50, 0xba, 0x64, 0x5d,
     0x95, 0xbf, 0x47, 0xb4, 0x1b, 0xdd, 0x11, 0x98, 0x4c, 0x2f, 0x83, 0x21,
     0x8d, 0x69, 0x64, 0xfb, 0x67, 0x86, 0x14, 0x2e, 0x92, 0xa2, 0x4a, 0xf0,
     0x15, 0x39, 0xfc, 0x36, 0x9c, 0x9e, 0x8f, 0x69, 0x31, 0xe9, 0xb4, 0x48,
     0x3d, 0x0a, 0x8e, 0x6d, 0x90, 0x45, 0xf3, 0x40},
    {0xac, 0xea, 0xec, 0x38, 0xf4, 0x3f, 0xd1, 0xd2, 0x95, 0xbd, 0xa9, 0xfe,
     0x26, 0x9c, 0xcd, 0xa7, 0xf0, 0x8a, 0xdf, 0x78, 0x24, 0x39, 0xde, 0x4c,
     0x4e, 0x20, 0xc6, 0x7f, 0xd6, 0x0e, 0x8d, 0x72, 0x8d, 0xbb, 0x7c, 0x05,
     0x16, 0xa8, 0xc8, 0xab, 0x5f, 0xc3, 0x0d, 0x80, 0xb7, 0xa8, 0xe6, 0x96,
     0x6f, 0xb3, 0x51, 0x68, 0xdc, 0xde, 0xf5, 0xcf, 0x6b, 0x1d, 0x17, 0x92,
     0xd0, 0x8d, 0xf1, 0x6c, 0x31, 0x26, 0x3f, 0xe6, 0xc2, 0x13, 0x9f, 0x0e,
     0x78, 0x6c, 0x54, 0x76, 0x8e, 0x12, 0x65, 0x7f, 0x67, 0xcd, 0x4c, 0xd0,
     0xa3, 0x0b, 0x02, 0x49, 0x3d, 0xe0, 0xef, 0x06, 0x7b, 0x4d, 0x57, 0xed,
     0xe0, 0xa1, 0x3c, 0x3d, 0x5c, 0xe2, 0x6f, 0x9f, 0x87, 0xcd, 0xa0, 0x65,
     0x8c, 0xff, 0x50, 0xe6, 0x54, 0xfe, 0x49, 0xb7, 0xd9, 0x9e, 0x5a, 0x1d,
     0xe7, 0x91, 0x94, 0xf6, 0x6a, 0xca, 0x1f, 0x17},
    {0x63, 0xea, 0x59, 0x75, 0x16, 0x0f, 0xda, 0x99, 0xcf, 0xfd, 0xa2, 0x91,
     0xe4, 0x76, 0xc3, 0x21, 0x1f, 0xc6, 0x28, 0x0b, 0x3c, 0xca, 0xe5, 0x46,
     0x96, 0x0b, 0x95, 0x40, 0xc9, 0x9d, 0x91, 0x9e, 0x81, 0x52, 0x7a, 0xe7,
     0xe8, 0x25, 0x08, 0x1e, 0x2b, 0x15, 0xec, 0x2e, 0x59, 0x00, 0xb4, 0xcc,
     0x58, 0x67, 0xd9, 0x34, 0x1f, 0xfb, 0x8e, 0x23, 0x2d, 0xc8, 0x0f, 0xec,
     0x1a, 0xec, 0x84, 0x59, 0x90, 0xd9, 0x57, 0xd2, 0xcb, 0x5a, 0x22, 0xa7,
     0x7a, 0x29, 0x54, 0xbf, 0xdc, 0xe2, 0xed, 0x54, 0x7c, 0xb3, 0x2b, 0x39,
     0x2f, 0x30, 0xb2, 0xb7, 0xcb, 0x59, 0x4c, 0xea, 0x75, 0xdc, 0x21, 0xd8,
     0x65, 0xee, 0x2b, 0x83, 0x2a, 0xe2, 0x4f, 0x5c, 0x2d, 0xb9, 0x37, 0x84,
     0x0f, 0x52, 0x41, 0x96, 0x69, 0xe8, 0xa4, 0x6e, 0xd4, 0xce, 0xf7, 0xf6,
     0x51, 0xc7, 0x64, 0xad, 0xec, 0xb3, 0x05, 0x70},
    {0x3d, 0xcf, 0x8d, 0xd2, 0xc7, 0x50, 0xdc, 0x9d, 0x2f, 0xb1, 0xfe, 0x80,
     0x1a, 0xe9, 0x15, 0xba, 0x04, 0xe7, 0x58, 0x62, 0x1b, 0x92, 0x98, 0xb5,
     0xb4, 0x58, 0x92, 0x20, 0x9e, 0xc4, 0xa5, 0xa5, 0xa5, 0xf3, 0xa1, 0x43,
     0x0e, 0xbe, 0x56, 0xff, 0x78, 0x91, 0x05, 0xe0, 0xb6, 0x7e, 0x4a, 0x6e,
     0x2c, 0x35, 0xfc, 0x2e, 0x19, 0x18, 0xc7, 0x14, 0xef, 0x09, 0x4e, 0x6d,
     0xdc, 0x65, 0x44, 0xed, 0x14, 0xda, 0x50, 0xca, 0x11, 0x8d, 0xe8, 0x44,
     0xa2, 0xed, 0x3f, 0x66, 0x64, 0x8d, 0xeb, 0x20, 0x2a, 0x15, 0x75, 0x9c,
     0x39, 0x07, 0x81, 0x76, 0x51, 0x88, 0x4c, 0x82, 0xe3, 0x35, 0xfe, 0x4d,
     0x88, 0xf8, 0x51, 0x51, 0xb9, 0xc9, 0x6c, 0x68, 0xa2, 0x95, 0x38, 0x2b,
     0x25, 0xc0, 0xfc, 0xca, 0xff, 0x40, 0x36, 0xf6, 0x5a, 0xe8, 0x7b, 0x89,
     0x8a, 0x2c, 0x6f, 0x86, 0x2f, 0x17, 0xc8, 0x0e},
    {0xe9, 0x0f, 0xfa, 0xa7, 0x4e, 0x04, 0x5b, 0xaa, 0x14, 0x81, 0xf9, 0x6c,
     0xcb, 0xe6, 0x91, 0x98, 0x08, 0x12, 0x1e, 0x87, 0x51, 0xd7, 0x4b, 0xf2,
     0x6e, 0x07, 0xef, 0x30, 0xd6, 0x69, 0xac, 0xa8, 0xa0, 0x7d, 0xe3, 0x0a,
     0xce, 0x9b, 0x86, 0x48, 0x1f, 0xcf, 0x8d, 0x1c, 0xba, 0x80, 0xf7, 0xd5,
     0x3a, 0xea, 0x5a, 0x97, 0xc8, 0x43, 0x24, 0x19, 0x5d, 0x96, 0x84, 0x8d,
     0x0b, 0x8d, 0x63, 0xfa, 0x6c, 0xda, 0x3e, 0xdd, 0x97, 0xfb, 0x19, 0x7a,
     0xa3, 0xde, 0x7f, 0xf0, 0x59, 0x89, 0x0d, 0x3f, 0xa1, 0xb5, 0xdf, 0x12,
     0xdc, 0x54, 0xb5, 0xf6, 0x97, 0x3a, 0x35, 0xcf, 0x67, 0x99, 0x98, 0x9a,
     0x2b, 0x4c, 0xdf, 0xa1, 0xdf, 0x89, 0x34, 0x39, 0x9d, 0xb2, 0x16, 0x23,
     0xc4, 0x3d, 0x30, 0xee, 0x6e, 0x4e, 0xfb, 0xac, 0xdf, 0x54, 0x06, 0x86,
     0xbc, 0x52, 0x6b, 0xfd, 0x0d, 0x35, 0x03, 0xa4},
    {0xd1, 0x14, 0xe5, 0x1d, 0xd2, 0x3c, 0x19, 0xe9, 0x1c, 0xb4, 0x80, 0x60,
     0x4b, 0xce, 0xcf, 0x73, 0x03, 0xca, 0xba, 0x69, 0xcb, 0x2d, 0x2a, 0xf7,
     0xf7, 0x7a, 0x2d, 0x18, 0xe8, 0x93, 0x7b, 0x7b, 0x7b, 0x85, 0x38, 0xb1,
     0x84, 0x30, 0xbe, 0x40, 0x62, 0x2c, 0xc3, 0x48, 0xf6, 0x20, 0x77, 0xec,
     0x9d, 0x97, 0xc1, 0x5c, 0x0a, 0x4a, 0x52, 0xcf, 0x8c, 0x86, 0x34, 0xed,
     0x19, 0xeb, 0x73, 0xcd, 0x0f, 0xdb, 0xbc, 0x97, 0x4c, 0x25, 0x8e, 0xb3,
     0x39, 0xcd, 0x50, 0x2a, 0xeb, 0xa5, 0x8f, 0x58, 0xdf, 0xcf, 0x67, 0xe9,
     0x12, 0x02, 0xa0, 0xa6, 0x7c, 0xe6, 0xf5, 0x21, 0x09, 0xd7, 0x40, 0xb5,
     0x26, 0x42, 0x7c, 0xfc, 0x32, 0x96, 0x2d, 0xee, 0xb9, 0xef, 0x92, 0x5f,
     0x1b, 0x90, 0x41, 0xd7, 0xc0, 0xb0, 0x56, 0x46, 0x7b, 0x4e, 0xa3, 0xe6,
     0x67, 0x9d, 0x6c, 0xa2, 0xdc, 0xce, 0x16, 0x04}
    /*
0xf5, 0xf1, 0xa4, 0x74, 0xd2, 0x8b, 0x85, 0xb6, 0x5c, 0xd6,
0x7d, 0x69, 0x75, 0x81, 0xf3, 0x82, 0x87, 0x2a, 0x49, 0x25,
0xa8, 0xa9, 0x3e, 0xd9, 0xbf, 0xec, 0x2e, 0x34, 0x85, 0x00,
0x2a, 0x07, 0x5d, 0x92, 0x8f, 0x55, 0xba, 0x82, 0x2e, 0x4e,
0xeb, 0x0c, 0x44, 0x5d, 0x1a, 0xc6, 0x79, 0xb2, 0x9f, 0x92,
0x27, 0x24, 0xe3, 0xc1, 0xa5, 0x0c, 0xe8, 0x10, 0x76, 0xc5,
0xe3, 0x7f, 0x69, 0x9f, 0x65, 0xfd, 0x9a, 0x79, 0x95, 0xef,
0x81, 0xcc, 0x16, 0xdb, 0x14, 0x07, 0x91, 0x76, 0x53, 0x9e,
0x9f, 0xb7, 0xfb, 0x94, 0x93, 0xdd, 0xbb, 0xc8, 0x47, 0x0e,
0x0e, 0x33, 0x9e, 0x40, 0x8d, 0x68, 0xf1, 0xf5, 0xcf, 0x21,
0xeb, 0x79, 0xac, 0x4d, 0x2a, 0xe2, 0x28, 0xac, 0x6e, 0x96,
0x50, 0x79, 0x9c, 0xe1, 0x26, 0xc7, 0x3e, 0x6f, 0x05, 0x01,
0x5b, 0x7e, 0x2b, 0xa5, 0xf9, 0x59, 0x10, 0xcd,
},
{
0xac, 0xb5, 0x63, 0x52, 0xab, 0x77, 0xf2, 0xb2, 0x6a, 0xf0,
0xf5, 0xe7, 0xd3, 0x9e, 0x0c, 0xb9, 0x67, 0x84, 0xec, 0x99,
0x56, 0xc0, 0x86, 0xfb, 0x4d, 0x55, 0xe7, 0x73, 0xd1, 0x4d,
0xa3, 0x7e, 0xc4, 0xeb, 0x52, 0xc2, 0xd3, 0xfd, 0xaa, 0xba,
0x75, 0x45, 0x68, 0x9b, 0xd8, 0xcc, 0x83, 0xb7, 0x41, 0x8e,
0x6b, 0x56, 0x01, 0x87, 0xc4, 0x13, 0x13, 0x59, 0x88, 0xe0,
0x54, 0x5b, 0xa2, 0x50, 0xf6, 0xb6, 0xbe, 0x85, 0xe2, 0x14,
0x8a, 0x1b, 0xcb, 0x64, 0x05, 0x4f, 0x85, 0xb5, 0x68, 0x8d,
0x25, 0x38, 0x64, 0x30, 0x46, 0xed, 0x29, 0x0b, 0x3f, 0xc9,
0x8c, 0x79, 0xbc, 0x58, 0xb9, 0x3e, 0xca, 0xf1, 0x57, 0x91,
0x64, 0xb5, 0x15, 0xd8, 0xdf, 0xc7, 0x65, 0x6e, 0x99, 0xeb,
0x04, 0x81, 0x60, 0x86, 0xc6, 0x42, 0xa7, 0x48, 0x87, 0x96,
0x14, 0xc0, 0x06, 0xd7, 0xd3, 0xd6, 0x44, 0xbc,
},
{
0x47, 0x84, 0x3b, 0xa7, 0x1d, 0x27, 0xe3, 0xf6, 0xb9, 0x5e,
0xe1, 0x6e, 0x67, 0xe0, 0x45, 0xa1, 0x22, 0x1f, 0x36, 0xdb,
0xfe, 0x7e, 0x90, 0x1a, 0xb0, 0x8d, 0x5c, 0x57, 0xa3, 0xc0,
0x5f, 0x82, 0x39, 0x2d, 0xe4, 0xbf, 0x33, 0xa1, 0x9c, 0xb4,
0x8f, 0xc5, 0x33, 0x79, 0x0b, 0x52, 0x62, 0x35, 0xa8, 0xed,
0x5a, 0x9b, 0x89, 0x10, 0x7b, 0x45, 0x0e, 0xcc, 0x26, 0x53,
0x49, 0x60, 0xae, 0x68, 0xab, 0xc1, 0xeb, 0xe2, 0xef, 0x4c,
0xe0, 0x15, 0x4e, 0x5b, 0xcf, 0x42, 0xac, 0xe6, 0x3d, 0xa8,
0xa8, 0x76, 0x43, 0x6f, 0xed, 0x99, 0xb3, 0x16, 0x72, 0xc4,
0x04, 0xd5, 0x68, 0x30, 0xa5, 0xae, 0xc4, 0x87, 0x14, 0xd8,
0x4f, 0xe2, 0xbd, 0x35, 0x5f, 0x09, 0xde, 0xfd, 0xec, 0x2e,
0x3c, 0xe2, 0x69, 0xc8, 0x9a, 0xd2, 0x50, 0x2c, 0x43, 0x80,
0x3b, 0x20, 0x9f, 0x3b, 0x02, 0xba, 0x0c, 0x55,
},
{
0x3d, 0x77, 0x69, 0x7d, 0xbf, 0x66, 0x05, 0xf5, 0xef, 0xc4,
0x87, 0xca, 0x1d, 0x68, 0xc5, 0xb2, 0xaa, 0x23, 0xcd, 0x2a,
0xbe, 0xd0, 0x62, 0xc3, 0x35, 0xff, 0x4a, 0x65, 0x1c, 0x35,
0x39, 0xa0, 0x93, 0x8f, 0x3d, 0x11, 0x1d, 0x81, 0x7f, 0x73,
0x67, 0xf3, 0x6e, 0xab, 0x1a, 0xd5, 0xe1, 0xb6, 0x30, 0xa4,
0xaf, 0x7e, 0xc0, 0x62, 0x13, 0xcd, 0x8d, 0x3a, 0xe6, 0x08,
0xff, 0x3b, 0xf9, 0x3c, 0x06, 0xb6, 0x30, 0xe3, 0x09, 0x0f,
0x67, 0xcb, 0xd7, 0x6b, 0x43, 0xb4, 0x63, 0xf7, 0xee, 0x65,
0x1b, 0x92, 0x6b, 0x54, 0xf2, 0x8d, 0x5e, 0x87, 0x90, 0x56,
0x65, 0xe2, 0x71, 0x3a, 0xb2, 0x90, 0x57, 0x04, 0x3e, 0x6c,
0x6b, 0xf7, 0xcf, 0x9a, 0x18, 0x92, 0xab, 0xec, 0x6a, 0xcf,
0x03, 0xe0, 0xe8, 0x62, 0xd2, 0xb1, 0x7a, 0x36, 0x22, 0x6e,
0x4f, 0x50, 0x42, 0x1e, 0xdd, 0xde, 0x33, 0xb1,
},
*/
};

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
#define BIT(n) (1U << (n))
#define GET_BIT(n, num) (((num) >> (n)) & 1U)

int gen_rand_num(unsigned short seed, int len, unsigned char *out)
{
	int i, cnt, bi;
	unsigned short pnsr, pns_out;

	if (len & 0x1) {
		PHY_ERR(
		    "gen_rand_num, wrong input len %d, len must be even!!\n",
		    len);
		return ERR_NO_35;
	}

	pnsr = seed;
	pns_out = 0;
	cnt = 0;

	for (bi = 0; bi < (len >> 1); bi++) {
		for (i = 0; i < 14; i++) {
			// pns_out[i]  = pnsr[i] ^ pnsr[i+1];
			pns_out &= (~BIT(i));
			pns_out |=
			    ((GET_BIT(i, pnsr) ^ GET_BIT(i + 1, pnsr)) << i);

			// pns_out[14] = pnsr[14] ^ pnsr[0] ^ pnsr[1];
			pns_out &= (~BIT(14));
			pns_out |= ((GET_BIT(14, pnsr) ^ GET_BIT(0, pnsr) ^
				     GET_BIT(1, pnsr))
				    << 14);

			// pns_out[15] = pnsr[0]  ^ pnsr[2];
			pns_out &= (~BIT(15));
			pns_out |=
			    ((GET_BIT(0, pnsr) ^ GET_BIT(2, pnsr)) << 15);
		}

		// update seed
		pnsr = pns_out >> 1;

		out[cnt++] = pns_out & 0xff;
		out[cnt++] = (pns_out >> 8) & 0xff;
	}

	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int wait_reg_status_half_us(volatile u32 *reg, u32 mark, u32 value)
{
	int i, ret;
	volatile u32 data;

	for (i = 0; i < 0xfff; i++) {
		ret = ERR_TIMEOUT;
		data = *reg;
		if ((data & mark) == value) {
			ret = 0;
			break;
		}
		//cond_resched();
	}
	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
s32 wait_reg_status(volatile u32 *reg, u32 mark, u32 value, u32 us)
{
	// volatile u32 data;
	int ret = 0;

	for (; us > 0; us--) {
		ret = wait_reg_status_half_us(reg, mark, value);
		if (ret == 0) {
			break;
		}
		cond_resched();
	}

	if (ret != 0)
		PHY_ERR("nand wait_reg_status timeout, reg:0x%x, value:0x%x\n",
			reg, *reg);

	return ret;
}

/*****************************************************************************
*Name         : ndfc_print_reg
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void ndfc_print_dma(struct _nand_controller_info *nctri)
{
	unsigned char i;
	for (i = 0; i < 8; i++)
		PHY_DBG("*****reg_data_dma_size_2x+%d(0x%08x): 0x%08x*****\n",
			i, (nctri->nreg.reg_data_dma_size_2x + i),
			*(nctri->nreg.reg_data_dma_size_2x + i));
}

/*****************************************************************************
*Name         : ndfc_print_reg
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void ndfc_print_random(struct _nand_controller_info *nctri)
{
	unsigned char i;
	for (i = 0; i < 8; i++)
		PHY_DBG("*****reg_random_seed_x+%d(0x%08x): 0x%08x*****\n", i,
			(nctri->nreg.reg_random_seed_x + i),
			*(nctri->nreg.reg_random_seed_x + i));
}

/*****************************************************************************
*Name         : ndfc_print_reg
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void ndfc_print_reg(struct _nand_controller_info *nctri)
{
	PHY_DBG("print ndfc reg:\n");
	PHY_DBG("0x0000 reg_ctl:			0x%08x\n",
		*nctri->nreg.reg_ctl);
	PHY_DBG("0x0004 reg_sta:			0x%08x\n",
		*nctri->nreg.reg_sta);
	PHY_DBG("0x0008 reg_int:			0x%08x\n",
		*nctri->nreg.reg_int);
	PHY_DBG("0x000C reg_timing_ctl:		0x%08x\n",
		*nctri->nreg.reg_timing_ctl);
	PHY_DBG("0x0010 reg_timing_cfg:		0x%08x\n",
		*nctri->nreg.reg_timing_cfg);
	PHY_DBG("0x0014 reg_addr_low:		0x%08x\n",
		*nctri->nreg.reg_addr_low);
	PHY_DBG("0x0018 reg_addr_high:		0x%08x\n",
		*nctri->nreg.reg_addr_high);
	PHY_DBG("0x001C reg_data_block_mask:	0x%08x\n",
		*nctri->nreg.reg_data_block_mask);
	PHY_DBG("0x0020 reg_ndfc_cnt:		0x%08x\n",
		*nctri->nreg.reg_ndfc_cnt);
	PHY_DBG("0x0024 reg_cmd:			0x%08x\n",
		*nctri->nreg.reg_cmd);
	PHY_DBG("0x0028 reg_read_cmd_set:		0x%08x\n",
		*nctri->nreg.reg_read_cmd_set);
	PHY_DBG("0x002C reg_write_cmd_set:	0x%08x\n",
		*nctri->nreg.reg_write_cmd_set);
	PHY_DBG("0x0034 reg_ecc_ctl:		0x%08x\n",
		*nctri->nreg.reg_ecc_ctl);
	PHY_DBG("0x0038 reg_ecc_sta:		0x%08x\n",
		*nctri->nreg.reg_ecc_sta);
	PHY_DBG("0x003C reg_data_pattern_sta:	0x%08x\n",
		*nctri->nreg.reg_data_pattern_sta);
	PHY_DBG("0x0040 reg_efr:			0x%08x\n",
		*nctri->nreg.reg_efr);
	PHY_DBG("0x0044 reg_rdata_sta_ctl:	0x%08x\n",
		*nctri->nreg.reg_rdata_sta_ctl);
	PHY_DBG("0x0048 reg_rdata_sta_0:		0x%08x\n",
		*nctri->nreg.reg_rdata_sta_0);
	PHY_DBG("0x004C reg_rdata_sta_1:		0x%08x\n",
		*nctri->nreg.reg_rdata_sta_1);
	PHY_DBG("0x0050 reg_err_cnt0:		0x%08x\n",
		*nctri->nreg.reg_err_cnt0);
	PHY_DBG("0x0054 reg_err_cnt1:		0x%08x\n",
		*nctri->nreg.reg_err_cnt1);
	PHY_DBG("0x0058 reg_err_cnt2:		0x%08x\n",
		*nctri->nreg.reg_err_cnt2);
	PHY_DBG("0x005C reg_err_cnt3:		0x%08x\n",
		*nctri->nreg.reg_err_cnt3);
	PHY_DBG("0x0060 reg_err_cnt4:		0x%08x\n",
		*nctri->nreg.reg_err_cnt4);
	PHY_DBG("0x0064 reg_err_cnt5:		0x%08x\n",
		*nctri->nreg.reg_err_cnt5);
	PHY_DBG("0x0068 reg_err_cnt6:		0x%08x\n",
		*nctri->nreg.reg_err_cnt6);
	PHY_DBG("0x006C reg_err_cnt7:		0x%08x\n",
		*nctri->nreg.reg_err_cnt7);
	PHY_DBG("0x0070 reg_user_data_len_base:	0x%08x\n",
		*nctri->nreg.reg_user_data_len_base);
	PHY_DBG("0x0080 reg_user_data_base:	0x%08x\n",
		*nctri->nreg.reg_user_data_base);
	PHY_DBG("0x0100 reg_flash_sta:		0x%08x\n",
		*nctri->nreg.reg_flash_sta);
	PHY_DBG("0x0104 reg_cmd_repeat_cnt:	0x%08x\n",
		*nctri->nreg.reg_cmd_repeat_cnt);
	PHY_DBG("0x0108 reg_cmd_repeat_interval:	0x%08x\n",
		*nctri->nreg.reg_cmd_repeat_interval);
	PHY_DBG("0x0110 reg_efnand_sta:		0x%08x\n",
		*nctri->nreg.reg_efnand_sta);
	PHY_DBG("0x0114 reg_spare_area:		0x%08x\n",
		*nctri->nreg.reg_spare_area);
	PHY_DBG("0x0118 reg_pat_id:		0x%08x\n",
		*nctri->nreg.reg_pat_id);
	PHY_DBG("0x011C reg_ddr2_spec_ctl:	0x%08x\n",
		*nctri->nreg.reg_ddr2_spec_ctl);
	PHY_DBG("0x0120 reg_ndma_mode_ctl:	0x%08x\n",
		*nctri->nreg.reg_ndma_mode_ctl);
	PHY_DBG("0x012C reg_valid_data_dma_cnt:	0x%08x\n",
		*nctri->nreg.reg_valid_data_dma_cnt);
	PHY_DBG("0x0130 reg_data_dma_base_0:	0x%08x\n",
		*nctri->nreg.reg_data_dma_base);
	PHY_DBG("0x0170 reg_data_dma_size_2x:	0x%08x\n",
		*nctri->nreg.reg_data_dma_size_2x);
	PHY_DBG("0x0190 reg_random_seed_x:	0x%08x\n",
		*nctri->nreg.reg_random_seed_x);
	PHY_DBG("0x0214 reg_dma_cnt:		0x%08x\n",
		*nctri->nreg.reg_dma_cnt);
	PHY_DBG("0x0218 reg_emce_ctl: 		0x%08x\n",
		*nctri->nreg.reg_emce_ctl);
	PHY_DBG("0x021C reg_emce_iv_fac_cmp_val:	0x%08x\n",
		*nctri->nreg.reg_emce_iv_fac_cmp_val);
	PHY_DBG("0x0220 reg_emce_iv_cal_fac_base:	0x%08x\n",
		*nctri->nreg.reg_emce_iv_cal_fac_base);
	PHY_DBG("0x02A0 reg_io_data:		0x%08x\n",
		*nctri->nreg.reg_io_data);
	PHY_DBG("0x02F0 reg_ndfc_ver:		0x%08x\n",
		*nctri->nreg.reg_ndfc_ver);
	PHY_DBG("0x02FC reg_ldpc_ctl: 		0x%08x\n",
		*nctri->nreg.reg_ldpc_ctl);
	PHY_DBG("0x0300 reg_enc_ldpc_mode_set:	0x%08x\n",
		*nctri->nreg.reg_enc_ldpc_mode_set);
	PHY_DBG("0x0304 reg_cor_ldpc_mode_set:	0x%08x\n",
		*nctri->nreg.reg_cor_ldpc_mode_set);
	PHY_DBG("0x0308 reg_c0_llr_tbl_addr:	0x%08x\n",
		*nctri->nreg.reg_c0_llr_tbl_addr);
	PHY_DBG("0x0328 reg_c1_llr_tbl_addr:	0x%08x\n",
		*nctri->nreg.reg_c1_llr_tbl_addr);
	PHY_DBG("0x0400 reg_ram0_base:		0x%08x\n",
		*nctri->nreg.reg_ram0_base);
	PHY_DBG("0x0800 reg_ram1_base:		0x%08x\n",
		*nctri->nreg.reg_ram1_base);
	PHY_DBG("0x0C00 reg_glb_cfg:		0x%08x\n",
		*nctri->nreg.reg_glb_cfg);
	PHY_DBG("0x0C04 reg_cmd_descr_base_addr:	0x%08x\n",
		*nctri->nreg.reg_cmd_descr_base_addr);
	PHY_DBG("0x0C08 reg_cmd_descr_sta:	0x%08x\n",
		*nctri->nreg.reg_cmd_descr_sta);
	PHY_DBG("0x0C0C reg_cmd_descr_intr:	0x%08x\n",
		*nctri->nreg.reg_cmd_descr_intr);
	PHY_DBG("0x0C10 reg_csic_bist_ctl_reg:	0x%08x\n",
		*nctri->nreg.reg_csic_bist_ctl_reg);
	PHY_DBG("0x0C14 reg_bist_start_addr:	0x%08x\n",
		*nctri->nreg.reg_bist_start_addr);
	PHY_DBG("0x0C18 reg_bist_end_addr:	0x%08x\n",
		*nctri->nreg.reg_bist_end_addr);
	PHY_DBG("0x0C1C reg_bist_data_mask:	0x%08x\n",
		*nctri->nreg.reg_bist_data_mask);
}

/*****************************************************************************
*Name         : ndfc_print_save_reg
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void ndfc_print_save_reg(struct _nand_controller_info *nctri)
{
	PHY_DBG("print ndfc save reg:\n");
	PHY_DBG("0x0000 reg_ctl:			0x%08x\n",
		nctri->nreg.reg_ctl);
	PHY_DBG("0x0004 reg_sta:			0x%08x\n",
		nctri->nreg.reg_sta);
	PHY_DBG("0x0008 reg_int:			0x%08x\n",
		nctri->nreg.reg_int);
	PHY_DBG("0x000C reg_timing_ctl:		0x%08x\n",
		nctri->nreg.reg_timing_ctl);
	PHY_DBG("0x0010 reg_timing_cfg:		0x%08x\n",
		nctri->nreg.reg_timing_cfg);
	PHY_DBG("0x0014 reg_addr_low:		0x%08x\n",
		nctri->nreg.reg_addr_low);
	PHY_DBG("0x0018 reg_addr_high:		0x%08x\n",
		nctri->nreg.reg_addr_high);
	PHY_DBG("0x001C reg_data_block_mask:	0x%08x\n",
		nctri->nreg.reg_data_block_mask);
	PHY_DBG("0x0020 reg_ndfc_cnt:	0x%08x\n", nctri->nreg.reg_ndfc_cnt);
	PHY_DBG("0x0024 reg_cmd:			0x%08x\n",
		nctri->nreg.reg_cmd);
	PHY_DBG("0x0028 reg_read_cmd_set:	0x%08x\n",
		nctri->nreg.reg_read_cmd_set);
	PHY_DBG("0x002C reg_write_cmd_set:	0x%08x\n",
		nctri->nreg.reg_write_cmd_set);
	PHY_DBG("0x0034 reg_ecc_ctl:		0x%08x\n",
		nctri->nreg.reg_ecc_ctl);
	PHY_DBG("0x0038 reg_ecc_sta:		0x%08x\n",
		nctri->nreg.reg_ecc_sta);
	PHY_DBG("0x003C reg_data_pattern_sta:	0x%08x\n",
		nctri->nreg.reg_data_pattern_sta);
	PHY_DBG("0x0040 reg_efr:			0x%08x\n",
		nctri->nreg.reg_efr);
	PHY_DBG("0x0044 reg_rdata_sta_ctl:	0x%08x\n",
		nctri->nreg.reg_rdata_sta_ctl);
	PHY_DBG("0x0048 reg_rdata_sta_0:	0x%08x\n",
		nctri->nreg.reg_rdata_sta_0);
	PHY_DBG("0x004C reg_rdata_sta_1:	0x%08x\n",
		nctri->nreg.reg_rdata_sta_1);
	PHY_DBG("0x0050 reg_err_cnt0:		0x%08x\n",
		nctri->nreg.reg_err_cnt0);
	PHY_DBG("0x0054 reg_err_cnt1:		0x%08x\n",
		nctri->nreg.reg_err_cnt1);
	PHY_DBG("0x0058 reg_err_cnt2:		0x%08x\n",
		nctri->nreg.reg_err_cnt2);
	PHY_DBG("0x005C reg_err_cnt3:		0x%08x\n",
		nctri->nreg.reg_err_cnt3);
	PHY_DBG("0x0060 reg_err_cnt4:		0x%08x\n",
		nctri->nreg.reg_err_cnt4);
	PHY_DBG("0x0064 reg_err_cnt5:		0x%08x\n",
		nctri->nreg.reg_err_cnt5);
	PHY_DBG("0x0068 reg_err_cnt6:		0x%08x\n",
		nctri->nreg.reg_err_cnt6);
	PHY_DBG("0x006C reg_err_cnt7:		0x%08x\n",
		nctri->nreg.reg_err_cnt7);
	PHY_DBG("0x0070 reg_user_data_len_base:	0x%08x\n",
		nctri->nreg.reg_user_data_len_base);
	PHY_DBG("0x0080 reg_user_data_base:	0x%08x\n",
		nctri->nreg.reg_user_data_base);
	PHY_DBG("0x0100 reg_flash_sta:		0x%08x\n",
		nctri->nreg.reg_flash_sta);
	PHY_DBG("0x0104 reg_cmd_repeat_cnt:	0x%08x\n",
		nctri->nreg.reg_cmd_repeat_cnt);
	PHY_DBG("0x0108 reg_cmd_repeat_interval:	0x%08x\n",
		nctri->nreg.reg_cmd_repeat_interval);
	PHY_DBG("0x0110 reg_efnand_sta:		0x%08x\n",
		nctri->nreg.reg_efnand_sta);
	PHY_DBG("0x0114 reg_spare_area:		0x%08x\n",
		nctri->nreg.reg_spare_area);
	PHY_DBG("0x0118 reg_pat_id:			0x%08x\n",
		nctri->nreg.reg_pat_id);
	PHY_DBG("0x011C reg_ddr2_spec_ctl:	0x%08x\n",
		nctri->nreg.reg_ddr2_spec_ctl);
	PHY_DBG("0x0120 reg_ndma_mode_ctl:	0x%08x\n",
		nctri->nreg.reg_ndma_mode_ctl);
	PHY_DBG("0x012C reg_valid_data_dma_cnt:	0x%08x\n",
		nctri->nreg.reg_valid_data_dma_cnt);
	PHY_DBG("0x0130 reg_data_dma_base_0:	0x%08x\n",
		nctri->nreg.reg_data_dma_base);
	PHY_DBG("0x0170 reg_data_dma_size_2x:	0x%08x\n",
		nctri->nreg.reg_data_dma_size_2x);
	PHY_DBG("0x0190 reg_random_seed_x:		0x%08x\n",
		nctri->nreg.reg_random_seed_x);
	PHY_DBG("0x0214 reg_dma_cnt:           0x%08x\n",
		nctri->nreg.reg_dma_cnt);
	PHY_DBG("0x0218 reg_emce_ctl: 		0x%08x\n",
		nctri->nreg.reg_emce_ctl);
	PHY_DBG("0x021C reg_emce_iv_fac_cmp_val:	0x%08x\n",
		nctri->nreg.reg_emce_iv_fac_cmp_val);
	PHY_DBG("0x0220 reg_emce_iv_cal_fac_base:	0x%08x\n",
		nctri->nreg.reg_emce_iv_cal_fac_base);
	PHY_DBG("0x02A0 reg_io_data:		0x%08x\n",
		nctri->nreg.reg_io_data);
	PHY_DBG("0x02F0 reg_ndfc_ver:           0x%08x\n",
		nctri->nreg.reg_ndfc_ver);
	PHY_DBG("0x02FC reg_ldpc_ctl: 		0x%08x\n",
		nctri->nreg.reg_ldpc_ctl);
	PHY_DBG("0x0300 reg_enc_ldpc_mode_set:	0x%08x\n",
		nctri->nreg.reg_enc_ldpc_mode_set);
	PHY_DBG("0x0304 reg_cor_ldpc_mode_set:	0x%08x\n",
		nctri->nreg.reg_cor_ldpc_mode_set);
	PHY_DBG("0x0308 reg_c0_llr_tbl_addr:	0x%08x\n",
		nctri->nreg.reg_c0_llr_tbl_addr);
	PHY_DBG("0x0328 reg_c1_llr_tbl_addr:	0x%08x\n",
		nctri->nreg.reg_c1_llr_tbl_addr);
	PHY_DBG("0x0400 reg_ram0_base:			0x%08x\n",
		nctri->nreg.reg_ram0_base);
	PHY_DBG("0x0800 reg_ram1_base:			0x%08x\n",
		nctri->nreg.reg_ram1_base);
	PHY_DBG("0x0C00 reg_glb_cfg:			0x%08x\n",
		nctri->nreg.reg_glb_cfg);
	PHY_DBG("0x0C04 reg_cmd_descr_base_addr:	0x%08x\n",
		nctri->nreg.reg_cmd_descr_base_addr);
	PHY_DBG("0x0C08 reg_cmd_descr_sta:		0x%08x\n",
		nctri->nreg.reg_cmd_descr_sta);
	PHY_DBG("0x0C0C reg_cmd_descr_intr:		0x%08x\n",
		nctri->nreg.reg_cmd_descr_intr);
	PHY_DBG("0x0C10 reg_csic_bist_ctl_reg:	0x%08x\n",
		nctri->nreg.reg_csic_bist_ctl_reg);
	PHY_DBG("0x0C14 reg_bist_start_addr:	0x%08x\n",
		nctri->nreg.reg_bist_start_addr);
	PHY_DBG("0x0C18 reg_bist_end_addr:		0x%08x\n",
		nctri->nreg.reg_bist_end_addr);
	PHY_DBG("0x0C1C reg_bist_data_mask:		0x%08x\n",
		nctri->nreg.reg_bist_data_mask);
}

/*****************************************************************************
*Name         : dma_print_reg
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
#if 0
void dma_print_reg(int n)
{

	u32 dma_reg_base = 0xf1c02000;

	if (n < 0xffff) {
		g_nctri->dma_channel = n;
	}

	n = g_nctri->dma_channel;

	volatile u32 *dma_irq_en0 =    (volatile u32 *)(dma_reg_base + 0x0000);
	volatile u32 *dma_irq_en1 =    (volatile u32 *)(dma_reg_base + 0x0004);
	volatile u32 *dma_irq_pend0 =  (volatile u32 *)(dma_reg_base + 0x0010);
	volatile u32 *dma_irq_pend1 =  (volatile u32 *)(dma_reg_base + 0x0014);
	volatile u32 *dma_sta =        (volatile u32 *)(dma_reg_base + 0x0030);
	volatile u32 *dma_en =         (volatile u32 *)(dma_reg_base + 0x100 + n*0x40 + 0x00);
	volatile u32 *dma_pau =        (volatile u32 *)(dma_reg_base + 0x100 + n*0x40 + 0x04);
	volatile u32 *dma_start_addr = (volatile u32 *)(dma_reg_base + 0x100 + n*0x40 + 0x08);
	volatile u32 *dma_cfg =        (volatile u32 *)(dma_reg_base + 0x100 + n*0x40 + 0x0c);
	volatile u32 *dma_cur_src =    (volatile u32 *)(dma_reg_base + 0x100 + n*0x40 + 0x10);
	volatile u32 *dma_cur_dest =   (volatile u32 *)(dma_reg_base + 0x100 + n*0x40 + 0x14);
	volatile u32 *dma_bcnt_left =  (volatile u32 *)(dma_reg_base + 0x100 + n*0x40 + 0x18);
	volatile u32 *dma_para =       (volatile u32 *)(dma_reg_base + 0x100 + n*0x40 + 0x1c);

	PHY_DBG("dma_irq_en0: 0x%08x\n", *dma_irq_en0);
	PHY_DBG("dma_irq_en1: 0x%08x\n", *dma_irq_en1);
	PHY_DBG("dma_irq_pend0: 0x%08x\n", *dma_irq_pend0);
	PHY_DBG("dma_irq_pend1: 0x%08x\n", *dma_irq_pend1);
	PHY_DBG("dma_sta: 0x%08x\n", *dma_sta);
	PHY_DBG("dma_en: 0x%08x\n", *dma_en);
	PHY_DBG("dma_pau: 0x%08x\n", *dma_pau);
	PHY_DBG("dma_start_addr: 0x%08x\n", *dma_start_addr);
	PHY_DBG("dma_cfg: 0x%08x\n", *dma_cfg);
	PHY_DBG("dma_cur_src: 0x%08x\n", *dma_cur_src);
	PHY_DBG("dma_cur_dest: 0x%08x\n", *dma_cur_dest);
	PHY_DBG("dma_bcnt_left: 0x%08x\n", *dma_bcnt_left);
	PHY_DBG("dma_para: 0x%08x\n", *dma_para);
}

/*****************************************************************************
*Name         : check_print_reg
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void check_print_reg(void)
{
	u32 n, m;
	u32 dma_reg_base = 0xf1c02000;
	n = g_nctri->dma_channel;

	volatile u32 *dma_en = (volatile u32 *)(dma_reg_base + 0x100 + n*0x40 + 0x00);

	m = *dma_en;

	if (m == 0) {
		PHY_DBG("----------------------1\n");
		dma_print_reg(n);
		PHY_DBG("----------------------2\n");
	}
}
#endif
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
// void dma_gating_reg_close(void)
//{
//    u32 i;
//	u32 dma_reg_base = 0xf1c20000;
//	volatile u32 * dma_gating_reg0  =   (volatile u32 *)(dma_reg_base +
//0x0060);
//	volatile u32 * dma_clk_rest     =   (volatile u32 *)(dma_reg_base +
//0x02c0);
//
//    *dma_gating_reg0  &= 0xffffffbf;   //bit 6 clean
//    *dma_clk_rest  &= 0xffffffbf;   //bit 6 clean
//    *dma_clk_rest  |= 0x40;   //bit 6 clean
//}
//
// void dma_gating_reg_open(void)
//{
//    u32 i;
//	u32 dma_reg_base = 0xf1c20000;
//	volatile u32 * dma_gating_reg0  =   (volatile u32 *)(dma_reg_base +
//0x0060);
//	volatile u32 * dma_clk_rest     =   (volatile u32 *)(dma_reg_base +
//0x02c0);
//
//    *dma_clk_rest  &= 0xffffffbf;   //bit 6 clean
//    *dma_clk_rest  |= 0x40;   //bit 6 set
//
//    *dma_gating_reg0  |= 0x40;   //bit 6 set
//}

/*****************************************************************************
*Name         : _cal_random_seed
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
u32 _cal_random_seed(u32 page)
{
	return random_seed[page % 128];
}

/*****************************************************************************
*Name         : _set_addr
*Description  : set address register
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void _set_addr(struct _nand_controller_info *nctri, u8 acnt, u8 *abuf)
{
	s32 i;
	u32 addr_low = 0, addr_high = 0;

	for (i = 0; i < acnt; i++) {
		if (i < 4)
			addr_low |= abuf[i] << (i * 8);
		else
			addr_high |= abuf[i] << ((i - 4) * 8);
	}

	*nctri->nreg.reg_addr_low = addr_low;
	*nctri->nreg.reg_addr_high = addr_high;
}

/*****************************************************************************
*Name         : _soft_reset_nfdc
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
int ndfc_soft_reset(struct _nand_controller_info *nctri)
{
	int ret;

	// PHY_DBG("Reset NDFC start %d  %x\n",
	// nctri->channel_id,*nctri->nreg.reg_glb_cfg);

	*nctri->nreg.reg_glb_cfg |= NDFC_RESET;
	ret = wait_reg_status(nctri->nreg.reg_glb_cfg, NDFC_RESET, 0, 3);

	// PHY_DBG("Reset NDFC end %d  %x\n",
	// nctri->channel_id,*nctri->nreg.reg_glb_cfg);

	return ret;
}

/*****************************************************************************
*Name         : _disable_ecc
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void ndfc_disable_ecc(struct _nand_controller_info *nctri)
{
	*nctri->nreg.reg_ecc_ctl &= (~NDFC_ECC_EN);
}

/*****************************************************************************
*Name         : _enable_ecc
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void ndfc_enable_ecc(struct _nand_controller_info *nctri, u32 pipline,
		     u32 randomize)
{
	u32 cfg = *nctri->nreg.reg_ecc_ctl;

	if (pipline == 1)
		cfg |= NDFC_ECC_PIPELINE;
	else
		cfg &= ~NDFC_ECC_PIPELINE;

	// after erased, all data is 0xff, but ecc is not 0xff,if random open,
	// disable exception
	if (randomize)
		cfg &= ~NDFC_ECC_EXCEPTION;
	else
		cfg |= NDFC_ECC_EXCEPTION;

	cfg |= NDFC_ECC_EN;

	*nctri->nreg.reg_ecc_ctl = cfg;
}

/*****************************************************************************
*Name         : _enable_ldpc_ecc
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void ndfc_enable_ldpc_ecc(struct _nand_controller_info *nctri, u32 pipline)
{
	u32 cfg;

	cfg = *nctri->nreg.reg_ecc_ctl;
	cfg |= NDFC_ECC_EN;
	*nctri->nreg.reg_ecc_ctl = cfg;
}

/*****************************************************************************
*Name         : _repeat_mode_enable
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void ndfc_repeat_mode_enable(struct _nand_controller_info *nctri)
{
	u32 reg_val;

	reg_val = *nctri->nreg.reg_ctl;
	if (((reg_val >> 18) & 0x3) > 1) {// ddr type
		reg_val |= 0x1 << 20;
		*nctri->nreg.reg_ctl = reg_val;
	}
}

/*****************************************************************************
*Name         : _repeat_mode_disable
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void ndfc_repeat_mode_disable(struct _nand_controller_info *nctri)
{
	u32 reg_val;

	reg_val = *nctri->nreg.reg_ctl;
	if (((reg_val >> 18) & 0x3) > 1) { // ddr type
		reg_val &= ~(0x1 << 20);
		*nctri->nreg.reg_ctl = reg_val;
	}
}

/*****************************************************************************
*Name         : _wait_cmdfifo_free
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
s32 ndfc_wait_cmdfifo_free(struct _nand_controller_info *nctri)
{
	s32 ret;
	ret =
	    wait_reg_status(nctri->nreg.reg_sta, NDFC_CMD_FIFO_STATUS, 0, 0x0f);
	if (ret != 0) {
		PHY_ERR("ndfc wait cmdfifo free error!\n");
		show_nctri(nctri);
	}

	return ret;
}

/*****************************************************************************
*Name         : _wait_cmdfifo_free
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
s32 ndfc_wait_fsm_ready(struct _nand_controller_info *nctri)
{
	s32 ret;
	ret = wait_reg_status(nctri->nreg.reg_sta, NDFC_STA, 0, 0xfffff);
	if (ret != 0) {
		PHY_ERR("ndfc wait fsm ready error!\n");
	}

	return ret;
}

/*****************************************************************************
*Name         : _wait_cmdfifo_free
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
s32 ndfc_wait_cmd_finish(struct _nand_controller_info *nctri)
{
	s32 ret;
	ret = wait_reg_status(nctri->nreg.reg_sta, NDFC_CMD_INT_FLAG,
			      NDFC_CMD_INT_FLAG, 0xffffff); // songwj
	if (ret != 0) {
		PHY_ERR("ndfc wait cmd finish error!\n");
		show_nctri(nctri);
		*nctri->nreg.reg_sta = NDFC_CMD_INT_FLAG;
		return ret;
	}

	// bit2, bit 1, bit 0 are cleared after writing '1'. other bits are read
	// only.
	// here, we only clear bit 1.

	return 0;
}

/*****************************************************************************
*Name         : _select_chip
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void ndfc_select_chip(struct _nand_controller_info *nctri, u32 chip)
{
	u32 cfg;

	cfg = *nctri->nreg.reg_ctl;
	cfg &= ~NDFC_CE_SEL;
	cfg |= (chip & 0xf) << 24; // 0x7 -> 0xf
	*nctri->nreg.reg_ctl = cfg;

	// ddr nand
	if ((cfg >> 18) & 0x3) {
		*nctri->nreg.reg_timing_ctl = nctri->ddr_timing_ctl[0];
	}
}

/*****************************************************************************
*Name         : _select_chip
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void ndfc_select_rb(struct _nand_controller_info *nctri, u32 rb)
{
	u32 cfg;

	cfg = *nctri->nreg.reg_ctl;
	cfg &= ~NDFC_RB_SEL;
	cfg |= (rb & 0x3) << 3; // 0x1 -> 0x3
	*nctri->nreg.reg_ctl = cfg;
}

/*****************************************************************************
*Name         : _deselect_chip
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void ndfc_deselect_chip(struct _nand_controller_info *nctri, u32 chip)
{
}

/*****************************************************************************
*Name         : _deselect_rb
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void ndfc_deselect_rb(struct _nand_controller_info *nctri, u32 rb)
{
	return;
}

/*****************************************************************************
*Name         : _get_selected_rb_no
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
u32 ndfc_get_selected_rb_no(struct _nand_controller_info *nctri)
{
	return ((*nctri->nreg.reg_ctl & NDFC_RB_SEL) >> 3);
}

/*****************************************************************************
*Name         : _get_rb_sta
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
s32 ndfc_get_rb_sta(struct _nand_controller_info *nctri, u32 rb)
{
	u32 rb_check = 0;

	if (rb == 0)
		rb_check = NDFC_RB_STATE0;
	if (rb == 1)
		rb_check = NDFC_RB_STATE1;

	if ((*nctri->nreg.reg_sta & rb_check) != 0) {
		return 1;
	} else {
		return 0;
	}
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
u32 ndfc_get_page_size(struct _nand_controller_info *nctri)
{
	return (*(nctri->nreg.reg_ctl) >> 8) & 0xf;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int ndfc_set_page_size(struct _nand_controller_info *nctri, u32 page_size)
{
	*(nctri->nreg.reg_ctl) =
	    ((*(nctri->nreg.reg_ctl)) & (~NDFC_PAGE_SIZE)) |
	    ((page_size & 0xf) << 8);
	return 0;
}

/*****************************************************************************
*Name         : _set_ecc_mode
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void ndfc_set_ecc_mode(struct _nand_controller_info *nctri, u8 ecc_mode)
{
	u32 cfg;
	cfg = *nctri->nreg.reg_ecc_ctl;
	cfg &= ~NDFC_ECC_MODE;
	cfg |= NDFC_ECC_MODE & (ecc_mode << 8); // cfg |= NDFC_ECC_MODE &
						// (ecc_mode<<12); modifyfor
						// AW1728 new nand controller
	*nctri->nreg.reg_ecc_ctl = cfg;
}

/*****************************************************************************
*Name         : _set_boot0_ldpc_mode
*Description  : set ldpc mode for burn or read boot0
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void ndfc_boot0_ldpc_mode(struct _nand_controller_info *nctri)
{
	u32 cfg;

	cfg = *nctri->nreg.reg_enc_ldpc_mode_set;
	cfg &= ~LDPC_MODE;
	cfg |= BOOT0_LDPC_MODE; // ldpc mode = 0
	cfg &= ~FW_EXTEND_ENCODE;
	cfg |= BOOT0_FW_EXTEND_ENCODE; // firmware 0:8B 1:16B
	*nctri->nreg.reg_enc_ldpc_mode_set = cfg;

	cfg = *nctri->nreg.reg_cor_ldpc_mode_set;
	cfg &= ~C0_LDPC_MODE;
	cfg |= BOOT0_C0_LDPC_MODE; // c0 ldpc mode = 0
	cfg &= ~C1_LDPC_MODE;
	cfg |= BOOT0_C1_LDPC_MODE; // c0 ldpc mode = 0
	cfg &= ~FW_EXTEND_DECODE;
	cfg |= BOOT0_FW_EXTEND_DECODE; // firmware 0:8B 1:16B
	*nctri->nreg.reg_cor_ldpc_mode_set = cfg;
}

/*****************************************************************************
*Name         : _enable_encode
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void ndfc_enable_encode(struct _nand_controller_info *nctri)
{
	u32 cfg;

	cfg = *nctri->nreg.reg_enc_ldpc_mode_set;
	cfg |= ENCODE; // decode = 0, encode =1
	*nctri->nreg.reg_enc_ldpc_mode_set = cfg;

	cfg = *nctri->nreg.reg_glb_cfg;
	cfg &= ~LDPC_OUTPUT_DISORDER_CTL; // open disorder
	*nctri->nreg.reg_glb_cfg = cfg;
}

/*****************************************************************************
*Name         : _enable_decode
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void ndfc_enable_decode(struct _nand_controller_info *nctri)
{
	u32 cfg;

	cfg = *nctri->nreg.reg_enc_ldpc_mode_set;
	cfg &= ~ENCODE; // decode = 0, encode =1
	*nctri->nreg.reg_enc_ldpc_mode_set = cfg;

	cfg = *nctri->nreg.reg_cor_ldpc_mode_set;
	cfg &= ~C0_DECODE_MODE; // C0_Decode_Mode, 0~4, 0: HB_Mode
	cfg &= ~C0_SCALE_MODE;  // C0_SCALE_Mode, 0: HB, 1: SB
	cfg &= ~C1_DECODE_MODE; // C1_Decode_Mode, 0~4, 0-HB_Mode
	cfg &= ~C1_SCALE_MODE;  // C1_SCALE_Mode, 0: HB, 1: SB
	*nctri->nreg.reg_cor_ldpc_mode_set = cfg;

	cfg = *nctri->nreg.reg_ldpc_ctl;
	cfg &= ~CH1_HB_LLR_VAL;
	cfg |= (0xa & 0xf) << 28; // default value
	*nctri->nreg.reg_ldpc_ctl = cfg;
}

/*****************************************************************************
*Name         : _get_ecc_mode
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
s32 ndfc_get_ecc_mode(struct _nand_controller_info *nctri)
{
	return ((*nctri->nreg.reg_ecc_ctl >> 8) & 0xff);
}

/*****************************************************************************
*Name         : _enable_randomize
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void ndfc_set_rand_seed(struct _nand_controller_info *nctri, u32 page_no)
{
	u32 cfg, random_seed;

	random_seed = _cal_random_seed(page_no);

	cfg = *nctri->nreg.reg_ecc_ctl;
	cfg &= 0x0000ffff;
	cfg |= (random_seed << 16);
	*nctri->nreg.reg_ecc_ctl = cfg;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void ndfc_set_new_rand_seed(struct _nand_controller_info *nctri, u32 page_no)
{
	u32 i, j, index, cfg;
	struct _nand_chip_info *nci = nctri->nci;

	index = page_no % 67;

	for (i = 0, j = 0; i < 16; i++, j++) {
		if ((index + i) > 66) {
			index = 0;
			j = 0;
		}

		*(nctri->nreg.reg_random_seed_x + i) =
		    random_seed_new[index + j];
	}
}

/*****************************************************************************
*Name		  : _channel_select
*Description  :
*Parameter	  :	0: BCH & 1KB Frame_Size & Random_OLD; 1: LDPC & 2KB
*Frame_Size & Random_NEW
*Return 	  : NULL
*Note		  :
*****************************************************************************/
void ndfc_channel_select(struct _nand_controller_info *nctri, u32 channel_sel)
{
	u32 cfg;

	ndfc_soft_reset(nctri);
	cfg = *nctri->nreg.reg_glb_cfg;
	cfg &= ~NDFC_CHANNEL_SEL;
	cfg |= (channel_sel & 0x1) << 2;
	*nctri->nreg.reg_glb_cfg = cfg;
	ndfc_soft_reset(nctri);
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void ndfc_set_default_rand_seed(struct _nand_controller_info *nctri)
{
	u32 cfg, random_seed = 0x4a80;

	cfg = *nctri->nreg.reg_ecc_ctl;
	cfg &= 0x0000ffff;
	cfg |= (random_seed << 16);
	*nctri->nreg.reg_ecc_ctl = cfg;
}

/*****************************************************************************
*Name         : _enable_randomize
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void ndfc_enable_randomize(struct _nand_controller_info *nctri)
{
	*nctri->nreg.reg_ecc_ctl |= NDFC_RANDOM_EN;
}

/*****************************************************************************
*Name         : _enable_randomize
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void ndfc_disable_randomize(struct _nand_controller_info *nctri)
{
	u32 cfg;

	cfg = *nctri->nreg.reg_ecc_ctl;
	cfg &= ~NDFC_RANDOM_EN;
	cfg &= 0x0000ffff;
	cfg |= (default_random_seed << 16);
	*nctri->nreg.reg_ecc_ctl = cfg;
}

/*****************************************************************************
*Name         : _enable_rb_int
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void ndfc_enable_rb_b2r_int(struct _nand_controller_info *nctri)
{
	*nctri->nreg.reg_int |= NDFC_B2R_INT_ENABLE;
}

/*****************************************************************************
*Name         : _enable_rb_int
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void ndfc_disable_rb_b2r_int(struct _nand_controller_info *nctri)
{
	*nctri->nreg.reg_int &= ~NDFC_B2R_INT_ENABLE;
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void ndfc_clear_rb_b2r_int(struct _nand_controller_info *nctri)
{
	*nctri->nreg.reg_sta = NDFC_RB_B2R;
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
u32 ndfc_get_rb_b2r_int_sta(struct _nand_controller_info *nctri)
{
	return (*nctri->nreg.reg_sta & NDFC_RB_B2R);
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
u32 ndfc_check_rb_b2r_int_occur(struct _nand_controller_info *nctri)
{
	return ((*nctri->nreg.reg_sta & NDFC_RB_B2R) &&
		(*nctri->nreg.reg_int & NDFC_B2R_INT_ENABLE));
}

/*****************************************************************************
*Name         : _enable_rb_int
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void ndfc_enable_cmd_int(struct _nand_controller_info *nctri)
{
	*nctri->nreg.reg_int |= NDFC_CMD_INT_ENABLE;
}

/*****************************************************************************
*Name         : _enable_rb_int
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void ndfc_disable_cmd_int(struct _nand_controller_info *nctri)
{
	*nctri->nreg.reg_int &= ~NDFC_CMD_INT_ENABLE;
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void ndfc_clear_cmd_int(struct _nand_controller_info *nctri)
{
	*nctri->nreg.reg_sta = NDFC_CMD_INT_FLAG;
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
u32 ndfc_get_cmd_int_sta(struct _nand_controller_info *nctri)
{
	return (*nctri->nreg.reg_sta & NDFC_CMD_INT_FLAG);
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
u32 ndfc_check_cmd_int_occur(struct _nand_controller_info *nctri)
{
	return ((*nctri->nreg.reg_sta & NDFC_CMD_INT_FLAG) &&
		(*nctri->nreg.reg_int & NDFC_CMD_INT_ENABLE));
}

/*****************************************************************************
*Name         : _enable_rb_int
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void ndfc_enable_dma_int(struct _nand_controller_info *nctri)
{
	*nctri->nreg.reg_int |= NDFC_DMA_INT_ENABLE;
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void ndfc_disable_dma_int(struct _nand_controller_info *nctri)
{
	*nctri->nreg.reg_int &= ~NDFC_DMA_INT_ENABLE;
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void ndfc_clear_dma_int(struct _nand_controller_info *nctri)
{
	*nctri->nreg.reg_sta = NDFC_DMA_INT_FLAG;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
u32 ndfc_get_dma_int_sta(struct _nand_controller_info *nctri)
{
	return (*nctri->nreg.reg_sta & NDFC_DMA_INT_FLAG);
}
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
u32 ndfc_check_dma_int_occur(struct _nand_controller_info *nctri)
{
	return ((*nctri->nreg.reg_sta & NDFC_DMA_INT_FLAG) &&
		(*nctri->nreg.reg_int & NDFC_DMA_INT_ENABLE));
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
s32 ndfc_wait_rb_ready_int(struct _nand_controller_info *nctri, u32 rb)
{
	s32 ret;
	u32 rb_check = 0;

	if (rb == 0)
		rb_check = NDFC_RB_STATE0;
	if (rb == 1)
		rb_check = NDFC_RB_STATE1;

	if ((*nctri->nreg.reg_sta & rb_check) == 0) {
		if (nctri->rb_ready_flag == 1) {
			goto wait_rb_ready_int;
		}

		ndfc_enable_rb_b2r_int(nctri);

		if (nand_rb_wait_time_out(nctri->channel_id,
					  &nctri->rb_ready_flag) == 0) {
			//dump_stack();
			PHY_ERR("_wait_rb ready_int int timeout, ch: 0x%x, "
				"sta: 0x%x\n",
				nctri->channel_id, *nctri->nreg.reg_sta);
		}
	}

wait_rb_ready_int:
	ret = wait_reg_status(nctri->nreg.reg_sta, rb_check, rb_check, 0xfffff);
	if (ret != 0) {
		PHY_ERR(
		    "_wait_rb ready_int wait timeout, ch: 0x%x, sta: 0x%x\n",
		    nctri->channel_id, *nctri->nreg.reg_sta);
		// return ret;
	}

	ndfc_disable_rb_b2r_int(nctri);

	ndfc_clear_rb_b2r_int(nctri);

	nctri->rb_ready_flag = 0;

	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
s32 ndfc_wait_rb_ready(struct _nand_controller_info *nctri, u32 rb)
{
	s32 ret;
	u32 rb_check = 0;

	if (rb == 0)
		rb_check = NDFC_RB_STATE0;
	if (rb == 1)
		rb_check = NDFC_RB_STATE1;

	ret =
	    wait_reg_status(nctri->nreg.reg_sta, rb_check, rb_check, 0xffffff);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
s32 ndfc_wait_all_rb_ready(struct _nand_controller_info *nctri)
{
	s32 i, ret = 0;

	for (i = 0; i < 2; i++) {
		ret |= ndfc_wait_rb_ready(nctri, i);
	}

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
s32 ndfc_write_wait_rb_ready(struct _nand_controller_info *nctri, u32 rb)
{
	if (nctri->write_wait_rb_mode == 0) {
		return ndfc_wait_rb_ready(nctri, rb);
	} else {
		return ndfc_wait_rb_ready_int(nctri, rb);
	}
}

/*****************************************************************************
*Name         : fill_nctri
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
s32 fill_nctri(struct _nand_controller_info *nctri)
{
	void *reg_base = NAND_GetIOBaseAddr(nctri->channel_id);

	nctri->type = NAND_GetNdfcVersion();
	nctri->dma_type = NAND_GetNdfcDmaMode();
	nctri->nreg.reg_ctl = (volatile u32 *)((u8 *)reg_base + 0x0000);
	nctri->nreg.reg_sta = (volatile u32 *)((u8 *)reg_base + 0x0004);
	nctri->nreg.reg_int = (volatile u32 *)((u8 *)reg_base + 0x0008);
	nctri->nreg.reg_timing_ctl = (volatile u32 *)((u8 *)reg_base + 0x000c);
	nctri->nreg.reg_timing_cfg = (volatile u32 *)((u8 *)reg_base + 0x0010);
	nctri->nreg.reg_addr_low = (volatile u32 *)((u8 *)reg_base + 0x0014);
	nctri->nreg.reg_addr_high = (volatile u32 *)((u8 *)reg_base + 0x0018);
	nctri->nreg.reg_data_block_mask =
	    (volatile u32 *)((u8 *)reg_base + 0x001c);
	nctri->nreg.reg_ndfc_cnt = (volatile u32 *)((u8 *)reg_base + 0x0020);
	nctri->nreg.reg_cmd = (volatile u32 *)((u8 *)reg_base + 0x0024);
	nctri->nreg.reg_read_cmd_set =
	    (volatile u32 *)((u8 *)reg_base + 0x0028);
	nctri->nreg.reg_write_cmd_set =
	    (volatile u32 *)((u8 *)reg_base + 0x002c);
	nctri->nreg.reg_ecc_ctl = (volatile u32 *)((u8 *)reg_base + 0x0034);
	nctri->nreg.reg_ecc_sta = (volatile u32 *)((u8 *)reg_base + 0x0038);
	nctri->nreg.reg_data_pattern_sta =
	    (volatile u32 *)((u8 *)reg_base + 0x003c);
	nctri->nreg.reg_efr = (volatile u32 *)((u8 *)reg_base + 0x0040);
	nctri->nreg.reg_rdata_sta_ctl =
	    (volatile u32 *)((u8 *)reg_base + 0x0044);
	nctri->nreg.reg_rdata_sta_0 = (volatile u32 *)((u8 *)reg_base + 0x0048);
	nctri->nreg.reg_rdata_sta_1 = (volatile u32 *)((u8 *)reg_base + 0x004c);
	nctri->nreg.reg_err_cnt0 = (volatile u32 *)((u8 *)reg_base + 0x0050);
	nctri->nreg.reg_err_cnt1 = (volatile u32 *)((u8 *)reg_base + 0x0054);
	nctri->nreg.reg_err_cnt2 = (volatile u32 *)((u8 *)reg_base + 0x0058);
	nctri->nreg.reg_err_cnt3 = (volatile u32 *)((u8 *)reg_base + 0x005c);
	nctri->nreg.reg_err_cnt4 = (volatile u32 *)((u8 *)reg_base + 0x0060);
	nctri->nreg.reg_err_cnt5 = (volatile u32 *)((u8 *)reg_base + 0x0064);
	nctri->nreg.reg_err_cnt6 = (volatile u32 *)((u8 *)reg_base + 0x0068);
	nctri->nreg.reg_err_cnt7 = (volatile u32 *)((u8 *)reg_base + 0x006c);
	nctri->nreg.reg_user_data_len_base =
	    (volatile u32 *)((u8 *)reg_base + 0x0070);
	nctri->nreg.reg_user_data_base =
	    (volatile u32 *)((u8 *)reg_base + 0x0080);
	nctri->nreg.reg_flash_sta = (volatile u32 *)((u8 *)reg_base + 0x00100);
	nctri->nreg.reg_cmd_repeat_cnt =
	    (volatile u32 *)((u8 *)reg_base + 0x00104);
	nctri->nreg.reg_cmd_repeat_interval =
	    (volatile u32 *)((u8 *)reg_base + 0x00108);
	nctri->nreg.reg_efnand_sta = (volatile u32 *)((u8 *)reg_base + 0x0110);
	nctri->nreg.reg_spare_area = (volatile u32 *)((u8 *)reg_base + 0x0114);
	nctri->nreg.reg_pat_id = (volatile u32 *)((u8 *)reg_base + 0x0118);
	nctri->nreg.reg_ddr2_spec_ctl =
	    (volatile u32 *)((u8 *)reg_base + 0x011c);
	nctri->nreg.reg_ndma_mode_ctl =
	    (volatile u32 *)((u8 *)reg_base + 0x0120);
	nctri->nreg.reg_valid_data_dma_cnt =
	    (volatile u32 *)((u8 *)reg_base + 0x012c);
	nctri->nreg.reg_data_dma_base =
	    (volatile u32 *)((u8 *)reg_base + 0x0130);
	nctri->nreg.reg_data_dma_size_2x =
	    (volatile u32 *)((u8 *)reg_base + 0x0170);
	nctri->nreg.reg_random_seed_x =
	    (volatile u32 *)((u8 *)reg_base + 0x0190);
	nctri->nreg.reg_dma_cnt = (volatile u32 *)((u8 *)reg_base + 0x0214);
	nctri->nreg.reg_emce_ctl = (volatile u32 *)((u8 *)reg_base + 0x0218);
	nctri->nreg.reg_emce_iv_fac_cmp_val =
	    (volatile u32 *)((u8 *)reg_base + 0x021c);
	nctri->nreg.reg_emce_iv_cal_fac_base =
	    (volatile u32 *)((u8 *)reg_base + 0x0220);
	nctri->nreg.reg_io_data = (volatile u32 *)((u8 *)reg_base + 0x02a0);
	nctri->nreg.reg_ndfc_ver = (volatile u32 *)((u8 *)reg_base + 0x02f0);
	nctri->nreg.reg_ldpc_ctl = (volatile u32 *)((u8 *)reg_base + 0x02fc);
	nctri->nreg.reg_enc_ldpc_mode_set =
	    (volatile u32 *)((u8 *)reg_base + 0x0300);
	nctri->nreg.reg_cor_ldpc_mode_set =
	    (volatile u32 *)((u8 *)reg_base + 0x0304);
	nctri->nreg.reg_c0_llr_tbl_addr =
	    (volatile u32 *)((u8 *)reg_base + 0x0308);
	nctri->nreg.reg_c1_llr_tbl_addr =
	    (volatile u32 *)((u8 *)reg_base + 0x0328);
	nctri->nreg.reg_ram0_base = (volatile u32 *)((u8 *)reg_base + 0x0400);
	nctri->nreg.reg_ram1_base = (volatile u32 *)((u8 *)reg_base + 0x0800);
	nctri->nreg.reg_glb_cfg = (volatile u32 *)((u8 *)reg_base + 0x0c00);
	nctri->nreg.reg_cmd_descr_base_addr =
	    (volatile u32 *)((u8 *)reg_base + 0x0c04);
	nctri->nreg.reg_cmd_descr_sta =
	    (volatile u32 *)((u8 *)reg_base + 0x0c08);
	nctri->nreg.reg_cmd_descr_intr =
	    (volatile u32 *)((u8 *)reg_base + 0x0c0c);
	nctri->nreg.reg_csic_bist_ctl_reg =
	    (volatile u32 *)((u8 *)reg_base + 0x0c10);
	nctri->nreg.reg_bist_start_addr =
	    (volatile u32 *)((u8 *)reg_base + 0x0c14);
	nctri->nreg.reg_bist_end_addr =
	    (volatile u32 *)((u8 *)reg_base + 0x0c18);
	nctri->nreg.reg_bist_data_mask =
	    (volatile u32 *)((u8 *)reg_base + 0x0c1c);
	return 0;
}
/*****************************************************************************
*Name         : save_nctri
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
s32 save_nctri(struct _nand_controller_info *nctri)
{
	nctri->nreg_bak.reg_ctl = *nctri->nreg.reg_ctl;
	nctri->nreg_bak.reg_sta = *nctri->nreg.reg_sta;
	nctri->nreg_bak.reg_int = *nctri->nreg.reg_int;
	nctri->nreg_bak.reg_timing_ctl = *nctri->nreg.reg_timing_ctl;
	nctri->nreg_bak.reg_timing_cfg = *nctri->nreg.reg_timing_cfg;
	nctri->nreg_bak.reg_addr_low = *nctri->nreg.reg_addr_low;
	nctri->nreg_bak.reg_addr_high = *nctri->nreg.reg_addr_high;
	nctri->nreg_bak.reg_data_block_mask = *nctri->nreg.reg_data_block_mask;
	nctri->nreg_bak.reg_ndfc_cnt = *nctri->nreg.reg_ndfc_cnt;
	nctri->nreg_bak.reg_cmd = *nctri->nreg.reg_cmd;
	nctri->nreg_bak.reg_read_cmd_set = *nctri->nreg.reg_read_cmd_set;
	nctri->nreg_bak.reg_write_cmd_set = *nctri->nreg.reg_write_cmd_set;
	nctri->nreg_bak.reg_ecc_ctl = *nctri->nreg.reg_ecc_ctl;
	nctri->nreg_bak.reg_ecc_sta = *nctri->nreg.reg_ecc_sta;
	nctri->nreg_bak.reg_data_pattern_sta =
	    *nctri->nreg.reg_data_pattern_sta;
	nctri->nreg_bak.reg_efr = *nctri->nreg.reg_efr;
	nctri->nreg_bak.reg_rdata_sta_ctl = *nctri->nreg.reg_rdata_sta_ctl;
	nctri->nreg_bak.reg_rdata_sta_0 = *nctri->nreg.reg_rdata_sta_0;
	nctri->nreg_bak.reg_rdata_sta_1 = *nctri->nreg.reg_rdata_sta_1;
	nctri->nreg_bak.reg_err_cnt0 = *nctri->nreg.reg_err_cnt0;
	nctri->nreg_bak.reg_err_cnt1 = *nctri->nreg.reg_err_cnt1;
	nctri->nreg_bak.reg_err_cnt2 = *nctri->nreg.reg_err_cnt2;
	nctri->nreg_bak.reg_err_cnt3 = *nctri->nreg.reg_err_cnt3;
	nctri->nreg_bak.reg_err_cnt4 = *nctri->nreg.reg_err_cnt4;
	nctri->nreg_bak.reg_err_cnt5 = *nctri->nreg.reg_err_cnt5;
	nctri->nreg_bak.reg_err_cnt6 = *nctri->nreg.reg_err_cnt6;
	nctri->nreg_bak.reg_err_cnt7 = *nctri->nreg.reg_err_cnt7;
	nctri->nreg_bak.reg_user_data_len_base =
	    *nctri->nreg.reg_user_data_len_base;
	nctri->nreg_bak.reg_user_data_base = *nctri->nreg.reg_user_data_base;
	nctri->nreg_bak.reg_flash_sta = *nctri->nreg.reg_flash_sta;
	nctri->nreg_bak.reg_cmd_repeat_cnt = *nctri->nreg.reg_cmd_repeat_cnt;
	nctri->nreg_bak.reg_cmd_repeat_interval =
	    *nctri->nreg.reg_cmd_repeat_interval;
	nctri->nreg_bak.reg_efnand_sta = *nctri->nreg.reg_efnand_sta;
	nctri->nreg_bak.reg_spare_area = *nctri->nreg.reg_spare_area;
	nctri->nreg_bak.reg_pat_id = *nctri->nreg.reg_pat_id;
	nctri->nreg_bak.reg_ddr2_spec_ctl = *nctri->nreg.reg_ddr2_spec_ctl;
	nctri->nreg_bak.reg_ndma_mode_ctl = *nctri->nreg.reg_ndma_mode_ctl;
	nctri->nreg_bak.reg_valid_data_dma_cnt =
	    *nctri->nreg.reg_valid_data_dma_cnt;
	nctri->nreg_bak.reg_data_dma_base = *nctri->nreg.reg_data_dma_base;
	nctri->nreg_bak.reg_data_dma_size_2x =
	    *nctri->nreg.reg_data_dma_size_2x;
	nctri->nreg_bak.reg_random_seed_x = *nctri->nreg.reg_random_seed_x;
	nctri->nreg_bak.reg_dma_cnt = *nctri->nreg.reg_dma_cnt;
	nctri->nreg_bak.reg_emce_ctl = *nctri->nreg.reg_emce_ctl;
	nctri->nreg_bak.reg_emce_iv_fac_cmp_val =
	    *nctri->nreg.reg_emce_iv_fac_cmp_val;
	nctri->nreg_bak.reg_emce_iv_cal_fac_base =
	    *nctri->nreg.reg_emce_iv_cal_fac_base;
	nctri->nreg_bak.reg_io_data = *nctri->nreg.reg_io_data;
	nctri->nreg_bak.reg_ndfc_ver = *nctri->nreg.reg_ndfc_ver;
	nctri->nreg_bak.reg_ldpc_ctl = *nctri->nreg.reg_ldpc_ctl;
	nctri->nreg_bak.reg_enc_ldpc_mode_set =
	    *nctri->nreg.reg_enc_ldpc_mode_set;
	nctri->nreg_bak.reg_cor_ldpc_mode_set =
	    *nctri->nreg.reg_cor_ldpc_mode_set;
	nctri->nreg_bak.reg_c0_llr_tbl_addr = *nctri->nreg.reg_c0_llr_tbl_addr;
	nctri->nreg_bak.reg_c1_llr_tbl_addr = *nctri->nreg.reg_c1_llr_tbl_addr;
	nctri->nreg_bak.reg_ram0_base = *nctri->nreg.reg_ram0_base;
	nctri->nreg_bak.reg_ram1_base = *nctri->nreg.reg_ram1_base;
	nctri->nreg_bak.reg_glb_cfg = *nctri->nreg.reg_glb_cfg;
	nctri->nreg_bak.reg_cmd_descr_base_addr =
	    *nctri->nreg.reg_cmd_descr_base_addr;
	nctri->nreg_bak.reg_cmd_descr_sta = *nctri->nreg.reg_cmd_descr_sta;
	nctri->nreg_bak.reg_cmd_descr_intr = *nctri->nreg.reg_cmd_descr_intr;
	nctri->nreg_bak.reg_csic_bist_ctl_reg =
	    *nctri->nreg.reg_csic_bist_ctl_reg;
	nctri->nreg_bak.reg_bist_start_addr = *nctri->nreg.reg_bist_start_addr;
	nctri->nreg_bak.reg_bist_end_addr = *nctri->nreg.reg_bist_end_addr;
	nctri->nreg_bak.reg_bist_data_mask = *nctri->nreg.reg_bist_data_mask;
	return 0;
}

/*****************************************************************************
*Name         : recover_nctri
*Description  :
*Parameter    :
*Return       : NULL
*Note         :don't recover all register --zzm 20180309
*****************************************************************************/
s32 recover_nctri(struct _nand_controller_info *nctri)
{
	//        *nctri->nreg.reg_sta = nctri->nreg_bak.reg_sta;
	//        *nctri->nreg.reg_int = nctri->nreg_bak.reg_int;
	*nctri->nreg.reg_timing_ctl = nctri->nreg_bak.reg_timing_ctl;
	*nctri->nreg.reg_timing_cfg = nctri->nreg_bak.reg_timing_cfg;
	//        *nctri->nreg.reg_addr_low = nctri->nreg_bak.reg_addr_low;
	//        *nctri->nreg.reg_addr_high = nctri->nreg_bak.reg_addr_high;
	//        *nctri->nreg.reg_sect_num = nctri->nreg_bak.reg_sect_num;
	//        *nctri->nreg.reg_byte_cnt = nctri->nreg_bak.reg_byte_cnt;
	//        *nctri->nreg.reg_cmd = nctri->nreg_bak.reg_cmd;
	//        *nctri->nreg.reg_read_cmd_set =
	//        nctri->nreg_bak.reg_read_cmd_set;
	//        *nctri->nreg.reg_write_cmd_set =
	//        nctri->nreg_bak.reg_write_cmd_set;
	//        *nctri->nreg.reg_io = nctri->nreg_bak.reg_io;
	//        *nctri->nreg.reg_ecc_ctl = nctri->nreg_bak.reg_ecc_ctl;
	//        *nctri->nreg.reg_ecc_sta = nctri->nreg_bak.reg_ecc_sta;
	//        *nctri->nreg.reg_debug = nctri->nreg_bak.reg_debug;
	//        *nctri->nreg.reg_err_cnt0 = nctri->nreg_bak.reg_err_cnt0;
	//        *nctri->nreg.reg_err_cnt1 = nctri->nreg_bak.reg_err_cnt1;
	//        *nctri->nreg.reg_err_cnt2 = nctri->nreg_bak.reg_err_cnt2;
	//        *nctri->nreg.reg_err_cnt3 = nctri->nreg_bak.reg_err_cnt3;
	//        *nctri->nreg.reg_user_data_base =
	//        nctri->nreg_bak.reg_user_data_base;
	//        *nctri->nreg.reg_efnand_sta = nctri->nreg_bak.reg_efnand_sta;
	//        *nctri->nreg.reg_spare_area = nctri->nreg_bak.reg_spare_area;
	*nctri->nreg.reg_pat_id = nctri->nreg_bak.reg_pat_id;
	//        *nctri->nreg.reg_mbus_dma_addr =
	//        nctri->nreg_bak.reg_mbus_dma_addr;
	*nctri->nreg.reg_dma_cnt = nctri->nreg_bak.reg_dma_cnt;
	//        *nctri->nreg.reg_ram0_base = nctri->nreg_bak.reg_ram0_base;
	//        *nctri->nreg.reg_ram1_base = nctri->nreg_bak.reg_ram1_base;
	*nctri->nreg.reg_ctl = nctri->nreg_bak.reg_ctl;
	*nctri->nreg.reg_ddr2_spec_ctl = nctri->nreg_bak.reg_ddr2_spec_ctl;
	*nctri->nreg.reg_glb_cfg = nctri->nreg_bak.reg_glb_cfg;
	return 0;
}

/*****************************************************************************
*Name         : ndfc_change_page_size
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
s32 ndfc_change_page_size(struct _nand_controller_info *nctri)
{
	struct _nand_chip_info *nci = nctri->nci; // get first nand flash chip

	u32 cfg = *nctri->nreg.reg_ctl;

	cfg &= ~NDFC_PAGE_SIZE;
	if (nci->sector_cnt_per_page == 2)
		cfg |= 0x0 << 8; // 1K
	else if (nci->sector_cnt_per_page == 4)
		cfg |= 0x1 << 8; // 2K
	else if (nci->sector_cnt_per_page == 8)
		cfg |= 0x2 << 8; // 4K
	else if (nci->sector_cnt_per_page == 16)
		cfg |= 0x3 << 8; // 8K
	else if ((nci->sector_cnt_per_page > 16) &&
		 (nci->sector_cnt_per_page < 32))
		cfg |= 0x4 << 8; // 12K
	else if (nci->sector_cnt_per_page == 32)
		cfg |= 0x4 << 8; // 16K
	else if (nci->sector_cnt_per_page == 64)
		cfg |= 0x5 << 8; // 32K
	else {
		PHY_ERR("ndfc_change_page_size, wrong page size!\n");
		return ERR_NO_34;
	}
	*nctri->nreg.reg_ctl = cfg;

	*nctri->nreg.reg_spare_area =
	    (nci->sector_cnt_per_page << 9); // spare area

	return 0;
}

/*****************************************************************************
*Name         :
*Description  :Command FSM
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
s32 _normal_cmd_io_send(struct _nand_controller_info *nctri,
			struct _nctri_cmd *icmd)
{
	s32 ret = 0;
	u32 cmd_val = 0;
	// u32 rb = ndfc_get_selected_rb_no(nctri);
	u32 reg_val;

	//===================================
	//     configure cmd
	//===================================
	if (icmd->cmd_send) {
		cmd_val |= icmd->cmd & 0xff;
		cmd_val |= NDFC_SEND_CMD1;
		if (icmd->cmd_wait_rb)
			cmd_val |= NDFC_WAIT_FLAG;
	}

	//===================================
	//     configure address
	//===================================
	if (icmd->cmd_acnt) {
		_set_addr(nctri, icmd->cmd_acnt, icmd->cmd_addr);
		cmd_val |= ((icmd->cmd_acnt - 1) & 0x7) << 16;
		cmd_val |= NDFC_SEND_ADR;
	}

	if ((ndfc_wait_cmdfifo_free(nctri) != 0) ||
	    (ndfc_wait_fsm_ready(nctri) != 0)) {
		PHY_ERR("_normal_cmd_io_send, wait cmd fifo free timeout!\n");
		return ERR_TIMEOUT;
	}

	//===================================
	//     configure data
	//===================================
	if (icmd->cmd_trans_data_nand_bus) {
		*nctri->nreg.reg_ndfc_cnt = (icmd->cmd_mdata_len & 0x3ff);
		cmd_val |= NDFC_DATA_TRANS;
		if (icmd->cmd_direction) // write
			cmd_val |= NDFC_ACCESS_DIR;

		if (icmd->cmd_swap_data) {
			if (icmd->cmd_swap_data_dma) {
				cmd_val |= NDFC_DATA_SWAP_METHOD; // dma
				ndfc_dma_config_start(
				    nctri, icmd->cmd_direction,
				    icmd->cmd_mdata_addr, icmd->cmd_mdata_len);
			} else {
				// set AHB mode//zengyu
				reg_val = *nctri->nreg.reg_ctl;
				reg_val &= ~(0x1U << 14);
				*nctri->nreg.reg_ctl = reg_val;
				if (icmd->cmd_direction) {// write
					MEMCPY(
					    (void *)nctri->nreg.reg_ram0_base,
					    (void *)icmd->cmd_mdata_addr,
					    icmd->cmd_mdata_len);
				}
			}
		}
	}

	//===================================
	//     send cmd id
	//===================================
	*nctri->nreg.reg_cmd = cmd_val;

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :Command FSM
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
s32 _normal_cmd_io_wait(struct _nand_controller_info *nctri,
			struct _nctri_cmd *icmd)
{
	s32 ret = 0;
	u32 rb = ndfc_get_selected_rb_no(nctri);
	// u32 reg_val;

	// wait cmd finish
	ret = ndfc_wait_cmd_finish(nctri);
	if (ret != 0) {
		PHY_ERR("_normal_cmd io_wait, wait cmd finish timeout 0x%x!\n",
			icmd->cmd);
	}

	// check data
	if (icmd->cmd_swap_data) {
		if (icmd->cmd_swap_data_dma != 0) {
			ret |= ndfc_wait_dma_end(nctri, icmd->cmd_direction,
						 icmd->cmd_mdata_addr,
						 icmd->cmd_mdata_len);
			if (ret != 0) {
				PHY_ERR(
				    "_normal_cmd io_wait, wait dma timeout!\n");
			}
		} else {
			//			//set AHB mode//zengyu
			//			reg_val = *nctri->nreg.reg_ctl;
			//			reg_val &= ~(0x1U<<14);
			//			*nctri->nreg.reg_ctl = reg_val;
			if (icmd->cmd_direction == 0) {// read
				MEMCPY((void *)icmd->cmd_mdata_addr,
				       (void *)nctri->nreg.reg_ram0_base,
				       icmd->cmd_mdata_len);
			}
		}
	}

	// check rb status
	if (icmd->cmd_wait_rb) {
		if (icmd->cmd == CMD_RESET)
			ret = ndfc_wait_rb_ready(nctri, rb);
		else
			ret = ndfc_write_wait_rb_ready(nctri, rb);
		if (ret != 0) {
			PHY_ERR("_normal cmd io_wait, sw wait reset rb error "
				"0x%x, wait mode %d!\n",
				*nctri->nreg.reg_sta, icmd->cmd_wait_rb);
		}
	}

	if (ret)
		return ERR_NO_33;
	else
		return 0;
}

/*****************************************************************************
*Name         :
*Description  :Command FSM
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
s32 _batch_cmd_io_send(struct _nand_controller_info *nctri,
		       struct _nctri_cmd_seq *cmd_list)
{
	s32 ret = 0;
	u32 cmd0, cmd1, cmd2, cmd3, cmd_val = 0;
	u32 rb = ndfc_get_selected_rb_no(nctri);

	// PHY_DBG("before send cmd: %d 0x%x 0x%x\n", nctri->nci->randomizer,
	// *(nctri->nreg.reg_ecc_ctl), *(nctri->nreg.reg_ecc_sta));

	if (cmd_list->cmd_type != 1) {
		PHY_ERR("_batch_cmd io_send, cmd type error, %d!\n",
			cmd_list->cmd_type);
		return ERR_NO_32;
	}

	//===================================
	//     check FSM
	//===================================
	if (nctri->write_wait_rb_before_cmd_io) {
		ndfc_write_wait_rb_ready(nctri, rb);
	}
	ndfc_wait_rb_ready(nctri, rb);

	if ((ndfc_wait_cmdfifo_free(nctri) != 0) ||
	    (ndfc_wait_fsm_ready(nctri) != 0)) {
		PHY_ERR("_batch cmd io send, wait cmd fifo free timeout!\n");
		// print_cmd_seq(cmd_list);
		return ERR_TIMEOUT;
	}
	*nctri->nreg.reg_ctl |= 1 << 14; // songwj 20171115
	if (nctri->channel_sel == 0) {
		*nctri->nreg.reg_ndfc_cnt = 0x400; // songwj 20171115
		*nctri->nreg.reg_sta |=
		    *nctri->nreg.reg_sta &
		    (NDFC_RB_B2R | NDFC_CMD_INT_FLAG | NDFC_DMA_INT_FLAG);
	} else {
		*nctri->nreg.reg_ndfc_cnt =
		    0x800; // zhouk for ldpc (why is no 0x800? by Zzm)
		*nctri->nreg.reg_sta |=
		    (NDFC_RB_B2R | NDFC_CMD_INT_FLAG | NDFC_DMA_INT_FLAG);
	}

	//===================================
	//     configure cmd
	//===================================
	cmd0 = cmd_list->nctri_cmd[0].cmd;
	cmd1 = cmd_list->nctri_cmd[1].cmd;
	cmd2 = cmd_list->nctri_cmd[2].cmd;
	cmd3 = cmd_list->nctri_cmd[3].cmd;
	if (cmd_list->nctri_cmd[0].cmd_direction == 1) {
		// batch write cmd set
		*nctri->nreg.reg_write_cmd_set =
		    (((cmd1 & 0xff) << 0) | ((cmd2 & 0xff) << 8));
	} else {
		// batch read cmd set
		*nctri->nreg.reg_read_cmd_set =
		    (((cmd1 & 0xff) << 0) | ((cmd2 & 0xff) << 8) |
		     ((cmd3 & 0xff) << 16));
	}

	cmd_val |= cmd0 & 0xff;
	if (cmd_list->nctri_cmd[0].cmd_direction) {
		cmd_val |= NDFC_ACCESS_DIR;
	}
	cmd_val |= NDFC_DATA_TRANS;
	cmd_val |= NDFC_SEND_CMD1;
	if (nctri->current_op_type != 1) {// only write controller don't wait rb
		cmd_val |= NDFC_WAIT_FLAG; // wait rb
	}
	cmd_val |= NDFC_SEND_CMD2;
	if (cmd_list->nctri_cmd[0].cmd_swap_data) {
		cmd_val |= NDFC_DATA_SWAP_METHOD; // dma
	}
	if (cmd_list->ecc_layout) {
		cmd_val |= NDFC_SEQ;
	}
	if (nctri->random_cmd2_send_flag) {
		*nctri->nreg.reg_read_cmd_set |= (nctri->random_cmd2 << 24);
		cmd_val |= NDFC_SEND_RAN_CMD2;
	} else {
		*nctri->nreg.reg_read_cmd_set &= (~(0xff << 24));
		cmd_val &= (~NDFC_SEND_RAN_CMD2);
	}

	cmd_val |= ((nctri->random_addr_num << 8) & NDFC_ADR_NUM_IN_PAGECMD);

	cmd_val |= 0x2U << 30; // page command

	//===================================
	//     configure address
	//===================================
	if (cmd_list->nctri_cmd[0].cmd_acnt) {
		_set_addr(nctri, cmd_list->nctri_cmd[0].cmd_acnt,
			  cmd_list->nctri_cmd[0].cmd_addr);
		cmd_val |= ((cmd_list->nctri_cmd[0].cmd_acnt - 1) & 0x7) << 16;
		cmd_val |= NDFC_SEND_ADR;
	}
	//===================================
	//     configure dma
	//===================================
	if (cmd_list->nctri_cmd[0].cmd_mdata_addr != NULL) {
		ndfc_dma_config_start(nctri,
				      cmd_list->nctri_cmd[0].cmd_direction,
				      cmd_list->nctri_cmd[0].cmd_mdata_addr,
				      cmd_list->nctri_cmd[0].cmd_mdata_len);
	}
	*nctri->nreg.reg_data_block_mask =
	    cmd_list->nctri_cmd[0].cmd_data_block_mask;

	/*boot0 LDPC*/
	if (nctri->channel_sel == 1) {
		ndfc_channel_select(nctri, 1);
		ndfc_boot0_ldpc_mode(nctri);
	}

	save_nctri(nctri);
	// ndfc_print_reg(nctri);

	*nctri->nreg.reg_cmd = cmd_val;

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :Command FSM
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
s32 _batch_cmd_io_wait(struct _nand_controller_info *nctri,
		       struct _nctri_cmd_seq *cmd_list)
{
	s32 ret = 0;
	struct _nctri_cmd *icmd = &(cmd_list->nctri_cmd[0]);
	u32 rb = ndfc_get_selected_rb_no(nctri);

	// check data
	if (cmd_list->nctri_cmd[0].cmd_mdata_addr != 0) {
		ret |= ndfc_wait_dma_end(nctri, icmd->cmd_direction,
					 icmd->cmd_mdata_addr,
					 icmd->cmd_mdata_len);
		if (ret) {
			PHY_ERR("_batch cmd io wait, wait dma timeout!\n");
			ndfc_print_reg(nctri);
			PHY_ERR("wait dma timeout end!\n");
		}
	}

	// wait cmd finish
	ret = ndfc_wait_cmd_finish(nctri);
	if (ret) {
		PHY_ERR("_batch cmd io wait, wait cmd finish timeout 0x%x!\n",
			icmd->cmd);
		ndfc_print_reg(nctri);
		ret = 0;
	}

	// check rb status
	if (nctri->current_op_type == 1) {// write
		if (nctri->write_wait_rb_before_cmd_io == 0) {
			ndfc_write_wait_rb_ready(nctri, rb);
		}
	} else {
		if (icmd->cmd_wait_rb) {
			// ndfc_wait_rb_ready(nctri,rb);
			ndfc_write_wait_rb_ready(nctri, rb);
		}
	}

	if (nctri->random_cmd2_send_flag) {
		*nctri->nreg.reg_read_cmd_set &= (~(0xf << 28));
		*nctri->nreg.reg_cmd &= (~NDFC_SEND_RAN_CMD2);
	}

	if (ret)
		return ERR_NO_31;
	else
		return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
void ndfc_clean_cmd_seq(struct _nctri_cmd_seq *cmd_list)
{
	MEMSET(cmd_list, 0x0, sizeof(struct _nctri_cmd_seq));
	return;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
void print_cmd_seq(struct _nctri_cmd_seq *cmd_list)
{
	int i, j;
	PHY_DBG("cmd_type:0x%x!\n", cmd_list->cmd_type);
	PHY_DBG("ecc_layout:0x%x!\n", cmd_list->ecc_layout);
	// PHY_ERR("row_addr_auto_inc:0x%x!\n", cmd_list->row_addr_auto_inc);
	PHY_DBG("re_start_cmd:0x%x!\n", cmd_list->re_start_cmd);
	PHY_DBG("re_end_cmd:0x%x!\n", cmd_list->re_end_cmd);
	PHY_DBG("re_cmd_times:0x%x!\n", cmd_list->re_cmd_times);

	for (i = 0; i < MAX_CMD_PER_LIST; i++) {
		PHY_DBG("==========0x%x===========\n", i);
		if (cmd_list->nctri_cmd[i].cmd_valid != 0) {
			PHY_DBG("cmd:0x%x!\n", cmd_list->nctri_cmd[i].cmd);
			PHY_DBG("cmd_send:0x%x!\n",
				cmd_list->nctri_cmd[i].cmd_send);
			PHY_DBG("cmd_wait_rb:0x%x!\n",
				cmd_list->nctri_cmd[i].cmd_wait_rb);
			PHY_DBG("cmd_acnt:0x%x!\n",
				cmd_list->nctri_cmd[i].cmd_acnt);
			PHY_DBG("cmd_trans_data_nand_bus:0x%x!\n",
				cmd_list->nctri_cmd[i].cmd_trans_data_nand_bus);
			PHY_DBG("cmd_swap_data:0x%x!\n",
				cmd_list->nctri_cmd[i].cmd_swap_data);
			PHY_DBG("cmd_swap_data_dma:0x%x!\n",
				cmd_list->nctri_cmd[i].cmd_swap_data_dma);
			PHY_DBG("cmd_direction:0x%x!\n",
				cmd_list->nctri_cmd[i].cmd_direction);
			PHY_DBG("cmd_mdata_len:0x%x!\n",
				cmd_list->nctri_cmd[i].cmd_mdata_len);
			PHY_DBG("cmd_mdata_addr:0x%x!\n",
				cmd_list->nctri_cmd[i].cmd_mdata_addr);
			for (j = 0; j < MAX_CMD_PER_LIST; j++)
				PHY_DBG("cmd_addr[%d]:0x%x!\n", j,
					cmd_list->nctri_cmd[i].cmd_addr[j]);
		}
	}
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
s32 ndfc_execute_cmd(struct _nand_controller_info *nctri,
		     struct _nctri_cmd_seq *cmd_list)
{
	s32 ret = 0;
	s32 c = 0;
	s32 rb;
	struct _nctri_cmd *pcmd;

	rb = ndfc_get_selected_rb_no(nctri);
	ndfc_wait_rb_ready(nctri, rb);

	if (cmd_list->cmd_type == CMD_TYPE_NORMAL) {
		for (c = 0; c < MAX_CMD_PER_LIST; c++) {
			pcmd = &cmd_list->nctri_cmd[c];
			if (!pcmd->cmd_valid) {
				// PHY_DBG("ndfc_execute_cmd, no more cmd, total
				// cmd %d\n", c);
				break;
			}
			ret = _normal_cmd_io_send(nctri, pcmd);
			if (ret) {
				PHY_ERR("ndfc_execute_cmd, send normal cmd %d "
					"error!\n",
					c);
				return ret;
			}
			ret = _normal_cmd_io_wait(nctri, pcmd);
			if (ret) {
				PHY_ERR("ndfc_execute_cmd, wait normal cmd %d "
					"error!\n",
					c);
				return ret;
			}
		}
	} else if (cmd_list->cmd_type == CMD_TYPE_BATCH) {
		ret = _batch_cmd_io_send(nctri, cmd_list);
		if (ret) {
			PHY_ERR("ndfc_execute_cmd, send batch cmd %d error!\n",
				c);
			return ret;
		}
		ret = _batch_cmd_io_wait(nctri, cmd_list);
		if (ret) {
			PHY_ERR("ndfc_execute_cmd, wait batch cmd %d error!\n",
				c);
			return ret;
		}
	} else {
		PHY_ERR("ndfc_execute_cmd, wrong cmd type, %d!\n",
			cmd_list->cmd_type);
		return ERR_NO_30;
	}

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
s32 ndfc_get_spare_data(struct _nand_controller_info *nctri, u8 *sbuf,
			u32 udata_cnt)
{
	s32 i;

	if (sbuf == NULL) {
		PHY_ERR("ndfc_get_spare_data, wrong input parameter!\n");
		return ERR_NO_29;
	}

	for (i = 0; i < udata_cnt; i++) {
		sbuf[i] = *((volatile u8 *)nctri->nreg.reg_user_data_base + i);
		// PHY_DBG("%x ", sbuf[i]);
	}
	// PHY_DBG("\n");

	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
s32 ndfc_set_spare_data(struct _nand_controller_info *nctri, u8 *sbuf,
			u32 udata_cnt)
{
	s32 i;
	u32 cfg = 0;

	if (sbuf == NULL) {
		PHY_ERR("ndfc set spare data, wrong input parameter!\n");
		return ERR_NO_28;
	}

	for (i = 0; i < udata_cnt; i = i + 4) {
		*(nctri->nreg.reg_user_data_base + (i >> 2)) =
		    sbuf[i + 3] << 24 | sbuf[i + 2] << 16 | sbuf[i + 1] << 8 |
		    sbuf[i];
	}
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         : udata_cnt is 4 bytes aligned
*****************************************************************************/
s32 ndfc_set_user_data_len(struct _nand_controller_info *nctri)
{
	s32 i;
	u32 cfg = 0;

	for (i = 0; i < MAX_ECC_BLK_CNT; i++) {
		cfg |= nctri->nctri_cmd_seq.udata_len_mode[i]
		       << ((i & 0x07) << 2); // 4 bit indecate one ecc block
					     // mode. a 32 register include 8
					     // ecc block mode.
		if (((i + 1) & 0x07) == 0) {
			*((volatile u32 *)nctri->nreg.reg_user_data_len_base +
			  (i >> 3)) = cfg; // cheney modify
			cfg = 0;
		}
	}

	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         : udata_cnt is 4 bytes aligned
*****************************************************************************/
s32 ndfc_set_user_data_len_cfg(struct _nand_controller_info *nctri,
			       u32 udata_cnt)
{
	u32 udata_len[10] = {0, 4, 8, 12, 16, 20, 24, 28, 32};
	u32 i, ecc_block_cnt, last_udata_len;
	u8 last_udata_len_mode = 0;

	ecc_block_cnt = (udata_cnt + MAX_UDATA_LEN_FOR_ECCBLOCK - 1) /
			MAX_UDATA_LEN_FOR_ECCBLOCK;
	// ecc_block_cnt = (udata_cnt + MAX_UDATA_LEN_FOR_ECCBLOCK ) /
	// MAX_UDATA_LEN_FOR_ECCBLOCK;
	last_udata_len = udata_cnt % MAX_UDATA_LEN_FOR_ECCBLOCK;
	if (udata_cnt && (!last_udata_len))
		last_udata_len = 32;

	for (i = 0; i < 10; i++) {
		if (last_udata_len == udata_len[i]) {
			last_udata_len_mode = i;
			break;
		}
	}

	for (i = 0; i < MAX_ECC_BLK_CNT; i++) {
		if (i < (ecc_block_cnt - 1))
			nctri->nctri_cmd_seq.udata_len_mode[i] = 8;
		else if (i == (ecc_block_cnt - 1))
			nctri->nctri_cmd_seq.udata_len_mode[i] =
			    last_udata_len_mode;
		else
			nctri->nctri_cmd_seq.udata_len_mode[i] = 0;
	}

	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         : udata_cnt is 4 bytes aligned
*****************************************************************************/
s32 ndfc_set_user_data_len_cfg_4bytesper1k(struct _nand_controller_info *nctri,
					   u32 udata_cnt)
{

	u32 i = 0;
	u32 ecc_block_cnt;

	ecc_block_cnt = (udata_cnt + UDATA_LEN_FOR_4BYTESPER1K - 1) /
			UDATA_LEN_FOR_4BYTESPER1K;

	for (i = 0; i < MAX_ECC_BLK_CNT; i++) {
		if (i < ecc_block_cnt) {
			nctri->nctri_cmd_seq.udata_len_mode[i] = 1;
		} else {
			nctri->nctri_cmd_seq.udata_len_mode[i] = 0;
		}
	}

	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int ndfc_is_toogle_interface(struct _nand_controller_info *nctri)
{
	u32 cfg = 0;
	cfg = *nctri->nreg.reg_ctl;
	if ((cfg >> 18) & 0x3) {
		return 1;
	}
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int ndfc_set_legacy_interface(struct _nand_controller_info *nctri)
{
	u32 cfg = 0;
	cfg = *nctri->nreg.reg_ctl;

	cfg &= ~(0x3 << 18);
	*nctri->nreg.reg_ctl = cfg;

	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int ndfc_set_toogle_interface(struct _nand_controller_info *nctri)
{
	u32 cfg = 0;
	cfg = *nctri->nreg.reg_ctl;
	cfg |= 0x3 << 18;
	*nctri->nreg.reg_ctl = cfg;

	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int ndfc_set_dummy_byte(struct _nand_controller_info *nctri, int dummy_byte)
{
	u32 cfg = 0;
	cfg = *nctri->nreg.reg_efr;
	cfg &= (~NDFC_DUMMY_BYTE_CNT);
	cfg |= (dummy_byte & 0xff) << 16;

	*nctri->nreg.reg_efr = cfg;

	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
void ndfc_enable_dummy_byte(struct _nand_controller_info *nctri)
{
	u32 cfg = 0;
	cfg = *nctri->nreg.reg_efr;
	cfg |= (1 << 24);
	*nctri->nreg.reg_efr = cfg;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
void ndfc_disable_dummy_byte(struct _nand_controller_info *nctri)
{
	u32 cfg = 0;
	cfg = *nctri->nreg.reg_efr;
	cfg &= ~(1 << 24);
	*nctri->nreg.reg_efr = cfg;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
void ndfc_get_nand_interface(struct _nand_controller_info *nctri,
			     u32 *pddr_type, u32 *psdr_edo, u32 *pddr_edo,
			     u32 *pddr_delay)
{
	u32 cfg = 0;

	/* ddr type */
	cfg = *nctri->nreg.reg_ctl;
	*pddr_type = (cfg >> 18) & 0x3;
	if (nctri->type == NDFC_VERSION_V2)
		*pddr_type |= (((cfg >> 28) & 0x1) << 4);

	/* edo && delay */
	cfg = *nctri->nreg.reg_timing_ctl;
	*psdr_edo = (cfg >> 8) & 0x3;
	*pddr_edo = (cfg >> 8) & 0xf;
	*pddr_delay = cfg & 0x3f;

	return;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
s32 ndfc_change_nand_interface(struct _nand_controller_info *nctri,
			       u32 ddr_type, u32 sdr_edo, u32 ddr_edo,
			       u32 ddr_delay)
{
	u32 cfg = 0;

	/* ddr type */
	cfg = *nctri->nreg.reg_ctl;
	cfg &= ~(0x3U << 18);
	cfg |= (ddr_type & 0x3) << 18;
	if (nctri->type == NDFC_VERSION_V2) {
		cfg &= ~(0x1 << 28);
		cfg |= ((ddr_type >> 4) & 0x1) << 28;
	}
	*nctri->nreg.reg_ctl = cfg;

	/* edo && delay */
	cfg = *nctri->nreg.reg_timing_ctl;
	cfg &= ~((0xf << 8) | 0x3f);
	if (ddr_type == 0) {
		cfg |= (sdr_edo << 8);
	} else {
		cfg |= (ddr_edo << 8);
		cfg |= ddr_delay;
	}
	*nctri->nreg.reg_timing_ctl = cfg;

	/*
		 ndfc's timing cfg
		 1. default value: 0x95
		 2. bit-16, tCCS=1 for micron l85a, nvddr-100mhz
	 */
	*nctri->nreg.reg_timing_cfg = 0x10095;

	return 0;
}

/*****************************************************************************
*Name         : ndfc_emce_ctrl_setting
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void ndfc_emce_ctrl_setting(struct _nand_controller_info *nctri,
			    u32 enable_flag)
{
	if (enable_flag)
		*nctri->nreg.reg_emce_ctl = 1;
	else
		*nctri->nreg.reg_emce_ctl = 0;
}

/*****************************************************************************
*Name         : ndfc_emce_fac_compare_value_setting
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void ndfc_emce_fac_compare_value_setting(struct _nand_controller_info *nctri,
					 u32 value)
{
	*nctri->nreg.reg_emce_iv_fac_cmp_val = value;
}

/*****************************************************************************
*Name         : ndfc_emce_calculate_factor_setting_for_serial_addr
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void ndfc_emce_calculate_factor_setting_for_serial_addr(
    struct _nand_controller_info *nctri, u32 sector_addr, u32 sector_mapping)
{
	int i = 0;
	for (i = 0; i < 32; i++) {
		if ((sector_mapping >> i) & 0x01) {
			// need encryption sector
			*nctri->nreg.reg_emce_iv_cal_fac_base = sector_addr + i;
		} else {
			// not need encryption sector
			*nctri->nreg.reg_emce_iv_cal_fac_base =
			    *nctri->nreg.reg_emce_iv_fac_cmp_val;
		}
	}
}

/*****************************************************************************
*Name         : ndfc_emce_calculate_factor_setting_for_serial_addr
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void ndfc_emce_calculate_factor_setting(struct _nand_controller_info *nctri,
					u32 *values, u32 offset_start,
					u32 lenth)
{
	int i = offset_start;
	if ((offset_start + lenth) > 32) {
		return;
	}

	for (i = offset_start; i < offset_start + lenth; i++) {
		*nctri->nreg.reg_emce_iv_cal_fac_base = values[i];
	}
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void _setup_ndfc_ddr2_para(struct _nand_controller_info *nctri)
{
#if 0
	u32 reg_val;
	struct _nand_chip_info *nci = nctri->nci;
	//set ndfc's ddr2 feature
	reg_val = 0;
	reg_val |= (nci->nfc_init_ddr_info->en_ext_verf & 0x1);
	reg_val |= ((nci->nfc_init_ddr_info->en_dqs_c   & 0x1) << 1);
	reg_val |= ((nci->nfc_init_ddr_info->en_re_c    & 0x1) << 2);


	if (nci->nfc_init_ddr_info->dout_re_warmup_cycle == 0)
		reg_val |= ((0x0 & 0x7) << 8);
	else if (nci->nfc_init_ddr_info->dout_re_warmup_cycle == 1)
		reg_val |= ((0x1 & 0x7) << 8);
	else if (nci->nfc_init_ddr_info->dout_re_warmup_cycle == 2)
		reg_val |= ((0x2 & 0x7) << 8);
	else if (nci->nfc_init_ddr_info->dout_re_warmup_cycle == 4)
		reg_val |= ((0x3 & 0x7) << 8);
	else
		reg_val |= ((0x0 & 0x7) << 8);


	if (nci->nfc_init_ddr_info->din_dqs_warmup_cycle == 0)
		reg_val |= ((0x0 & 0x7) << 12);
	else if (nci->nfc_init_ddr_info->din_dqs_warmup_cycle == 1)
		reg_val |= ((0x1 & 0x7) << 12);
	else if (nci->nfc_init_ddr_info->din_dqs_warmup_cycle == 2)
		reg_val |= ((0x2 & 0x7) << 12);
	else if (nci->nfc_init_ddr_info->din_dqs_warmup_cycle == 4)
		reg_val |= ((0x3 & 0x7) << 12);
	else
		reg_val |= ((0x0 & 0x7) << 12);

	NDFC_WRITE_REG_SPEC_CTL(reg_val);
#endif

	return;
}

/*****************************************************************************
*Name         :
*Description  :set defualt timming  config base on timming controller
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
void _set_ndfc_def_timing_param(struct _nand_chip_info *nci)
{
	u32 reg_val, sync_mode;

	// reg_val = NDFC_READ_REG_CTL();
	reg_val = *(nci->nctri->nreg.reg_ctl);
	sync_mode = (reg_val >> 18) & 0x3;
	sync_mode |= (((reg_val >> 28) & 0x1) << 4);

	if (sync_mode == 0) {// async
		reg_val = 0;
		reg_val |= 0x1 << 16; // tCCS
		reg_val |= 0x0 << 14; // tCLHZ
		reg_val |= 0x0 << 12; // tCS
		reg_val |= 0x0 << 11; // tCDQSS
		reg_val |= 0x1 << 8;  // tCAD
		reg_val |= 0x0 << 6;  // tRHW
		reg_val |= 0x0 << 4;  // tWHR
		reg_val |= 0x0 << 2;  // tADL
		reg_val |= 0x0;       //<<0;  //tWB
	} else if (sync_mode == 2) {    // onfi ddr
		reg_val = 0;
		reg_val |= 0x1 << 16; // tCCS
		reg_val |= 0x0 << 14; // tCLHZ
		reg_val |= 0x0 << 12; // tCS
		reg_val |= 0x0 << 11; // tCDQSS
		reg_val |= 0x1 << 8;  // tCAD
		reg_val |= 0x1 << 6;  // tRHW
		reg_val |= 0x0 << 4;  // tWHR
		reg_val |= 0x0 << 2;  // tADL
		reg_val |= 0x1;       //<<0;  //tWB
	} else if (sync_mode == 3) {// toggle ddr
		reg_val = 0;
		reg_val |= 0x2 << 16; // tCCS
		reg_val |= 0x0 << 14; // tCLHZ
		reg_val |= 0x0 << 12; // tCS
		reg_val |= 0x0 << 11; // tCDQSS
		reg_val |= 0x4 << 8;  // tCAD
		reg_val |= 0x0 << 6;  // tRHW
		reg_val |= 0x0 << 4;  // tWHR
		reg_val |= 0x2 << 2;  // tADL
		reg_val |= 0x0;       //<<0;  //tWB
	} else {
		// fault
		PHY_ERR("wrong interface , 0x%x\n", sync_mode);
	}

	// NDFC_WRITE_REG_TIMING_CFG(reg_val);
	*(nci->nctri->nreg.reg_timing_cfg) = reg_val;

	return;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : NULL
*Note         :
*****************************************************************************/
s32 init_nctri(struct _nand_controller_info *nctri)
{
	s32 ret = 0;
	u32 cfg = 0;

	nctri->type = NAND_GetNdfcVersion();
	nctri->dma_type = NAND_GetNdfcDmaMode();

	nctri->write_wait_rb_before_cmd_io =
	    g_phy_cfg->phy_wait_rb_before; // 1: before send cmd io; 0: after
					   // send cmd io;
	nctri->write_wait_rb_mode =
	    g_phy_cfg->phy_wait_rb_mode; // 0: query mode; 1: interrupt mode
	nctri->write_wait_dma_mode = g_phy_cfg->phy_wait_dma_mode;
	nctri->rb_ready_flag = 1;
	nctri->dma_ready_flag = 0;

	if (nctri->type == NDFC_VERSION_V1)
		nctri->max_ecc_level = 14;
	else if (nctri->type == NDFC_VERSION_V2)
		nctri->max_ecc_level = 9;
	else {
		PHY_ERR("init nctri, request dma fail!\n");
		ret = ERR_NO_26;
		goto ERR;
	}

	if (NAND_GetVoltage())
		return -1;

	if (nctri->dma_type == DMA_MODE_GENERAL_DMA) {
		if (nand_request_dma() != 0) {
			PHY_ERR("init nctri, request dma fail!\n");
			ret = ERR_NO_25;
			goto ERR;
		}
	}

	if (NAND_PIORequest(nctri->channel_id) != 0) {
		PHY_ERR("init nctri NAND PIORequest error!\n");
		ret = ERR_NO_24;
		goto ERR;
	}

	if (NAND_ClkRequest(nctri->channel_id) != 0) {
		PHY_ERR("init nctri NAND_ClkRequest error!\n");
		ret = ERR_NO_23;
		goto ERR;
	}

	NAND_SetClk(nctri->channel_id, 20, 20 * 2);

	// soft reset ndfc
	ndfc_soft_reset(nctri);

	// set NFC_REG_CTL
	cfg = 0;
	cfg |= ((0x0 & 0x1) << 6);  // ce_ctl
	cfg |= ((0x0 & 0x1) << 7);  // ce_ctl1
	cfg |= ((0x1 & 0xf) << 8);  // page size, 2KB
	cfg |= ((0x0 & 0x1) << 31); // debug
	*nctri->nreg.reg_ctl = cfg;

	// enable nand flash controller
	*nctri->nreg.reg_glb_cfg |= NDFC_EN;

	// set NFC_SPARE_AREA
	*nctri->nreg.reg_spare_area = 2048; // 2KB

	// disable write protect
	*nctri->nreg.reg_efr = 0x100;

	// disable randomize
	ndfc_disable_randomize(nctri);

	// init nand flash interface
	ndfc_change_nand_interface(nctri, 0, 1, 0, 0);

	return ret;

ERR:
	return ret;
}
