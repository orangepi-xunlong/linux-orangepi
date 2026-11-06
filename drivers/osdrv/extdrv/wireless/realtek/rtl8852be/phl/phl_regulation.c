/******************************************************************************
 *
 * Copyright(c) 2020 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/
#include "phl_headers.h"

/* regu tbl definition starts here */
#define REGULATION_CHPLAN_VERSION 64
#define REGULATION_COUNTRY_VERSION 41

static const struct chdef_2ghz chdef2g[] = {
    {0, {0x00, 0x00}, {0x00, 0x00}},
    {1, {0xff, 0x1f}, {0x00, 0x18}},
    {2, {0xff, 0x1f}, {0x00, 0x00}},
    {3, {0xff, 0x07}, {0x00, 0x00}},
    {4, {0xff, 0x3f}, {0x00, 0x00}},
    {5, {0x00, 0x1e}, {0x00, 0x00}},
    {6, {0xff, 0x3f}, {0x00, 0x38}},
};

static const struct chdef_5ghz chdef5g[] = {
    {0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, {0x00, 0x00}, {0x00, 0x00}, {0x00, 0x00}, 0x00, 0x00, 0x00},
    {1, 0x0f, 0x00, 0x00, 0x0f, 0x00, 0x0f, {0x1f, 0x07}, {0x00, 0x00}, {0x1f, 0x07}, 0x1f, 0x00, 0x00},
    {2, 0x0f, 0x00, 0x00, 0x0f, 0x00, 0x0f, {0xff, 0x07}, {0x00, 0x00}, {0xff, 0x07}, 0x00, 0x00, 0x00},
    {3, 0x0f, 0x00, 0x00, 0x0f, 0x00, 0x0f, {0xff, 0x07}, {0x00, 0x00}, {0xff, 0x07}, 0x1f, 0x00, 0x00},
    {4, 0x0f, 0x00, 0x00, 0x0f, 0x00, 0x0f, {0xff, 0x01}, {0x00, 0x00}, {0xff, 0x01}, 0x1f, 0x00, 0x00},
    {5, 0x0f, 0x00, 0x00, 0x0f, 0x00, 0x0f, {0x7f, 0x00}, {0x00, 0x00}, {0x7f, 0x00}, 0x0f, 0x00, 0x00},
    {6, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, {0x00, 0x00}, {0x00, 0x00}, {0x00, 0x00}, 0x1f, 0x00, 0x00},
    {7, 0x0f, 0x00, 0x00, 0x0f, 0x00, 0x0f, {0x00, 0x00}, {0x00, 0x00}, {0x00, 0x00}, 0x1f, 0x00, 0x00},
    {8, 0x0f, 0x00, 0x00, 0x0f, 0x00, 0x0f, {0x00, 0x00}, {0x00, 0x00}, {0x00, 0x00}, 0x0f, 0x00, 0x00},
    {9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, {0x00, 0x00}, {0x00, 0x00}, {0x00, 0x00}, 0x1f, 0x00, 0x00},
    {10, 0x0f, 0x00, 0x00, 0x0f, 0x00, 0x0f, {0x00, 0x00}, {0x00, 0x00}, {0x00, 0x00}, 0x00, 0x00, 0x00},
    {11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, {0xff, 0x07}, {0x00, 0x00}, {0xff, 0x07}, 0x00, 0x00, 0x00},
    {12, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x0e, {0x1f, 0x07}, {0x00, 0x00}, {0x1f, 0x07}, 0x1f, 0x00, 0x00},
    {13, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x0e, {0x00, 0x00}, {0x00, 0x00}, {0x00, 0x00}, 0x1f, 0x00, 0x00},
    {14, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, {0x00, 0x00}, {0x00, 0x00}, {0x00, 0x00}, 0x00, 0x00, 0x00},
    {15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, {0x00, 0x00}, {0x00, 0x00}, {0x00, 0x00}, 0x0f, 0x00, 0x00},
    {16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, {0xff, 0x07}, {0x00, 0x00}, {0x00, 0x00}, 0x00, 0x00, 0x00},
    {17, 0x0f, 0x00, 0x00, 0x0f, 0x00, 0x0f, {0x00, 0x07}, {0x00, 0x00}, {0x00, 0x07}, 0x1f, 0x00, 0x00},
    {18, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x0f, {0x1f, 0x07}, {0x00, 0x00}, {0x1f, 0x07}, 0x1f, 0x00, 0x00},
    {19, 0x0f, 0x00, 0x00, 0x0f, 0x00, 0x0f, {0x1f, 0x07}, {0x00, 0x00}, {0x1f, 0x07}, 0x00, 0x00, 0x00},
    {20, 0x0f, 0x00, 0x00, 0x0f, 0x00, 0x0f, {0x1f, 0x07}, {0x00, 0x00}, {0x1f, 0x07}, 0x0f, 0x00, 0x00},
    {21, 0x0f, 0x00, 0x00, 0x0f, 0x00, 0x0f, {0x00, 0x07}, {0x00, 0x00}, {0x00, 0x07}, 0x00, 0x00, 0x00},
    {22, 0x0f, 0x00, 0x00, 0x0f, 0x00, 0x0f, {0xff, 0x0f}, {0x00, 0x00}, {0xff, 0x0f}, 0x1f, 0x00, 0x00},
    {23, 0x0f, 0x00, 0x00, 0x0f, 0x00, 0x0f, {0xff, 0x00}, {0x00, 0x00}, {0xff, 0x00}, 0x1f, 0x00, 0x00},
    {24, 0x0f, 0x00, 0x00, 0x0f, 0x0f, 0x00, {0xff, 0x07}, {0xff, 0x07}, {0x00, 0x00}, 0x1f, 0x1f, 0x00},
    {25, 0x0f, 0x00, 0x00, 0x0f, 0x0f, 0x00, {0xff, 0x07}, {0xff, 0x07}, {0x00, 0x00}, 0x1f, 0x00, 0x00},
    {26, 0x0f, 0x0f, 0x00, 0x0f, 0x0f, 0x00, {0xff, 0x07}, {0xff, 0x07}, {0x00, 0x00}, 0x1f, 0x00, 0x00},
    {27, 0x0f, 0x00, 0x00, 0x0f, 0x0f, 0x00, {0x1f, 0x07}, {0x1f, 0x07}, {0x00, 0x00}, 0x1f, 0x00, 0x00},
    {28, 0x0f, 0x00, 0x00, 0x0f, 0x0f, 0x00, {0x00, 0x00}, {0x00, 0x00}, {0x00, 0x00}, 0x1f, 0x00, 0x00},
    {29, 0x0f, 0x0f, 0x00, 0x0f, 0x0f, 0x00, {0x00, 0x00}, {0x00, 0x00}, {0x00, 0x00}, 0x1f, 0x00, 0x00},
    {30, 0x0f, 0x0f, 0x00, 0x00, 0x00, 0x00, {0x00, 0x00}, {0x00, 0x00}, {0x00, 0x00}, 0x1f, 0x1f, 0x00},
    {31, 0x0f, 0x0f, 0x00, 0x0f, 0x0f, 0x00, {0xff, 0x07}, {0xff, 0x07}, {0x00, 0x00}, 0x1f, 0x1f, 0x00},
    {32, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x0f, {0x00, 0x00}, {0x00, 0x00}, {0x00, 0x00}, 0x1f, 0x00, 0x00},
    {33, 0x0f, 0x00, 0x00, 0x0f, 0x00, 0x0f, {0x1f, 0x0f}, {0x00, 0x00}, {0x1f, 0x0f}, 0x1f, 0x00, 0x00},
    {34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, {0x1f, 0x07}, {0x00, 0x00}, {0x1f, 0x07}, 0x1f, 0x00, 0x00},
    {35, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, {0x1f, 0x07}, {0x00, 0x00}, {0x1f, 0x07}, 0x00, 0x00, 0x00},
    {36, 0x0f, 0x0f, 0x00, 0x0f, 0x0f, 0x0f, {0xff, 0x0f}, {0xff, 0x0f}, {0xff, 0x0f}, 0x1f, 0x1f, 0x00},
    {37, 0x0f, 0x0f, 0x00, 0x0f, 0x0f, 0x00, {0x00, 0x00}, {0x00, 0x00}, {0x00, 0x00}, 0x00, 0x00, 0x00},
    {38, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x0f, {0xff, 0x0f}, {0x00, 0x00}, {0xff, 0x0f}, 0x00, 0x00, 0x00},
    {39, 0x0f, 0x0f, 0x00, 0x0f, 0x00, 0x0f, {0xff, 0x00}, {0x00, 0x00}, {0xff, 0x00}, 0x1f, 0x00, 0x1f},
    {40, 0x0f, 0x0f, 0x00, 0x0f, 0x00, 0x0f, {0x1f, 0x07}, {0x00, 0x00}, {0x1f, 0x07}, 0x1f, 0x00, 0x00},
    {41, 0x0f, 0x00, 0x00, 0x0f, 0x00, 0x0f, {0xff, 0x07}, {0x00, 0x00}, {0xff, 0x07}, 0x1f, 0x1f, 0x00},
    {42, 0x0f, 0x0f, 0x00, 0x0f, 0x00, 0x0f, {0xff, 0x07}, {0x00, 0x00}, {0xff, 0x07}, 0x1f, 0x1f, 0x00},
    {43, 0x0f, 0x0f, 0x00, 0x0f, 0x0f, 0x00, {0xff, 0x06}, {0xff, 0x06}, {0x00, 0x00}, 0x1f, 0x1f, 0x00},
    {44, 0x0f, 0x0f, 0x00, 0x0f, 0x0f, 0x00, {0x1f, 0x07}, {0x1f, 0x07}, {0x00, 0x00}, 0x1f, 0x1f, 0x00},
    {45, 0x0f, 0x0f, 0x00, 0x0f, 0x0f, 0x00, {0x00, 0x00}, {0x00, 0x00}, {0x00, 0x00}, 0x1f, 0x1f, 0x00},
    {46, 0x0f, 0x00, 0x00, 0x0f, 0x0f, 0x00, {0x00, 0x00}, {0x00, 0x00}, {0x00, 0x00}, 0x0f, 0x00, 0x00},
    {47, 0x0f, 0x00, 0x00, 0x0f, 0x0f, 0x00, {0xff, 0x07}, {0xff, 0x07}, {0x00, 0x00}, 0x00, 0x00, 0x00},
    {48, 0x0f, 0x00, 0x00, 0x0f, 0x00, 0x0f, {0xff, 0x0f}, {0x00, 0x00}, {0xff, 0x0f}, 0x00, 0x00, 0x00},
    {49, 0x0f, 0x00, 0x00, 0x0f, 0x00, 0x0f, {0xff, 0x01}, {0x00, 0x00}, {0xff, 0x01}, 0x00, 0x00, 0x00},
    {50, 0x0f, 0x00, 0x00, 0x0f, 0x00, 0x0f, {0x00, 0x0f}, {0x00, 0x00}, {0x00, 0x0f}, 0x1f, 0x00, 0x00},
    {51, 0x0f, 0x00, 0x00, 0x0f, 0x00, 0x0f, {0x1f, 0x00}, {0x00, 0x00}, {0x1f, 0x00}, 0x00, 0x00, 0x00},
    {52, 0x0f, 0x00, 0x00, 0x0f, 0x00, 0x0f, {0xff, 0x0f}, {0x00, 0x00}, {0xff, 0x0f}, 0xff, 0x00, 0x00},
    {53, 0x0f, 0x00, 0x00, 0x0f, 0x00, 0x0f, {0x0f, 0x00}, {0x00, 0x00}, {0x0f, 0x00}, 0x1f, 0x00, 0x00},
    {54, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, {0x00, 0x00}, {0x00, 0x00}, {0x00, 0x00}, 0x0f, 0x00, 0x00},
    {55, 0x0f, 0x0f, 0x00, 0x0f, 0x0f, 0x0f, {0xff, 0x0f}, {0xff, 0x0f}, {0xff, 0x0f}, 0xff, 0xff, 0x00},
    {57, 0x0f, 0x0f, 0x00, 0x0f, 0x0f, 0x00, {0xff, 0x0f}, {0xff, 0x0f}, {0x00, 0x00}, 0x1f, 0x1f, 0x00},
    {58, 0x0f, 0x0f, 0x00, 0x0f, 0x0f, 0x00, {0x00, 0x07}, {0x00, 0x07}, {0x00, 0x00}, 0x1f, 0x1f, 0x00},
};


