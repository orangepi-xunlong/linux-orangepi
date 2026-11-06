#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/kthread.h>
#include <uapi/linux/sched/types.h>

#include "vpss_hal.h"
#include "vpss_debug.h"
#include "vpss_platform.h"
#include "ion.h"
#include "base_common.h"
#include "base_cb.h"
#include "vi_cb.h"

#define FBD_MAX_CHN (2)
#define HAL_WAIT_TIMEOUT_MS  1000
#define IS_POWER_OF_TWO(x) ((x != 0) && ((x & (x - 1)) == 0))

enum vpss_online_state {
	VPSS_ONLINE_UNEXIST,
	VPSS_ONLINE_READY,
	VPSS_ONLINE_RUN,
	VPSS_ONLINE_CONTINUE,
};

struct vpss_task_ctx {
	wait_queue_head_t wait;
	struct task_struct *thread;
	struct list_head job_wait_queue;
	struct list_head job_online_queue;
	spinlock_t task_lock;
	u8 is_suspend;
	u8 available;
	u8 online_dev_max_num;
	u8 online_status[VPSS_ONLINE_NUM];
	struct vpss_cmdq_buf cmdq_buf[2];
};

struct vpss_convert_to_bak {
	u8 enable;
	struct vip_frmsize grp_src_size;
	unsigned int grp_bytesperline[2];
	struct vip_rect grp_crop;
	struct vip_frmsize chn_src_size;
	struct vip_frmsize chn_dst_size;
	unsigned int chn_bytesperline[2];
	struct vip_rect chn_crop;
	struct vip_rect chn_rect;
};

static struct vpss_task_ctx task_ctx;
static struct vpss_device *vpss_dev;
struct vpss_convert_to_bak convert_to_bak[VPSS_MAX];

int work_mask = 0xff; //default vpss_v + vpss_t
int avail_mask = 0xff;
int sche_thread_enable = 1;
unsigned short reset_time[VPSS_MAX];
unsigned char core_last_sign[VPSS_MAX] = {0};

module_param(work_mask, int, 0644);

module_param(sche_thread_enable, int, 0644);

static void show_hw_state(void)
{
	u8 state[VPSS_MAX];
	u8 i;

	for (i = 0; i < VPSS_MAX; i++)
		state[i] = atomic_read(&vpss_dev->vpss_cores[i].state);

	TRACE_VPSS(DBG_DEBUG, "v(%d %d %d %d) t(%d %d %d %d) d(%d %d).\n",
		state[0], state[1], state[2], state[3], state[4], state[5], state[6],
		state[7], state[8], state[9]);
}

static int find_available_dev(u8 chn_num, struct vpss_hal_grp_cfg cfg, int after_core)
{
	int i;
	int start_idx = VPSS_V0;
	int end_idx = VPSS_V3;
	//int dev_num = 4;
	u8 mask = 0, mask_tmp;
	u8 user_mask = BIT(chn_num) - 1;

	for (i = start_idx; i <= end_idx; i++)
		if ((work_mask & BIT(i)) && (atomic_read(&vpss_dev->vpss_cores[i].state) == VIP_IDLE))
			mask |= BIT(i);

	for (i = start_idx; i <= end_idx; i++) {
		mask_tmp = mask;
		mask_tmp &= user_mask;
		if (!(user_mask ^ mask_tmp)){
			if (cfg.addr[2] && core_last_sign[i]){ // 3chn format limit
				after_core = i;
				mask = mask >> 1;
				continue;
			}
			return i;
		}
		//if (mask & 0x1) //v3 -> v0
		//	mask |= BIT(dev_num);
		mask = mask >> 1;
	}

	if (after_core != VPSS_MAX) {
		if(!IS_POWER_OF_TWO(work_mask))
			return after_core;
		else { // slt scene, single core, find alternative core
			for (i = VPSS_V0; i < VPSS_MAX; ++i) {
				if ((avail_mask & BIT(i)) && (atomic_read(&vpss_dev->vpss_cores[i].state) == VIP_IDLE)) {
					if (cfg.addr[2] && core_last_sign[i])
						continue;
					return i;
				}
			}
		}
	}

	return -1;
}

static int job_check_hw_ready(bool is_fbd, u8 chn_num, struct vpss_hal_grp_cfg cfg)
{
	int i, start_core, after_core;

	if (is_fbd && (chn_num > FBD_MAX_CHN)) {
		TRACE_VPSS(DBG_ERR, "FBD maximum number of channels(%d), job chn num(%d).\n",
			FBD_MAX_CHN, chn_num);
		return -1;
	}

	start_core = VPSS_T0; //vpss_t -> vpss_v
	after_core = VPSS_MAX;

	if (cfg.bm_scene) {
		for (i = start_core; i < VPSS_MAX; ++i) {
			if ((work_mask & BIT(i)) && (atomic_read(&vpss_dev->vpss_cores[i].state) == VIP_IDLE)) {
				if ((!is_fbd) && cfg.addr[2] && core_last_sign[i]){ // 3chn format limit
					after_core = i;
					continue;
				}
				return i;
			}
		}
		if (is_fbd)
			return -1;
		for (i = VPSS_V0; i < VPSS_T0; ++i) {
			if ((work_mask & BIT(i)) && (atomic_read(&vpss_dev->vpss_cores[i].state) == VIP_IDLE)) {
				if (cfg.addr[2] && core_last_sign[i])
					continue;
				return i;
			}
		}
		if (after_core != VPSS_MAX)
			return after_core;
		return -1;
	}

	if (chn_num == 1) {
		for (i = start_core; i < VPSS_MAX; ++i) {
			if ((work_mask & BIT(i)) && (atomic_read(&vpss_dev->vpss_cores[i].state) == VIP_IDLE)){
				if ((!is_fbd) && cfg.addr[2] && core_last_sign[i]){ // 3chn format limit
					after_core = i;
					continue;
				}
				return i;
			}
		}

		if (is_fbd)
			return -1;
	} else if (chn_num == 2) {
		if ((atomic_read(&vpss_dev->vpss_cores[VPSS_T0].state) == VIP_IDLE)
			&& (atomic_read(&vpss_dev->vpss_cores[VPSS_T1].state) == VIP_IDLE)){
			if ((!is_fbd) && cfg.addr[2] && core_last_sign[VPSS_T0]) // 3chn format limit
				after_core = i;
			else
				return VPSS_T0;
		}
		if ((atomic_read(&vpss_dev->vpss_cores[VPSS_T2].state) == VIP_IDLE)
			&& (atomic_read(&vpss_dev->vpss_cores[VPSS_T3].state) == VIP_IDLE)){
			if ((!is_fbd) && cfg.addr[2] && core_last_sign[VPSS_T2]) // 3chn format limit
				after_core = i;
			else
				return VPSS_T2;
		}
		if ((work_mask & BIT(VPSS_D0))  && (work_mask & BIT(VPSS_D1))
			&& (atomic_read(&vpss_dev->vpss_cores[VPSS_D0].state) == VIP_IDLE)
			&& (atomic_read(&vpss_dev->vpss_cores[VPSS_D1].state) == VIP_IDLE)){
			if ((!is_fbd) && cfg.addr[2] && core_last_sign[VPSS_D0]) // 3chn format limit
				after_core = i;
			else
				return VPSS_D0;
		}

		if (is_fbd)
			return -1;
	}

	return find_available_dev(chn_num, cfg, after_core);
}

