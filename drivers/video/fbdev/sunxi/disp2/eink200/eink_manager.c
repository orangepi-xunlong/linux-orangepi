/*
 * Copyright (C) 2019 Allwinnertech, <liuli@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include "include/eink_driver.h"
#include "include/fmt_convert.h"
#include "include/eink_sys_source.h"
#include "lowlevel/eink_reg.h"
#include "libeink.h"
#include <asm/neon.h>

static struct eink_manager *g_eink_manager;
extern u32 force_temp;


#ifdef VIRTUAL_REGISTER
static void *test_preg_base;
static void *test_vreg_base;
#endif

struct eink_manager *get_eink_manager(void)
{
	return g_eink_manager;
}

void eink_free_unused_node(struct buf_manager *buf_mgr, struct img_node *img_node, int upd_master)
{
	if ((img_node == NULL) || (img_node->img == NULL)) {
		pr_warn("[%s]:process node is NULL or img is NULL\n", __func__);
		return;
	}

	EINK_DEBUG_MSG("prcessing node 0x%lx, upd_master = %d, coll_flag = 0x%llx\n",
			(unsigned long)img_node, upd_master, img_node->coll_flag);

	if (upd_master == 0) {
		buf_mgr->dequeue_image(buf_mgr, img_node);
		EINK_DEBUG_MSG("Free img node order=%d\n", img_node->upd_order);
	} else {
		buf_mgr->remove_img_from_coll_list(buf_mgr, img_node);
		EINK_DEBUG_MSG("Free coll node order=%d\n", img_node->upd_order);
	}
	EINK_DEBUG_MSG("finish!\n");

	return;
}

static int eink_get_vcom_file(char *path, int *vcom)
{
	struct file *fp = NULL;
	char vcombuf[64] = {0};
	__s32 file_len = 0;
	__s32 read_len = 0;
	mm_segment_t fs;
	loff_t pos;
	int nVcomVoltage = 0;
	int ret = 0;
	struct kstat stat;

	if ((vcom == NULL) || (path == NULL)) {
		pr_err("input param is null\n");
		return -1;
	}

	fp = filp_open(path, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		pr_err("VCOM: fail to open %s\n", path);
		return -2;
	}

	/* get vcom voltage */
	fs = get_fs();
	set_fs(KERNEL_DS);
	pos = 0;
	ret = vfs_stat(path, &stat);
	if (ret)
		pr_err("fail to get %s'stat:%d\n", path, ret);
	file_len = stat.size;

	pr_debug("file-len=%d\n", file_len);

	read_len = kernel_read(fp, (char *)vcombuf, file_len, &pos);
	if (read_len != file_len) {
		pr_err("read file(%s) error(read=%d byte, file=%d byte)\n", path, read_len, file_len);
		ret = -3;
		goto out;
	}

	nVcomVoltage = atoi_float(vcombuf);
	*vcom = nVcomVoltage;
	EINK_INFO_MSG("get nVcomVoltage = %d\n", nVcomVoltage);

	ret = 0;

out:
	if (fp) {
		filp_close(fp, NULL);
		set_fs(fs);
	}
	return ret;
}

static int eink_set_vcom_voltage(int vcom)
{
	int ret = -1;
#if IS_ENABLED(CONFIG_TPS65185_VCOM)
	ret = tps65185_vcom_set(vcom);		/* unit is mv */
	if (ret != 0) {
		pr_err("%s: set vcom fail, ret=%d\n", __func__, ret);
	}
#else
	/* other method */
#endif

	return ret;
}

static int eink_get_vcom_config(struct eink_manager *mgr, char *path)
{
	int vcom = 0;
	int ret  = -1;

	ret = eink_get_vcom_file(path, &vcom);
	if (ret) {
		pr_warn("Try to open vcom from default dir:%s\n", DEFAULT_VCOM_PATH);
		ret = eink_get_vcom_file(DEFAULT_VCOM_PATH, &vcom);
	}
	mgr->vcom_init_flag = true;
	if (ret != 0) {
		pr_warn("fail to get vcom from file, use default value\n");
		vcom = DEFAULT_VCOM_VOLTAGE;
	}
	eink_set_vcom_voltage(vcom);
	pr_info("[%s]: vcom value is %d mV\n", __func__, vcom);

	return ret;
}

#if 0
static int eink_get_vcom_voltage(int disp)
{
	struct eink_manager *eink_mgr = get_eink_manager();

	if (eink_mgr == NULL) {
		return 0;
	}

	return eink_mgr->vcom_voltage;
}
#endif

int eink_get_temperature_thread(void *parg)
{
	struct eink_manager *manager;
#if IS_ENABLED(CONFIG_TPS65185_VCOM)
	unsigned int temperature = 28;
#endif

	manager = (struct eink_manager *)parg;
	for (;;) {
		if (kthread_should_stop()) {
			break;
		}

#if IS_ENABLED(CONFIG_TPS65185_VCOM)
		/*NOTIC: make sure you board has NTC*/
		tps65185_temperature_get(&temperature);
		if (temperature == -1) {
			pr_debug("%s: get temperature=%d fail from tps65185\n",
								__func__, temperature);
		} else {
			manager->panel_temperature = temperature;
			pr_debug("%s: get temperature=%d degree from tps65185\n",
						__func__, manager->panel_temperature);
		}
#else
		manager->panel_temperature = 28;
		pr_debug("%s: set default temperature to %d degree\n",
						__func__, manager->panel_temperature);
#endif

		msleep(1000);
	}

	return 0;
}

