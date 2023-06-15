/*
 * Copyright (c) 2020-2031 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "di_client.h"
#include "../common/di_utils.h"
#include "../common/di_debug.h"
#include "sunxi_di.h"

#include <linux/kernel.h>
#include <linux/slab.h>

#define DI_TNR_BUF_ALIGN_LEN 16
#define DI_MD_BUF_ALIGN_LEN 32

extern int di_drv_get_version(struct di_version *version);
extern int di_drv_client_inc(struct di_client *c);
extern int di_drv_client_dec(struct di_client *c);
extern int di_drv_is_valid_client(struct di_client *c);
extern int di_drv_process_fb(struct di_client *c);

static int di_client_alloc_mbuf(struct di_mapped_buf **mbuf, u32 size)
{
	struct di_mapped_buf *p = *mbuf;

	if (p != NULL) {
		u32 size_alloced = PAGE_ALIGN(size);

		if (p->size_alloced != size_alloced) {
			di_dma_buf_unmap_free(p);
			p = NULL;
			*mbuf = NULL;
		} else {
			p->size_request = size;
		}
	}
	if (p == NULL) {
		p = di_dma_buf_alloc_map(size);
		if (p == NULL)
			return -ENOMEM;
	}
	memset((void *)p->vir_addr, 0, p->size_alloced);
	*mbuf = p;

	return 0;
}

static void di_client_free_mbuf(struct di_mapped_buf **mbuf)
{
	if ((*mbuf)->size_alloced) {
		di_dma_buf_unmap_free(*mbuf);
		*mbuf = NULL;
	}
}

static int di_client_setup_md_buf(struct di_client *c)
{
	return di_client_alloc_mbuf(&c->md_buf,
		DI_ALIGN(c->fb_arg.size.width, 2)
		* DI_ALIGN(c->fb_arg.size.height, 4));
}

static int di_client_init(struct di_client *c)
{
	DI_DEBUG("%s: %s\n", c->name, __func__);

	c->proc_fb_seqno = 0;
	atomic_set(&c->wait_con, DI_PROC_STATE_IDLE);

	return 0;
}

/*checking and correcting di_client para before running proccess_fb*/
int di_client_check_para(struct di_client *c, void *data)
{
	DI_DEBUG("%s: %s\n", c->name, __func__);

	if ((c->fb_arg.size.height == 0)
		|| ((c->fb_arg.size.height & 0x1) != 0)
		|| (c->fb_arg.size.width == 0)
		|| ((c->fb_arg.size.width & 0x1) != 0)) {
		DI_ERR("%s: invalid size W(%d)xH(%d)\n",
			c->name, c->fb_arg.size.height,
			c->fb_arg.size.width);
		goto err_out;
	}

	atomic_set(&c->wait_con, DI_PROC_STATE_IDLE);

	return 0;

err_out:
	return -EINVAL;
}

static bool di_client_check_fb_arg(struct di_client *c,
	struct di_process_fb_arg *fb_arg)
{
	DI_DEBUG("%s topFieldFirst[%s]\n",
		fb_arg->is_interlace ? "Interlace" : "P",
		fb_arg->field_order ? "Y" : "N");

	/* TODO: add more check ? */
	return true;
}

static int di_client_get_fb(struct di_client *c,
	struct di_dma_item **dma_fb_item, struct di_fb *fb,
	enum dma_data_direction dir)
{
	struct di_dma_item *dma_fb = *dma_fb_item;

	if ((fb->dma_buf_fd <= 0) && (fb->buf.addr.y_addr == 0))
		return 0;

	DI_DEBUG("%s: buf_offset[0x%llx,0x%llx,0x%llx],"
		"fd=%d,size[%d,%d]\n",
		c->name,
		fb->buf.offset.y_offset, fb->buf.offset.cb_offset,
		fb->buf.offset.cr_offset,
		fb->dma_buf_fd,
		fb->size.width, fb->size.height);

	if (dma_fb != NULL) {
		di_dma_buf_self_unmap(dma_fb);
		dma_fb = NULL;
	}