static int online_get_dev(struct vpss_job *job)
{
	u8 i;
	u8 chn_num = job->cfg.chn_num;
	unsigned long flags;

	if (task_ctx.online_dev_max_num >= chn_num)
		return 0;

	for (i = task_ctx.online_dev_max_num; i < chn_num; ++i)
		if (!(atomic_read(&vpss_dev->vpss_cores[i].state) == VIP_IDLE)) {
			return -1;
		}

	spin_lock_irqsave(&vpss_dev->lock, flags);
	for (i = task_ctx.online_dev_max_num; i < chn_num; i++) {
		atomic_set(&vpss_dev->vpss_cores[i].state, VIP_ONLINE);
		vpss_dev->vpss_cores[i].is_online = 1;
	}
	task_ctx.online_dev_max_num = chn_num;
	spin_unlock_irqrestore(&vpss_dev->lock, flags);

	return 0;
}

static bool check_convert_to_addr(struct vpss_hw_cfg *cfg, u8 chn_num){
	u8 i;
	for(i = 0; i < chn_num; i++)
		if(((cfg->chn_cfg[i].addr[0] & 0xf) != 0 ||
			(cfg->chn_cfg[i].addr[1] & 0xf) != 0 ||
			(cfg->chn_cfg[i].addr[2] & 0xf) != 0) && cfg->chn_cfg[i].convert_to_cfg.enable)
			return true;
	return false;
};

static int job_convert_to_min_update(struct vpss_job *job, int dev_idx, u8 *chn_idx){
	struct vpss_hw_cfg *cfg = &job->cfg;
	u8 i, chn_id;

	convert_to_bak[dev_idx].enable = true;
	convert_to_bak[dev_idx].grp_src_size = cfg->grp_cfg.src_size;
	convert_to_bak[dev_idx].grp_bytesperline[0] = cfg->grp_cfg.bytesperline[0];
	convert_to_bak[dev_idx].grp_bytesperline[1] = cfg->grp_cfg.bytesperline[1];
	convert_to_bak[dev_idx].grp_crop = cfg->grp_cfg.crop;

	cfg->grp_cfg.src_size.width = 16;
	cfg->grp_cfg.src_size.height = 16;
	cfg->grp_cfg.bytesperline[0] = 16;
	cfg->grp_cfg.bytesperline[1] = 16;
	cfg->grp_cfg.crop.top = 0;
	cfg->grp_cfg.crop.left = 0;
	cfg->grp_cfg.crop.width = 16;
	cfg->grp_cfg.crop.height = 16;

	for (i = dev_idx; i < (dev_idx + cfg->chn_num); i++) {
		chn_id = chn_idx[i - dev_idx];
		convert_to_bak[i].chn_src_size = cfg->chn_cfg[chn_id].src_size;
		convert_to_bak[i].chn_crop = cfg->chn_cfg[chn_id].crop;
		convert_to_bak[i].chn_bytesperline[0] = cfg->chn_cfg[chn_id].bytesperline[0];
		convert_to_bak[i].chn_bytesperline[1] = cfg->chn_cfg[chn_id].bytesperline[1];
		convert_to_bak[i].chn_dst_size = cfg->chn_cfg[chn_id].dst_size;
		convert_to_bak[i].chn_rect = cfg->chn_cfg[chn_id].dst_rect;

		cfg->chn_cfg[chn_id].src_size = cfg->grp_cfg.src_size;
		cfg->chn_cfg[chn_id].crop = cfg->grp_cfg.crop;
		cfg->chn_cfg[chn_id].bytesperline[0] = cfg->grp_cfg.bytesperline[0];
		cfg->chn_cfg[chn_id].bytesperline[1] = cfg->grp_cfg.bytesperline[1];
		cfg->chn_cfg[chn_id].dst_rect = cfg->grp_cfg.crop;
		cfg->chn_cfg[chn_id].dst_size = cfg->grp_cfg.src_size;
	}
	return 0;
}

