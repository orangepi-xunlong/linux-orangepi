#ifndef __VC_DRV_PROC_H__
#define __VC_DRV_PROC_H__

#include <linux/device.h>

#define VENC_PROC_NAME "soph/venc"
#define H265E_PROC_NAME "soph/h265e"
#define H264E_PROC_NAME "soph/h264e"
#define JPEGE_PROC_NAME "soph/jpege"
#define CODEC_PROC_NAME "soph/codec"
#define RC_PROC_NAME "soph/rc"
#define VDEC_PROC_NAME "soph/vdec"
#define VIDEO_PROC_PERMS (0644)
#define VIDEO_PROC_PARENT (NULL)
#define MAX_PROC_STR_SIZE (255)
#define MAX_DIR_STR_SIZE (255)

typedef struct _proc_debug_config_t {
    uint32_t u32DbgMask;
    uint32_t u32StartFrmIdx;
    uint32_t u32EndFrmIdx;
    char cDumpPath[MAX_DIR_STR_SIZE];
    uint32_t u32NoDataTimeout;
} proc_debug_config_t;

int venc_proc_init(struct device *dev);
int venc_proc_deinit(void);
int h265e_proc_init(struct device *dev);
int codecinst_proc_init(struct device *dev);
int codecinst_proc_deinit(void);

int h265e_proc_deinit(void);
int h264e_proc_init(struct device *dev);
int h264e_proc_deinit(void);
int jpege_proc_init(struct device *dev);
int jpege_proc_deinit(void);
int rc_proc_init(struct device *dev);
int rc_proc_deinit(void);
int vdec_proc_init(struct device *dev);
int vdec_proc_deinit(void);

#endif /* __VC_DRV_PROC_H__ */