	if (fb->dma_buf_fd > 0) {
		dma_fb = di_dma_buf_self_map(fb->dma_buf_fd, dir);
		if (dma_fb == NULL) {
			DI_ERR("%s: %s,%d\n", c->name, __func__, __LINE__);
			return -EINVAL;
		}
		fb->buf.addr.y_addr = (u64)(dma_fb->dma_addr)
					+ fb->buf.offset.y_offset;
		fb->buf.addr.cb_addr = (u64)(dma_fb->dma_addr)
					+ fb->buf.offset.cb_offset;
		if (fb->buf.offset.cr_offset)
			fb->buf.addr.cr_addr = (u64)(dma_fb->dma_addr)
					+ fb->buf.offset.cr_offset;
		DI_DEBUG("%s:dma_addr=0x%llx,yuv[0x%llx,0x%llx,0x%llx]\n",
			c->name, (u64)(dma_fb->dma_addr),
			fb->buf.addr.y_addr,
			fb->buf.addr.cb_addr,
			fb->buf.addr.cr_addr);
	} else if (fb->buf.addr.y_addr && fb->buf.addr.cb_addr) {
		DI_DEBUG("%s: On phy_addr_buf method\n", c->name);
	}

	*dma_fb_item = dma_fb;
	DI_DEBUG("\n");

	return 0;
}

static int di_client_get_fbs(struct di_client *c)
{
	struct di_process_fb_arg *fb_arg = &c->fb_arg;

	if (di_client_get_fb(c, &c->in_fb0,
			&fb_arg->in_fb0, DMA_TO_DEVICE)) {
		DI_ERR("di_client_get_fb:in_fb0 failed\n");
		return -EINVAL;
	}
	if (di_client_get_fb(c, &c->in_fb1,
			&fb_arg->in_fb1, DMA_TO_DEVICE)) {
		DI_ERR("di_client_get_fb:in_fb1 failed\n");
		return -EINVAL;
	}
	if (di_client_get_fb(c, &c->in_fb2,
			&fb_arg->in_fb2, DMA_TO_DEVICE)) {
		DI_ERR("di_client_get_fb:in_fb2 failed\n");
		return -EINVAL;
	}
	if (di_client_get_fb(c, &c->out_fb0,
			&fb_arg->out_fb0, DMA_FROM_DEVICE)) {
		DI_ERR("di_client_get_fb:out_fb0 failed\n");
		return -EINVAL;
	}
	if (di_client_get_fb(c, &c->out_fb1,
			&fb_arg->out_fb1, DMA_FROM_DEVICE)) {
		DI_ERR("di_client_get_fb:out_fb1 failed\n");
		return -EINVAL;
	}

	if (fb_arg->tnr_mode.mode > 0) {
		if (di_client_get_fb(c, &c->out_tnr_fb,
				&fb_arg->out_tnr_fb, DMA_FROM_DEVICE)) {
			DI_ERR("di_client_get_fb:out_tnr_fb failed\n");
			return -EINVAL;
		}
	}

	return 0;
}

static int di_client_put_fbs(struct di_client *c)
{
	if (c->in_fb0 != NULL) {
		di_dma_buf_self_unmap(c->in_fb0);
		c->in_fb0 = NULL;
	}

	if (c->in_fb1 != NULL) {
		di_dma_buf_self_unmap(c->in_fb1);
		c->in_fb1 = NULL;
	}
	if (c->in_fb2 != NULL) {
		di_dma_buf_self_unmap(c->in_fb2);
		c->in_fb2 = NULL;
	}

	if (c->out_fb0 != NULL) {
		di_dma_buf_self_unmap(c->out_fb0);
		c->out_fb0 = NULL;
	}

	if (c->out_fb1 != NULL) {
		di_dma_buf_self_unmap(c->out_fb1);
		c->out_fb1 = NULL;
	}

	if (c->out_tnr_fb != NULL) {
		di_dma_buf_self_unmap(c->out_tnr_fb);
		c->out_tnr_fb = NULL;
	}

	return 0;
}

static bool di_client_workmode_is_change(struct di_client *c,
			struct di_process_fb_arg *fb_arg)
{
	struct di_process_fb_arg *current_fb_arg = &c->fb_arg;

