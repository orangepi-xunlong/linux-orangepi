/*
 * nand_blk.h for  SUNXI NAND .
 *
 * Copyright (C) 2016 Allwinner.
 *
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#ifndef __NAND_BLK_H__
#define __NAND_BLK_H__

#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/blkpg.h>
#include <linux/spinlock.h>
#include <linux/hdreg.h>
#include <linux/init.h>
#include <linux/semaphore.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/consumer.h>
#include <linux/gpio.h>
#include <asm/processor.h>
#include <linux/spinlock.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <asm/cacheflush.h>
#include <linux/pm.h>
#include <linux/kthread.h>
#include "nand_type.h"
#include "nand_lib.h"
#include <linux/scatterlist.h>
#include <linux/vmalloc.h>
#include <linux/freezer.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/mii.h>
#include <linux/skbuff.h>
#include <linux/irqreturn.h>
#include <linux/device.h>
#include <linux/pagemap.h>
#include <linux/mutex.h>
#include <linux/reboot.h>
#include <linux/kmod.h>
#include <linux/of_address.h>
#include <linux/of.h>
#include <linux/time.h>
#include "nand_boot.h"
#include "nand_dev.h"
#include "nand_type.h"

#if defined CONFIG_ARCH_SUN8IW1P1
#define  SUN8IW1P1
#elif defined CONFIG_ARCH_SUN8IW3P1
#define  SUN8IW3P1
#elif defined CONFIG_ARCH_SUN9IW1P1
#define  SUN9IW1P1
#elif defined CONFIG_ARCH_SUN8IW5P1
#define  SUN8IW5P1
#elif defined CONFIG_ARCH_SUN8IW6P1
#define  SUN8IW6P1
#elif defined CONFIG_ARCH_SUN50I
#define  SUN50IW1P1
#elif defined CONFIG_ARCH_SUN8IW10P1
#define  SUN8IW10P1
#elif defined CONFIG_ARCH_SUN8IW11P1
#define  SUN8IW11P1
#elif defined CONFIG_ARCH_SUN50IW2P1
#define  SUN50IW2P1
#elif defined CONFIG_ARCH_SUN8IW7P1
#define  SUN8IW7P1
#else
#error "please select a platform\n"
#endif

#define __FPGA_TEST__
#define __LINUX_NAND_SUPPORT_INT__
#define __LINUX_SUPPORT_DMA_INT__
#define __LINUX_SUPPORT_RB_INT__

#define BLK_ERR_MSG_ON
#ifdef BLK_ERR_MSG_ON
#define nand_dbg_err(fmt, args...) printk("[NAND]"fmt, ## args)
#else
#define nand_dbg_err(fmt, ...)  ({})
#endif

#ifdef BLK_INFO_MSG_ON
#define nand_dbg_inf(fmt, args...) printk("[NAND]"fmt, ## args)
#else
#define nand_dbg_inf(fmt, ...)  ({})
#endif
extern int nand_gpt_enable;
extern struct device *ndfc_dev;
#define NAND_SUPPORT_GPT (nand_gpt_enable)

struct nand_blk_ops;
struct list_head;
struct semaphore;
struct hd_geometry;

struct nand_blk_dev {
	struct nand_blk_ops *nandr;
	struct list_head list;
	struct device dev;
	struct mutex lock;
	struct gendisk *disk;
	struct kref ref;
	void *priv;
	struct class dev_class;

	unsigned char heads;
	unsigned char sectors;
	unsigned short cylinders;

	int open;
	int devnum;
	unsigned long size;
	unsigned long off_size;
	int readonly;
	int writeonly;
	int disable_access;
	char name[32];
};

struct _nand_dev {
	struct nand_blk_dev nbd;
	char name[24];
	unsigned int offset;
	unsigned int size;
	struct _nftl_blk *nftl_blk;
	struct _nand_dev *nand_dev_next;

	struct mutex dev_lock;

	int (*read_data)(struct _nand_dev *nand_dev, unsigned int sector,
			 unsigned int len, unsigned char *buf);
	int (*write_data)(struct _nand_dev *nand_dev, unsigned int sector,
			 unsigned int len, unsigned char *buf);
	int (*flush_write_cache)(struct _nand_dev *nand_dev,
			 unsigned int num);
	int (*discard)(struct _nand_dev *nand_dev, unsigned int sector,
			 unsigned int len);
	int (*flush_sector_write_cache)(struct _nand_dev *nand_dev,
			 unsigned int num);
};

struct nand_blk_ops {
	/* blk device ID */
	char name[32];
	int major;
	int minorbits;
	int blksize;
	int blkshift;

	struct task_struct *thread;
	struct request_queue *rq;
	spinlock_t queue_lock;

	/* add/remove nandflash devparts,use gendisk */
	int (*add_dev)(struct nand_blk_ops *tr,
			struct _nand_phy_partition *phy_partition);
	int (*add_dev_test)(struct nand_blk_ops *tr);
	int (*remove_dev)(struct nand_blk_ops *tr);

	/* Block layer ioctls */
	int (*getgeo)(struct nand_blk_dev *dev, struct hd_geometry *geo);
	int (*flush)(struct nand_blk_dev *dev);

	/* Called with mtd_table_mutex held; no race with add/remove */
	int (*open)(struct nand_blk_dev *dev);
	int (*release)(struct nand_blk_dev *dev);

	struct _nftl_blk nftl_blk_head;
	struct _nand_dev nand_dev_head;

	/* synchronization variable */
	struct completion thread_exit;
	int quit;
	wait_queue_head_t thread_wq;
	struct semaphore nand_ops_mutex;
	struct list_head devs;
	int bg_stop;
	int rq_null;

	struct module *owner;
};
/*****************************************************************************/

