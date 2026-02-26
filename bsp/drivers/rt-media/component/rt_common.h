/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/ktime.h>
#include <uapi/rt-media/uapi_rt_media.h>

#ifndef _RT_COMMON_H_
#define _RT_COMMON_H_

#ifndef LOG_TAG
#define LOG_TAG "rt"
#endif

#define RT_LOG_LEVEL_DEBUG "debug"
#define RT_LOG_LEVEL_INFO "info"
#define RT_LOG_LEVEL_WARNING "warn"
#define RT_LOG_LEVEL_ERROR "error"

#define RT_CLOSE_LOG_LEVEL (1)

#define TRY_TO_MODIFY 1

/*
RT_LOGD -- 7
RT_LOGI -- 6
RT_LOGW -- 4
RT_LOGE -- 3

as: echo 4 > /proc/sys/kernel/printk   --> just printk RT_LOGE
*/

#if RT_CLOSE_LOG_LEVEL
#define RT_LOGD(fmt, arg...)
#define RT_LOGI(fmt, arg...)
#else
#define RT_LOGD(fmt, arg...) printk(KERN_DEBUG "%s:%s <%s:%u> " fmt "\n", RT_LOG_LEVEL_DEBUG, LOG_TAG, __FUNCTION__, __LINE__, ##arg)
#define RT_LOGI(fmt, arg...)
#endif

#define RT_LOGS(fmt, arg...) printk(KERN_DEBUG "%s:%s <%s:%u> " fmt "\n", RT_LOG_LEVEL_DEBUG, LOG_TAG, __FUNCTION__, __LINE__, ##arg)

#define RT_LOGW(fmt, arg...) printk(KERN_WARNING "%s:%s <%s:%u> " fmt "\n", RT_LOG_LEVEL_WARNING, LOG_TAG, __FUNCTION__, __LINE__, ##arg)
#define RT_LOGE(fmt, arg...) printk(KERN_ERR "%s:%s <%s:%u> " fmt "\n", RT_LOG_LEVEL_ERROR, LOG_TAG, __FUNCTION__, __LINE__, ##arg)

#define PRINTF_FUNCTION RT_LOGW("Run this line")
#define RT_ALIGN(x, a) ((a) * (((x) + (a)-1) / (a)))

#ifndef PARAM_IN
#define PARAM_IN
#endif

#ifndef PARAM_OUT
#define PARAM_OUT
#endif

#ifndef PARAM_INOUT
#define PARAM_INOUT
#endif

#define LOCK_MUTEX(x)          \
	do {                   \
		mutex_lock(x); \
	} while (0)
#define UNLOCK_MUTEX(x)          \
	do {                     \
		mutex_unlock(x); \
	} while (0)

typedef int error_type;
#define ERROR_TYPE_OK (0)
#define ERROR_TYPE_ERROR (-1)
#define ERROR_TYPE_NOMEM (-2)
#define ERROR_TYPE_UNEXIST (-3)
#define ERROR_TYPE_STATE_ERROR (-4)
#define ERROR_TYPE_ILLEGAL_PARAM (-5)
#define ERROR_TYPE_VIN_ERR (-6)

#if defined CONFIG_RT_MEDIA_DUAL_SENSOR
#define RT_SECOND_SENSOR_KERNEL_START 0//set 1 to start second sensor kernel encoder
#endif

#define DEBUG_SHOW_ENCODE_TIME 1

typedef struct video_frame_s {
	unsigned int id;
	unsigned int width;
	unsigned int weight;

	unsigned char *phy_addr[3]; /* Y, U, V; Y, UV; Y, VU */
	void *vir_addr[3];
	unsigned int stride[3];

	unsigned int header_phy_addr[3];
	void *header_vir_addr[3];
	unsigned int header_stride[3];

	short offset_top; /* top offset of show area */
	short offset_nottom; /* bottom offset of show area */
	short offset_left; /* left offset of show area */
	short offset_right; /* right offset of show area */

	uint64_t pts; /* unit:us */
	unsigned int frame_cnt; /* rename mPrivateData to Framecnt_exp_start */

	void *private;
	int buf_size;
} video_frame_s;

