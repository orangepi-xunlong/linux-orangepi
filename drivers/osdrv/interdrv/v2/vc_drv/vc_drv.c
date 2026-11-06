#include <linux/slab.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/compat.h>

#include "vc_drv.h"
#include "h265_interface.h"
#include "jpuconfig.h"

unsigned int VENC_LOG_LV = 1;

static const struct of_device_id cvi_vc_drv_match_table[] = {
    { .compatible = "cvitek,cvi_vc_drv" },
    {},
};
MODULE_DEVICE_TABLE(of, cvi_vc_drv_match_table);

wait_queue_head_t tVencWaitQueue[VENC_MAX_CHN_NUM];
static DEFINE_SPINLOCK(vc_spinlock);


module_param(VENC_LOG_LV, int, 0644);

uint32_t MaxVencChnNum = VENC_MAX_CHN_NUM;
module_param(MaxVencChnNum, uint, 0644);
#ifdef ENABLE_DEC
uint32_t MaxVdecChnNum = VDEC_MAX_CHN_NUM;
module_param(MaxVdecChnNum, uint, 0644);
#endif
bool RcEn = 1;
module_param(RcEn, bool, 0644);

struct drv_vc_chn_info
{
    int channel_index;
    int is_encode;
    int is_jpeg;
    unsigned int ref_cnt;
    int is_channel_exist;
};

struct drv_vc_chn_info venc_chn_info[VENC_MAX_CHN_NUM];
struct drv_vc_chn_info vdec_chn_info[VDEC_MAX_CHN_NUM];


static uint32_t vencChnBitMap = 0;
static uint64_t vdecChnBitMap = 0;
static uint8_t  jpegEncChnBitMap[JPEG_MAX_CHN_NUM] = {0};
static uint8_t  jpegDecChnBitMap[JPEG_MAX_CHN_NUM] = {0};
static uint32_t jpegChnStart = 64;

wait_queue_head_t tVdecWaitQueue[VDEC_MAX_CHN_NUM];
static struct semaphore vencSemArry[VENC_MAX_CHN_NUM];
#ifdef ENABLE_DEC
static struct semaphore vdecSemArry[VDEC_MAX_CHN_NUM];
#endif
struct vc_drv_device *pVcDrvDevice;

struct clk_ctrl_info {
    int core_idx;
    int enable;
};

#ifdef VC_DRIVER_TEST
extern int jpeg_dec_test(u_long arg);
extern int jpeg_enc_test(u_long arg);
#endif

int jpeg_platform_init(struct platform_device *pdev);
void jpeg_platform_exit(void);
int vpu_drv_platform_init(struct platform_device *pdev);
int vpu_drv_platform_exit(void);
extern void drv_venc_deinit(void);
extern int drv_venc_init(void);
int vdec_drv_init(void);
void vdec_drv_deinit(void);


extern int venc_proc_init(struct device *dev);
extern int venc_proc_deinit(void);

extern int h265e_proc_init(struct device *dev);
extern int h265e_proc_deinit(void);
extern int codecinst_proc_init(struct device *dev);
extern int codecinst_proc_deinit(void);
extern int h264e_proc_init(struct device *dev);
extern int h264e_proc_deinit(void);
extern int jpege_proc_init(struct device *dev);
extern int jpege_proc_deinit(void);
extern int rc_proc_init(struct device *dev);
extern int rc_proc_deinit(void);

#ifdef ENABLE_DEC
extern int vdec_proc_init(struct device *dev);
extern int vdec_proc_deinit(void);
#endif


#if defined(CONFIG_PM)
int vpu_drv_suspend(struct platform_device *pdev, pm_message_t state);
int vpu_drv_resume(struct platform_device *pdev);
int jpeg_drv_suspend(struct platform_device *pdev, pm_message_t state);
int jpeg_drv_resume(struct platform_device *pdev);
#endif
void jpu_clk_disable(int core_idx);
void jpu_clk_enable(int core_idx);
static int _vc_drv_open(struct inode *inode, struct file *filp);
static long _vc_drv_venc_ioctl(struct file *filp, u_int cmd, u_long arg);
#ifdef ENABLE_DEC
static long _vc_drv_vdec_ioctl(struct file *filp, u_int cmd, u_long arg);
#endif
static int _vc_drv_venc_release(struct inode *inode, struct file *filp);
static int _vc_drv_vdec_release(struct inode *inode, struct file *filp);

static unsigned int _vc_drv_poll(struct file *filp,
                    struct poll_table_struct *wait);
static unsigned int _vdec_drv_poll(struct file *filp,
                    struct poll_table_struct *wait);


//extern unsigned long vpu_get_interrupt_reason(int coreIdx);
//extern unsigned long jpu_get_interrupt_flag(int chnIdx);

#ifdef CONFIG_COMPAT
static long _vc_drv_compat_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    if (!file->f_op->unlocked_ioctl)
        return -ENOIOCTLCMD;

    return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

const struct file_operations _vc_drv_venc_fops = {
    .owner = THIS_MODULE,
    .open = _vc_drv_open,
    .unlocked_ioctl = _vc_drv_venc_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = _vc_drv_compat_ptr_ioctl,
#endif
    .release = _vc_drv_venc_release,
    .poll = _vc_drv_poll,
};
#ifdef ENABLE_DEC
const struct file_operations _vc_drv_vdec_fops = {
    .owner = THIS_MODULE,
    .open = _vc_drv_open,
    .unlocked_ioctl = _vc_drv_vdec_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = _vc_drv_compat_ptr_ioctl,
#endif
    .poll = _vdec_drv_poll,
    .release = _vc_drv_vdec_release,
};
#endif
static int _vc_drv_open(struct inode *inode, struct file *filp)
{

    return 0;
}

