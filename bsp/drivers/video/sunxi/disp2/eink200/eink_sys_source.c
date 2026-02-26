/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs eink driver.
 *
 * Copyright (C) 2019 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "include/eink_driver.h"
#include "asm/cacheflush.h"
#include <linux/sunxi-gpio.h>
#include "include/eink_sys_source.h"

#define BYTE_ALIGN(x) (((x + (4 * 1024 - 1)) >> 12) << 12)

extern struct eink_driver_info g_eink_drvdata;


s32 eink_set_print_level(u32 level)
{
	EINK_INFO_MSG("print level = 0x%x\n", level);
	g_eink_drvdata.eink_dbg_info = level;

	return 0;
}

/* 0 -- no log
 * 8 -- print timer info
 * 7 -- capture DE WB BUF
 * 6 -- capture wavfile
 * 5 -- decode debug
 * 4 -- dump register
 * 3 -- print pipe and buf list
 * 2 -- reserve
 * 1 -- base debug info
 */

s32 eink_get_print_level(void)
{
	return g_eink_drvdata.eink_dbg_info;
}

s32 get_delt_ms_timer(struct timespec64 start_timer, struct timespec64 end_timer)
{
	return ((end_timer.tv_sec - start_timer.tv_sec) * 1000 + (end_timer.tv_nsec - start_timer.tv_nsec) / NSEC_PER_MSEC);
}

s32 get_delt_us_timer(struct timespec64 start_timer, struct timespec64 end_timer)
{
	return ((end_timer.tv_sec - start_timer.tv_sec) * 1000000 + (end_timer.tv_nsec - start_timer.tv_nsec) / NSEC_PER_USEC);
}

bool is_upd_win_zero(struct upd_win update_area)
{
	if ((update_area.left == 0) && (update_area.right == 0) && (update_area.top == 0) && (update_area.bottom == 0))
		return true;
	else
		return false;
}

int atoi_float(char *buf)
{
	int num = 0;
	int i = 0;
	int point_pos = -1;
	int pow = 0;

	while (buf[i] != '\0') {
		if (buf[i] >= '0' && buf[i] <= '9')	{
			if ((point_pos < 0) || ((point_pos >= 0) && ((point_pos + 4) > i))) {
				num = num * 10 + (buf[i] - '0');
			} else {
				/* drop the left char */
				break;
			}
		}

		if (buf[i] == '.') {
			point_pos = i;
		}
		i++;
	}

	/* auto *1000 */
	if ((point_pos >= 0) && ((point_pos + 4) > i)) {
		pow = (point_pos + 4 - i);
		while (pow > 0) {
			num = num * 10;
			pow--;
		}
	}

	if (buf[0] == '-')
		num = num * (-1);

	EINK_DEBUG_MSG("input str=%s, num=%d\n", buf, num);

	return num;
}

int eink_sys_pin_set_state(char *dev_name, char *name)
{
	char compat[32];
	u32 len = 0;
	struct device_node *node = NULL;
	struct platform_device *pdev = NULL;
	struct pinctrl *pctl = NULL;
	struct pinctrl_state *state = NULL;
	int ret = -1;

	len = sprintf(compat, "allwinner,sunxi-%s", dev_name);
	if (len > 32)
		sunxi_warn(NULL, "size of mian_name is out of range\n");

	node = of_find_compatible_node(NULL, NULL, compat);
	if (!node) {
		sunxi_err(NULL, "of_find_compatible_node %s fail\n", compat);
		goto exit;
	}

	pdev = of_find_device_by_node(node);
	if (!node) {
		sunxi_err(NULL, "of_find_device_by_node for %s fail\n", compat);
		goto exit;
	}

	pctl = pinctrl_get(&pdev->dev);
	if (IS_ERR(pctl)) {
		/* not every eink need pin config */
		sunxi_err(NULL, "%s:pinctrl_get for %s fail\n", __func__, compat);
		ret = PTR_ERR(pctl);
		goto exit;
	}

	state = pinctrl_lookup_state(pctl, name);
	if (IS_ERR(state)) {
		sunxi_err(NULL, "%s:pinctrl_lookup_state for %s fail\n", __func__, compat);
		ret = PTR_ERR(state);
		goto exit;
	}

	ret = pinctrl_select_state(pctl, state);
	if (ret < 0) {
		sunxi_err(NULL, "%s:pinctrl_select_state(%s) for %s fail\n", __func__, name, compat);
		goto exit;
	}
	ret = 0;
exit:
	return ret;
}
EXPORT_SYMBOL(eink_sys_pin_set_state);

int eink_sys_power_enable(struct regulator *regu)
{
	int ret = 0;

	if (!regu) {
		sunxi_err(NULL, "[%s]regulator is NULL\n", __func__);
		return -1;
	}
	/* enalbe regulator */
	ret = regulator_enable(regu);
	if (ret != 0)
		sunxi_err(NULL, "some err happen, fail to enable regulator!\n");

	return ret;
}
EXPORT_SYMBOL(eink_sys_power_enable);

int eink_sys_power_disable(struct regulator *regu)
{
	int ret = 0;

	if (!regu) {
		sunxi_err(NULL, "[%s]regulator is NULL\n", __func__);
		return -1;
	}
	/* disalbe regulator */
	ret = regulator_disable(regu);
	if (ret != 0)
		sunxi_err(NULL, "some err happen, fail to disable regulator!\n");

	return ret;
}
EXPORT_SYMBOL(eink_sys_power_disable);