static int job_try_schedule(struct vpss_job *job)
{
	u8 i, chn_id, j = 0;
	u8 chn_idx[VPSS_MAX_CHN_NUM];
	u8 dev_list[VPSS_MAX_CHN_NUM] = {false, false, false, false};
	struct vpss_hw_cfg *cfg = &job->cfg;
	bool is_fbd = cfg->grp_cfg.fbd_enable;
	u8 chn_num = cfg->chn_num;
	unsigned long flags;
	int dev_idx, dev_idx_max;
	u16 out_l_end;
	u16 online_l_width = (job->is_online && job->online_param.is_tile)
							? job->online_param.l_in.end - job->online_param.l_in.start + 1
							: 0;
	u16 online_r_start = (job->is_online && job->online_param.is_tile)
							? job->online_param.r_in.start : 0;
	u16 online_r_end = (job->is_online && job->online_param.is_tile)
							? job->online_param.r_in.end : 0;

	for (i = 0; i < VPSS_MAX_CHN_NUM; i++)
		if (cfg->chn_enable[i]) {
			chn_idx[j] = i;
			j++;
		}

	spin_lock_irqsave(&vpss_dev->lock, flags);
	if (job->is_online)
		dev_idx = 0;
	else
		dev_idx = job_check_hw_ready(is_fbd, chn_num, cfg->grp_cfg);
	if (dev_idx < 0) {
		TRACE_VPSS(DBG_DEBUG, "Grp(%d), hw not ready.\n", job->grp_id);
		show_hw_state();
		spin_unlock_irqrestore(&vpss_dev->lock, flags);
		return -1;
	}

	convert_to_bak[dev_idx].enable = false;
	if (check_convert_to_addr(cfg, chn_num)){
		job_convert_to_min_update(job, dev_idx, chn_idx);
	}

	if ((cfg->grp_cfg.crop.width > VPSS_HW_LIMIT_WIDTH) ||
		job->online_param.is_tile) {
		job->is_tile = true;
		job->tile_mode = 0;
	} else {
		job->is_tile = false;
	}

	if (cfg->grp_cfg.crop.height > VPSS_HW_LIMIT_HEIGHT) {
		job->is_v_tile = true;
		job->tile_mode &= SCL_TILE_BOTH;
	} else {
		job->is_v_tile = false;
	}
	job->vpss_dev_mask = 0;
	job->dev_idx_start = dev_idx;
	dev_idx_max = dev_idx + chn_num;

	if(cfg->grp_cfg.pixelformat == PIXEL_FORMAT_NV12 ||
		cfg->grp_cfg.pixelformat == PIXEL_FORMAT_NV21)
		core_last_sign[dev_idx] = true;
	else
		core_last_sign[dev_idx] = false;

	TRACE_VPSS(DBG_DEBUG, "Scheduling Grp(%d), tile(%d).\n",
		job->grp_id, job->is_tile);

	for (i = dev_idx; i < dev_idx_max; i++) {
		chn_id = chn_idx[i - dev_idx];
		dev_list[chn_id] = i;
		atomic_set(&vpss_dev->vpss_cores[i].state, VIP_RUNNING);
		vpss_dev->vpss_cores[i].job = (void *)job;
		vpss_dev->vpss_cores[i].map_chn = chn_id;
		vpss_dev->vpss_cores[i].start_cnt++;
		//job->dev_idx[i] = dev_idx;
		job->vpss_dev_mask |= BIT(i);

		if (vpss_dev->vpss_cores[i].clk_vpss)
			clk_enable(vpss_dev->vpss_cores[i].clk_vpss);

		if (i == dev_idx)
			img_update(i, true, &cfg->grp_cfg);
		else
			img_update(i, false, &cfg->grp_cfg); //use dma share
		sc_update(i, &cfg->chn_cfg[chn_id]);

		if (job->is_tile) {
			out_l_end = job->is_online ? job->online_param.l_out.end :
				((job->cfg.chn_cfg[chn_id].src_size.width >> 1) - 1);
			vpss_dev->vpss_cores[i].tile_mode = sclr_tile_cal_size(i, out_l_end);
			job->tile_mode |= vpss_dev->vpss_cores[i].tile_mode;
		}

		if (job->is_v_tile) {
			vpss_dev->vpss_cores[i].tile_mode |= (sclr_v_tile_cal_size(i,
				(job->cfg.chn_cfg[chn_id].src_size.height >> 1) - 1)) << SCL_V_TILE_OFFSET;
			job->tile_mode |= vpss_dev->vpss_cores[i].tile_mode;
			if((job->tile_mode & SCL_TILE_BOTH) == SCL_TILE_BOTH)
				job->tile_mode |= (job->tile_mode & SCL_V_TILE_BOTH) << SCL_V_TILE_OFFSET;
		}

		if (i == (dev_idx_max - 1))
			top_update(i, false, is_fbd); //last chn,not share
		else
			top_update(i, true, is_fbd);
	}

	if (job->is_tile) {
		if (job->tile_mode & SCL_TILE_LEFT) {
			job->tile_mode &= ~SCL_TILE_LEFT;
			job->is_work_on_r_tile = false;
			for (i = dev_idx; i < dev_idx_max; i++)
				if (!img_left_tile_cfg(i, online_l_width))
					atomic_set(&vpss_dev->vpss_cores[i].state, VIP_END);
		} else if (job->tile_mode & SCL_TILE_RIGHT) {
			job->tile_mode &= ~SCL_TILE_RIGHT;
			job->is_work_on_r_tile = true;
			for (i = dev_idx; i < dev_idx_max; i++)
				if (!img_right_tile_cfg(i, online_r_start, online_r_end))
					atomic_set(&vpss_dev->vpss_cores[i].state, VIP_END);
		}
	}

	if (job->is_v_tile) {
		if ((job->tile_mode) & SCL_TILE_TOP) {
			job->tile_mode &= ~(SCL_TILE_TOP);
			for (i = dev_idx; i < dev_idx_max; i++)
				if (!img_top_tile_cfg(i, false))
					atomic_set(&vpss_dev->vpss_cores[i].state, VIP_END);
		} else if ((job->tile_mode) & SCL_TILE_DOWN) {
			job->tile_mode &= ~(SCL_TILE_DOWN);
			for (i = dev_idx; i < dev_idx_max; i++)
				if (!img_down_tile_cfg(i, false))
					atomic_set(&vpss_dev->vpss_cores[i].state, VIP_END);
		}
	}
	spin_unlock_irqrestore(&vpss_dev->lock, flags);
	atomic_set(&job->job_state, JOB_WORKING);

	TRACE_VPSS(DBG_INFO, "Grp(%d) job working, chn enable(%d %d %d %d), dev(%d %d %d %d).\n",
		job->grp_id, cfg->chn_enable[0], cfg->chn_enable[1],
		cfg->chn_enable[2], cfg->chn_enable[3], dev_list[0], dev_list[1],
		dev_list[2], dev_list[3]);

	for (i = dev_idx; i < dev_idx_max; i++)
		ktime_get_ts64(&vpss_dev->vpss_cores[i].ts_start);

	img_start(dev_idx, chn_num);

	return 0;
}


static void vpss_hal_reset(u32 vpss_dev_mask, bool is_online)
{
	unsigned long flags;
	int i;

	spin_lock_irqsave(&vpss_dev->lock, flags);
	for (i = 0; i < VPSS_MAX; i++) {
		if (!(vpss_dev_mask & BIT(i)))
			continue;
		img_reset(i);
		vpss_dev->vpss_cores[i].job = NULL;
		vpss_dev->vpss_cores[i].intr_status = 0;
		if (is_online)
			atomic_set(&vpss_dev->vpss_cores[i].state, VIP_ONLINE);
		else
			atomic_set(&vpss_dev->vpss_cores[i].state, VIP_IDLE);
	}
	spin_unlock_irqrestore(&vpss_dev->lock, flags);
}

int hal_try_schedule(void)
{
	struct vpss_job *job = NULL;
	unsigned long flags, flags_job;
	int ret = -1;

	spin_lock_irqsave(&task_ctx.task_lock, flags);
	while (!list_empty(&task_ctx.job_wait_queue)) {
		job = list_first_entry(&task_ctx.job_wait_queue,
			struct vpss_job, list);

		spin_lock_irqsave(&job->lock, flags_job);
		if (job->is_online) {
			if (online_get_dev(job)) {
				spin_unlock_irqrestore(&job->lock, flags_job);
				break;
			}
			list_move_tail(&job->list, &task_ctx.job_online_queue);
			task_ctx.online_status[job->grp_id] = VPSS_ONLINE_READY;
			TRACE_VPSS(DBG_DEBUG, "online max chn num:%d.\n", task_ctx.online_dev_max_num);
		} else {
			if (job_try_schedule(job)) {
				//TRACE_VPSS(DBG_DEBUG, "Grp(%d), try schedule fail, wait for next time.\n",
				//	job->grp_id);
				spin_unlock_irqrestore(&job->lock, flags_job);
				break;
			}
			list_del_init(&job->list);
		}
		spin_unlock_irqrestore(&job->lock, flags_job);

		ret = 0;
	}
	spin_unlock_irqrestore(&task_ctx.task_lock, flags);

	return ret;
}

