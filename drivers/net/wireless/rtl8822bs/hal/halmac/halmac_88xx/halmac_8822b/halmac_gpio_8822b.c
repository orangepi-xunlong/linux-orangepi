/******************************************************************************
 *
 * Copyright(c) 2016 - 2017 Realtek Corporation. All rights reserved.
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
 ******************************************************************************/

#include "halmac_gpio_8822b.h"
#include "../halmac_gpio_88xx.h"

#if HALMAC_8822B_SUPPORT

const HALMAC_GPIO_PIMUX_LIST PIMUX_LIST_GPIO0_8822B[] = {
	HALMAC_GPIO0_BT_GPIO0_8822B,
	HALMAC_GPIO0_BT_ACT_8822B,
	HALMAC_GPIO0_WL_ACT_8822B,
	HALMAC_GPIO0_WLMAC_DBG_GPIO0_8822B,
	HALMAC_GPIO0_WLPHY_DBG_GPIO0_8822B,
	HALMAC_GPIO0_BT_DBG_GPIO0_8822B,
	HALMAC_GPIO0_SW_IO_8822B
};

const HALMAC_GPIO_PIMUX_LIST PIMUX_LIST_GPIO1_8822B[] = {
	HALMAC_GPIO1_BT_GPIO1_8822B,
	HALMAC_GPIO1_BT_3DD_SYNC_A_8822B,
	HALMAC_GPIO1_WL_CK_8822B,
	HALMAC_GPIO1_BT_CK_8822B,
	HALMAC_GPIO1_WLMAC_DBG_GPIO1_8822B,
	HALMAC_GPIO1_WLPHY_DBG_GPIO1_8822B,
	HALMAC_GPIO1_BT_DBG_GPIO1_8822B,
	HALMAC_GPIO1_SW_IO_8822B
};

const HALMAC_GPIO_PIMUX_LIST PIMUX_LIST_GPIO2_8822B[] = {
	HALMAC_GPIO2_BT_GPIO2_8822B,
	HALMAC_GPIO2_WL_STATE_8822B,
	HALMAC_GPIO2_BT_STATE_8822B,
	HALMAC_GPIO2_WLMAC_DBG_GPIO2_8822B,
	HALMAC_GPIO2_WLPHY_DBG_GPIO2_8822B,
	HALMAC_GPIO2_BT_DBG_GPIO2_8822B,
	HALMAC_GPIO2_RFE_CTRL_5_8822B,
	HALMAC_GPIO2_SW_IO_8822B
};

const HALMAC_GPIO_PIMUX_LIST PIMUX_LIST_GPIO3_8822B[] = {
	HALMAC_GPIO3_BT_GPIO3_8822B,
	HALMAC_GPIO3_WL_PRI_8822B,
	HALMAC_GPIO3_BT_PRI_8822B,
	HALMAC_GPIO3_WLMAC_DBG_GPIO3_8822B,
	HALMAC_GPIO3_WLPHY_DBG_GPIO3_8822B,
	HALMAC_GPIO3_BT_DBG_GPIO3_8822B,
	HALMAC_GPIO3_RFE_CTRL_4_8822B,
	HALMAC_GPIO3_SW_IO_8822B
};

const HALMAC_GPIO_PIMUX_LIST PIMUX_LIST_GPIO4_8822B[] = {
	HALMAC_GPIO4_BT_SPI_D0_8822B,
	HALMAC_GPIO4_WL_SPI_D0_8822B,
	HALMAC_GPIO4_SDIO_INT_8822B,
	HALMAC_GPIO4_JTAG_TRST_8822B,
	HALMAC_GPIO4_DBG_GNT_WL_8822B,
	HALMAC_GPIO4_WLMAC_DBG_GPIO4_8822B,
	HALMAC_GPIO4_WLPHY_DBG_GPIO4_8822B,
	HALMAC_GPIO4_BT_DBG_GPIO4_8822B,
	HALMAC_GPIO4_SW_IO_8822B
};

const HALMAC_GPIO_PIMUX_LIST PIMUX_LIST_GPIO5_8822B[] = {
	HALMAC_GPIO5_BT_SPI_D1_8822B,
	HALMAC_GPIO5_WL_SPI_D1_8822B,
	HALMAC_GPIO5_JTAG_TDI_8822B,
	HALMAC_GPIO5_DBG_GNT_BT_8822B,
	HALMAC_GPIO5_WLMAC_DBG_GPIO5_8822B,
	HALMAC_GPIO5_WLPHY_DBG_GPIO5_8822B,
	HALMAC_GPIO5_BT_DBG_GPIO5_8822B,
	HALMAC_GPIO5_SW_IO_8822B
};

