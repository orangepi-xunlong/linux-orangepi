/*
 * nand_dev.c for  SUNXI NAND .
 *
 * Copyright (C) 2016 Allwinner.
 *
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#define _NAND_DEV_C_

#include "nand_blk.h"
#include "nand_dev.h"


#define NAND_SCHEDULE_TIMEOUT  (HZ >> 1)
#define NFTL_SCHEDULE_TIMEOUT  (HZ >> 1)
#define NFTL_FLUSH_DATA_TIME	 1
#define WEAR_LEVELING 1

#define NAND_READ_RECLAIM_TIMEOUT  (864000)



static int dev_num;


unsigned long nand_active_time;
unsigned long nand_write_active_time;

struct nand_kobject *s_nand_kobj;
struct _nand_info *p_nand_info;
extern int flash_type;
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_thread(void *arg)
{
	unsigned long ret, time_val;
	struct nand_blk_ops *tr = (struct nand_blk_ops *)arg;

	unsigned int start_time = 600;
	unsigned char *temp_buf = kmalloc(64 * 1024, GFP_KERNEL);

	time_val = NAND_SCHEDULE_TIMEOUT;

	while (!kthread_should_stop()) {
		if (start_time != 0) {
			start_time--;
			goto nand_thread_exit;
		}
		ret =
		    nand_read_reclaim(tr->nftl_blk_head.nftl_blk_next,
				      temp_buf);

nand_thread_exit:
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(time_val);
	}

	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int tlc_nand_thread(void *arg)
{
	unsigned long ret = 0, time_val;
	struct nand_blk_ops *tr = (struct nand_blk_ops *)arg;

	unsigned int start_time = 150;

	time_val = NAND_SCHEDULE_TIMEOUT;

	while (!kthread_should_stop()) {
		if (start_time != 0) {
			start_time--;
			goto nand_thread_exit;
		}

	mutex_lock(tr->nftl_blk_head.nftl_blk_next->blk_lock);
		ret = tlc_nand_read_reclaim();
	mutex_unlock(tr->nftl_blk_head.nftl_blk_next->blk_lock);

	if (ret == 1) {
		start_time = NAND_READ_RECLAIM_TIMEOUT;/* 10 days */
		ret = 0;
		time_val = NAND_SCHEDULE_TIMEOUT << 1;
	}

nand_thread_exit:
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(time_val);
	}

	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