s32 eink_panel_pin_cfg(u32 en)
{
	struct eink_manager *eink_mgr = get_eink_manager();
	char dev_name[25];

	if (eink_mgr == NULL) {
		DE_WRN("NULL hdl!\n");
		return -1;
	}
	EINK_DEBUG_MSG("eink pin config, state %s, %d\n", (en) ? "on" : "off", en);

	/* io-pad */
	if (en == 1) {
		if (!((!strcmp(eink_mgr->eink_pin_power, "")) ||
					(!strcmp(eink_mgr->eink_pin_power, "none"))))
			eink_sys_power_enable(eink_mgr->pin_regulator);
	}

	sprintf(dev_name, "eink");
	eink_sys_pin_set_state(dev_name, (en == 1) ?
			EINK_PIN_STATE_ACTIVE : EINK_PIN_STATE_SLEEP);

	if (en == 0) {
		if (!((!strcmp(eink_mgr->eink_pin_power, "")) ||
					(!strcmp(eink_mgr->eink_pin_power, "none"))))
			eink_sys_power_disable(eink_mgr->pin_regulator);
	}

	return 0;
}

int eink_sys_gpio_request(struct eink_gpio_cfg *gpio_info)
{
	int ret = 0;

	if (!gpio_info) {
		sunxi_err(NULL, "%s: gpio_info is null\n", __func__);
		return -1;
	}

	if (!strlen(gpio_info->name))
		return 0;

	if (!gpio_is_valid(gpio_info->gpio)) {
		sunxi_err(NULL, "%s: gpio (%d) is invalid\n", __func__, gpio_info->gpio);
		return -1;
	}

	ret = gpio_direction_output(gpio_info->gpio, gpio_info->value);
	if (ret) {
		sunxi_err(NULL, "%s failed, gpio_name=%s, gpio=%d, value=%d, ret=%d\n", __func__,
				gpio_info->name, gpio_info->gpio, gpio_info->value, ret);
		return -1;
	}

	EINK_DEBUG_MSG("%s, gpio_name=%s, gpio=%d, value=%d, ret=%d\n", __func__,
			gpio_info->name, gpio_info->gpio, gpio_info->value, ret);

	return ret;
}
EXPORT_SYMBOL(eink_sys_gpio_request);

int eink_sys_gpio_release(struct eink_gpio_cfg *gpio_info)
{
	if (!gpio_info) {
		sunxi_err(NULL, "%s: gpio_info is null\n", __func__);
		return -1;
	}

	if (!strlen(gpio_info->name))
		return -1;

	if (!gpio_is_valid(gpio_info->gpio)) {
		sunxi_err(NULL, "%s: gpio(%d) is invalid\n", __func__, gpio_info->gpio);
		return -1;
	}

	gpio_free(gpio_info->gpio);

	return 0;
}
EXPORT_SYMBOL(eink_sys_gpio_release);

int eink_sys_gpio_set_value(u32 p_handler, u32 value_to_gpio,
			    const char *gpio_name)
{
	if (p_handler)
		__gpio_set_value(p_handler, value_to_gpio);
	else
		sunxi_warn(NULL, "EINK_SET_GPIO, hdl is NULL\n");

	return 0;
}

/* type: 0:invalid, 1: int; 2:str, 3: gpio */
int eink_sys_script_get_item(char *main_name, char *sub_name, int value[],
			     int type)
{
	char compat[32];
	u32 len = 0;
	struct device_node *node;
	int ret = 0;
	enum of_gpio_flags flags;

	len = sprintf(compat, "allwinner,sunxi-%s", main_name);
	if (len > 32)
		sunxi_err(NULL, "size of mian_name is out of range\n");

	node = of_find_compatible_node(NULL, NULL, compat);
	if (!node) {
		sunxi_err(NULL, "of_find_compatible_node %s fail\n", compat);
		return ret;
	}

	if (type == 1) {
		if (of_property_read_u32_array(node, sub_name, value, 1))
			sunxi_info(NULL, "of_property_read_u32_array %s.%s fail\n",
			      main_name, sub_name);
		else
			ret = type;
	} else if (type == 2) {
		const char *str;

		if (of_property_read_string(node, sub_name, &str))
			sunxi_info(NULL, "of_property_read_string %s.%s fail\n", main_name,
			      sub_name);
		else {
			ret = type;
			memcpy((void *)value, str, strlen(str) + 1);
		}
	} else if (type == 3) {
		int gpio;
		struct eink_gpio_cfg *gpio_info = (struct eink_gpio_cfg *)value;

		gpio_info->gpio = -1;
		gpio_info->name[0] = '\0';

		gpio =
		    of_get_named_gpio_flags(node, sub_name, 0, &flags);
		if (!gpio_is_valid(gpio))
			return -EINVAL;

		gpio_info->gpio = gpio;
		memcpy(gpio_info->name, sub_name, strlen(sub_name) + 1);
		gpio_info->value = (flags == OF_GPIO_ACTIVE_LOW) ? 0 : 1;

		EINK_DEBUG_MSG("%s.%s gpio=%d, value:%d\n", main_name, sub_name,
						gpio_info->gpio, gpio_info->value);
	}

	return ret;
}
EXPORT_SYMBOL(eink_sys_script_get_item);