static long _vc_drv_venc_ioctl(struct file *filp, u_int cmd, u_long arg)
{
    int s32Ret = -1;
    unsigned long   flags;
    struct drv_vc_chn_info *pstChnInfo = (struct drv_vc_chn_info *)filp->private_data;
    int minor;
    int isJpeg = 0, i = 0;

    if(pstChnInfo)
        minor = pstChnInfo->channel_index;
    if (cmd == DRV_VC_VCODEC_SET_CHN) {
        if (copy_from_user(&minor, (int *)arg, sizeof(int)) != 0) {
            pr_err("get chn fd failed.\n");
            return s32Ret;
        }

        if (minor < 0 || minor > VENC_MAX_CHN_NUM) {
            pr_err("invalid channel index %d.\n", minor);
            return -1;
        }
        spin_lock_irqsave(&vc_spinlock, flags);
        if(!pstChnInfo) {
            pstChnInfo = &venc_chn_info[minor];
            filp->private_data = &venc_chn_info[minor];
        }
        pstChnInfo->ref_cnt++;
        pstChnInfo->channel_index = minor;
        spin_unlock_irqrestore(&vc_spinlock, flags);
        return 0;
    } else if (cmd == DRV_VC_VCODEC_GET_CHN) {
        if (copy_from_user(&isJpeg, (int *)arg, sizeof(int)) != 0) {
            pr_err("get chn fd failed.\n");
            return s32Ret;
        }
        spin_lock_irqsave(&vc_spinlock, flags);
        if (isJpeg) {
            for (i = jpegChnStart; i < JPEG_MAX_CHN_NUM; i++) {
                if (jpegEncChnBitMap[i] == 0){
                    jpegEncChnBitMap[i] = 1;
                    venc_chn_info[i].is_jpeg = true;
                    spin_unlock_irqrestore(&vc_spinlock, flags);
                    return i;
                }
            }
            spin_unlock_irqrestore(&vc_spinlock, flags);
            pr_err("get chn fd failed, not enough jpeg enc chn\n");
            return -1;
        } else {
            for (i = 0; i < VC_MAX_CHN_NUM; i++){
                if (!(vencChnBitMap & (1 << i))) {
                    vencChnBitMap |= (1 << i);
                    venc_chn_info[i].is_jpeg = false;
                    spin_unlock_irqrestore(&vc_spinlock, flags);
                    return i;
                }
            }
            spin_unlock_irqrestore(&vc_spinlock, flags);
            pr_err("get chn fd failed, not enough venc chn\n");
            return -1;
        }
    }

    if (cmd != DRV_VC_VENC_ALLOC_PHYSICAL_MEMORY && cmd != DRV_VC_VENC_FREE_PHYSICAL_MEMORY) {
        if ((minor < 0 || minor > VENC_MAX_CHN_NUM)) {
            pr_err("invalid enc channel index %d.\n", minor);
            return -1;
        }
    } else {
        minor = 0;
    }

    if (down_interruptible(&vencSemArry[minor])) {
        return s32Ret;
    }

    switch (cmd) {
    case DRV_VC_VENC_CREATE_CHN: {
        venc_chn_attr_s stChnAttr;

        if (pstChnInfo->is_channel_exist) {
            s32Ret = DRV_ERR_VENC_EXIST;
            break;
        }
        if (copy_from_user(&stChnAttr, (venc_chn_attr_s *)arg,
                   sizeof(venc_chn_attr_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_create_chn(minor, &stChnAttr);
        if (s32Ret != 0) {
            pr_err("drv_venc_create_chn with %d\n", s32Ret);
        } else if (pstChnInfo){
            pstChnInfo->is_channel_exist = true;
        }
    } break;
    case DRV_VC_VENC_DESTROY_CHN: {
        s32Ret = drv_venc_destroy_chn(minor);
        if (s32Ret != 0) {
            pr_err("drv_venc_destroy_chn with %d\n", s32Ret);
        } else if (pstChnInfo){
            pstChnInfo->is_channel_exist = false;
        }
    } break;
    case DRV_VC_VENC_RESET_CHN: {
        s32Ret = drv_venc_reset_chn(minor);
        if (s32Ret != 0) {
            pr_err("drv_venc_reset_chn with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VENC_START_RECV_FRAME: {
        venc_recv_pic_param_s stRecvParam;

        if (copy_from_user(&stRecvParam, (venc_recv_pic_param_s *)arg,
                   sizeof(venc_recv_pic_param_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_start_recvframe(minor, &stRecvParam);
        if (s32Ret != 0) {
            pr_err("drv_venc_start_recvframe with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VENC_STOP_RECV_FRAME: {
        s32Ret = drv_venc_stop_recvframe(minor);
        if (s32Ret != 0) {
            pr_err("drv_venc_stop_recvframe with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VENC_QUERY_STATUS: {
        venc_chn_status_s stStatus;

        if (copy_from_user(&stStatus, (venc_chn_status_s *)arg,
                   sizeof(venc_chn_status_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_query_status(minor, &stStatus);
        if (s32Ret != 0) {
            pr_err("drv_venc_query_status with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((venc_chn_status_s *)arg, &stStatus,
                 sizeof(venc_chn_status_s)) != 0) {
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VENC_SET_CHN_ATTR: {
        venc_chn_attr_s stChnAttr;

        if (copy_from_user(&stChnAttr, (venc_chn_attr_s *)arg,
                   sizeof(venc_chn_attr_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_set_chn_attr(minor, &stChnAttr);
        if (s32Ret != 0) {
            pr_err("drv_venc_set_chn_attr with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VENC_GET_CHN_ATTR: {
        venc_chn_attr_s stChnAttr;

        s32Ret = drv_venc_get_chn_attr(minor, &stChnAttr);
        if (s32Ret != 0) {
            pr_err("drv_venc_get_chn_attr with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((venc_chn_attr_s *)arg, &stChnAttr,
                 sizeof(venc_chn_attr_s)) != 0) {
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VENC_GET_STREAM: {
        VENC_STREAM_EX_S stStreamEx;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        venc_stream_s stStream;
        venc_pack_s *pUserPack; // keep user space pointer on packs
#endif

        if (copy_from_user(&stStreamEx, (VENC_STREAM_EX_S *)arg,
                   sizeof(VENC_STREAM_EX_S)) != 0) {
            break;
        }

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        if (copy_from_user(&stStream, stStreamEx.pstStream,
                   sizeof(venc_stream_s)) != 0) {
            break;
        }

        // stStream.pstPack will be replaced by kernel space packs
        // in drv_venc_get_stream
        pUserPack = stStream.pstPack;
        stStream.pstPack = NULL;
        s32Ret = drv_venc_get_stream(minor, &stStream,
                        stStreamEx.s32MilliSec);
#else
        s32Ret = drv_venc_get_stream(minor, stStreamEx.pstStream,
                        stStreamEx.s32MilliSec);
#endif

        if (s32Ret != 0) {
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
            if (stStream.pstPack) {
                vfree(stStream.pstPack);
                stStream.pstPack = NULL;
            }
#endif
            break;
        }

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        // copy kernel space packs to user space
        if (stStream.pstPack) {
            if (copy_to_user(pUserPack, stStream.pstPack,
                     sizeof(venc_pack_s) *
                     stStream.u32PackCount) != 0) {

                if (stStream.pstPack) {
                    vfree(stStream.pstPack);
                    stStream.pstPack = NULL;
                }

                s32Ret = -1;
                break;
            }

            if (stStream.pstPack) {
                vfree(stStream.pstPack);
                stStream.pstPack = NULL;
            }
        }

        // restore user space pointer
        stStream.pstPack = pUserPack;
        if (copy_to_user(stStreamEx.pstStream, &stStream,
                 sizeof(venc_stream_s)) != 0) {
            pr_err("%s %d failed\n", __FUNCTION__, __LINE__);
            s32Ret = -1;
            break;
        }
#endif

        if (copy_to_user((VENC_STREAM_EX_S *)arg, &stStreamEx,
                 sizeof(VENC_STREAM_EX_S)) != 0) {
                 pr_err("%s %d failed\n", __FUNCTION__, __LINE__);
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VENC_RELEASE_STREAM: {
        venc_stream_s stStream;
        venc_pack_s *pstPack = NULL;

        if (copy_from_user(&stStream, (venc_stream_s *)arg,
                   sizeof(venc_stream_s)) != 0) {
            break;
        }

        if (stStream.u32PackCount > 0 && stStream.u32PackCount <= MAX_NUM_PACKS) {
            pstPack = vmalloc(sizeof(venc_pack_s) * stStream.u32PackCount);
            if (pstPack == NULL) {
                s32Ret = DRV_ERR_VENC_NOMEM;
                break;
            }

            if (copy_from_user(pstPack, stStream.pstPack,
                        sizeof(venc_pack_s) * stStream.u32PackCount) != 0) {
                vfree(pstPack);
                pstPack = NULL;
                break;
            }

            stStream.pstPack = pstPack;
        } else {
            pr_err("CVI_VENC_ReleaseStream invalid packs cnt:%d\n", stStream.u32PackCount);
            break;
        }

        s32Ret = drv_venc_release_stream(minor, &stStream);
        if (s32Ret != 0) {
            pr_err("drv_venc_release_stream with %d\n", s32Ret);
        }

        if (pstPack) {
            vfree(pstPack);
            pstPack = NULL;
        }
    } break;
    case DRV_VC_VENC_INSERT_USERDATA: {
        VENC_USER_DATA_S stUserData;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        __u8 *pUserData = NULL;
#endif

        if (copy_from_user(&stUserData, (VENC_USER_DATA_S *)arg,
                   sizeof(VENC_USER_DATA_S)) != 0) {
            break;
        }

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        pUserData = vmalloc(stUserData.u32Len);
        if (pUserData == NULL) {
            s32Ret = DRV_ERR_VENC_NOMEM;
            break;
        }

        if (copy_from_user(pUserData, stUserData.pu8Data,
                   stUserData.u32Len) != 0) {
            vfree(pUserData);
            break;
        }

        stUserData.pu8Data = pUserData;
#endif

        s32Ret = drv_venc_insert_userdata(minor, stUserData.pu8Data,
                         stUserData.u32Len);
        if (s32Ret != 0) {
            pr_err("drv_venc_insert_userdata with %d\n", s32Ret);
        }

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        if (pUserData)
            vfree(pUserData);
#endif
    } break;
    case DRV_VC_VENC_SEND_FRAME: {
        VIDEO_FRAME_INFO_EX_S stFrameEx;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        video_frame_info_s stFrame;
#endif

        if (copy_from_user(&stFrameEx, (VIDEO_FRAME_INFO_EX_S *)arg,
                   sizeof(VIDEO_FRAME_INFO_EX_S)) != 0) {
            break;
        }

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        if (copy_from_user(&stFrame, stFrameEx.pstFrame,
                   sizeof(video_frame_info_s)) != 0) {
            break;
        }
        stFrameEx.pstFrame = &stFrame;
#endif

        s32Ret = drv_venc_send_frame(minor, stFrameEx.pstFrame,
                        stFrameEx.s32MilliSec);
    } break;
    case DRV_VC_VENC_SEND_FRAMEEX: {
        USER_FRAME_INFO_EX_S stUserFrameEx;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        user_frame_info_s stUserFrameInfo;
        int w, h;
        __u8 *pu8QpMap = NULL;
#endif

        if (copy_from_user(&stUserFrameEx, (USER_FRAME_INFO_EX_S *)arg,
                   sizeof(USER_FRAME_INFO_EX_S)) != 0) {
            break;
        }

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        if (copy_from_user(&stUserFrameInfo, stUserFrameEx.pstUserFrame,
                   sizeof(user_frame_info_s)) != 0) {
            break;
        }
        stUserFrameEx.pstUserFrame = &stUserFrameInfo;

        w = (((stUserFrameInfo.stUserFrame.video_frame.width + 63) & ~63) >> 6);
        h = (((stUserFrameInfo.stUserFrame.video_frame.height + 63) & ~63) >> 6);
        pu8QpMap = vmalloc(w * h);
        if (pu8QpMap == NULL) {
            s32Ret = DRV_ERR_VENC_NOMEM;
            break;
        }

        if (copy_from_user(pu8QpMap, (__u8 *)stUserFrameInfo.stUserRcInfo.u64QpMapPhyAddr,
                   w * h) != 0) {
            vfree(pu8QpMap);
            break;
        }
        stUserFrameInfo.stUserRcInfo.u64QpMapPhyAddr = (__u64)pu8QpMap;
#endif

        s32Ret = drv_venc_send_frame_ex(minor, stUserFrameEx.pstUserFrame,
                          stUserFrameEx.s32MilliSec);

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        if (pu8QpMap)
            vfree(pu8QpMap);
#endif
    } break;
    case DRV_VC_VENC_REQUEST_IDR: {
        unsigned char bInstant;

        if (copy_from_user(&bInstant, (unsigned char *)arg,
                   sizeof(unsigned char)) != 0) {
            break;
        }

        s32Ret = drv_venc_request_idr(minor, bInstant);
        if (s32Ret != 0) {
            pr_err("drv_venc_request_idr with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VENC_ENABLE_IDR:{
        unsigned char bInstant;

        if (copy_from_user(&bInstant, (unsigned char *)arg,
                   sizeof(unsigned char)) != 0) {
            break;
        }

        s32Ret = drv_venc_enable_idr(minor, bInstant);
        if (s32Ret != 0) {
            pr_err("drv_venc_enable_idr with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VENC_SET_ROI_ATTR: {
        venc_roi_attr_s stRoiAttr;

        if (copy_from_user(&stRoiAttr, (venc_roi_attr_s *)arg,
                   sizeof(venc_roi_attr_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_set_roi_attr(minor, &stRoiAttr);
        if (s32Ret != 0) {
            pr_err("drv_venc_set_roi_attr with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VENC_GET_ROI_ATTR: {
        venc_roi_attr_s stRoiAttr;

        if (copy_from_user(&stRoiAttr, (venc_roi_attr_s *)arg,
                   sizeof(venc_roi_attr_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_get_roi_attr(minor, stRoiAttr.u32Index,
                         &stRoiAttr);
        if (s32Ret != 0) {
            pr_err("drv_venc_get_roi_attr with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((venc_roi_attr_s *)arg, &stRoiAttr,
                 sizeof(venc_roi_attr_s)) != 0) {
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VENC_SET_H264_TRANS: {
        venc_h264_trans_s stH264Trans;

        if (copy_from_user(&stH264Trans, (venc_h264_trans_s *)arg,
                   sizeof(venc_h264_trans_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_set_h264_trans(minor, &stH264Trans);
        if (s32Ret != 0) {
            pr_err("drv_venc_set_h264_trans with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VENC_GET_H264_TRANS: {
        venc_h264_trans_s stH264Trans;

        s32Ret = drv_venc_get_h264_trans(minor, &stH264Trans);
        if (s32Ret != 0) {
            pr_err("drv_venc_get_h264_trans with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((venc_h264_trans_s *)arg, &stH264Trans,
                 sizeof(venc_h264_trans_s)) != 0) {
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VENC_SET_H265_PRED_UNIT: {
        venc_h265_pu_s stH265PredUnit;

        if (copy_from_user(&stH265PredUnit,
                   (venc_h265_pu_s *)arg,
                   sizeof(venc_h265_pu_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_set_h265_predunit(minor, &stH265PredUnit);
        if (s32Ret != 0) {
            pr_err("drv_venc_set_h265_predunit with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VENC_GET_H265_PRED_UNIT: {
        venc_h265_pu_s stH265PredUnit;

        s32Ret = drv_venc_get_h265_predunit(minor, &stH265PredUnit);
        if (s32Ret != 0) {
            pr_err("drv_venc_get_h265_predunit with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((venc_h265_pu_s *)arg, &stH265PredUnit,
                 sizeof(venc_h265_pu_s)) != 0) {
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VENC_SET_H264_ENTROPY: {
        venc_h264_entropy_s stH264EntropyEnc;

        if (copy_from_user(&stH264EntropyEnc,
                   (venc_h264_entropy_s *)arg,
                   sizeof(venc_h264_entropy_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_set_h264_entropy(minor, &stH264EntropyEnc);
        if (s32Ret != 0) {
            pr_err("drv_venc_set_h264_entropy with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VENC_GET_H264_ENTROPY: {
        venc_h264_entropy_s stH264EntropyEnc;

        s32Ret = drv_venc_get_h264_entropy(minor, &stH264EntropyEnc);
        if (s32Ret != 0) {
            pr_err("drv_venc_get_h264_entropy with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((venc_h264_entropy_s *)arg, &stH264EntropyEnc,
                 sizeof(venc_h264_entropy_s)) != 0) {
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VENC_SET_H264_VUI: {
        venc_h264_vui_s stH264Vui;

        if (copy_from_user(&stH264Vui, (venc_h264_vui_s *)arg,
                   sizeof(venc_h264_vui_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_set_h264_vui(minor, &stH264Vui);
        if (s32Ret != 0) {
            pr_err("drv_venc_set_h264_vui with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VENC_GET_H264_VUI: {
        venc_h264_vui_s stH264Vui;

        s32Ret = drv_venc_get_h264_vui(minor, &stH264Vui);
        if (s32Ret != 0) {
            pr_err("drv_venc_get_h264_vui with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((venc_h264_vui_s *)arg, &stH264Vui,
                 sizeof(venc_h264_vui_s)) != 0) {
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VENC_SET_H265_VUI: {
        venc_h265_vui_s stH265Vui;

        if (copy_from_user(&stH265Vui, (venc_h265_vui_s *)arg,
                   sizeof(venc_h265_vui_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_set_h265_vui(minor, &stH265Vui);
        if (s32Ret != 0) {
            pr_err("drv_venc_set_h265_vui with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VENC_GET_H265_VUI: {
        venc_h265_vui_s stH265Vui;

        s32Ret = drv_venc_get_h265_vui(minor, &stH265Vui);
        if (s32Ret != 0) {
            pr_err("drv_venc_get_h265_vui with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((venc_h265_vui_s *)arg, &stH265Vui,
                 sizeof(venc_h265_vui_s)) != 0) {
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VENC_SET_JPEG_PARAM: {
        venc_jpeg_param_s stJpegParam;

        if (copy_from_user(&stJpegParam, (venc_jpeg_param_s *)arg,
                   sizeof(venc_jpeg_param_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_set_jpeg_param(minor, &stJpegParam);
        if (s32Ret != 0) {
            pr_err("drv_venc_set_jpeg_param with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VENC_GET_JPEG_PARAM: {
        venc_jpeg_param_s stJpegParam;

        s32Ret = drv_venc_get_jpeg_param(minor, &stJpegParam);
        if (s32Ret != 0) {
            pr_err("drv_venc_get_jpeg_param with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((venc_jpeg_param_s *)arg, &stJpegParam,
                 sizeof(venc_jpeg_param_s)) != 0) {
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VENC_SET_MJPEG_PARAM: {
        venc_mjpeg_param_s stMjpegParam;
        if (copy_from_user(&stMjpegParam, (venc_mjpeg_param_s *) arg,
            sizeof(venc_mjpeg_param_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_set_mjpeg_param(minor, &stMjpegParam);
        if (s32Ret != 0) {
            pr_err("drv_venc_set_mjpeg_param with:%d\n", s32Ret);
            break;
        }
    } break;

    case DRV_VC_VENC_GET_MJPEG_PARAM: {
        venc_mjpeg_param_s stMjpegParam;

        s32Ret = drv_venc_get_mjpeg_param(minor, &stMjpegParam);
        if (s32Ret != 0) {
            pr_err("drv_venc_get_mjpeg_param with:%d\n", s32Ret);
            break;
        }

        if (copy_to_user((venc_mjpeg_param_s *)arg, &stMjpegParam,
                sizeof(venc_mjpeg_param_s)) != 0) {
            pr_err("drv_venc_get_mjpeg_param with:%d", s32Ret);
            break;
        }
    } break;
    case DRV_VC_VENC_GET_RC_PARAM: {
        venc_rc_param_s stRcParam;

        s32Ret = drv_venc_get_rc_param(minor, &stRcParam);
        if (s32Ret != 0) {
            pr_err("drv_venc_get_rc_param with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((venc_rc_param_s *)arg, &stRcParam,
                 sizeof(venc_rc_param_s)) != 0) {
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VENC_SET_RC_PARAM: {
        venc_rc_param_s stRcParam;

        if (copy_from_user(&stRcParam, (venc_rc_param_s *)arg,
                   sizeof(venc_rc_param_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_set_rc_param(minor, &stRcParam);
        if (s32Ret != 0) {
            pr_err("drv_venc_set_rc_param with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VENC_SET_REF_PARAM: {
        venc_ref_param_s stRefParam;

        if (copy_from_user(&stRefParam, (venc_ref_param_s *)arg,
                   sizeof(venc_ref_param_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_set_ref_param(minor, &stRefParam);
        if (s32Ret != 0) {
            pr_err("drv_venc_set_ref_param with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VENC_GET_REF_PARAM: {
        venc_ref_param_s stRefParam;

        s32Ret = drv_venc_get_ref_param(minor, &stRefParam);
        if (s32Ret != 0) {
            pr_err("drv_venc_get_ref_param with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((venc_ref_param_s *)arg, &stRefParam,
                 sizeof(venc_ref_param_s)) != 0) {
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VENC_SET_H265_TRANS: {
        venc_h265_trans_s stH265Trans;

        if (copy_from_user(&stH265Trans, (venc_h265_trans_s *)arg,
                   sizeof(venc_h265_trans_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_set_h265_trans(minor, &stH265Trans);
        if (s32Ret != 0) {
            pr_err("drv_venc_set_h265_trans with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VENC_GET_H265_TRANS: {
        venc_h265_trans_s stH265Trans;

        s32Ret = drv_venc_get_h265_trans(minor, &stH265Trans);
        if (s32Ret != 0) {
            pr_err("drv_venc_get_h265_trans with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((venc_h265_trans_s *)arg, &stH265Trans,
                 sizeof(venc_h265_trans_s)) != 0) {
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VENC_SET_FRAMELOST_STRATEGY: {
        venc_framelost_s stFrmLostParam;

        if (copy_from_user(&stFrmLostParam, (venc_framelost_s *)arg,
                   sizeof(venc_framelost_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_set_framelost_strategy(minor, &stFrmLostParam);
        if (s32Ret != 0) {
            pr_err("drv_venc_set_framelost_strategy with %d\n",
                   s32Ret);
        }
    } break;
    case DRV_VC_VENC_GET_FRAMELOST_STRATEGY: {
        venc_framelost_s stFrmLostParam;

        s32Ret = drv_venc_get_framelost_strategy(minor, &stFrmLostParam);
        if (s32Ret != 0) {
            pr_err("drv_venc_get_framelost_strategy with %d\n",
                   s32Ret);
            break;
        }

        if (copy_to_user((venc_framelost_s *)arg, &stFrmLostParam,
                 sizeof(venc_framelost_s)) != 0) {
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VENC_SET_SUPERFRAME_STRATEGY: {
        venc_superframe_cfg_s stSuperFrmParam;

        if (copy_from_user(&stSuperFrmParam,
                   (venc_superframe_cfg_s *)arg,
                   sizeof(venc_superframe_cfg_s)) != 0) {
            break;
        }

        s32Ret =
            drv_venc_set_superframe_strategy(minor, &stSuperFrmParam);
        if (s32Ret != 0) {
            pr_err("drv_venc_set_superframe_strategy with %d\n",
                   s32Ret);
        }
    } break;
    case DRV_VC_VENC_GET_SUPERFRAME_STRATEGY: {
        venc_superframe_cfg_s stSuperFrmParam;

        s32Ret =
            drv_venc_get_superframe_strategy(minor, &stSuperFrmParam);
        if (s32Ret != 0) {
            pr_err("drv_venc_get_superframe_strategy with %d\n",
                   s32Ret);
            break;
        }

        if (copy_to_user((venc_superframe_cfg_s *)arg, &stSuperFrmParam,
                 sizeof(venc_superframe_cfg_s)) != 0) {
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VENC_SET_CHN_PARAM: {
        venc_chn_param_s stChnParam;

        if (copy_from_user(&stChnParam, (venc_chn_param_s *)arg,
                   sizeof(venc_chn_param_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_set_chn_param(minor, &stChnParam);
        if (s32Ret != 0) {
            pr_err("drv_venc_set_chn_param with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VENC_GET_CHN_PARAM: {
        venc_chn_param_s stChnParam;

        s32Ret = drv_venc_get_chn_param(minor, &stChnParam);
        if (s32Ret != 0) {
            pr_err("drv_venc_get_chn_param with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((venc_chn_param_s *)arg, &stChnParam,
                 sizeof(venc_chn_param_s)) != 0) {
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VENC_SET_MOD_PARAM: {
        venc_param_mod_s stModParam;

        if (copy_from_user(&stModParam, (venc_param_mod_s *)arg,
                   sizeof(venc_param_mod_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_set_modparam(&stModParam);
        if (s32Ret != 0) {
            pr_err("drv_venc_set_modparam with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VENC_GET_MOD_PARAM: {
        venc_param_mod_s stModParam;

        if (copy_from_user(&stModParam, (venc_param_mod_s *)arg,
                   sizeof(venc_param_mod_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_get_modparam(&stModParam);
        if (s32Ret != 0) {
            pr_err("drv_venc_get_modparam with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((venc_param_mod_s *)arg, &stModParam,
                 sizeof(venc_param_mod_s)) != 0) {
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VENC_ATTACH_VBPOOL: {
        venc_chn_pool_s stPool;

        if (copy_from_user(&stPool, (venc_chn_pool_s *)arg,
                   sizeof(venc_chn_pool_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_attach_vbpool(minor, &stPool);
        if (s32Ret != 0) {
            pr_err("drv_venc_attach_vbpool with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VENC_DETACH_VBPOOL: {
        s32Ret = drv_venc_detach_vbpool(minor);
        if (s32Ret != 0) {
            pr_err("drv_venc_detach_vbpool with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VENC_SET_CUPREDICTION: {
        venc_cu_prediction_s stCuPrediction;

        if (copy_from_user(&stCuPrediction, (venc_cu_prediction_s *)arg,
                   sizeof(venc_cu_prediction_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_set_cu_prediction(minor, &stCuPrediction);
        if (s32Ret != 0) {
            pr_err("drv_venc_set_cu_prediction with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VENC_GET_CUPREDICTION: {
        venc_cu_prediction_s stCuPrediction;

        s32Ret = drv_venc_get_cu_prediction(minor, &stCuPrediction);
        if (s32Ret != 0) {
            pr_err("drv_venc_get_cu_prediction with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((venc_cu_prediction_s *)arg, &stCuPrediction,
                 sizeof(venc_cu_prediction_s)) != 0) {
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VENC_CALC_FRAME_PARAM: {
        venc_frame_param_s stFrameParam;

        if (copy_from_user(&stFrameParam, (venc_frame_param_s *)arg,
                   sizeof(venc_frame_param_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_calc_frame_param(minor, &stFrameParam);
        if (s32Ret != 0) {
            pr_err("drv_venc_calc_frame_param with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VENC_SET_FRAME_PARAM: {
        venc_frame_param_s stFrameParam;

        if (copy_from_user(&stFrameParam, (venc_frame_param_s *)arg,
                   sizeof(venc_frame_param_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_set_frame_param(minor, &stFrameParam);
        if (s32Ret != 0) {
            pr_err("drv_venc_set_frame_param with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VENC_GET_FRAME_PARAM: {
        venc_frame_param_s stFrameParam;

        s32Ret = drv_venc_get_frame_param(minor, &stFrameParam);
        if (s32Ret != 0) {
            pr_err("drv_venc_get_frame_param with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((venc_frame_param_s *)arg, &stFrameParam,
                 sizeof(venc_frame_param_s)) != 0) {
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VENC_SET_H264_SLICE_SPLIT: {
        venc_h264_slice_split_s stH264Split;

        if (copy_from_user(&stH264Split, (venc_h264_slice_split_s *)arg,
                sizeof(venc_h264_slice_split_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_set_h264_slicesplit(minor, &stH264Split);
        if (s32Ret != 0) {
            pr_err("drv_venc_set_h264_slicesplit with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VENC_GET_H264_SLICE_SPLIT: {
        venc_h264_slice_split_s stH264Split;

        s32Ret = drv_venc_get_h264_slicesplit(minor, &stH264Split);
        if (s32Ret != 0) {
            pr_err("drv_venc_get_h264_slicesplit with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((venc_h264_slice_split_s *)arg, &stH264Split,
                 sizeof(venc_h264_slice_split_s)) != 0) {
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VENC_SET_H265_SLICE_SPLIT: {
        venc_h265_slice_split_s stH265Split;

        if (copy_from_user(&stH265Split, (venc_h265_slice_split_s *)arg,
                sizeof(venc_h265_slice_split_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_set_h265_slicesplit(minor, &stH265Split);
        if (s32Ret != 0) {
            pr_err("drv_venc_set_h265_slicesplit with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VENC_GET_H265_SLICE_SPLIT: {
        venc_h265_slice_split_s stH265Split;

        s32Ret = drv_venc_get_h265_slicesplit(minor, &stH265Split);
        if (s32Ret != 0) {
            pr_err("drv_venc_get_h265_slicesplit with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((venc_h265_slice_split_s *)arg, &stH265Split,
                 sizeof(venc_h265_slice_split_s)) != 0) {
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VENC_SET_H264_Dblk: {
        venc_h264_dblk_s stH264dblk;

        if (copy_from_user(&stH264dblk, (venc_h264_dblk_s *)arg,
                   sizeof(venc_h264_dblk_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_set_h264_dblk(minor, &stH264dblk);
        if (s32Ret != 0) {
            pr_err("drv_venc_set_h264_dblk with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VENC_GET_H264_Dblk: {
        venc_h264_dblk_s stH264dblk;

        s32Ret = drv_venc_get_h264_dblk(minor, &stH264dblk);
        if (s32Ret != 0) {
            pr_err("drv_venc_get_h264_dblk with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((venc_h264_dblk_s *)arg, &stH264dblk,
                 sizeof(venc_h264_dblk_s)) != 0) {
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VENC_SET_H265_Dblk: {
        venc_h265_dblk_s stH265dblk;

        if (copy_from_user(&stH265dblk, (venc_h265_dblk_s *)arg,
                   sizeof(venc_h265_dblk_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_set_h265_dblk(minor, &stH265dblk);
        if (s32Ret != 0) {
            pr_err("drv_venc_set_h265_dblk with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VENC_GET_H265_Dblk: {
        venc_h265_dblk_s stH265dblk;

        s32Ret = drv_venc_get_h265_dblk(minor, &stH265dblk);
        if (s32Ret != 0) {
            pr_err("drv_venc_get_h265_dblk with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((venc_h265_dblk_s *)arg, &stH265dblk,
                 sizeof(venc_h265_dblk_s)) != 0) {
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VENC_SET_H264_INTRA_PRED: {
        venc_h264_intra_pred_s stH264IntraPred;

        if (copy_from_user(&stH264IntraPred, (venc_h264_intra_pred_s *)arg,
                sizeof(venc_h264_intra_pred_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_set_h264_intrapred(minor, &stH264IntraPred);
        if (s32Ret != 0) {
            pr_err("drv_venc_set_h264_intrapred with %d\n", s32Ret);
            break;
        }
    } break;
    case DRV_VC_VENC_GET_H264_INTRA_PRED: {
        venc_h264_intra_pred_s stH264IntraPred;

        s32Ret = drv_venc_get_h264_intrapred(minor, &stH264IntraPred);
        if (s32Ret != 0) {
            pr_err("drv_venc_get_h264_intrapred with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((venc_h264_intra_pred_s *)arg, &stH264IntraPred,
                 sizeof(venc_h264_intra_pred_s)) != 0) {
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VENC_SET_CUSTOM_MAP: {
        venc_custom_map_s stVeCustomMap;

        if (copy_from_user(&stVeCustomMap, (venc_custom_map_s *)arg,
                sizeof(venc_custom_map_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_set_custommap(minor, &stVeCustomMap);
        if (s32Ret != 0) {
            pr_err("drv_venc_set_custommap with %d\n", s32Ret);
            break;
        }
    } break;
    case DRV_VC_VENC_GET_INTINAL_INFO: {
        venc_initial_info_s stVencInitialInfo;

        s32Ret = drv_venc_get_intialInfo(minor, &stVencInitialInfo);
        if (s32Ret != 0) {
            pr_err("drv_venc_get_intialInfo with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((venc_initial_info_s *)arg, &stVencInitialInfo,
                 sizeof(venc_initial_info_s)) != 0) {
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VENC_SET_H265_SAO: {
        venc_h265_sao_s stH265Sao;

        if (copy_from_user(&stH265Sao, (venc_h265_sao_s *)arg,
                   sizeof(venc_h265_sao_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_set_h265_sao(minor, &stH265Sao);
        if (s32Ret != 0) {
            pr_err("drv_venc_set_h265_sao with %d\n", s32Ret);
        }

    } break;
    case DRV_VC_VENC_GET_H265_SAO: {
        venc_h265_sao_s stH265Sao;

        s32Ret = drv_venc_get_h265_sao(minor, &stH265Sao);
        if (s32Ret != 0) {
            pr_err("drv_venc_get_h265_sao with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((venc_h265_sao_s *)arg, &stH265Sao,
                 sizeof(venc_h265_sao_s)) != 0) {
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VENC_GET_HEADER: {
        venc_encode_header_s stEncodeHeader;
        if (copy_from_user(&stEncodeHeader, (venc_encode_header_s *)arg,
                   sizeof(venc_encode_header_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_get_encode_header(minor, &stEncodeHeader);
        if (s32Ret != 0) {
            break;
        }

        if (copy_to_user((venc_encode_header_s *)arg, &stEncodeHeader,
            sizeof(venc_encode_header_s)) != 0) {
            pr_err("%s %d failed\n", __FUNCTION__, __LINE__);
            s32Ret = -1;
        }
    } break;

    case DRV_VC_VENC_GET_EXT_ADDR: {
        extern int vdi_get_ddr_map(unsigned long core_idx);
        int ext_addr = vdi_get_ddr_map(0);

        if (copy_to_user((int *)arg, &ext_addr, sizeof(int)) != 0) {
            pr_err("%s %d failed\n", __FUNCTION__, __LINE__);
            s32Ret = -1;
        }
    } break;

    case DRV_VC_VENC_SET_SEARCH_WINDOW: {
        venc_search_window_s stVencSearchWidow;

        if (copy_from_user(&stVencSearchWidow,
                   (venc_search_window_s *)arg,
                   sizeof(venc_search_window_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_set_search_window(minor, &stVencSearchWidow);
        if (s32Ret != 0) {
            pr_err("drv_venc_set_search_window with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VENC_GET_SEARCH_WINDOW: {
        venc_search_window_s stVencSearchWidow;

        s32Ret = drv_venc_get_search_window(minor, &stVencSearchWidow);
        if (s32Ret != 0) {
            pr_err("drv_venc_get_search_window with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((venc_search_window_s *)arg, &stVencSearchWidow,
                 sizeof(venc_search_window_s)) != 0) {
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VENC_SET_EXTERN_BUF: {
        venc_extern_buf_s stExternBuf;

        if (copy_from_user(&stExternBuf,
                   (venc_extern_buf_s *)arg,
                   sizeof(venc_extern_buf_s)) != 0) {
            break;
        }

        s32Ret = drv_venc_set_extern_buf(minor, &stExternBuf);
        if (s32Ret != 0) {
            pr_err("drv_venc_set_search_window with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VENC_ALLOC_PHYSICAL_MEMORY: {
        venc_phys_buf_s phys_buf;
        vpudrv_buffer_t vdb;

        if (copy_from_user(&phys_buf, (venc_phys_buf_s *)arg,
                   sizeof(venc_phys_buf_s)) != 0) {
            break;
        }

        osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));
        vdb.core_idx = 0;
        vdb.size = phys_buf.size;

        s32Ret = vpu_allocate_extern_memory(&vdb);
        if (s32Ret != 0) {
            pr_err("drv_venc_alloc_phys_buf with %d\n", s32Ret);
        }

        phys_buf.phys_addr = vdb.phys_addr;
        phys_buf.size = vdb.size;
        if (copy_to_user((venc_phys_buf_s *)arg, &phys_buf,
                 sizeof(venc_phys_buf_s)) != 0) {
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VENC_FREE_PHYSICAL_MEMORY: {
        vpudrv_buffer_t vdb;
        venc_phys_buf_s phys_buf;

        if (copy_from_user(&phys_buf,
                   (venc_phys_buf_s *)arg,
                   sizeof(venc_phys_buf_s)) != 0) {
            break;
        }

        osal_memset(&vdb, 0x00, sizeof(vpudrv_buffer_t));
        vdb.size = phys_buf.size;
        vdb.phys_addr = phys_buf.phys_addr;
        vdb.base = phys_buf.phys_addr;
        vdb.core_idx = 0;

        vpu_free_extern_memory(&vdb);
        s32Ret = 0;
    } break;
#ifdef VC_DRIVER_TEST
    case DRV_VC_VENC_ENC_JPEG_TEST: {
        s32Ret = jpeg_enc_test(arg);
    } break;
#endif
    default: {
        pr_err("venc un-handle cmd id: %x\n", cmd);
    } break;
    }

    up(&vencSemArry[minor]);
    return s32Ret;
}
#ifdef ENABLE_DEC
static long _vc_drv_vdec_ioctl(struct file *filp, u_int cmd, u_long arg)
{
    int s32Ret = -1;
    unsigned long   flags;
    struct drv_vc_chn_info *pstChnInfo = (struct drv_vc_chn_info *)filp->private_data;
    unsigned int minor;
    int isJpeg = 0, i = 0;

    if(pstChnInfo)
       minor = pstChnInfo->channel_index;
    if(cmd == DRV_VC_VCODEC_SET_CHN) {
        if (copy_from_user(&minor, (int *)arg, sizeof(int)) != 0) {
            pr_err("get chn fd failed.\n");
            return s32Ret;
        }

        if(minor < 0 || minor > VDEC_MAX_CHN_NUM) {
            pr_err("invalid channel index %d.\n", minor);
            return -1;
        }
        spin_lock_irqsave(&vc_spinlock, flags);
        if(!pstChnInfo) {
            pstChnInfo = &vdec_chn_info[minor];
            filp->private_data = &vdec_chn_info[minor];
        }
        pstChnInfo->ref_cnt++;
        pstChnInfo->channel_index = minor;
        spin_unlock_irqrestore(&vc_spinlock, flags);

        return 0;
    } else if (cmd == DRV_VC_VCODEC_GET_CHN) {
        if (copy_from_user(&isJpeg, (int *)arg, sizeof(int)) != 0) {
            pr_err("get chn fd failed.\n");
            return s32Ret;
        }
        spin_lock_irqsave(&vc_spinlock, flags);
        if (isJpeg) {
            for (i = jpegChnStart; i < JPEG_MAX_CHN_NUM; i++) {
                if (jpegDecChnBitMap[i] == 0){
                    jpegDecChnBitMap[i] = 1;
                    vdec_chn_info[i].is_jpeg = true;
                    spin_unlock_irqrestore(&vc_spinlock, flags);
                    return i;
                }
            }
            spin_unlock_irqrestore(&vc_spinlock, flags);
            pr_err("get chn fd failed, not enough jpeg dec chn\n");
            return -1;
        } else {
            for (i = 0; i < VC_MAX_CHN_NUM*2; i++){
                if (!(vdecChnBitMap & ((uint64_t)1 << i))) {
                    vdecChnBitMap |= ((uint64_t)1 << i);
                    vdec_chn_info[i].is_jpeg = false;
                    spin_unlock_irqrestore(&vc_spinlock, flags);
                    return i;
                }
            }
            spin_unlock_irqrestore(&vc_spinlock, flags);
            pr_err("get chn fd failed, not enough vdec chn\n");
            return -1;
        }
    }

    if (minor < 0 || minor > VDEC_MAX_CHN_NUM) {
        pr_err("invalid dec channel index %d.\n", minor);
        return -1;
    }

    if (down_interruptible(&vdecSemArry[minor])) {
        return s32Ret;
    }

    switch (cmd) {
    case DRV_VC_VDEC_CREATE_CHN: {
        vdec_chn_attr_s stChnAttr;
        buffer_info_s* bitstream_buffer = NULL;
	    buffer_info_s* frame_buffer = NULL;
	    buffer_info_s* Ytable_buffer = NULL;
	    buffer_info_s* Ctable_buffer = NULL;

        if (copy_from_user(&stChnAttr, (vdec_chn_attr_s *)arg,
                   sizeof(vdec_chn_attr_s)) != 0) {
            break;
        }

        if (pstChnInfo->is_channel_exist) {
            s32Ret = DRV_ERR_VDEC_EXIST;
            break;
        }
        if (stChnAttr.stBufferInfo.bitstream_buffer != NULL) {
            if(stChnAttr.enType == PT_JPEG || stChnAttr.enType == PT_MJPEG ||
                stChnAttr.enMode == VIDEO_MODE_STREAM) {
                bitstream_buffer = vmalloc(sizeof(buffer_info_s));
                if(bitstream_buffer == NULL) {
                    s32Ret = DRV_ERR_VDEC_NOMEM;
                    break;
                }

                if(copy_from_user(bitstream_buffer, stChnAttr.stBufferInfo.bitstream_buffer,
                    sizeof(buffer_info_s)) != 0) {
                    vfree(bitstream_buffer);
                    break;
                }
            } else {
                bitstream_buffer = vmalloc(sizeof(buffer_info_s) * stChnAttr.u8CommandQueueDepth);
                if(bitstream_buffer == NULL) {
                    s32Ret = DRV_ERR_VDEC_NOMEM;
                    break;
                }

                if(copy_from_user(bitstream_buffer, stChnAttr.stBufferInfo.bitstream_buffer,
                   sizeof(buffer_info_s) * stChnAttr.u8CommandQueueDepth) != 0) {
                    vfree(bitstream_buffer);
                    break;
                }
            }
        }

        if (stChnAttr.stBufferInfo.frame_buffer != NULL) {
            if(stChnAttr.enType == PT_JPEG || stChnAttr.enType == PT_MJPEG) {
                frame_buffer = vmalloc(sizeof(buffer_info_s));
                if (frame_buffer == NULL) {
                    s32Ret = DRV_ERR_VDEC_NOMEM;
                    if(bitstream_buffer) {
                        vfree(bitstream_buffer);
                    }
                    break;
                }

                if (copy_from_user(frame_buffer, stChnAttr.stBufferInfo.frame_buffer,
                    sizeof(buffer_info_s)) != 0) {
                    if(bitstream_buffer) {
                        vfree(bitstream_buffer);
                    }
                    vfree(frame_buffer);
                    break;
                }
            } else if(stChnAttr.enType == PT_H264 || stChnAttr.enType == PT_H265) {
                if(stChnAttr.stBufferInfo.Ytable_buffer == NULL &&
                    stChnAttr.stBufferInfo.Ctable_buffer == NULL) {
                    if(bitstream_buffer) {
                        vfree(bitstream_buffer);
                    }
                    s32Ret = DRV_ERR_VDEC_NOBUF;
                    break;
                }

                frame_buffer = vmalloc(sizeof(buffer_info_s) *
                    (stChnAttr.stBufferInfo.numOfDecFbc + stChnAttr.stBufferInfo.numOfDecwtl));
                if (frame_buffer == NULL) {
                    s32Ret = DRV_ERR_VDEC_NOMEM;
                    if(bitstream_buffer) {
                        vfree(bitstream_buffer);
                    }
                    break;
                }

                Ytable_buffer = vmalloc(sizeof(buffer_info_s) * stChnAttr.stBufferInfo.numOfDecFbc);
                if (Ytable_buffer == NULL) {
                    s32Ret = DRV_ERR_VDEC_NOMEM;
                    if(bitstream_buffer) {
                        vfree(bitstream_buffer);
                    }
                    vfree(frame_buffer);
                    break;
                }

                Ctable_buffer = vmalloc(sizeof(buffer_info_s) * stChnAttr.stBufferInfo.numOfDecFbc);
                if (Ctable_buffer == NULL) {
                    s32Ret = DRV_ERR_VDEC_NOMEM;
                    if(bitstream_buffer) {
                        vfree(bitstream_buffer);
                    }
                    vfree(frame_buffer);
                    vfree(Ytable_buffer);
                    break;
                }

                if (copy_from_user(frame_buffer, stChnAttr.stBufferInfo.frame_buffer, sizeof(buffer_info_s) *
                    (stChnAttr.stBufferInfo.numOfDecFbc + stChnAttr.stBufferInfo.numOfDecwtl)) != 0) {
                    if(bitstream_buffer) {
                        vfree(bitstream_buffer);
                    }
                    vfree(frame_buffer);
                    vfree(Ytable_buffer);
                    vfree(Ctable_buffer);
                    break;
                }

                if (copy_from_user(Ytable_buffer, stChnAttr.stBufferInfo.Ytable_buffer,
                    sizeof(buffer_info_s) * stChnAttr.stBufferInfo.numOfDecFbc) != 0) {
                    if(bitstream_buffer) {
                        vfree(bitstream_buffer);
                    }
                    vfree(frame_buffer);
                    vfree(Ytable_buffer);
                    vfree(Ctable_buffer);
                    break;
                }

                if (copy_from_user(Ctable_buffer, stChnAttr.stBufferInfo.Ctable_buffer,
                    sizeof(buffer_info_s) * stChnAttr.stBufferInfo.numOfDecFbc) != 0) {
                    if(bitstream_buffer) {
                        vfree(bitstream_buffer);
                    }
                    vfree(frame_buffer);
                    vfree(Ytable_buffer);
                    vfree(Ctable_buffer);
                    break;
                }
            }
        }

        stChnAttr.stBufferInfo.bitstream_buffer = bitstream_buffer;
        stChnAttr.stBufferInfo.frame_buffer = frame_buffer;
        stChnAttr.stBufferInfo.Ytable_buffer = Ytable_buffer;
        stChnAttr.stBufferInfo.Ctable_buffer = Ctable_buffer;
        s32Ret = drv_vdec_create_chn(minor, &stChnAttr);
        if (s32Ret != 0) {
            pr_err("drv_vdec_create_chn with %d\n", s32Ret);
        } else if (pstChnInfo){
            pstChnInfo->is_channel_exist = true;
        }

        if(bitstream_buffer) {
            vfree(bitstream_buffer);
        }
        if(frame_buffer) {
            vfree(frame_buffer);
            vfree(Ytable_buffer);
            vfree(Ctable_buffer);
        }
    } break;
    case DRV_VC_VDEC_DESTROY_CHN: {
        if (!pstChnInfo->is_channel_exist) {
            s32Ret = DRV_ERR_VDEC_UNEXIST;
            break;
        }
        s32Ret = drv_vdec_destroy_chn(minor);
        if (s32Ret != 0) {
            pr_err("drv_vdec_destroy_chn with %d\n", s32Ret);
        } else if (pstChnInfo){
            pstChnInfo->is_channel_exist = false;
        }
    } break;
    case DRV_VC_VDEC_GET_CHN_ATTR: {
        vdec_chn_attr_s stChnAttr;

        s32Ret = drv_vdec_get_chn_attr(minor, &stChnAttr);
        if (s32Ret != 0) {
            pr_err("drv_vdec_get_chn_attr with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((vdec_chn_attr_s *)arg, &stChnAttr,
                 sizeof(vdec_chn_attr_s)) != 0) {
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VDEC_SET_CHN_ATTR: {
        vdec_chn_attr_s stChnAttr;

        if (copy_from_user(&stChnAttr, (vdec_chn_attr_s *)arg,
                   sizeof(vdec_chn_attr_s)) != 0) {
            break;
        }

        s32Ret = drv_vdec_set_chn_attr(minor, &stChnAttr);
        if (s32Ret != 0) {
            pr_err("drv_vdec_set_chn_attr with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VDEC_START_RECV_STREAM: {
        s32Ret = drv_vdec_start_recv_stream(minor);
        if (s32Ret != 0) {
            pr_err("drv_vdec_start_recv_stream with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VDEC_STOP_RECV_STREAM: {
        s32Ret = drv_vdec_stop_recv_stream(minor);
        if (s32Ret != 0) {
            pr_err("drv_vdec_stop_recv_stream with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VDEC_QUERY_STATUS: {
        vdec_chn_status_s stStatus;

        if (copy_from_user(&stStatus, (vdec_chn_status_s *)arg,
                   sizeof(vdec_chn_status_s)) != 0) {
            break;
        }

        s32Ret = drv_vdec_query_status(minor, &stStatus);
        if (s32Ret != 0) {
            pr_err("drv_vdec_query_status with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((vdec_chn_status_s *)arg, &stStatus,
                 sizeof(vdec_chn_status_s)) != 0) {
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VDEC_RESET_CHN: {
        s32Ret = drv_vdec_reset_chn(minor);
        if (s32Ret != 0) {
            pr_err("drv_vdec_reset_chn with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VDEC_SET_CHN_PARAM: {
        vdec_chn_param_s stChnParam;

        if (copy_from_user(&stChnParam, (vdec_chn_param_s *)arg,
                   sizeof(vdec_chn_param_s)) != 0) {
            break;
        }

        s32Ret = drv_vdec_set_chn_param(minor, &stChnParam);
        if (s32Ret != 0) {
            pr_err("drv_vdec_set_chn_param with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VDEC_GET_CHN_PARAM: {
        vdec_chn_param_s stChnParam;

        s32Ret = drv_vdec_get_chn_param(minor, &stChnParam);
        if (s32Ret != 0) {
            pr_err("drv_vdec_get_chn_param with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((vdec_chn_param_s *)arg, &stChnParam,
                 sizeof(vdec_chn_param_s)) != 0) {
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VDEC_SEND_STREAM: {
        VDEC_STREAM_EX_S stStreamEx;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        vdec_stream_s stStream;
        __u8 *pStreamData = NULL;
#endif

        if (copy_from_user(&stStreamEx, (VDEC_STREAM_EX_S *)arg,
                   sizeof(VDEC_STREAM_EX_S)) != 0) {
            break;
        }

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        if (copy_from_user(&stStream, stStreamEx.pstStream,
                   sizeof(vdec_stream_s)) != 0) {
            break;
        }
        stStreamEx.pstStream = &stStream;

        if (stStream.u32Len) {
            pStreamData = vmalloc(stStream.u32Len);
            if (pStreamData == NULL) {
                s32Ret = DRV_ERR_VENC_NOMEM;
                break;
            }

            if (copy_from_user(pStreamData, stStream.pu8Addr,
                       stStream.u32Len) != 0) {
                vfree(pStreamData);
                break;
            }
        }
        stStream.pu8Addr = pStreamData;
#endif

        s32Ret = drv_vdec_send_stream(minor, stStreamEx.pstStream,
                         stStreamEx.s32MilliSec);
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        if (pStreamData)
            vfree(pStreamData);
#endif
    } break;
    case DRV_VC_VDEC_GET_FRAME: {
        VIDEO_FRAME_INFO_EX_S stFrameInfoEx;
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        video_frame_info_s stFrameInfo;
        video_frame_info_s *pUserFrameInfo; // keep user space pointer on frame info
#endif

        if (copy_from_user(&stFrameInfoEx, (VIDEO_FRAME_INFO_EX_S *)arg,
                   sizeof(VIDEO_FRAME_INFO_EX_S)) != 0) {
            break;
        }

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        if (copy_from_user(&stFrameInfo, stFrameInfoEx.pstFrame,
                   sizeof(video_frame_info_s)) != 0) {
            break;
        }
        pUserFrameInfo = stFrameInfoEx.pstFrame;
        stFrameInfoEx.pstFrame = &stFrameInfo; // replace user space pointer
#endif

        s32Ret = drv_vdec_get_frame(minor, stFrameInfoEx.pstFrame,
                       stFrameInfoEx.s32MilliSec);
        if (s32Ret != 0) {
            break;
        }

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
        if (copy_to_user(pUserFrameInfo, stFrameInfoEx.pstFrame,
                   sizeof(video_frame_info_s)) != 0) {
            break;
        }
        stFrameInfoEx.pstFrame = pUserFrameInfo; // restore user space pointer
#endif

        if (copy_to_user((VIDEO_FRAME_INFO_EX_S *)arg, &stFrameInfoEx,
                 sizeof(VIDEO_FRAME_INFO_EX_S)) != 0) {
            s32Ret = -1;
        }
    } break;
    case DRV_VC_VDEC_RELEASE_FRAME: {
        video_frame_info_s stFrameInfo;

        if (copy_from_user(&stFrameInfo, (video_frame_info_s *)arg,
                   sizeof(video_frame_info_s)) != 0) {
            break;
        }

        s32Ret = drv_vdec_release_frame(minor, &stFrameInfo);
        if (s32Ret != 0) {
            pr_err("drv_vdec_release_frame with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VDEC_ATTACH_VBPOOL: {
        vdec_chn_pool_s stPool;

        if (copy_from_user(&stPool, (vdec_chn_pool_s *)arg,
                   sizeof(vdec_chn_pool_s)) != 0) {
            break;
        }

        s32Ret = drv_vdec_attach_vbpool(minor, &stPool);
        if (s32Ret != 0) {
            pr_err("drv_vdec_attach_vbpool with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VDEC_DETACH_VBPOOL: {
        s32Ret = drv_vdec_detach_vbpool(minor);
        if (s32Ret != 0) {
            pr_err("drv_vdec_detach_vbpool with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VDEC_SET_MOD_PARAM: {
        vdec_mod_param_s stModParam;

        if (copy_from_user(&stModParam, (vdec_mod_param_s *)arg,
                   sizeof(vdec_mod_param_s)) != 0) {
            break;
        }

        s32Ret = drv_vdec_set_mod_param(&stModParam);
        if (s32Ret != 0) {
            pr_err("drv_vdec_set_mod_param with %d\n", s32Ret);
        }
    } break;
    case DRV_VC_VDEC_GET_MOD_PARAM: {
        vdec_mod_param_s stModParam;

        if (copy_from_user(&stModParam, (vdec_mod_param_s *)arg,
                   sizeof(vdec_mod_param_s)) != 0) {
            break;
        }

        s32Ret = drv_vdec_get_mod_param(&stModParam);
        if (s32Ret != 0) {
            pr_err("drv_vdec_get_mod_param with %d\n", s32Ret);
            break;
        }

        if (copy_to_user((vdec_mod_param_s *)arg, &stModParam,
                 sizeof(vdec_mod_param_s)) != 0) {
            s32Ret = -1;
        }
    } break;
#ifdef VC_DRIVER_TEST
    case DRV_VC_VDEC_DEC_JPEG_TEST: {
        s32Ret = jpeg_dec_test(arg);
    } break;
#endif
    case DRV_VC_VDEC_FRAME_ADD_USER: {
        video_frame_info_s stFrameInfo;

        if (copy_from_user(&stFrameInfo, (video_frame_info_s *)arg, sizeof(video_frame_info_s)) != 0) {
            break;
        }

        s32Ret = drv_vdec_frame_buffer_add_user(minor, &stFrameInfo);

    } break;

    case DRV_VC_VDEC_SET_STRIDE_ALIGN: {
        unsigned int align;

        if (copy_from_user(&align, (unsigned int *)arg, sizeof(unsigned int)) != 0) {
            break;
        }

        s32Ret = drv_vdec_set_stride_align(minor, align);
        if (s32Ret != 0) {
            pr_err("drv_vdec_set_stride_align with %d\n", s32Ret);
            break;
        }
    } break;

    case DRV_VC_VDEC_SET_USER_PIC: {
        video_frame_info_s usr_pic;

        if (copy_from_user(&usr_pic, (video_frame_info_s *)arg,
                   sizeof(video_frame_info_s)) != 0) {
            break;
        }

        s32Ret = drv_vdec_set_user_pic(minor, &usr_pic);
        if (s32Ret != 0) {
            pr_err("drv_vdec_set_user_pic with %d\n", s32Ret);
            break;
        }
    } break;

    case DRV_VC_VDEC_ENABLE_USER_PIC: {
        unsigned char instant;

        if (copy_from_user(&instant, (unsigned char *)arg, sizeof(unsigned char)) != 0) {
            break;
        }

        s32Ret = drv_vdec_enable_user_pic(minor, instant);
        if (s32Ret != 0) {
            pr_err("drv_vdec_enable_user_pic with %d\n", s32Ret);
            break;
        }
    } break;

    case DRV_VC_VDEC_DISABLE_USER_PIC: {
        s32Ret = drv_vdec_disable_user_pic(minor);
        if (s32Ret != 0) {
            pr_err("drv_vdec_disable_user_pic with %d\n", s32Ret);
            break;
        }
    } break;

    case DRV_VC_VDEC_SET_DISPLAY_MODE: {
        video_display_mode_e display_mode;

        if (copy_from_user(&display_mode, (video_display_mode_e *)arg,
            sizeof(video_display_mode_e)) != 0) {
            break;
        }

        s32Ret = drv_vdec_set_display_mode(minor, display_mode);
        if (s32Ret != 0) {
            pr_err("drv_vdec_set_display_mode with %d\n", s32Ret);
            break;
        }
    } break;

    default: {
        pr_err("vdec un-handle cmd id: %x\n", cmd);
    } break;
    }

    up(&vdecSemArry[minor]);
    return s32Ret;
}
#endif
extern int drv_venc_get_left_streamframes(int VeChn);
int vdec_get_output_frame_count(vdec_chn VdChn);

static unsigned int _vc_drv_poll(struct file *filp,
                    struct poll_table_struct *wait)
{
    unsigned int mask = 0;
    unsigned int minor = -1;
    struct drv_vc_chn_info *pstChnInfo = (struct drv_vc_chn_info *)filp->private_data;

    if(pstChnInfo && pstChnInfo->channel_index >= 0) {
        minor = pstChnInfo->channel_index;
        poll_wait(filp, &tVencWaitQueue[minor], wait);

        if(drv_venc_get_left_streamframes(minor) > 0) {
            mask |= POLLIN | POLLRDNORM;
        }
    }

    return mask;
}


static unsigned int _vdec_drv_poll(struct file *filp,
                    struct poll_table_struct *wait)
{
    unsigned int mask = 0;
    unsigned int minor = -1;
    struct drv_vc_chn_info *pstChnInfo = (struct drv_vc_chn_info *)filp->private_data;

    if(pstChnInfo && pstChnInfo->channel_index >= 0) {
        minor = pstChnInfo->channel_index;
        poll_wait(filp, &tVdecWaitQueue[minor], wait);

        if(vdec_get_output_frame_count(minor) > 0) {
            mask |= POLLIN | POLLRDNORM;
        }
    }

    return mask;
}

static int _vc_drv_venc_release(struct inode *inode, struct file *filp)
{
    struct drv_vc_chn_info *pstChnInfo = (struct drv_vc_chn_info *)filp->private_data;
    unsigned long   flags;

    if(pstChnInfo && pstChnInfo->channel_index >= 0) {
        spin_lock_irqsave(&vc_spinlock, flags);
        pstChnInfo->ref_cnt--;
        spin_unlock_irqrestore(&vc_spinlock, flags);
        if(!pstChnInfo->ref_cnt) {
            if(pstChnInfo->is_channel_exist) {
                drv_venc_stop_recvframe(pstChnInfo->channel_index);
                drv_venc_destroy_chn(pstChnInfo->channel_index);
                pstChnInfo->is_channel_exist = false;
            }
            spin_lock_irqsave(&vc_spinlock, flags);
            if (pstChnInfo->is_jpeg) {
                jpegEncChnBitMap[pstChnInfo->channel_index] = 0;
                pstChnInfo->is_jpeg = false;
            } else {
                vencChnBitMap &= ~(1<<pstChnInfo->channel_index);
            }
            spin_unlock_irqrestore(&vc_spinlock, flags);
            filp->private_data = NULL;
        }
    }

    return 0;
}

static int _vc_drv_vdec_release(struct inode *inode, struct file *filp)
{
    struct drv_vc_chn_info *pstChnInfo = (struct drv_vc_chn_info *)filp->private_data;
    unsigned long   flags;

    if(pstChnInfo && pstChnInfo->channel_index >= 0) {
        spin_lock_irqsave(&vc_spinlock, flags);
        pstChnInfo->ref_cnt--;
        spin_unlock_irqrestore(&vc_spinlock, flags);
        if(!pstChnInfo->ref_cnt) {
            if(pstChnInfo->is_channel_exist) {
                drv_vdec_destroy_chn(pstChnInfo->channel_index);
                pstChnInfo->is_channel_exist = false;
            }
            spin_lock_irqsave(&vc_spinlock, flags);
            if (pstChnInfo->is_jpeg) {
                jpegDecChnBitMap[pstChnInfo->channel_index] = 0;
                pstChnInfo->is_jpeg = false;
            } else {
                vdecChnBitMap &= ~((uint64_t)1<<pstChnInfo->channel_index);
            }
            spin_unlock_irqrestore(&vc_spinlock, flags);
            filp->private_data = NULL;
        }
    }

    return 0;
}


static int _vc_drv_register_cdev(struct vc_drv_device *vdev)
{
    int err = 0;
    int i = 0;

    vdev->vc_class = class_create(THIS_MODULE, DRV_VC_DRV_CLASS_NAME);
    if (IS_ERR(vdev->vc_class)) {
        pr_err("create class failed\n");
        return PTR_ERR(vdev->vc_class);
    }

    /* get the major number of the character device */
    if ((alloc_chrdev_region(&vdev->venc_cdev_id, 0, 1,
                 VC_DRV_ENCODER_DEV_NAME)) < 0) {
        err = -EBUSY;
        pr_err("could not allocate major number\n");
        return err;
    }
    vdev->s_venc_major = MAJOR(vdev->venc_cdev_id);
    vdev->p_venc_cdev = vzalloc(sizeof(struct cdev));

    {
        dev_t subDevice;
        subDevice = MKDEV(vdev->s_venc_major, 0);

        /* initialize the device structure and register the device with the kernel */
        cdev_init(vdev->p_venc_cdev, &_vc_drv_venc_fops);
        vdev->p_venc_cdev->owner = THIS_MODULE;

        if ((cdev_add(vdev->p_venc_cdev, subDevice, 1)) < 0) {
            err = -EBUSY;
            pr_err("could not allocate chrdev\n");
            return err;
        }

        device_create(vdev->vc_class, NULL, subDevice, NULL, "%s",
                  VC_DRV_ENCODER_DEV_NAME);
        for( i = 0; i <VENC_MAX_CHN_NUM; i++) {
            init_waitqueue_head(&tVencWaitQueue[i]);
            sema_init(&vencSemArry[i], 1);
        }
        memset(venc_chn_info, 0, sizeof(venc_chn_info));
    }
#ifdef ENABLE_DEC
    /* get the major number of the character device */
    if ((alloc_chrdev_region(&vdev->vdec_cdev_id, 0, 1,
                 VC_DRV_DECODER_DEV_NAME)) < 0) {
        err = -EBUSY;
        pr_err("could not allocate major number\n");
        return err;
    }
    vdev->s_vdec_major = MAJOR(vdev->vdec_cdev_id);
    vdev->p_vdec_cdev = vzalloc(sizeof(struct cdev));

    {
        dev_t subDevice;

        subDevice = MKDEV(vdev->s_vdec_major, 0);

        /* initialize the device structure and register the device with the kernel */
        cdev_init(vdev->p_vdec_cdev, &_vc_drv_vdec_fops);
        vdev->p_vdec_cdev->owner = THIS_MODULE;

        if ((cdev_add(vdev->p_vdec_cdev, subDevice, 1)) < 0) {
            err = -EBUSY;
            pr_err("could not allocate chrdev\n");
            return err;
        }

        device_create(vdev->vc_class, NULL, subDevice, NULL, "%s",
                  VC_DRV_DECODER_DEV_NAME);
        for (i = 0; i < VDEC_MAX_CHN_NUM; i++) {
            init_waitqueue_head(&tVdecWaitQueue[i]);
            sema_init(&vdecSemArry[i], 1);
        }
        memset(vdec_chn_info, 0, sizeof(vdec_chn_info));
    }
#endif

    return err;
}

static int vc_drv_plat_probe(struct platform_device *pdev)
{
    int ret = 0;
    ret = jpeg_platform_init(pdev);
    ret = vpu_drv_platform_init(pdev);
    ret = drv_venc_init();
    ret = vdec_drv_init();

    venc_proc_init(&pdev->dev);
    h265e_proc_init(&pdev->dev);
    h264e_proc_init(&pdev->dev);
    jpege_proc_init(&pdev->dev);
    rc_proc_init(&pdev->dev);
    vdec_proc_init(&pdev->dev);
    codecinst_proc_init(&pdev->dev);

    return ret;
}

static int vc_drv_plat_remove(struct platform_device *pdev)
{
    int ret = 0;
    jpeg_platform_exit();
    vpu_drv_platform_exit();
    drv_venc_deinit();
    vdec_drv_deinit();

    venc_proc_deinit();
    h265e_proc_deinit();
    h264e_proc_deinit();
    jpege_proc_deinit();
    rc_proc_deinit();
    #ifdef ENABLE_DEC
    vdec_proc_deinit();
    #endif
    codecinst_proc_deinit();

    return ret;
}

#if defined(CONFIG_PM)
int _vc_drv_suspend(struct platform_device *pdev, pm_message_t state)
{
    vpu_drv_suspend(pdev, state);
    jpeg_drv_suspend(pdev, state);
    pr_info("vc drv suspended\n");
    return 0;
}
int _vc_drv_resume(struct platform_device *pdev)
{
    vpu_drv_resume(pdev);
    jpeg_drv_resume(pdev);
    pr_info("vc drv resumed\n");
    return 0;
}
#endif



static const struct of_device_id vc_drv_match_table[] = {
    {.compatible = "sophgo,vc_drv"},
    {},
};

static struct platform_driver vc_plat_driver = {
    .driver = {
        .name = "sophgo,vc_drv",
        .of_match_table = vc_drv_match_table,
    },
    .probe      = vc_drv_plat_probe,
    .remove     = vc_drv_plat_remove,
    #if defined(CONFIG_PM)
    .suspend    = _vc_drv_suspend,
    .resume     = _vc_drv_resume,
    #endif
};

static int __init _vc_drv_init(void)
{
    int ret = 0;
    int core;
    struct vc_drv_device *vdev;

    vdev = vzalloc( sizeof(*vdev));
    if (!vdev) {
        return -ENOMEM;
    }

    memset(vdev, 0, sizeof(*vdev));

    ret = _vc_drv_register_cdev(vdev);
    if (ret < 0) {
        pr_err("_vc_drv_register_cdev fail\n");
        vfree(vdev);
        return -1;
    }

    pVcDrvDevice = vdev;

    ret = platform_driver_register(&vc_plat_driver);
    for (core = 0; core < MAX_NUM_VPU_CORE; core++) {
        vpu_clk_enable(core);
        vpu_clk_disable(core);
    }
    for (core = 0; core < MAX_NUM_JPU_CORE; core++) {
        jpu_clk_enable(core);
        jpu_clk_disable(core);
    }
    pr_info("_vc_drv_init result = 0x%x\n", ret);

    return ret;
}

static void __exit _vc_drv_exit(void)
{
    struct vc_drv_device *vdev = pVcDrvDevice;

    if (vdev->s_venc_major > 0) {
        dev_t subDevice;
        subDevice = MKDEV(vdev->s_venc_major, 0);
        cdev_del(vdev->p_venc_cdev);
        device_destroy(vdev->vc_class, subDevice);
        vfree(vdev->p_venc_cdev);
        unregister_chrdev_region(vdev->s_venc_major, 1);
        vdev->s_venc_major = 0;
    }

#ifdef ENABLE_DEC
    if (vdev->s_vdec_major > 0) {
        dev_t subDevice;

        subDevice = MKDEV(vdev->s_vdec_major, 0);
        cdev_del(vdev->p_vdec_cdev);
        device_destroy(vdev->vc_class, subDevice);
        vfree(vdev->p_vdec_cdev);
        unregister_chrdev_region(vdev->s_vdec_major, 1);
        vdev->s_vdec_major = 0;

    }
#endif
    class_destroy(vdev->vc_class);
    vfree(vdev);
    pVcDrvDevice = NULL;

    platform_driver_unregister(&vc_plat_driver);
}

MODULE_AUTHOR("vc sdk driver.");
MODULE_DESCRIPTION("vc sdk driver");
MODULE_LICENSE("GPL");

module_init(_vc_drv_init);
module_exit(_vc_drv_exit);