int detect_fresh_thread(void *parg)
{
	int ret = 0;
#ifdef OFFLINE_SINGLE_MODE
	unsigned long flags = 0;
#endif
	u32 temperature = 28;
	int pipe_id = -1;
	u32 request_fail_cnt = 0;
	struct eink_manager *eink_mgr = NULL;
	struct buf_manager *buf_mgr = NULL;
	struct pipe_manager *pipe_mgr = NULL;
	struct index_manager *index_mgr = NULL;
	struct timing_ctrl_manager *timing_ctrl_mgr = NULL;
	struct img_node *curnode = NULL;
	struct eink_img *cur_img = NULL;
	struct pipe_info_node pipe_info;

//	long timeout = 1000; /* 1000ms */

	u64 free_pipe_state = 0;

	static bool first_fresh = true;
	bool can_upd_flag = 0;

	unsigned long dbg_reg_base = 0, dbg_reg_end = 0;

	eink_mgr = (struct eink_manager *)parg;
	if (eink_mgr == NULL) {
		pr_err("%s: eink manager is not initial\n", __func__);
		return -1;
	}

	buf_mgr = eink_mgr->buf_mgr;
	pipe_mgr = eink_mgr->pipe_mgr;
	index_mgr = eink_mgr->index_mgr;
	timing_ctrl_mgr = eink_mgr->timing_ctrl_mgr;

	for (;;) {
		if (kthread_should_stop()) {
			break;
		}

		if (buf_mgr->is_upd_list_empty(buf_mgr) && buf_mgr->is_coll_list_empty(buf_mgr)) {
			msleep(1);
			continue;
		}

		free_pipe_state = pipe_mgr->get_free_pipe_state(pipe_mgr);
		can_upd_flag = buf_mgr->check_upd_coll_state(buf_mgr, free_pipe_state);
		EINK_DEBUG_MSG("free_pipe_state = 0x%llx, coll can upd_flag = %d\n",
				free_pipe_state, can_upd_flag);

		if ((can_upd_flag == false) && buf_mgr->is_upd_list_empty(buf_mgr)) {
			msleep(1);
			continue;
		}

		curnode = buf_mgr->get_img_from_coll_list(buf_mgr);
		if (curnode == NULL) {
			curnode = buf_mgr->get_img_from_upd_list(buf_mgr);
			if (curnode == NULL) {
				msleep(1);
				continue;
			} else {
				EINK_DEBUG_MSG("img node from upd list\n");
				curnode->update_master = 0;
			}
		} else {
			EINK_DEBUG_MSG("img node from coll list\n");
			curnode->update_master = 1;
		}

		if (first_fresh) {
			index_mgr->set_rmi_addr(index_mgr);
		}

		temperature = eink_mgr->get_temperature(eink_mgr);

		if (force_temp > 0) {
			temperature = force_temp;
		}

		cur_img = curnode->img;

		if (eink_get_print_level() == 5) {
			if (curnode->update_master == 1) {
				eink_save_img(curnode->img->fd, 2,
						buf_mgr->width, buf_mgr->height, curnode->upd_order, curnode->kaddr, 1);
			} else {
				printk("order = %d, kaddr = %p, curnode = %p\n", curnode->upd_order, curnode->kaddr, curnode);
				eink_save_img(curnode->img->fd, 5,
						buf_mgr->width, buf_mgr->height, curnode->upd_order, curnode->kaddr, 1);
			}
		//	eink_kunmap_img(curnode);
		}

		memset(&pipe_info, 0, sizeof(struct pipe_info_node));
		pipe_info.img = cur_img;
		memcpy(&pipe_info.upd_win, &cur_img->upd_win, sizeof(struct upd_win));
		pipe_info.upd_mode = cur_img->upd_mode;

#ifdef DRIVER_REMAP_WAVEFILE
		eink_get_wf_data(pipe_info.upd_mode, temperature,
				&pipe_info.total_frames, &pipe_info.wav_paddr,
				&pipe_info.wav_vaddr);/* 还没想好结果 fix me */
		if (pipe_info.wav_paddr == 0) {
			pr_err("wavefile not support mode 0x%x, use GC16 replace\n", pipe_info.upd_mode);
			pipe_info.upd_mode = EINK_GC16_MODE;
			eink_get_wf_data(pipe_info.upd_mode, temperature,
					&pipe_info.total_frames, &pipe_info.wav_paddr,
					&pipe_info.wav_vaddr);
		}
#else
		get_waveform_data(pipe_info.upd_mode, temperature,
				&pipe_info.total_frames, &pipe_info.wav_paddr,
				&pipe_info.wav_vaddr);
		if (pipe_info.wav_paddr == 0) {
			pr_err("wavefile not support mode 0x%x, use GC16 replace\n", pipe_info.upd_mode);
			pipe_info.upd_mode = EINK_GC16_MODE;
			get_waveform_data(pipe_info.upd_mode, temperature,
					&pipe_info.total_frames, &pipe_info.wav_paddr,
					&pipe_info.wav_vaddr);
		}
#endif

		EINK_INFO_MSG("temp=%d, mode=0x%x, total=%d, waveform_paddr=0x%x, waveform_vaddr=0x%x\n",
				temperature, pipe_info.upd_mode,
				pipe_info.total_frames, (unsigned int)pipe_info.wav_paddr,
				(unsigned int)pipe_info.wav_vaddr);

		/* confirm the wavfile read from system is right or not */
		if (eink_get_print_level() == 6) {
			save_waveform_to_mem(curnode->upd_order,
					(u8 *)pipe_info.wav_vaddr,
					pipe_info.total_frames,
					eink_mgr->panel_info.bit_num);
		}

#ifdef DEC_WAV_DEBUG
		if (eink_get_print_level() == 6) {
			pipe_mgr->pipe_mgr_config_wb(pipe_mgr, &pipe_info);
		}
#endif

#ifdef WAVEDATA_DEBUG
		/* use a right wavfile to debug */
		if (pipe_info.upd_mode == EINK_INIT_MODE) {
			EINK_INFO_MSG("get_gray_mem mode = %d\n", pipe_info.upd_mode);
			eink_get_default_file_from_mem((u8 *)pipe_info.wav_vaddr,
					DEFAULT_INIT_WAV_PATH,
					(pipe_info.total_frames * 256 / 4), 0);
		} else if (pipe_info.upd_mode == EINK_GC16_MODE) {
			EINK_INFO_MSG("get_gray_mem mode = %d\n", pipe_info.upd_mode);
			eink_get_default_file_from_mem((u8 *)pipe_info.wav_vaddr,
					DEFAULT_GC16_WAV_PATH,
					(51 * 64), 0);
		}
#endif
		pipe_id = -1;
		while (pipe_id < 0 && request_fail_cnt < REQUST_PIPE_FAIL_MAX_CNT) {
			pipe_id = pipe_mgr->request_pipe(pipe_mgr);
			if (pipe_id < 0) {
				request_fail_cnt++;
				msleep(5);
				continue;
			}
		}
		if (pipe_id < 0) {
			pr_err("Request free pipe failed!\n");
			ret = -1;
			goto err_out;
		}

		pipe_info.pipe_id = pipe_id;

		/* config pipe */
		ret = pipe_mgr->config_pipe(pipe_mgr, pipe_info);

		buf_mgr->processing_img_node = curnode; /* fix me */

		EINK_DEBUG_MSG("%s: processing_node %p, upd_order=%d\n", __func__,
					buf_mgr->processing_img_node,
					buf_mgr->processing_img_node->upd_order);
		if (eink_mgr->upd_coll_win_flag == 1)
			eink_mgr->upd_coll_win_flag = 0;
		if (eink_mgr->upd_pic_fin_flag == 1)
			eink_mgr->upd_pic_fin_flag = 0;

		/* calc time for debug before pipe en */
		if (eink_get_print_level() == 8) {
			getnstimeofday(&eink_mgr->stimer);
		}

		/* rmi debug */
		if (eink_get_print_level() == 5) {
			save_upd_rmi_buffer(curnode->upd_order,
					index_mgr->rmi_vaddr,
					buf_mgr->width * buf_mgr->height * 2, 1);

			save_index_buffer(curnode->upd_order, index_mgr->rmi_vaddr,
					buf_mgr->width * buf_mgr->height * 2,
					eink_mgr->panel_info.bit_num,
					buf_mgr->width, buf_mgr->height, 1);
		}
		/* enable pipe */
		ret = pipe_mgr->active_pipe(pipe_mgr, pipe_info.pipe_id);
		eink_mgr->ee_finish = false;

#ifdef OFFLINE_MULTI_MODE
		if (first_fresh) {
			ret = request_multi_frame_buffer(pipe_mgr, &timing_ctrl_mgr->info);
			if (ret) {
				pr_err("request buf for offline molti mode fail!\n");
				goto err_out;
			}
			EINK_INFO_MSG("request buffer paddr=%p,vaddr=%p\n", pipe_mgr->dec_wav_paddr, pipe_mgr->dec_wav_vaddr);
			eink_edma_cfg(&eink_mgr->panel_info);
			eink_edma_start();
			eink_prepare_decode((unsigned long)pipe_mgr->dec_wav_paddr, &pipe_mgr->panel_info);
		}
#elif defined OFFLINE_SINGLE_MODE
		spin_lock_irqsave(&pipe_mgr->list_lock, flags);
		EINK_INFO_MSG("OFFLINE_SINGLE_MODE!\n");
		if (pipe_mgr->ee_processing == false) {
			pipe_mgr->dec_wav_paddr =
				request_buffer_for_decode(&pipe_mgr->wavedata_ring_buffer, &pipe_mgr->dec_wav_vaddr);
			if (pipe_mgr->dec_wav_paddr == NULL) {
				pr_err("[%s]:request buf for offline single mode dec fail!\n", __func__);
				ret = -1;
				goto err_out;
			}

			EINK_INFO_MSG("request buffer paddr=0x%p,vaddr=0x%p\n",
						pipe_mgr->dec_wav_paddr,
						pipe_mgr->dec_wav_vaddr);
			eink_prepare_decode((unsigned long)pipe_mgr->dec_wav_paddr, &pipe_mgr->panel_info);
			if (!first_fresh) {
				eink_start();
			}

			pipe_mgr->ee_processing = true;
		}
		spin_unlock_irqrestore(&pipe_mgr->list_lock, flags);
#endif

		/* enable eink engine */
		EINK_INFO_MSG("Eink start!%u\n", curnode->upd_order);
		eink_start();

		/* for debug reg info */
		if (eink_get_print_level() == 4) {
			pr_info("[EINK PRINT EE REG]:-----\n");
			dbg_reg_base = eink_get_reg_base();
			EINK_INFO_MSG("reg_base = 0x%x\n", (unsigned int)dbg_reg_base);
			dbg_reg_end = dbg_reg_base + 0x3ff;
			eink_print_register(dbg_reg_base, dbg_reg_end);
		}
		wait_event_interruptible(eink_mgr->upd_pic_queue,
				((eink_mgr->upd_coll_win_flag == 1) ||
				 (eink_mgr->upd_pic_fin_flag == 1)));

		if (eink_mgr->upd_coll_win_flag == 1)
			eink_mgr->upd_coll_win_flag = 0;
		else if (eink_mgr->upd_pic_fin_flag == 1)
			eink_mgr->upd_pic_fin_flag = 0;

#if 0
		if (eink_get_print_level() == 5) {
			if (curnode->update_master == 1) {
				eink_save_img(curnode->img->fd, 2,
						buf_mgr->width, buf_mgr->height, curnode->upd_order, curnode->kaddr, 0);
			} else {
				printk("order = %d, kaddr = %p, curnode = %p\n", curnode->upd_order, curnode->kaddr, curnode);
				eink_save_img(curnode->img->fd, 5,
						buf_mgr->width, buf_mgr->height, curnode->upd_order, curnode->kaddr, 0);
			}
		//	eink_kunmap_img(curnode);
		}
		if (eink_get_print_level() == 5) {
			save_upd_rmi_buffer(curnode->upd_order,
					index_mgr->rmi_vaddr,
					buf_mgr->width * buf_mgr->height * 2, 0);
			save_index_buffer(curnode->upd_order, index_mgr->rmi_vaddr,
					buf_mgr->width * buf_mgr->height * 2,
					eink_mgr->panel_info.bit_num,
					buf_mgr->width, buf_mgr->height, 0);
		}
#endif
		/////////////////////////////////////////////////
		//
		//
		/* 释放上一幅图的buf, 第一幅图则会跳过此流程 */
		if (curnode && curnode->img) {
			switch (curnode->update_master) {
			case 0:
				eink_free_unused_node(buf_mgr, curnode, 0);
				break;
			case 1:
				eink_free_unused_node(buf_mgr, curnode, 1);
				break;
			case -1:
				EINK_DEBUG_MSG("maybr init or already merge free\n");
				break;
			default:
				pr_err("unknown update master(%d)\n", curnode->update_master);
				break;
			}
		} else
			EINK_DEBUG_MSG("First fresh or the Node is NULL\n");

		first_fresh = false;
#if 0
		wait_event_interruptible(eink_mgr->upd_pic_accept_queue,
				(eink_mgr->upd_pic_accept_flag == 1));
		/* spin_lock_irqsave();记得后面加个锁 */
		eink_mgr->upd_pic_accept_flag = 0;
		/* spin_unlock_irerestore(); */
#endif
	}
	return ret;
err_out:
	return ret;
}

 int regal_eink_process(struct eink_img *cur_img, struct eink_img *last_img)
{
#ifdef CONFIG_EINK_REGAL_PROCESS
	struct dma_buf *cur_dmabuf = NULL, *last_dmabuf = NULL;
	char *cur_addr = NULL, *last_addr = NULL;
	int ret = -1;
	/*struct timespec	stimer, etimer;*/
	/*getnstimeofday(&stimer);*/

	cur_dmabuf = dma_buf_get(cur_img->fd);
	last_dmabuf = dma_buf_get(last_img->fd);

	if (IS_ERR_OR_NULL(cur_dmabuf) || IS_ERR_OR_NULL(last_dmabuf)) {
		pr_err("dma_buf_get fail\n");
		goto OUT;
	}
	ret = dma_buf_begin_cpu_access(cur_dmabuf, DMA_BIDIRECTIONAL);
	cur_addr = dma_buf_kmap(cur_dmabuf, 0);
	if (!cur_addr) {
		pr_err("dma_buf_kmap cur fail!\n");
		goto FREE_DMABUF;
	}
	ret = dma_buf_begin_cpu_access(last_dmabuf, DMA_BIDIRECTIONAL);
	last_addr = dma_buf_kmap(last_dmabuf, 0);
	if (!last_addr) {
		pr_err("dma_buf_kmap last_dmabuf fail!\n");
		goto FREE_CUR;
	}
	kernel_neon_begin();
	eink_process_neon(cur_addr, last_addr, cur_img->pitch, cur_img->size.height);
	kernel_neon_end();
	/*getnstimeofday(&etimer);*/
	ret = 0;

	dma_buf_kunmap(last_dmabuf, 0, last_addr);
	dma_buf_end_cpu_access(last_dmabuf, DMA_BIDIRECTIONAL);
FREE_CUR:
	dma_buf_kunmap(cur_dmabuf, 0, cur_addr);
	dma_buf_end_cpu_access(cur_dmabuf, DMA_BIDIRECTIONAL);
FREE_DMABUF:
	dma_buf_put(last_dmabuf);
	dma_buf_put(cur_dmabuf);
OUT:
	/*pr_err("regal process take %d ms\n",*/
		/*get_delt_ms_timer(stimer, etimer));*/
	return ret;
#else
	return 0;
#endif
}

