/**
 ******************************************************************************
 *
 * @file bl_platorm.h
 *
 * Copyright (C) BouffaloLab 2017-2018
 *
 ******************************************************************************
 */

#ifndef _BL_PLAT_H_
#define _BL_PLAT_H_

#include <linux/mmc/sdio.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>

#define BL_CONFIG_FW_NAME             "bl_settings.ini"
#define BL_PHY_CONFIG_TRD_NAME        "bl_trident.ini"
#define BL_PHY_CONFIG_KARST_NAME      "bl_karst.ini"
#define BL_AGC_FW_NAME                "agcram.bin"
#define BL_LDPC_RAM_NAME              "ldpcram.bin"
#define BL_MAC_FW_NAME               "fmacfw.bin"

#define BL_FCU_FW_NAME                "fcuram.bin"

/**
 * Type of memory to access (cf bl_plat.get_address)
 *
 * @BL_ADDR_CPU To access memory of the embedded CPU
 * @BL_ADDR_SYSTEM To access memory/registers of one subsystem of the
 * embedded system
 *
 */
enum bl_platform_addr {
    BL_ADDR_CPU,
    BL_ADDR_SYSTEM,
    BL_ADDR_MAX,
};

struct bl_hw;

/**
 * @pci_dev pointer to pci dev
 * @enabled Set if embedded platform has been enabled (i.e. fw loaded and
 *          ipc started)
 * @enable Configure communication with the fw (i.e. configure the transfers
 *         enable and register interrupt)
 * @disable Stop communication with the fw
 * @deinit Free all ressources allocated for the embedded platform
 * @get_address Return the virtual address to access the requested address on
 *              the platform.
 * @ack_irq Acknowledge the irq at link level.
 *
 * @priv Private data for the link driver
 */
struct bl_plat {
	struct sdio_func *func;
	bool enabled;

	u8 *mp_regs;
	u32 mp_rd_bitmap;
	u32 mp_wr_bitmap;
	
	u8 curr_rd_port;
	u8 curr_wr_port;

	u32 io_port;

    u8 priv[0] __aligned(sizeof(void *));
};

int bl_platform_init(struct bl_plat *bl_plat, void **platform_data);
void bl_platform_deinit(struct bl_hw *bl_hw);

int bl_platform_on(struct bl_hw *bl_hw);
void bl_platform_off(struct bl_hw *bl_hw);

int bl_platform_register_drv(void);
void bl_platform_unregister_drv(void);

static inline struct device *bl_platform_get_dev(struct bl_plat *bl_plat)
{
    return &(bl_plat->func->dev);
}

#endif /* _BL_PLAT_H_ */