static int nftl_thread(void *arg)
{
	struct _nftl_blk *nftl_blk = arg;
	unsigned long time;
	unsigned long swl_time = jiffies;
	int first_miss_swl = 0;
	int need_swl = 0;
	int swl_done = 0;

	while (!kthread_should_stop()) {
		mutex_lock(nftl_blk->blk_lock);

		time = jiffies;
		if (flash_type == 1) {
				if (time_after((unsigned long)time, (unsigned long)(nand_write_active_time + HZ + HZ)) != 0)
					nftl_blk->flush_write_cache(nftl_blk, 2);

		static_wear_leveling(nftl_blk->nftl_zone);

#if WEAR_LEVELING

	/** add static WL:
		* we do the static WL when nftl ops is idle over 4s;
		* if missing the chance over 64s, we will do it mandatorily!!!
		* if we have done the static WL successfully, the time interval
		* to next static WL must be over 64s.
		*/
		time = jiffies;
		if (swl_done) {
			if (time_after(time, swl_time + (HZ << 6)))
				need_swl = 1;	/* next static WL is over 64s */
		} else if (time_after(time,
			 (unsigned long)(nftl_blk->ops_time + (HZ << 2)))) {
			/* nftl ops is idle over 4s */
			need_swl = 1;
		} else if (first_miss_swl) {
			if (time_after(time, swl_time + (HZ << 6)))
				need_swl = 1;	/* next static WL is over 64s */
		} else {
			first_miss_swl = 1;
			swl_time = jiffies;
		}

		if (need_swl) {
			first_miss_swl = 0;
			need_swl = 0;
			swl_done = do_static_wear_leveling(nftl_blk->nftl_zone);
			if (!swl_done) {
				swl_done = 1;
				swl_time = jiffies;
			} else
				swl_done = 0;
		}
#endif

		if (garbage_collect(nftl_blk->nftl_zone) != 0)
			nand_dbg_err("nftl_thread garbage_collect error!\n");

				if (do_prio_gc(nftl_blk->nftl_zone) != 0)
					nand_dbg_err("nftl_thread do_prio_gc error!\n");

		}

		mutex_unlock(nftl_blk->blk_lock);

		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(NFTL_SCHEDULE_TIMEOUT);
	}

	nftl_blk->nftl_thread = (void *)NULL;
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
void add_nand_dev_list(struct _nand_dev *head, struct _nand_dev *nand_dev)
{
	struct _nand_dev *p = head;

	nand_dev->nand_dev_next = NULL;
	while (p->nand_dev_next != NULL)
		p = p->nand_dev_next;

	p->nand_dev_next = nand_dev;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
struct _nand_dev *del_last_nand_dev(struct _nand_dev *head)
{
	struct _nand_dev *nand_dev = NULL;
	struct _nand_dev *p = head;

	while (p->nand_dev_next != NULL) {
		nand_dev = p->nand_dev_next;
		if (nand_dev->nand_dev_next == NULL) {
			p->nand_dev_next = NULL;
			return nand_dev;
		}
		p = p->nand_dev_next;
	}
	return NULL;
}

int nand_gpt_enable;
static unsigned int gpt_flag_parse(char *gpt_flag)
{
	long int ret;
	kstrtol(gpt_flag, 0, &ret);
	return ret;
}

static int __init early_gpt_flag(char *gpt_flag)
{
	nand_gpt_enable = gpt_flag_parse(gpt_flag);
	return 0;
}
early_param("gpt", early_gpt_flag);

static int get_gpt_para_from_cmdline(const char *cmdline, const char *name, char *value, int maxsize)
{
	char *p = (char *)cmdline;
	char *value_p = value;
	int size = 0;

	if (!cmdline || !name || !value)
		return -1;

	for (; *p != 0;) {
		if (*p++ == ' ') {
			if (strncmp(p, name, strlen(name)) == 0) {
				p += strlen(name);
				if (*p++ != '=')
					continue;
				while ((*p != 0) && (*p != ' ') && (++size < maxsize))
					*value_p++ = *p++;
				*value_p = '\0';
				return value_p - value;
			}
		}
	}
	return 0;
}

static int nand_get_gpt_flg(void)
{
	char gpt_flg[16] = {'0', 0};

	get_gpt_para_from_cmdline(saved_command_line, "gpt",  gpt_flg, 16);
	nand_dbg_inf("nand gpt commandline =%c\n", gpt_flg[0]);
	if (gpt_flg[0] == '1') {
		nand_dbg_inf("nand enable GPT\n");
		nand_gpt_enable = 1;
	} else {
		nand_dbg_inf("nand disable GPT\n");
		nand_gpt_enable = 0;
	}
	return 0;
}

extern uint32 get_nftl_logic_size(void);
/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int add_nand(struct nand_blk_ops *tr, struct _nand_phy_partition *phy_partition)
{
	int i;
	__u32 cur_offset = 0;
	struct _nftl_blk *nftl_blk;
	struct _nand_dev *nand_dev;
	struct _nand_disk *disk;
	struct _nand_disk *head_disk;
	uint16 PartitionNO = 0;
	unsigned int partition_total_sector = 0;
	unsigned char patition_cnt = MAX_PART_COUNT_PER_FTL;

	if (flash_type == 1) {
		PartitionNO = get_partitionNO(phy_partition);

	nftl_blk = kmalloc(sizeof(struct _nftl_blk), GFP_KERNEL);
	if (!nftl_blk)
		return -ENOMEM;

	nftl_blk->nand = build_nand_partition(phy_partition);

	if (nftl_initialize(nftl_blk, PartitionNO)) {
		nand_dbg_err("nftl_initialize failed\n");
		return 1;
	}

	nftl_blk->blk_lock = kmalloc(sizeof(struct mutex), GFP_KERNEL);
	if (!nftl_blk->blk_lock)
		return -ENOMEM;

	mutex_init(nftl_blk->blk_lock);

	nftl_blk->nftl_thread =
	    kthread_run(nftl_thread, nftl_blk, "%sd", "nftl");
	if (IS_ERR(nftl_blk->nftl_thread)) {
		nand_dbg_err("init kthread_run fail!\n");
		return 1;
	}

	add_nftl_blk_list(&tr->nftl_blk_head, nftl_blk);

	s_nand_kobj = kzalloc(sizeof(struct nand_kobject), GFP_KERNEL);
	if (!s_nand_kobj)
		return -ENOMEM;

	s_nand_kobj->nftl_blk = nftl_blk;
	if (kobject_init_and_add
	    (&s_nand_kobj->kobj, &ktype, NULL, "nand_driver%d",
	     PartitionNO) != 0) {
		nand_dbg_err("init nand sysfs fail!\n");
		return 1;
	}

	disk = get_disk_from_phy_partition(phy_partition);
	for (i = 0; i < MAX_PART_COUNT_PER_FTL; i++) {
		/*nand_dbg_err("disk->name %s\n",(char *)(disk->name));*/
		/*nand_dbg_err("disk->type %x\n",disk[i].type);*/
		/*nand_dbg_err("disk->size %x\n",disk[i].size);*/
		partition_total_sector += disk[i].size;
	}

	head_disk = get_disk_from_phy_partition(phy_partition);

	//nand_get_gpt_flg();

	if (NAND_SUPPORT_GPT) {
		patition_cnt = 1;
		head_disk->size = partition_total_sector;
		partition_total_sector = 0;
	}

	for (i = 0; i < patition_cnt; i++) {
		disk = head_disk + i;
		if (disk->type == 0xffffffff)
			break;

		nand_dev = kmalloc(sizeof(struct _nand_dev), GFP_KERNEL);
		if (!nand_dev)
			return -ENOMEM;

		add_nand_dev_list(&tr->nand_dev_head, nand_dev);

		nand_dev->nbd.nandr = &mytr;

		if (dev_initialize(nand_dev, nftl_blk, cur_offset, disk->size))
			return 1;

		nand_dev->nbd.size = (unsigned int)nand_dev->size;
		nand_dev->nbd.priv = (void *)nand_dev;

		memcpy(nand_dev->nbd.name, disk->name, strlen(disk->name) + 1);
		memcpy(nand_dev->name, disk->name, strlen(disk->name) + 1);
		nand_dbg_inf("nand_dev add %s\n", nand_dev->name);

			if (NAND_SUPPORT_GPT) {
				dev_num = 0;
				nand_dev->nbd.devnum = dev_num;
				if (add_nand_blktrans_dev(&nand_dev->nbd)) {
					nand_dbg_err("nftl add blk disk dev failed\n");
					return 1;
				}
			} else {
				if ((PartitionNO == 0) && (i == 0)) {
					dev_num = -1;
				} else {
					dev_num++;
					nand_dev->nbd.devnum = dev_num;
					if (add_nand_blktrans_dev(&nand_dev->nbd)) {
						nand_dbg_err("nftl add blk disk dev failed\n");
						return 1;
					}
				}
			}
			cur_offset += disk->size;
		}
	} else if (flash_type == 2) {
		nftl_blk = kmalloc(sizeof(struct _nftl_blk), GFP_KERNEL);
		if (!nftl_blk)
			return -ENOMEM;

		nftl_blk->nftl_logic_size = get_nftl_logic_size();


		nftl_blk->blk_lock = kmalloc(sizeof(struct mutex), GFP_KERNEL);
		if (!nftl_blk->blk_lock)
			return -ENOMEM;

		mutex_init(nftl_blk->blk_lock);

		nftl_blk->nftl_thread =
			kthread_run(nftl_thread, nftl_blk, "%sd", "nftl");
		if (IS_ERR(nftl_blk->nftl_thread)) {
			nand_dbg_err("init kthread_run fail!\n");
			return 1;
		}
		tr->nftl_blk_head.nftl_blk_next = nftl_blk;

		s_nand_kobj = kzalloc(sizeof(struct nand_kobject), GFP_KERNEL);
		if (!s_nand_kobj)
			return -ENOMEM;

		s_nand_kobj->nftl_blk = nftl_blk;
		nand_get_gpt_flg();

		if (NAND_SUPPORT_GPT) {
			patition_cnt = 1;
			partition_total_sector = 0;
		}

		for (i = 0; i < patition_cnt; i++) {
			nand_dev = kmalloc(sizeof(struct _nand_dev), GFP_KERNEL);
			if (!nand_dev)
				return -ENOMEM;

			add_nand_dev_list(&tr->nand_dev_head, nand_dev);

			nand_dev->nbd.nandr = &mytr;

			if (dev_initialize(nand_dev, nftl_blk, cur_offset, 0))
				return 1;

			nand_dev->nbd.size = (unsigned int)nand_dev->size;
			nand_dev->nbd.priv = (void *)nand_dev;


			if (NAND_SUPPORT_GPT) {
				dev_num = 0;
				nand_dev->nbd.devnum = dev_num;
				if (add_nand_blktrans_dev(&nand_dev->nbd)) {
					nand_dbg_err("nftl add blk disk dev failed\n");
					return 1;
				}
			} else {
				if ((PartitionNO == 0) && (i == 0)) {
					dev_num = -1;
				} else {
					dev_num++;
					nand_dev->nbd.devnum = dev_num;
					if (add_nand_blktrans_dev(&nand_dev->nbd)) {
						nand_dbg_err("nftl add blk disk dev failed\n");
						return 1;
					}
				}
			}
			/*cur_offset += disk->size;*/
		}
	}
	return 0;
}

int add_nand_for_dragonboard_test(struct nand_blk_ops *tr)
{
	struct _nand_dev *nand_dev;

	nand_dev = kmalloc(sizeof(struct _nand_dev), GFP_KERNEL);
	if (!nand_dev)
		return 1;

	add_nand_dev_list(&tr->nand_dev_head, nand_dev);

	nand_dev->nbd.nandr = &mytr;

	nand_dev->nbd.size = 1024 * 4096;
	nand_dev->nbd.priv = (void *)nand_dev;

	dev_num = 0;
	nand_dev->nbd.devnum = dev_num;
	nand_dbg_inf("before add nand blktrans dev\n");
	if (add_nand_blktrans_dev_for_dragonboard(&nand_dev->nbd)) {
		nand_dbg_err("nftl add blk disk dev failed\n");
		return 1;
	}
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int remove_nand(struct nand_blk_ops *tr)
{
	struct _nftl_blk *nftl_blk;
	struct _nand_dev *nand_dev;

	nand_dbg_err("remove_nand\n");

	nftl_blk = &tr->nftl_blk_head;
	nand_dev = &tr->nand_dev_head;

	nand_dev = del_last_nand_dev(&tr->nand_dev_head);

	if (flash_type == 1) {
		while (nand_dev != NULL) {
			nand_flush(&nand_dev->nbd);
			del_nand_blktrans_dev(&nand_dev->nbd);
			kfree(nand_dev);
			nand_dev = del_last_nand_dev(&tr->nand_dev_head);
		}
		nftl_blk = del_last_nftl_blk(&tr->nftl_blk_head);
		while (nftl_blk != NULL) {
			if (nftl_blk->nftl_thread != NULL) {
				kthread_stop(nftl_blk->nftl_thread);
				nftl_blk->nftl_thread = NULL;
			}
			kfree(nftl_blk->blk_lock);
			nftl_exit(nftl_blk);
			kfree(nftl_blk);
			nftl_blk = del_last_nftl_blk(&tr->nftl_blk_head);
		}
	} else if (flash_type == 2) {
		while (nand_dev != NULL) {
			del_nand_blktrans_dev(&nand_dev->nbd);
			kfree(nand_dev);
			nand_dev = del_last_nand_dev(&tr->nand_dev_head);
		}
	}
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int nand_flush(struct nand_blk_dev *dev)
{
	int error = 0;
	struct _nand_dev *nand_dev = (struct _nand_dev *)(dev->priv);

	error = nand_dev->flush_write_cache(nand_dev, 0xffff);

	return error;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int dev_initialize(struct _nand_dev *nand_dev, struct _nftl_blk *nftl_blk, __u32 offset, __u32 size)
{
	__u32 offset_t, size_t;

	offset_t = offset;

	if (flash_type == 1) {
		size_t = size;
		nand_dev->nftl_blk = nftl_blk;

	if (offset_t < nftl_blk->nftl_logic_size)
		nand_dev->offset = offset_t;
	else
		return 1;

	if ((nand_dev->offset + size_t) <= nftl_blk->nftl_logic_size)
		nand_dev->size = size_t;
	else
		nand_dev->size = nftl_blk->nftl_logic_size - nand_dev->offset;

		nand_dev->read_data = _dev_nand_read;
		nand_dev->write_data = _dev_nand_write;
		nand_dev->flush_write_cache = _dev_flush_write_cache;
		nand_dev->flush_sector_write_cache = _dev_flush_sector_write_cache;
		nand_dev->discard = _dev_nand_discard;
	} else if (flash_type == 2) {
		nand_dev->nftl_blk = nftl_blk;

		if (offset_t < nftl_blk->nftl_logic_size)
			nand_dev->offset = offset_t;
		else
			return 1;

		nand_dev->size = nftl_blk->nftl_logic_size;

		nand_dev->read_data = _dev_nand_read;
		nand_dev->write_data = _dev_nand_write;
		nand_dev->flush_write_cache = _dev_flush_write_cache;
	}

	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int _dev_nand_read(struct _nand_dev *nand_dev, __u32 start_sector, __u32 len,
		   unsigned char *buf)
{
	int ret;
	struct _nftl_blk *nftl_blk = nand_dev->nftl_blk;

	nand_active_time = jiffies;

	mutex_lock(nftl_blk->blk_lock);

	if (start_sector + len > nand_dev->size) {
		ret = 0xfffff;
		nand_dbg_err("_dev_nand_read over size 0x%x 0x%x\n",
			     start_sector, nand_dev->size);
		while (--ret)
			;
		ret = -1;
		goto _dev_nand_read_end;

	}

	if (flash_type == 1) {
		ret = nand_dev->nftl_blk->read_data(nand_dev->nftl_blk,
				start_sector + nand_dev->offset, len, buf);
	} else if (flash_type ==  2) {
		ret = tsector_read(start_sector, len, buf);
	}

_dev_nand_read_end:

	nand_active_time = jiffies;

	mutex_unlock(nftl_blk->blk_lock);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int _dev_nand_write(struct _nand_dev *nand_dev, __u32 start_sector, __u32 len,
		    unsigned char *buf)
{
	__u32 ret;
	struct _nftl_blk *nftl_blk = nand_dev->nftl_blk;

	nand_active_time = jiffies;
	nand_write_active_time = jiffies;

	mutex_lock(nftl_blk->blk_lock);

	if (start_sector + len > nand_dev->size) {
		ret = 0xffffff;
		nand_dbg_err("_dev_nand_write over size 0x%x 0x%x\n",
		    start_sector, nand_dev->size);
		while (--ret)
			;
		ret = -1;
		goto _dev_nand_write_end;
	}

	if (flash_type == 1) {
		ret = nand_dev->nftl_blk->write_data(nand_dev->nftl_blk,
		    start_sector + nand_dev->offset, len, buf);
	} else if (flash_type ==  2) {
			ret = tsector_write(start_sector, len, buf);
	}

_dev_nand_write_end:

	nand_active_time = jiffies;
	nand_write_active_time = jiffies;

	mutex_unlock(nftl_blk->blk_lock);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int _dev_nand_read_sg(struct _nand_dev *nand_dev, struct scatterlist_ftl *sg_info, int count)
{
	int ret;
	struct _nftl_blk *nftl_blk = nand_dev->nftl_blk;

	nand_active_time = jiffies;

	mutex_lock(nftl_blk->blk_lock);

	if ((sg_info[count-1].addr + sg_info[count-1].len) > nand_dev->size) {
		ret = 0xfffff;
		nand_dbg_err("_dev_nand_read over size 0x%x 0x%x\n",
			     (unsigned int)sg_info[count-1].addr, (unsigned int)nand_dev->size);
		while (--ret)
			;
		ret = -1;
		goto _dev_nand_read_end;

	}

	ret = tlinux_sector_read(sg_info, count);

_dev_nand_read_end:

	nand_active_time = jiffies;

	mutex_unlock(nftl_blk->blk_lock);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int _dev_nand_write_sg(struct _nand_dev *nand_dev, struct scatterlist_ftl *sg_info, int count)
{
	__u32 ret;
	struct _nftl_blk *nftl_blk = nand_dev->nftl_blk;

	nand_active_time = jiffies;
	nand_write_active_time = jiffies;

	mutex_lock(nftl_blk->blk_lock);

	if ((sg_info[count-1].addr + sg_info[count-1].len) > nand_dev->size) {
		ret = 0xffffff;
		nand_dbg_err("_dev_nand_write over size 0x%x 0x%x\n",
		    (unsigned int)sg_info[count-1].addr, (unsigned int)nand_dev->size);
		while (--ret)
			;
		ret = -1;
		goto _dev_nand_write_end;
	}

	ret = tlinux_sector_write(sg_info, count);

_dev_nand_write_end:

	nand_active_time = jiffies;
	nand_write_active_time = jiffies;

	mutex_unlock(nftl_blk->blk_lock);

	return ret;
}


/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int _dev_nand_discard(struct _nand_dev *nand_dev, __u32 start_sector, __u32 len)
{
	__u32 ret = 0;
	struct _nftl_blk *nftl_blk = nand_dev->nftl_blk;

	mutex_lock(nftl_blk->blk_lock);

	/*nand_dbg_err("======nand_discard====== %d,%d\n",start_sector,len);*/

	ret =
		nand_dev->nftl_blk->discard(nand_dev->nftl_blk,
					start_sector + nand_dev->offset, len);

	nand_active_time = jiffies;

	mutex_unlock(nftl_blk->blk_lock);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int _dev_flush_write_cache(struct _nand_dev *nand_dev, __u32 num)
{
	__u32 ret;
	struct _nftl_blk *nftl_blk = nand_dev->nftl_blk;

	nand_active_time = jiffies;

	mutex_lock(nftl_blk->blk_lock);

	if (flash_type ==  1) {
		ret = nand_dev->nftl_blk->flush_write_cache(nand_dev->nftl_blk, num);
	} else if (flash_type ==  2) {
		ret = tflush_write_cache();
	}
	nand_active_time = jiffies;

	mutex_unlock(nftl_blk->blk_lock);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int _dev_flush_sector_write_cache(struct _nand_dev *nand_dev, __u32 num)
{
	__u32 ret;
	struct _nftl_blk *nftl_blk = nand_dev->nftl_blk;

	mutex_lock(nftl_blk->blk_lock);

	ret =
	    nand_dev->nftl_blk->flush_sector_write_cache(nand_dev->nftl_blk,
							 num);

	mutex_unlock(nftl_blk->blk_lock);

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
struct _nand_dev *_get_nand_dev_by_name(char *name)
{
	int len;
	struct _nand_dev *p;

	len = strlen(name) + 1;

	if (len > 16)
		len = 16;

	for (p = &mytr.nand_dev_head; p != NULL; p = p->nand_dev_next) {
		if (memcmp(p->name, name, len) == 0)
			return p;
	}
	return NULL;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int _dev_nand_read2(char *name, unsigned int start_sector, unsigned int len,
		    unsigned char *buf)
{
	int ret;
	struct _nand_dev *nand_dev = _get_nand_dev_by_name(name);

	if (nand_dev == NULL)
		return -1;

	if (start_sector + len > nand_dev->size) {
		nand_dbg_err("_dev_nand_read over size 0x%x 0x%x\n",
			     start_sector, nand_dev->size);
		return -1;
	}
	if (flash_type == 1) {
		ret = nand_dev->nftl_blk->read_data(nand_dev->nftl_blk,
				start_sector + nand_dev->offset, len, buf);
	} else if (flash_type == 2) {
		ret = tsector_read(start_sector + nand_dev->offset, len, buf);
	}

	nand_active_time = jiffies;

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
int _dev_nand_write2(char *name, unsigned int start_sector, unsigned int len,
		     unsigned char *buf)
{
	int ret;
	struct _nand_dev *nand_dev = _get_nand_dev_by_name(name);

	if (nand_dev == NULL)
		return -1;

	if (start_sector + len > nand_dev->size) {
		nand_dbg_err("_dev_nand_write over size 0x%x 0x%x\n",
		    start_sector, nand_dev->size);
		return -1;
	}

	if (flash_type == 1) {
		ret = nand_dev->nftl_blk->write_data(nand_dev->nftl_blk,
				start_sector + nand_dev->offset, len, buf);
	} else if (flash_type == 2) {
		ret = tsector_write(start_sector + nand_dev->offset, len, buf);
	}

	nand_active_time = jiffies;

	return ret;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       :
*Note         :
*****************************************************************************/
uint32 nand_read_reclaim(struct _nftl_blk *start_blk, unsigned char *buf)
{
	uint32 ret = 0;
	struct _nftl_blk *nftl_blk;

	nftl_blk = get_nftl_need_read_claim(start_blk);
	if (nftl_blk == NULL)
		return 1;

	mutex_lock(nftl_blk->blk_lock);

	ret = read_reclaim(start_blk, nftl_blk, buf);

	mutex_unlock(nftl_blk->blk_lock);

	return ret;
}