typedef struct video_frame_node {
	video_frame_s video_frame;
	struct list_head mList;
} video_frame_node;

typedef enum comp_port_type {
	COMP_INPUT_PORT,
	COMP_OUTPUT_PORT
} comp_port_type;

typedef void *comp_handle;

typedef struct comp_tunnel_info {
	comp_handle tunnel_comp;
	unsigned int valid_flag;
} comp_tunnel_info;

typedef enum comp_command_type {
	COMP_COMMAND_INIT	   = 0,
	COMP_COMMAND_START	  = 1,
	COMP_COMMAND_PAUSE	  = 2,
	COMP_COMMAND_STOP	   = 3,
	COMP_COMMAND_RESET_HIGH_FPS = 4,
	COMP_COMMAND_EXIT	   = 5,
	COMP_COMMAND_VENC_INPUT_FRAME_VALID,

	COMP_COMMAND_MAX = 0X7FFFFFFF
} comp_command_type;

#define WAIT_REPLY_NUM (COMP_COMMAND_EXIT + 1)

typedef enum comp_state_type {
	// Error state.
	COMP_STATE_ERROR = 0,

	// rt_media was just created.
	COMP_STATE_IDLE = 1,

	// rt_media has been initialized.
	COMP_STATE_INITIALIZED = 2,

	// rt_media is ready to start.
	//RT_MEDIA_PREPARED              = 3,

	// rt_media is in progress.
	COMP_STATE_EXECUTING = 4,

	// rt_media is in pause.
	COMP_STATE_PAUSE = 5,
	COMP_STATE_EXIT = 6,
} comp_state_type;

