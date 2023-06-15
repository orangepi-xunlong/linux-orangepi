/*
 * /home/zhengxiaobin/work/android12/longan/kernel/linux-5.4/drivers/video/fbdev/sunxi/disp2/eink200/hand_write/hand_write.c
 *
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "hand_write.h"


static struct hand_write_t g_hand_write;


int get_hand_write_phaddr(int fd, u32 *paddr)
{
	struct hand_write_dma_t *new_item = NULL;

	if (fd < 0 || !paddr)
		return -1;

	new_item = kmalloc(sizeof(struct hand_write_dma_t), __GFP_ZERO | GFP_KERNEL);
	if (!new_item) {
		return -2;
	}
	new_item->fd = fd;
	new_item->item = eink_dma_map(new_item->fd);
	if (!new_item->item) {
		goto FREE_FATHER;
	}
	*paddr = (u32)new_item->item->dma_addr;
	mutex_lock(&g_hand_write.list_lock);
	list_add_tail(&new_item->node, &g_hand_write.hw_used_list);
	/*pr_err("%s:%d add fd:%d\n",__func__, __LINE__, fd);*/
	mutex_unlock(&g_hand_write.list_lock);

	return 0;
FREE_FATHER:
	if (new_item)
		kfree(new_item);
	return -3;

}

static void __handwrite_unmap_work(struct work_struct *work)
{
	struct hand_write_dma_t *p_item;
	mutex_lock(&g_hand_write.list_lock);
	list_for_each_entry(p_item, &g_hand_write.hw_free_list, node) {
		/*pr_err("%s:%d del:%d\n",__func__, __LINE__, p_item->fd);*/
		list_del(&p_item->node);
		eink_dma_unmap(p_item->item);
		kfree(p_item);
		break;
	}
	mutex_unlock(&g_hand_write.list_lock);
}

void release_hand_write_memory(int fd)
{
	struct hand_write_dma_t *p_item;
	mutex_lock(&g_hand_write.list_lock);
	list_for_each_entry(p_item, &g_hand_write.hw_used_list, node) {
		list_move_tail(&p_item->node, &g_hand_write.hw_free_list);
		break;
	}
	mutex_unlock(&g_hand_write.list_lock);
	queue_delayed_work(g_hand_write.p_hw_wq, &g_hand_write.hw_delay_work, msecs_to_jiffies(500));
}

int hand_write_init(void)
{
	int ret = -1;
	g_hand_write.p_hw_wq = create_workqueue("sunxi hand_write");
	if (!g_hand_write.p_hw_wq)
		goto OUT;

	g_hand_write.handwrite_unmap_work = __handwrite_unmap_work;
	mutex_init(&g_hand_write.list_lock);
	INIT_DELAYED_WORK(&g_hand_write.hw_delay_work, g_hand_write.handwrite_unmap_work);
	INIT_LIST_HEAD(&g_hand_write.hw_used_list);
	INIT_LIST_HEAD(&g_hand_write.hw_free_list);
	ret = 0;

OUT:
	return ret;
}