const HALMAC_GPIO_PIMUX_LIST PIMUX_LIST_GPIO6_8822B[] = {
	HALMAC_GPIO6_BT_SPI_D2_8822B,
	HALMAC_GPIO6_WL_SPI_D2_8822B,
	HALMAC_GPIO6_EEDO_8822B,
	HALMAC_GPIO6_JTAG_TDO_8822B,
	HALMAC_GPIO6_BT_3DD_SYNC_B_8822B,
	HALMAC_GPIO6_BT_GPIO18_8822B,
	HALMAC_GPIO6_SIN_8822B,
	HALMAC_GPIO6_WLMAC_DBG_GPIO6_8822B,
	HALMAC_GPIO6_WLPHY_DBG_GPIO6_8822B,
	HALMAC_GPIO6_BT_DBG_GPIO6_8822B,
	HALMAC_GPIO6_SW_IO_8822B
};

const HALMAC_GPIO_PIMUX_LIST PIMUX_LIST_GPIO7_8822B[] = {
	HALMAC_GPIO7_BT_SPI_D3_8822B,
	HALMAC_GPIO7_WL_SPI_D3_8822B,
	HALMAC_GPIO7_EEDI_8822B,
	HALMAC_GPIO7_JTAG_TMS_8822B,
	HALMAC_GPIO7_BT_GPIO16_8822B,
	HALMAC_GPIO7_SOUT_8822B,
	HALMAC_GPIO7_WLMAC_DBG_GPIO7_8822B,
	HALMAC_GPIO7_WLPHY_DBG_GPIO7_8822B,
	HALMAC_GPIO7_BT_DBG_GPIO7_8822B,
	HALMAC_GPIO7_SW_IO_8822B
};

const HALMAC_GPIO_PIMUX_LIST PIMUX_LIST_GPIO8_8822B[] = {
	HALMAC_GPIO8_WL_EXT_WOL_8822B,
	HALMAC_GPIO8_WL_LED_8822B,
	HALMAC_GPIO8_SW_IO_8822B
};

const HALMAC_GPIO_PIMUX_LIST PIMUX_LIST_GPIO9_8822B[] = {
	HALMAC_GPIO9_DIS_WL_N_8822B,
	HALMAC_GPIO9_WL_EXT_WOL_8822B,
	HALMAC_GPIO9_USCTS0_8822B,
	HALMAC_GPIO9_SW_IO_8822B
};

const HALMAC_GPIO_PIMUX_LIST PIMUX_LIST_GPIO10_8822B[] = {
	HALMAC_GPIO10_SW_IO_8822B
};

const HALMAC_GPIO_PIMUX_LIST PIMUX_LIST_GPIO11_8822B[] = {
	HALMAC_GPIO11_DIS_BT_N_8822B,
	HALMAC_GPIO11_USOUT0_8822B,
	HALMAC_GPIO11_SW_IO_8822B
};

const HALMAC_GPIO_PIMUX_LIST PIMUX_LIST_GPIO12_8822B[] = {
	HALMAC_GPIO12_USIN0_8822B,
	HALMAC_GPIO12_SW_IO_8822B
};

const HALMAC_GPIO_PIMUX_LIST PIMUX_LIST_GPIO13_8822B[] = {
	HALMAC_GPIO13_BT_WAKE_8822B,
	HALMAC_GPIO13_SW_IO_8822B
};

const HALMAC_GPIO_PIMUX_LIST PIMUX_LIST_GPIO14_8822B[] = {
	HALMAC_GPIO14_UART_WAKE_8822B,
	HALMAC_GPIO14_SW_IO_8822B
};

const HALMAC_GPIO_PIMUX_LIST PIMUX_LIST_GPIO15_8822B[] = {
	HALMAC_GPIO15_EXT_XTAL_8822B,
	HALMAC_GPIO15_SW_IO_8822B
};

static HALMAC_RET_STATUS
halmac_get_pinmux_list_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_GPIO_FUNC gpio_func,
	OUT const HALMAC_GPIO_PIMUX_LIST **ppPinmux_list,
	OUT u32 *pList_size,
	OUT u32 *pGpio_id
);

static HALMAC_RET_STATUS
halmac_chk_pinmux_valid_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_GPIO_FUNC gpio_func
);