static const struct regulatory_domain_mapping rdmap[] = {
    {0x00, {REGULATION_ETSI, 2}, {REGULATION_ETSI, 49}},
    {0x01, {REGULATION_ETSI, 2}, {REGULATION_ETSI, 50}},
    {0x02, {REGULATION_ETSI, 3}, {REGULATION_ETSI, 7}},
    {0x03, {REGULATION_ACMA, 2}, {REGULATION_ACMA, 33}},
    {0x04, {REGULATION_ETSI, 2}, {REGULATION_ETSI, 51}},
    {0x05, {REGULATION_ETSI, 2}, {REGULATION_ETSI, 6}},
    {0x06, {REGULATION_ETSI, 2}, {REGULATION_ETSI, 7}},
    {0x07, {REGULATION_ETSI, 2}, {REGULATION_ETSI, 23}},
    {0x08, {REGULATION_ETSI, 2}, {REGULATION_ETSI, 21}},
    {0x09, {REGULATION_ETSI, 2}, {REGULATION_ETSI, 17}},
    {0x0a, {REGULATION_NA, 0}, {REGULATION_NA, 0}},
    {0x0b, {REGULATION_ETSI, 2}, {REGULATION_ETSI, 22}},
    {0x0c, {REGULATION_FCC, 3}, {REGULATION_FCC, 54}},
    {0x0d, {REGULATION_MKK, 4}, {REGULATION_MKK, 14}},
    {0x0e, {REGULATION_ETSI, 1}, {REGULATION_ETSI, 57}},
    {0x0f, {REGULATION_ETSI, 1}, {REGULATION_ETSI, 58}},
    {0x1b, {REGULATION_FCC, 2}, {REGULATION_FCC, 52}},
    {0x1c, {REGULATION_KCC, 2}, {REGULATION_KCC, 53}},
    {0x20, {REGULATION_WW, 1}, {REGULATION_NA, 0}},
    {0x21, {REGULATION_ETSI, 2}, {REGULATION_NA, 0}},
    {0x22, {REGULATION_FCC, 3}, {REGULATION_NA, 0}},
    {0x23, {REGULATION_MKK, 4}, {REGULATION_NA, 0}},
    {0x24, {REGULATION_ETSI, 5}, {REGULATION_NA, 0}},
    {0x25, {REGULATION_FCC, 3}, {REGULATION_FCC, 3}},
    {0x26, {REGULATION_ETSI, 1}, {REGULATION_ETSI, 2}},
    {0x27, {REGULATION_MKK, 4}, {REGULATION_MKK, 2}},
    {0x28, {REGULATION_KCC, 1}, {REGULATION_KCC, 5}},
    {0x29, {REGULATION_FCC, 1}, {REGULATION_FCC, 6}},
    {0x2a, {REGULATION_FCC, 2}, {REGULATION_NA, 0}},
    {0x2b, {REGULATION_IC, 2}, {REGULATION_IC, 33}},
    {0x2c, {REGULATION_MKK, 2}, {REGULATION_NA, 0}},
    {0x2d, {REGULATION_CHILE, 1}, {REGULATION_CHILE, 22}},
    {0x2e, {REGULATION_WW, 3}, {REGULATION_WW, 37}},
    {0x2f, {REGULATION_CHILE, 1}, {REGULATION_CHILE, 38}},
    {0x30, {REGULATION_FCC, 1}, {REGULATION_FCC, 7}},
    {0x31, {REGULATION_FCC, 1}, {REGULATION_FCC, 8}},
    {0x32, {REGULATION_FCC, 1}, {REGULATION_FCC, 9}},
    {0x33, {REGULATION_FCC, 1}, {REGULATION_FCC, 10}},
    {0x34, {REGULATION_FCC, 3}, {REGULATION_FCC, 1}},
    {0x35, {REGULATION_ETSI, 1}, {REGULATION_ETSI, 3}},
    {0x36, {REGULATION_ETSI, 1}, {REGULATION_ETSI, 4}},
    {0x37, {REGULATION_MKK, 4}, {REGULATION_MKK, 10}},
    {0x38, {REGULATION_MKK, 4}, {REGULATION_MKK, 11}},
    {0x39, {REGULATION_NCC, 3}, {REGULATION_NCC, 12}},
    {0x3a, {REGULATION_ETSI, 2}, {REGULATION_ETSI, 2}},
    {0x3b, {REGULATION_ACMA, 2}, {REGULATION_ACMA, 1}},
    {0x3c, {REGULATION_ETSI, 2}, {REGULATION_ETSI, 10}},
    {0x3d, {REGULATION_ETSI, 2}, {REGULATION_ETSI, 15}},
    {0x3e, {REGULATION_KCC, 2}, {REGULATION_KCC, 3}},
    {0x3f, {REGULATION_FCC, 3}, {REGULATION_FCC, 22}},
    {0x40, {REGULATION_NCC, 3}, {REGULATION_NCC, 13}},
    {0x41, {REGULATION_WW, 6}, {REGULATION_NA, 0}},
    {0x42, {REGULATION_ETSI, 2}, {REGULATION_ETSI, 14}},
    {0x43, {REGULATION_FCC, 3}, {REGULATION_FCC, 6}},
    {0x44, {REGULATION_NCC, 3}, {REGULATION_NCC, 9}},
    {0x45, {REGULATION_ACMA, 1}, {REGULATION_ACMA, 1}},
    {0x46, {REGULATION_FCC, 3}, {REGULATION_FCC, 15}},
    {0x47, {REGULATION_ETSI, 1}, {REGULATION_ETSI, 10}},
    {0x48, {REGULATION_ETSI, 1}, {REGULATION_ETSI, 7}},
    {0x49, {REGULATION_ETSI, 1}, {REGULATION_ETSI, 6}},
    {0x4a, {REGULATION_IC, 3}, {REGULATION_IC, 33}},
    {0x4b, {REGULATION_KCC, 2}, {REGULATION_KCC, 22}},
    {0x4c, {REGULATION_FCC, 3}, {REGULATION_FCC, 28}},
    {0x4d, {REGULATION_MEX, 2}, {REGULATION_MEX, 1}},
    {0x4e, {REGULATION_ETSI, 2}, {REGULATION_ETSI, 42}},
    {0x4f, {REGULATION_NA, 0}, {REGULATION_MKK, 43}},
    {0x50, {REGULATION_ETSI, 1}, {REGULATION_ETSI, 16}},
    {0x51, {REGULATION_ETSI, 1}, {REGULATION_ETSI, 9}},
    {0x52, {REGULATION_ETSI, 1}, {REGULATION_ETSI, 17}},
    {0x53, {REGULATION_NCC, 3}, {REGULATION_NCC, 18}},
    {0x54, {REGULATION_ETSI, 1}, {REGULATION_ETSI, 15}},
    {0x55, {REGULATION_FCC, 3}, {REGULATION_FCC, 1}},
    {0x56, {REGULATION_ETSI, 1}, {REGULATION_ETSI, 19}},
    {0x57, {REGULATION_FCC, 3}, {REGULATION_FCC, 20}},
    {0x58, {REGULATION_MKK, 2}, {REGULATION_MKK, 14}},
    {0x59, {REGULATION_ETSI, 1}, {REGULATION_ETSI, 21}},
    {0x5a, {REGULATION_NA, 0}, {REGULATION_FCC, 44}},
    {0x5b, {REGULATION_NA, 0}, {REGULATION_FCC, 45}},
    {0x5c, {REGULATION_NA, 0}, {REGULATION_FCC, 43}},
    {0x5d, {REGULATION_ETSI, 2}, {REGULATION_ETSI, 8}},
    {0x5e, {REGULATION_ETSI, 2}, {REGULATION_ETSI, 3}},
    {0x5f, {REGULATION_MKK, 2}, {REGULATION_MKK, 47}},
    {0x60, {REGULATION_FCC, 3}, {REGULATION_FCC, 9}},
    {0x61, {REGULATION_FCC, 2}, {REGULATION_FCC, 1}},
    {0x62, {REGULATION_FCC, 2}, {REGULATION_FCC, 3}},
    {0x63, {REGULATION_ETSI, 1}, {REGULATION_ETSI, 23}},
    {0x64, {REGULATION_MKK, 2}, {REGULATION_MKK, 24}},
    {0x65, {REGULATION_ETSI, 2}, {REGULATION_ETSI, 24}},
    {0x66, {REGULATION_FCC, 3}, {REGULATION_FCC, 27}},
    {0x67, {REGULATION_FCC, 3}, {REGULATION_FCC, 25}},
    {0x68, {REGULATION_FCC, 2}, {REGULATION_FCC, 27}},
    {0x69, {REGULATION_FCC, 2}, {REGULATION_FCC, 25}},
    {0x6a, {REGULATION_ETSI, 2}, {REGULATION_ETSI, 25}},
    {0x6b, {REGULATION_FCC, 1}, {REGULATION_FCC, 29}},
    {0x6c, {REGULATION_FCC, 1}, {REGULATION_FCC, 26}},
    {0x6d, {REGULATION_FCC, 2}, {REGULATION_FCC, 28}},
    {0x6e, {REGULATION_FCC, 1}, {REGULATION_FCC, 25}},
    {0x6f, {REGULATION_NA, 0}, {REGULATION_ETSI, 6}},
    {0x70, {REGULATION_NA, 0}, {REGULATION_ETSI, 30}},
    {0x71, {REGULATION_NA, 0}, {REGULATION_ETSI, 25}},
    {0x72, {REGULATION_NA, 0}, {REGULATION_ETSI, 31}},
    {0x73, {REGULATION_FCC, 1}, {REGULATION_FCC, 1}},
    {0x74, {REGULATION_FCC, 2}, {REGULATION_FCC, 19}},
    {0x75, {REGULATION_ETSI, 1}, {REGULATION_ETSI, 32}},
    {0x76, {REGULATION_FCC, 2}, {REGULATION_FCC, 22}},
    {0x77, {REGULATION_ETSI, 1}, {REGULATION_ETSI, 34}},
    {0x78, {REGULATION_FCC, 3}, {REGULATION_FCC, 35}},
    {0x79, {REGULATION_MKK, 2}, {REGULATION_MKK, 2}},
    {0x7a, {REGULATION_ETSI, 2}, {REGULATION_ETSI, 28}},
    {0x7b, {REGULATION_ETSI, 2}, {REGULATION_ETSI, 46}},
    {0x7c, {REGULATION_ETSI, 2}, {REGULATION_ETSI, 47}},
    {0x7d, {REGULATION_MKK, 4}, {REGULATION_MKK, 48}},
    {0x7e, {REGULATION_MKK, 2}, {REGULATION_MKK, 48}},
    {0x7f, {REGULATION_WW, 1}, {REGULATION_WW, 55}},
};