extern struct nand_blk_ops mytr;
extern struct _nand_info *p_nand_info;

extern int init_blklayer(void);
extern int init_blklayer_for_dragonboard(void);
extern void exit_blklayer(void);
extern void set_cache_level(struct _nand_info *nand_info,
			    unsigned short cache_level);
extern void set_capacity_level(struct _nand_info *nand_info,
			       unsigned short capacity_level);
extern __u32 nand_wait_rb_mode(void);
extern __u32 nand_wait_dma_mode(void);
extern void do_nand_interrupt(unsigned int no);
extern void print_nftl_zone(void *zone);
extern int nand_before_shutdown(void *zone);
extern int NAND_get_storagetype(void);
extern int NAND_Get_Dragonboard_Flag(void);
extern int nand_thread(void *arg);
extern int NAND_CheckBoot(void);

int test_mbr(uchar *data);
extern int NAND_Print_DBG(const char *fmt, ...);
extern __u32 NAND_GetMaxChannelCnt(void);

extern int get_nand_secure_storage_max_item(void);
extern int nand_secure_storage_read(int item, unsigned char *buf,
				    unsigned int len);
extern int nand_secure_storage_write(int item, unsigned char *buf,
				     unsigned int len);

extern int NAND_ReadBoot0(unsigned int length, void *buf);
extern int NAND_ReadBoot1(unsigned int length, void *buf);
extern int NAND_BurnBoot0(unsigned int length, void *buf);
extern int NAND_BurnBoot1(unsigned int length, void *buf);

extern struct _nand_info *p_nand_info;

extern int add_nand(struct nand_blk_ops *tr,
		    struct _nand_phy_partition *phy_partition);
extern int add_nand_for_dragonboard_test(struct nand_blk_ops *tr);
extern int remove_nand(struct nand_blk_ops *tr);
extern int nand_flush(struct nand_blk_dev *dev);
extern struct _nand_phy_partition *get_head_phy_partition_from_nand_info( \
		struct _nand_info *nand_info);
extern struct _nand_phy_partition *get_next_phy_partition( \
		struct  _nand_phy_partition *phy_partition);

extern int debug_data;

extern uint32 gc_all(void *zone);
extern uint32 gc_one(void *zone);
extern uint32 prio_gc_one(void *zone, uint16 block, uint32 flag);
//tern void print_nftl_zone(void *zone);
extern void print_free_list(void *zone);
extern void print_smart(void *zone);
extern void print_block_invalid_list(void *zone);
extern void print_logic_page_map(void *_zone, uint32 num);
extern uint32 nftl_set_zone_test(void *_zone, uint32 num);
extern int nand_dbg_phy_read(unsigned short nDieNum, unsigned short nBlkNum,
			     unsigned short nPage);
extern int nand_dbg_zone_phy_read(void *zone, uint16 block, uint16 page);
extern int nand_dbg_zone_erase(void *zone, uint16 block, uint16 erase_num);
extern int nand_dbg_zone_phy_write(void *zone, uint16 block, uint16 page);
extern int nand_dbg_phy_write(unsigned short nDieNum, unsigned short nBlkNum,
			      unsigned short nPage);
extern int nand_dbg_phy_erase(unsigned short nDieNum, unsigned short nBlkNum);
extern int nand_dbg_single_phy_erase(unsigned short nDieNum,
				     unsigned short nBlkNum);
extern int _dev_nand_read2(char *name, __u32 start_sector, __u32 len,
			   unsigned char *buf);

extern void nand_phy_test(void);
extern int nand_check_table(void *zone);

extern void udisk_test_start(struct _nftl_blk *nftl_blk);
extern void udisk_test_stop(void);

extern int _dev_nand_read2(char *name, unsigned int start_sector,
			   unsigned int len, unsigned char *buf);
extern int _dev_nand_write2(char *name, unsigned int start_sector,
			    unsigned int len, unsigned char *buf);

extern struct nand_blk_ops mytr;
extern struct kobj_type ktype;

extern struct _nand_partition *build_nand_partition(struct _nand_phy_partition
						    *phy_partition);
extern void add_nftl_blk_list(struct _nftl_blk *head,
			      struct _nftl_blk *nftl_blk);
