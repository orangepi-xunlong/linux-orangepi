/*
 * nand_base.h for  SUNXI NAND .
 *
 * Copyright (C) 2016 Allwinner.
 *
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef _NAND_BASE_H_
#define _NAND_BASE_H_

#include "nand_blk.h"
//#include "nand_dev.h"

/*****************************************************************************/

#ifdef CONFIG_AW_RAWNAND_CD
/**
 * struct nand_slot - nand slot functions
 *
 * @cd_irq:		nand slot hotplug detection IRQ or -EINVAL
 * @handler_priv:	nand slot context
 *
 * some soc not the rtc detect power down scene, we detect via a external device,
 * which send a singal to a gpio, the gpio must be not used by other module.
 *
 */
struct nand_slot {
	int cd_irq;
	bool cd_wake_enabled;
	void *handler_priv;
};

struct nand_gpio {
/*	struct gpio_desc *ro_gpio;*/
	struct gpio_desc *cd_gpio;
	bool override_cd_active_level;
	irqreturn_t (*cd_gpio_isr)(int irq, void *dev_id);
/*	char *ro_label;*/
	char *cd_label;
	u32 cd_debounce_delay_ms;
};

#endif

/*--> FOR TEMP <--*/
#define NAND_STORAGE_TYPE_NULL 0
#define NAND_STORAGE_TYPE_RAWNAND 1
#define NAND_STORAGE_TYPE_SPINAND 2

struct sunxi_ndfc {
	struct nand_controller_info *nctri;
	struct device *dev;
	struct pinctrl *p;
	struct dma_chan *dma_hdl;
	struct clk *pclk;/*pll clock*/
	struct clk *mdclk; /*nand module clock*/
	struct clk *mcclk; /*nand ecc engine clock*/
	struct clk *busclk;
	struct clk *mbusclk;
	struct reset_control *rst;

	struct regulator *regu1; /*vcc-nand*/
	struct regulator *regu2; /*vcc-io*/

	/*sysfs node*/
	struct device_attribute sunxi_nand_debug;
	struct _nftl_blk *nftl_blk;
#ifdef CONFIG_AW_RAWNAND_CD
	struct nand_slot		slot;
#endif

	spinlock_t nand_int_lock;
};

extern struct sunxi_ndfc aw_ndfc;

extern int nand_type;

extern struct nand_blk_ops mytr;
extern struct _nand_info *p_nand_info;

extern int init_blklayer(void);
extern int init_blklayer_for_dragonboard(void);
extern void exit_blklayer(void);
extern void set_cache_level(struct _nand_info *nand_info,
			    unsigned short cache_level);
extern void set_capacity_level(struct _nand_info *nand_info,
			       unsigned short capacity_level);
extern int nand_wait_rb_mode(void);
extern int nand_wait_dma_mode(void);
extern void do_nand_interrupt(unsigned int no);
extern void print_nftl_zone(void *zone);
extern int nand_get_dragon_board_flag(struct sunxi_ndfc *ndfc);
extern int nand_thread(void *arg);
extern int nand_check_boot(void);

extern int nand_print_dbg(const char *fmt, ...);
extern __u32 nand_get_max_channel_cnt(void);

extern int nand_clean_zone_table(void *p);
extern int nand_find_zone_table(void *p);
extern unsigned int nftl_get_boot_cnt(void *_zone);
int test_mbr(uchar *data);
#define BLK_ERR_MSG_ON

#endif
