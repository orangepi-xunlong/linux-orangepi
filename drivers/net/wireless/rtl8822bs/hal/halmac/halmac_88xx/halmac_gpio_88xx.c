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

#include "halmac_gpio_88xx.h"

#if HALMAC_88XX_SUPPORT

/**
 * halmac_pinmux_wl_led_mode_88xx() -control wlan led gpio function
 * @pHalmac_adapter : the adapter of halmac
 * @wlled_mode : wlan led mode
 * Author : Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_pinmux_wl_led_mode_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_WLLED_MODE wlled_mode
)
{
	u8 value8;
	PHALMAC_API pHalmac_api;
	VOID *pDriver_adapter = NULL;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_pinmux_wl_led_mode_88xx ==========>\n");

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_LED_CFG + 2);
	value8 &= ~(BIT(6));
	value8 |= BIT(3);
	value8 &= ~(BIT(0) | BIT(1) | BIT(2));

	switch (wlled_mode) {
	case HALMAC_WLLED_MODE_TRX:
		value8 |= 2;
		break;
	case HALMAC_WLLED_MODE_TX:
		value8 |= 4;
		break;
	case HALMAC_WLLED_MODE_RX:
		value8 |= 6;
		break;
	case HALMAC_WLLED_MODE_SW_CTRL:
		value8 |= 0;
		break;
	default:
		return HALMAC_RET_SWITCH_CASE_ERROR;
	}

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_LED_CFG + 2, value8);

	PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_TRACE, "[TRACE]halmac_pinmux_wl_led_mode_88xx <==========\n");

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_pinmux_wl_led_sw_ctrl_88xx() -control wlan led on/off
 * @pHalmac_adapter : the adapter of halmac
 * @led_on : on(1), off(0)
 * Author : Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
VOID
halmac_pinmux_wl_led_sw_ctrl_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 led_on
)
{
	u8 value8;
	PHALMAC_API pHalmac_api;

	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_LED_CFG + 2);
	value8 = (led_on == 0) ? value8 | BIT(3) : value8 & ~(BIT(3));

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_LED_CFG + 2, value8);
}

/**
 * halmac_pinmux_sdio_int_polarity_88xx() -control sdio int polarity
 * @pHalmac_adapter : the adapter of halmac
 * @low_active : low active(1), high active(0)
 * Author : Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
VOID
halmac_pinmux_sdio_int_polarity_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 low_active
)
{
	u8 value8;
	PHALMAC_API pHalmac_api;

	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, REG_SYS_SDIO_CTRL + 2);
	value8 = (low_active == 0) ? value8 | BIT(3) : value8 & ~(BIT(3));

	HALMAC_REG_WRITE_8(pHalmac_adapter, REG_SYS_SDIO_CTRL + 2, value8);
}

/**
 * halmac_pinmux_gpio_mode_88xx() -control gpio io mode
 * @pHalmac_adapter : the adapter of halmac
 * @gpio_id : gpio0~15(0~15)
 * @output : output(1), input(0)
 * Author : Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_pinmux_gpio_mode_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 gpio_id,
	IN u8 output
)
{
	u16 value16;
	u8 in_out;
	u32 offset;
	PHALMAC_API pHalmac_api;

	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	if (gpio_id <= 7)
		offset = REG_GPIO_PIN_CTRL + 2;
	else if (gpio_id >= 8 && gpio_id <= 15)
		offset = REG_GPIO_EXT_CTRL + 2;
	else
		return HALMAC_RET_WRONG_GPIO;

	in_out = (output == 0) ? 0 : 1;
	gpio_id &= (8 - 1);

	value16 = HALMAC_REG_READ_16(pHalmac_adapter, offset);
	value16 &= ~((1 << gpio_id) | (1 << gpio_id << 8));
	value16 |= (in_out << gpio_id);
	HALMAC_REG_WRITE_16(pHalmac_adapter, offset, value16);

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_pinmux_gpio_output_88xx() -control gpio output high/low
 * @pHalmac_adapter : the adapter of halmac
 * @gpio_id : gpio0~15(0~15)
 * @high : high(1), low(0)
 * Author : Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_pinmux_gpio_output_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 gpio_id,
	IN u8 high
)
{
	u8 value8;
	u8 hi_low;
	u32 offset;
	PHALMAC_API pHalmac_api;

	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	if (gpio_id <= 7)
		offset = REG_GPIO_PIN_CTRL + 1;
	else if (gpio_id >= 8 && gpio_id <= 15)
		offset = REG_GPIO_EXT_CTRL + 1;
	else
		return HALMAC_RET_WRONG_GPIO;

	hi_low = (high == 0) ? 0 : 1;
	gpio_id &= (8 - 1);

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, offset);
	value8 &= ~(1 << gpio_id);
	value8 |= (hi_low << gpio_id);
	HALMAC_REG_WRITE_8(pHalmac_adapter, offset, value8);

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_pinmux_status_88xx() -get current gpio status(high/low)
 * @pHalmac_adapter : the adapter of halmac
 * @pin_id : 0~15(0~15)
 * @phigh : high(1), low(0)
 * Author : Ivan Lin
 * Return : HALMAC_RET_STATUS
 * More details of status code can be found in prototype document
 */