int eink_fmt_convert_image(struct disp_layer_config_inner *config, unsigned int layer_num,
		struct eink_img *last_img, struct eink_img *cur_img)
{
	int ret = 0, sel = 0;
	struct fmt_convert_manager *cvt_mgr = NULL;
	struct eink_manager *eink_mgr = get_eink_manager();

	if ((eink_mgr == NULL) || (config == NULL) || (last_img == NULL) || (cur_img == NULL)) {
		pr_err("[%s]:some parm is NULL 0x%p 0x%p 0x%p 0x%p!\n",
		       __func__, eink_mgr, config, last_img, cur_img);
		return -1;
	}
	mutex_lock(&eink_mgr->standby_lock);
	if (eink_mgr->suspend_state || eink_mgr->shutdown_state) {
		mutex_unlock(&eink_mgr->standby_lock);
		pr_warn("[%s]:eink is suspend(%d)or shutdown(%d), don't update\n",
			__func__, eink_mgr->suspend_state, eink_mgr->shutdown_state);
		return -1;
	}

	cvt_mgr = get_fmt_convert_mgr(sel);

	ret = cvt_mgr->enable(sel);
	if (ret) {
		pr_err("%s:enable convert failed\n", __func__);
		mutex_unlock(&eink_mgr->standby_lock);
		return ret;
	}