/**
 * halmac_pinmux_get_func_8822b() -get current gpio status
 * @pHalmac_adapter : the adapter of halmac
 * @gpio_func : gpio function
 * @pEnable : function is enable(1) or disable(0)
 * Author : Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_pinmux_get_func_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_GPIO_FUNC gpio_func,
	OUT u8 *pEnable
)
{
	u32 list_size;
	u32 curr_func;
	u32 gpio_id;
	HALMAC_RET_STATUS status;
	const HALMAC_GPIO_PIMUX_LIST *pPinmux_list = NULL;
	VOID *pDriver_adapter = NULL;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_pinmux_get_func_8822b ==========>\n");

	status = halmac_get_pinmux_list_8822b(pHalmac_adapter, gpio_func, &pPinmux_list, &list_size, &gpio_id);
	if (status != HALMAC_RET_SUCCESS)
		return status;

	status = halmac_pinmux_parser_88xx(pHalmac_adapter, pPinmux_list, list_size, gpio_id, &curr_func);
	if (status != HALMAC_RET_SUCCESS)
		return status;

	switch (gpio_func) {
	case HALMAC_GPIO_FUNC_WL_LED:
		*pEnable = (curr_func == HALMAC_WL_LED) ? 1 : 0;
		break;
	case HALMAC_GPIO_FUNC_SDIO_INT:
		*pEnable = (curr_func == HALMAC_SDIO_INT) ? 1 : 0;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_0:
	case HALMAC_GPIO_FUNC_SW_IO_1:
	case HALMAC_GPIO_FUNC_SW_IO_2:
	case HALMAC_GPIO_FUNC_SW_IO_3:
	case HALMAC_GPIO_FUNC_SW_IO_4:
	case HALMAC_GPIO_FUNC_SW_IO_5:
	case HALMAC_GPIO_FUNC_SW_IO_6:
	case HALMAC_GPIO_FUNC_SW_IO_7:
	case HALMAC_GPIO_FUNC_SW_IO_8:
	case HALMAC_GPIO_FUNC_SW_IO_9:
	case HALMAC_GPIO_FUNC_SW_IO_10:
	case HALMAC_GPIO_FUNC_SW_IO_11:
	case HALMAC_GPIO_FUNC_SW_IO_12:
	case HALMAC_GPIO_FUNC_SW_IO_13:
	case HALMAC_GPIO_FUNC_SW_IO_14:
	case HALMAC_GPIO_FUNC_SW_IO_15:
		*pEnable = (curr_func == HALMAC_SW_IO) ? 1 : 0;
		break;
	default:
		*pEnable = 0;
		return HALMAC_RET_GET_PINMUX_ERR;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_pinmux_get_func_8822b <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_pinmux_set_func_8822b() -set gpio function
 * @pHalmac_adapter : the adapter of halmac
 * @gpio_func : gpio function
 * Author : Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_pinmux_set_func_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_GPIO_FUNC gpio_func
)
{
	u32 list_size;
	u32 gpio_id;
	HALMAC_RET_STATUS status;
	const HALMAC_GPIO_PIMUX_LIST *pPinmux_list = NULL;
	VOID *pDriver_adapter = NULL;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_pinmux_set_func_8822b ==========>\n");
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]func name : %d\n", gpio_func);

	status = halmac_chk_pinmux_valid_8822b(pHalmac_adapter, gpio_func);
	if (status != HALMAC_RET_SUCCESS)
		return status;

	status = halmac_get_pinmux_list_8822b(pHalmac_adapter, gpio_func, &pPinmux_list, &list_size, &gpio_id);
	if (status != HALMAC_RET_SUCCESS)
		return status;

	status = halmac_pinmux_switch_88xx(pHalmac_adapter, pPinmux_list, list_size, gpio_id, gpio_func);
	if (status != HALMAC_RET_SUCCESS)
		return status;

	status = halmac_pinmux_record_88xx(pHalmac_adapter, gpio_func, 1);
	if (status != HALMAC_RET_SUCCESS)
		return status;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_pinmux_set_func_8822b <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_pinmux_free_func_8822b() -free locked gpio function
 * @pHalmac_adapter : the adapter of halmac
 * @gpio_func : gpio function
 * Author : Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_pinmux_free_func_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_GPIO_FUNC gpio_func
)
{
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;
	VOID *pDriver_adapter = NULL;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_pinmux_free_func_8822b ==========>\n");

	switch (gpio_func) {
	case HALMAC_GPIO_FUNC_SW_IO_0:
		pHalmac_adapter->pinmux_info.sw_io_0 = 0;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_1:
		pHalmac_adapter->pinmux_info.sw_io_1 = 0;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_2:
		pHalmac_adapter->pinmux_info.sw_io_2 = 0;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_3:
		pHalmac_adapter->pinmux_info.sw_io_3 = 0;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_4:
	case HALMAC_GPIO_FUNC_SDIO_INT:
		pHalmac_adapter->pinmux_info.sw_io_4 = 0;
		pHalmac_adapter->pinmux_info.sdio_int = 0;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_5:
		pHalmac_adapter->pinmux_info.sw_io_5 = 0;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_6:
		pHalmac_adapter->pinmux_info.sw_io_6 = 0;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_7:
		pHalmac_adapter->pinmux_info.sw_io_7 = 0;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_8:
	case HALMAC_GPIO_FUNC_WL_LED:
		pHalmac_adapter->pinmux_info.sw_io_8 = 0;
		pHalmac_adapter->pinmux_info.wl_led = 0;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_9:
		pHalmac_adapter->pinmux_info.sw_io_9 = 0;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_10:
		pHalmac_adapter->pinmux_info.sw_io_10 = 0;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_11:
		pHalmac_adapter->pinmux_info.sw_io_11 = 0;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_12:
		pHalmac_adapter->pinmux_info.sw_io_12 = 0;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_13:
		pHalmac_adapter->pinmux_info.sw_io_13 = 0;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_14:
		pHalmac_adapter->pinmux_info.sw_io_14 = 0;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_15:
		pHalmac_adapter->pinmux_info.sw_io_15 = 0;
		break;
	default:
		return HALMAC_RET_SWITCH_CASE_ERROR;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]func : %X\n", gpio_func);
	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_pinmux_free_func_8822b <==========\n");

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_get_pinmux_list_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_GPIO_FUNC gpio_func,
	OUT const HALMAC_GPIO_PIMUX_LIST **ppPinmux_list,
	OUT u32 *pList_size,
	OUT u32 *pGpio_id
)
{
	switch (gpio_func) {
	case HALMAC_GPIO_FUNC_SW_IO_0:
		*ppPinmux_list = PIMUX_LIST_GPIO0_8822B;
		*pList_size = ARRAY_SIZE(PIMUX_LIST_GPIO0_8822B);
		*pGpio_id = HALMAC_GPIO0;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_1:
		*ppPinmux_list = PIMUX_LIST_GPIO1_8822B;
		*pList_size = ARRAY_SIZE(PIMUX_LIST_GPIO1_8822B);
		*pGpio_id = HALMAC_GPIO1;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_2:
		*ppPinmux_list = PIMUX_LIST_GPIO2_8822B;
		*pList_size = ARRAY_SIZE(PIMUX_LIST_GPIO2_8822B);
		*pGpio_id = HALMAC_GPIO2;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_3:
		*ppPinmux_list = PIMUX_LIST_GPIO3_8822B;
		*pList_size = ARRAY_SIZE(PIMUX_LIST_GPIO3_8822B);
		*pGpio_id = HALMAC_GPIO3;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_4:
	case HALMAC_GPIO_FUNC_SDIO_INT:
		*ppPinmux_list = PIMUX_LIST_GPIO4_8822B;
		*pList_size = ARRAY_SIZE(PIMUX_LIST_GPIO4_8822B);
		*pGpio_id = HALMAC_GPIO4;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_5:
		*ppPinmux_list = PIMUX_LIST_GPIO5_8822B;
		*pList_size = ARRAY_SIZE(PIMUX_LIST_GPIO5_8822B);
		*pGpio_id = HALMAC_GPIO5;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_6:
		*ppPinmux_list = PIMUX_LIST_GPIO6_8822B;
		*pList_size = ARRAY_SIZE(PIMUX_LIST_GPIO6_8822B);
		*pGpio_id = HALMAC_GPIO6;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_7:
		*ppPinmux_list = PIMUX_LIST_GPIO7_8822B;
		*pList_size = ARRAY_SIZE(PIMUX_LIST_GPIO7_8822B);
		*pGpio_id = HALMAC_GPIO7;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_8:
	case HALMAC_GPIO_FUNC_WL_LED:
		*ppPinmux_list = PIMUX_LIST_GPIO8_8822B;
		*pList_size = ARRAY_SIZE(PIMUX_LIST_GPIO8_8822B);
		*pGpio_id = HALMAC_GPIO8;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_9:
		*ppPinmux_list = PIMUX_LIST_GPIO9_8822B;
		*pList_size = ARRAY_SIZE(PIMUX_LIST_GPIO9_8822B);
		*pGpio_id = HALMAC_GPIO9;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_10:
		*ppPinmux_list = PIMUX_LIST_GPIO10_8822B;
		*pList_size = ARRAY_SIZE(PIMUX_LIST_GPIO10_8822B);
		*pGpio_id = HALMAC_GPIO10;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_11:
		*ppPinmux_list = PIMUX_LIST_GPIO11_8822B;
		*pList_size = ARRAY_SIZE(PIMUX_LIST_GPIO11_8822B);
		*pGpio_id = HALMAC_GPIO11;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_12:
		*ppPinmux_list = PIMUX_LIST_GPIO12_8822B;
		*pList_size = ARRAY_SIZE(PIMUX_LIST_GPIO12_8822B);
		*pGpio_id = HALMAC_GPIO12;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_13:
		*ppPinmux_list = PIMUX_LIST_GPIO13_8822B;
		*pList_size = ARRAY_SIZE(PIMUX_LIST_GPIO13_8822B);
		*pGpio_id = HALMAC_GPIO13;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_14:
		*ppPinmux_list = PIMUX_LIST_GPIO14_8822B;
		*pList_size = ARRAY_SIZE(PIMUX_LIST_GPIO14_8822B);
		*pGpio_id = HALMAC_GPIO14;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_15:
		*ppPinmux_list = PIMUX_LIST_GPIO15_8822B;
		*pList_size = ARRAY_SIZE(PIMUX_LIST_GPIO15_8822B);
		*pGpio_id = HALMAC_GPIO15;
		break;
	default:
		return HALMAC_RET_SWITCH_CASE_ERROR;
	}

	return HALMAC_RET_SUCCESS;
}

static HALMAC_RET_STATUS
halmac_chk_pinmux_valid_8822b(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_GPIO_FUNC gpio_func
)
{
	HALMAC_RET_STATUS status = HALMAC_RET_SUCCESS;
	VOID *pDriver_adapter = NULL;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;

	switch (gpio_func) {
	case HALMAC_GPIO_FUNC_SW_IO_0:
		if (pHalmac_adapter->pinmux_info.sw_io_0 == 1)
			status = HALMAC_RET_PINMUX_USED;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_1:
		if (pHalmac_adapter->pinmux_info.sw_io_1 == 1)
			status = HALMAC_RET_PINMUX_USED;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_2:
		if (pHalmac_adapter->pinmux_info.sw_io_2 == 1)
			status = HALMAC_RET_PINMUX_USED;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_3:
		if (pHalmac_adapter->pinmux_info.sw_io_3 == 1)
			status = HALMAC_RET_PINMUX_USED;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_4:
	case HALMAC_GPIO_FUNC_SDIO_INT:
		if ((pHalmac_adapter->pinmux_info.sw_io_4 == 1) || (pHalmac_adapter->pinmux_info.sdio_int == 1))
			status = HALMAC_RET_PINMUX_USED;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_5:
		if (pHalmac_adapter->pinmux_info.sw_io_5 == 1)
			status = HALMAC_RET_PINMUX_USED;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_6:
		if (pHalmac_adapter->pinmux_info.sw_io_6 == 1)
			status = HALMAC_RET_PINMUX_USED;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_7:
		if (pHalmac_adapter->pinmux_info.sw_io_7 == 1)
			status = HALMAC_RET_PINMUX_USED;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_8:
	case HALMAC_GPIO_FUNC_WL_LED:
		if ((pHalmac_adapter->pinmux_info.sw_io_8 == 1) || (pHalmac_adapter->pinmux_info.wl_led == 1))
			status = HALMAC_RET_PINMUX_USED;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_9:
		if (pHalmac_adapter->pinmux_info.sw_io_9 == 1)
			status = HALMAC_RET_PINMUX_USED;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_10:
		if (pHalmac_adapter->pinmux_info.sw_io_10 == 1)
			status = HALMAC_RET_PINMUX_USED;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_11:
		if (pHalmac_adapter->pinmux_info.sw_io_11 == 1)
			status = HALMAC_RET_PINMUX_USED;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_12:
		if (pHalmac_adapter->pinmux_info.sw_io_12 == 1)
			status = HALMAC_RET_PINMUX_USED;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_13:
		if (pHalmac_adapter->pinmux_info.sw_io_13 == 1)
			status = HALMAC_RET_PINMUX_USED;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_14:
		if (pHalmac_adapter->pinmux_info.sw_io_14 == 1)
			status = HALMAC_RET_PINMUX_USED;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_15:
		if (pHalmac_adapter->pinmux_info.sw_io_15 == 1)
			status = HALMAC_RET_PINMUX_USED;
		break;
	default:
		return HALMAC_RET_SWITCH_CASE_ERROR;
	}

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]chk_pinmux_valid func : %X status : %X\n",
									gpio_func, status);

	return status;
}

#endif /* HALMAC_8822B_SUPPORT */