#if IS_ENABLED(CONFIG_ION) && defined(EINK_CACHE_MEM)
static int __eink_ion_alloc_coherent(struct eink_ion_mem *mem)
{
	unsigned int flags = ION_FLAG_CACHED;
	unsigned int heap_id_mask;
	struct dma_buf *dmabuf;
	int fd = -1;

	if (mem == NULL) {
		sunxi_err(NULL, "input param is null\n");
		return -1;
	}

#if IS_ENABLED(CONFIG_AW_IOMMU) || IS_ENABLED(CONFIG_ARM_SMMU_V3)
	heap_id_mask = 1 << ION_HEAP_TYPE_SYSTEM;
#else
	heap_id_mask = 1 << ION_HEAP_TYPE_DMA;
#endif

	dmabuf = ion_alloc(mem->size, heap_id_mask, flags);
	if (IS_ERR_OR_NULL(dmabuf)) {
		sunxi_err(NULL, "ion_alloc failed, size=%lu!\n", (unsigned long)mem->size);
		return -2;
	}

	mem->vaddr = ion_heap_map_kernel(NULL, dmabuf->priv);

	if (IS_ERR_OR_NULL(mem->vaddr)) {
		sunxi_err(NULL, "ion_map_kernel failed!!\n");
		goto err_map_kernel;
	}

	fd = dma_buf_fd(dmabuf, O_CLOEXEC);
	mem->p_item = eink_dma_map(fd);

	if (!mem->p_item)
		goto err_phys;

	EINK_INFO_MSG("ion malloc size=0x%x, vaddr=%p, paddr=0x%08x\n",
			(unsigned int)mem->size, mem->vaddr,
			(unsigned int)mem->p_item->dma_addr);

	put_unused_fd(fd);
	return 0;

err_phys:
	ion_heap_unmap_kernel(NULL, mem->p_item->dmabuf->priv);
err_map_kernel:
	ion_free(mem->p_item->dmabuf->priv);
	return -ENOMEM;
}

static void __eink_ion_free_coherent(struct eink_ion_mem *mem)
{
	if (IS_ERR_OR_NULL(mem->p_item) || IS_ERR_OR_NULL(mem->vaddr)) {
		sunxi_err(NULL, "input param is null\n");
		return;
	}

	mem->p_item->dmabuf->ops->release(mem->p_item->dmabuf);
	ion_heap_unmap_kernel(NULL, mem->p_item->dmabuf->priv);
	ion_free(mem->p_item->dmabuf->priv);
	eink_dma_unmap(mem->p_item);
	return;
}

static void __eink_ion_dump(void)
{
#ifdef ION_DEBUG_DUMP
	struct eink_ion_mgr *ion_mgr =  &(g_eink_drvdata.ion_mgr);
	struct ion_client *client = NULL;
	struct eink_ion_list_node *ion_node = NULL, *tmp_ion_node = NULL;
	struct eink_ion_mem *mem = NULL;
	unsigned int i = 0;

	if (ion_mgr == NULL) {
		sunxi_err(NULL, "eink ion mgr is not initial yet\n");
		return;
	}

	client = ion_mgr->client;

	mutex_lock(&(ion_mgr->mlock));
	list_for_each_entry_safe(ion_node, tmp_ion_node, &ion_mgr->ion_list, node) {
		if (ion_node != NULL) {
			mem = &ion_node->mem;
			EINK_INFO_MSG("ion node %d: vaddr=0x%08x, paddr=0x%08x, size=%d\n",
				i, (unsigned int)mem->vaddr, (unsigned int)mem->paddr, mem->size);
			i++;
		}
	}
	mutex_unlock(&(ion_mgr->mlock));
#endif
	return;
}

static void *eink_ion_malloc(u32 num_bytes, void *phys_addr)
{
	struct eink_ion_mgr *ion_mgr =  &(g_eink_drvdata.ion_mgr);
	struct eink_ion_list_node *ion_node = NULL;
	struct eink_ion_mem *mem = NULL;
	u32 *paddr = NULL;
	int ret = -1;

	if (ion_mgr == NULL) {
		sunxi_err(NULL, "eink ion manager has not initial yet\n");
		return NULL;
	}

	ion_node = kmalloc(sizeof(struct eink_ion_list_node), GFP_KERNEL);
	if (ion_node == NULL) {
		sunxi_err(NULL, "fail to alloc ion node, size=%u\n",
			(unsigned int)sizeof(struct eink_ion_list_node));
		return NULL;
	}

	mutex_lock(&(ion_mgr->mlock));
	mem = &ion_node->mem;
	mem->size = BYTE_ALIGN(num_bytes);
	ret = __eink_ion_alloc_coherent(mem);
	if (ret != 0) {
		sunxi_err(NULL, "fail to alloc ion, ret=%d\n", ret);
		goto err_hdl;
	}

	paddr = (u32 *)phys_addr;
	*paddr = (u32)mem->p_item->dma_addr;
	list_add_tail(&(ion_node->node), &(ion_mgr->ion_list));

	mutex_unlock(&(ion_mgr->mlock));

	__eink_ion_dump();

	return mem->vaddr;

err_hdl:
	kfree(ion_node);
	mutex_unlock(&(ion_mgr->mlock));

	return NULL;
}