HALMAC_RET_STATUS
halmac_pinmux_pin_status_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 pin_id,
	IN u8 *pHigh
)
{
	u8 value8;
	u32 offset;
	PHALMAC_API pHalmac_api;

	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	if (pin_id <= 7)
		offset = REG_GPIO_PIN_CTRL;
	else if (pin_id >= 8 && pin_id <= 15)
		offset = REG_GPIO_EXT_CTRL;
	else
		return HALMAC_RET_WRONG_GPIO;

	pin_id &= (8 - 1);

	value8 = HALMAC_REG_READ_8(pHalmac_adapter, offset);
	*pHigh = (value8 & (1 << pin_id)) >> pin_id;

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_pinmux_parser_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN const HALMAC_GPIO_PIMUX_LIST *pPinmux_list,
	IN u32 list_size,
	IN u32 gpio_id,
	OUT u32 *pCur_func
)
{
	u32 i;
	u8 value8;
	const HALMAC_GPIO_PIMUX_LIST *pCurr_func = pPinmux_list;
	HALMAC_GPIO_CFG_STATE *pGpio_state = &pHalmac_adapter->halmac_state.gpio_cfg_state;
	PHALMAC_API pHalmac_api;
	VOID *pDriver_adapter = NULL;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	if (*pGpio_state == HALMAC_GPIO_CFG_STATE_BUSY)
		return HALMAC_RET_BUSY_STATE;

	*pGpio_state = HALMAC_GPIO_CFG_STATE_BUSY;

	for (i = 0; i < list_size; i++) {
		if (gpio_id != pCurr_func->id) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_ERR, "[ERR]offset : %X, value : %X, func : %X\n",
							pCurr_func->offset, pCurr_func->value, pCurr_func->func);
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_ERR, "[ERR]id1 : %X, id2 : %X\n", gpio_id, pCurr_func->id);
			*pGpio_state = HALMAC_GPIO_CFG_STATE_IDLE;
			return HALMAC_RET_GET_PINMUX_ERR;
		}
		value8 = HALMAC_REG_READ_8(pHalmac_adapter, pCurr_func->offset);
		value8 &= pCurr_func->msk;
		if (value8 == pCurr_func->value) {
			*pCur_func = pCurr_func->func;
			break;
		}
		pCurr_func++;
	}

	*pGpio_state = HALMAC_GPIO_CFG_STATE_IDLE;

	if (i == list_size)
		return HALMAC_RET_GET_PINMUX_ERR;

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_pinmux_switch_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN const HALMAC_GPIO_PIMUX_LIST *pPinmux_list,
	IN u32 list_size,
	IN u32 gpio_id,
	IN HALMAC_GPIO_FUNC gpio_func
)
{
	u32 i;
	u8 value8;
	u16 switch_func;
	const HALMAC_GPIO_PIMUX_LIST *pCurr_func = pPinmux_list;
	HALMAC_GPIO_CFG_STATE *pGpio_state = &pHalmac_adapter->halmac_state.gpio_cfg_state;
	PHALMAC_API pHalmac_api;
	VOID *pDriver_adapter = NULL;

	pDriver_adapter = pHalmac_adapter->pDriver_adapter;
	pHalmac_api = (PHALMAC_API)pHalmac_adapter->pHalmac_api;

	if (*pGpio_state == HALMAC_GPIO_CFG_STATE_BUSY)
		return HALMAC_RET_BUSY_STATE;

	switch (gpio_func) {
	case HALMAC_GPIO_FUNC_WL_LED:
		switch_func = HALMAC_WL_LED;
		break;
	case HALMAC_GPIO_FUNC_SDIO_INT:
		switch_func = HALMAC_SDIO_INT;
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
		switch_func = HALMAC_SW_IO;
		break;
	default:
		return HALMAC_RET_SWITCH_CASE_ERROR;
	}

	for (i = 0; i < list_size; i++) {
		if (gpio_id != pCurr_func->id) {
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_ERR, "[ERR]offset : %X, value : %X, func : %X\n",
							pCurr_func->offset, pCurr_func->value, pCurr_func->func);
			PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_ERR, "[ERR]id1 : %X, id2 : %X\n", gpio_id, pCurr_func->id);
			return HALMAC_RET_GET_PINMUX_ERR;
		}

		if (switch_func == pCurr_func->func)
			break;

		pCurr_func++;
	}

	if (i == list_size) {
		PLATFORM_MSG_PRINT(pDriver_adapter, HALMAC_MSG_COMMON, HALMAC_DBG_ERR, "[ERR]gpio func error : %X %X\n", gpio_id, pCurr_func->id);
		return HALMAC_RET_GET_PINMUX_ERR;
	}

	*pGpio_state = HALMAC_GPIO_CFG_STATE_BUSY;

	pCurr_func = pPinmux_list;
	for (i = 0; i < list_size; i++) {
		value8 = HALMAC_REG_READ_8(pHalmac_adapter, pCurr_func->offset);
		value8 &= ~(pCurr_func->msk);

		if (switch_func == pCurr_func->func) {
			value8 |= (pCurr_func->value & pCurr_func->msk);
			HALMAC_REG_WRITE_8(pHalmac_adapter, pCurr_func->offset, value8);
			break;
		}

		value8 |= (~pCurr_func->value & pCurr_func->msk);
		HALMAC_REG_WRITE_8(pHalmac_adapter, pCurr_func->offset, value8);

		pCurr_func++;
	}

	*pGpio_state = HALMAC_GPIO_CFG_STATE_IDLE;

	return HALMAC_RET_SUCCESS;
}

