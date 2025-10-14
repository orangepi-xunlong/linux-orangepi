// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Cix Technology Group Co., Ltd.*/
#include <linux/soc/cix/cix_ap2se_ipc.h>
#include <linux/module.h>
#include <linux/syscore_ops.h>
#include <linux/bits.h>

#define WAKEUP_WORK_DELAY_MS 2000

#ifndef MIN
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#endif

struct wakeup_source_dat {
	uint32_t plat_id;
	uint32_t wakeup_src;
	uint32_t wakeup_sub_src[6];
};

struct wakeup_source_dat g_wakeup_src_dat;
struct delayed_work g_delay_wq;

void wakeup_src_rx_callback(char *inbuf, size_t len)
{
	memcpy(&g_wakeup_src_dat, inbuf, MIN(len, sizeof(g_wakeup_src_dat)));
}

static int wakeup_src_send_msg(void)
{
	return cix_ap2se_ipc_send(FFA_REQ_WAKEUP_SOURCE, NULL, 0, 1);
}

static void wakeup_src_work(struct work_struct *work)
{
	pr_info("%s\n", __func__);
	if (wakeup_src_send_msg() >= 0) {
		pr_info("wakeup_src_resume: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n",
			g_wakeup_src_dat.plat_id, g_wakeup_src_dat.wakeup_src,
			g_wakeup_src_dat.wakeup_sub_src[0],
			g_wakeup_src_dat.wakeup_sub_src[1],
			g_wakeup_src_dat.wakeup_sub_src[2],
			g_wakeup_src_dat.wakeup_sub_src[3]);
	}
}
static void wakeup_src_resume(void)
{
	schedule_delayed_work(&g_delay_wq, msecs_to_jiffies(WAKEUP_WORK_DELAY_MS));
}
static struct syscore_ops wakeup_src_syscore_ops = {
	.suspend = NULL,
	.resume = wakeup_src_resume,
};
static int wakeup_src_init(void)
{
	int ret;
	ret = cix_ap2se_register_rx_cbk(FFA_CMDID_PLAT_WAKEUP_SRC,
					wakeup_src_rx_callback);
	if (ret) {
		pr_err("register wakeup src rx callback failed\n");
	}
	register_syscore_ops(&wakeup_src_syscore_ops);
	INIT_DELAYED_WORK(&g_delay_wq, wakeup_src_work);
	return ret;
}
module_init(wakeup_src_init);
MODULE_AUTHOR("Vincent Wu <vincent.wu@cixtech.com>");
MODULE_DESCRIPTION("CIX Wakeup Source Driver");
MODULE_LICENSE("GPL v2");
