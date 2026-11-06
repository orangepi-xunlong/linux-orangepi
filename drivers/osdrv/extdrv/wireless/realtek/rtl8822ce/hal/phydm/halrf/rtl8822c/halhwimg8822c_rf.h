/******************************************************************************
 *
 * Copyright(c) 2007 - 2017 Realtek Corporation.
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
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
 * Realtek Corporation, No. 2, Innovation Road II, Hsinchu Science Park,
 * Hsinchu 300, Taiwan.
 *
 * Larry Finger <Larry.Finger@lwfinger.net>
 *
 *****************************************************************************/

/*Image2HeaderVersion: R3 1.5.12*/
#if (RTL8822C_SUPPORT == 1)
#ifndef __INC_MP_RF_HW_IMG_8822C_H
#define __INC_MP_RF_HW_IMG_8822C_H

/* Please add following compiler flags definition (#define CONFIG_XXX_DRV_DIS)
 * into driver source code to reduce code size if necessary.
 * #define CONFIG_8822C_DRV_DIS
 * #define CONFIG_8822CTSSI_DRV_DIS
 * #define CONFIG_8822C_TYPE5_DRV_DIS
 */

#define CONFIG_8822C
#ifdef CONFIG_8822C_DRV_DIS
    #undef CONFIG_8822C
#endif

#define CONFIG_8822CTSSI
#ifdef CONFIG_8822CTSSI_DRV_DIS
    #undef CONFIG_8822CTSSI
#endif

#define CONFIG_8822C_TYPE5
#ifdef CONFIG_8822C_TYPE5_DRV_DIS
    #undef CONFIG_8822C_TYPE5
#endif

struct txpwr_lmt_t_8822c {
	u8			reg;
	u8			band:1;
	u8			bw:3;
	u8			rs:2;
	u8			ntx:2;
	u8			ch;
	s8			val;
};

/******************************************************************************
 *                           radioa.TXT
 ******************************************************************************/

/* tc: Test Chip, mp: mp Chip*/
void
odm_read_and_config_mp_8822c_radioa(struct dm_struct *dm);
u32 odm_get_version_mp_8822c_radioa(void);

/******************************************************************************
 *                           radiob.TXT
 ******************************************************************************/

/* tc: Test Chip, mp: mp Chip*/
void
odm_read_and_config_mp_8822c_radiob(struct dm_struct *dm);
u32 odm_get_version_mp_8822c_radiob(void);

/******************************************************************************
 *                           txpowertrack.TXT
 ******************************************************************************/

/* tc: Test Chip, mp: mp Chip*/
void
odm_read_and_config_mp_8822c_txpowertrack(struct dm_struct *dm);
u32 odm_get_version_mp_8822c_txpowertrack(void);

/******************************************************************************
 *                           txpowertracktssi.TXT
 ******************************************************************************/

/* tc: Test Chip, mp: mp Chip*/
void
odm_read_and_config_mp_8822c_txpowertracktssi(struct dm_struct *dm);
u32 odm_get_version_mp_8822c_txpowertracktssi(void);

/******************************************************************************
 *                           txpwr_lmt.TXT
 ******************************************************************************/

/* tc: Test Chip, mp: mp Chip*/
void
odm_read_and_config_mp_8822c_txpwr_lmt(struct dm_struct *dm);
u32 odm_get_version_mp_8822c_txpwr_lmt(void);

/******************************************************************************
 *                           txpwr_lmt_type5.TXT
 ******************************************************************************/

/* tc: Test Chip, mp: mp Chip*/
void
odm_read_and_config_mp_8822c_txpwr_lmt_type5(struct dm_struct *dm);
u32 odm_get_version_mp_8822c_txpwr_lmt_type5(void);

#endif
#endif /* end of HWIMG_SUPPORT*/