static void eink_ion_free(void *virt_addr, void *phys_addr, u32 num_bytes)
{
	struct eink_ion_mgr *ion_mgr =  &(g_eink_drvdata.ion_mgr);
	struct eink_ion_list_node *ion_node = NULL, *tmp_ion_node = NULL;
	struct eink_ion_mem *mem = NULL;
	bool found = false;

	if (ion_mgr == NULL) {
		sunxi_err(NULL, "eink ion manager has not initial yet\n");
		return;
	}

	mutex_lock(&(ion_mgr->mlock));
	list_for_each_entry_safe(ion_node, tmp_ion_node, &ion_mgr->ion_list,
				node) {
		if (ion_node != NULL) {
			mem = &ion_node->mem;
			if ((((unsigned long)mem->p_item->dma_addr) ==
			     ((unsigned long)phys_addr)) &&
			     (((unsigned long)mem->vaddr) ==
			      ((unsigned long)virt_addr))) {
				__eink_ion_free_coherent(mem);
				__list_del_entry(&(ion_node->node));
				found = true;
				break;
			}
		}
	}
	mutex_unlock(&(ion_mgr->mlock));

	if (false == found) {
		sunxi_err(NULL, "vaddr=0x%08x, paddr=0x%08x is not found in ion\n",
				*((unsigned int *)virt_addr),
				*((unsigned int *)phys_addr));
	}

	return;
}

int init_eink_ion_mgr(struct eink_ion_mgr *ion_mgr)
{
	if (ion_mgr == NULL) {
		sunxi_err(NULL, "[%s]:input param is null\n", __func__);
		return -EINVAL;
	}

	mutex_init(&(ion_mgr->mlock));

	mutex_lock(&(ion_mgr->mlock));
	INIT_LIST_HEAD(&(ion_mgr->ion_list));
	mutex_unlock(&(ion_mgr->mlock));

	return 0;

}

void deinit_eink_ion_mgr(struct eink_ion_mgr *ion_mgr)
{
	struct eink_ion_list_node *ion_node = NULL, *tmp_ion_node = NULL;
	struct eink_ion_mem *mem = NULL;

	if (ion_mgr == NULL) {
		sunxi_err(NULL, "[%s]:input param is null\n", __func__);
		return;
	}

	mutex_lock(&(ion_mgr->mlock));
	list_for_each_entry_safe(ion_node, tmp_ion_node,
						&ion_mgr->ion_list, node) {
		if (ion_node != NULL) {
			/* free all ion node */
			mem = &ion_node->mem;
			__eink_ion_free_coherent(mem);
			__list_del_entry(&(ion_node->node));
			kfree(ion_node);
		}
	}
	mutex_unlock(&(ion_mgr->mlock));
}

#else
void *eink_mem_malloc(u32 num_bytes, void *phys_addr)
{
	u32 actual_bytes;
	void *address = NULL;

	EINK_DEBUG_MSG("input!\n");
	if (num_bytes != 0) {
		actual_bytes = ALIGN(num_bytes, PAGE_SIZE);

		address =
		    dma_alloc_coherent(g_eink_drvdata.device, actual_bytes,
				       (dma_addr_t *) phys_addr, GFP_KERNEL);
		if (address) {
			EINK_DEBUG_MSG
			    ("dma_alloc_coherent ok, address=0x%p, size=0x%x\n",
			     (void *)(*(unsigned long *)phys_addr), num_bytes);
			return address;
		}

		sunxi_err(NULL, "dma_alloc_coherent fail, size=0x%x\n", num_bytes);
		return NULL;
	}

	sunxi_warn(NULL, "%s size is zero\n", __func__);

	return NULL;
}

void eink_mem_free(void *virt_addr, void *phys_addr, u32 num_bytes)
{
	u32 actual_bytes;

	actual_bytes = BYTE_ALIGN(num_bytes);
	if (phys_addr && virt_addr)
		dma_free_coherent(g_eink_drvdata.device, actual_bytes, virt_addr,
				  (dma_addr_t)phys_addr);
}
#endif

void *eink_malloc(u32 num_bytes, void *phys_addr)
{
#if IS_ENABLED(CONFIG_ION) && defined(EINK_CACHE_MEM)
	return eink_ion_malloc(num_bytes, phys_addr); /* cache */
#else
	return eink_mem_malloc(num_bytes, phys_addr); /* skip cache */
#endif
}
EXPORT_SYMBOL(eink_malloc);

void eink_free(void *virt_addr, void *phys_addr, u32 num_bytes)
{
#if IS_ENABLED(CONFIG_ION) && defined(EINK_CACHE_MEM)
	return eink_ion_free(virt_addr, phys_addr, num_bytes);
#else
	return eink_mem_free(virt_addr, phys_addr, num_bytes);
#endif
}
EXPORT_SYMBOL(eink_free);

void eink_cache_sync(void *startAddr, int size)
{
#if IS_ENABLED(CONFIG_ION) && defined(EINK_CACHE_MEM)
	/*
	 * fix if add or not
	dma_sync_single_for_cpu(g_eink_drvdata.device,
			(dma_addr_t)startAddr, size, DMA_TO_DEVICE);
	*/
	dma_sync_single_for_device(g_eink_drvdata.device,
			(dma_addr_t)startAddr, size, DMA_TO_DEVICE);
	return;
#endif
}
EXPORT_SYMBOL(eink_cache_sync);