const struct chdef_6ghz chdef6g[] = {
    {0, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, 0x00, 0x00, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00}, {0x00, 0x00}},
    {1, {0xff, 0xff, 0xff}, {0x00, 0x00, 0x00}, 0x00, 0x00, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00}, {0x00, 0x00}}, 
    {2, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, 0x3f, 0x00, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0x00, 0x00}, {0x00, 0x00}}, 
    {3, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, 0x00, 0x00, {0xff, 0xff, 0x03}, {0x00, 0x00, 0x00}, {0x00, 0x00}, {0x00, 0x00}}, 
    {4, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, 0x00, 0x00, {0x00, 0x00, 0x00}, {0x00, 0x00, 0x00}, {0xff, 0x07}, {0x00, 0x00}}, 
    {5, {0xff, 0xff, 0xff}, {0xff, 0xff, 0xff}, 0x3f, 0x3f, {0xff, 0xff, 0x03}, {0xff, 0xff, 0x03}, {0xff, 0x07}, {0xff, 0x07}}, 
    {6, {0xff, 0xff, 0xff}, {0x00, 0x00, 0x00}, 0x3f, 0x00, {0xff, 0xff, 0x03}, {0x00, 0x00, 0x00}, {0xff, 0x07}, {0x00, 0x00}}, 
};

const struct regulatory_domain_mapping_6g rdmap6[] = {
    {0x00, REGULATION_NA, 0},
    {0x01, REGULATION_FCC, 1},
    {0x02, REGULATION_FCC, 2},
    {0x03, REGULATION_FCC, 3},
    {0x04, REGULATION_FCC, 4},
    {0x05, REGULATION_FCC, 6},
    {0x06, REGULATION_ETSI, 1},
    {0x07, REGULATION_IC, 6},
    {0x08, REGULATION_KCC, 6},
    {0x09, REGULATION_KCC, 1},
    {0x1b, REGULATION_ACMA, 1},
    {0x1c, REGULATION_MKK, 1},
    {0x7f, REGULATION_WW, 5},
};

