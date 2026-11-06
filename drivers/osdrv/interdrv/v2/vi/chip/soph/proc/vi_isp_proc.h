#ifndef __VI_ISP_PROC_H__
#define __VI_ISP_PROC_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/uaccess.h>
#include <vi_defines.h>


void vi_event_queue(struct sop_vi_dev *vdev, const u32 type, const u32 frm_num);
int isp_proc_init(struct sop_vi_dev *_vdev);
int isp_proc_remove(void);
int isp_proc_set_proc_content(void *buffer, size_t count);

#ifdef __cplusplus
}
#endif

#endif // __VI_ISP_PROC_H__