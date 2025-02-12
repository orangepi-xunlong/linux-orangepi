/*
 * The confidential and proprietary information contained in this file may
 * only be used by a person authorised under and to the extent permitted
 * by a subsisting licensing agreement from Arm Technology (China) Co., Ltd.
 *
 *            (C) COPYRIGHT 2021-2021 Arm Technology (China) Co., Ltd.
 *                ALL RIGHTS RESERVED
 *
 * This entire notice must be reproduced on all copies of this file
 * and copies of this file may only be made by a person if such person is
 * permitted to do so under the terms of a subsisting license agreement
 * from Arm Technology (China) Co., Ltd.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef _MVX_SESSION_H_
#define _MVX_SESSION_H_

/****************************************************************************
 * Includes
 ****************************************************************************/

#include <linux/kref.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/wait.h>
#include "mvx_buffer.h"
#include "mvx_firmware.h"
#include "mvx_firmware_cache.h"
#include "mvx_if.h"
#include "mvx_log_group.h"

/****************************************************************************
 * Defines
 ****************************************************************************/

#define MVX_SESSION_LOG(severity, session, msg, ...)		      \
	MVX_LOG_PRINT_SESSION(&mvx_log_session_if, severity, session, \
			      msg, ## __VA_ARGS__)

#define MVX_SESSION_VERBOSE(session, msg, ...) \
	MVX_SESSION_LOG(MVX_LOG_VERBOSE, session, msg, ## __VA_ARGS__)

#define MVX_SESSION_DEBUG(session, msg, ...) \
	MVX_SESSION_LOG(MVX_LOG_DEBUG, session, msg, ## __VA_ARGS__)

#define MVX_SESSION_INFO(session, msg, ...) \
	MVX_SESSION_LOG(MVX_LOG_INFO, session, msg, ## __VA_ARGS__)

#define MVX_SESSION_WARN(session, msg, ...) \
	MVX_SESSION_LOG(MVX_LOG_WARNING, session, msg, ## __VA_ARGS__)

#define MVX_SESSION_ERR(session, msg, ...) \
	MVX_SESSION_LOG(MVX_LOG_ERROR, session, msg, ## __VA_ARGS__)

/****************************************************************************
 * Types
 ****************************************************************************/

struct device;
struct file;
struct mvx_csched;
struct mvx_fw_cache;
struct poll_table_struct;

/**
 * enum mvx_session_event - Session events.
 * @MVX_SESSION_EVENT_BUFFER:		struct mvx_buffer.
 * @MVX_SESSION_EVENT_PORT_CHANGED:	enum mvx_direction.
 * @MVX_SESSION_EVENT_COLOR_DESC:	struct mvx_fw_color_desc.
 * @MVX_SESSION_EVENT_ERROR:		void
 */
enum mvx_session_event {
	MVX_SESSION_EVENT_BUFFER,
	MVX_SESSION_EVENT_PORT_CHANGED,
	MVX_SESSION_EVENT_COLOR_DESC,
	MVX_SESSION_EVENT_ERROR
};

/**
 * struct mvx_session_port - Session input and output port settings.
 * @format:		Port format.
 * @width:		Width in pixels.
 * @height:		Height in pixels.
 * @nplanes:		Number for planes for current format.
 * @stride:		Stride per line in bytes for each plane.
 * @size:		Size in bytes for each plane.
 * @afbc_alloc_bytes:	Minimum number of bytes required for AFBC.
 * @afbc_width:		AFBC width in superblocks.
 * @stream_on:		Boolean if the port has been enabled.
 * @buffer_min:		Minimum number of buffers required.
 * @buffer_count:	Number of buffers currently queued to firmware.
 * @buffer_queue:	Buffers waiting to be queued to the firmware.
 * @is_flushing:	Set true when port is waiting for a fw flush confirm.
 * @flushed:		Port has been flushed an no buffers have been queued.
 * @interlaced:		True if frames are interlaced.
 */
struct mvx_session_port {
	enum mvx_format format;
	unsigned int width;
	unsigned int height;
	uint8_t nplanes;
	unsigned int stride[MVX_BUFFER_NPLANES];
	unsigned int size[MVX_BUFFER_NPLANES];
	unsigned int afbc_alloc_bytes;
	unsigned int afbc_width;
	bool stream_on;
	unsigned int buffer_min;
	unsigned int buffer_count;
    unsigned int buffer_allocated;
	struct list_head buffer_queue;
	bool is_flushing;
	bool flushed;
	bool interlaced;
	unsigned int scaling_shift;
	struct mvx_roi_config roi_config_queue[MVX_ROI_QP_NUMS];
    int qp_queue[MVX_ROI_QP_NUMS];
	unsigned int roi_config_num;
    unsigned int qp_num;
    bool isreallocting;
    bool isallocparam;
    struct mvx_fw_seq_param seq_param;
    uint32_t buffer_on_hold_count;              /**< Number of buffers that the firmware has completed process of,
                                                 *   but is holding on to for frame reordering. */
    uint32_t pending_buffer_on_hold_count;      /**< Some FW versions signal the DPB in the form of a message
                                                 *   that will be valid once the next frame has been returned
                                                 *   by the FW. In these cases, the new DPB is stored in this
                                                 *   variable and copied to buffer_on_hold_count once the next
                                                 *   buffer arrives. */
    unsigned int buffer_size[MVX_BUFFER_NPLANES];
};

/**
 * struct mvx_session_qp - QP settings.
 * @i_frame:		QP for I frame.
 * @p_frame:            QP for P frame.
 * @b_frame:            QP for B frame.
 * @min:		Minimum QP value.
 * @max:		Maximum QP value.
 */
struct mvx_session_qp {
	int i_frame;
	int p_frame;
	int b_frame;
	int min;
	int max;
};

/**
 * struct mvx_session - Session instance.
 * @dev:		Pointer to device.
 * @cache:		Pointer to firmware cache.
 * @isession:		This instance is used to register the session to the
 *                      client.
 * @client_ops:		Client operations.
 * @csession:		Client session.
 * @destructor:		When the isession.kref reaches zero and after the
 * session
 *                      object has been destructed, this callback routine is
 * invoked
 *                      to allow the owner of the session object to clean up any
 *                      allocated resources.
 * @event:		Event callback routine.
 * @mutex:		Mutex protecting the session objects.
 * @port:		Input and output port settings.
 * @mmu:		MMU instance.
 * @fw:			Firmware instance.
 * @fw_bin:		Pointer to firmware binary.
 * @fw_event:		Event handler for loading a firmware binary.
 * @fw_state:		Current firmware state.
 * @waitq:		Wait queue to signal changes to the session.
 * @dentry:		Debugfs directory entry for the session.
 * @frame_rate:		Frame rate in Q16 format.
 * @target_bitrate:	Bitrate.
 * @rc_enabled:		Defines if rate control is enabled for the session.
 * @profile:		Profile for encoder.
 * @level:		Level for encoder.
 * @nalu_format:	NALU format.
 * @stream_escaping:	Defines if stream escaping is enabled.
 * @ignore_stream_headers:Defines if decoder should ignore stream headers.
 * @frame_reordering:	Defines if decoder should reorder frames.
 * @intbuf_size:	Suggested internal buffer size.
 * @p_frames:		Number of P-frames for encoder.
 * @b_frames:		Number of B-frames for encoder.
 * @gop_type:		GOP type.
 * @cyclic_intra_refresh_mb:Intra MB refresh.
 * @constr_ipred:	Constrained intra prediction.
 * @entropy_sync:	Enabled entropy synchronization.
 * @temporal_mvp:	Enable temporal motion vector prediction.
 * @tile_rows:		Tile size.
 * @tile_cols:		Tile size.
 * @min_luma_cb_size:	Minimum luma coding block size.
 * @mb_mask:		MB mask.
 * @entropy_mode:	Entropy mode.
 * @multi_slice_mode:	Multi slice mode.
 * @multi_slice_max_mb:	Maximum number of macroblocks in a slice.
 * @vp9_prob_update:	Probability update method.
 * @mv_h_search_range:	Horizontal search range.
 * @mv_v_search_range:	Vertical search range.
 * @bitdepth_chroma:	Bitdepth for chroma.
 * @bitdepth_luma:	Bitdepth for luma.
 * @force_chroma_format:Chroma format.
 * @rgb_to_yuv:		RGB to YUV conversion mode.
 * @band_limit:		Maximum bandwidth limit.
 * @cabac_init_idc:	CABAC initialization table.
 * @qp:			QP settings per codec.
 * @resync_interval:	JPEG resync interval.
 * @jpeg_quality:	JPEG quality level.
 * @color_desc:		HDR color description.
 *
 * There is one session for each file handle that has been opened from the
 * video device.
 *
 * There is a separate set of QP controls for every codec. There is no
 * information on which codec will be used when controls are initialized with
 * their default values. That's why a set of QP-settings is maintained for
 * every codec.
 */
struct mvx_session {
	struct device *dev;
	struct mvx_fw_cache *cache;
	struct mvx_if_session isession;
	struct mvx_client_ops *client_ops;
	struct mvx_client_session *csession;
	void (*destructor)(struct mvx_session *session);
	void (*event)(struct mvx_session *session,
		      enum mvx_session_event event,
		      void *arg);
	struct mvx_session_port port[MVX_DIR_MAX];
	struct mvx_mmu mmu;
	struct mvx_fw fw;
	struct mvx_fw_bin *fw_bin;
	struct mvx_fw_event fw_event;
	enum mvx_fw_state fw_state;
	wait_queue_head_t waitq;
	struct timer_list watchdog_timer;
	struct work_struct watchdog_work;
	unsigned int watchdog_count;
	bool switched_in;
	unsigned int idle_count;
	long error;
	struct dentry *dentry;

	int64_t frame_rate;
	unsigned int target_bitrate;
    unsigned int maximum_bitrate;
	bool rc_enabled;
    int rc_type;
	enum mvx_profile profile[MVX_FORMAT_BITSTREAM_LAST + 1];
	enum mvx_level level[MVX_FORMAT_BITSTREAM_LAST + 1];
	enum mvx_nalu_format nalu_format;
	enum mvx_tristate stream_escaping;
	enum mvx_tristate ignore_stream_headers;
	enum mvx_tristate frame_reordering;
	int64_t intbuf_size;
	int p_frames;
	int b_frames;
	enum mvx_gop_type gop_type;
	int cyclic_intra_refresh_mb;
	enum mvx_tristate constr_ipred;
	enum mvx_tristate entropy_sync;
	enum mvx_tristate temporal_mvp;
	int tile_rows;
	int tile_cols;
	int min_luma_cb_size;
	int mb_mask;
	enum mvx_entropy_mode entropy_mode;
	enum mvx_multi_slice_mode multi_slice_mode;
	int multi_slice_max_mb;
	enum mvx_vp9_prob_update vp9_prob_update;
	int mv_h_search_range;
	int mv_v_search_range;
	int bitdepth_chroma;
	int bitdepth_luma;
	int force_chroma_format;
	enum mvx_rgb_to_yuv_mode rgb_to_yuv;
	int band_limit;
	int cabac_init_idc;
	struct mvx_session_qp qp[MVX_FORMAT_BITSTREAM_LAST + 1];
	int resync_interval;
	int jpeg_quality;
	struct mvx_fw_color_desc color_desc;
    unsigned int crop_left;
    unsigned int crop_right;
    unsigned int crop_top;
    unsigned int crop_bottom;
    struct mvx_sei_userdata sei_userdata;
    unsigned int nHRDBufsize;
    struct mvx_dsl_frame dsl_frame;
    struct mvx_dsl_ratio dsl_ratio;
    struct mvx_long_term_ref mvx_ltr;
    int dsl_pos_mode;
    /*variable for DDR qos setting. */
    uint32_t estimated_ddr_read_throughput;    /**<estimated ddr throughput requirement for this session */
    uint32_t estimated_ddr_write_throughput;    /**<estimated ddr throughput requirement for this session */
    bool eos_queued;                            /**< Indicates last incoming buffer has EOS flag */
    bool keep_freq_high;                        /**< Control DVFS by avoiding low frequency at start of usecase */
    bool is_suspend;
    struct list_head buffer_corrupt_queue;
    unsigned int watchdog_timeout;             //timeout value[ms] for watchdog
    /*for debug. */
    uint64_t bus_write_bytes_total;
    uint64_t bus_read_bytes_total;
    uint64_t frame_id;
    int enable_profiling;
};

/****************************************************************************
 * Exported functions
 ****************************************************************************/

/**
 * mvx_session_construct - Construct the session object.
 * @session:	Pointer to session.
 * @dev:	Pointer to device.
 * @client_ops:	Pointer to client ops.
 * @cache:	Pointer to firmware cache.
 * @mutex:	Pointer to mutex protecting the session object.
 * @destructor:	Destructor that will be invoked after the session referece count
 *              has reached zero. The destructor may be NULL if the owner of the
 *              session object does not need to be notified.
 * @event:	Event notification from the session to the client. This function
 *              must not call session API which could take mvx_session mutex.
 * @dsession:	Debugfs directory entry for the session.
 *
 * Return: 0 on success, else error code.
 */
int mvx_session_construct(struct mvx_session *session,
			  struct device *dev,
			  struct mvx_client_ops *client_ops,
			  struct mvx_fw_cache *cache,
			  struct mutex *mutex,
			  void (*destructor)(struct mvx_session *session),
			  void (*event)(struct mvx_session *session,
					enum mvx_session_event event,
					void *arg),
			  struct dentry *dsession);

/**
 * mvx_session_construct - Destruct the session object.
 * @session:	Pointer to session.
 */
void mvx_session_destruct(struct mvx_session *session);

/**
 * mvx_session_get - Increment the session reference count.
 * @session:	Pointer to session.
 */
void mvx_session_get(struct mvx_session *session);

/**
 * mvx_session_put - Decrement the session reference count.
 * @session:	Pointer to session.
 *
 * If the reference count reaches 0 the session object will be destructed.
 *
 * Return: 1 if session was removed, else 0.
 */
int mvx_session_put(struct mvx_session *session);

/**
 * mvx_session_get_formats() - Get bitmask of supported formats.
 * @session:	Pointer to session.
 * @dir:	Which direction to get formats for.
 * @formats:	Pointer to bitmask listing supported formats.
 */
void mvx_session_get_formats(struct mvx_session *session,
			     enum mvx_direction dir,
			     uint64_t *formats);

/**
 * mvx_session_try_format() - Validate port format.
 * @session:	Pointer to session.
 * @dir:	Which direction to get formats for.
 * @format:	MVX format.
 * @width:	Width. Only valid for frame formats.
 * @height:	Height. Only valid for frame formats.
 * @nplanes:	Number for planes.
 * @stride:	Horizonal stride in bytes for each plane.
 * @size:	Size in bytes for each plane.
 * @interlace:	True if frames are interlaced.
 *
 * Return: 0 on success, else error code.
 */
int mvx_session_try_format(struct mvx_session *session,
			   enum mvx_direction dir,
			   enum mvx_format format,
			   unsigned int *width,
			   unsigned int *height,
			   uint8_t *nplanes,
			   unsigned int *stride,
			   unsigned int *size,
			   bool *interlaced);

/**
 * mvx_session_set_format() - Validate and set port format.
 * @session:	Pointer to session.
 * @dir:	Which direction to get formats for.
 * @format:	MVX format.
 * @width:	Width. Only valid for frame formats.
 * @height:	Height. Only valid for frame formats.
 * @nplanes:	Number for planes.
 * @stride:	Horizonal stride in bytes for each plane.
 * @size:	Size in bytes for each plane.
 * @interlaced:	True if frames are interlaced.
 *
 * If *nplanes is 0, then the values of stride and size should be ignored, else
 * size and stride should be used when setting the format.
 *
 * Return: 0 on success, else error code.
 */
int mvx_session_set_format(struct mvx_session *session,
			   enum mvx_direction dir,
			   enum mvx_format format,
			   unsigned int *width,
			   unsigned int *height,
			   uint8_t *nplanes,
			   unsigned int *stride,
			   unsigned int *size,
			   bool *interlaced);

/**
 * mvx_session_qbuf() - Queue a buffer.
 * @session:	Pointer to session.
 * @buf:	Pointer to buffer.
 *
 * Return: 0 on success, else error code.
 */
int mvx_session_qbuf(struct mvx_session *session,
		     enum mvx_direction dir,
		     struct mvx_buffer *buf);

/**
 * mvx_session_send_eos() - Queue an empty buffer with EOS flag.
 * @session:	Pointer to session.
 *
 * If firmware is loaded an empty input buffer will be queued with the EOS flag
 * set. EOS will be propagated by the firmware to the output queue.
 *
 * If the firmware is not loaded a buffer will be dequeued from the output
 * queue, cleared and returned with the EOS flag set.
 *
 * Return: 0 on success, else error code.
 */
int mvx_session_send_eos(struct mvx_session *session);

/**
 * mvx_session_streamon() - Enable stream on input or output port.
 * @session:	Pointer to session.
 * @dir:	Port direction.
 *
 * Both input and output ports must be enabled for streaming to begin.
 *
 * Return: 0 on success, else error code.
 */
int mvx_session_streamon(struct mvx_session *session,
			 enum mvx_direction dir);

/**
 * mvx_session_streamoff() - Disable stream on input or output port.
 * @session:	Pointer to session.
 * @dir:	Port direction.
 *
 * Return: 0 on success, else error code.
 */
int mvx_session_streamoff(struct mvx_session *session,
			  enum mvx_direction dir);

/**
 * mvx_session_irq() - Handle IRQ event from the client.
 * @isession:	Pointer to if-session.
 */
void mvx_session_irq(struct mvx_if_session *isession);

/**
 * mvx_if_session_to_session() - Convert mvx_is_session to mvx_session.
 * @session:	Pointer to mvx_if_session object.
 *
 * Return: Pointer to mvx_session object.
 */
static inline struct mvx_session *mvx_if_session_to_session(
	struct mvx_if_session *session)
{
	return container_of(session, struct mvx_session, isession);
}

/**
 * mvx_session_port_show() - Print debug information into seq-file.
 * @port:	Pointer to port.
 * @s:		Seq-file to print to.
 */
void mvx_session_port_show(struct mvx_session_port *port,
			   struct seq_file *s);

/*
 * Functions bellow implement different settings for a session.
 *
 * Most of options could be set only when the FW is in STOPPED state or not
 * loaded. In this case the value will be stored in mvx_session structure
 * and applied lated in fw_initial_setup().
 *
 * Some options support runtime modification. For them we issue a command
 * to mvx_fw module if the FW is loaded. For others we return -EBUSY if the
 * FW is loaded.
 *
 * ATTENTION. Currently there is no way to querry from mvx_fw API of from
 * mvx_session API if the option supports runtime configuration.
 */

/**
 * mvx_session_set_securevideo() - Enabled or disable secure video.
 * @session:	Session.
 * @securevideo:Enable or disable secure video.
 *
 * Return: 0 on success, else error code.
 */
int mvx_session_set_securevideo(struct mvx_session *session,
				bool securevideo);

/**
 * mvx_session_set_frame_rate() - Set frame rate.
 * @session:	Session.
 * @frame_rate:	Frame rate in Q16 format.
 *
 * This option could be set in runtime.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_frame_rate(struct mvx_session *session,
			       int64_t frame_rate);

/**
 * mvx_session_set_rate_control() - Enable/disable rate controller.
 * @session:	Session.
 * @enabled:	Rate controller status.
 *
 * This option could be set in runtime.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_rate_control(struct mvx_session *session,
				 bool enabled);

/**
 * mvx_session_set_bitrate() - Set bitrate rate.
 * @session:	Session.
 * @bitrate:	Bitrate in bits per second.
 *
 * This option could be set in runtime.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_bitrate(struct mvx_session *session,
			    int bitrate);

/**
 * mvx_session_set_bitrate() - Set bitrate rate control.
 * @session:	Session.
 * @rc_type:	bitrate rate control type.
 *
 * This option could be set in runtime.
 *
 * Return: 0 in case of success, error code otherwise.
 */

int mvx_session_set_bitrate_control(struct mvx_session *session,
			    struct mvx_buffer_param_rate_control *rc_type);
/**
 * mvx_session_set_crop_left() - Set crop left.
 * @session:	Session.
 * @left:	    encoder SPS crop param, left offset.
 *
 * Return: 0 in case of success, error code otherwise.
 */

int mvx_session_set_crop_left(struct mvx_session *session,
			    int32_t left);

/**
 * mvx_session_set_crop_right() - Set crop right.
 * @session:	Session.
 * @right:	    encoder SPS crop param, right offset.
 *
 * Return: 0 in case of success, error code otherwise.
 */

int mvx_session_set_crop_right(struct mvx_session *session,
			    int32_t right);

/**
 * mvx_session_set_crop_top() - Set crop top.
 * @session:	Session.
 * @top:	    encoder SPS crop param, top offset.
 *
 * Return: 0 in case of success, error code otherwise.
 */

int mvx_session_set_crop_top(struct mvx_session *session,
			    int32_t top);

/**
 * mvx_session_set_crop_bottom() - Set crop bottom.
 * @session:	Session.
 * @top:	    encoder SPS crop param, bottom offset.
 *
 * Return: 0 in case of success, error code otherwise.
 */

int mvx_session_set_crop_bottom(struct mvx_session *session,
			    int32_t bottom);
/**
 * mvx_session_set_nalu_format() - Set NALU format.
 * @session:	Session.
 * @fmt:	NALU format.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_nalu_format(struct mvx_session *session,
				enum mvx_nalu_format fmt);

/**
 * mvx_session_set_stream_escaping() - Enable/disable stream escaping
 * @session:	Session.
 * @status:	Status
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_stream_escaping(struct mvx_session *session,
				    enum mvx_tristate status);

/**
 * mvx_session_set_profile() - Set profile for encoder.
 * @session:	Session.
 * @format:	Format.
 * @profile:	Encoder profile.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_profile(struct mvx_session *session,
			    enum mvx_format format,
			    enum mvx_profile profile);

/**
 * mvx_session_set_level() - Set level for encoder.
 *
 * @session:	Session.
 * @format:	Format.
 * @level:	Encoder level.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_level(struct mvx_session *session,
			  enum mvx_format format,
			  enum mvx_level level);

/**
 * mvx_session_set_ignore_stream_headers() - Enable/disable stream headers
 * ignore.
 * @session:	Session.
 * @status:	Status.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_ignore_stream_headers(struct mvx_session *session,
					  enum mvx_tristate status);

/**
 * mvx_session_set_frame_reordering() - Enable/disable frames reordering.
 * @session:	Session.
 * @status:	Status.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_frame_reordering(struct mvx_session *session,
				     enum mvx_tristate status);

/**
 * mvx_session_set_intbuf_size() - Set internal buffer size.
 * @session:	Session.
 * @size:	Size.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_intbuf_size(struct mvx_session *session,
				int size);

/**
 * mvx_session_set_p_frame() - Set number of P-frames.
 * @session:	Session.
 * @val:	Number of P-frames.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_p_frames(struct mvx_session *session,
			     int val);

/**
 * mvx_session_set_b_frame() - Set number of B-frames.
 * @session:	Session.
 * @val:	Number of B-frames.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_b_frames(struct mvx_session *session,
			     int val);

/**
 * mvx_session_set_gop_type() - Set GOP type.
 * @session:	Session.
 * @gop_type:	GOP type.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_gop_type(struct mvx_session *session,
			     enum mvx_gop_type gop_type);

/**
 * mvx_session_set_cyclic_intra_refresh_mb() - Set intra MB refresh.
 * @session:	Session.
 * @val:	Value.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_cyclic_intra_refresh_mb(struct mvx_session *session,
					    int val);

/**
 * mvx_session_set_constr_ipred() - Enabled/disable constrained intra
 * prediction.
 * @session:	Session.
 * @status:	Status.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_constr_ipred(struct mvx_session *session,
				 enum mvx_tristate status);

/**
 * mvx_session_set_entropy_sync() - Enable/disable entropy synchronization.
 * @session:	Session.
 * @status:	Status.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_entropy_sync(struct mvx_session *session,
				 enum mvx_tristate status);

/**
 * mvx_session_set_temporal_mvp() - Enable/disable temporal MVP.
 * @session:	Session.
 * @status:	Status.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_temporal_mvp(struct mvx_session *session,
				 enum mvx_tristate status);

/**
 * mvx_session_set_tile_rows() - Set tile size.
 * @session:	Session.
 * @val:	Value.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_tile_rows(struct mvx_session *session,
			      int val);

/**
 * mvx_session_set_tile_cols() - Set tile size.
 * @session:	Session.
 * @val:	Value.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_tile_cols(struct mvx_session *session,
			      int val);

/**
 * mvx_session_set_min_luma_cb_size() - Set minimum luma coding block size.
 * @session:	Session.
 * @val:	Value.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_min_luma_cb_size(struct mvx_session *session,
				     int val);

/**
 * mvx_session_set_mb_mask() - Set MB mask.
 * @session:	Session.
 * @val:	Value.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_mb_mask(struct mvx_session *session,
			    int val);

/**
 * mvx_session_set_entropy_mode() - Set entropy mode.
 * @session:	Session.
 * @mode:	Entropy mode.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_entropy_mode(struct mvx_session *session,
				 enum mvx_entropy_mode mode);

/**
 * mvx_session_set_multi_slice_mode() - Set multi slice mode.
 * @session:	Session.
 * @mode:	Mode.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_multi_slice_mode(struct mvx_session *session,
				     enum mvx_multi_slice_mode mode);

/**
 * mvx_session_set_multi_slice_max_mb() - Set suggested number of CTUs in a
 * slice.
 * @session:	Session.
 * @val:	Value.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_multi_slice_max_mb(struct mvx_session *session,
				       int val);

/**
 * mvx_session_set_vp9_prob_update() - Set probability update mode.
 * @session:	Session.
 * @mode:	Mode.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_vp9_prob_update(struct mvx_session *session,
				    enum mvx_vp9_prob_update mode);

/**
 * mvx_session_set_mv_h_search_range() - Set horizontal search range for motion
 * vectors.
 * @session:	Session.
 * @val:	Value.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_mv_h_search_range(struct mvx_session *session,
				      int val);

/**
 * mvx_session_set_mv_v_search_range() - Set vertical search range for motion
 * vectors.
 * @session:	Session.
 * @val:	Value.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_mv_v_search_range(struct mvx_session *session,
				      int val);

/**
 * mvx_session_set_bitdepth_chroma() - Set bitdepth.
 * @session:	Session.
 * @val:	Value.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_bitdepth_chroma(struct mvx_session *session,
				    int val);

/**
 * mvx_session_set_bitdepth_luma() - Set bitdepth.
 * @session:	Session.
 * @val:	Value.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_bitdepth_luma(struct mvx_session *session,
				  int val);

/**
 * mvx_session_set_force_chroma_format() - Set chroma format.
 * @session:	Session.
 * @fmt:	chroma format.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_force_chroma_format(struct mvx_session *session,
					int fmt);

/**
 * mvx_session_set_rgb_to_yuv_mode() - Set RGB to YUV conversion mode.
 * @session:	Session.
 * @mode:	Mode.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_rgb_to_yuv_mode(struct mvx_session *session,
				    enum mvx_rgb_to_yuv_mode mode);

/**
 * mvx_session_set_band_limit() - Set maximum bandwidth limit.
 * @session:	Session.
 * @val:	Value.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_band_limit(struct mvx_session *session,
			       int val);

/**
 * mvx_session_set_cabac_init_idc() - Set CABAC initialization table.
 * @session:	Session.
 * @val:	Value.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_cabac_init_idc(struct mvx_session *session,
				   int val);

/**
 * mvx_session_set_i_frame_qp() - Set QP for I frames.
 * @session:	Session.
 * @format:	Format.
 * @qp:		Quantization parameter.
 *
 * This option could be set in runtime.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_i_frame_qp(struct mvx_session *session,
			       enum mvx_format format,
			       int qp);

/**
 * mvx_session_set_p_frame_qp() - Set QP for P frames.
 * @session:	Session.
 * @format:	Format.
 * @qp:		Quantization parameter.
 *
 * This option could be set in runtime.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_p_frame_qp(struct mvx_session *session,
			       enum mvx_format format,
			       int qp);

/**
 * mvx_session_set_b_frame_qp() - Set QP for B frames.
 * @session:	Session.
 * @format:	Format.
 * @qp:		Quantization parameter.
 *
 * This option could be set in runtime.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_b_frame_qp(struct mvx_session *session,
			       enum mvx_format format,
			       int qp);

/**
 * mvx_session_set_min_qp() - Set minimum value of QP range.
 * @session:	Session.
 * @format:	Format.
 * @qp:		Quantization parameter.
 *
 * This option could be set in runtime.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_min_qp(struct mvx_session *session,
			   enum mvx_format format,
			   int qp);

/**
 * mvx_session_set_max_qp() - Set maximum value of QP range.
 * @session:	Session.
 * @format:	Format.
 * @qp:		Quantization parameter.
 *
 * This option could be set in runtime.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_max_qp(struct mvx_session *session,
			   enum mvx_format format,
			   int qp);

/**
 * mvx_session_set_resync_interval() - Set resync interval for JPEG encoder.
 * @session:	Session.
 * @val:	Resync interval.
 *
 * Return: 0 in case of success, error code otherwise.
 */
int mvx_session_set_resync_interval(struct mvx_session *session,
				    int val);

/**
 * mvx_session_set_jpeg_quality() - Set JPEG quality.
 * @session:	Session.
 * @val:	Quality level (1-100).
 *
 * Return: 0 in case of success, error otherwise.
 */
int mvx_session_set_jpeg_quality(struct mvx_session *session,
				 int val);

/**
 * mvx_session_get_color_desc() - Get color description.
 * @session:	Pointer to session.
 * @color_desc:	Color description.
 *
 * Return: 0 on success, else error code.
 */
int mvx_session_get_color_desc(struct mvx_session *session,
			       struct mvx_fw_color_desc *color_desc);

/**
 * mvx_session_set_color_desc() - Set color description.
 * @session:	Pointer to session.
 * @color_desc:	Color description.
 *
 * Return: 0 on success, else error code.
 */
int mvx_session_set_color_desc(struct mvx_session *session,
			       struct mvx_fw_color_desc *color_desc);

/**
 * mvx_session_set_roi_regions() - Set ROI regions.
 * @vsession:	Pointer to v4l2 session.
 * @roi:	ROI regions.
 *
 * Return: 0 on success, else error code.
 */
int mvx_session_set_roi_regions(struct mvx_session *session,
			       struct mvx_roi_config *roi);

/**
 * mvx_session_set_qp_epr() - Set qp for epr config.
 * @vsession:	Pointer to v4l2 session.
 * @qp:	qp.
 *
 * Return: 0 on success, else error code.
 */

int mvx_session_set_qp_epr(struct mvx_session *session,
			       int *qp);

/**
 * mvx_session_set_sei_userdata() - Set SEI userdata.
 * @vsession:	Pointer to v4l2 session.
 * @userdata:	SEI userdata.
 *
 * Return: 0 on success, else error code.
 */

int mvx_session_set_sei_userdata(struct mvx_session *session,
			       struct mvx_sei_userdata *userdata);

/**
 * mvx_session_set_hrd_buffer_size() - Set hrd buffer size.
 * @vsession:	Pointer to v4l2 session.
 * @size:	hrd buffer size.
 *
 * Return: 0 on success, else error code.
 */

int mvx_session_set_hrd_buffer_size(struct mvx_session *session,
			      int size);

/**
 * mvx_session_set_dsl_frame() - Set DownScale dst frame.
 * @vsession:	Pointer to v4l2 session.
 * @dsl:	DownScale dst frame.
 *
 * Return: 0 on success, else error code.
 */

int mvx_session_set_dsl_frame(struct mvx_session *session,
			      struct mvx_dsl_frame *dsl);

/**
 * mvx_session_set_dsl_ratio() - Set DownScale ratio.
 * @vsession:	Pointer to v4l2 session.
 * @dsl:	DownScale ratio.
 *
 * Return: 0 on success, else error code.
 */

int mvx_session_set_dsl_ratio(struct mvx_session *session,
			      struct mvx_dsl_ratio *dsl);

/**
 * mvx_session_set_long_term_ref() - Set long term ref.
 * @vsession:	Pointer to v4l2 session.
 * @ltr:	long term ref.
 *
 * Return: 0 on success, else error code.
 */

int mvx_session_set_long_term_ref(struct mvx_session *session,
			      struct mvx_long_term_ref *ltr);

/**
 * mvx_session_set_dsl_mode() - Set DownScale mode.
 * @vsession:	Pointer to v4l2 session.
 * @mode:	DownScale mode, oly enable on high precision mode.
 *
 * Return: 0 on success, else error code.
 */

int mvx_session_set_dsl_mode(struct mvx_session *session,
			       int *mode);

/*force insert IDR frame.*/
int mvx_session_set_force_idr(struct mvx_session *session);

/*set timeout value for watchdog*/
int mvx_session_set_watchdog_timeout(struct mvx_session *session, int timeout);

/*enable/disable profiling for vpu*/
int mvx_session_set_profiling(struct mvx_session *session, int enable);

#endif /* _MVX_SESSION_H_ */