extern struct _nftl_blk *del_last_nftl_blk(struct _nftl_blk *head);
extern int add_nand_blktrans_dev(struct nand_blk_dev *dev);
extern int add_nand_blktrans_dev_for_dragonboard(struct nand_blk_dev *dev);
extern int del_nand_blktrans_dev(struct nand_blk_dev *dev);
extern struct _nand_disk *get_disk_from_phy_partition(struct _nand_phy_partition
						      *phy_partition);
extern uint16 get_partitionNO(struct _nand_phy_partition *phy_partition);
extern int nftl_exit(struct _nftl_blk *nftl_blk);
extern int NAND_Print_DBG(const char *fmt, ...);
extern struct _nftl_blk *get_nftl_need_read_claim(struct _nftl_blk *start_blk);
extern int read_reclaim(struct _nftl_blk *start_blk, struct _nftl_blk *nftl_blk,
			uchar *buf);

extern void *NDFC0_BASE_ADDR;
extern void *NDFC1_BASE_ADDR;
extern struct device *ndfc_dev;

extern int NAND_Print(const char *fmt, ...);
extern int NAND_PhysicLock(void);
extern int NAND_PhysicUnLock(void);

extern int nand_get_param(boot_nand_para_t *nand_param);

extern int nand_read_nboot_data(unsigned char *buf, unsigned int len);
extern int nand_read_uboot_data(unsigned char *buf, unsigned int len);
extern int nand_write_nboot_data(unsigned char *buf, unsigned int len);
extern int nand_write_uboot_data(unsigned char *buf, unsigned int len);
extern int nand_dragonborad_test_one(unsigned char *buf, unsigned char *oob,
				     unsigned int blk_num);
extern int NAND_IS_Secure_sys(void);
extern int nand_check_uboot(unsigned char *buf, unsigned int len);
extern int nand_get_uboot_total_len(void);

//extern uint32 gc_all(void *zone);
//extern uint32 gc_one(void *zone);
extern uint32 prio_gc_one(void *zone, uint16 block, uint32 flag);
//extern void print_nftl_zone(void *zone);
//extern void print_free_list(void *zone);
//extern void print_block_invalid_list(void *zone);
//extern uint32 nftl_set_zone_test(void *_zone, uint32 num);
extern int nand_dbg_phy_read(unsigned short nDieNum, unsigned short nBlkNum,
			     unsigned short nPage);
extern int nand_dbg_zone_phy_read(void *zone, uint16 block, uint16 page);
extern int nand_dbg_zone_erase(void *zone, uint16 block, uint16 erase_num);
extern int nand_dbg_zone_phy_write(void *zone, uint16 block, uint16 page);
extern int nand_dbg_phy_write(unsigned short nDieNum, unsigned short nBlkNum,
			      unsigned short nPage);
extern int nand_dbg_phy_erase(unsigned short nDieNum, unsigned short nBlkNum);
extern int _dev_nand_read2(char *name, __u32 start_sector, __u32 len,
			   unsigned char *buf);
extern void nand_phy_test(void);
extern int nand_check_table(void *zone);

extern int _dev_nand_read(struct _nand_dev *nand_dev, __u32 start_sector,
			  __u32 len, unsigned char *buf);
extern int _dev_nand_write(struct _nand_dev *nand_dev, __u32 start_sector,
			   __u32 len, unsigned char *buf);
extern int _dev_flush_write_cache(struct _nand_dev *nand_dev, __u32 num);
extern struct _nand_dev *_get_nand_dev_by_name(char *name);

//extern struct _nand_info *p_nand_info;
//extern struct kobj_type ktype;
extern struct _nftl_blk nftl_blk_head;

//tern struct _nand_partition *build_nand_partition(struct _nand_phy_partition *phy_partition);
//extern void add_nftl_blk_list(struct _nftl_blk *head, struct  _nftl_blk *nftl_blk);
//extern uint16 get_partitionNO(struct _nand_phy_partition *phy_partition);
//extern struct _nand_phy_partition *get_head_phy_partition_from_nand_info(struct _nand_info *nand_info);
//extern struct _nand_phy_partition *get_next_phy_partition(struct _nand_phy_partition *phy_partition);

//extern uint32 nand_wait_rb_mode(void);
//extern uint32 nand_wait_dma_mode(void);
//extern void do_nand_interrupt(unsigned int no);

extern int nand_ftl_exit(void);
extern unsigned int nftl_read(unsigned int start_sector, unsigned int len, unsigned char *buf);
extern unsigned int nftl_write(unsigned int start_sector, unsigned int len, unsigned char *buf);
extern unsigned int nftl_flush_write_cache(void);

//extern uint32 gc_all(struct _nftl_zone *zone);
//extern uint32 gc_one(struct _nftl_zone *zone);
//extern void print_nftl_zone(struct _nftl_zone *zone);
//extern void print_free_list(struct _nftl_zone *zone);
//extern void print_block_invalid_list(struct _nftl_zone *zone);
//extern uint32 nftl_set_zone_test(void *_zone, uint32 num);
extern void obj_test_release(struct kobject *kobject);
extern void udisk_test_speed(struct _nftl_blk *nftl_blk);
#endif