static int schedule_thread(void *arg)
{
	struct vpss_task_ctx *ctx = (struct vpss_task_ctx *)arg;
	unsigned long timeout = msecs_to_jiffies(HAL_WAIT_TIMEOUT_MS);
	int ret;

	while (!kthread_should_stop()) {
		ret = wait_event_interruptible_timeout(ctx->wait,
			ctx->available || kthread_should_stop(), timeout);

		/*timeout*/
		if (!ret)
			continue;

		/* -%ERESTARTSYS */
		if (ret < 0 || kthread_should_stop())
			break;

		ctx->available = 0;
		hal_try_schedule();
	}

	return 0;
}

int vpss_hal_try_schedule(void)
{
	int ret = 0;

	if (sche_thread_enable) {
		task_ctx.available = 1;
		wake_up_interruptible(&task_ctx.wait);
	} else {
		ret = hal_try_schedule();
	}

	return ret;
}

int vpss_hal_init(struct vpss_device *dev)
{
	int i, ret;
	struct sched_param tsk;

	vpss_dev = dev;
	init_waitqueue_head(&task_ctx.wait);
	spin_lock_init(&task_ctx.task_lock);
	INIT_LIST_HEAD(&task_ctx.job_wait_queue);
	INIT_LIST_HEAD(&task_ctx.job_online_queue);
	task_ctx.online_dev_max_num = 0;
	task_ctx.available = 0;
	task_ctx.is_suspend = 0;

	for (i = 0; i < VPSS_ONLINE_NUM; i++)
		task_ctx.online_status[i] = VPSS_ONLINE_UNEXIST;

	for (i = 0; i < 2; i++) {
		task_ctx.cmdq_buf[i].cmdq_phy_addr = 0;
		task_ctx.cmdq_buf[i].cmdq_vir_addr = NULL;
		task_ctx.cmdq_buf[i].cmdq_buf_size = 0;
	}

	if (sche_thread_enable) {
		task_ctx.thread = kthread_run(schedule_thread, &task_ctx,
			"vpss_task_schedule");
		if (IS_ERR(task_ctx.thread))
			TRACE_VPSS(DBG_ERR, "failed to create vpss kthread\n");

		tsk.sched_priority = MAX_USER_RT_PRIO - 4;
		ret = sched_setscheduler(task_ctx.thread, SCHED_FIFO, &tsk);
		if (ret)
			TRACE_VPSS(DBG_WARN, "vpss schedule thread priority update failed: %d\n", ret);
	}

	return 0;
}

void vpss_hal_deinit(void)
{
	int ret;
	struct vpss_job *job;
	unsigned long flags;
	int i;

	if (sche_thread_enable) {
		if (!task_ctx.thread) {
			TRACE_VPSS(DBG_ERR, "vpss schedule thread not initialized yet\n");
			return;
		}
		ret = kthread_stop(task_ctx.thread);
		if (ret)
			TRACE_VPSS(DBG_ERR, "fail to stop vpss thread, err=%d\n", ret);

		task_ctx.thread = NULL;
	}

	spin_lock_irqsave(&task_ctx.task_lock, flags);
	while (!list_empty(&task_ctx.job_wait_queue)) {
		job = list_first_entry(&task_ctx.job_wait_queue,
				struct vpss_job, list);
		list_del_init(&job->list);
	}
	while (!list_empty(&task_ctx.job_online_queue)) {
		job = list_first_entry(&task_ctx.job_online_queue,
				struct vpss_job, list);
		list_del_init(&job->list);
	}
	spin_unlock_irqrestore(&task_ctx.task_lock, flags);

	for (i = 0; i < 2; i++) {
		if (task_ctx.cmdq_buf[i].cmdq_phy_addr) {
			base_ion_free(task_ctx.cmdq_buf[i].cmdq_phy_addr);
			task_ctx.cmdq_buf[i].cmdq_phy_addr = 0;
			task_ctx.cmdq_buf[i].cmdq_vir_addr = NULL;
			task_ctx.cmdq_buf[i].cmdq_buf_size = 0;
		}
	}

	vpss_dev = NULL;
}

int vpss_hal_push_job(struct vpss_job *job)
{
	unsigned long flags, flags_job;

	if (!job || !job->cfg.chn_num)
		return -1;

	spin_lock_irqsave(&task_ctx.task_lock, flags);
	list_add_tail(&job->list, &task_ctx.job_wait_queue);

	spin_lock_irqsave(&job->lock, flags_job);
	atomic_set(&job->job_state, JOB_WAIT);
	spin_unlock_irqrestore(&job->lock, flags_job);
	spin_unlock_irqrestore(&task_ctx.task_lock, flags);

	return 0;
}

int vpss_hal_push_online_job(struct vpss_job *job)
{
	unsigned long flags, flags_job;

	if (!job || !job->cfg.chn_num)
		return -1;

	spin_lock_irqsave(&task_ctx.task_lock, flags);
	list_add_tail(&job->list, &task_ctx.job_online_queue);

	spin_lock_irqsave(&job->lock, flags_job);
	atomic_set(&job->job_state, JOB_WAIT);
	spin_unlock_irqrestore(&job->lock, flags_job);
	spin_unlock_irqrestore(&task_ctx.task_lock, flags);

	return 0;
}