	if ((fb_arg->is_interlace != current_fb_arg->is_interlace)
		|| (fb_arg->field_order != current_fb_arg->field_order)
		|| (fb_arg->pixel_format != current_fb_arg->pixel_format)
		|| (fb_arg->size.width != current_fb_arg->size.width)
		|| (fb_arg->size.height != current_fb_arg->size.height))
		return true;

	return false;
}


static void di_dump_fb_arg(struct di_process_fb_arg *arg)
{
	DI_INFO("is_interlace:%u field_prder:%u\n", arg->is_interlace, arg->field_order);
	DI_INFO("SIZE:(%ux%u) format:%u\n", arg->size.width, arg->size.height, arg->pixel_format);
	DI_INFO("DIMODE:%u output_mode:%u\n", arg->di_mode, arg->output_mode);
	DI_INFO("tnr:mode:%u level:%u\n", arg->tnr_mode.mode, arg->tnr_mode.level);

	DI_INFO("\ninFB0:\n");
	if (arg->in_fb0.dma_buf_fd > 0) {
		DI_INFO("dmafd:%d offset[%llu, %llu, %llu]\n",
			arg->in_fb0.dma_buf_fd,
			arg->in_fb0.buf.offset.y_offset,
			arg->in_fb0.buf.offset.cb_offset,
			arg->in_fb0.buf.offset.cr_offset);
	} else {
		DI_INFO("addr[%llu, %llu, %llu]\n",
			arg->in_fb0.buf.addr.y_addr,
			arg->in_fb0.buf.addr.cb_addr,
			arg->in_fb0.buf.addr.cr_addr);
	}

	DI_INFO("\ninFB1:\n");
	if (arg->in_fb1.dma_buf_fd > 0) {
		DI_INFO("dmafd:%d offset[%llu, %llu, %llu]\n",
			arg->in_fb1.dma_buf_fd,
			arg->in_fb1.buf.offset.y_offset,
			arg->in_fb1.buf.offset.cb_offset,
			arg->in_fb1.buf.offset.cr_offset);
	} else {
		DI_INFO("addr[%llu, %llu, %llu]\n",
			arg->in_fb1.buf.addr.y_addr,
			arg->in_fb1.buf.addr.cb_addr,
			arg->in_fb1.buf.addr.cr_addr);
	}

	DI_INFO("\noutFB0:\n");
	if (arg->out_fb0.dma_buf_fd > 0) {
		DI_INFO("dmafd:%d offset[%llu, %llu, %llu]\n",
			arg->out_fb0.dma_buf_fd,
			arg->out_fb0.buf.offset.y_offset,
			arg->out_fb0.buf.offset.cb_offset,
			arg->out_fb0.buf.offset.cr_offset);
	} else {
		DI_INFO("addr[%llu, %llu, %llu]\n",
			arg->out_fb0.buf.addr.y_addr,
			arg->out_fb0.buf.addr.cb_addr,
			arg->out_fb0.buf.addr.cr_addr);
	}

	DI_INFO("\noutFB1:\n");
	if (arg->out_fb1.dma_buf_fd > 0) {
		DI_INFO("dmafd:%d offset[%llu, %llu, %llu]\n",
			arg->out_fb1.dma_buf_fd,
			arg->out_fb1.buf.offset.y_offset,
			arg->out_fb1.buf.offset.cb_offset,
			arg->out_fb1.buf.offset.cr_offset);
	} else {
		DI_INFO("addr[%llu, %llu, %llu]\n",
			arg->out_fb1.buf.addr.y_addr,
			arg->out_fb1.buf.addr.cb_addr,
			arg->out_fb1.buf.addr.cr_addr);
	}
}

int di_client_process_fb(struct di_client *c,
	struct di_process_fb_arg *fb_arg)
{
	int ret = 0;
	bool is_change = false;
	ktime_t time;
	unsigned long long t0 = 0, t1 = 0, t2 = 0, t3 = 0;

	di_dump_fb_arg(fb_arg);