typedef enum comp_index_type {
	COMP_INDEX_VENC_CONFIG_Base = 0x01000000,
	COMP_INDEX_VENC_CONFIG_Normal,
	COMP_INDEX_VENC_CONFIG_Dynamic_ForceKeyFrame,
	COMP_INDEX_VENC_CONFIG_Dynamic_Flush_buffer,

	COMP_INDEX_VENC_CONFIG_CATCH_JPEG_START,
	COMP_INDEX_VENC_CONFIG_CATCH_JPEG_STOP,
	COMP_INDEX_VENC_CONFIG_CATCH_JPEG_GET_DATA,

	COMP_INDEX_VENC_CONFIG_GET_VBV_BUF_INFO,
	COMP_INDEX_VENC_CONFIG_GET_STREAM_HEADER,
	COMP_INDEX_VENC_CONFIG_SET_OSD,
	COMP_INDEX_VENC_CONFIG_Dynamic_SET_QP_RANGE,
	COMP_INDEX_VENC_CONFIG_Dynamic_SET_BITRATE,
	COMP_INDEX_VENC_CONFIG_Dynamic_SET_FPS,
	COMP_INDEX_VENC_CONFIG_Dynamic_SET_VBR_PARAM,
	COMP_INDEX_VENC_CONFIG_Dynamic_SET_IS_NIGHT_CASE,
	COMP_INDEX_VENC_CONFIG_Dynamic_SET_SUPER_FRAME_PARAM,
	COMP_INDEX_VENC_CONFIG_Dynamic_GET_SUM_MB_INFO,
	COMP_INDEX_VENC_CONFIG_Dynamic_GET_MOTION_SEARCH_RESULT,
	COMP_INDEX_VENC_CONFIG_Dynamic_GET_REGION_D3D_RESULT,

	//COMP_INDEX_VENC_CONFIG_ENABLE_BIN_IMAGE,
	//COMP_INDEX_VENC_CONFIG_ENABLE_MV_INFO,
	//COMP_INDEX_VENC_CONFIG_GET_BIN_IMAGE_DATA,
	//COMP_INDEX_VENC_CONFIG_GET_MV_INFO_DATA,
	COMP_INDEX_VENC_CONFIG_SET_SHARP,
	COMP_INDEX_VENC_CONFIG_SET_MOTION_SEARCH_PARAM,
	COMP_INDEX_VENC_CONFIG_SET_IPC_CASE,
	COMP_INDEX_VENC_CONFIG_SET_ROI,
	COMP_INDEX_VENC_CONFIG_SET_GDC,
	COMP_INDEX_VENC_CONFIG_SET_ROTATE,
	COMP_INDEX_VENC_CONFIG_SET_REC_REF_LBC_MODE,
	COMP_INDEX_VENC_CONFIG_SET_WEAK_TEXT_TH,
	COMP_INDEX_VENC_CONFIG_SET_REGION_D3D_PARAM,
	COMP_INDEX_VENC_CONFIG_SET_CHROMA_QP_OFFSET,
	COMP_INDEX_VENC_CONFIG_SET_H264_CONSTRAINT_FLAG,
	COMP_INDEX_VENC_CONFIG_SET_VE2ISP_D2D_LIMIT,
	COMP_INDEX_VENC_CONFIG_SET_GRAY,
	COMP_INDEX_VENC_CONFIG_SET_WBYUV,
	COMP_INDEX_VENC_CONFIG_GET_WBYUV,
	COMP_INDEX_VENC_CONFIG_SET_2DNR,
	COMP_INDEX_VENC_CONFIG_SET_3DNR,
	COMP_INDEX_VENC_CONFIG_SET_CYCLE_INTRA_REFRESH,
	COMP_INDEX_VENC_CONFIG_SET_P_FRAME_INTRA,

	COMP_INDEX_VI_CONFIG_Base = 0x01000000,
	COMP_INDEX_VI_CONFIG_Normal,
	COMP_INDEX_VI_CONFIG_SET_YUV_BUF_INFO,
	COMP_INDEX_VI_CONFIG_SET_RESET_HIGH_FPS,
	COMP_INDEX_VI_CONFIG_ENABLE_HIGH_FPS,

	COMP_INDEX_VI_CONFIG_Dynamic_ForceKeyFrame,
	COMP_INDEX_VI_CONFIG_Dynamic_REQUEST_YUV_FRAME,
	COMP_INDEX_VI_CONFIG_Dynamic_RETURN_YUV_FRAME,
	COMP_INDEX_VI_CONFIG_Dynamic_GET_LV,
	COMP_INDEX_VI_CONFIG_Dynamic_SET_IR_PARAM,
	COMP_INDEX_VI_CONFIG_Dynamic_SET_H_FLIP,
	COMP_INDEX_VI_CONFIG_Dynamic_SET_V_FLIP,
	COMP_INDEX_VI_CONFIG_Dynamic_CATCH_JPEG,
	COMP_INDEX_VI_CONFIG_Dynamic_SET_POWER_LINE_FREQ,
	COMP_INDEX_VI_CONFIG_Dynamic_GET_EXP_GAIN,
	COMP_INDEX_VI_CONFIG_Dynamic_GET_HIST,
	COMP_INDEX_VI_CONFIG_Dynamic_SET_AE_METERING_MODE,
	COMP_INDEX_VI_CONFIG_Dynamic_SET_ISP_ARRT_CFG,
	COMP_INDEX_VI_CONFIG_Dynamic_GET_ISP_ARRT_CFG,
	COMP_INDEX_VI_CONFIG_Dynamic_SET_FPS,
	COMP_INDEX_VI_CONFIG_Dynamic_SET_ORL,

	COMP_INDEX_VI_CONFIG_GET_BASE_CONFIG,
} comp_index_type;

typedef enum comp_event_type {
	COMP_EVENT_CMD_COMPLETE,
	COMP_EVENT_CMD_ERROR,
	COMP_EVENT_CMD_RESET_ISP_HIGH_FPS,
	COMP_EVENT_MAX = 0x7FFFFFFF
} comp_event_type;

typedef struct comp_buffer_header_type {
	//* todo
	void *private;
	void *input_port_private;
	void *output_port_private;
} comp_buffer_header_type;