int vpss_hal_remove_job(struct vpss_job *job)
{
	int i, count = 20;
	unsigned long flags, flags_job;
	struct vpss_job *job_item;
	enum job_state job_state;

	spin_lock_irqsave(&task_ctx.task_lock, flags);
	spin_lock_irqsave(&job->lock, flags_job);

	job_state = atomic_read(&job->job_state);

	if ((job_state == JOB_WORKING) || (job_state == JOB_HALF)) {
		spin_unlock_irqrestore(&job->lock, flags_job);
		spin_unlock_irqrestore(&task_ctx.task_lock, flags);

		//wait hw done
		while (--count > 0) {
			if (atomic_read(&job->job_state) == JOB_END)
				break;
			if(!job->cfg.grp_cfg.bm_scene) // cvi sence
				TRACE_VPSS(DBG_WARN, "wait count(%d)\n", count);
			usleep_range(1000, 2000);
		}
		//hw hang
		if (count == 0) {
			if(!job->cfg.grp_cfg.bm_scene) // cvi sence
				TRACE_VPSS(DBG_ERR, "Grp(%d) Wait timeout, HW hang.\n", job->grp_id);
			for (i = 0; i < VPSS_MAX; i++) {
				if (!(job->vpss_dev_mask & BIT(i)))
					continue;
				vpss_stauts(i);
				// BIT(10) always reset; BIT(11) never reset
				// VPSS2 and VPSS3 need binding reset, manual set BIT(13) can reset
				if(((BIT(10) & work_mask) ||
					(!reset_time[i] && !(BIT(11) & work_mask))) &&
					(i != VPSS_V2 || (BIT(13) & work_mask))) {
					vpss_hal_reset(job->vpss_dev_mask, job->is_online);
					TRACE_VPSS(DBG_WARN, "core(%d) reset.\n", i);
					reset_time[i] = 1000;
				} else {
					work_mask &= (~BIT(i));
					avail_mask &= (~BIT(i));
					TRACE_VPSS(DBG_WARN, "core(%d) mask.\n", i);
				}
				if (vpss_dev->vpss_cores[i].clk_vpss &&
					__clk_is_enabled(vpss_dev->vpss_cores[i].clk_vpss))
					clk_disable(vpss_dev->vpss_cores[i].clk_vpss);
			}
		}
		return 0;
	} else if (job_state == JOB_WAIT) {
		list_for_each_entry(job_item, &task_ctx.job_wait_queue, list) {
			if (job_item == job) {
				atomic_set(&job->job_state, JOB_INVALID);
				list_del_init(&job->list);
				spin_unlock_irqrestore(&job->lock, flags_job);
				spin_unlock_irqrestore(&task_ctx.task_lock, flags);
				return 0;
			}
		}
		list_for_each_entry(job_item, &task_ctx.job_online_queue, list) {
			if (job_item == job) {
				atomic_set(&job->job_state, JOB_INVALID);
				list_del_init(&job->list);
				spin_unlock_irqrestore(&job->lock, flags_job);
				spin_unlock_irqrestore(&task_ctx.task_lock, flags);
				return 0;
			}
		}
	}
	spin_unlock_irqrestore(&job->lock, flags_job);
	spin_unlock_irqrestore(&task_ctx.task_lock, flags);

	return 0;
}


int vpss_hal_stitch_schedule(struct vpss_stitch_cfg *cfg)
{
	u8 dev_idx;
	unsigned long flags;
	int ret;
	struct vpss_cmdq_buf *cmdq_buf;

	if (task_ctx.is_suspend) {
		return -1;
	}

	spin_lock_irqsave(&vpss_dev->lock, flags);
	if (atomic_cmpxchg(&vpss_dev->vpss_cores[VPSS_D0].state,
		VIP_IDLE, VIP_RUNNING) == VIP_IDLE) {
		dev_idx = VPSS_D0;
	} else if (atomic_cmpxchg(&vpss_dev->vpss_cores[VPSS_D1].state,
		VIP_IDLE, VIP_RUNNING) == VIP_IDLE) {
		dev_idx = VPSS_D1;
	} else {
		spin_unlock_irqrestore(&vpss_dev->lock, flags);
		TRACE_VPSS(DBG_WARN, "vpss device busy.\n");
		return -1;
	}

	vpss_dev->vpss_cores[dev_idx].job = (void *)cfg;
	vpss_dev->vpss_cores[dev_idx].start_cnt++;
	spin_unlock_irqrestore(&vpss_dev->lock, flags);

	cmdq_buf = &task_ctx.cmdq_buf[dev_idx - VPSS_D0];
	if (!cmdq_buf->cmdq_phy_addr) {
		ret = base_ion_alloc(&cmdq_buf->cmdq_phy_addr, &cmdq_buf->cmdq_vir_addr,
				"vpss_cmdq_buf", VPSS_CMDQ_BUF_SIZE, true);
		if (ret) {
			TRACE_VPSS(DBG_ERR, "ion alloc fail! ret(%d)\n", ret);
			cmdq_buf->cmdq_phy_addr = 0;
			atomic_set(&vpss_dev->vpss_cores[dev_idx].state, VIP_IDLE);
			return ret;
		}
		cmdq_buf->cmdq_buf_size = VPSS_CMDQ_BUF_SIZE;
	}

	ktime_get_ts64(&vpss_dev->vpss_cores[dev_idx].ts_start);
	if (vpss_dev->vpss_cores[dev_idx].clk_vpss)
		clk_enable(vpss_dev->vpss_cores[dev_idx].clk_vpss);

	TRACE_VPSS(DBG_DEBUG, "stitch dev(%d) cfg(%px) cb(%px) data(%px).\n",
		dev_idx, cfg, cfg->job_cb, cfg->data);

	return vpss_stitch_platform(dev_idx, cmdq_buf, cfg);
}

int vpss_hal_direct_schedule(struct vpss_job *job){
	return job_try_schedule(job);
}

void vpss_hal_online_release_dev(void)
{
	unsigned long flags, flags_dev;
	int i;

	spin_lock_irqsave(&task_ctx.task_lock, flags);

	spin_lock_irqsave(&vpss_dev->lock, flags_dev);
	for (i = 0; i < task_ctx.online_dev_max_num; i++) {
		atomic_set(&vpss_dev->vpss_cores[i].state, VIP_IDLE);
		vpss_dev->vpss_cores[i].is_online = 0;
	}
	spin_unlock_irqrestore(&vpss_dev->lock, flags_dev);

	task_ctx.online_dev_max_num = 0;
	for (i = 0; i < VPSS_ONLINE_NUM; i++)
		task_ctx.online_status[i] = VPSS_ONLINE_UNEXIST;
	spin_unlock_irqrestore(&task_ctx.task_lock, flags);
}

