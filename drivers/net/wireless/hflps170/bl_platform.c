/**
 ******************************************************************************
 *
 * @file bl_platform.c
 *
 * Copyright (C) BouffaloLab 2017-2018
 *
 ******************************************************************************
 */

#include <linux/module.h>
#include <linux/firmware.h>

#include "bl_platform.h"
#include "bl_irqs.h"
#include "hal_desc.h"
#include "bl_main.h"
#include "bl_sdio.h"

/**
 * bl_platform_on - Start the platform
 *
 * @bl_hw Main driver data
 *
 * It starts the platform :
 * - load fw and ucodes
 * - initialize IPC
 * - boot the fw
 * - enable link communication/IRQ
 *
 * Called by 802.11 part
 */
int bl_platform_on(struct bl_hw *bl_hw)
{
    struct bl_plat *bl_plat = bl_hw->plat;
    int ret;
    int count = 0;
	u8 fw_ready = 0;

    if (bl_plat->enabled)
        return 0;
	
    if ((ret = bl_ipc_init(bl_hw)))
        return ret;

	//BL_DBG("start firmware...\n");
	//bl_write_reg(bl_hw, 0x60, 1);
	//msleep(500);

    printk("Wait firmware...\n");
    do {
        bl_read_reg(bl_hw, 0x60, &fw_ready);
        printk("FW Mark %c:%x\n", fw_ready, fw_ready);
        count++;
        msleep(100);
    } while ('B' != fw_ready && count<50);//TODO put 'B' into header file
    msleep(500);

    if(count >= 50)
        return -1;


    bl_plat->enabled = true;

    return 0;
}

/**
 * bl_platform_off - Stop the platform
 *
 * @bl_hw Main driver data
 *
 * Called by 802.11 part
 */
void bl_platform_off(struct bl_hw *bl_hw)
{
    if (!bl_hw->plat->enabled)
        return;

    bl_ipc_deinit(bl_hw);
    bl_hw->plat->enabled = false;
}

/**
 * bl_platform_init - Initialize the platform
 *
 * @bl_plat platform data (already updated by platform driver)
 * @platform_data Pointer to store the main driver data pointer (aka bl_hw)
 *                That will be set as driver data for the platform driver
 * @return 0 on success, < 0 otherwise
 *
 * Called by the platform driver after it has been probed
 */
int bl_platform_init(struct bl_plat *bl_plat, void **platform_data)
{
    BL_DBG(BL_FN_ENTRY_STR);

    bl_plat->enabled = false;

    return bl_cfg80211_init(bl_plat, platform_data);
}

/**
 * bl_platform_deinit - Deinitialize the platform
 *
 * @bl_hw ain driver data
 *
 * Called by the platform driver after it is removed
 */
void bl_platform_deinit(struct bl_hw *bl_hw)
{
    BL_DBG(BL_FN_ENTRY_STR);

    bl_cfg80211_deinit(bl_hw);
}

/**
 * bl_platform_register_drv - Register all possible platform drivers
 */
int bl_platform_register_drv(void)
{
	return bl_sdio_register_drv();
}


/**
 * bl_platform_unregister_drv - Unegister all platform drivers
 */
void bl_platform_unregister_drv(void)
{
	return bl_sdio_unregister_drv();
}

MODULE_FIRMWARE(BL_AGC_FW_NAME);
MODULE_FIRMWARE(BL_FCU_FW_NAME);
MODULE_FIRMWARE(BL_LDPC_RAM_NAME);
MODULE_FIRMWARE(BL_MAC_FW_NAME);