typedef struct comp_callback_type {
	error_type (*EventHandler)(PARAM_IN comp_handle component,
				   PARAM_IN void *pAppData,
				   PARAM_IN comp_event_type eEvent,
				   PARAM_IN unsigned int nData1,
				   PARAM_IN unsigned int nData2,
				   PARAM_IN void *pEventData);

	error_type (*empty_in_buffer_done)(PARAM_IN comp_handle component,
					   PARAM_IN void *pAppData,
					   PARAM_IN comp_buffer_header_type *pBuffer);

	error_type (*fill_out_buffer_done)(PARAM_IN comp_handle component,
					   PARAM_IN void *pAppData,
					   PARAM_IN comp_buffer_header_type *pBuffer);

} comp_callback_type;

//* set_config --> prepare --> start --> stop --> destroy
typedef struct rt_component_type {
	void *component_private;

	//void* application_private;

	error_type (*init)(PARAM_IN comp_handle component);

	error_type (*start)(PARAM_IN comp_handle component);

	error_type (*pause)(PARAM_IN comp_handle component);

	error_type (*stop)(PARAM_IN comp_handle component);

	error_type (*destroy)(PARAM_IN comp_handle component);

	error_type (*get_config)(PARAM_IN comp_handle component,
				 PARAM_IN comp_index_type index,
				 PARAM_INOUT void *param_data);

	error_type (*set_config)(PARAM_IN comp_handle component,
				 PARAM_IN comp_index_type index,
				 PARAM_IN void *param_data);

	error_type (*get_state)(PARAM_IN comp_handle component,
				PARAM_OUT comp_state_type *pState);

	/* please comp empty this in-buffer*/
	error_type (*empty_this_in_buffer)(PARAM_IN comp_handle component,
					   PARAM_IN comp_buffer_header_type *pBuffer);

	/* please comp fill this out-buffer*/
	error_type (*fill_this_out_buffer)(PARAM_IN comp_handle component,
					   PARAM_IN comp_buffer_header_type *pBuffer);

	error_type (*set_callbacks)(PARAM_IN comp_handle component,
				    PARAM_IN comp_callback_type *pCallbacks,
				    PARAM_IN void *pAppData);

	error_type (*setup_tunnel)(PARAM_IN comp_handle component,
				   PARAM_IN comp_port_type port_type,
				   PARAM_IN comp_handle tunnel_comp,
				   PARAM_IN int connect_flag);

} rt_component_type;

#define comp_init(component) \
	component->init(component)

#define comp_start(component) \
	component->start(component)

#define comp_pause(component) \
	component->pause(component)

#define comp_stop(component) \
	component->stop(component)

#define comp_destroy(component) \
	component->destroy(component)

#define comp_get_config(component, index, param) \
	component->get_config(component, index, param)

#define comp_set_config(component, index, param) \
	component->set_config(component, index, param)

#define comp_get_state(component, pstate) \
	component->get_state(component, pstate)

#define comp_empty_this_in_buffer(component, buffer) (((rt_component_type *)(component))->empty_this_in_buffer(component, buffer))

#define comp_fill_this_out_buffer(component, buffer) (((rt_component_type *)(component))->fill_this_out_buffer(component, buffer))

#define comp_set_callbacks(component, pcallback, pappdata) \
	(((rt_component_type *)(component))->set_callbacks(component, pcallback, pappdata))

#define comp_setup_tunnel(component, port_type, tunnel_comp, connect_flag) \
	((component)->setup_tunnel(component, port_type, tunnel_comp, connect_flag))

typedef error_type (*component_init)(PARAM_IN comp_handle component, const rt_media_config_s *pmedia_config);

static inline int64_t get_cur_time(void)
{
	struct timespec64 tv;
	int64_t time;
	memset(&tv, 0, sizeof(struct timespec64));
	ktime_get_real_ts64(&tv);
	time = tv.tv_sec * 1000000 + tv.tv_nsec/1000;
	return time;
}

#endif