int vpss_hal_online_run(struct vpss_online_cb *param)
{
	int i;
	unsigned long flags, flags_job;
	struct vpss_job *work_job = NULL;
	struct vpss_job *job_item;

	if (param->snr_num >= VPSS_ONLINE_NUM) {
		TRACE_VPSS(DBG_WARN, "param->snr_num(%d) err.\n", param->snr_num);
		return -1;
	}

	TRACE_VPSS(DBG_INFO, "online trigger vpss, snr_num(%d).\n", param->snr_num);

	if (param->is_tile) {
		TRACE_VPSS(DBG_DEBUG, "is_left_tile(%d)\n" \
			"left in(%d %d) out(%d %d).\n" \
			"right in(%d %d) out(%d %d).\n",
			param->is_left_tile, param->l_in.start, param->l_in.end,
			param->l_out.start, param->l_out.end,
			param->r_in.start, param->r_in.end,
			param->r_out.start, param->r_out.end);
	}

	spin_lock_irqsave(&task_ctx.task_lock, flags);
	if (task_ctx.online_status[param->snr_num] == VPSS_ONLINE_UNEXIST) {
		TRACE_VPSS(DBG_WARN, "vpss(%d) not ready.\n", param->snr_num);
		goto err;
	} else if (task_ctx.online_status[param->snr_num] == VPSS_ONLINE_RUN) {
		TRACE_VPSS(DBG_NOTICE, "vpss(%d) is running.\n", param->snr_num);
		goto err;
	}

	if (param->is_tile && (!param->is_left_tile)) {
		task_ctx.online_status[param->snr_num] = VPSS_ONLINE_RUN;
		spin_unlock_irqrestore(&task_ctx.task_lock, flags);
		return 0;
	}

	if (task_ctx.is_suspend) {
		spin_unlock_irqrestore(&task_ctx.task_lock, flags);
		return -1;
	}

	list_for_each_entry(job_item, &task_ctx.job_online_queue, list) {
		if (job_item->grp_id == param->snr_num) {
			work_job = job_item;
			break;
		}
	}

	if (!work_job) {
		TRACE_VPSS(DBG_WARN, "online grp(%d), Empty job.\n", param->snr_num);
		goto err;
	}

	spin_lock_irqsave(&work_job->lock, flags_job);
	for (i = 0; i < VPSS_MAX_CHN_NUM; i++) {
		if (!work_job->cfg.chn_enable[i])
			continue;
		if (work_job->cfg.chn_cfg[i].addr[0] == 0) {
			TRACE_VPSS(DBG_WARN, "online grp(%d), Not buffer.\n", param->snr_num);
			spin_unlock_irqrestore(&work_job->lock, flags_job);
			goto err;
		}
	}
	memcpy(&work_job->online_param, param, sizeof(struct vpss_online_cb));
	job_try_schedule(work_job);
	list_del_init(&work_job->list);
	spin_unlock_irqrestore(&work_job->lock, flags_job);

	task_ctx.online_status[param->snr_num] = VPSS_ONLINE_RUN;
	spin_unlock_irqrestore(&task_ctx.task_lock, flags);

	return 0;

err:
	spin_unlock_irqrestore(&task_ctx.task_lock, flags);
	return -1;
}

static int vpss_job_restart(struct vpss_job *job){
	u8 chn_idx[VPSS_MAX_CHN_NUM];
	u8 i, chn_id, j = 0;
	int dev_idx = job->dev_idx_start;
	int dev_idx_max = dev_idx + job->cfg.chn_num;
	struct vpss_hw_cfg *cfg = &job->cfg;
	unsigned long flags;
	u16 online_l_width = (job->is_online && job->online_param.is_tile)
							? job->online_param.l_in.end - job->online_param.l_in.start + 1
							: 0;
	u16 online_r_start = (job->is_online && job->online_param.is_tile)
							? job->online_param.r_in.start : 0;
	u16 online_r_end = (job->is_online && job->online_param.is_tile)
							? job->online_param.r_in.end : 0;

	for (i = 0; i < VPSS_MAX_CHN_NUM; i++)
		if (cfg->chn_enable[i]) {
			chn_idx[j] = i;
			j++;
		}

	cfg->grp_cfg.src_size = convert_to_bak[dev_idx].grp_src_size;
	cfg->grp_cfg.bytesperline[0] = convert_to_bak[dev_idx].grp_bytesperline[0];
	cfg->grp_cfg.bytesperline[1] = convert_to_bak[dev_idx].grp_bytesperline[1];
	cfg->grp_cfg.crop = convert_to_bak[dev_idx].grp_crop;

	if (cfg->grp_cfg.crop.width > VPSS_HW_LIMIT_WIDTH) { //output > 4608 ?
		job->is_tile = true;
		job->tile_mode = 0;
	} else {
		job->is_tile = false;
	}

	if (cfg->grp_cfg.crop.height > VPSS_HW_LIMIT_HEIGHT) {
		job->is_v_tile = true;
		job->tile_mode &= SCL_TILE_BOTH;
	} else {
		job->is_v_tile = false;
	}

	spin_lock_irqsave(&vpss_dev->lock, flags);
	for (i = dev_idx; i < dev_idx_max; i++) {
		chn_id = chn_idx[i - dev_idx];

		cfg->chn_cfg[chn_id].src_size = convert_to_bak[i].chn_src_size;
		cfg->chn_cfg[chn_id].crop = convert_to_bak[i].chn_crop;
		cfg->chn_cfg[chn_id].bytesperline[0] = convert_to_bak[i].chn_bytesperline[0];
		cfg->chn_cfg[chn_id].bytesperline[1] = convert_to_bak[i].chn_bytesperline[1];
		cfg->chn_cfg[chn_id].dst_size = convert_to_bak[i].chn_dst_size;
		cfg->chn_cfg[chn_id].dst_rect = convert_to_bak[i].chn_rect;

		atomic_set(&vpss_dev->vpss_cores[i].state, VIP_RUNNING);

		if (i == dev_idx)
			img_update(i, true, &cfg->grp_cfg);
		else
			img_update(i, false, &cfg->grp_cfg); //use dma share
		sc_update(i, &cfg->chn_cfg[chn_id]);

		if (job->is_tile) {
			vpss_dev->vpss_cores[i].tile_mode = sclr_tile_cal_size(i,
				(job->cfg.chn_cfg[chn_id].src_size.width >> 1) - 1);
			job->tile_mode |= vpss_dev->vpss_cores[i].tile_mode;
		}

		if (job->is_v_tile) {
			vpss_dev->vpss_cores[i].tile_mode |= (sclr_v_tile_cal_size(i,
				(job->cfg.chn_cfg[chn_id].src_size.height >> 1) - 1)) << SCL_V_TILE_OFFSET;
			job->tile_mode |= vpss_dev->vpss_cores[i].tile_mode;
			if((job->tile_mode & SCL_TILE_BOTH) == SCL_TILE_BOTH)
				job->tile_mode |= (job->tile_mode & SCL_V_TILE_BOTH) << SCL_V_TILE_OFFSET;
		}
	}
	if (job->is_tile) {
		if (job->tile_mode & SCL_TILE_LEFT) {
			job->tile_mode &= ~SCL_TILE_LEFT;
			job->is_work_on_r_tile = false;
			for (i = dev_idx; i < dev_idx_max; i++)
				if (!img_left_tile_cfg(i, online_l_width))
					atomic_set(&vpss_dev->vpss_cores[i].state, VIP_END);
		} else if (job->tile_mode & SCL_TILE_RIGHT) {
			job->tile_mode &= ~SCL_TILE_RIGHT;
			job->is_work_on_r_tile = true;
			for (i = dev_idx; i < dev_idx_max; i++)
				if (!img_right_tile_cfg(i, online_r_start, online_r_end))
					atomic_set(&vpss_dev->vpss_cores[i].state, VIP_END);
		}
	}

	if (job->is_v_tile) {
		if ((job->tile_mode) & SCL_TILE_TOP) {
			job->tile_mode &= ~(SCL_TILE_TOP);
			for (i = dev_idx; i < dev_idx_max; i++)
				if (!img_top_tile_cfg(i, false))
					atomic_set(&vpss_dev->vpss_cores[i].state, VIP_END);
		} else if ((job->tile_mode) & SCL_TILE_DOWN) {
			job->tile_mode &= ~(SCL_TILE_DOWN);
			for (i = dev_idx; i < dev_idx_max; i++)
				if (!img_down_tile_cfg(i, false))
					atomic_set(&vpss_dev->vpss_cores[i].state, VIP_END);
		}
	}
	spin_unlock_irqrestore(&vpss_dev->lock, flags);

	TRACE_VPSS(DBG_INFO, "Grp(%d) job working, chn enable(%d %d %d %d).\n",
		job->grp_id, cfg->chn_enable[0], cfg->chn_enable[1],
		cfg->chn_enable[2], cfg->chn_enable[3]);

	convert_to_bak[dev_idx].enable = false;
	atomic_set(&job->job_state, JOB_WORKING);

	img_start(dev_idx, cfg->chn_num);

	return 0;
}