	/* used DE hardware to convert 32bpp to 8bpp */
	ret = cvt_mgr->start_convert(sel, config, layer_num, last_img, cur_img);
	if (ret < 0) {
		pr_err("%s: fmt convert failed!\n", __func__);
		mutex_unlock(&eink_mgr->standby_lock);
		return ret;
	}

	if (cur_img->upd_mode == EINK_GLD16_MODE ||
	    cur_img->upd_mode == EINK_GLR16_MODE) {
		regal_eink_process(cur_img, last_img);
	}
	mutex_unlock(&eink_mgr->standby_lock);

	/* for debug capture wb data */
	if (eink_get_print_level() == 7) {
		eink_save_img(cur_img->fd, 1, cur_img->size.width,
				cur_img->size.height, 0, NULL, 0);
	}

	return ret;
}

int eink_update_image(struct eink_manager *eink_mgr, struct eink_upd_cfg *upd_cfg)
{
	int ret = 0;

	EINK_DEBUG_MSG("func input!\n");

	mutex_lock(&eink_mgr->standby_lock);
	if (eink_mgr->suspend_state || eink_mgr->shutdown_state) {
		mutex_unlock(&eink_mgr->standby_lock);
		pr_warn("[%s]:eink is suspend(%d)or shutdown(%d), don't update\n",
			__func__, eink_mgr->suspend_state, eink_mgr->shutdown_state);
		return -1;
	}

	if (eink_mgr->waveform_init_flag == false) {
		ret = waveform_mgr_init(eink_mgr->wav_path, eink_mgr->panel_info.bit_num);
		if (ret) {
			mutex_unlock(&eink_mgr->standby_lock);
			pr_err("%s:init waveform failed!\n", __func__);
			return ret;
		} else
			eink_mgr->waveform_init_flag = true;

	}

	if (false == eink_mgr->vcom_init_flag) {
		eink_mgr->vcom_voltage = eink_get_vcom_config(eink_mgr, eink_mgr->vcom_path);
	}

	ret = eink_mgr->eink_mgr_enable(eink_mgr);
	if (ret) {
		mutex_unlock(&eink_mgr->standby_lock);
		pr_err("enable eink mgr failed, ret = %d\n", ret);
		return -1;
	}

	ret = eink_mgr->buf_mgr->queue_image(eink_mgr->buf_mgr, upd_cfg);

	mutex_unlock(&eink_mgr->standby_lock);
	return ret;

}

int upd_pic_accept_irq_handler(void)
{
	struct eink_manager *mgr = g_eink_manager;

	EINK_DEBUG_MSG("\n");
	mgr->upd_pic_accept_flag = 1;
	wake_up_interruptible(&(mgr->upd_pic_accept_queue));

	return 0;
}

int upd_pic_finish_irq_handler(struct buf_manager *mgr)
{
	struct eink_manager *eink_mgr = get_eink_manager();

	EINK_DEBUG_MSG("\n");

	eink_mgr->upd_pic_fin_flag = 1;
	wake_up_interruptible(&eink_mgr->upd_pic_queue);

	return 0;
}

void upd_coll_win_irq_handler(struct work_struct *work)
{
	struct eink_manager *eink_mgr = get_eink_manager();
	struct buf_manager *mgr = eink_mgr->buf_mgr;
	struct pipe_manager *pipe_mgr = eink_mgr->pipe_mgr;
	struct upd_win *win = NULL;
	struct img_node *img_node = NULL;
	u32 status0 = 0;
	u64 status1 = 0;
	unsigned long dbg_reg_base = 0, dbg_reg_end = 0;

	u32 max_pipe_cnt = pipe_mgr->max_pipe_cnt;

	img_node = mgr->processing_img_node;
	if (img_node == NULL || !img_node->img) {
		pr_err("%s:something wrong!\n", __func__);
		upd_pic_finish_irq_handler(mgr);
		return;
	}
	EINK_INFO_MSG("COLL node addr is 0x%p, order=%d\n", img_node, img_node->upd_order);

	win = &mgr->processing_img_node->img->upd_win;
	eink_get_coll_win(win);

	EINK_INFO_MSG("COLL upd_win: (%d, %d)~(%d, %d)\n",
				win->left, win->top, win->right, win->bottom);

	if (eink_get_print_level() == 4) {
		pr_info("[EINK PRINT EE REG]:-----\n");
		dbg_reg_base = eink_get_reg_base();
		EINK_INFO_MSG("reg_base = 0x%x\n", (unsigned int)dbg_reg_base);
		dbg_reg_end = dbg_reg_base + 0x3ff;
		eink_print_register(dbg_reg_base, dbg_reg_end);
	}

	status0 = eink_get_coll_status0();
	img_node->coll_flag = status0;


	if (max_pipe_cnt == 64) {
		status1 = eink_get_coll_status1();
		img_node->coll_flag |= (status1 << 32);
	}

	EINK_INFO_MSG("coll status0=0x%x, status1=0x%llx, coll flag = 0x%llx\n",
					status0, status1, img_node->coll_flag);

	if (img_node->coll_flag) {
		mgr->add_img_to_coll_list(mgr, img_node);
	} else
		pr_err("%s:coll flag = 0 wrong\n", __func__);

	eink_mgr->upd_coll_win_flag = 1;
	wake_up_interruptible(&(eink_mgr->upd_pic_queue));

	return;
}

