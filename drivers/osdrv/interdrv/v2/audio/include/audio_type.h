#ifndef __AUDIO_TYPE_H__
#define __AUDIO_TYPE_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/clk.h>
#include <linux/kthread.h>
#include <linux/mm.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/sched/signal.h>
#include <linux/slab.h>
#include <linux/streamline_annotate.h>
#include <linux/version.h>
//#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0))
#include <uapi/linux/sched/types.h>
//#endif
//include  header type ----------------------start
#include <comm_aio.h>
//include  header type ----------------------stop
#define AUDIO_SHARE_MEM_SIZE	(0x32000)
//define VQE_DEBUG
//define error check macro for audio channels number / device numbers
#define AUDIO_CHECK_CHN_VALID(Y) ((((Y) >= _AUD_MAX_CHANNEL_NUM) || ((Y) < 0)) ? -1:1)
#define AUDIO_CHECK_DEV_VALID(DEV_ID) (((DEV_ID) >= _MAX_AI_DEVICE_ID_NUM) ? -1:1)

#ifndef SAFE_FREE_BUF
#define SAFE_FREE_BUF(OBJ) {if (OBJ != NULL) {kfree(OBJ); OBJ = NULL; } }
#endif

#ifndef SAFE_FREE_BUF_PHY
#define SAFE_FREE_BUF_PHY(VAR)  {if ((*VAR) != 0) {kfree(phys_to_virt((*VAR))); (*VAR) = 0; } }
#endif

//define log print for kernel mode ------------------------start

extern u32 audio_log_lv;
enum aud_msg_pri {
	AUDIO_ERR		= 0x1,
	AUDIO_WARN		= 0x2,
	AUDIO_NOTICE		= 0x4,
	AUDIO_INFO		= 0x8,
	AUDIO_DBG		= 0x10,
};
extern u32 audio_log_lv;
#define audio_pr(level, fmt, arg...) \
	do { \
		if (audio_log_lv & level) { \
			if (level == AUDIO_ERR) \
				pr_err("[audio_core]%s:%d(): " fmt, __func__, __LINE__, ## arg); \
			else if (level == AUDIO_WARN) \
				pr_warn("[audio_core]%s:%d(): " fmt, __func__, __LINE__, ## arg); \
			else if (level == AUDIO_NOTICE) \
				pr_notice("[audio_core]%s:%d(): " fmt, __func__, __LINE__, ## arg); \
			else if (level == AUDIO_INFO) \
				pr_err("[audio_core][info]%s:%d(): " fmt, __func__, __LINE__, ## arg); \
			else if (level == AUDIO_DBG) \
				pr_info("[audio_core]%s:%d(): " fmt, __func__, __LINE__, ## arg); \
		} \
	} while (0)
//define log print for kernel mode---------------------------stop
//global structure for audio device -----------------------start
struct audio_dev {
	struct device			*dev;
	struct class			*audio_class;
	struct cdev			cdev_ceo;
	dev_t				cdev_id;
	void				*shared_mem;
	int				audio_major;
	const struct vcodec_drv_ops	*pdata;
};

struct audio_status {
	int counter;
};

typedef struct audio_status st_audio_status;
//global structure for audio device -----------------------stop


#ifdef __cplusplus
}
#endif

#endif /* __AUDIO_INTERFACE_H__ */