static void vpss_job_finish(struct vpss_job *job)
{
	u8 i, chn_id;
	u32 max_duration = 0;
	struct vpss_device *dev = vpss_dev;
	unsigned long flags_job;

	spin_lock_irqsave(&job->lock, flags_job);
	for (i = 0; i < VPSS_MAX; i++) {
		if (!(job->vpss_dev_mask & BIT(i)))
			continue;

		if (atomic_read(&dev->vpss_cores[i].state) != VIP_END) {
			spin_unlock_irqrestore(&job->lock, flags_job);
			return;
		}
	}

	if (atomic_cmpxchg(&job->job_state, JOB_WORKING, JOB_HALF) != JOB_WORKING) {
		spin_unlock_irqrestore(&job->lock, flags_job);
		return;
	}

	if (convert_to_bak[job->dev_idx_start].enable == true){
		vpss_job_restart(job);
		spin_unlock_irqrestore(&job->lock, flags_job);
		return;
	}

	if ((job->is_v_tile) && (job->tile_mode & 0xc)) {
		job->tile_mode &= ~(SCL_TILE_DOWN);

		for (i = 0; i < VPSS_MAX; i++) {
			if (!(job->vpss_dev_mask & BIT(i)))
				continue;
			if (img_down_tile_cfg(i, (job->tile_mode & SCL_RIGHT_DOWN_TILE_FLAG)))
				atomic_set(&dev->vpss_cores[i].state, VIP_RUNNING);
			else
				atomic_set(&dev->vpss_cores[i].state, VIP_END);
		}
		job->tile_mode &= ~SCL_RIGHT_DOWN_TILE_FLAG;
		atomic_set(&job->job_state, JOB_WORKING);
		img_start(job->dev_idx_start, job->cfg.chn_num);
		spin_unlock_irqrestore(&job->lock, flags_job);
		return;
	}

	if ((job->is_tile) && job->tile_mode) { //right tile
		u16 online_r_start = (job->is_online && job->online_param.is_tile)
								? job->online_param.r_in.start : 0;
		u16 online_r_end = (job->is_online && job->online_param.is_tile)
								? job->online_param.r_in.end : 0;
		job->tile_mode &= ~SCL_TILE_RIGHT;
		job->is_work_on_r_tile = true;

		for (i = 0; i < VPSS_MAX; i++) {
			if (!(job->vpss_dev_mask & BIT(i)))
				continue;
			if (img_right_tile_cfg(i, online_r_start, online_r_end))
				atomic_set(&dev->vpss_cores[i].state, VIP_RUNNING);
			else
				atomic_set(&dev->vpss_cores[i].state, VIP_END);
		}
		if(job->is_v_tile){
			job->tile_mode = job->tile_mode >> SCL_V_TILE_OFFSET;
			if ((job->tile_mode) & SCL_TILE_TOP) {
				job->tile_mode &= ~(SCL_TILE_TOP);
				for (i = 0; i < VPSS_MAX; i++){
					if (!(job->vpss_dev_mask & BIT(i)))
						continue;
					if (!img_top_tile_cfg(i, true))
						atomic_set(&dev->vpss_cores[i].state, VIP_END);
					else
						atomic_set(&dev->vpss_cores[i].state, VIP_RUNNING);
				}
				if(job->tile_mode)
					job->tile_mode |= SCL_RIGHT_DOWN_TILE_FLAG;
			} else if (job->tile_mode & SCL_TILE_DOWN) {
				job->tile_mode &= ~(SCL_TILE_DOWN);
				for (i = 0; i < VPSS_MAX; i++){
					if (!(job->vpss_dev_mask & BIT(i)))
						continue;
					if (!img_down_tile_cfg(i, true))
						atomic_set(&dev->vpss_cores[i].state, VIP_END);
					else
						atomic_set(&dev->vpss_cores[i].state, VIP_RUNNING);
				}
			}
		}

		atomic_set(&job->job_state, JOB_WORKING);
		img_start(job->dev_idx_start, job->cfg.chn_num);
		spin_unlock_irqrestore(&job->lock, flags_job);

		//wake up isp for vi-vpss online tile mode
		if (job->is_online && job->online_param.is_tile) {
			struct base_exe_m_cb exe_cb;

			task_ctx.online_status[job->grp_id] = VPSS_ONLINE_CONTINUE;

			exe_cb.callee = E_MODULE_VI;
			exe_cb.caller = E_MODULE_VPSS;
			exe_cb.cmd_id = VI_CB_SC_FRM_DONE;
			exe_cb.data = NULL;
			base_exe_module_cb(&exe_cb);
		}
		return;
	}

	//job finish
	if ((!job->is_tile) || (!job->is_v_tile) || ((job->is_tile || job->is_v_tile) && !job->tile_mode)) {

		for (i = 0; i < VPSS_MAX; ++i) {
			if (!(job->vpss_dev_mask & BIT(i)))
				continue;

			if(reset_time[i] != 0 && reset_time[i] <= 1000)
				reset_time[i]--;

			max_duration = dev->vpss_cores[i].hw_duration > max_duration ?
							dev->vpss_cores[i].hw_duration : max_duration;
			// disable clk
			if (dev->vpss_cores[i].clk_vpss)
				clk_disable(dev->vpss_cores[i].clk_vpss);

			if (!job->is_online)
				atomic_set(&dev->vpss_cores[i].state, VIP_IDLE);
			else
				task_ctx.online_status[job->grp_id] = VPSS_ONLINE_READY;

			chn_id = dev->vpss_cores[i].map_chn;
			if (chn_id >= VPSS_MAX_CHN_NUM)
				TRACE_VPSS(DBG_ERR, "map_chn err.\n");
			else
				job->checksum[chn_id] = dev->vpss_cores[i].checksum;
		}
		job->hw_duration = max_duration;
		TRACE_VPSS(DBG_DEBUG, "vpss grp(%d) Hw Duration (%d).\n", job->grp_id, max_duration);

		atomic_set(&job->job_state, JOB_END);
		job->vpss_dev_mask = 0;
		job->job_cb(&job->data);

		spin_unlock_irqrestore(&job->lock, flags_job);
		vpss_hal_try_schedule();
		return;
	}
	spin_unlock_irqrestore(&job->lock, flags_job);
}