int pipe_finish_irq_handler(struct pipe_manager *mgr)
{
	unsigned long flags = 0;
	u32 pipe_cnt = 0, pipe_id = 0;
	u64 finish_val = 0;
	struct pipe_info_node *cur_pipe = NULL, *tmp_pipe = NULL;

	pipe_cnt = mgr->max_pipe_cnt;

	spin_lock_irqsave(&mgr->list_lock, flags);

	finish_val = eink_get_fin_pipe_id();
	EINK_DEBUG_MSG("Pipe finish val = 0x%llx\n", finish_val);

	for (pipe_id = 0; pipe_id < pipe_cnt; pipe_id++) {
		if ((1ULL << pipe_id) & finish_val) {
			list_for_each_entry_safe(cur_pipe, tmp_pipe, &mgr->pipe_used_list, node) {
				if (cur_pipe->pipe_id != pipe_id) {
					continue;
				}

				if (mgr->release_pipe)
					mgr->release_pipe(mgr, cur_pipe);
				list_move_tail(&cur_pipe->node, &mgr->pipe_free_list);
				break;
			}
		}
	}

	eink_reset_fin_pipe_id(finish_val);

	spin_unlock_irqrestore(&mgr->list_lock, flags);
	return 0;
}

static int eink_intterupt_proc(int irq, void *parg)
{
	struct eink_manager *eink_mgr = NULL;
	struct buf_manager *buf_mgr = NULL;
	struct pipe_manager *pipe_mgr = NULL;
	int reg_val = -1;
	unsigned int ee_fin, upd_pic_accept, upd_pic_fin, pipe_fin, edma_fin, upd_coll_occur;

	eink_mgr = (struct eink_manager *)parg;
	if (eink_mgr == NULL) {
		pr_err("%s:mgr is NULL!\n", __func__);
		return EINK_IRQ_RETURN;
	}

	buf_mgr = eink_mgr->buf_mgr;
	pipe_mgr = eink_mgr->pipe_mgr;
	if ((buf_mgr == NULL) || (pipe_mgr == NULL)) {
		pr_err("buf or pipe mgr is NULL\n");
		return EINK_IRQ_RETURN;
	}

	reg_val = eink_irq_query();
	EINK_DEBUG_MSG("Enter Interrupt Proc, Reg_Val = 0x%x\n", reg_val);

	upd_pic_accept = reg_val & 0x10;
	upd_pic_fin = reg_val & 0x100;
	pipe_fin = reg_val & 0x1000;
	upd_coll_occur = reg_val & 0x100000;
	ee_fin = reg_val & 0x1000000;
	edma_fin = reg_val & 0x10000000;

	if (pipe_fin == 0x1000) {
		pipe_finish_irq_handler(pipe_mgr);

		/* debug */
		if (eink_get_print_level() == 8) {
			getnstimeofday(&eink_mgr->pipe_timer);
			pr_info("Pipe enable to pipe finish take %d ms\n",
				get_delt_ms_timer(eink_mgr->stimer, eink_mgr->pipe_timer));
		}
#ifdef DEC_WAV_DEBUG
		if (eink_get_print_level() == 6) {
			queue_work(pipe_mgr->wav_dbg_workqueue, &pipe_mgr->wav_dbg_work);
		}
#endif
	}

	if (upd_pic_accept == 0x10) {
		upd_pic_accept_irq_handler();

		/* debug */
		if (eink_get_print_level() == 8) {
			getnstimeofday(&eink_mgr->acept_timer);
			pr_info("upd accept take %d ms\n",
				get_delt_ms_timer(eink_mgr->stimer, eink_mgr->acept_timer));
		}
	}

	if (upd_pic_fin == 0x100) {
		if (upd_coll_occur == 0x100000) {
			queue_work(buf_mgr->coll_img_workqueue, &buf_mgr->coll_handle_work);
		} else {
			upd_pic_finish_irq_handler(buf_mgr);
		}

		/* debug */
		if (eink_get_print_level() == 8) {
			getnstimeofday(&eink_mgr->upd_pic_timer);
			pr_info("Pipe enable to upd pic finish take %d ms\n",
				get_delt_ms_timer(eink_mgr->stimer, eink_mgr->upd_pic_timer));
		}
	}

	if (ee_fin == 0x1000000) {
		eink_mgr->ee_finish = true;/*后面可以选择送给应用层*/

		/* calc time for debug */
		if (eink_get_print_level() == 8) {
			getnstimeofday(&eink_mgr->etimer);
			pr_info("Pipe enable to ee finish take %d ms\n",
				get_delt_ms_timer(eink_mgr->stimer, eink_mgr->etimer));
		}
#ifdef OFFLINE_SINGLE_MODE
		queue_work(pipe_mgr->dec_workqueue, &pipe_mgr->decode_ctrl_work);
#endif
	}

	if (edma_fin == 0x10000000) {
#ifdef OFFLINE_SINGLE_MODE
		queue_work(pipe_mgr->edma_workqueue, &pipe_mgr->edma_ctrl_work);
#endif
	}

	return EINK_IRQ_RETURN;
}

static int eink_set_temperature(struct eink_manager *mgr, u32 temp)
{
	int ret = 0;

	if (mgr)
		mgr->panel_temperature = temp;
	else {
		pr_err("%s:mgr is NULL!\n", __func__);
		ret = -1;
	}

	return ret;
}

s32 eink_get_resolution(struct eink_manager *mgr, u32 *xres, u32 *yres)
{
	if (mgr == NULL) {
		pr_err("[%s]: eink manager is NULL!\n", __func__);
		return -EINVAL;
	}

	*xres = mgr->panel_info.width;
	*yres = mgr->panel_info.height;
	return 0;
}

u32 eink_get_temperature(struct eink_manager *mgr)
{
	u32 temp = 28;

	if (mgr) {
		temp = mgr->panel_temperature;
	} else
		pr_err("%s: mgr is NULL!\n", __func__);

	return temp;
}

