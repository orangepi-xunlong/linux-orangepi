/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _SOPHGO_COMMON_H_
#define _SOPHGO_COMMON_H_

#include <linux/platform_device.h>
#include <linux/netdevice.h>
#include <linux/workqueue.h>
#include <linux/types.h>

#include "sophgo_pt.h"
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef u64 dma_addr_t;

#define HOST_READY_FLAG 0x01234567
#define DEVICE_READY_FLAG 0x89abcde

#define PT_ALIGN 8
#define VETH_DEFAULT_MTU 65536

#define SHM_HOST_PHY_OFFSET 0x00
#define SHM_HOST_LEN_OFFSET 0x08
#define SHM_HOST_HEAD_OFFSET 0x10
#define SHM_HOST_TAIL_OFFSET 0x14
#define SHM_HANDSHAKE_OFFSET 0x40

#define VETH_IPADDR_OFFSET			0x44
#define VETH_A53_STATE_OFFSET		0x48
#define VETH_GW_OFFSET				0x4c
#define VETH_RESET_OFFSET			0x50
#define VETH_MASK_OFFSET			0x54
#define MIXMODE_CHIP_TEMP_OFFSET	0x58
#define MIXMODE_BOARD_TEMP_OFFSET	0x5c


#define VETH_CTRL_STATUS			0x7C

#define TOP_MISC_GP_REG30_SET_OFFSET 0xF8
#define TOP_MISC_GP_REG30_CLR_OFFSET 0x78

#define TOP_MISC_GP_REG31_SET_OFFSET 0xFC
#define TOP_MISC_GP_REG31_CLR_OFFSET 0x7C

#define SOPH_VETH_RX_IRQ_CLR_BIT BIT(2)
#define SOPH_VETH_TX_IRQ_SET_BIT BIT(12)

#define SOPH_VETH_CTRL_HOST_UPDATE BIT(0)

struct veth_dev {
	struct platform_device *pdev;

	void __iomem *shm_cfg_reg;
	void __iomem *top_misc_reg;
	void __iomem *cdma_cfg_reg;
	void __iomem *intc_cfg_reg;

	struct resource *shm_cfg_res;
	struct resource *top_misc_res;
	struct resource *cdma_cfg_res;
	struct resource *intc_cfg_res;

	int rx_irq;
	atomic_t link;
	struct pt *pt;
	/* lock for operate cdma */
	spinlock_t lock_cdma;

	struct work_struct net_state_worker;
	struct napi_struct napi;
	struct net_device *ndev;
};

void sg_write32(void __iomem *base, u32 offset, u32 value);
u32 sg_read32(void __iomem *base, u32 offset);
#endif