int eink_dma_map_core(int fd, struct dmabuf_item *item)
{
	struct dma_buf *dmabuf;
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
	int ret = -1;

	if (fd < 0) {
		sunxi_err(NULL, "[EINK]dma_buf_id(%d) is invalid\n", fd);
		goto exit;
	}
	dmabuf = dma_buf_get(fd);
	if (IS_ERR(dmabuf)) {
		sunxi_err(NULL, "[EINK]dma_buf_get failed, fd=%d\n", fd);
		goto exit;
	}

	attachment = dma_buf_attach(dmabuf, g_eink_drvdata.device);
	if (IS_ERR(attachment)) {
		sunxi_err(NULL, "[EINK]dma_buf_attach failed\n");
		goto err_buf_put;
	}
	sgt = dma_buf_map_attachment(attachment, DMA_FROM_DEVICE);
	if (IS_ERR_OR_NULL(sgt)) {
		sunxi_err(NULL, "[EINK]dma_buf_map_attachment failed\n");
		goto err_buf_detach;
	}

	item->dmabuf = dmabuf;
	item->sgt = sgt;
	item->attachment = attachment;
	item->dma_addr = sg_dma_address(sgt->sgl);
	ret = 0;

	goto exit;

	/* unmap attachment sgt, not sgt_bak, because it's not alloc yet! */
	dma_buf_unmap_attachment(attachment, sgt, DMA_FROM_DEVICE);
err_buf_detach:
	dma_buf_detach(dmabuf, attachment);
err_buf_put:
	dma_buf_put(dmabuf);
exit:
	return ret;
}

void eink_dma_unmap_core(struct dmabuf_item *item)
{
	dma_buf_unmap_attachment(item->attachment, item->sgt, DMA_FROM_DEVICE);
	dma_buf_detach(item->dmabuf, item->attachment);
	dma_buf_put(item->dmabuf);
}

struct dmabuf_item *eink_dma_map(int fd)
{
	struct dmabuf_item *item = NULL;

	EINK_INFO_MSG("Func intput!\n");
	item = kmalloc(sizeof(struct dmabuf_item),
			      GFP_KERNEL | __GFP_ZERO);

	if (item == NULL) {
		sunxi_err(NULL, "malloc memory of size %d fail!\n",
		       (unsigned int)sizeof(struct dmabuf_item));
		goto exit;
	}
	if (eink_dma_map_core(fd, item) != 0) {
		kfree(item);
		item = NULL;
	}

exit:
	return item;
}

void eink_dma_unmap(struct dmabuf_item *item)
{
	eink_dma_unmap_core(item);
	kfree(item);
}

void eink_print_register(unsigned long start_addr, unsigned long end_addr)
{
	unsigned long start_addr_align = ALIGN(start_addr, 4);
	unsigned long end_addr_align = ALIGN(end_addr, 4);
	unsigned long addr = 0;
	unsigned char tmp_buf[50] = {0};
	unsigned char buf[256] = {0};

	sunxi_info(NULL, "\n");
	sunxi_info(NULL, "print reg: [0x%08x ~ 0x%08x]\n", (unsigned int)start_addr_align, (unsigned int)end_addr_align);
	for (addr = start_addr_align; addr <= end_addr_align; addr += 4) {
		if (0 == (addr & 0xf)) {
			memset(tmp_buf, 0, sizeof(tmp_buf));
			memset(buf, 0, sizeof(buf));
			snprintf(tmp_buf, sizeof(tmp_buf), "0x%08x: ", (unsigned int)addr);
			strncat(buf, tmp_buf, sizeof(tmp_buf));
		}

		memset(tmp_buf, 0, sizeof(tmp_buf));
		/* snprintf(tmp_buf, sizeof(tmp_buf), "0x%08x ", readl((void __iomem *)(addr | 0xf0000000))); */
		snprintf(tmp_buf, sizeof(tmp_buf), "0x%08x ", readl((void __iomem *)addr));
		strncat(buf, tmp_buf, sizeof(tmp_buf));
		if (0 == ((addr + 4) & 0xf)) {
			sunxi_info(NULL, "%s\n", buf);
			msleep(5);
		}
	}
}

void print_free_pipe_list(struct pipe_manager *mgr)
{
	struct pipe_info_node *pipe, *tpipe;
	unsigned long flags = 0;
	unsigned int count = 0;

	spin_lock_irqsave(&mgr->list_lock, flags);
	if (list_empty(&mgr->pipe_free_list)) {
		EINK_INFO_MSG("FREE_LIST is empty\n");
		spin_unlock_irqrestore(&mgr->list_lock, flags);
		return;
	}

	list_for_each_entry_safe(pipe, tpipe, &mgr->pipe_free_list, node) {
#ifdef PIPELINE_DEBUG
		EINK_INFO_MSG("FREE_LIST: pipe %02d is free\n", pipe->pipe_id);
#endif
		count++;
	}

	EINK_INFO_MSG("[+++] %d Pipes are Free!\n", count);

	spin_unlock_irqrestore(&mgr->list_lock, flags);
}