int eink_get_sys_config(struct init_para *para)
{
	int ret = 0, i = 0;
	s32 value = 0;
	char primary_key[20], sub_name[25];
	struct eink_gpio_info *gpio_info;

	struct eink_panel_info *panel_info = NULL;
	struct timing_info *timing_info = NULL;

	panel_info = &para->panel_info;
	timing_info = &para->timing_info;

	sprintf(primary_key, "eink");

	/* eink power */
	ret = eink_sys_script_get_item(primary_key, "eink_power", (int *)para->eink_power, 2);
	if (ret == 2) {
		para->power_used = 1;
	}

	/* eink panel gpio */
	for (i = 0; i < EINK_GPIO_NUM; i++) {
		sprintf(sub_name, "eink_gpio_%d", i);

		gpio_info = &(para->eink_gpio[i]);
		ret = eink_sys_script_get_item(primary_key, sub_name,
					     (int *)gpio_info, 3);
		if (ret == 3)
			para->eink_gpio_used[i] = 1;
		EINK_DEBUG_MSG("eink_gpio_%d used = %d\n", i, para->eink_gpio_used[i]);
	}

	/* eink pin power */
	ret = eink_sys_script_get_item(primary_key, "eink_pin_power", (int *)para->eink_pin_power, 2);

	/* single or dual 0 single 1 dual 2 four*/
	ret = eink_sys_script_get_item(primary_key, "eink_scan_mode", &value, 1);
	if (ret == 1) {
		panel_info->eink_scan_mode = value;
	}
	/* eink panel cfg */
	ret = eink_sys_script_get_item(primary_key, "eink_width", &value, 1);
	if (ret == 1) {
		panel_info->width = value;
	}

	ret = eink_sys_script_get_item(primary_key, "eink_height", &value, 1);
	if (ret == 1) {
		panel_info->height = value;
	}

	ret = eink_sys_script_get_item(primary_key, "eink_fresh_hz", &value, 1);
	if (ret == 1) {
		panel_info->fresh_hz = value;
	}

	ret = eink_sys_script_get_item(primary_key, "eink_gray_level", &value, 1);
	if (ret == 1) {
		panel_info->gray_level_cnt = value;
	}

/*
	ret = eink_sys_script_get_item(primary_key, "eink_sdck", &value, 1);
	if (ret == 1) {
		panel_info->sdck = value;
	}
*/
	ret = eink_sys_script_get_item(primary_key, "eink_bits", &value, 1);
	if (ret == 1) {
		panel_info->bit_num = value;
	}

	ret = eink_sys_script_get_item(primary_key, "eink_data_len", &value, 1);
	if (ret == 1) {
		panel_info->data_len = value;
	}


	ret = eink_sys_script_get_item(primary_key, "eink_pwm_used", &value, 1);
	if (ret == 1)
		timing_info->backlight.pwm_info.used = value;

	if (timing_info->backlight.pwm_info.used) {
		ret = eink_sys_script_get_item(primary_key, "eink_backlight", &value, 1);
		if (ret == 1)
			timing_info->backlight.backlight_bright = value;

		ret = eink_sys_script_get_item(primary_key, "eink_pwm_ch", &value, 1);
		if (ret == 1)
			timing_info->backlight.pwm_info.channel = value;

		ret = eink_sys_script_get_item(primary_key, "eink_pwm_freq", &value, 1);
		if (ret == 1) {
			if (value != 0) {
				timing_info->backlight.pwm_info.period_ns =
				    1000 * 1000 * 1000 / value;
			} else {
				/* default 1khz */
				timing_info->backlight.pwm_info.period_ns = 1000 * 1000 * 1000 / 1000;
			}


			timing_info->backlight.pwm_info.duty_ns =
			    (timing_info->backlight.backlight_bright *
			     timing_info->backlight.pwm_info.period_ns) /
			    256;
		}
		ret = eink_sys_script_get_item(primary_key, "eink_pwm_pol", &value, 1);
		if (ret == 1)
			timing_info->backlight.pwm_info.polarity = value;

		gpio_info = &(timing_info->backlight.lcd_bl_en);
		ret = eink_sys_script_get_item(primary_key, "eink_bl_en", (int *)gpio_info, 3);
		if (ret == 3) {
			timing_info->backlight.lcd_bl_en_used = 1;
		}
	}

	/* eink timing para */
	ret = eink_sys_script_get_item(primary_key, "eink_lsl", &value, 1);
	if (ret == 1) {
		timing_info->lsl = value;
	}

	ret = eink_sys_script_get_item(primary_key, "eink_lbl", &value, 1);
	if (ret == 1) {
		timing_info->lbl = value;
	}

	ret = eink_sys_script_get_item(primary_key, "eink_lel", &value, 1);
	if (ret == 1) {
		timing_info->lel = value;
	}

	ret = eink_sys_script_get_item(primary_key, "eink_gdck_sta", &value, 1);
	if (ret == 1) {
		timing_info->gdck_sta = value;
	}

	ret = eink_sys_script_get_item(primary_key, "eink_lgonl", &value, 1);
	if (ret == 1) {
		timing_info->lgonl = value;
	}

	ret = eink_sys_script_get_item(primary_key, "eink_gdoe_start_line", &value, 1);
	if (ret == 1) {
		timing_info->gdoe_start_line = value;
	}

	ret = eink_sys_script_get_item(primary_key, "eink_fsl", &value, 1);
	if (ret == 1) {
		timing_info->fsl = value;
	}

	ret = eink_sys_script_get_item(primary_key, "eink_fbl", &value, 1);
	if (ret == 1) {
		timing_info->fbl = value;
	}

	ret = eink_sys_script_get_item(primary_key, "eink_fel", &value, 1);
	if (ret == 1) {
		timing_info->fel = value;
	}

	timing_info->ldl = (panel_info->width) / (panel_info->data_len / 2);
	timing_info->fdl = panel_info->height;

	ret = eink_sys_script_get_item(primary_key, "eink_wav_path", (int *)para->wav_path, 2);
	if (ret != 2) {
		memcpy(para->wav_path, DEFAULT_WAVEFORM_PATH, strnlen(DEFAULT_WAVEFORM_PATH, WAV_PATH_LEN));
		pr_err("[EINK]get wf path fail, set default path=%s\n", para->wav_path);
	}

	ret = eink_sys_script_get_item(primary_key, "vcom_path", (int *)para->vcom_path, 2);
	if (ret != 2) {
		memcpy(para->wav_path, DEFAULT_VCOM_PATH, strnlen(DEFAULT_VCOM_PATH, VCOM_PATH_LEN));
		pr_err("[EINK]get vcom path fail, set default path=%s\n", para->vcom_path);
	}

	return ret;
}

s32 eink_set_global_clean_cnt(struct eink_manager *mgr, u32 cnt)
{
	struct buf_manager *buf_mgr = NULL;
	s32 ret = -1;

	if (mgr == NULL) {
		pr_err("%s:eink manager is null\n", __func__);
		return -1;
	}

	buf_mgr = mgr->buf_mgr;
	if (buf_mgr == NULL) {
		pr_err("%s:buf_mgr is null\n", __func__);
		return -1;
	}

	if (mgr->buf_mgr->set_global_clean_counter)
		ret = mgr->buf_mgr->set_global_clean_counter(buf_mgr, cnt);

	return ret;
}

static void print_panel_info(struct init_para *para)
{
	struct eink_panel_info *info = &para->panel_info;
	struct timing_info *timing = &para->timing_info;

	EINK_INFO_MSG("width=%d, height=%d, fresh_hz=%d, scan_mode=%d\n",
			info->width, info->height, info->fresh_hz, info->eink_scan_mode);
	EINK_INFO_MSG("sdck=%d, bit_num=%d, data_len=%d, gray_level_cnt=%d\n",
			info->sdck, info->bit_num, info->data_len, info->gray_level_cnt);
	EINK_INFO_MSG("lsl=%d, lbl=%d, ldl=%d, lel=%d, gdck_sta=%d, lgonl=%d\n",
			timing->lsl, timing->lbl, timing->ldl, timing->lel,
			timing->gdck_sta, timing->lgonl);
	EINK_INFO_MSG("fsl=%d, fbl=%d, fdl=%d, fel=%d, gdoe_start_line=%d\n",
			timing->fsl, timing->fbl, timing->fdl, timing->fel,
			timing->gdoe_start_line);
	EINK_INFO_MSG("wavdata_path = %s\n", para->wav_path);
	EINK_INFO_MSG("vcom_path = %s\n", para->vcom_path);
}

