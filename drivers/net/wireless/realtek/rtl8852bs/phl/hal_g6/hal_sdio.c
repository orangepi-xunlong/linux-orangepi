/******************************************************************************
 *
 * Copyright(c) 2019 Realtek Corporation.
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
#define _HAL_SDIO_C_
#include "hal_headers.h"

#ifdef CONFIG_SDIO_HCI
static u8 hal_sdio_f0_read8(struct rtw_hal_com_t *hal, u32 addr)
{
	u8 val = 0;
	u8 ret;

	ret = _os_sdio_f0_read(hal->drv_priv, addr, &val, 1);
	if (_FAIL == ret)
		PHL_ERR("%s: Read f0 register(0x%x) FAIL!\n",
			__FUNCTION__, addr);

	return val;
}
#ifdef RTW_WKARD_BUS_WRITE
static int sdio_write_post_cfg(struct rtw_hal_com_t *hal, u32 addr, u32 val)
{
	struct hal_info_t	*hal_info = hal->hal_priv;
	struct hal_ops_t	*hal_ops = hal_get_ops(hal_info);

	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;
	
	if(NULL != hal_ops->write_reg_post_cfg) {
		hal_status = hal_ops->write_reg_post_cfg(hal_info, addr, val);
	}

	return hal_status;
}
#endif
void hal_sdio_set_io_ops(struct rtw_hal_com_t *hal, struct hal_io_ops *pops)
{
	pops->_read8 = hal_mac_sdio_read8;
	pops->_read16 = hal_mac_sdio_read16;
	pops->_read32 = hal_mac_sdio_read32;
	pops->_read_mem = hal_mac_sdio_read_mem;

	pops->_write8 = hal_mac_sdio_write8;
	pops->_write16 = hal_mac_sdio_write16;
	pops->_write32 = hal_mac_sdio_write32;

	pops->_sd_f0_read8 = hal_sdio_f0_read8;

#ifdef CONFIG_SDIO_INDIRECT_ACCESS
	pops->_sd_iread8 = hal_mac_sdio_iread8;
	pops->_sd_iread16 = hal_mac_sdio_iread16;
	pops->_sd_iread32 = hal_mac_sdio_iread32;
	pops->_sd_iwrite8 = hal_mac_sdio_write8;
	pops->_sd_iwrite16 = hal_mac_sdio_write16;
	pops->_sd_iwrite32 = hal_mac_sdio_write32;
#endif /* CONFIG_SDIO_INDIRECT_ACCESS */
#ifdef RTW_WKARD_BUS_WRITE
	pops->_write_post_cfg = &sdio_write_post_cfg;
#endif
}
#endif