void print_used_pipe_list(struct pipe_manager *mgr)
{
	struct pipe_info_node *pipe, *tpipe;
	unsigned long flags = 0;
	unsigned int count = 0;

	spin_lock_irqsave(&mgr->list_lock, flags);
	if (list_empty(&mgr->pipe_used_list)) {
		EINK_INFO_MSG("USED_LIST is empty\n");
		spin_unlock_irqrestore(&mgr->list_lock, flags);
		return;
	}

	list_for_each_entry_safe(pipe, tpipe, &mgr->pipe_used_list, node) {
#ifdef PIPELINE_DEBUG
		EINK_INFO_MSG("USED_LIST: pipe %02d is used\n", pipe->pipe_id);
#endif
		count++;
	}

	EINK_INFO_MSG("[+++] %d Pipes are Used!\n", count);

	spin_unlock_irqrestore(&mgr->list_lock, flags);
}

void print_coll_img_list(struct buf_manager *mgr)
{
	struct img_node *curnode = NULL, *tmpnode = NULL;

	mutex_lock(&mgr->mlock);
	if (list_empty(&mgr->img_collision_list)) {
		EINK_INFO_MSG("COLL_LIST is empty\n");
		mutex_unlock(&mgr->mlock);
		return;
	}

	list_for_each_entry_safe(curnode, tmpnode, &mgr->img_collision_list, node) {
		EINK_INFO_MSG("COLL_LIST: [img %d] is collision\n", curnode->upd_order);
	}
	mutex_unlock(&mgr->mlock);
}

void print_used_img_list(struct buf_manager *mgr)
{
	struct img_node *curnode = NULL, *tmpnode = NULL;

	mutex_lock(&mgr->mlock);
	if (list_empty(&mgr->img_used_list)) {
		EINK_INFO_MSG("USED_LIST is empty\n");
		mutex_unlock(&mgr->mlock);
		return;
	}

	list_for_each_entry_safe(curnode, tmpnode, &mgr->img_used_list, node) {
		EINK_INFO_MSG("IMG_USED_LIST: [img %d] is used\n", curnode->upd_order);
	}

	mutex_unlock(&mgr->mlock);
}

void print_free_img_list(struct buf_manager *mgr)
{
	struct img_node *curnode = NULL, *tmpnode = NULL;

	mutex_lock(&mgr->mlock);
	if (list_empty(&mgr->img_free_list)) {
		EINK_INFO_MSG("IMG_FREE_LIST is empty\n");
		mutex_unlock(&mgr->mlock);
		return;
	}

	list_for_each_entry_safe(curnode, tmpnode, &mgr->img_free_list, node) {
		EINK_INFO_MSG("FREE_LIST: [img %d] is free\n", curnode->upd_order);
	}

	mutex_unlock(&mgr->mlock);
}