int eink_clk_enable(struct eink_manager *mgr)
{
	int ret = 0;
	u32 vsync = 0, hsync = 0;
	struct timing_info *timing = NULL;
	u32 fresh_rate = 0;
	unsigned long panel_freq = 0, temp_freq = 0;

	if (mgr == NULL) {
		pr_err("[%s]: mgr is NULL!\n", __func__);
		return -1;
	}

	if (mgr->clk_en_flag == 1) {
		pr_info("[%s]:clk already enable\n", __func__);
		return 0;
	}

	if (mgr->ee_rst_clk) {
		ret = reset_control_deassert(mgr->ee_rst_clk);
		if (ret) {
			pr_err("[EINK] deassert failed!\n");
			return -1;
		}
	}

	if (mgr->ee_bus_clk) {
		ret = clk_prepare_enable(mgr->ee_bus_clk);
	}

	if (mgr->ee_clk) {
		ret = clk_prepare_enable(mgr->ee_clk);
	}

	timing = &mgr->timing_info;
	fresh_rate = mgr->panel_info.fresh_hz;

	hsync = timing->lsl + timing->lbl + timing->ldl + timing->lel;
	vsync = timing->fsl + timing->fbl + timing->fdl + timing->fel;
	panel_freq = fresh_rate * hsync * vsync;

	EINK_INFO_MSG("panel_freq = %lu\n", panel_freq);

	if (mgr->panel_clk) {
		ret = clk_set_rate(mgr->panel_clk, panel_freq);
		if (ret) {
			pr_err("%s:set panel freq failed!\n", __func__);
			return -1;
		}

		temp_freq = clk_get_rate(mgr->panel_clk);
		if (panel_freq != temp_freq) {
			pr_warn("%s: not set real clk, freq=%lu\n", __func__, temp_freq);
		}

		ret = clk_prepare_enable(mgr->panel_clk);
	}

	mgr->clk_en_flag = 1;
	return ret;
}

void eink_clk_disable(struct eink_manager *mgr)
{
	if (mgr->clk_en_flag == 0) {
		pr_info("[%s]:clk already disable\n", __func__);
		return;
	}
	if (mgr->ee_clk)
		clk_disable(mgr->ee_clk);

	if (mgr->ee_bus_clk)
		clk_disable(mgr->ee_bus_clk);

	if (mgr->panel_clk)
		clk_disable(mgr->panel_clk);

	if (mgr->ee_rst_clk) {
		reset_control_assert(mgr->ee_rst_clk);
	}

	mgr->clk_en_flag = 0;
	return;
}

s32 eink_get_clk_rate(struct clk *device_clk)
{
	unsigned long freq = 0;

	if (device_clk == NULL) {
		pr_err("[%s]: clk is NULL!\n", __func__);
		return -EINVAL;
	}

	freq = clk_get_rate(device_clk);
	EINK_DEBUG_MSG("clk freq = %ld\n", freq);

	return freq;
}

s32 eink_get_fps(struct eink_manager *mgr)
{
	int fps = 0;

	if (mgr == NULL) {
		pr_err("[%s]: mgr is NULL\n", __func__);
		return -EINVAL;
	}
	fps = mgr->panel_info.fresh_hz;
	return fps;
}

s32 eink_dump_config(struct eink_manager *mgr, char *buf)
{
	u32 count = 0;
	struct buf_manager *buf_mgr = NULL;

	if (mgr == NULL) {
		pr_err("[%s]:NULL hdl!\n", __func__);
		return -EINVAL;
	}

	buf_mgr = mgr->buf_mgr;

	count += sprintf(buf + count, "\timg_order[%3d] ", buf_mgr->upd_order);
#if 0
	count += sprintf(buf + count, "fb[%4u,%4u;%4u,%4u;%4u,%4u] ",
			 data.config.info.fb.size[0].width,
			 data.config.info.fb.size[0].height,
			 data.config.info.fb.size[1].width,
			 data.config.info.fb.size[1].height,
			 data.config.info.fb.size[2].width,
			 data.config.info.fb.size[2].height);
	count += sprintf(buf + count, "crop[%4u,%4u,%4u,%4u] ",
			 (unsigned int)(data.config.info.fb.crop.x >> 32),
			 (unsigned int)(data.config.info.fb.crop.y >> 32),
			 (unsigned int)(data.config.info.fb.crop.width >> 32),
			 (unsigned int)(data.config.info.fb.crop.height >> 32));
	count += sprintf(buf + count, "frame[%4d,%4d,%4u,%4u] ",
			 data.config.info.screen_win.x,
			 data.config.info.screen_win.y,
			 data.config.info.screen_win.width,
			 data.config.info.screen_win.height);
	count += sprintf(buf + count, "addr[%8llx,%8llx,%8llx] ",
			 data.config.info.fb.addr[0],
			 data.config.info.fb.addr[1],
			 data.config.info.fb.addr[2]);
	count += sprintf(buf + count, "flags[0x%8x] trd[%1d,%1d]\n",
			 data.config.info.fb.flags, data.config.info.b_trd_out,
			 data.config.info.out_trd_mode);
#endif
	return count;

}

s32 eink_mgr_enable(struct eink_manager *eink_mgr)
{
	int ret = 0;
	static int first_en = 1;

	struct pipe_manager *pipe_mgr = NULL;
	struct timing_ctrl_manager *timing_cmgr = NULL;

	EINK_DEBUG_MSG("input!\n");

	pipe_mgr = eink_mgr->pipe_mgr;
	timing_cmgr = eink_mgr->timing_ctrl_mgr;

	if ((pipe_mgr == NULL) || (timing_cmgr == NULL)) {
		pr_err("%s: eink is not initial\n", __func__);
		return -1;
	}
	mutex_lock(&eink_mgr->enable_lock);
	if (eink_mgr->enable_flag == true) {
		mutex_unlock(&eink_mgr->enable_lock);
		return 0;
	}
	if (first_en) {
		eink_clk_enable(eink_mgr);
		first_en = 0;
	}

	ret = pipe_mgr->pipe_mgr_enable(pipe_mgr);
	if (ret) {
		pr_err("%s:fail to enable pipe mgr", __func__);
		goto pipe_enable_fail;
	}

	ret = timing_cmgr->enable(timing_cmgr);
	if (ret) {
		pr_err("%s:fail to enable timing ctrl mgr", __func__);
		goto timing_enable_fail;
	}
#if defined(OFFLINE_MULTI_MODE) || defined(OFFLINE_SINGLE_MODE)
	eink_offline_enable(1);
#endif
	eink_mgr->enable_flag = true;
	mutex_unlock(&eink_mgr->enable_lock);

	return 0;

timing_enable_fail:
	pipe_mgr->pipe_mgr_disable();
pipe_enable_fail:
	eink_mgr->enable_flag = false;
	mutex_unlock(&eink_mgr->enable_lock);
	return ret;
}