	time = ktime_get();
	t0 = ktime_to_us(time);

	if (!di_client_check_fb_arg(c, fb_arg)) {
		DI_ERR("%s: check_fb_arg fail\n", c->name);
		return -EINVAL;
	}

	is_change = di_client_workmode_is_change(c, fb_arg);
	if (is_change) {
		DI_INFO("di workmode changes, init client\n");
		di_client_init(c);
	}

	memcpy((void *)&c->fb_arg, fb_arg, sizeof(c->fb_arg));

	if (is_change && (fb_arg->di_mode == DI_INTP_MODE_MOTION)) {
		DI_INFO("di workmode changes, creat and setup md flag buf\n");
		di_client_setup_md_buf(c);
	}

	ret = di_client_get_fbs(c);
	if (ret < 0) {
		DI_ERR("di_client_get_fbs failed!\n");
		return ret;
	}

	time = ktime_get();
	t1 = ktime_to_us(time);

	ret = di_drv_process_fb(c);

	time = ktime_get();
	t2 = ktime_to_us(time);

	di_client_put_fbs(c);

	time = ktime_get();
	t3 = ktime_to_us(time);

	DI_TEST("total:%lluus     t0~t1:%lluus  t1~t2:%lluus  t2~t3:%lluus\n",
		(t3 - t0),
		(t1 - t0),
		(t2 - t1),
		(t3 - t2));
	return ret;
}

int di_client_get_version(struct di_client *c,
	struct di_version *version)
{
	return di_drv_get_version(version);
}

int di_client_set_timeout(struct di_client *c,
	struct di_timeout_ns *timeout)
{
	DI_DEBUG("%s:wait4start=%llu,wait4finish=%llu\n",
		c->name, timeout->wait4start, timeout->wait4finish);
	if (timeout->wait4start > 0)
		c->timeout.wait4start = timeout->wait4start;
	if (timeout->wait4finish > 0)
		c->timeout.wait4finish = timeout->wait4finish;

	return 0;
}

void *di_client_create(const char *name)
{
	struct di_client *client;

	if (!name) {
		DI_ERR("%s: Name cannot be null\n", __func__);
		return NULL;
	}

	client = kzalloc(sizeof(*client), GFP_KERNEL);
	if (client == NULL) {
		DI_ERR("kzalloc for client%s fail\n", name);
		return NULL;
	}

	client->name = kstrdup(name, GFP_KERNEL);
	if (client->name == NULL) {
		kfree(client);
		DI_ERR("kstrdup for name(%s) fail\n", name);
		return NULL;
	}

	INIT_LIST_HEAD(&client->node);

	client->timeout.wait4start = 3 * 1000000000UL;
	client->timeout.wait4finish = 3 * 1000000000UL;

	init_waitqueue_head(&client->wait);
	atomic_set(&client->wait_con, DI_PROC_STATE_IDLE);

	if (di_drv_client_inc(client)) {
		kfree(client);
		return NULL;
	}

	return client;
}
EXPORT_SYMBOL_GPL(di_client_create);

void di_client_destroy(void *client)
{
	struct di_client *c = (struct di_client *)client;

	if (!di_drv_is_valid_client(c)) {
		DI_ERR("%s, invalid client(%p)\n", __func__, c);
		return;
	}
	di_drv_client_dec(c);

	if (c->md_buf)
		di_client_free_mbuf(&c->md_buf);

	kfree(c->name);
	kfree(c);
}
EXPORT_SYMBOL_GPL(di_client_destroy);

int di_client_mem_request(struct di_client *c, void *data)
{
	struct di_mem_arg *mem = (struct di_mem_arg *)data;

	mem->handle = di_mem_request(mem->size, &mem->phys_addr);

	if (mem->handle < 0)
		return -1;
	return 0;
}
EXPORT_SYMBOL_GPL(di_client_mem_request);

int di_client_mem_release(struct di_client *c, void *data)
{
	struct di_mem_arg *mem = (struct di_mem_arg *)data;

	return mem->handle = di_mem_release(mem->handle);
}
EXPORT_SYMBOL_GPL(di_client_mem_release);