HALMAC_RET_STATUS
halmac_pinmux_record_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_GPIO_FUNC gpio_func,
	IN u8 val
)
{
	switch (gpio_func) {
	case HALMAC_GPIO_FUNC_WL_LED:
		pHalmac_adapter->pinmux_info.wl_led = val;
		break;
	case HALMAC_GPIO_FUNC_SDIO_INT:
		pHalmac_adapter->pinmux_info.sdio_int = val;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_0:
		pHalmac_adapter->pinmux_info.sw_io_0 = val;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_1:
		pHalmac_adapter->pinmux_info.sw_io_1 = val;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_2:
		pHalmac_adapter->pinmux_info.sw_io_2 = val;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_3:
		pHalmac_adapter->pinmux_info.sw_io_3 = val;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_4:
		pHalmac_adapter->pinmux_info.sw_io_4 = val;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_5:
		pHalmac_adapter->pinmux_info.sw_io_5 = val;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_6:
		pHalmac_adapter->pinmux_info.sw_io_6 = val;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_7:
		pHalmac_adapter->pinmux_info.sw_io_7 = val;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_8:
		pHalmac_adapter->pinmux_info.sw_io_8 = val;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_9:
		pHalmac_adapter->pinmux_info.sw_io_9 = val;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_10:
		pHalmac_adapter->pinmux_info.sw_io_10 = val;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_11:
		pHalmac_adapter->pinmux_info.sw_io_11 = val;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_12:
		pHalmac_adapter->pinmux_info.sw_io_12 = val;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_13:
		pHalmac_adapter->pinmux_info.sw_io_13 = val;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_14:
		pHalmac_adapter->pinmux_info.sw_io_14 = val;
		break;
	case HALMAC_GPIO_FUNC_SW_IO_15:
		pHalmac_adapter->pinmux_info.sw_io_15 = val;
		break;
	default:
		return HALMAC_RET_GET_PINMUX_ERR;
	}

	return HALMAC_RET_SUCCESS;
}

#endif /* HALMAC_88XX_SUPPORT */