s32 eink_mgr_disable(struct eink_manager *eink_mgr)
{
	int ret = 0;

	struct pipe_manager *pipe_mgr = NULL;
	struct timing_ctrl_manager *timing_cmgr = NULL;

	pipe_mgr = eink_mgr->pipe_mgr;
	timing_cmgr = eink_mgr->timing_ctrl_mgr;

	if ((pipe_mgr == NULL) || (timing_cmgr == NULL)) {
		pr_err("%s:eink manager is not initial\n", __func__);
		return -1;
	}

	mutex_lock(&eink_mgr->enable_lock);
	if (eink_mgr->enable_flag == false) {
		mutex_unlock(&eink_mgr->enable_lock);
		return 0;
	}

	ret = timing_cmgr->disable(timing_cmgr);
	if (ret) {
		pr_err("%s:fail to enable timing ctrl mgr", __func__);
		goto timing_enable_fail;
	}

	ret = pipe_mgr->pipe_mgr_disable();
	if (ret) {
		pr_err("fail to disable pipe(%d)\n", ret);
		goto pipe_disable_fail;
	}

#if defined(OFFLINE_MULTI_MODE) || defined(OFFLINE_SINGLE_MODE)
	eink_offline_enable(0);
#endif
	eink_mgr->enable_flag = false;
	mutex_unlock(&eink_mgr->enable_lock);

	return 0;

pipe_disable_fail:
	timing_cmgr->enable(timing_cmgr);
timing_enable_fail:
	eink_mgr->enable_flag = true;
	mutex_unlock(&eink_mgr->enable_lock);

	return ret;
}

static void eink_get_regulator_handle(struct eink_manager *mgr)
{
	struct regulator *regulator = NULL;
	struct device *dev = g_eink_drvdata.device;

	if (strlen(mgr->eink_pin_power) > 0) {
		regulator = regulator_get(dev, mgr->eink_pin_power);
		if (!regulator) {
			pr_err("[%s] get %s handle failed\n", __func__, mgr->eink_pin_power);
		} else
			mgr->pin_regulator = regulator;
	}
	return;
}

static void eink_put_regulator_handle(struct eink_manager *mgr)
{
	if (mgr->pin_regulator)
		regulator_put(mgr->pin_regulator);
	return;
}
int eink_mgr_init(struct init_para *para)
{
	int ret = 0;
	int irq_no = 0;
	struct eink_manager *eink_mgr = NULL;

	eink_mgr = (struct eink_manager *)kmalloc(sizeof(struct eink_manager), GFP_KERNEL | __GFP_ZERO);
	if (eink_mgr == NULL) {
		pr_err("%s:malloc mgr failed!\n", __func__);
		ret = -ENOMEM;
		goto eink_mgr_err;
	}

	g_eink_manager = eink_mgr;

	memset(eink_mgr, 0, sizeof(struct eink_manager));

	print_panel_info(para);

	memcpy(&eink_mgr->panel_info, &para->panel_info, sizeof(struct eink_panel_info));
	memcpy(&eink_mgr->timing_info, &para->timing_info, sizeof(struct timing_info));
	memcpy(eink_mgr->wav_path, para->wav_path, WAV_PATH_LEN);
	memcpy(eink_mgr->vcom_path, para->vcom_path, VCOM_PATH_LEN);
	memcpy(eink_mgr->eink_pin_power, para->eink_pin_power, POWER_STR_LEN);

	eink_get_regulator_handle(eink_mgr);

	eink_mgr->eink_update = eink_update_image;
	eink_mgr->eink_fmt_cvt_img = eink_fmt_convert_image;
	eink_mgr->eink_set_global_clean_cnt = eink_set_global_clean_cnt;
	eink_mgr->eink_mgr_enable = eink_mgr_enable;
	eink_mgr->eink_mgr_disable = eink_mgr_disable;

	eink_mgr->set_temperature = eink_set_temperature;
	eink_mgr->get_temperature = eink_get_temperature;
	eink_mgr->get_resolution = eink_get_resolution;
	eink_mgr->get_clk_rate = eink_get_clk_rate;
	eink_mgr->dump_config = eink_dump_config;
	eink_mgr->get_fps = eink_get_fps;

	eink_mgr->upd_pic_accept_flag = 0;
	eink_mgr->panel_temperature = 28;
	eink_mgr->waveform_init_flag = false;
	eink_mgr->enable_flag = false;

	eink_mgr->ee_clk = para->ee_clk;
	eink_mgr->ee_bus_clk = para->ee_bus_clk;
	eink_mgr->ee_rst_clk = para->ee_rst_clk;
	eink_mgr->panel_clk = para->panel_clk;

#ifdef VIRTUAL_REGISTER
	test_vreg_base = eink_malloc(0x1ffff, &test_preg_base);
	eink_set_reg_base((void __iomem *)test_vreg_base);
#else
	eink_set_reg_base(para->ee_reg_base);
#endif

	irq_no = para->ee_irq_no;
	ret = request_irq(irq_no, (irq_handler_t)eink_intterupt_proc, 0, "sunxi-eink", (void *)eink_mgr);
	if (ret) {
		pr_err("%s:irq request failed!\n", __func__);
		goto eink_mgr_err;
	}

	eink_mgr->convert_mgr = get_fmt_convert_mgr(0);
	init_waitqueue_head(&eink_mgr->upd_pic_accept_queue);
	init_waitqueue_head(&eink_mgr->upd_pic_queue);

	eink_mgr->suspend_state = 0;
	eink_mgr->shutdown_state = 0;

	mutex_init(&eink_mgr->standby_lock);
	mutex_init(&eink_mgr->enable_lock);

	ret = buf_mgr_init(eink_mgr);
	if (ret) {
		pr_err("%s:init buf mgr failed!\n", __func__);
		goto eink_mgr_err;
	}
	ret = index_mgr_init(eink_mgr);
	if (ret) {
		pr_err("%s:init index mgr failed!\n", __func__);
		goto eink_mgr_err;
	}
	ret = pipe_mgr_init(eink_mgr);
	if (ret) {
		pr_err("%s:init pipe mgr failed!\n", __func__);
		goto eink_mgr_err;
	}
	ret = timing_ctrl_mgr_init(para);
	if (ret) {
		pr_err("%s:init timing ctrl mgr failed!\n", __func__);
		goto eink_mgr_err;
	}

	eink_mgr->detect_fresh_task = kthread_create(detect_fresh_thread, (void *)eink_mgr, "eink_fresh_proc");
	if (IS_ERR_OR_NULL(eink_mgr->detect_fresh_task)) {
		pr_err("%s:create fresh thread failed!\n", __func__);
		ret = PTR_ERR(eink_mgr->detect_fresh_task);
		goto eink_mgr_err;
	}

	ret = wake_up_process(eink_mgr->detect_fresh_task);

	eink_mgr->get_temperature_task = kthread_create(eink_get_temperature_thread, (void *)eink_mgr, "get temperature proc");
	if (IS_ERR_OR_NULL(eink_mgr->get_temperature_task)) {
		pr_err("create getting temperature of eink thread fail!\n");
		ret = PTR_ERR(eink_mgr->get_temperature_task);
		goto eink_mgr_err;
	}
	ret = wake_up_process(eink_mgr->get_temperature_task);

	return ret;
eink_mgr_err:
	kfree(eink_mgr);
	return ret;
}

void eink_mgr_exit(void)
{
	struct eink_manager *eink_mgr = get_eink_manager();

	if (eink_mgr == NULL) {
		pr_err("[%s]hdl is NULL\n", __func__);
		return;
	}
	eink_put_regulator_handle(eink_mgr);

	return;
}
