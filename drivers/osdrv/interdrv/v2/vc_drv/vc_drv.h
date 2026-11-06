#ifndef __DRV_VC_DRV_H__
#define __DRV_VC_DRV_H__

#include <linux/fs.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/of_device.h>
#include <linux/poll.h>
#include <linux/io.h>


#include <linux/vc_uapi.h>
#include "drv_venc.h"
#ifdef ENABLE_DEC
#include "drv_vdec.h"
#endif

#define DRV_VC_DRV_PLATFORM_DEVICE_NAME "vc_drv"
#define DRV_VC_DRV_CLASS_NAME "vc_drv"

typedef struct vc_drv_buffer_t {
    __u32 size;
    __u64 phys_addr;
    __u64 base; /* kernel logical address in use kernel */
    __u8 *virt_addr; /* virtual user space address */
#ifdef __arm__
    __u32 padding; /* padding for keeping same size of this structure */
#endif
} vc_drv_buffer_t;

struct vc_drv_ops {
    //    void    (*clk_get)(struct vc_drv_device *vdev);
};

struct vc_drv_pltfm_data {
    unsigned int quirks;
    unsigned int version;
};

struct vc_drv_device {
    struct device *dev;

    struct class *vc_class;

    dev_t venc_cdev_id;
    struct cdev *p_venc_cdev;
    int s_venc_major;

    dev_t vdec_cdev_id;
    struct cdev *p_vdec_cdev;
    int s_vdec_major;

    vc_drv_buffer_t ctrl_register;
    vc_drv_buffer_t remap_register;

    const struct vc_drv_ops *pdata;

    // padding buffer
    uint64_t align_physical;
    void *align_virtual;
    uint32_t align_size;
};

typedef struct _VENC_STREAM_EX_S {
    venc_stream_s *pstStream;
    int s32MilliSec;
} VENC_STREAM_EX_S;

typedef struct _VENC_USER_DATA_S {
    unsigned char *pu8Data;
    unsigned int u32Len;
} VENC_USER_DATA_S;

typedef struct _VIDEO_FRAME_INFO_EX_S {
    video_frame_info_s *pstFrame;
    int s32MilliSec;
} VIDEO_FRAME_INFO_EX_S;

typedef struct _USER_FRAME_INFO_EX_S {
    user_frame_info_s *pstUserFrame;
    int s32MilliSec;
} USER_FRAME_INFO_EX_S;

#ifdef ENABLE_DEC
typedef struct _VDEC_STREAM_EX_S {
    vdec_stream_s *pstStream;
    int s32MilliSec;
} VDEC_STREAM_EX_S;
#endif
typedef enum {
    REG_CTRL = 0,
    REG_REMAP,
} REG_TYPE;



#endif /* __DRV_VC_DRV_H__ */
