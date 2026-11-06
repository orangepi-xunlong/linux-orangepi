#ifndef __VI_SDK_LAYER_H__
#define __VI_SDK_LAYER_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <vi_defines.h>
#include <linux/types.h>
#include <vb.h>

/*****************************************************************************
 *  vi structure and enum for vi sdk layer
 ****************************************************************************/
struct sop_isp_buf {
	struct vi_buffer buf;
	struct list_head list;
};

/*****************************************************************************
 *  vi function prototype for vi sdk layer
 ****************************************************************************/
int vi_get_ion_buf(struct sop_vi_dev *vdev);
int vi_free_ion_buf(struct sop_vi_dev *dev);
int vi_create_thread(struct sop_vi_dev *vdev, enum E_VI_TH th_id);
void vi_destory_thread(struct sop_vi_dev *vdev, enum E_VI_TH th_id);
int vi_start_streaming(struct sop_vi_dev *vdev);
int vi_stop_streaming(struct sop_vi_dev *vdev);
void sop_isp_rdy_buf_queue(struct sop_vi_dev *vdev, struct sop_isp_buf *b);
int vi_mac_clk_ctrl(struct sop_vi_dev *vdev, u8 mac_num, u8 enable);
int usr_pic_timer_init(struct sop_vi_dev *vdev);
int user_pic_trig(struct sop_vi_dev *vdev);
void usr_pic_time_remove(void);
void vi_destory_dbg_thread(struct sop_vi_dev *vdev);

/*****************************************************************************
 *  vi sdk ioctl function prototype for vi layer
 ****************************************************************************/
int vi_disable_chn(int vi_chn);
long vi_sdk_ctrl(struct sop_vi_dev *vdev, struct vi_ext_control *p);
int vi_sdk_qbuf(mmf_chn_s chn, void *data);
void vi_fill_mlv_info(struct vb_s *blk, u8 dev, struct mlv_i_s *m_lv_i, u8 is_vpss_offline);
void vi_fill_dis_info(struct vb_s *blk, u8 dev);

#ifdef __cplusplus
}
#endif

#endif //__VI_SDK_LAYER_H__