int save_as_bin_file(__u8 *buf, char *file_name, __u32 length, loff_t pos)
{
	struct file *fp = NULL;
	mm_segment_t old_fs;
	ssize_t ret = 0;

	if ((buf == NULL) || (file_name == NULL)) {
		sunxi_err(NULL, "%s: buf or file_name is null\n", __func__);
		return -1;
	}

	fp = filp_open(file_name, O_RDWR | O_CREAT, 0644);
	if (IS_ERR(fp)) {
		sunxi_err(NULL, "%s: fail to open file(%s), ret=%d\n",
					__func__, file_name, *((u32 *)fp));
		return -1;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	ret = vfs_write(fp, buf, length, &pos);
	sunxi_info(NULL, "%s: save %s done, len=%u, pos=%lld, ret=%d\n",
					__func__, file_name, length, pos, (unsigned int)ret);

	set_fs(old_fs);
	filp_close(fp, NULL);

	return ret;

}

s32 save_gray_image_as_bmp(u8 *buf, char *file_name, u32 scn_w, u32 scn_h)
{
	int ret = -1;
	ES_FILE *fd = NULL;
	mm_segment_t old_fs;
	loff_t pos = 0;
	BMP_FILE_HEADER st_file_header;
	BMP_INFO_HEADER st_info_header;
	char *src_buf = buf;
	char *path = file_name;
	BMP_INFO dest_info;
	u32 bit_count = 0;
	ST_RGB *dest_buf = NULL;
	ST_ARGB color_table[256];
	u32 i  = 0;

	if ((path == NULL) || (src_buf == NULL)) {
		sunxi_err(NULL, "%s: input param is null\n", __func__);
		return -1;
	}

	fd = filp_open(path, O_RDWR | O_CREAT, 0644);
	if (IS_ERR(fd)) {
		sunxi_err(NULL, "%s: create bmp file(%s) error\n", __func__, path);
		return -2;
	}

	bit_count = 8;
	memset(&dest_info, 0, sizeof(BMP_INFO));
	dest_info.width = scn_w;
	dest_info.height = scn_h;
	dest_info.bit_count = 8;
	dest_info.color_tbl_size = 0;
	dest_info.image_size = (dest_info.width * dest_info.height * dest_info.bit_count)/8;

	st_file_header.bfType[0] = 'B';
	st_file_header.bfType[1] = 'M';
	st_file_header.bfOffBits = BMP_IMAGE_DATA_OFFSET;
	st_file_header.bfSize = st_file_header.bfOffBits + dest_info.image_size;
	st_file_header.bfReserved1 = 0;
	st_file_header.bfReserved2 = 0;

	st_info_header.biSize = sizeof(BMP_INFO_HEADER);
	st_info_header.biWidth = dest_info.width;
	st_info_header.biHeight = dest_info.height;
	st_info_header.biPlanes = 1;
	st_info_header.biBitCount = dest_info.bit_count;
	st_info_header.biCompress = 0;
	st_info_header.biSizeImage = dest_info.image_size;
	st_info_header.biXPelsPerMeter = 0;
	st_info_header.biYPelsPerMeter = 0;
	st_info_header.biClrUsed = 0;
	st_info_header.biClrImportant = 0;

	for (i = 0; i < 256; i++) {
		color_table[i].reserved = 0;
		color_table[i].r = i;
		color_table[i].g = i;
		color_table[i].b = i;
	}

	dest_buf = (ST_RGB *)src_buf;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	ret = vfs_write(fd, (u8 *)(&st_file_header), BMP_FILE_HEADER_SIZE, &pos);
	EINK_INFO_MSG("save file header, len=%u, pos=%d, ret=%d\n", (unsigned int)BMP_FILE_HEADER_SIZE, (int)pos, ret);

	ret = vfs_write(fd, (u8 *)(&st_info_header), BMP_INFO_HEADER_SIZE, &pos);
	EINK_INFO_MSG("save file header, len=%u, pos=%d, ret=%d\n", (unsigned int)BMP_INFO_HEADER_SIZE, (int)pos, ret);

	ret = vfs_write(fd, (u8 *)(color_table), BMP_COLOR_SIZE, &pos);
	EINK_INFO_MSG("save file header, len=%u, pos=%d, ret=%d\n", (unsigned int)BMP_COLOR_SIZE, (int)pos, ret);

	ret = vfs_write(fd, (u8 *)(dest_buf), dest_info.image_size, &pos);
	EINK_INFO_MSG("save file header, len=%u, pos=%d, ret=%d\n", dest_info.image_size, (int)pos, ret);

	set_fs(old_fs);

	ret = 0;

	if (!IS_ERR(fd)) {
		filp_close(fd, NULL);
	}

	return ret;
}

void save_upd_rmi_buffer(u32 order, u8 *buf, u32 len)
{
	char file_name[256] = {0};

	if (buf == NULL) {
		sunxi_err(NULL, "%s:input param is null\n", __func__);
		return;
	}

	memset(file_name, 0, sizeof(file_name));
	snprintf(file_name, sizeof(file_name), "/tmp/rmi_buf%d.bin", order);
	save_as_bin_file(buf, file_name, len, 0);
}

void eink_put_gray_to_mem(u32 master, char *buf, u32 width, u32 height)
{
	char file_name[256] = {0};
	static u32 order = 1;

	if (buf == NULL) {
		sunxi_err(NULL, "input param is null\n");
		return;
	}

	memset(file_name, 0, sizeof(file_name));
	if (master == 1)
		snprintf(file_name, sizeof(file_name), "/tmp/eink_image_1_%d.bmp", order);
	else
		snprintf(file_name, sizeof(file_name), "/tmp/eink_image%d.bmp", order);
	save_gray_image_as_bmp((u8 *)buf, file_name, width, height);
	order++;
}

void eink_save_last_img(char *buf, u32 width, u32 height)
{
	char file_name[256] = {0};
	static int order;

	if (buf == NULL) {
		sunxi_err(NULL, "input param is null\n");
		return;
	}

	sunxi_info(NULL, "[%s]: ", __func__);
	memset(file_name, 0, sizeof(file_name));
	snprintf(file_name, sizeof(file_name), "/tmp/last_img%d.bmp", order);
	save_gray_image_as_bmp((u8 *)buf, file_name, width, height);
	order++;
}

void eink_save_current_img(char *buf, u32 width, u32 height)
{
	char file_name[256] = {0};
	static int order;

	if (buf == NULL) {
		sunxi_err(NULL, "input param is null\n");
		return;
	}

	sunxi_info(NULL, "[%s]: ", __func__);
	memset(file_name, 0, sizeof(file_name));
	snprintf(file_name, sizeof(file_name), "/tmp/cur_img%d.bmp", order);
	save_gray_image_as_bmp((u8 *)buf, file_name, width, height);
	order++;
}

int eink_get_default_file_from_mem(__u8 *buf, char *file_name, __u32 length, loff_t pos)
{
	struct file *fp = NULL;
	mm_segment_t fs;
	__s32 read_len = 0;
	ssize_t ret = 0;

	if ((buf == NULL) || (file_name == NULL)) {
		sunxi_err(NULL, "%s: buf or file_name is null\n", __func__);
		return -1;
	}

	EINK_INFO_MSG("\n");
	fp = filp_open(file_name, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		sunxi_err(NULL, "%s: fail to open file(%s), ret=0x%p\n",
				__func__, file_name, fp);
		return -1;
	}
	fs = get_fs();
	set_fs(KERNEL_DS);

	read_len = vfs_read(fp, buf, length, &pos);
	if (read_len != length) {
		sunxi_err(NULL, "maybe miss some data(read=%d byte, file=%d byte)\n",
				read_len, length);
		ret = -EAGAIN;
	}
	set_fs(fs);
	filp_close(fp, NULL);

	return ret;
}

void save_waveform_to_mem(u32 order, u8 *buf, u32 frames, u32 bit_num)
{
	char file_name[256] = {0};
	u32 len = 0, per_size = 0;

	if (buf == NULL) {
		sunxi_err(NULL, "%s:input param is null\n", __func__);
		return;
	}
	if (bit_num == 5)
		per_size = 1024;
	else
		per_size = 256;

	len = frames * per_size;

	sunxi_info(NULL, "[%s] size = (%d x %d) = %d\n", __func__, frames, bit_num, len);
	memset(file_name, 0, sizeof(file_name));
	snprintf(file_name, sizeof(file_name), "/tmp/waveform%d.bin", order);
	save_as_bin_file(buf, file_name, len, 0);
}

void save_rearray_waveform_to_mem(u8 *buf, u32 len)
{
	char file_name[256] = {0};

	if (buf == NULL) {
		sunxi_err(NULL, "%s:input param is null\n", __func__);
		return;
	}
	sunxi_info(NULL, "%s:len is %d\n", __func__, len);

	memset(file_name, 0, sizeof(file_name));
	snprintf(file_name, sizeof(file_name), "/tmp/waveform_rearray.bin");
	save_as_bin_file(buf, file_name, len, 0);
}

#ifdef OFFLINE_SINGLE_MODE
void save_one_wavedata_buffer(u8 *buf, bool is_edma)
{
	struct wavedata_queue *queue = NULL;
	/* u8 *buf = NULL; */
	char file_name[256] = {0};
	static u32 dec_id = 0, edma_id;
	u32 data_len = 0;
	u32 vsync = 0, hsync = 0;
	struct eink_manager *mgr = get_eink_manager();
	struct pipe_manager *pipe_mgr = get_pipeline_manager();

	if (pipe_mgr == NULL) {
		sunxi_err(NULL, "%s;pipe mgr is NULL\n", __func__);
		return;
	}

	hsync = mgr->timing_info.lbl + mgr->timing_info.lsl + mgr->timing_info.ldl + mgr->timing_info.lel;
	vsync = mgr->timing_info.fbl + mgr->timing_info.fsl + mgr->timing_info.fdl + mgr->timing_info.fel;

	if (mgr->panel_info.data_len == 8) {
		data_len = hsync * vsync;
	} else {
		data_len = hsync * vsync * 2;
	}

	queue = &pipe_mgr->wavedata_ring_buffer;
	memset(file_name, 0, sizeof(file_name));
	if (is_edma) {
		snprintf(file_name, sizeof(file_name), "/tmp/edma_wf_data%d.bin", edma_id);
		/* buf = (void*)queue->wavedata_vaddr[0]; */
		edma_id++;
	} else {
		snprintf(file_name, sizeof(file_name), "/tmp/dc_wf_data%d.bin", dec_id);
		/* buf = (void*)queue->wavedata_vaddr[0]; */
		dec_id++;
	}

	save_as_bin_file(buf, file_name, data_len, 0);
}

#if defined(__DISP_TEMP_CODE__)
static void dump_wavedata_buffer(struct wavedata_queue *queue)
{
	int i = 0;
	unsigned long flags = 0;
	unsigned int head = 0, tail = 0, tmp_tail = 0, tmp_head = 0;
	enum wv_buffer_state buffer_state[WAVE_DATA_BUF_NUM];

	spin_lock_irqsave(&queue->slock, flags);
	head = queue->head;
	tail = queue->tail;
	tmp_tail = queue->tmp_tail;
	tmp_head = queue->tmp_head;

	for (i = 0; i < WAVE_DATA_BUF_NUM; i++)
		buffer_state[i] = queue->buffer_state[i];
	spin_unlock_irqrestore(&queue->slock, flags);

	EINK_INFO_MSG("head=%d, tail=%d, tmp_head=%d, tmp_tail=%d\n", head, tail, tmp_head, tmp_tail);
	for (i = 0; i < WAVE_DATA_BUF_NUM; i++) {
		if (buffer_state[i] != WV_INIT_STATE) {
			sunxi_info(NULL, "miss buffer %d, state=%d\n", i, buffer_state[i]);
		}
	}
}
#endif

void save_all_wavedata_buffer(struct eink_manager *mgr, u32 all_total_frames)
{
	struct wavedata_queue *queue = NULL;
	u8 *buf = NULL;
	char file_name[256] = {0};
	u32 id = 0;
	u32 data_len = 0;
	u32 vsync = 0, hsync = 0;
	struct pipe_manager *pipe_mgr = get_pipeline_manager();

	if (pipe_mgr == NULL) {
		sunxi_err(NULL, "%s;pipe mgr is NULL\n", __func__);
		return;
	}

	if (all_total_frames > WAVE_DATA_BUF_NUM) {
		sunxi_err(NULL, "too many frames(%d), not to save\n ", all_total_frames);
		return;
	}

	hsync = mgr->timing_info.lbl + mgr->timing_info.lsl + mgr->timing_info.ldl + mgr->timing_info.lel;
	vsync = mgr->timing_info.fbl + mgr->timing_info.fsl + mgr->timing_info.fdl + mgr->timing_info.fel;

	if (mgr->panel_info.data_len == 8) {
		data_len = hsync * vsync;
	} else {
		data_len = hsync * vsync * 2;
	}

	EINK_INFO_MSG("save wavedata, total_frames=%d\n", all_total_frames);
	queue = &pipe_mgr->wavedata_ring_buffer;
	for (id = 0; id < all_total_frames; id++) {
		memset(file_name, 0, sizeof(file_name));
		snprintf(file_name, sizeof(file_name), "/tmp/wvdata_%d.bin", id);
		buf = (void *)queue->wavedata_vaddr[id];
		save_as_bin_file(buf, file_name, data_len, 0);
	}
}
#endif