const struct country_domain_mapping cdmap[] = {
    {0x4d, 0x05, {'A', 'R'}, TPO_NA, 0x0f, 0, 0x01, 1}, /* 0000 */
    {0x61, 0x05, {'B', 'O'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0001 */
    {0x62, 0x05, {'B', 'R'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0002 */
    {0x76, 0x01, {'C', 'L'}, TPO_CHILE, 0x0f, 0, 0x01, 0}, /* 0003 */
    {0x76, 0x05, {'C', 'O'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0004 */
    {0x76, 0x05, {'C', 'R'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0005 */
    {0x76, 0x00, {'E', 'C'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0006 */
    {0x76, 0x05, {'S', 'V'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0007 */
    {0x76, 0x05, {'G', 'T'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0008 */
    {0x76, 0x05, {'H', 'N'}, TPO_NA, 0x0f, 0, 0x01, 1}, /* 0009 */
    {0x4d, 0x01, {'M', 'X'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0010 */
    {0x76, 0x00, {'N', 'I'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0011 */
    {0x76, 0x00, {'P', 'A'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0012 */
    {0x76, 0x00, {'P', 'Y'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0013 */
    {0x76, 0x05, {'P', 'E'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0014 */
    {0x1b, 0x05, {'U', 'S'}, TPO_NA, 0x0f, 0, 0x03, 0}, /* 0015 */
    {0x30, 0x00, {'U', 'Y'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0016 */
    {0x30, 0x00, {'V', 'E'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0017 */
    {0x76, 0x00, {'P', 'R'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0018 */
    {0x76, 0x00, {'D', 'O'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0019 */
    {0x5e, 0x06, {'A', 'T'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 0}, /* 0020 */
    {0x5e, 0x06, {'B', 'E'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 0}, /* 0021 */
    {0x5e, 0x06, {'C', 'Y'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 0}, /* 0022 */
    {0x5e, 0x06, {'C', 'Z'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 0}, /* 0023 */
    {0x5e, 0x06, {'D', 'K'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 0}, /* 0024 */
    {0x5e, 0x06, {'E', 'E'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 0}, /* 0025 */
    {0x5e, 0x06, {'F', 'I'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 0}, /* 0026 */
    {0x5e, 0x06, {'F', 'R'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 0}, /* 0027 */
    {0x5e, 0x06, {'D', 'E'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 0}, /* 0028 */
    {0x5e, 0x06, {'G', 'R'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 0}, /* 0029 */
    {0x5e, 0x06, {'H', 'U'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 0}, /* 0030 */
    {0x5e, 0x06, {'I', 'S'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 0}, /* 0031 */
    {0x5e, 0x06, {'I', 'E'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 0}, /* 0032 */
    {0x5e, 0x06, {'I', 'T'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 0}, /* 0033 */
    {0x5e, 0x06, {'L', 'V'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 0}, /* 0034 */
    {0x5e, 0x06, {'L', 'I'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 0}, /* 0035 */
    {0x5e, 0x06, {'L', 'T'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 0}, /* 0036 */
    {0x5e, 0x06, {'L', 'U'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 0}, /* 0037 */
    {0x5e, 0x06, {'M', 'T'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 0}, /* 0038 */
    {0x5e, 0x06, {'M', 'C'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 0}, /* 0039 */
    {0x5e, 0x06, {'N', 'L'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 0}, /* 0040 */
    {0x5e, 0x06, {'N', 'O'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 0}, /* 0041 */
    {0x5e, 0x06, {'P', 'L'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 0}, /* 0042 */
    {0x5e, 0x06, {'P', 'T'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 0}, /* 0043 */
    {0x5e, 0x06, {'S', 'K'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 0}, /* 0044 */
    {0x5e, 0x06, {'S', 'I'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 0}, /* 0045 */
    {0x5e, 0x06, {'E', 'S'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 0}, /* 0046 */
    {0x5e, 0x06, {'S', 'E'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 0}, /* 0047 */
    {0x5e, 0x06, {'C', 'H'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 0}, /* 0048 */
    {0x0b, 0x06, {'G', 'B'}, TPO_UK, 0x0f, 0, 0x05, 0}, /* 0049 */
    {0x5e, 0x00, {'A', 'L'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0050 */
    {0x5e, 0x06, {'A', 'Z'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0051 */
    {0x06, 0x06, {'B', 'H'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0052 */
    {0x5e, 0x00, {'B', 'A'}, TPO_NA, 0x0f, 0, 0x01, 1}, /* 0053 */
    {0x5e, 0x06, {'B', 'G'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 1}, /* 0054 */
    {0x5e, 0x06, {'H', 'R'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 1}, /* 0055 */
    {0x3c, 0x00, {'E', 'G'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0056 */
    {0x5e, 0x06, {'G', 'H'}, TPO_NA, 0x0f, 0, 0x01, 1}, /* 0057 */
    {0x05, 0x00, {'I', 'Q'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0058 */
    {0x5e, 0x06, {'I', 'L'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0059 */
    {0x5e, 0x06, {'J', 'O'}, TPO_NA, 0x0f, 0, 0x05, 0}, /* 0060 */
    {0x5e, 0x00, {'K', 'Z'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0061 */
    {0x5e, 0x06, {'K', 'E'}, TPO_NA, 0x0f, 0, 0x01, 1}, /* 0062 */
    {0x5e, 0x06, {'K', 'W'}, TPO_NA, 0x0f, 0, 0x01, 1}, /* 0063 */
    {0x5e, 0x06, {'K', 'G'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0064 */
    {0x5e, 0x06, {'L', 'B'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0065 */
    {0x5e, 0x00, {'L', 'S'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0066 */
    {0x5e, 0x00, {'M', 'K'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0067 */
    {0x3c, 0x06, {'M', 'A'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0068 */
    {0x5e, 0x00, {'M', 'Z'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0069 */
    {0x5e, 0x00, {'N', 'A'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0070 */
    {0x75, 0x00, {'N', 'G'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0071 */
    {0x5e, 0x00, {'O', 'M'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0072 */
    {0x5e, 0x06, {'Q', 'A'}, TPO_QATAR, 0x0f, 0, 0x05, 0}, /* 0073 */
    {0x5e, 0x06, {'R', 'O'}, TPO_NA, 0x0f, CNTRY_EU, 0x05, 1}, /* 0074 */
    {0x09, 0x00, {'R', 'U'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0075 */
    {0x5e, 0x06, {'S', 'A'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0076 */
    {0x3a, 0x00, {'S', 'N'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0077 */
    {0x5e, 0x06, {'R', 'S'}, TPO_NA, 0x0f, 0, 0x01, 1}, /* 0078 */
    {0x3a, 0x00, {'M', 'E'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0079 */
    {0x5e, 0x06, {'Z', 'A'}, TPO_NA, 0x0f, 0, 0x01, 1}, /* 0080 */
    {0x5e, 0x06, {'T', 'R'}, TPO_NA, 0x0f, 0, 0x01, 1}, /* 0081 */
    {0x5e, 0x00, {'U', 'A'}, TPO_UKRAINE, 0x0f, 0, 0x00, 0}, /* 0082 */
    {0x5e, 0x06, {'A', 'E'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0083 */
    {0x3a, 0x00, {'Y', 'E'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0084 */
    {0x5e, 0x00, {'Z', 'W'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0085 */
    {0x5e, 0x00, {'B', 'D'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0086 */
    {0x5e, 0x00, {'K', 'H'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0087 */
    {0x06, 0x00, {'C', 'N'}, TPO_CN, 0x0f, 0, 0x00, 0}, /* 0088 */
    {0x5e, 0x06, {'H', 'K'}, TPO_NA, 0x0f, 0, 0x05, 1}, /* 0089 */
    {0x5e, 0x00, {'I', 'N'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0090 */
    {0x5d, 0x00, {'I', 'D'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0091 */
    {0x4b, 0x08, {'K', 'R'}, TPO_NA, 0x0f, 0, 0x05, 0}, /* 0092 */
    {0x07, 0x06, {'M', 'Y'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0093 */
    {0x5e, 0x00, {'P', 'K'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0094 */
    {0x5e, 0x00, {'P', 'H'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0095 */
    {0x5e, 0x00, {'S', 'G'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0096 */
    {0x5e, 0x00, {'L', 'K'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0097 */
    {0x76, 0x00, {'T', 'W'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0098 */
    {0x5e, 0x06, {'T', 'H'}, TPO_NA, 0x0f, 0, 0x05, 0}, /* 0099 */
    {0x5e, 0x00, {'V', 'N'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0100 */
    {0x03, 0x1b, {'A', 'U'}, TPO_NA, 0x0f, 0, 0x05, 0}, /* 0101 */
    {0x03, 0x1b, {'N', 'Z'}, TPO_NA, 0x0f, 0, 0x05, 0}, /* 0102 */
    {0x5e, 0x06, {'P', 'G'}, TPO_NA, 0x0f, 0, 0x01, 1}, /* 0103 */
    {0x2b, 0x07, {'C', 'A'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0104 */
    {0x7d, 0x1c, {'J', 'P'}, TPO_NA, 0x0f, 0, 0x05, 0}, /* 0105 */
    {0x76, 0x05, {'J', 'M'}, TPO_NA, 0x0f, 0, 0x01, 1}, /* 0106 */
    {0x76, 0x05, {'A', 'N'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0107 */
    {0x76, 0x00, {'T', 'T'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0108 */
    {0x04, 0x00, {'T', 'N'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0109 */
    {0x42, 0x00, {'A', 'F'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0110 */
    {0x00, 0x06, {'D', 'Z'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0111 */
    {0x76, 0x00, {'A', 'S'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0112 */
    {0x3a, 0x00, {'A', 'D'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0113 */
    {0x5e, 0x00, {'A', 'O'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0114 */
    {0x5e, 0x06, {'A', 'I'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0115 */
    {0x26, 0x00, {'A', 'Q'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0116 */
    {0x76, 0x05, {'A', 'G'}, TPO_NA, 0x0f, 0, 0x03, 0}, /* 0117 */
    {0x5e, 0x06, {'A', 'M'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0118 */
    {0x76, 0x05, {'A', 'W'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0119 */
    {0x76, 0x05, {'B', 'S'}, TPO_NA, 0x0f, 0, 0x01, 1}, /* 0120 */
    {0x76, 0x05, {'B', 'B'}, TPO_NA, 0x0f, 0, 0x01, 1}, /* 0121 */
    {0x08, 0x00, {'B', 'Y'}, TPO_NA, 0x07, 0, 0x00, 0}, /* 0122 */
    {0x76, 0x00, {'B', 'Z'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0123 */
    {0x3a, 0x00, {'B', 'J'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0124 */
    {0x76, 0x05, {'B', 'M'}, TPO_NA, 0x0f, 0, 0x01, 1}, /* 0125 */
    {0x5e, 0x00, {'B', 'T'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0126 */
    {0x5e, 0x06, {'B', 'W'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0127 */
    {0x5e, 0x00, {'B', 'V'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0128 */
    {0x3a, 0x00, {'I', 'O'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0129 */
    {0x76, 0x05, {'V', 'G'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0130 */
    {0x06, 0x00, {'B', 'N'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0131 */
    {0x5e, 0x06, {'B', 'F'}, TPO_NA, 0x0f, 0, 0x01, 1}, /* 0132 */
    {0x3a, 0x00, {'M', 'M'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0133 */
    {0x3a, 0x06, {'B', 'I'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0134 */
    {0x5e, 0x00, {'C', 'M'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0135 */
    {0x5e, 0x00, {'C', 'V'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0136 */
    {0x76, 0x05, {'K', 'Y'}, TPO_NA, 0x0f, 0, 0x01, 1}, /* 0137 */
    {0x3a, 0x00, {'C', 'F'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0138 */
    {0x3a, 0x06, {'T', 'D'}, TPO_NA, 0x0f, 0, 0x01, 1}, /* 0139 */
    {0x03, 0x00, {'C', 'X'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0140 */
    {0x03, 0x00, {'C', 'C'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0141 */
    {0x5e, 0x06, {'K', 'M'}, TPO_NA, 0x0f, 0, 0x01, 1}, /* 0142 */
    {0x5e, 0x00, {'C', 'G'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0143 */
    {0x5e, 0x00, {'C', 'D'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0144 */
    {0x5e, 0x00, {'C', 'K'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0145 */
    {0x42, 0x00, {'C', 'I'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0146 */
    {0x5e, 0x06, {'D', 'J'}, TPO_NA, 0x0f, 0, 0x01, 1}, /* 0147 */
    {0x76, 0x00, {'D', 'M'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0148 */
    {0x5e, 0x06, {'G', 'Q'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0149 */
    {0x3a, 0x00, {'E', 'R'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0150 */
    {0x3a, 0x00, {'E', 'T'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0151 */
    {0x5e, 0x00, {'F', 'K'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0152 */
    {0x5e, 0x00, {'F', 'O'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0153 */
    {0x76, 0x00, {'F', 'J'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0154 */
    {0x3a, 0x00, {'G', 'F'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0155 */
    {0x3a, 0x00, {'P', 'F'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0156 */
    {0x3a, 0x00, {'T', 'F'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0157 */
    {0x5e, 0x00, {'G', 'A'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0158 */
    {0x5e, 0x06, {'G', 'M'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0159 */
    {0x5e, 0x00, {'G', 'E'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0160 */
    {0x5e, 0x00, {'G', 'I'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0161 */
    {0x5e, 0x00, {'G', 'L'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0162 */
    {0x76, 0x00, {'G', 'D'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0163 */
    {0x5e, 0x00, {'G', 'P'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0164 */
    {0x76, 0x00, {'G', 'U'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0165 */
    {0x5e, 0x00, {'G', 'G'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0166 */
    {0x5e, 0x06, {'G', 'N'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0167 */
    {0x5e, 0x00, {'G', 'W'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0168 */
    {0x76, 0x00, {'G', 'Y'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0169 */
    {0x76, 0x01, {'H', 'T'}, TPO_NA, 0x07, 0, 0x01, 1}, /* 0170 */
    {0x03, 0x00, {'H', 'M'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0171 */
    {0x5e, 0x00, {'V', 'A'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0172 */
    {0x5e, 0x00, {'I', 'M'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0173 */
    {0x5e, 0x00, {'J', 'E'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0174 */
    {0x5e, 0x00, {'K', 'I'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0175 */
    {0x5e, 0x06, {'L', 'A'}, TPO_NA, 0x0f, 0, 0x01, 1}, /* 0176 */
    {0x5e, 0x00, {'L', 'R'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0177 */
    {0x5e, 0x00, {'L', 'Y'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0178 */
    {0x5e, 0x00, {'M', 'O'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0179 */
    {0x5e, 0x06, {'M', 'G'}, TPO_NA, 0x0f, 0, 0x01, 1}, /* 0180 */
    {0x5e, 0x00, {'M', 'W'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0181 */
    {0x3c, 0x00, {'M', 'V'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0182 */
    {0x5e, 0x00, {'M', 'L'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0183 */
    {0x76, 0x00, {'M', 'H'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0184 */
    {0x3a, 0x00, {'M', 'Q'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0185 */
    {0x5e, 0x00, {'M', 'R'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0186 */
    {0x5e, 0x06, {'M', 'U'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0187 */
    {0x5e, 0x00, {'Y', 'T'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0188 */
    {0x76, 0x00, {'F', 'M'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0189 */
    {0x5e, 0x06, {'M', 'D'}, TPO_NA, 0x0f, 0, 0x05, 0}, /* 0190 */
    {0x5e, 0x06, {'M', 'N'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0191 */
    {0x3a, 0x00, {'M', 'S'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0192 */
    {0x5e, 0x00, {'N', 'R'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0193 */
    {0x06, 0x00, {'N', 'P'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0194 */
    {0x3a, 0x00, {'N', 'C'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0195 */
    {0x5e, 0x00, {'N', 'E'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0196 */
    {0x03, 0x00, {'N', 'U'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0197 */
    {0x03, 0x00, {'N', 'F'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0198 */
    {0x76, 0x00, {'M', 'P'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0199 */
    {0x76, 0x00, {'P', 'W'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0200 */
    {0x5e, 0x00, {'R', 'E'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0201 */
    {0x5e, 0x00, {'R', 'W'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0202 */
    {0x5e, 0x00, {'S', 'H'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0203 */
    {0x76, 0x05, {'K', 'N'}, TPO_NA, 0x0f, 0, 0x01, 1}, /* 0204 */
    {0x76, 0x05, {'L', 'C'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0205 */
    {0x76, 0x00, {'M', 'F'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0206 */
    {0x76, 0x00, {'S', 'X'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0207 */
    {0x5e, 0x00, {'P', 'M'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0208 */
    {0x76, 0x00, {'V', 'C'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0209 */
    {0x76, 0x00, {'W', 'S'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0210 */
    {0x3a, 0x00, {'S', 'M'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0211 */
    {0x5e, 0x00, {'S', 'T'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0212 */
    {0x76, 0x00, {'S', 'C'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0213 */
    {0x5e, 0x06, {'S', 'L'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0214 */
    {0x3a, 0x00, {'S', 'B'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0215 */
    {0x3a, 0x00, {'S', 'O'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0216 */
    {0x3a, 0x00, {'G', 'S'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0217 */
    {0x74, 0x05, {'S', 'R'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0218 */
    {0x5e, 0x00, {'S', 'J'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0219 */
    {0x5e, 0x00, {'S', 'Z'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0220 */
    {0x5e, 0x06, {'T', 'J'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0221 */
    {0x5e, 0x00, {'T', 'Z'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0222 */
    {0x5e, 0x06, {'T', 'G'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0223 */
    {0x03, 0x00, {'T', 'K'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0224 */
    {0x3a, 0x00, {'T', 'O'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0225 */
    {0x3a, 0x00, {'T', 'M'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0226 */
    {0x3a, 0x00, {'T', 'C'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0227 */
    {0x21, 0x00, {'T', 'V'}, TPO_NA, 0x01, 0, 0x00, 0}, /* 0228 */
    {0x3a, 0x00, {'U', 'G'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0229 */
    {0x76, 0x00, {'V', 'I'}, TPO_NA, 0x0f, 0, 0x00, 1}, /* 0230 */
    {0x3a, 0x06, {'U', 'Z'}, TPO_NA, 0x0f, 0, 0x01, 0}, /* 0231 */
    {0x26, 0x00, {'V', 'U'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0232 */
    {0x3a, 0x00, {'W', 'F'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0233 */
    {0x3c, 0x00, {'E', 'H'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0234 */
    {0x5e, 0x00, {'Z', 'M'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0235 */
    {0x3a, 0x00, {'I', 'R'}, TPO_NA, 0x01, 0, 0x00, 0}, /* 0236 */
    {0x5e, 0x00, {'P', 'S'}, TPO_NA, 0x0f, 0, 0x00, 0}, /* 0237 */
};
/* regu tbl definition ends here */

#define _set_country(_a_, _id_, _c_) \
	(_a_)[(_id_)] = (_c_)[0]; \
	(_a_)[(_id_) + 1] = (_c_)[1];

u8 rtw_phl_get_domain_index(
	void *phl, u8 domain, bool is_6g, u8 tbl_idx)
{
	struct phl_info_t *phl_info = (struct phl_info_t *)phl;
	u8 did = INVALID_DOMAIN_INDEX;

	if (!phl_info)
		return did;

	if (tbl_idx == CMN_TBL) {
		if (is_6g)
			PHL_GET_DOMAIN_INDEX_6G(did, domain, rdmap6);
		else
			PHL_GET_DOMAIN_INDEX(did, domain, rdmap);
	} else if (tbl_idx == IC_TBL) {
		did = rtw_hal_get_domain_idx(phl_info->hal, domain, is_6g);
	}

	return did;
}

u8 rtw_phl_get_cntry_index(void *phl,
	char *cntry, u8 tbl_idx)
{
	struct phl_info_t *phl_info = (struct phl_info_t *)phl;
	u8 idx = INVALID_CNTRY_IDX;

	if (!phl_info)
		return idx;

	if (tbl_idx == CMN_TBL) {
		PHL_GET_CNTRY_INDEX(cntry, idx, cdmap);
	} else if (tbl_idx == IC_TBL) {
		idx = rtw_hal_get_cntry_idx(phl_info->hal, cntry);
	}

	return idx;
}

u8 rtw_phl_get_cntry_tbl_size(void *phl, u8 tbl_idx)
{
	struct phl_info_t *phl_info = (struct phl_info_t *)phl;
	u8 size = 0;

	if (!phl_info)
		return size;

	if (tbl_idx == CMN_TBL) {
		PHL_GET_CNTRY_TBL_SIZE(size, cdmap);
	} else if (tbl_idx == IC_TBL) {
		size = rtw_hal_get_cntry_tbl_size(phl_info->hal);
	}

	return size;
}

enum rtw_regulation_freq_group
rtw_phl_get_regu_freq_group(enum band_type band, u8 ch)
{
	if (BAND_2GHZ(band))
		return FREQ_GROUP_2GHZ;
	else if (BAND_5GHZ(band)) {
		if (CH_5GHZ_BAND1(ch))
			return FREQ_GROUP_5GHZ_BAND1;
		else if (CH_5GHZ_BAND2(ch))
			return FREQ_GROUP_5GHZ_BAND2;
		else if (CH_5GHZ_BAND3(ch))
			return FREQ_GROUP_5GHZ_BAND3;
		else if (CH_5GHZ_BAND4(ch))
			return FREQ_GROUP_5GHZ_BAND4;
	} else if (BAND_6GHZ(band)) {
		if (CH_6GHZ_UNII5(ch))
			return FREQ_GROUP_6GHZ_UNII5;
		else if (CH_6GHZ_UNII6(ch))
			return FREQ_GROUP_6GHZ_UNII6;
		else if (CH_6GHZ_UNII7(ch))
			return FREQ_GROUP_6GHZ_UNII7;
		else if (CH_6GHZ_UNII8(ch))
			return FREQ_GROUP_6GHZ_UNII8;
	}
	return FREQ_GROUP_MAX;
}

/*
 * @ Function description
 *	Check if domain is valid or not
 *
 * @ parameter
 *	domain : domain code to query
 *
 * @ return :
 *	true : if domain code exists in data base
 *	false : invalid domain code
 *
 */
bool rtw_phl_valid_regulation_domain(u8 domain)
{
	u8 did = INVALID_DOMAIN_INDEX;

	PHL_GET_DOMAIN_INDEX(did, domain, rdmap);

	return (did == INVALID_DOMAIN_INDEX ? false : true);
}

bool rtw_phl_valid_regulation_domain_6ghz(u8 domain)
{
	u8 did = INVALID_DOMAIN_INDEX;

	PHL_GET_DOMAIN_INDEX_6G(did, domain, rdmap6);

	return (did == INVALID_DOMAIN_INDEX ? false : true);
}

/*
 * @ Function description
 *	Query basic regulation info
 *
 * @ parameter
 * 	phl : struct phl_info_t *
 *	info : struct rtw_regulation_info *, query result will be filled here
 *
 * @ return :
 *	true : regulation query successfully, caller can check result
 *		by input parameter *info.
 *	false : regulation query fail
 *
 */
bool rtw_phl_query_regulation_info(
	void *phl, struct rtw_regulation_info *info)
{
	struct phl_info_t *phl_info = (struct phl_info_t *)phl;
	struct rtw_regulation_interface *rg_interface = NULL;
	void *d = NULL;

	if (!phl_info || !info)
		return false;

	rg_interface = &phl_info->rg_interface;
	d = phl_to_drvpriv(phl_info);

	_os_spinlock(d, &rg_interface->lock, _bh, NULL);
	_os_mem_cpy(d, info, &rg_interface->regu_info,
		sizeof(struct rtw_regulation_info));
	_os_spinunlock(d, &rg_interface->lock, _bh, NULL);

	return true;
}

bool rtw_phl_query_country_chplan(char *country,
	struct rtw_regulation_country_chplan *chplan)
{

	chplan->valid = false;

	PHL_QRY_CNTRY_CHNLPLAN(country, chplan, cdmap);

	return (chplan->valid == true? true : false);
}

void rtw_phl_query_country_chplan_ex(
	void *phl, char *country,
	struct rtw_regulation_country_chplan *chplan,
	u8 tbl_idx)
{
	struct phl_info_t *phl_info = (struct phl_info_t *)phl;
	chplan->valid = false;

	if (!phl_info)
		return;

	if (tbl_idx == CMN_TBL) {
		PHL_QRY_CNTRY_CHNLPLAN(country,
			chplan, cdmap);
	} else if (tbl_idx == IC_TBL) {
		rtw_hal_qry_cntry_chnlplan(
			phl_info->hal, chplan, country);
	}
}

void rtw_phl_fill_group_cntry_list(
	void *phl, struct rtw_regu_policy *policy,
	char *list, u32 group_size,
	u8 group_id, u8 tbl_idx)
{
	struct phl_info_t *phl_info = (struct phl_info_t *)phl;

	if (!phl_info || !policy)
		return;

	if (tbl_idx == CMN_TBL) {
		PHL_FILL_GRP_CNTRY_LIST(policy, list,
			group_size, group_id, rdmap, cdmap);
	} else if (tbl_idx == IC_TBL) {
		rtw_hal_fill_group_cntry_list(phl_info->hal,
			policy, list, group_size, group_id);
	}
}

void regu_fill_specific_group_cntry_list(
	struct rtw_regu_policy *policy, char *list, u32 group_size,
	u8 group_id, u8 rdmap_size, u8 cdmap_size,
	const struct regulatory_domain_mapping *rdmap_tbl,
	const struct country_domain_mapping *cdmap_tbl)
{
	u8 item = 0, item_num = 0;
	PHL_INFO("[REGU], get group cntry, group id = %d \n", group_id);

	if (!rdmap_tbl || !cdmap_tbl || !policy)
		return;

	switch (group_id) {
	case DEFAULT_SUPPORT_6G:
		for (item = 0; item < cdmap_size; item++) {
			if (cdmap_tbl[item].domain_code_6g != 0x00) {
				if (item_num < group_size) {
					_set_country(list, 2 * item_num,
						cdmap_tbl[item].char2);
					item_num++;
				} else {
					PHL_INFO("[REGU], buffer not enough \n");
					return;
				}
			}
		}
		break;
	case CURRENT_SUPPORT_6G:
		for (item = 0; item < cdmap_size; item++) {
			if (cdmap_tbl[item ].domain_code_6g != 0x00 &&
				!(policy->cp_6g_bp[item] & CP_6G_BAND_BLOCKED)) {
				if (item_num < group_size) {
					_set_country(list, 2 * item_num,
						cdmap_tbl[item].char2);
					item_num++;
				} else {
					PHL_INFO("[REGU], buffer not enough \n");
					return;
				}
			}
		}
		break;
	case EU_GROUP:
		for (item = 0; item < cdmap_size; item++) {
			if (cdmap_tbl[item].country_property == CNTRY_EU) {
				if (item_num < group_size) {
					_set_country(list, 2 * item_num,
						cdmap_tbl[item].char2);
					item_num++;
				} else {
					PHL_INFO("[REGU], buffer not enough \n");
					return;
				}
			}
		}
		break;
	case FCC_GROUP:
	{
		u8 domain = 0;
		u8 i = 0;
		u8 did = INVALID_DOMAIN_INDEX;
		for (item = 0; item < cdmap_size; item++) {
			domain = cdmap_tbl[item].domain_code;

			for (i = 0; i < rdmap_size; i++) {
				if (domain == rdmap_tbl[i].domain_code) {
					did = i;
					break;
				}
			}

			if (did != INVALID_DOMAIN_INDEX &&
				rdmap_tbl[did].freq_2g.regulation == REGULATION_FCC) {
				if (item_num < group_size) {
					_set_country(list, 2 * item_num,
						cdmap_tbl[item].char2);
					item_num++;
				} else {
					PHL_INFO("[REGU], buffer not enough \n");
					return;
				}
			}
		}
		break;
	}
	case OTHERS_MODULE_RF_APPROVAL:
		for (item = 0; item < cdmap_size; item++) {
			if (cdmap_tbl[item].others_module_rf_approval == 1) {
				if (item_num < group_size) {
					_set_country(list, 2 * item_num,
						cdmap_tbl[item].char2);
					item_num++;
				} else {
					PHL_INFO("[REGU], buffer not enough \n");
					return;
				}
			}
		}
		break;

	}
}

void
regu_fill_chnlplan_content(char *country, u8 size,
	struct rtw_regulation_country_chplan *chnlplan,
	const struct country_domain_mapping *cdmap_tbl)
{
	u8 i;

	if (!country || !chnlplan || !cdmap_tbl)
		return;

	chnlplan->valid = false;

	for (i = 0; i < size; i++) {
		if (cdmap_tbl[i].char2[0] == country[0] &&
			cdmap_tbl[i].char2[1] == country[1]) {
			chnlplan->domain_code = cdmap_tbl[i].domain_code;
			chnlplan->domain_code_6g = cdmap_tbl[i].domain_code_6g;
			if(cdmap_tbl[i].support & BIT(0))
				chnlplan->support_mode |=
				(SUPPORT_11B | SUPPORT_11G | SUPPORT_11N);
			if(cdmap_tbl[i].support & BIT(1))
				chnlplan->support_mode |= (SUPPORT_11A);
			if(cdmap_tbl[i].support & BIT(2))
				chnlplan->support_mode |= (SUPPORT_11AC);
			if(cdmap_tbl[i].support & BIT(3))
				chnlplan->support_mode |= (SUPPORT_11AX);
			chnlplan->tpo = cdmap_tbl[i].tpo;
			chnlplan->valid = true;
			return;
        }
    }
}

u8 regu_calculate_group_cntry_num(
	struct rtw_regu_policy *policy, u8 group_id,
	u8 rdmap_size, u8 cdmap_size,
	const struct regulatory_domain_mapping *rdmap_tbl,
	const struct country_domain_mapping *cdmap_tbl)
{
	u8 item = 0, item_num = 0;

	PHL_INFO("[REGU], calculate group cntry num, group id = %d \n", group_id);

	if (!rdmap_tbl || !cdmap_tbl || !policy)
		return 0;

	switch (group_id) {
	case DEFAULT_SUPPORT_6G:
		for (item = 0; item < cdmap_size; item++) {
			if (cdmap_tbl[item].domain_code_6g != 0x00)
				item_num++;
		}
		break;
	case CURRENT_SUPPORT_6G:
		for (item = 0; item < cdmap_size; item++) {
			if (cdmap_tbl[item].domain_code_6g != 0x00 &&
				!(policy->cp_6g_bp[item] & CP_6G_BAND_BLOCKED))
				item_num++;
		}
		break;
	case EU_GROUP:
		for (item = 0; item < cdmap_size; item++) {
			if (cdmap_tbl[item].country_property == CNTRY_EU)
				item_num++;
		}
		break;
	case FCC_GROUP:
		{
			u8 domain = 0;
			u8 i = 0;
			u8 did = INVALID_DOMAIN_INDEX;
			for (item = 0; item < cdmap_size; item++) {
				domain = cdmap_tbl[item].domain_code;

				for (i = 0; i < rdmap_size; i++) {
					if (domain == rdmap_tbl[i].domain_code) {
						did = i;
						break;
					}
				}

				if (did != INVALID_DOMAIN_INDEX &&
					rdmap_tbl[did].freq_2g.regulation == REGULATION_FCC)
					item_num++;
			}
		}
		break;
	case OTHERS_MODULE_RF_APPROVAL:
		for (item = 0; item < cdmap_size; item++) {
			if (cdmap_tbl[item].others_module_rf_approval == 1)
				item_num++;
		}
		break;
	}
	return item_num;
}

u8 rtw_phl_query_group_cntry_num(
	void *phl, struct rtw_regu_policy *policy,
	u8 group_id, u8 tbl_idx)
{
	struct phl_info_t *phl_info = (struct phl_info_t *)phl;
	u8 num = 0;

	if (!phl_info || !policy)
		return 0;

	if (tbl_idx == CMN_TBL) {
		PHL_QRY_GRP_CNTRY_NUM(num, policy, group_id, rdmap, cdmap);
	} else if (tbl_idx == IC_TBL) {
		num = rtw_hal_query_group_cntry_num(phl_info->hal,
			policy, group_id);
	}
	return num;
}

bool rtw_phl_set_regulation_info(
	void *phl, struct rtw_regulation_info *regu_info)
{
	struct phl_info_t *phl_info = (struct phl_info_t *)phl;
	struct rtw_regulation_interface *rg_interface = NULL;
	void *d = NULL;

	if (!phl_info || !regu_info)
		return false;

	rg_interface = &phl_info->rg_interface;
	d = phl_to_drvpriv(phl_info);

	_os_spinlock(d, &rg_interface->lock, _bh, NULL);
	_os_mem_cpy(d, &rg_interface->regu_info,
		regu_info, sizeof(struct rtw_regulation_info));
	_os_spinunlock(d, &rg_interface->lock, _bh, NULL);

	return true;
}

u8 rtw_phl_get_domain_regulation(
	void *phl, u8 domain, u8 tbl_idx, enum band_type band)
{
	struct phl_info_t *phl_info = (struct phl_info_t *)phl;
	u8 regu = REGULATION_MAX;

	if (!phl_info)
		return regu;

	if (tbl_idx == CMN_TBL) {
		PHL_GET_DOMAIN_REGU(regu, domain,
			rdmap, band);
	} else if (tbl_idx == IC_TBL) {
		regu = rtw_hal_get_domain_regulation(phl_info->hal,
			domain, band);
	}

	return regu;
}

static void _get_5ghz_ch_info_ex(
	const struct chdef_5ghz *chdef, u8 group,
	struct rtw_chplan_update_info_5g *info)
{
	switch (group) {
	case FREQ_GROUP_5GHZ_BAND1:
		info->ch = chdef->support_ch_b1;
		info->passive = chdef->passive_b1;
		info->dfs = chdef->dfs_b1;
		info->max_num = MAX_CH_NUM_BAND1;
		info->ch_start = 36;
		break;
	case FREQ_GROUP_5GHZ_BAND2:
		info->ch = chdef->support_ch_b2;
		info->passive = chdef->passive_b2;
		info->dfs = chdef->dfs_b2;
		info->max_num = MAX_CH_NUM_BAND2;
		info->ch_start = 52;
		break;
	case FREQ_GROUP_5GHZ_BAND3:
		info->ch = ((chdef->support_ch_b3[1] << 8) |
			(chdef->support_ch_b3[0]));
		info->passive = ((chdef->passive_b3[1] << 8) |
			(chdef->passive_b3[0]));
		info->dfs = ((chdef->dfs_b3[1] << 8) |
			(chdef->dfs_b3[0])) ;
		info->max_num = MAX_CH_NUM_BAND3;
		info->ch_start = 100;
		break;
	case FREQ_GROUP_5GHZ_BAND4:
		info->ch = chdef->support_ch_b4;
		info->passive = chdef->passive_b4;
		info->dfs = chdef->dfs_b4;
		info->max_num = MAX_CH_NUM_BAND4;
		info->ch_start = 149;
		break;
	default:
		info->ch = 0;
		info->passive = 0;
		info->dfs = 0;
		info->max_num = 0;
		info->ch_start = 0;
		break;
	}
}

void regu_get_chplan_update_result_2g(
	u8 did, u8 size, struct rtw_chplan_update_info_2g *info,
	const struct regulatory_domain_mapping *rdmap_tbl,
	const struct chdef_2ghz *chdef_tbl)
{
	struct freq_plan plan;
	u8 i;

	if (!rdmap_tbl || !info || did == INVALID_DOMAIN_INDEX)
		return;
	info->valid = false;

	plan = rdmap_tbl[did].freq_2g;
	for (i = 0; i < size; i++) {
		if (plan.ch_idx == chdef_tbl[i].idx) {
			info->ch_idx = plan.ch_idx;
			info->regulation = plan.regulation;
			info->ch = ((chdef_tbl[i].support_ch[1] << 8) |
				(chdef_tbl[i].support_ch[0]));
			info->passive = ((chdef_tbl[i].passive[1] << 8) |
				(chdef_tbl[i].passive[0]));
			info->valid = true;
			return;
		}
	}
}

void regu_get_chplan_update_result_5g(
	u8 group, u8 did, u8 size,
	struct rtw_chplan_update_info_5g *info,
	const struct regulatory_domain_mapping *rdmap_tbl,
	const struct chdef_5ghz *chdef_tbl)
{
	struct freq_plan plan;
	u8 i;

	if (!rdmap_tbl || !info || did == INVALID_DOMAIN_INDEX)
		return;
	info->valid = false;

	plan = rdmap_tbl[did].freq_5g;
	for (i = 0; i < size; i++) {
		if (plan.ch_idx == chdef_tbl[i].idx) {
			info->ch_idx = plan.ch_idx;
			info->regulation = plan.regulation;
			_get_5ghz_ch_info_ex(&chdef_tbl[i], group, info);
			info->valid = true;
			return;
		}
	}
}

static void _fill_chdef_6g(struct chdef_6ghz *chdef,
	const struct chdef_6ghz *chdef_tgt)
{
	if (!chdef_tgt)
		return;

	chdef->idx = chdef_tgt->idx;

	/* UNII-5 */
	chdef->support_ch_u5[0] = chdef_tgt->support_ch_u5[0];
	chdef->support_ch_u5[1] = chdef_tgt->support_ch_u5[1];
	chdef->support_ch_u5[2] = chdef_tgt->support_ch_u5[2];
	chdef->passive_u5[0] = chdef_tgt->passive_u5[0];
	chdef->passive_u5[1] = chdef_tgt->passive_u5[1];
	chdef->passive_u5[2] = chdef_tgt->passive_u5[2];

	/* UNII-6 */
	chdef->support_ch_u6 = chdef_tgt->support_ch_u6;
	chdef->passive_u6 = chdef_tgt->passive_u6;

	/* UNII-7 */
	chdef->support_ch_u7[0] = chdef_tgt->support_ch_u7[0];
	chdef->support_ch_u7[1] = chdef_tgt->support_ch_u7[1];
	chdef->support_ch_u7[2] = chdef_tgt->support_ch_u7[2];
	chdef->passive_u7[0] = chdef_tgt->passive_u7[0];
	chdef->passive_u7[1] = chdef_tgt->passive_u7[1];
	chdef->passive_u7[2] = chdef_tgt->passive_u7[2];

	/* UNII-8 */
	chdef->support_ch_u8[0] = chdef_tgt->support_ch_u8[0];
	chdef->support_ch_u8[1] = chdef_tgt->support_ch_u8[1];
	chdef->passive_u8[0] = chdef_tgt->passive_u8[0];
	chdef->passive_u8[1] = chdef_tgt->passive_u8[1];
}

void regu_get_chdef_6g(
	u8 idx, u8 size, struct chdef_6ghz *chdef,
	const struct chdef_6ghz *chdef_tbl)
{
	u8 i;
	chdef->idx = INVALID_CH_IDX;

	if (!chdef_tbl)
		return;

	for (i = 0; i < size; i++) {
		if (idx == chdef_tbl[i].idx) {
			_fill_chdef_6g(chdef, &chdef_tbl[i]);
			break;
		}
	}
}

static void _get_chplan_update_info(
	u8 group, u8 did, void *info, enum band_type band)
{
	if (!info)
		return;

	if (band == BAND_ON_24G)
		PHL_GET_CHPLAN_UPDATE_INFO_2G(did, info,
			rdmap, chdef2g);
	else if (band == BAND_ON_5G)
		PHL_GET_CHPLAN_UPDATE_INFO_5G(group, did, info,
			rdmap, chdef5g);
}

void rtw_phl_get_chplan_update_info(
	void *phl, u8 group, u8 did, void *info,
	u8 tbl_idx, enum band_type band)
{
	struct phl_info_t *phl_info = (struct phl_info_t *)phl;

	if (!phl_info || !info)
		return;

	if (tbl_idx == CMN_TBL) {
		_get_chplan_update_info(group, did, info, band);
	} else if (tbl_idx == IC_TBL) {
		rtw_hal_get_chplan_update_info(
			phl_info->hal, group, did, info, band);
	}
}

bool rtw_phl_regu_interface_init(void *phl)
{
	struct phl_info_t *phl_info = (struct phl_info_t *)phl;
	struct rtw_regulation_interface *rg_interface = NULL;
	void *d = NULL;

	if (!phl_info)
		return false;

	PHL_INFO("rtw_phl_regu_interface_init \n");

	rg_interface = &phl_info->rg_interface;
	d = phl_to_drvpriv(phl_info);
	_os_spinlock_init(d, &rg_interface->lock);

	_os_spinlock(d, &rg_interface->lock, _bh, NULL);
	rg_interface->regu_info.domain_code = INVALID_DOMAIN_CODE;
	rg_interface->regu_info.domain_code_6g = INVALID_DOMAIN_CODE;
	rg_interface->regu_info.tpo = TPO_NA;
	_os_spinunlock(d, &rg_interface->lock, _bh, NULL);

	return true;
}

bool rtw_phl_regu_interface_deinit(void *phl)
{
	struct phl_info_t *phl_info = (struct phl_info_t *)phl;
	struct rtw_regulation_interface *rg_interface = NULL;
	void *d = NULL;

	if (!phl_info)
		return false;

	PHL_INFO("rtw_phl_regu_interface_deinit \n");

	rg_interface = &phl_info->rg_interface;
	d = phl_to_drvpriv(phl_info);
	_os_spinlock_free(d, &rg_interface->lock);

	return true;
}

u8 rtw_phl_get_regu_country_ver_ex(
	void *phl, u8 tbl_idx)
{
	struct phl_info_t *phl_info = (struct phl_info_t *)phl;

	if (!phl_info)
		return INVALID_VER;

	if (tbl_idx == CMN_TBL)
		return REGULATION_COUNTRY_VERSION;
	else if (tbl_idx == IC_TBL)
		return rtw_hal_get_country_ver(phl_info->hal);
	else
		return INVALID_VER;
}

u8 rtw_phl_get_regu_chplan_ver_ex(
	void *phl, u8 tbl_idx)
{
	struct phl_info_t *phl_info = (struct phl_info_t *)phl;

	if (!phl_info)
		return INVALID_VER;

	if (tbl_idx == CMN_TBL)
		return REGULATION_CHPLAN_VERSION;
	else if (tbl_idx == IC_TBL)
		return rtw_hal_get_chnlplan_ver(phl_info->hal);
	else
		return INVALID_VER;
}

u8 rtw_phl_get_regu_country_ver(void)
{
	return REGULATION_COUNTRY_VERSION;
}

u8 rtw_phl_get_regu_chplan_ver(void)
{
	return REGULATION_CHPLAN_VERSION;
}

/* 6g */
void rtw_phl_get_6g_regulatory_info(void *phl,
	u8 domain, u8 *dm_code, u8 *regulation, u8 *ch_idx,
	u8 tbl_idx)
{
	struct phl_info_t *phl_info = (struct phl_info_t *)phl;

	if (!phl_info || !dm_code || !regulation || !ch_idx)
		return;

	if (tbl_idx == CMN_TBL) {
		PHL_GET_6G_REGULATORY_INFO(*dm_code, *regulation,
			*ch_idx, domain, rdmap6);
	} else if (tbl_idx == IC_TBL) {
		rtw_hal_get_6g_regulatory_info(
			phl_info->hal, domain, dm_code, regulation,
			ch_idx);
	}
}

void
rtw_phl_get_chdef_6g(void *phl,
	u8 ch_idx, struct chdef_6ghz *chdef, u8 tbl_idx)
{
	struct phl_info_t *phl_info = (struct phl_info_t *)phl;
	chdef->idx = INVALID_CH_IDX;

	if (!phl_info || !chdef)
		return;

	if (tbl_idx == CMN_TBL) {
		PHL_GET_CHDEF_6G(ch_idx, chdef, chdef6g);
	} else if (tbl_idx == IC_TBL) {
		rtw_hal_get_chdef_6g(
			phl_info->hal, ch_idx, chdef);
	}
}

u8 rtw_phl_get_cat6g_by_country(char *cntry)
{
	u8 cat6g = 0;

	PHL_GET_CAT6G_BY_COUNTRY(cat6g, cntry, cdmap);

	return cat6g;
}

u8 rtw_phl_get_cat6g_by_country_ex(void * phl,
	char *country, u8 tbl_idx)
{
	struct phl_info_t *phl_info = (struct phl_info_t *)phl;
	u8 cat6g = 0;

	if (!phl_info || !country)
		return cat6g;

	if (tbl_idx == CMN_TBL) {
		PHL_GET_CAT6G_BY_COUNTRY(cat6g, country, cdmap);
	} else if (tbl_idx == IC_TBL) {
		cat6g = rtw_hal_get_cat6g_by_country(
			phl_info->hal, country);
	}

	return cat6g;
}

/* legacy api below, will be removed */
static void _get_5ghz_ch_info(const struct chdef_5ghz *chdef,
	u8 group, u16 *ch, u16 *passive, u16 *dfs, u8 *max_num, u8 *ch_start)
{
	switch (group) {
	case FREQ_GROUP_5GHZ_BAND1:
		*ch = chdef->support_ch_b1;
		*passive = chdef->passive_b1;
		*dfs = chdef->dfs_b1;
		*max_num = MAX_CH_NUM_BAND1;
		*ch_start = 36;
		break;
	case FREQ_GROUP_5GHZ_BAND2:
		*ch = chdef->support_ch_b2;
		*passive = chdef->passive_b2;
		*dfs = chdef->dfs_b2;
		*max_num = MAX_CH_NUM_BAND2;
		*ch_start = 52;
		break;
	case FREQ_GROUP_5GHZ_BAND3:
		*ch = ((chdef->support_ch_b3[1] << 8) |
			(chdef->support_ch_b3[0]));
		*passive = ((chdef->passive_b3[1] << 8) |
			(chdef->passive_b3[0]));
		*dfs = ((chdef->dfs_b3[1] << 8) |
			(chdef->dfs_b3[0])) ;
		*max_num = MAX_CH_NUM_BAND3;
		*ch_start = 100;
		break;
	case FREQ_GROUP_5GHZ_BAND4:
		*ch = chdef->support_ch_b4;
		*passive = chdef->passive_b4;
		*dfs = chdef->dfs_b4;
		*max_num = MAX_CH_NUM_BAND4;
		*ch_start = 149;
		break;
	default:
		*ch = 0;
		*passive = 0;
		*dfs = 0;
		*max_num = 0;
		*ch_start = 0;
		break;
	}
}

static u8 _domain_index(u8 domain)
{
	u8 num_rdmap;
	u8 i = 0;

	num_rdmap = sizeof(rdmap) / sizeof(struct regulatory_domain_mapping);

	for (i = 0; i < num_rdmap; i++) {
		if (domain == rdmap[i].domain_code) {
			return i;
		}
	}

	return num_rdmap;
}

u8 rtw_phl_get_domain_regulation_2g(u8 domain)
{
	u8 did, num_rdmap;

	num_rdmap = sizeof(rdmap) / sizeof(struct regulatory_domain_mapping);
	did = num_rdmap;

	if (!rtw_phl_valid_regulation_domain(domain))
		return REGULATION_MAX;

	did = _domain_index(domain);
	if (did >= num_rdmap)
		return REGULATION_MAX;

	return rdmap[did].freq_2g.regulation;
}

u8 rtw_phl_get_domain_regulation_5g(u8 domain)
{
	u8 did, num_rdmap;

	num_rdmap = sizeof(rdmap) / sizeof(struct regulatory_domain_mapping);
	did = num_rdmap;

	if (!rtw_phl_valid_regulation_domain(domain))
		return REGULATION_MAX;

	did = _domain_index(domain);
	if (did >= num_rdmap)
		return REGULATION_MAX;

	return rdmap[did].freq_5g.regulation;
}

static u8 _domain_index_6g(u8 domain)
{
	u8 num_rdmap6;
	u8 i = 0;

	num_rdmap6 = sizeof(rdmap6) / sizeof(struct regulatory_domain_mapping_6g);

	for (i = 0; i < num_rdmap6; i++) {
		if (domain == rdmap6[i].domain_code) {
			return i;
		}
	}

	return num_rdmap6;
}

static bool _valid_domain_6g(u8 domain)
{
	u8 num_rdmap6;

	num_rdmap6 = sizeof(rdmap6) / sizeof(struct regulatory_domain_mapping_6g);

	if (domain == RSVD_DOMAIN)
		return true;

	if (_domain_index_6g(domain) >= num_rdmap6)
		return false;

	return true;
}

static u8 _get_domain_regulation_6g(u8 domain)
{
	u8 did, num_rdmap6;

	num_rdmap6 = sizeof(rdmap6) / sizeof(struct regulatory_domain_mapping_6g);
	did = num_rdmap6;

	if (!_valid_domain_6g(domain))
		return REGULATION_MAX;

	did = _domain_index_6g(domain);
	if (did >= num_rdmap6)
		return REGULATION_MAX;

	return rdmap6[did].regulation;
}

u8 rtw_phl_get_domain_regulation_6g(u8 domain)
{
	return _get_domain_regulation_6g(domain);
}

static bool _query_regu_ch2g(u8 did, u8 ch, enum ch_property *prop)
{
	u8 idx2g = rdmap[did].freq_2g.ch_idx;
	const struct chdef_2ghz *chdef2 = NULL;
	u8 i, num_chdef2g;

	num_chdef2g = sizeof(chdef2g) / sizeof(struct chdef_2ghz);

	for (i = 0; i < num_chdef2g; i++) {
		if (idx2g == chdef2g[i].idx) {
			chdef2 = &chdef2g[i];
			break;
		}
	}

	if (chdef2) {
		u16 ch_bmp = ((chdef2->support_ch[1] << 8) |
			(chdef2->support_ch[0]));

		if (ch_bmp) {
			u16 passive_bmp = ((chdef2->passive[1] << 8) |
				(chdef2->passive[0]));
			u32 shift;

			for (i = 0; i < MAX_CH_NUM_2GHZ; i++) {
				shift = (1 << i);
				if ((ch_bmp & shift) && ch == i + 1) {
					if (prop)
						*prop = (passive_bmp & shift) ? CH_PASSIVE : 0;
					return true;
				}
			}
		}
	}

	return false;
}

static bool _query_regu_ch5g(u8 did, enum rtw_regulation_freq_group freq_gid,
	u8 ch, enum ch_property *prop)
{
	u8 idx5g = idx5g = rdmap[did].freq_5g.ch_idx;
	const struct chdef_5ghz *chdef5 = NULL;
	u16 i, num_chdef5g;

	num_chdef5g = sizeof(chdef5g) / sizeof(struct chdef_5ghz);

	for (i = 0; i < num_chdef5g; i++) {
		if (idx5g == chdef5g[i].idx) {
			chdef5 = &chdef5g[i];
			break;
		}
	}

	if (chdef5) {
		u16 ch_bmp, passive_bmp, dfs_bmp;
		u8 max_num, ch_start;

		_get_5ghz_ch_info(chdef5, freq_gid, &ch_bmp, &passive_bmp, &dfs_bmp,
			&max_num, &ch_start);

		if (ch_bmp) {
			u32 shift;

			for (i = 0; i < max_num; i++) {
				shift = (1 << i);
				if ((ch_bmp & shift) && ch == ch_start + i * 4) {
					if (prop) {
						*prop = ((passive_bmp & shift) ? CH_PASSIVE : 0)
							| ((dfs_bmp & shift) ? CH_DFS : 0);
					}
					return true;
				}
			}
		}
	}

	return false;
}

/*
 * @ Function description
 *	Use the domain code, band, ch  to query the corresponding
 *	regulation channel and property
 *
 * @ parameter
 * 	domain : the specified domain code
 *    band : the specified band
 *    ch : the specified channel
 *	prop : if the regulation channel exist, the resulting property will
 *              be filled
 *
 * @ return :
 *	true : the queried regulation channel exist
 *	false : not exist
 *
 */
bool rtw_phl_query_domain_channel(u8 domain, enum band_type band, u8 ch,
			enum ch_property *prop)
{
	enum rtw_regulation_freq_group freq_gid;
	u8 did, num_rdmap;

	num_rdmap = sizeof(rdmap) / sizeof(struct regulatory_domain_mapping);

	if (band != BAND_ON_24G && band != BAND_ON_5G)
		return false;

	freq_gid = rtw_phl_get_regu_freq_group(band, ch);
	if (freq_gid < FREQ_GROUP_2GHZ || freq_gid > FREQ_GROUP_5GHZ_BAND4)
		return false;

	did = _domain_index(domain);
	if (did >= num_rdmap)
		return false;

	if (band == BAND_ON_24G)
		return _query_regu_ch2g(did, ch, prop);
	else if (band == BAND_ON_5G)
		return _query_regu_ch5g(did, freq_gid, ch, prop);
	return false;
}

static u16 _country_index(char *cntry)
{
	u16 i = 0;
	u8 num_cdmap;

	num_cdmap = sizeof(cdmap) / sizeof(struct country_domain_mapping);

	for (i = 0; i < num_cdmap; i++) {
		if (cdmap[i].char2[0] == cntry[0] &&
			cdmap[i].char2[1] == cntry[1]) {
			return i;
		}
	}

	return num_cdmap;
}

bool rtw_phl_query_cntry_exist(char *cntry)
{
	bool exist = false;
	u8 num_cdmap;

	num_cdmap = sizeof(cdmap) / sizeof(struct country_domain_mapping);

	if (_country_index(cntry) < num_cdmap)
		exist = true;

	return exist;
}

static void _get_6ghz_ch_info(const struct chdef_6ghz *chdef,
	u8 group, u32 *ch, u32 *passive, u8 *max_num, u8 *ch_start)
{
	switch (group) {
	case FREQ_GROUP_6GHZ_UNII5:
		*ch = ((chdef->support_ch_u5[2] << 16) |
			(chdef->support_ch_u5[1] << 8) |
			(chdef->support_ch_u5[0]));
		*passive = ((chdef->passive_u5[2] << 16) |
			(chdef->passive_u5[1] << 8) |
			(chdef->passive_u5[0]));
		*max_num = MAX_CH_NUM_UNII5;
		*ch_start = 1;
		break;
	case FREQ_GROUP_6GHZ_UNII6:
		*ch = chdef->support_ch_u6;
		*passive = chdef->passive_u6;
		*max_num = MAX_CH_NUM_UNII6;
		*ch_start = 97;
		break;
	case FREQ_GROUP_6GHZ_UNII7:
		*ch = ((chdef->support_ch_u7[2] << 16) |
			(chdef->support_ch_u7[1] << 8) |
			(chdef->support_ch_u7[0]));
		*passive = ((chdef->passive_u7[2] << 16) |
			(chdef->passive_u7[1] << 8) |
			(chdef->passive_u7[0]));
		*max_num = MAX_CH_NUM_UNII7;
		*ch_start = 121;
		break;
	case FREQ_GROUP_6GHZ_UNII8:
		*ch = ((chdef->support_ch_u8[1] << 8) |
			(chdef->support_ch_u8[0]));
		*passive = ((chdef->passive_u8[1] << 8) |
			(chdef->passive_u8[0]));
		*max_num = MAX_CH_NUM_UNII8;
		*ch_start = 193;
		break;
	default:
		*ch = 0;
		*passive = 0;
		*max_num = 0;
		*ch_start = 0;
		break;
	}
}

static bool _query_regu_ch6g(u8 did, enum rtw_regulation_freq_group freq_gid,
	u8 ch, enum ch_property *prop)
{
	u8 idx6g = rdmap6[did].ch_idx;
	u8 	num_chdef6g;
	const struct chdef_6ghz *chdef6 = NULL;
	u16 i;
	u32 shift;

	num_chdef6g = sizeof(chdef6g) / sizeof(struct chdef_6ghz);

	for (i = 0; i < num_chdef6g; i++) {
		if (idx6g == chdef6g[i].idx) {
			chdef6 = &chdef6g[i];
			break;
		}
	}

	if (chdef6) {
		u32 ch_bmp, passive_bmp;
		u8 max_num, ch_start;

		_get_6ghz_ch_info(chdef6, freq_gid, &ch_bmp, &passive_bmp,
			&max_num, &ch_start);

		if (ch_bmp) {
			for (i = 0; i < max_num; i++) {
				shift = (1 << i);
				if ((ch_bmp & shift) && ch == ch_start + i * 4) {
					if (prop)
						*prop = (passive_bmp & shift) ? CH_PASSIVE : 0;
					return true;
				}
			}
		}
	}

	return false;
}

/*
 * @ Function description
 *	Use the domain code, band, ch  to query the corresponding
 *	regulation channel  and property
 *
 * @ parameter
 * 	domain : the specified domain code
 *    band : the specified band
 *    ch : the specified channel
 *	prop : if the regulation channel exist, the resulting property will
 *              be filled
 *
 * @ return :
 *	true : the queried regulation channel exist
 *	false : not exist
 *
 */
bool regu_query_domain_6g_channel(u8 domain, enum band_type band, u8 ch,
			enum ch_property *prop)
{
	enum rtw_regulation_freq_group freq_gid;
	u8 did, num_rdmap6;

	num_rdmap6 = sizeof(rdmap6) / sizeof(struct regulatory_domain_mapping_6g);

	if (band != BAND_ON_6G)
		return false;

	freq_gid = rtw_phl_get_regu_freq_group(band, ch);
	if (freq_gid < FREQ_GROUP_6GHZ_UNII5 || freq_gid > FREQ_GROUP_6GHZ_UNII8)
		return false;

	did = _domain_index_6g(domain);
	if (did >= num_rdmap6)
		return false;

	return _query_regu_ch6g(did, freq_gid, ch, prop);
}

bool rtw_phl_query_domain_6g_channel(u8 domain, enum band_type band, u8 ch,
	enum ch_property *prop)
{
	return regu_query_domain_6g_channel(domain, band, ch, prop);
}