void vpss_irq_handler(struct vpss_core *core)
{
	u8 vpss_idx = core->vpss_type;
	struct vpss_job *job = (struct vpss_job *)core->job;

	if (!job) {
		TRACE_VPSS(DBG_INFO, "vpss(%d), job canceled.\n", vpss_idx);
		atomic_set(&core->state, VIP_IDLE);
		return;
	}
	if (!(job->vpss_dev_mask & BIT(vpss_idx))) {
		TRACE_VPSS(DBG_ERR, "core(%d) grp(%d) vpss_dev_mask(%x) err.\n",
			vpss_idx, job->grp_id, job->vpss_dev_mask);
		return;
	}

	atomic_set(&core->state, VIP_END);
	ktime_get_ts64(&core->ts_end);
	core->hw_duration = get_diff_in_us(core->ts_start, core->ts_end);
	core->hw_duration_total += core->hw_duration;
	core->int_cnt++;

	if (job->is_tile) {
		if (job->is_work_on_r_tile)
			core->tile_mode &= ~(SCL_TILE_RIGHT);
		else
			core->tile_mode &= ~(SCL_TILE_LEFT);

		if (!core->tile_mode) { //work finish
			core->job = NULL;
		}
	}

	vpss_job_finish(job);
}

void vpss_cmdq_irq_handler(struct vpss_core *core)
{
	u8 vpss_idx = core->vpss_type;
	struct vpss_stitch_cfg *cfg = (struct vpss_stitch_cfg *)core->job;
	unsigned long flags;

	if (!cfg) {
		TRACE_VPSS(DBG_INFO, "vpss(%d), job NULL.\n", vpss_idx);
		atomic_set(&core->state, VIP_IDLE);
		return;
	}
	if (core->clk_vpss)
		clk_disable(core->clk_vpss);

	spin_lock_irqsave(&vpss_dev->lock, flags);
	atomic_set(&core->state, VIP_IDLE);
	ktime_get_ts64(&core->ts_end);
	core->hw_duration = get_diff_in_us(core->ts_start, core->ts_end);
	core->hw_duration_total += core->hw_duration;
	core->int_cnt++;
	core->job = NULL;
	spin_unlock_irqrestore(&vpss_dev->lock, flags);
	TRACE_VPSS(DBG_DEBUG, "vpss(%d) Hw Duration (%d).\n", vpss_idx, core->hw_duration);

	cfg->job_cb(cfg->data);
}

void vpss_hal_suspend(void)
{
	unsigned long flags;
	int i, count;

	spin_lock_irqsave(&task_ctx.task_lock, flags);
	task_ctx.is_suspend = 1;
	spin_unlock_irqrestore(&task_ctx.task_lock, flags);

	//stop online
	for (i = 0; i < VPSS_ONLINE_NUM; i++) {
		count = 20;
		//wait online frame done
		while ((task_ctx.online_status[i] == VPSS_ONLINE_RUN) ||
			(task_ctx.online_status[i] == VPSS_ONLINE_CONTINUE)) {
			usleep_range(1000, 2000);
			if (--count <= 0)
				break;
		}
		if (count == 0) {
			TRACE_VPSS(DBG_ERR, "Grp(%d) Wait timeout, online HW hang.\n", i);
			//TODO: reset dev and job
		}
	}

	//stop stitch
	for (i = VPSS_D0; i <= VPSS_D1; i++) {
		count = 20;
		while (atomic_read(&vpss_dev->vpss_cores[i].state) != VIP_IDLE) {
			usleep_range(1000, 2000);
			if (--count <= 0)
				break;
		}
		if (count == 0) {
			TRACE_VPSS(DBG_ERR, "vpss(%d) Wait timeout, HW hang.\n", i);
			//TODO: reset dev and job
		}
	}
}

void vpss_hal_down_reg(unsigned char inst){
	if(work_mask & BIT(12)) // BIT(12) always dump reg
		vpss_error_stauts(inst);
}

void vpss_hal_resume(void)
{
	unsigned long flags;

	spin_lock_irqsave(&task_ctx.task_lock, flags);
	task_ctx.is_suspend = 0;
	spin_unlock_irqrestore(&task_ctx.task_lock, flags);
}

void vpss_clk_disable(void)
{
	u8 i = 0;

	for (i = 0; i < VPSS_MAX; i++) {
		if (vpss_dev->vpss_cores[i].clk_src)
			clk_disable(vpss_dev->vpss_cores[i].clk_src);
		if (vpss_dev->vpss_cores[i].clk_apb && __clk_is_enabled(vpss_dev->vpss_cores[i].clk_apb))
			clk_disable(vpss_dev->vpss_cores[i].clk_apb);
	}
}

void vpss_clk_enable(void)
{
	u8 i = 0;

	for (i = 0; i < VPSS_MAX; i++) {
		if (vpss_dev->vpss_cores[i].clk_src)
			clk_enable(vpss_dev->vpss_cores[i].clk_src);
		if (vpss_dev->vpss_cores[i].clk_apb && !__clk_is_enabled(vpss_dev->vpss_cores[i].clk_apb))
			clk_enable(vpss_dev->vpss_cores[i].clk_apb);
	}
}

bool vpss_check_suspend(void)
{
	return task_ctx.is_suspend == 1;
}