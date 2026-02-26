/******************************************************************************
 *
 * Copyright(c) 2007 - 2021 Realtek Corporation.
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
#define _RTW_TDLS_C_

#include <drv_types.h>
#if defined(CONFIG_FPGA_INCLUDED)

void rtw_fpga_start(_adapter *padapter)
{
	struct dvobj_priv *dvobj = adapter_to_dvobj(padapter);
	struct rtw_test_module_info test_module_info;

	test_module_info.tm_mode = RTW_DRV_MODE_FPGA_SMDL_TEST;
	test_module_info.tm_type = RTW_TEST_SUB_MODULE_FPGA;
	rtw_phl_test_submodule_init(dvobj->phl_com, &test_module_info);
}

void rtw_fpga_stop(_adapter *padapter)
{
	struct dvobj_priv *dvobj = adapter_to_dvobj(padapter);
	struct rtw_test_module_info test_module_info;

	test_module_info.tm_mode = RTW_DRV_MODE_NORMAL;
	test_module_info.tm_type = RTW_TEST_SUB_MODULE_FPGA;
	rtw_phl_test_submodule_deinit(dvobj->phl_com, &test_module_info);
}


#endif /* CONFIG_FPGA_INCLUDED */
