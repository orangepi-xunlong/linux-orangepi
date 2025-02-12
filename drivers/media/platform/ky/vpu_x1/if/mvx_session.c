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

/****************************************************************************
 * Includes
 ****************************************************************************/

#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/version.h>

#include "mvx-v4l2-controls.h"
#include "mvx_bitops.h"
#include "mvx_firmware.h"
#include "mvx_firmware_cache.h"
#include "mvx_session.h"
#include "mvx_seq.h"
#include "mvx_dvfs.h"
#include "mvx_v4l2_session.h"
#include "mvx_mmu.h"

/****************************************************************************
 * Private variables
 ****************************************************************************/

int session_wait_pending_timeout = 10000;  //10s
int session_wait_flush_timeout = 2000;   //2s

int session_watchdog_timeout = 30000;   //30s
module_param(session_watchdog_timeout, int, 0660);

static int fw_watchdog_timeout;
module_param(fw_watchdog_timeout, int, 0660);

/****************************************************************************
 * Private functions
 ****************************************************************************/

static void watchdog_start(struct mvx_session *session,
			   unsigned int timeout_ms)
{
	int ret;

	if (session->error != 0)
		return;

	MVX_SESSION_DEBUG(session, "Watchdog start. timeout_ms=%u.",
			  timeout_ms);

	ret = mod_timer(&session->watchdog_timer,
			jiffies + msecs_to_jiffies(timeout_ms));
	if (ret != 0) {
		MVX_SESSION_WARN(session, "Failed to start watchdog. ret=%d",
				 ret);
		return;
	}

	kref_get(&session->isession.kref);
}

static void watchdog_stop(struct mvx_session *session)
{
	int ret;

	ret = del_timer_sync(&session->watchdog_timer);

	/* ret: 0=watchdog expired, 1=watchdog still running */
	MVX_SESSION_DEBUG(session, "Watchdog stop. ret=%d", ret);
	session->watchdog_count = 0;

	/* Decrement the kref if the watchdog was still running. */
	if (ret != 0)
		kref_put(&session->isession.kref, session->isession.release);
}

static void watchdog_update(struct mvx_session *session,
			    unsigned int timeout_ms)
{
	int ret;

#ifndef MODULE
	ret = mod_timer_pending(&session->watchdog_timer,
				jiffies + msecs_to_jiffies(timeout_ms));
#else
	if (timer_pending(&session->watchdog_timer))
		ret = mod_timer(&session->watchdog_timer,
				jiffies + msecs_to_jiffies(timeout_ms));
#endif

	/* ret: 0=no restart, 1=restarted */
	MVX_SESSION_DEBUG(session, "Watchdog update. ret=%d, timeout_ms=%u.",
			  ret, timeout_ms);
}

static bool is_fw_loaded(struct mvx_session *session)
{
	return (IS_ERR_OR_NULL(session->fw_bin) == false);
}

static void print_debug(struct mvx_session *session)
{
	MVX_SESSION_INFO(session, "Print debug.");

	if (session->csession != NULL)
		session->client_ops->print_debug(session->csession);

	if (is_fw_loaded(session))
		session->fw.ops.print_debug(&session->fw);
}

static void send_event_error(struct mvx_session *session,
			     long error)
{
	session->error = error;
	wake_up(&session->waitq);
	MVX_SESSION_WARN(session, "send event error. error=%ld", error);
	session->event(session, MVX_SESSION_EVENT_ERROR,
		       (void *)session->error);
}

static void session_unregister(struct mvx_session *session)
{
	if (!IS_ERR_OR_NULL(session->csession)) {
		mvx_dvfs_unregister_session(session);
		session->client_ops->unregister_session(session->csession);
		session->csession = NULL;
	}
}

static void release_fw_bin(struct mvx_session *session)
{
	if (is_fw_loaded(session) != false) {
		MVX_SESSION_INFO(session, "Release firmware binary.");

        if (session->switched_in == true) {
            if (IS_ERR_OR_NULL(session->csession)) {
                MVX_SESSION_WARN(session, "release_fw_bin. csession is null. mvx_session:%p", session);
            } else {
                session->client_ops->wait_session_idle(session->csession);
            }
        }

		mvx_fw_destruct(&session->fw);
		mvx_fw_cache_put(session->cache, session->fw_bin);
		session->fw_bin = NULL;
	}

	watchdog_stop(session);
	session_unregister(session);
}

static struct mvx_session *kref_to_session(struct kref *kref)
{
	return container_of(kref, struct mvx_session, isession.kref);
}

static void session_destructor(struct kref *kref)
{
	struct mvx_session *session = kref_to_session(kref);

	session->destructor(session);
}

static const char *state_to_string(enum mvx_fw_state state)
{
	switch (state) {
	case MVX_FW_STATE_STOPPED:
		return "Stopped";
	case MVX_FW_STATE_RUNNING:
		return "Running";
	default:
		return "Unknown";
	}
}

static enum mvx_direction get_bitstream_port(struct mvx_session *session)
{
	if (mvx_is_bitstream(session->port[MVX_DIR_INPUT].format) &&
	    mvx_is_frame(session->port[MVX_DIR_OUTPUT].format))
		return MVX_DIR_INPUT;
	else if (mvx_is_frame(session->port[MVX_DIR_INPUT].format) &&
		 mvx_is_bitstream(session->port[MVX_DIR_OUTPUT].format))
		return MVX_DIR_OUTPUT;

	return MVX_DIR_MAX;
}

static bool is_stream_on(struct mvx_session *session)
{
	return session->port[MVX_DIR_INPUT].stream_on &&
	       session->port[MVX_DIR_OUTPUT].stream_on;
}

/**
 * wait_pending() - Wait for procedure to finish.
 *
 * Wait for the number of pending firmware messages to reach 0, or for an error
 * to happen.
 *
 * Return: 0 on success, else error code.
 */
static int wait_pending(struct mvx_session *session)
{
    int ret = 0;
    int count = 0;
    struct timespec64 curtime;
    uint64_t timestart;
    uint64_t timeend;

    while (is_fw_loaded(session) != false &&
            session->fw.msg_pending > 0 &&
            session->error == 0) {
        mutex_unlock(session->isession.mutex);

        count++;
        ktime_get_boottime_ts64(&curtime);
        timestart = curtime.tv_sec*1000*1000l + curtime.tv_nsec/1000l;

        ret = wait_event_timeout(
                session->waitq,
                is_fw_loaded(session) == false ||
                session->fw.msg_pending == 0 ||
                session->error != 0,
                msecs_to_jiffies(session->watchdog_timeout*2));

        ktime_get_boottime_ts64(&curtime);
        timeend = curtime.tv_sec*1000*1000l + curtime.tv_nsec/1000l;
        if (timeend - timestart >= 10000000l) {
            MVX_SESSION_WARN(session, "wait_pending cost time is greater than 10 secs. cost_time=%llu us, ret=%d, error=%d, msg_pending=%d, in_fmt=%d, out_fmt=%d", timeend - timestart, ret, session->error, session->fw.msg_pending, session->port[MVX_DIR_INPUT].format, session->port[MVX_DIR_OUTPUT].format);
        }

        if (ret < 0)
            goto lock_mutex;

        if (ret == 0) {
            send_event_error(session, -ETIME);
            ret = -ETIME;
            goto lock_mutex;
        }

        mutex_lock(session->isession.mutex);
    }

    return session->error;

lock_mutex:
    mutex_lock(session->isession.mutex);

    if (ret < 0) {
        MVX_SESSION_WARN(session,
                "Wait pending returned error. ret=%d, error=%d, msg_pending=%d.",
                ret, session->error, session->fw.msg_pending);

        print_debug(session);
    }
    return ret;
}

static int wait_flush_done(struct mvx_session *session, enum mvx_direction dir)
{
    int ret = 0;
    struct mvx_session_port *port = &session->port[dir];

    while (is_fw_loaded(session) != false &&
            port->buffer_count > 0 &&
            session->error == 0) {
        mutex_unlock(session->isession.mutex);

        ret = wait_event_timeout(
                session->waitq,
                is_fw_loaded(session) == false ||
                port->buffer_count == 0 ||
                session->error != 0,
                msecs_to_jiffies(session_wait_flush_timeout));

        if (ret < 0)
            goto lock_mutex;

        if (ret == 0) {
            send_event_error(session, -ETIME);
            ret = -ETIME;
            goto lock_mutex;
        }

        mutex_lock(session->isession.mutex);
    }

    return session->error;

lock_mutex:
    mutex_lock(session->isession.mutex);

    if (ret < 0) {
        MVX_SESSION_WARN(session,
                "wait_flush_done returned error. ret=%d, error=%d, msg_pending=%d, buffer_count=%d",
                ret, session->error, session->fw.msg_pending, port->buffer_count);

        print_debug(session);
    }
    return ret;
}

static int wait_switch_out(struct mvx_session *session)
{
    int ret = 0;

    while (is_fw_loaded(session) != false &&
            session->switched_in == true &&
            session->error == 0) {
        mutex_unlock(session->isession.mutex);

        ret = wait_event_timeout(
                session->waitq,
                is_fw_loaded(session) == false ||
                session->switched_in == false ||
                session->error != 0,
                msecs_to_jiffies(session_wait_pending_timeout));

        if (ret < 0)
            goto lock_mutex;

        if (ret == 0) {
            send_event_error(session, -ETIME);
            ret = -ETIME;
            goto lock_mutex;
        }

        mutex_lock(session->isession.mutex);
    }

    return session->error;

lock_mutex:
    mutex_lock(session->isession.mutex);

    if (ret < 0) {
        MVX_SESSION_WARN(session,
                "wait_switch_out returned error. ret=%d, error=%d, msg_pending=%d, switch_in=%d",
                ret, session->error, session->fw.msg_pending, session->switched_in);

        print_debug(session);
    }
    return ret;
}

static int send_irq(struct mvx_session *session)
{
	if (IS_ERR_OR_NULL(session->csession))
		return -EINVAL;

	return session->client_ops->send_irq(session->csession);
}

/**
 * switch_in() - Request the client device to switch in the session.
 *
 * Return: 0 on success, else error code.
 */
static int switch_in(struct mvx_session *session)
{
	int ret;

	session->idle_count = 0;

	if (session->switched_in != false)
		return 0;

	if (IS_ERR_OR_NULL(session->csession))
		return -EINVAL;

	MVX_SESSION_INFO(session, "Switch in.");

	ret = session->client_ops->switch_in(session->csession);
	if (ret != 0) {
		MVX_SESSION_WARN(session, "Failed to switch in session.");
		send_event_error(session, ret);
		return ret;
	}

	session->switched_in = true;

	return 0;
}

/**
 * fw_send_msg() - Send firmware message and signal IRQ.
 *
 * Return: 0 on success, else error code.
 */
static int fw_send_msg(struct mvx_session *session,
		       struct mvx_fw_msg *msg)
{
	int ret;

	if (session->error != 0)
		return session->error;

	ret = session->fw.ops.put_message(&session->fw, msg);
	if (ret != 0) {
		MVX_SESSION_WARN(session,
				 "Failed to queue firmware message.");
		goto send_error;
	}

	ret = send_irq(session);
	if (ret != 0) {
		MVX_SESSION_WARN(session, "Failed to send irq.");
		goto send_error;
	}

    return switch_in(session);

send_error:
	send_event_error(session, ret);
	return ret;
}

static int fw_send_msg_simple(struct mvx_session *session,
			      enum mvx_fw_code code,
			      const char *str)
{
	struct mvx_fw_msg msg = { .code = code };

	MVX_SESSION_INFO(session, "Firmware req: %s.", str);

	return fw_send_msg(session, &msg);
}

static int fw_flush(struct mvx_session *session,
		    enum mvx_direction dir)
{
	struct mvx_fw_msg msg = { .code = MVX_FW_CODE_FLUSH, .flush.dir = dir };
	int ret;

	MVX_SESSION_INFO(session, "Firmware req: Flush. dir=%d.", dir);

	ret = fw_send_msg(session, &msg);
	if (ret != 0)
		return ret;

	session->port[dir].is_flushing = true;

	return 0;
}

static int fw_state_change(struct mvx_session *session,
			   enum mvx_fw_state state)
{
	struct mvx_fw_msg msg = {
		.code  = MVX_FW_CODE_STATE_CHANGE,
		.state = state
	};
	int ret = 0;

	if (state != session->fw_state) {
		MVX_SESSION_INFO(session,
				 "Firmware req: State change. current=%d, new=%d.",
				 session->fw_state, state);
		ret = fw_send_msg(session, &msg);
	}

	return ret;
}

static int fw_job(struct mvx_session *session,
		  unsigned int frames)
{
	struct mvx_fw_msg msg = {
		.code       = MVX_FW_CODE_JOB,
		.job.cores  = session->isession.ncores,
		.job.frames = frames
	};

	MVX_SESSION_INFO(session, "Firmware req: Job. frames=%u.", frames);

	return fw_send_msg(session, &msg);
}

static int fw_switch_out(struct mvx_session *session)
{
	unsigned int idle_count = session->idle_count;
	int ret;

	ret = fw_send_msg_simple(session, MVX_FW_CODE_SWITCH_OUT,
				 "Switch out");

	/*
	 * Restore idle count. Switch out is the only message where we do not
	 * want to reset the idle counter.
	 */
	session->idle_count = idle_count;

	return ret;
}

static int fw_ping(struct mvx_session *session)
{
	return fw_send_msg_simple(session, MVX_FW_CODE_PING, "Ping");
}

static int fw_dump(struct mvx_session *session)
{
	return fw_send_msg_simple(session, MVX_FW_CODE_DUMP, "Dump");
}

#ifdef MVX_FW_DEBUG_ENABLE
static int fw_set_debug(struct mvx_session *session, uint32_t debug_level)
{
	struct mvx_fw_msg msg = {
		.code       = MVX_FW_CODE_DEBUG,
		.arg = debug_level
	};

	MVX_SESSION_INFO(session, "Firmware req: Set debug. debug_level=%d.", debug_level);

	return fw_send_msg(session, &msg);
}
#endif

static int fw_set_option(struct mvx_session *session,
			 struct mvx_fw_set_option *option)
{
	struct mvx_fw_msg msg = {
		.code       = MVX_FW_CODE_SET_OPTION,
		.set_option = *option
	};

	MVX_SESSION_INFO(session, "Firmware req: Set option. code=%d.",
			 option->code);

	return fw_send_msg(session, &msg);
}

static bool is_encoder(struct mvx_session *session)
{
	return get_bitstream_port(session) == MVX_DIR_OUTPUT;
}

static int fw_eos(struct mvx_session *session)
{
	struct mvx_fw_msg msg = {
		.code         = MVX_FW_CODE_EOS,
		.eos_is_frame = is_encoder(session) ? true : false
	};
	int ret;

	MVX_SESSION_INFO(session, "Firmware req: Buffer EOS.");

	ret = fw_send_msg(session, &msg);
	if (ret != 0)
		return ret;

	session->port[MVX_DIR_INPUT].flushed = false;

	return 0;
}

static int fw_set_qp(struct mvx_session *session,
		     int code,
		     int qp)
{
	struct mvx_fw_set_option option;
	int ret;

	if (qp < 0)
		return -EINVAL;

	if (qp == 0)
		return 0;

	option.code = code;
	option.qp = qp;
	ret = fw_set_option(session, &option);
	if (ret != 0) {
		MVX_SESSION_WARN(session,
				 "Failed to set QP. code=%d, ret=%d.",
				 code, ret);
		return ret;
	}

	return 0;
}

static int fw_set_roi_regions(struct mvx_session *session,
		     int code,
		      struct mvx_roi_config *roi)
{
	struct mvx_fw_set_option option;
	int ret;

	if (roi->num_roi < 0)
		return -EINVAL;

	if (roi->num_roi == 0)
		return 0;

	option.code = code;
	option.roi_config = *roi;
	ret = fw_set_option(session, &option);
	if (ret != 0) {
		MVX_SESSION_WARN(session,
				 "Failed to set ROI. code=%d, ret=%d.",
				 code, ret);
		return ret;
	}

	return 0;
}


static int fw_common_setup(struct mvx_session *session)
{
	int ret = 0;
	struct mvx_fw_set_option option;
	enum mvx_direction dir = get_bitstream_port(session);

	if ((session->port[dir].format == MVX_FORMAT_VP8
		|| session->port[dir].format == MVX_FORMAT_VP9)
		&& session->nalu_format != MVX_NALU_FORMAT_ONE_NALU_PER_BUFFER) {
		session->nalu_format = MVX_NALU_FORMAT_UNDEFINED;
	}

	if (session->nalu_format != MVX_NALU_FORMAT_UNDEFINED) {
		option.code = MVX_FW_SET_NALU_FORMAT;
		option.nalu_format = session->nalu_format;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set NALU format.");
			return ret;
		}
	}

	if (session->stream_escaping != MVX_TRI_UNSET) {
		option.code = MVX_FW_SET_STREAM_ESCAPING;
		option.stream_escaping = session->stream_escaping;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set stream escaping.");
			return ret;
		}
	}

    if (session->enable_profiling) {
        MVX_SESSION_WARN(session, "[Debug]set MVX_FW_SET_INDEX_PROFILING to read profiling data.");
        session->bus_read_bytes_total = 0;
        session->bus_write_bytes_total = 0;
        option.code = MVX_FW_SET_INDEX_PROFILING;
        option.index_profiling = 1;
        ret = fw_set_option(session, &option);
        if (ret != 0) {
            MVX_SESSION_WARN(session,
                    "Failed to set set index profiling");
            return ret;
        }
    }

	return ret;
}

/* JPEG standard, Annex K */
static const uint8_t qtbl_chroma_ref[MVX_FW_QUANT_LEN] = {
	17, 18, 24, 47, 99, 99, 99, 99,
	18, 21, 26, 66, 99, 99, 99, 99,
	24, 26, 56, 99, 99, 99, 99, 99,
	47, 66, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99
};

static const uint8_t qtbl_luma_ref[MVX_FW_QUANT_LEN] = {
	16, 11, 10, 16, 24,  40,  51,  61,
	12, 12, 14, 19, 26,  58,  60,  55,
	14, 13, 16, 24, 40,  57,  69,  56,
	14, 17, 22, 29, 51,  87,  80,  62,
	18, 22, 37, 56, 68,  109, 103, 77,
	24, 35, 55, 64, 81,  104, 113, 92,
	49, 64, 78, 87, 103, 121, 120, 101,
	72, 92, 95, 98, 112, 100, 103, 99
};

void generate_quant_tbl(int quality,
			const uint8_t qtbl_ref[MVX_FW_QUANT_LEN],
			uint8_t qtbl[MVX_FW_QUANT_LEN])
{
	int i;
	int q;

	q = (quality < 50) ? (5000 / quality) : (200 - 2 * quality);

	for (i = 0; i < MVX_FW_QUANT_LEN; ++i) {
		qtbl[i] = ((qtbl_ref[i] * q) + 50) / 100;
		qtbl[i] = min_t(int, qtbl[i], 255);
		qtbl[i] = max_t(int, qtbl[i], 1);
	}
}

static int fw_encoder_setup(struct mvx_session *session)
{
	int ret;
	enum mvx_format codec;
	struct mvx_fw_set_option option;
	enum mvx_direction dir;

	dir = get_bitstream_port(session);
	codec = session->port[dir].format;

	if (session->profile[codec] != MVX_PROFILE_NONE) {
		option.code = MVX_FW_SET_PROFILE_LEVEL;
		option.profile_level.profile = session->profile[codec];
		option.profile_level.level = session->level[codec];
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set profile/level.");
			return ret;
		}
	}

	if (codec != MVX_FORMAT_JPEG) {
		option.code = MVX_FW_SET_FRAME_RATE;
		option.frame_rate = session->frame_rate;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to put frame rate.");
			return ret;
		}

        if (session->rc_type) {
            option.code = MVX_FW_SET_RATE_CONTROL;
            option.rate_control.target_bitrate =
                    session->rc_type ? session->target_bitrate:0;
            option.rate_control.rate_control_mode = session->rc_type;
            if (session->rc_type == MVX_OPT_RATE_CONTROL_MODE_C_VARIABLE) {
                option.rate_control.maximum_bitrate = session->maximum_bitrate;
            }
            ret = fw_set_option(session, &option);
            if (ret != 0) {
                MVX_SESSION_WARN(session,
                     "Failed to put target bitrate.");
                return ret;
            }
        }

		if (session->rc_enabled != false) {
			if (session->qp[codec].min < session->qp[codec].max) {
				option.code = MVX_FW_SET_QP_RANGE;
				option.qp_range.min = session->qp[codec].min;
				option.qp_range.max = session->qp[codec].max;
				ret = fw_set_option(session, &option);
				if (ret != 0) {
					MVX_SESSION_WARN(session,
							 "Failed to set qp range.");
					return ret;
				}
			}
		} else {
			ret = fw_set_qp(session, MVX_FW_SET_QP_I,
					session->qp[codec].i_frame);
			if (ret != 0)
				return ret;

			ret = fw_set_qp(session, MVX_FW_SET_QP_P,
					session->qp[codec].p_frame);
			if (ret != 0)
				return ret;

			ret = fw_set_qp(session, MVX_FW_SET_QP_B,
					session->qp[codec].b_frame);
			if (ret != 0)
				return ret;
		}

		if (session->p_frames >= 0) {
			option.code = MVX_FW_SET_P_FRAMES;
			option.pb_frames = session->p_frames;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set P frames.");
				return ret;
			}
		}

		if (session->b_frames != 0) {
			option.code = MVX_FW_SET_B_FRAMES;
			option.pb_frames = session->b_frames;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set B frames.");
				return ret;
			}
		}

		if (session->gop_type != MVX_GOP_TYPE_NONE) {
			option.code = MVX_FW_SET_GOP_TYPE;
			option.gop_type = session->gop_type;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set GOP type.");
				return ret;
			}
		}

		if (session->cyclic_intra_refresh_mb != 0) {
			option.code = MVX_FW_SET_INTRA_MB_REFRESH;
			option.intra_mb_refresh =
				session->cyclic_intra_refresh_mb;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set cyclic intra refresh Mb.");
				return ret;
			}
		}

		if (session->constr_ipred != MVX_TRI_UNSET &&
		    (codec == MVX_FORMAT_H264 || codec == MVX_FORMAT_HEVC)) {
			option.code = MVX_FW_SET_CONSTR_IPRED;
			option.constr_ipred = session->constr_ipred;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set constr ipred.");
				return ret;
			}
		}
	}

	if (codec == MVX_FORMAT_HEVC) {
		if (session->entropy_sync != MVX_TRI_UNSET) {
			option.code = MVX_FW_SET_ENTROPY_SYNC;
			option.entropy_sync = session->entropy_sync;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set entropy sync.");
				return ret;
			}
		}

		if (session->temporal_mvp != MVX_TRI_UNSET) {
			option.code = MVX_FW_SET_TEMPORAL_MVP;
			option.temporal_mvp = session->temporal_mvp;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set temporal mvp.");
				return ret;
			}
		}

		option.code = MVX_FW_SET_MIN_LUMA_CB_SIZE;
		option.min_luma_cb_size = session->min_luma_cb_size;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set min luma cb size.");
			return ret;
		}
	}

	if ((codec == MVX_FORMAT_HEVC ||
	     codec == MVX_FORMAT_VP9) &&
	    (session->tile_rows != 0 ||
	     session->tile_cols != 0)) {
		option.code = MVX_FW_SET_TILES;
		option.tile.rows = session->tile_rows;
		option.tile.cols = session->tile_cols;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set tile dims.");
			return ret;
		}
	}

	if (session->entropy_mode != MVX_ENTROPY_MODE_NONE &&
	    codec == MVX_FORMAT_H264) {
		option.code = MVX_FW_SET_ENTROPY_MODE;
		option.entropy_mode = session->entropy_mode;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set entropy mode.");
			return ret;
		}
	}

	if (codec == MVX_FORMAT_H264 ||
	    codec == MVX_FORMAT_HEVC) {
		option.code = MVX_FW_SET_SLICE_SPACING_MB;
		if (session->multi_slice_mode ==
		    MVX_MULTI_SLICE_MODE_SINGLE)
			option.slice_spacing_mb = 0;
		else
			option.slice_spacing_mb =
				session->multi_slice_max_mb;

		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set slice spacing.");
			return ret;
		}

		option.code = MVX_FW_SET_CABAC_INIT_IDC;
		option.cabac_init_idc = session->cabac_init_idc;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set CABAC init IDC.");
			return ret;
		}
        if (session->crop_left != 0) {
            option.code = MVX_FW_SET_CROP_LEFT;
            option.crop_left = session->crop_left;
            ret = fw_set_option(session, &option);
            if (ret != 0) {
                MVX_SESSION_WARN(session,
                     "Failed to set crop left");
                return ret;
            }
        }
        if (session->crop_right != 0) {
            option.code = MVX_FW_SET_CROP_RIGHT;
            option.crop_right = session->crop_right;
            ret = fw_set_option(session, &option);
            if (ret != 0) {
                MVX_SESSION_WARN(session,
                     "Failed to set crop right");
                return ret;
            }
        }
        if (session->crop_top != 0) {
            option.code = MVX_FW_SET_CROP_TOP;
            option.crop_top = session->crop_top;
            ret = fw_set_option(session, &option);
            if (ret != 0) {
                MVX_SESSION_WARN(session,
                     "Failed to set crop top");
                return ret;
            }
        }
        if (session->crop_bottom != 0) {
            option.code = MVX_FW_SET_CROP_BOTTOM;
            option.crop_bottom = session->crop_bottom;
            ret = fw_set_option(session, &option);
            if (ret != 0) {
                MVX_SESSION_WARN(session,
                     "Failed to set crop bottom");
                return ret;
            }
        }
        if (session->nHRDBufsize != 0) {
            option.code = MVX_FW_SET_HRD_BUF_SIZE;
            option.nHRDBufsize = session->nHRDBufsize;
            ret = fw_set_option(session, &option);
            if (ret != 0) {
                MVX_SESSION_WARN(session,
                     "Failed to set HRD Buffer Size");
                return ret;
            }
        }
        if (session->color_desc.range != 0 || session->color_desc.matrix != 0 ||
            session->color_desc.primaries != 0 || session->color_desc.transfer != 0 ||
            session->color_desc.sar_height != 0 || session->color_desc.sar_width != 0 ||
            session->color_desc.aspect_ratio_idc != 0) {
            struct mvx_fw_set_option option;

            option.code = MVX_FW_SET_COLOUR_DESC;
            option.colour_desc = session->color_desc;
            ret = fw_set_option(session, &option);
            if (ret != 0) {
                MVX_SESSION_WARN(session,
                     "Failed to set vui colour description");
                return ret;
            }
        }

        if (session->sei_userdata.flags) {
            option.code = MVX_FW_SET_SEI_USERDATA;
            option.userdata = session->sei_userdata;
            ret = fw_set_option(session, &option);
            if (ret != 0) {
                MVX_SESSION_WARN(session,
                     "Failed to set sei userdata");
                return ret;
            }
        }

        if (session->mvx_ltr.mode != 0 || session->mvx_ltr.period != 0){
            option.code = MVX_FW_SET_LONG_TERM_REF;
            option.ltr.mode = session->mvx_ltr.mode;
            option.ltr.period = session->mvx_ltr.period;
            ret = fw_set_option(session, &option);
            if (ret != 0) {
                MVX_SESSION_WARN(session,
                     "Failed to set ltr mode/period");
                return ret;
            }
        }
	}

	if (codec == MVX_FORMAT_VP9) {
		MVX_SESSION_INFO(session, "VP9 option!");
		option.code = MVX_FW_SET_VP9_PROB_UPDATE;
		option.vp9_prob_update = session->vp9_prob_update;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set VP9 prob update mode.");
			return ret;
		}
	}

	if (session->mv_h_search_range != 0 &&
	    session->mv_v_search_range != 0) {
		option.code = MVX_FW_SET_MV_SEARCH_RANGE;
		option.mv.x = session->mv_h_search_range;
		option.mv.y = session->mv_v_search_range;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set motion vector search range.");
			return ret;
		}
	}

	if (session->bitdepth_chroma != 0 &&
	    session->bitdepth_luma != 0) {
		option.code = MVX_FW_SET_BITDEPTH;
		option.bitdepth.chroma = session->bitdepth_chroma;
		option.bitdepth.luma = session->bitdepth_luma;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set bitdepth.");
			return ret;
		}
	}

	if (session->force_chroma_format != 0) {
		option.code = MVX_FW_SET_CHROMA_FORMAT;
		option.chroma_format = session->force_chroma_format;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set chroma format.");
			return ret;
		}
	}

	if (mvx_is_rgb(session->port[MVX_DIR_INPUT].format) != false) {
		option.code = MVX_FW_SET_RGB_TO_YUV_MODE;
		if (session->color_desc.primaries == MVX_FW_PRIMARIES_BT709)
		{
		    session->rgb_to_yuv = (session->color_desc.range == MVX_FW_RANGE_FULL)?MVX_RGB_TO_YUV_MODE_BT709_FULL:MVX_RGB_TO_YUV_MODE_BT709_STUDIO;
		}
		else if ((session->color_desc.primaries == MVX_FW_PRIMARIES_BT601_625) ||
		    (session->color_desc.primaries == MVX_FW_PRIMARIES_BT601_525))
		{
		    session->rgb_to_yuv = (session->color_desc.range == MVX_FW_RANGE_FULL)?MVX_RGB_TO_YUV_MODE_BT601_FULL:MVX_RGB_TO_YUV_MODE_BT601_STUDIO;
		}
		option.rgb_to_yuv_mode = session->rgb_to_yuv;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set RGB to YUV mode.");
			return ret;
		}
	}

	if (session->band_limit != 0) {
		option.code = MVX_FW_SET_BAND_LIMIT;
		option.band_limit = session->band_limit;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set bandwidth limit.");
			return ret;
		}
	}

	if (codec == MVX_FORMAT_JPEG) {
		if (session->resync_interval >= 0) {
			option.code = MVX_FW_SET_RESYNC_INTERVAL;
			option.resync_interval = session->resync_interval;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set resync interval.");
				return ret;
			}
		}

		if (session->jpeg_quality != 0) {
			uint8_t qtbl_chroma[MVX_FW_QUANT_LEN];
			uint8_t qtbl_luma[MVX_FW_QUANT_LEN];

			generate_quant_tbl(session->jpeg_quality,
					   qtbl_chroma_ref, qtbl_chroma);
			generate_quant_tbl(session->jpeg_quality,
					   qtbl_luma_ref, qtbl_luma);
			option.code = MVX_FW_SET_QUANT_TABLE;
			option.quant_tbl.chroma = qtbl_chroma;
			option.quant_tbl.luma = qtbl_luma;
			ret = fw_set_option(session, &option);
			if (ret != 0) {
				MVX_SESSION_WARN(session,
						 "Failed to set quantization table.");
				return ret;
			}
		}
	}

	ret = fw_common_setup(session);

	return ret;
}

static int fw_decoder_setup(struct mvx_session *session)
{
	int ret;
	struct mvx_fw_set_option option;

	enum mvx_format codec;
	enum mvx_direction dir;

	dir = get_bitstream_port(session);
	codec = session->port[dir].format;

	if (codec == MVX_FORMAT_VC1 &&
	    session->profile[codec] != MVX_PROFILE_NONE) {
		option.code = MVX_FW_SET_PROFILE_LEVEL;
		option.profile_level.profile = session->profile[codec];
		option.profile_level.level = session->level[codec];
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set profile/level.");
			return ret;
		}
	}

	if (session->ignore_stream_headers != MVX_TRI_UNSET) {
		option.code = MVX_FW_SET_IGNORE_STREAM_HEADERS;
		option.ignore_stream_headers = session->ignore_stream_headers;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set ignore stream headers.");
			return ret;
		}
	}

	if (session->frame_reordering != MVX_TRI_UNSET) {
		option.code = MVX_FW_SET_FRAME_REORDERING;
		option.frame_reordering = session->frame_reordering;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set frame reordering.");
			return ret;
		}
	}

	if (session->intbuf_size != 0) {
		option.code = MVX_FW_SET_INTBUF_SIZE;
		option.intbuf_size = session->intbuf_size;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to set internal buffer size.");
			return ret;
		}
	}

    if (session->dsl_frame.width != 0 && session->dsl_frame.height != 0) {
        option.code = MVX_FW_SET_DSL_FRAME;
        option.dsl_frame.width = session->dsl_frame.width;
        option.dsl_frame.height = session->dsl_frame.height;
        ret = fw_set_option(session, &option);
        if (ret != 0) {
            MVX_SESSION_WARN(session,
                    "Failed to set DSL frame width/height.");
            return ret;
        }
    }

    if (session->dsl_pos_mode >= 0 && session->dsl_pos_mode <= 2) {
        option.code = MVX_FW_SET_DSL_MODE;
        option.dsl_pos_mode = session->dsl_pos_mode;
         ret = fw_set_option(session, &option);
        if (ret != 0) {
            MVX_SESSION_WARN(session,
                    "Failed to set DSL mode.");
            return ret;
        }
    }

	ret = fw_common_setup(session);

	return ret;
}

static int fw_initial_setup(struct mvx_session *session)
{
	int ret;
	enum mvx_direction dir;
	enum mvx_format codec;
	struct mvx_fw_set_option option;

	MVX_SESSION_INFO(session, "Firmware initial setup.");

#ifdef MVX_FW_DEBUG_ENABLE
	fw_set_debug(session, 5);
#endif

	option.code = MVX_FW_SET_WATCHDOG_TIMEOUT;
	option.watchdog_timeout = fw_watchdog_timeout;
	ret = fw_set_option(session, &option);
	if (ret != 0)
		return ret;

	dir = get_bitstream_port(session);
	codec = session->port[dir].format;

	ret = fw_job(session, 1);
	if (ret != 0)
		return ret;

	if (is_encoder(session))
		ret = fw_encoder_setup(session);
	else
		ret = fw_decoder_setup(session);

	if (ret != 0) {
		MVX_SESSION_WARN(session,
				 "Failed to perform initial setup.\n");
		return ret;
	}

	ret = fw_state_change(session, MVX_FW_STATE_RUNNING);
	if (ret != 0) {
		MVX_SESSION_WARN(session, "Failed to queue state change.");
		return ret;
	}

	ret = fw_ping(session);
	if (ret != 0) {
		MVX_SESSION_WARN(session, "Failed to put ping message.");
		send_event_error(session, ret);
		return ret;
	}

	return ret;
}

static void ddr_qos_request_update(struct mvx_session *session)
{
    struct estimate_ddr_input ddr_input;
    //struct estimate_ddr_output ddr_output;

    ddr_input.isEnc = is_encoder(session);
    ddr_input.fps = session->frame_rate >> 16;

    if (ddr_input.isEnc) {
        ddr_input.isAFBC = mvx_is_afbc(session->port[MVX_DIR_INPUT].format);
        ddr_input.width = session->port[MVX_DIR_INPUT].width;
        ddr_input.height = session->port[MVX_DIR_INPUT].height;
    } else {
        ddr_input.isAFBC = mvx_is_afbc(session->port[MVX_DIR_OUTPUT].format);
        ddr_input.width = session->port[MVX_DIR_OUTPUT].width;
        ddr_input.height = session->port[MVX_DIR_OUTPUT].height;
    }

    //mvx_dvfs_estimate_ddr_bandwidth(&ddr_input, &ddr_output);

    //session->estimated_ddr_read_throughput = (uint32_t)ddr_output.estimated_read;
    //session->estimated_ddr_write_throughput = (uint32_t)ddr_output.estimated_write;

    //mvx_dvfs_session_update_ddr_qos(session, session->estimated_ddr_read_throughput, session->estimated_ddr_write_throughput);
}

/**
 * map_buffer() - Memory map buffer to MVE address space.
 *
 * Return 0 on success, else error code.
 */
static int map_buffer(struct mvx_session *session,
		      enum mvx_direction dir,
		      struct mvx_buffer *buf)
{
	mvx_mmu_va begin;
	mvx_mmu_va end;
	enum mvx_fw_region region;
	int ret;

	if (mvx_is_bitstream(session->port[dir].format))
		region = MVX_FW_REGION_PROTECTED;
	else if (mvx_is_frame(session->port[dir].format))
		region = MVX_FW_REGION_FRAMEBUF;
	else
		return -EINVAL;

	ret = session->fw.ops.get_region(region, &begin, &end);
	if (ret != 0)
		return ret;

	ret = mvx_buffer_map(buf, begin, end);
	if (ret != 0)
		return ret;

	return 0;
}

static int queue_roi_regions(struct mvx_session *session,
			struct mvx_roi_config *roi_cfg)
{
    int ret = 0;
    if ( roi_cfg->qp_present ) {
        ret = fw_set_qp(session, MVX_FW_SET_QP_REGION,
					roi_cfg->qp);
    }
    if ( roi_cfg->roi_present ) {
        ret = fw_set_roi_regions(session, MVX_FW_SET_ROI_REGIONS,
					roi_cfg);
    }
    return ret;
}

static int queue_qp_epr(struct mvx_session *session,
			int *qp)
{
    int ret = 0;
    ret = fw_set_qp(session, MVX_FW_SET_QP_REGION,
					*qp);

    return ret;
}

/**
 * queue_buffer() - Put buffer to firmware queue.
 *
 * Return: 0 on success, else error code.
 */
static int queue_buffer(struct mvx_session *session,
			enum mvx_direction dir,
			struct mvx_buffer *buf)
{
	struct mvx_session_port *port = &session->port[dir];
	struct mvx_fw_msg msg;
       int i;

	/*
	 * Vb2 cannot allocate buffers with bidirectional mapping, therefore
	 * proper direction should be set.
	 */
	enum dma_data_direction dma_dir =
		(dir == MVX_DIR_OUTPUT) ? DMA_FROM_DEVICE : DMA_TO_DEVICE;

	int ret;
       bool mapped = mvx_buffer_is_mapped(buf);

	if (dir == MVX_DIR_OUTPUT) {
		port->scaling_shift = (buf->flags & MVX_BUFFER_FRAME_FLAG_SCALING_MASK) >> 14;
	}
	if (mvx_buffer_is_mapped(buf) == false) {
		ret = map_buffer(session, dir, buf);
		if (ret != 0)
			return ret;
	}
    buf->flags &= ~MVX_BUFFER_FRAME_NEED_REALLOC;

    if (dir == MVX_DIR_OUTPUT && port->isreallocting == true) {
        buf->flags |= MVX_BUFFER_FRAME_NEED_REALLOC;
        return -EAGAIN;
    }

    if (dir == MVX_DIR_OUTPUT && port->buffer_allocated < port->buffer_min) {
        buf->flags |= MVX_BUFFER_FRAME_NEED_REALLOC;
        return -EAGAIN;
    }

    if (dir == MVX_DIR_INPUT && (buf->flags & MVX_BUFFER_EOS) != 0) {
        session->eos_queued = true;
    }

    if (dir == MVX_DIR_OUTPUT) {
        session->keep_freq_high = false;
    }

	/*
	 * Update frame dimensions. They might have changed due to a resolution
	 * change.
	 */
	if (mvx_is_afbc(port->format) != false) {
		port->afbc_width = DIV_ROUND_UP(port->width, 16 << (!!(buf->flags & MVX_BUFFER_AFBC_32X8_SUPERBLOCK)));
		ret = mvx_buffer_afbc_set(buf, port->format, port->width,
					  port->height, port->afbc_width,
					  port->size[0], port->interlaced);
		if (ret != 0)
			return ret;
	} else if (mvx_is_frame(port->format) != false) {
		ret = mvx_buffer_frame_set(buf, port->format, port->width,
					   port->height, port->stride,
					   port->size,
					   port->interlaced);
		if (ret != 0)
			return ret;
	}

	if (mapped &&
		((dir == MVX_DIR_OUTPUT) ||
		 (dir == MVX_DIR_INPUT &&
		  mvx_is_frame(port->format) &&
		  (buf->flags & MVX_BUFFER_FLAG_DISABLE_CACHE_MAINTENANCE))))
	{
		/*
		1. no need to do cache invalidate each time for output buffer,
				only invalidate cache when buffer is mapped
		2. no need to do cache clean for input buffer, if there is
			on cpu write/read usage.
		*/
	}
	else
	{
		ret = mvx_buffer_synch(buf, dma_dir);
	}
	if (ret != 0)
		return ret;

	msg.code = MVX_FW_CODE_BUFFER;
	msg.buf = buf;

	MVX_SESSION_INFO(session,
			 "Firmware req: Buffer. dir=%u, len=[%u, %u, %u], flags=0x%08x, eos=%u, interlace=%u",
			 buf->dir,
			 buf->planes[0].filled,
			 buf->planes[1].filled,
			 buf->planes[2].filled,
			 buf->flags,
			 (buf->flags & MVX_BUFFER_EOS) != 0,
			 (buf->flags & MVX_BUFFER_INTERLACE) != 0);

	ret = session->fw.ops.put_message(&session->fw, &msg);
	if (ret != 0)
		goto send_error;

	port->buffer_count++;

    if (dir == MVX_DIR_OUTPUT && mvx_is_frame(buf->format)) {
        for (i = 0; i < buf->nplanes; i++) {
            if (port->buffer_size[i] == 0)
                port->buffer_size[i] = mvx_buffer_size(buf, i);
        }
    }

	port->flushed = false;
    if (dir == MVX_DIR_OUTPUT && port->isreallocting == true) {
        port->isreallocting = false;
    }
	ret = send_irq(session);
	if (ret != 0)
		goto send_error;

	return 0;

send_error:
	send_event_error(session, ret);
	return ret;
}

/**
 * queue_pending_buffers() - Queue pending buffers.
 *
 * Buffer that are queued when the port is still stream off will be put in the
 * pending queue. Once both input- and output ports are stream on the pending
 * buffers will be forwarded to the firmware.
 *
 * Return: 0 on success, else error code.
 */
static int queue_pending_buffers(struct mvx_session *session,
				 enum mvx_direction dir)
{
	struct mvx_buffer *buf;
	struct mvx_buffer *tmp;
    int roi_config_num = 0;
    int roi_config_index = 0;
    int qp_num = 0;
    int qp_index = 0;
    struct mvx_roi_config roi_config;
	int ret = 0;

    if (dir == MVX_DIR_INPUT && session->port[dir].roi_config_num > 0) {
        roi_config_num = session->port[dir].roi_config_num;
    }
    if (dir == MVX_DIR_INPUT && session->port[dir].qp_num > 0) {
        qp_num = session->port[dir].qp_num;
    }
    list_for_each_entry_safe(buf, tmp, &session->port[dir].buffer_queue,
                    head) {
        if ((buf->flags & MVX_BUFFER_FRAME_FLAG_ROI) == MVX_BUFFER_FRAME_FLAG_ROI &&
            roi_config_index < roi_config_num) {
            roi_config = session->port[dir].roi_config_queue[roi_config_index];
            ret = queue_roi_regions(session, &roi_config);
            roi_config_index++;
        }
        if ((buf->flags & MVX_BUFFER_FRAME_FLAG_GENERAL) == MVX_BUFFER_FRAME_FLAG_GENERAL &&
                            qp_index < qp_num) {
            ret = queue_qp_epr(session, &session->port[dir].qp_queue[qp_index]);
            qp_index++;
        }
        ret = queue_buffer(session, dir, buf);
        if ((buf->flags & MVX_BUFFER_FRAME_NEED_REALLOC) == MVX_BUFFER_FRAME_NEED_REALLOC) {
            session->event(session, MVX_SESSION_EVENT_BUFFER, buf);
        } else if (ret != 0) {
            break;
        }
        list_del(&buf->head);
    }

    session->port[dir].roi_config_num = 0;
    session->port[dir].qp_num = 0;
	return ret;
}

/**
 * fw_bin_ready() - Complete firmware configuration.
 *
 * The firmware binary load has completed and the firmware configuration can
 * begin.
 *
 * If the session is no longer 'stream on' (someone issued 'stream off' before
 * the firmware load completed) the firmware binary is put back to the cache.
 *
 * Else the the client session is registered and the firmware instance is
 * constructed.
 */
static void fw_bin_ready(struct mvx_fw_bin *bin,
			 void *arg,
			 bool same_thread)
{
	struct mvx_session *session = arg;
	int lock_failed = 1;
	int ret;

	/*
	 * Only lock the mutex if the firmware binary was loaded by a
	 * background thread.
	 */
	if (same_thread == false) {
		lock_failed = mutex_lock_interruptible(session->isession.mutex);
		if (lock_failed != 0) {
			send_event_error(session, lock_failed);
			goto put_fw_bin;
		}
	}

	/* Return firmware binary if session is no longer 'stream on'. */
	if (!is_stream_on(session))
		goto put_fw_bin;

	/* Create client session. */
	session->isession.ncores = session->client_ops->get_ncores(
		session->client_ops);
	session->isession.l0_pte = mvx_mmu_set_pte(
		MVX_ATTR_PRIVATE, phys_cpu2vpu(virt_to_phys(session->mmu.page_table)),
		MVX_ACCESS_READ_WRITE);

	session->csession = session->client_ops->register_session(
		session->client_ops, &session->isession);

        mvx_dvfs_register_session(session, is_encoder(session));

	if (IS_ERR(session->csession)) {
		ret = PTR_ERR(session->csession);
		send_event_error(session, ret);
		goto put_fw_bin;
	}

	/* Construct the firmware instance. */
	ret = mvx_fw_factory(&session->fw, bin, &session->mmu,
			     session, session->client_ops, session->csession,
			     session->isession.ncores,
			     session->dentry);
	if (ret != 0) {
		send_event_error(session, ret);
		goto unregister_csession;
	}

	session->fw_bin = bin;

	mvx_fw_cache_log(bin, session->csession);

	ret = fw_initial_setup(session);
	if (ret != 0)
		goto unregister_csession;

	ret = queue_pending_buffers(session, MVX_DIR_INPUT);
	if (ret != 0)
		goto unregister_csession;

	ret = queue_pending_buffers(session, MVX_DIR_OUTPUT);
	if (ret != 0)
		goto unregister_csession;

	ret = mvx_session_put(session);
	if (ret == 0 && lock_failed == 0)
		mutex_unlock(session->isession.mutex);

	return;

unregister_csession:
	mvx_dvfs_unregister_session(session);
	session->client_ops->unregister_session(session->csession);
	session->csession = NULL;

put_fw_bin:
	mvx_fw_cache_put(session->cache, bin);
	session->fw_bin = NULL;

	ret = mvx_session_put(session);
	if (ret == 0 && lock_failed == 0)
		mutex_unlock(session->isession.mutex);
}

static int calc_afbc_size(struct mvx_session *session,
			  enum mvx_format format,
			  unsigned int width,
			  unsigned int height,
			  bool tiled_headers,
			  bool tiled_body,
			  bool superblock,
			  bool interlaced)
{
	static const unsigned int mb_header_size = 16;
	unsigned int payload_align = 128;
	unsigned int mb_size;
	int size;

	/* Calculate width and height in super blocks. */
	if (superblock != false) {
		width = DIV_ROUND_UP(width, 32);
		height = DIV_ROUND_UP(height, 8) + 1;
	} else {
		width = DIV_ROUND_UP(width, 16);
		height = DIV_ROUND_UP(height, 16) + 1;
	}

	/* Round up size to 8x8 tiles. */
	if (tiled_headers != false || tiled_body != false) {
		width = roundup(width, 8);
		height = roundup(height, 8);
	}

	switch (format) {
	case MVX_FORMAT_YUV420_AFBC_8:
		mb_size = 384;
		break;
	case MVX_FORMAT_YUV420_AFBC_10:
		mb_size = 480;
		break;
	case MVX_FORMAT_YUV422_AFBC_8:
		mb_size = 512;
		break;
	case MVX_FORMAT_YUV422_AFBC_10:
		mb_size = 656;
		break;
	default:
		MVX_SESSION_WARN(session,
				 "Unsupported AFBC format. format=%u.",
				 format);
		return -EINVAL;
	}

	/* Round up tiled body to 128 byte boundary. */
	if (tiled_body != false)
		mb_size = roundup(mb_size, payload_align);

	if (interlaced != false)
		height = DIV_ROUND_UP(height, 2);

	/* Calculate size of AFBC makroblock headers. */
	size = roundup(width * height * mb_header_size, payload_align);
	size += roundup(width * height * mb_size, payload_align);

	if (interlaced != false)
		size *= 2;

	return size;
}

static int try_format(struct mvx_session *session,
		      enum mvx_direction dir,
		      enum mvx_format format,
		      unsigned int *width,
		      unsigned int *height,
		      uint8_t *nplanes,
		      unsigned int *stride,
		      unsigned int *size,
		      bool *interlaced)
{
	int ret = 0;

	/* Limit width and height to 8k. */
	if (*width == ((unsigned int) (-1))) *width = 0;
       if (*height == ((unsigned int) (-1))) *height = 0;
	*width = min_t(unsigned int, *width, 8192);
	*height = min_t(unsigned int, *height, 8192);

	/* Stream dimensions are dictated by the input port. */
	if (dir == MVX_DIR_OUTPUT) {
		*width = session->port[MVX_DIR_INPUT].width >> session->port[MVX_DIR_OUTPUT].scaling_shift;
		*height = session->port[MVX_DIR_INPUT].height >> session->port[MVX_DIR_OUTPUT].scaling_shift;
	}
    if (session->dsl_frame.width != 0 && session->dsl_frame.height != 0) {
        *width = session->dsl_frame.width;
        *height = session->dsl_frame.height;
    } else if (dir == MVX_DIR_OUTPUT && (session->dsl_ratio.hor != 1 || session->dsl_ratio.ver !=  1)) {
        *width = session->port[MVX_DIR_INPUT].width / session->dsl_ratio.hor;
        *height = session->port[MVX_DIR_INPUT].height / session->dsl_ratio.ver;
        *width &= ~(1);
        *height &= ~(1);
    }
	/* Interlaced input is not supported by the firmware. */
	if (dir == MVX_DIR_INPUT)
		*interlaced = false;

	if (mvx_is_afbc(format) != false) {
		unsigned int afbc_alloc_bytes =
			session->port[dir].afbc_alloc_bytes;
		if (*nplanes <= 0)
			size[0] = 0;

		if (dir == MVX_DIR_INPUT) {
			/* it is basically a worst-case calcualtion based on a size rounded up to tile size*/
			int s1 = calc_afbc_size(session, format, *width,
					       *height, false, false, false, //*height, false, false, false,
					       *interlaced);
			int s2 = calc_afbc_size(session, format, *width,
					       *height, false, false, true, //*height, false, false, false,
					       *interlaced);
			int s = max_t(unsigned int, s1, s2);
			if (s < 0)
				return s;

			size[0] = max_t(unsigned int, size[0], s);
		}

		if (*interlaced != false)
			afbc_alloc_bytes *= 2;

		size[0] = max_t(unsigned int, size[0],
				afbc_alloc_bytes);
		size[0] = roundup(size[0], PAGE_SIZE);

		*nplanes = 1;
	} else if (mvx_is_frame(format) != false) {
		ret = mvx_buffer_frame_dim(format, *width, *height, nplanes,
					   stride, size);
	} else {
		/*
		 * For compressed formats the size should be the maximum number
		 * of bytes an image is expected to become. This is calculated
		 * as width * height * 2 B/px / 2. Size should be at least one
		 * page.
		 */

		stride[0] = 0;

		if (*nplanes <= 0)
			size[0] = 0;

		size[0] = max_t(unsigned int, size[0], PAGE_SIZE);
		size[0] = max_t(unsigned int, size[0], (*width) * (*height));
		size[0] = roundup(size[0], PAGE_SIZE);

		*nplanes = 1;
	}

	return ret;
}

static void watchdog_work(struct work_struct *work)
{
	struct mvx_session *session =
		container_of(work, struct mvx_session, watchdog_work);
	int ret;

	mutex_lock(session->isession.mutex);
	session->watchdog_count++;

	MVX_SESSION_WARN(session, "Watchdog timeout. count=%u. is_encoder=%d. in_fmt=%d, out_fmt=%d",
			 session->watchdog_count, is_encoder(session), session->port[MVX_DIR_INPUT].format, session->port[MVX_DIR_OUTPUT].format);

	/* Print debug information. */
	print_debug(session);

	if (session->watchdog_count == 1) {
		/* Request firmware to dump its state. */
		fw_dump(session);

		/* Restart watchdog. */
		watchdog_start(session, 3000);
	} else {
		send_event_error(session, -ETIME);
	}

	ret = kref_put(&session->isession.kref, session->isession.release);
	if (ret != 0)
		return;

	mutex_unlock(session->isession.mutex);
}

static void watchdog_timeout(struct timer_list *timer)
{
	struct mvx_session *session =
		container_of(timer, struct mvx_session, watchdog_timer);

	queue_work(system_unbound_wq, &session->watchdog_work);
}

#if KERNEL_VERSION(4, 14, 0) > LINUX_VERSION_CODE
static void watchdog_timeout_legacy(unsigned long data)
{
	watchdog_timeout((struct timer_list *)data);
}

#endif

/****************************************************************************
 * Exported functions
 ****************************************************************************/

int mvx_session_construct(struct mvx_session *session,
			  struct device *dev,
			  struct mvx_client_ops *client_ops,
			  struct mvx_fw_cache *cache,
			  struct mutex *mutex,
			  void (*destructor)(struct mvx_session *session),
			  void (*event)(struct mvx_session *session,
					enum mvx_session_event event,
					void *arg),
			  struct dentry *dentry)
{
	int i;
	int ret;

	if (event == NULL || destructor == NULL)
		return -EINVAL;

	memset(session, 0, sizeof(*session));
	session->dev = dev;
	session->client_ops = client_ops;
	session->cache = cache;
	kref_init(&session->isession.kref);
	session->isession.release = session_destructor;
	session->isession.mutex = mutex;
	session->destructor = destructor;
	session->event = event;
	session->fw_event.fw_bin_ready = fw_bin_ready;
	session->fw_event.arg = session;
	session->fw_state = MVX_FW_STATE_STOPPED;
	init_waitqueue_head(&session->waitq);
	session->dentry = dentry;
	session->port[MVX_DIR_INPUT].buffer_min = 1;
	session->port[MVX_DIR_OUTPUT].buffer_min = 1;
        session->port[MVX_DIR_OUTPUT].seq_param.afbc.buffers_min = 1;
        session->port[MVX_DIR_OUTPUT].seq_param.planar.buffers_min = 1;
    session->port[MVX_DIR_INPUT].buffer_allocated = 1;
    session->port[MVX_DIR_OUTPUT].buffer_allocated = 1;
	session->port[MVX_DIR_INPUT].scaling_shift = 0;
	session->port[MVX_DIR_OUTPUT].scaling_shift = 0;
	session->stream_escaping = MVX_TRI_UNSET;
	session->ignore_stream_headers = MVX_TRI_UNSET;
	session->frame_reordering = MVX_TRI_UNSET;
	session->constr_ipred = MVX_TRI_UNSET;
	session->entropy_sync = MVX_TRI_UNSET;
	session->temporal_mvp = MVX_TRI_UNSET;
	session->resync_interval = -1;
    session->port[MVX_DIR_OUTPUT].roi_config_num = 0;
    session->port[MVX_DIR_INPUT].roi_config_num = 0;
    session->port[MVX_DIR_OUTPUT].qp_num = 0;
    session->port[MVX_DIR_INPUT].qp_num = 0;
    session->crop_left = 0;
    session->crop_right = 0;
    session->crop_top = 0;
    session->crop_bottom = 0;
    session->dsl_ratio.hor = 1;
    session->dsl_ratio.ver = 1;
    session->dsl_pos_mode = -1;//disable by default
    session->estimated_ddr_read_throughput = 0;
    session->estimated_ddr_write_throughput = 0;
    session->port[MVX_DIR_OUTPUT].buffer_on_hold_count = 0;
    session->port[MVX_DIR_OUTPUT].pending_buffer_on_hold_count = 0;
    session->port[MVX_DIR_OUTPUT].isallocparam = false;
    session->eos_queued = false;
    session->keep_freq_high = true;
    session->is_suspend = false;
    for (i = 0; i < MVX_BUFFER_NPLANES; i++) {
        session->port[MVX_DIR_OUTPUT].buffer_size[i] = 0;
    }
    session->port[MVX_DIR_OUTPUT].buffer_count = 0;
    session->port[MVX_DIR_INPUT].buffer_count = 0;
    session->watchdog_count = 0;
    session->watchdog_timeout = session_watchdog_timeout;
    session->frame_id = 0;

    INIT_LIST_HEAD(&session->buffer_corrupt_queue);
	ret = mvx_mmu_construct(&session->mmu, session->dev);
	if (ret != 0)
		return ret;

	for (i = 0; i < MVX_DIR_MAX; i++)
		INIT_LIST_HEAD(&session->port[i].buffer_queue);

#if KERNEL_VERSION(4, 14, 0) <= LINUX_VERSION_CODE
	timer_setup(&session->watchdog_timer, watchdog_timeout, 0);
#else
	setup_timer(&session->watchdog_timer, watchdog_timeout_legacy,
		    (uintptr_t)&session->watchdog_timer);
#endif
	INIT_WORK(&session->watchdog_work, watchdog_work);

	return 0;
}

void mvx_session_destruct(struct mvx_session *session)
{
	/* Destruct the session object. */
	struct mvx_corrupt_buffer* corrupt_buf;
	struct mvx_corrupt_buffer* tmp;

    if (session->enable_profiling && session->frame_id > 0) {
        MVX_SESSION_WARN(session, "[Debug] Destroy session. is_encoder=%d, frame_count=%d", is_encoder(session), session->frame_id);
        MVX_SESSION_WARN(session, "[Debug] bus_write_bytes_total=%lu. bus_read_bytes_total=%lu. avg_write_bw=%lu, avg_read_bw=%lu, 30fps_write_bw=%lu, 30fps_read_bw=%lu", session->bus_write_bytes_total, session->bus_read_bytes_total, session->bus_write_bytes_total/session->frame_id, session->bus_read_bytes_total/session->frame_id, session->bus_write_bytes_total/session->frame_id * 30, session->bus_read_bytes_total/session->frame_id * 30);
    }

	MVX_SESSION_INFO(session, "Destroy session.");

	release_fw_bin(session);
	mvx_mmu_destruct(&session->mmu);
	list_for_each_entry_safe(corrupt_buf, tmp, &session->buffer_corrupt_queue, head) {
		list_del(&corrupt_buf->head);
		vfree(corrupt_buf);
	}
}

void mvx_session_get(struct mvx_session *session)
{
	kref_get(&session->isession.kref);
}

int mvx_session_put(struct mvx_session *session)
{
	return kref_put(&session->isession.kref,
			session->isession.release);
}

void mvx_session_get_formats(struct mvx_session *session,
			     enum mvx_direction dir,
			     uint64_t *formats)
{
	uint64_t fw_formats;

	session->client_ops->get_formats(session->client_ops, dir, formats);
	mvx_fw_cache_get_formats(session->cache, dir, &fw_formats);

	*formats &= fw_formats;
}

int mvx_session_try_format(struct mvx_session *session,
			   enum mvx_direction dir,
			   enum mvx_format format,
			   unsigned int *width,
			   unsigned int *height,
			   uint8_t *nplanes,
			   unsigned int *stride,
			   unsigned int *size,
			   bool *interlaced)
{
	return try_format(session, dir, format, width, height, nplanes,
			  stride, size, interlaced);
}

int mvx_session_set_format(struct mvx_session *session,
			   enum mvx_direction dir,
			   enum mvx_format format,
			   unsigned int *width,
			   unsigned int *height,
			   uint8_t *nplanes,
			   unsigned int *stride,
			   unsigned int *size,
			   bool *interlaced)
{
	struct mvx_session_port *port = &session->port[dir];
	int ret;

	if (session->error != 0)
		return session->error;

	if (is_stream_on(session) != false)
		return -EBUSY;

	ret = try_format(session, dir, format, width, height, nplanes,
			 stride, size, interlaced);
	if (ret != 0)
		return ret;

	/*
	 * If the bitstream format changes, then the firmware binary must be
	 * released.
	 */
	if (mvx_is_bitstream(port->format) != false &&
	    format != port->format) {
		if (IS_ERR(session->fw_bin) != false) {
			MVX_SESSION_WARN(session,
					 "Can't set format when firmware binary is pending. dir=%d.",
					 dir);
			return -EINVAL;
		}

		release_fw_bin(session);
	}

	/* Update port settings. */
	port->format = format;
	port->width = *width;
	port->height = *height;
	port->nplanes = *nplanes;
	port->interlaced = *interlaced;
	memcpy(port->stride, stride, sizeof(*stride) * MVX_BUFFER_NPLANES);
	memcpy(port->size, size, sizeof(*size) * MVX_BUFFER_NPLANES);

	/* TODO AFBC width will have to be provided by user space. */
	if (dir == MVX_DIR_INPUT)
		port->afbc_width = DIV_ROUND_UP(*width, 16);

	/* Input dimensions dictate output dimensions. */
	if (dir == MVX_DIR_INPUT) {
		struct mvx_session_port *p = &session->port[MVX_DIR_OUTPUT];
		(void)try_format(session, MVX_DIR_OUTPUT, p->format, &p->width,
				 &p->height, &p->nplanes, p->stride, p->size,
				 &p->interlaced);
	}

        if (dir == MVX_DIR_OUTPUT) {
            if (mvx_is_afbc(port->format)) {
                port->buffer_min = port->seq_param.afbc.buffers_min;
            } else {
                port->buffer_min = port->seq_param.planar.buffers_min;
            }
        }

	return 0;
}

int mvx_session_qbuf(struct mvx_session *session,
		     enum mvx_direction dir,
		     struct mvx_buffer *buf)
{
	int ret;
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) == false ||
	    session->port[dir].is_flushing != false) {
		list_add_tail(&buf->head, &session->port[dir].buffer_queue);
		return 0;
	}

	ret = queue_buffer(session, dir, buf);
	if (ret != 0)
		return ret;

	ret = switch_in(session);
	if (ret != 0)
		return ret;

	return 0;
}

int mvx_session_send_eos(struct mvx_session *session)
{
	struct mvx_session_port *port = &session->port[MVX_DIR_OUTPUT];
	struct mvx_buffer *buf;

	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return fw_eos(session);

	if (list_empty(&port->buffer_queue) != false) {
		MVX_SESSION_WARN(session,
				 "Unable to signal EOS. Output buffer queue empty.");
		return 0;
	}

	buf = list_first_entry(&port->buffer_queue, struct mvx_buffer, head);
	list_del(&buf->head);

	mvx_buffer_clear(buf);
	buf->flags |= MVX_BUFFER_EOS;

	session->event(session, MVX_SESSION_EVENT_BUFFER, buf);

	return 0;
}

int mvx_session_streamon(struct mvx_session *session,
			 enum mvx_direction dir)
{
	enum mvx_direction bdir;
	struct mvx_hw_ver hw_ver;
	enum mvx_direction i;
	int ret;

	MVX_SESSION_INFO(session, "Stream on. dir=%u.", dir);

    if (dir == MVX_DIR_OUTPUT && session->port[dir].isallocparam == true) {
        session->port[dir].isallocparam = false;
        session->port[dir].isreallocting = false;
    }

	/* Verify that we don't enable an already activated port. */
	if (session->port[dir].stream_on != false)
		return 0;

	session->port[dir].stream_on = true;

	/* Check that both ports are stream on. */
	if (!is_stream_on(session))
		return 0;

        ddr_qos_request_update(session);

	/* Verify that a firmware binary load is not in progress. */
	if (IS_ERR(session->fw_bin)) {
		ret = PTR_ERR(session->fw_bin);
		goto disable_port;
	}

	/* If a firmware binary is already loaded, then we are done. */
	if (session->fw_bin != NULL) {
		ret = wait_pending(session);
		if (ret != 0)
			goto disable_port;

		ret = fw_state_change(session, MVX_FW_STATE_RUNNING);
		if (ret != 0)
			goto disable_port;

		return 0;
	}

	bdir = get_bitstream_port(session);
	if (bdir >= MVX_DIR_MAX) {
		MVX_SESSION_WARN(session,
				 "Session only support decoding and encoding, but not transcoding. input_format=%u, output_format=%u.",
				 session->port[MVX_DIR_INPUT].format,
				 session->port[MVX_DIR_OUTPUT].format);
		ret = -EINVAL;
		goto disable_port;
	}

	/* Verify that client can handle input and output formats. */
	for (i = MVX_DIR_INPUT; i < MVX_DIR_MAX; i++) {
		uint64_t formats;

		session->client_ops->get_formats(session->client_ops,
						 MVX_DIR_INPUT, &formats);

		if (!mvx_test_bit(session->port[i].format, &formats)) {
			MVX_SESSION_WARN(session,
					 "Client cannot support requested formats. input_format=%u, output_format=%u.",
					 session->port[MVX_DIR_INPUT].format,
					 session->port[MVX_DIR_OUTPUT].format);
			ret = -ENODEV;
			goto disable_port;
		}
	}

	/* Increment session reference count and flag fw bin as pending. */
	mvx_session_get(session);
	session->fw_bin = ERR_PTR(-EINPROGRESS);
	session->client_ops->get_hw_ver(session->client_ops, &hw_ver);

	/* Requesting firmware binary to be loaded. */
	ret = mvx_fw_cache_get(session->cache, session->port[bdir].format,
			       bdir, &session->fw_event, &hw_ver,
			       session->isession.securevideo);
	if (ret != 0) {
		session->port[dir].stream_on = false;
		session->fw_bin = NULL;
		mvx_session_put(session);
		return ret;
	}

	return 0;

disable_port:
	session->port[dir].stream_on = false;

	return ret;
}

int mvx_session_streamoff(struct mvx_session *session,
			  enum mvx_direction dir)
{
    struct mvx_session_port *port = &session->port[dir];
    struct mvx_session_port *port_input = &session->port[MVX_DIR_INPUT];
    struct mvx_buffer *buf;
    struct mvx_buffer *tmp;
    bool force_stop = (dir == MVX_DIR_OUTPUT && port_input->stream_on == false) ? 1 : 0;
    int ret = 0;
    int i;

	MVX_SESSION_INFO(session, "Stream off. dir=%u, flushed=%d, is_flushing=%d, isreallocting=%d, isallocparam=%d, force_stop=%d", dir,port->flushed,port->is_flushing,port->isreallocting,port->isallocparam,force_stop);

	port->stream_on = false;

	if (is_fw_loaded(session) != false) {
		/*
		 * Flush the ports if at least one buffer has been queued
		 * since last flush.
		 */
		/* workaround for fw issue: It won't flush output buffer when STOP output stream
		 * if flushed flag is true. It will result in no frame buffer output for seek operations.
		 * Now force to flush output buffer if input stream is off.
		 */

		if ((port->flushed == false && port->is_flushing == false) || force_stop) {
			ret = wait_pending(session);
			if (ret != 0)
				goto dequeue_buffers;
            if (!(dir == MVX_DIR_OUTPUT && port->isreallocting == true) || force_stop) {
    			ret = fw_state_change(session, MVX_FW_STATE_STOPPED);
    			if (ret != 0)
    				goto dequeue_buffers;

    			ret = fw_flush(session, dir);
    			if (ret != 0) {
    				goto dequeue_buffers;
    			}
            }
			ret = wait_pending(session);
			if (ret != 0)
				goto dequeue_buffers;

			send_irq(session);
		}
        if (dir == MVX_DIR_OUTPUT && port->isallocparam == true) {
            wait_flush_done(session, dir);
            for (i = 0; i < MVX_BUFFER_NPLANES; i++)
                session->port[MVX_DIR_OUTPUT].buffer_size[i] = 0;
        }
        if (session->fw_state == MVX_FW_STATE_STOPPED) {
            fw_switch_out(session);
            wait_switch_out(session);
            if (session->switched_in) {
                MVX_SESSION_WARN(session, "warn: switch_in is %d when stream off done.", session->switched_in);
            }
        }
    }
dequeue_buffers:
	if (ret != 0) {
		MVX_SESSION_WARN(session, "stream off error. ret=%d. mvx_session=%p", ret, session);
		session_unregister(session);
	}

	/* Return buffers in pending queue. */
	list_for_each_entry_safe(buf, tmp, &port->buffer_queue, head) {
		list_del(&buf->head);
		session->event(session, MVX_SESSION_EVENT_BUFFER, buf);
	}

	return 0;
}

static void handle_fw_message(struct mvx_session *session,
			      struct mvx_fw_msg *msg)
{
	switch (msg->code) {
       case MVX_FW_CODE_ALLOC_PARAM: {
           struct mvx_session_port *input = &session->port[MVX_DIR_INPUT];
           struct mvx_session_port *output = &session->port[MVX_DIR_OUTPUT];
           unsigned int old_stride[MVX_BUFFER_NPLANES];

		/* Update input port. */
		input->width = msg->alloc_param.width;
		input->height = msg->alloc_param.height;

		try_format(session, MVX_DIR_INPUT, input->format, &input->width,
			   &input->height, &input->nplanes, input->stride,
			   input->size, &input->interlaced);

		/*
		 * Update output port. Set number of valid planes to 0 to force
		 * stride to be recalculated.
		 */

		output->nplanes = 0;
        output->afbc_alloc_bytes = msg->alloc_param.afbc_alloc_bytes;
        output->afbc_width = msg->alloc_param.afbc_width;
        old_stride[0] = output->stride[0];
        old_stride[1] = output->stride[1];
        old_stride[2] = output->stride[2];

		try_format(session, MVX_DIR_OUTPUT, output->format,
			   &output->width, &output->height, &output->nplanes,
			   output->stride, output->size,
			   &output->interlaced);

		MVX_SESSION_INFO(session,
				 "Firmware rsp: Alloc param. width=%u, height=%u, nplanes=%u, size=[%u, %u, %u], stride=[%u, %u, %u], interlaced=%d.",
				 msg->alloc_param.width,
				 msg->alloc_param.height,
				 output->nplanes,
				 output->size[0],
				 output->size[1],
				 output->size[2],
				 output->stride[0],
				 output->stride[1],
				 output->stride[2],
				 output->interlaced);

		//update ddr qos for decoder output size changed.
        ddr_qos_request_update(session);

        if (output->size[0] > output->buffer_size[0] ||
                output->size[1] > output->buffer_size[1] ||
                output->size[2] > output->buffer_size[2]) {
            output->isreallocting = true;
        } else {
            // don't update strides for some vp9 cases. gralloc buffer stride is fixed if no realloc.
            output->stride[0] = old_stride[0];
            output->stride[1] = old_stride[1];
            output->stride[2] = old_stride[2];
        }
		break;
	}
    case MVX_FW_CODE_BUFFER_GENERAL: {
        struct mvx_buffer *buf = msg->buf;
        session->port[buf->dir].buffer_count--;
        session->event(session, MVX_SESSION_EVENT_BUFFER, buf);
        break;
    }
	case MVX_FW_CODE_BUFFER: {
		struct mvx_buffer *buf = msg->buf;
        struct mvx_session_port *output =
			&session->port[MVX_DIR_OUTPUT];
        struct mvx_v4l2_session *vsession =
               container_of(session, struct mvx_v4l2_session, session);
		MVX_SESSION_INFO(session,
				 "Firmware rsp: Buffer. dir=%u, len=[%u, %u, %u], flags=0x%08x, eos=%u",
				 buf->dir,
				 buf->planes[0].filled,
				 buf->planes[1].filled,
				 buf->planes[2].filled,
				 buf->flags,
				 (buf->flags & MVX_BUFFER_EOS) != 0);

		session->port[buf->dir].buffer_count--;

       if (buf->dir == MVX_DIR_OUTPUT && (buf->flags & MVX_BUFFER_EOS) != 0)	{
           session->eos_queued = false;
       }

		/*
		 * There is no point to flush or invalidate input buffer
		 * after it was returned from the HW.
		 */
        if (buf->dir == MVX_DIR_OUTPUT && mvx_is_frame(buf->format)) {
            if (!(buf->flags & MVX_BUFFER_FRAME_PRESENT)) {
                if (output->size[0] > mvx_buffer_size(buf, 0) ||
                    output->size[1] > mvx_buffer_size(buf, 1) ||
                    output->size[2] > mvx_buffer_size(buf, 2) ||
                    session->port[buf->dir].buffer_allocated < session->port[buf->dir].buffer_min) {
                    buf->flags |= MVX_BUFFER_FRAME_NEED_REALLOC;
                    //output->isreallocting = true;
                }
            }
        }

		if (buf->dir == MVX_DIR_OUTPUT && vsession->port[MVX_DIR_OUTPUT].q_set == true)
			mvx_buffer_synch(buf, DMA_FROM_DEVICE);

		if (buf->dir == MVX_DIR_OUTPUT && !is_encoder(session)) {
                       session->port[MVX_DIR_OUTPUT].buffer_on_hold_count = session->port[MVX_DIR_OUTPUT].pending_buffer_on_hold_count;
		}

		session->event(session, MVX_SESSION_EVENT_BUFFER, buf);
		break;
	}
	case MVX_FW_CODE_COLOR_DESC: {
		MVX_SESSION_INFO(session,
				 "Firmware rsp: Color desc.");
		session->color_desc = msg->color_desc;
		session->event(session, MVX_SESSION_EVENT_COLOR_DESC, NULL);
		break;
	}
	case MVX_FW_CODE_ERROR: {
		MVX_SESSION_WARN(session,
				 "Firmware rsp: Error. code=%u, message=%s.",
				 msg->error.error_code, msg->error.message);
		print_debug(session);
		fw_dump(session);

		/*
		 * Release the dev session. It will prevent a dead session from
		 * blocking the scheduler.
		 */
		watchdog_stop(session);
		session_unregister(session);
		send_event_error(session, -EINVAL);
		break;
	}
	case MVX_FW_CODE_FLUSH: {
		MVX_SESSION_INFO(session, "Firmware rsp: Flushed. dir=%d.",
				 msg->flush.dir);
		session->port[msg->flush.dir].is_flushing = false;
		session->port[msg->flush.dir].flushed = true;
		(void)queue_pending_buffers(session, msg->flush.dir);
		break;
	}
	case MVX_FW_CODE_IDLE: {
		int ret;
		struct mvx_fw_msg msg_ack;

		MVX_SESSION_INFO(session, "Firmware rsp: Idle.");

		session->idle_count++;

		if (session->idle_count == 2)
			fw_switch_out(session);

		msg_ack.code = MVX_FW_CODE_IDLE_ACK;
		ret = session->fw.ops.put_message(&session->fw, &msg_ack);
		if (ret == 0)
			ret = send_irq(session);

		if (ret != 0)
			send_event_error(session, ret);

		break;
	}
	case MVX_FW_CODE_JOB: {
		MVX_SESSION_INFO(session, "Firmware rsp: Job.");
		(void)fw_job(session, 1);
		break;
	}
	case MVX_FW_CODE_PONG:
		MVX_SESSION_INFO(session, "Firmware rsp: Pong.");
		break;
	case MVX_FW_CODE_SEQ_PARAM: {
		struct mvx_session_port *p = &session->port[MVX_DIR_OUTPUT];

		MVX_SESSION_INFO(session,
				 "Firmware rsp: Seq param. planar={buffers_min=%u}, afbc={buffers_min=%u}, interlaced=%d.",
				 msg->seq_param.planar.buffers_min,
				 msg->seq_param.afbc.buffers_min,
				 p->interlaced);

		if (mvx_is_afbc(p->format) != false)
			p->buffer_min = msg->seq_param.afbc.buffers_min;
		else
			p->buffer_min = msg->seq_param.planar.buffers_min;
                p->seq_param = msg->seq_param;

		(void)fw_flush(session, MVX_DIR_OUTPUT);

        //force to realloc if received seq_param message for it will flush output buffer.
        p->isreallocting = true;

		break;
	}
	case MVX_FW_CODE_SET_OPTION: {
		MVX_SESSION_INFO(session, "Firmware rsp: Set option.");
		break;
	}
	case MVX_FW_CODE_STATE_CHANGE: {
		MVX_SESSION_INFO(session,
				 "Firmware rsp: State changed. old=%s, new=%s.",
				 state_to_string(session->fw_state),
				 state_to_string(msg->state));
		session->fw_state = msg->state;
		break;
	}
	case MVX_FW_CODE_SWITCH_IN: {
		watchdog_start(session, session->watchdog_timeout);
		break;
	}
	case MVX_FW_CODE_SWITCH_OUT: {
		MVX_SESSION_INFO(session, "Firmware rsp: Switched out.");

		watchdog_stop(session);
		session->switched_in = false;

		if (session->is_suspend == false && ((session->fw_state == MVX_FW_STATE_RUNNING &&
		     session->idle_count < 2) ||
		    session->fw.msg_pending > 0))
			switch_in(session);

		break;
	}
	case MVX_FW_CODE_DUMP:
		break;
	case MVX_FW_CODE_DEBUG:
		break;
	case MVX_FW_CODE_UNKNOWN: {
		print_debug(session);
		break;
	}
	case MVX_FW_CODE_DPB_HELD_FRAMES: {
		session->port[MVX_DIR_OUTPUT].pending_buffer_on_hold_count = msg->arg;
		break;
	}
	case MVX_FW_CODE_MAX:
		break;
	default:
		MVX_SESSION_WARN(session, "Unknown fw msg code. code=%u.",
				 msg->code);
	}
}

void mvx_session_irq(struct mvx_if_session *isession)
{
	struct mvx_session *session = mvx_if_session_to_session(isession);
	int ret;
	struct mvx_session_port *output = &session->port[MVX_DIR_OUTPUT];

	if (is_fw_loaded(session) == false)
		return;

	if (IS_ERR_OR_NULL(session->csession)) {
		return;
	}

	ret = session->fw.ops.handle_rpc(&session->fw);
	if (ret < 0) {
		send_event_error(session, ret);
		return;
	}

	do {
		struct mvx_fw_msg msg;

		watchdog_update(session, session->watchdog_timeout);

		ret = session->fw.ops.get_message(&session->fw, &msg);
		if (ret < 0) {
			session_unregister(session);
			send_event_error(session, ret);
			return;
		}

		if (ret > 0)
			handle_fw_message(session, &msg);
	} while (ret > 0 && session->error == 0);

    if (output->isallocparam == false && output->isreallocting == true) {
        output->isallocparam = true;
        session->event(session, MVX_SESSION_EVENT_PORT_CHANGED, (void *)MVX_DIR_OUTPUT);
    }

#ifdef MVX_FW_DEBUG_ENABLE
	ret = session->fw.ops.handle_fw_ram_print(&session->fw);
	if (ret < 0) {
		send_event_error(session, ret);
		return;
	}
#endif

	wake_up(&session->waitq);
}

void mvx_session_port_show(struct mvx_session_port *port,
			   struct seq_file *s)
{
	mvx_seq_printf(s, "mvx_session_port", 0, "%p\n", port);
	mvx_seq_printf(s, "format", 1, "%08x\n", port->format);
	mvx_seq_printf(s, "width", 1, "%u\n", port->width);
	mvx_seq_printf(s, "height", 1, "%u\n", port->height);
	mvx_seq_printf(s, "buffer_min", 1, "%u\n", port->buffer_min);
	mvx_seq_printf(s, "buffer_count", 1, "%u\n", port->buffer_count);
}

int mvx_session_set_securevideo(struct mvx_session *session,
				bool securevideo)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->isession.securevideo = securevideo;

	return 0;
}

int mvx_session_set_frame_rate(struct mvx_session *session,
			       int64_t frame_rate)
{
	int ret;
	if (session->error != 0)
		return session->error;

	/*Frame rate values are limited to between 1 and 256 frames per second*/
	if (frame_rate < (1 << 16)) frame_rate = (1 << 16); /*1 fps*/
	else if (frame_rate > (256 << 16)) frame_rate = (256 << 16); /*256 fps*/

	if (is_fw_loaded(session) != false) {
		struct mvx_fw_set_option option;

		option.code = MVX_FW_SET_FRAME_RATE;
		option.frame_rate = frame_rate;
		ret = fw_set_option(session, &option);
		if (ret != 0)
			return ret;
	}

        //update ddr qos for framerate changed.
        if (frame_rate != session->frame_rate) {
             session->frame_rate = frame_rate;
             ddr_qos_request_update(session);
        }

	return 0;
}

int mvx_session_set_rate_control(struct mvx_session *session,
				 bool enabled)
{
	int ret;
	if (session->error != 0)
		return session->error;

	session->rc_enabled = enabled;
	//set default rc type.
	session->rc_type = enabled?((session->rc_type)?session->rc_type:MVX_OPT_RATE_CONTROL_MODE_STANDARD):0;

	if (is_fw_loaded(session) != false) {
		struct mvx_fw_set_option option;

		option.code = MVX_FW_SET_TARGET_BITRATE;
		option.target_bitrate =
			(session->rc_enabled != false) ?
			session->target_bitrate : 0;
		ret = fw_set_option(session, &option);
		if (ret != 0)
			return ret;
	}

	return 0;
}

int mvx_session_set_bitrate(struct mvx_session *session,
			    int bitrate)
{
    int ret;

    if (session->error != 0)
        return session->error;

    session->target_bitrate = bitrate;

    if (is_fw_loaded(session) != false && session->rc_enabled != false) {
	    struct mvx_fw_set_option option;

        option.code = MVX_FW_SET_RATE_CONTROL;
        option.rate_control.target_bitrate = session->target_bitrate;
        option.rate_control.rate_control_mode = session->rc_type;

        ret = fw_set_option(session, &option);
        if (ret != 0)
            return ret;
    }

    return 0;
}

int mvx_session_set_bitrate_control(struct mvx_session *session,
			    struct mvx_buffer_param_rate_control *rc){
    int ret;

    if (session->error != 0)
        return session->error;

    session->rc_type = rc->rate_control_mode;
    session->target_bitrate = rc->target_bitrate;
    session->maximum_bitrate = rc->maximum_bitrate;
    if (is_fw_loaded(session) != false) {
        struct mvx_fw_set_option option;

        option.code = MVX_FW_SET_RATE_CONTROL;
        option.rate_control.target_bitrate = rc->target_bitrate;
        option.rate_control.rate_control_mode = rc->rate_control_mode;
        if (rc->rate_control_mode == MVX_OPT_RATE_CONTROL_MODE_C_VARIABLE) {
            option.rate_control.maximum_bitrate = rc->maximum_bitrate;
        }
        ret = fw_set_option(session, &option);
        if (ret != 0)
            return ret;
    }
    return 0;
}

int mvx_session_set_crop_left(struct mvx_session * session, int32_t left){
    int ret;

    if (session->error != 0)
        return session->error;

    session->crop_left = left;

    if (is_fw_loaded(session) != false) {
        struct mvx_fw_set_option option;

        option.code = MVX_FW_SET_CROP_LEFT;
        option.crop_left = left;
        ret = fw_set_option(session, &option);
        if (ret != 0)
            return ret;
    }
    return 0;

}

int mvx_session_set_crop_right(struct mvx_session * session, int32_t right){
    int ret;

    if (session->error != 0)
        return session->error;

    session->crop_right = right;

    if (is_fw_loaded(session) != false) {
        struct mvx_fw_set_option option;

        option.code = MVX_FW_SET_CROP_RIGHT;
        option.crop_right = right;
        ret = fw_set_option(session, &option);
        if (ret != 0)
            return ret;
    }
    return 0;

}

int mvx_session_set_crop_top(struct mvx_session * session, int32_t top){
    int ret;

    if (session->error != 0)
        return session->error;

    session->crop_top = top;

    if (is_fw_loaded(session) != false) {
        struct mvx_fw_set_option option;

        option.code = MVX_FW_SET_CROP_TOP;
        option.crop_top = top;
        ret = fw_set_option(session, &option);
        if (ret != 0)
            return ret;
    }
    return 0;

}

int mvx_session_set_crop_bottom(struct mvx_session * session, int32_t bottom){
    int ret;

    if (session->error != 0)
        return session->error;

    session->crop_bottom = bottom;

    if (is_fw_loaded(session) != false) {
        struct mvx_fw_set_option option;

        option.code = MVX_FW_SET_CROP_BOTTOM;
        option.crop_bottom = bottom;
        ret = fw_set_option(session, &option);
        if (ret != 0)
            return ret;
    }
    return 0;

}

int mvx_session_set_nalu_format(struct mvx_session *session,
				enum mvx_nalu_format fmt)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->nalu_format = fmt;

	return 0;
}

int mvx_session_set_stream_escaping(struct mvx_session *session,
				    enum mvx_tristate status)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->stream_escaping = status;

	return 0;
}

int mvx_session_set_profile(struct mvx_session *session,
			    enum mvx_format format,
			    enum mvx_profile profile)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->profile[format] = profile;

	return 0;
}

int mvx_session_set_level(struct mvx_session *session,
			  enum mvx_format format,
			  enum mvx_level level)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->level[format] = level;

	return 0;
}

int mvx_session_set_ignore_stream_headers(struct mvx_session *session,
					  enum mvx_tristate status)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->ignore_stream_headers = status;

	return 0;
}

int mvx_session_set_frame_reordering(struct mvx_session *session,
				     enum mvx_tristate status)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->frame_reordering = status;

	return 0;
}

int mvx_session_set_intbuf_size(struct mvx_session *session,
				int size)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->intbuf_size = size;

	return 0;
}

int mvx_session_set_p_frames(struct mvx_session *session,
			     int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->p_frames = val;

	return 0;
}

int mvx_session_set_b_frames(struct mvx_session *session,
			     int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->b_frames = val;

	return 0;
}

int mvx_session_set_gop_type(struct mvx_session *session,
			     enum mvx_gop_type gop_type)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->gop_type = gop_type;

	return 0;
}

int mvx_session_set_cyclic_intra_refresh_mb(struct mvx_session *session,
					    int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->cyclic_intra_refresh_mb = val;

	return 0;
}

int mvx_session_set_constr_ipred(struct mvx_session *session,
				 enum mvx_tristate status)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->constr_ipred = status;

	return 0;
}

int mvx_session_set_entropy_sync(struct mvx_session *session,
				 enum mvx_tristate status)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->entropy_sync = status;

	return 0;
}

int mvx_session_set_temporal_mvp(struct mvx_session *session,
				 enum mvx_tristate status)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->temporal_mvp = status;

	return 0;
}

int mvx_session_set_tile_rows(struct mvx_session *session,
			      int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->tile_rows = val;

	return 0;
}

int mvx_session_set_tile_cols(struct mvx_session *session,
			      int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->tile_cols = val;

	return 0;
}

int mvx_session_set_min_luma_cb_size(struct mvx_session *session,
				     int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->min_luma_cb_size = val;

	return 0;
}

int mvx_session_set_mb_mask(struct mvx_session *session,
			    int val)
{
	/*
	 * This controls is not implemented.
	 */
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->mb_mask = val;

	return 0;
}

int mvx_session_set_entropy_mode(struct mvx_session *session,
				 enum mvx_entropy_mode mode)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->entropy_mode = mode;

	return 0;
}

int mvx_session_set_multi_slice_mode(struct mvx_session *session,
				     enum mvx_multi_slice_mode mode)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->multi_slice_mode = mode;

	return 0;
}

int mvx_session_set_multi_slice_max_mb(struct mvx_session *session,
				       int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->multi_slice_max_mb = val;

	return 0;
}

int mvx_session_set_vp9_prob_update(struct mvx_session *session,
				    enum mvx_vp9_prob_update mode)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->vp9_prob_update = mode;

	return 0;
}

int mvx_session_set_mv_h_search_range(struct mvx_session *session,
				      int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->mv_h_search_range = val;

	return 0;
}

int mvx_session_set_mv_v_search_range(struct mvx_session *session,
				      int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->mv_v_search_range = val;

	return 0;
}

int mvx_session_set_bitdepth_chroma(struct mvx_session *session,
				    int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->bitdepth_chroma = val;

	return 0;
}

int mvx_session_set_bitdepth_luma(struct mvx_session *session,
				  int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->bitdepth_luma = val;

	return 0;
}

int mvx_session_set_force_chroma_format(struct mvx_session *session,
					int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->force_chroma_format = val;

	return 0;
}

int mvx_session_set_rgb_to_yuv_mode(struct mvx_session *session,
				    enum mvx_rgb_to_yuv_mode mode)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->rgb_to_yuv = mode;

	return 0;
}

int mvx_session_set_band_limit(struct mvx_session *session,
			       int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->band_limit = val;

	return 0;
}

int mvx_session_set_cabac_init_idc(struct mvx_session *session,
				   int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->cabac_init_idc = val;

	return 0;
}

int mvx_session_set_i_frame_qp(struct mvx_session *session,
			       enum mvx_format fmt,
			       int qp)
{
	int ret;

	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false) {
		enum mvx_direction dir = get_bitstream_port(session);

		fmt = session->port[dir].format;
		ret = fw_set_qp(session, MVX_FW_SET_QP_I, qp);
		if (ret != 0)
			return ret;
	}

	session->qp[fmt].i_frame = qp;

	return 0;
}

int mvx_session_set_p_frame_qp(struct mvx_session *session,
			       enum mvx_format fmt,
			       int qp)
{
	int ret;

	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false) {
		enum mvx_direction dir = get_bitstream_port(session);

		fmt = session->port[dir].format;
		ret = fw_set_qp(session, MVX_FW_SET_QP_P, qp);
		if (ret != 0)
			return ret;
	}

	session->qp[fmt].p_frame = qp;

	return 0;
}

int mvx_session_set_b_frame_qp(struct mvx_session *session,
			       enum mvx_format fmt,
			       int qp)
{
	int ret;

	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false) {
		enum mvx_direction dir = get_bitstream_port(session);

		fmt = session->port[dir].format;
		ret = fw_set_qp(session, MVX_FW_SET_QP_B, qp);
		if (ret != 0)
			return ret;
	}

	session->qp[fmt].b_frame = qp;

	return 0;
}

int mvx_session_set_min_qp(struct mvx_session *session,
			   enum mvx_format fmt,
			   int qp)
{
	int ret;

	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false) {
		struct mvx_fw_set_option option;
		enum mvx_direction dir = get_bitstream_port(session);
		int codec = session->port[dir].format;

		option.code = MVX_FW_SET_QP_RANGE;
		option.qp_range.min = qp;
		option.qp_range.max = session->qp[codec].max;
		ret = fw_set_option(session, &option);
		if (ret != 0)
			return ret;
	}

	session->qp[fmt].min = qp;

	return 0;
}

int mvx_session_set_max_qp(struct mvx_session *session,
			   enum mvx_format fmt,
			   int qp)
{
	int ret;

	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false) {
		struct mvx_fw_set_option option;
		enum mvx_direction dir = get_bitstream_port(session);
		int codec = session->port[dir].format;

		option.code = MVX_FW_SET_QP_RANGE;
		option.qp_range.min = session->qp[codec].min;
		option.qp_range.max = qp;
		ret = fw_set_option(session, &option);
		if (ret != 0)
			return ret;
	}

	session->qp[fmt].max = qp;

	return 0;
}

int mvx_session_set_resync_interval(struct mvx_session *session,
				    int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->resync_interval = val;

	return 0;
}

int mvx_session_set_jpeg_quality(struct mvx_session *session,
				 int val)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->jpeg_quality = val;

	return 0;
}

int mvx_session_get_color_desc(struct mvx_session *session,
			       struct mvx_fw_color_desc *color_desc)
{
	*color_desc = session->color_desc;
	return 0;
}

int mvx_session_set_color_desc(struct mvx_session *session,
			       struct mvx_fw_color_desc *color_desc)
{
    int ret = 0;
    if (session->error != 0)
        return session->error;

    session->color_desc = *color_desc;
    if (is_fw_loaded(session) != false) {
        struct mvx_fw_set_option option;

        option.code = MVX_FW_SET_COLOUR_DESC;
        option.colour_desc = *color_desc;
        ret = fw_set_option(session, &option);
        if (ret != 0)
            return ret;
    }
	return 0;
}

int mvx_session_set_roi_regions(struct mvx_session *session,
			       struct mvx_roi_config *roi)
{
    int ret = 0;
    int roi_config_num = 0;
    if (is_fw_loaded(session) == false ||
        session->port[MVX_DIR_INPUT].is_flushing != false) {
        roi_config_num = session->port[MVX_DIR_INPUT].roi_config_num;
        if (roi_config_num < MVX_ROI_QP_NUMS) {
            MVX_SESSION_INFO(session, "fw is not ready!!!, pending roi num:%d",roi_config_num);
            session->port[MVX_DIR_INPUT].roi_config_queue[roi_config_num] = *roi;
            session->port[MVX_DIR_INPUT].roi_config_num++;
        } else {
            MVX_SESSION_ERR(session, "fw is not ready for long time, too many roi pending:%d",roi_config_num);
        }
        return 0;
    }
    ret = queue_roi_regions(session, roi);
    return 0;
}

int mvx_session_set_qp_epr(struct mvx_session *session,
			       int *qp)
{
    int ret = 0;
    int qp_num = 0;
    if (is_fw_loaded(session) == false ||
        session->port[MVX_DIR_INPUT].is_flushing != false) {
        qp_num = session->port[MVX_DIR_INPUT].qp_num;
        if (qp_num < MVX_ROI_QP_NUMS) {
            MVX_SESSION_WARN(session, "fw is not ready!!!, pending qp num:%d",qp_num);
            session->port[MVX_DIR_INPUT].qp_queue[qp_num] = *qp;
            session->port[MVX_DIR_INPUT].qp_num++;
        } else {
            MVX_SESSION_ERR(session, "fw is not ready for long time, too many qp pending:%d",qp_num);
        }
        return 0;
    }
    ret = queue_qp_epr(session, qp);
    return 0;
}

int mvx_session_set_sei_userdata(struct mvx_session *session,
			       struct mvx_sei_userdata *userdata)
{
    int ret = 0;
    if (session->error != 0)
        return session->error;

    session->sei_userdata = *userdata;
    if (is_fw_loaded(session) != false) {
        struct mvx_fw_set_option option;

        option.code = MVX_FW_SET_SEI_USERDATA;
        option.userdata = *userdata;
        ret = fw_set_option(session, &option);
        if (ret != 0)
            return ret;
    }
    return ret;
}

int mvx_session_set_hrd_buffer_size(struct mvx_session *session,
			      int size)
{
    int ret;

    if (session->error != 0)
        return session->error;

    session->nHRDBufsize = size;

    if (is_fw_loaded(session) != false) {
        struct mvx_fw_set_option option;

        option.code = MVX_FW_SET_HRD_BUF_SIZE;
        option.nHRDBufsize = size;
        ret = fw_set_option(session, &option);
        if (ret != 0)
            return ret;
    }
    return 0;
}

int mvx_session_set_dsl_frame(struct mvx_session *session,
			      struct mvx_dsl_frame *dsl)
{
    int ret;

    if (session->error != 0)
        return session->error;

    session->dsl_frame.width = dsl->width;
    session->dsl_frame.height = dsl->height;
    if (is_fw_loaded(session) != false) {
        struct mvx_fw_set_option option;

        option.code = MVX_FW_SET_DSL_FRAME;
        option.dsl_frame.width = dsl->width;
        option.dsl_frame.height = dsl->height;
        ret = fw_set_option(session, &option);
        if (ret != 0)
            return ret;
    }
    return 0;
}

int mvx_session_set_dsl_ratio(struct mvx_session *session,
			      struct mvx_dsl_ratio *dsl)
{
    if (session->error != 0)
        return session->error;

    session->dsl_ratio.hor = dsl->hor;
    session->dsl_ratio.ver = dsl->ver;
    return 0;
}

int mvx_session_set_long_term_ref(struct mvx_session *session,
			      struct mvx_long_term_ref *ltr)
{
    int ret;

    if (session->error != 0)
        return session->error;

    session->mvx_ltr.mode = ltr->mode;
    session->mvx_ltr.period = ltr->period;
    if (is_fw_loaded(session) != false) {
        struct mvx_fw_set_option option;

        option.code = MVX_FW_SET_LONG_TERM_REF;
        option.ltr.mode = ltr->mode;
        option.ltr.period = ltr->period;
        ret = fw_set_option(session, &option);
        if (ret != 0)
            return ret;
    }
    return 0;
}

int mvx_session_set_dsl_mode(struct mvx_session *session,
			       int *mode)
{
	if (session->error != 0)
		return session->error;

	if (is_fw_loaded(session) != false)
		return -EBUSY;

	session->dsl_pos_mode = *mode;

	return 0;
}
int mvx_session_set_force_idr(struct mvx_session *session)
{
	int ret;
	if (session->error != 0)
		return session->error;
	if (is_fw_loaded(session) != false) {
		struct mvx_fw_set_option option;
		/*reset GOP type to force idr frame.*/
		option.code = MVX_FW_SET_GOP_RESET;
		ret = fw_set_option(session, &option);
		if (ret != 0) {
			MVX_SESSION_WARN(session,
					 "Failed to  GOP reset.");
			return ret;
		}
	}
	return 0;
}

int mvx_session_set_watchdog_timeout(struct mvx_session *session, int timeout)
{
    if (session->error != 0)
        return session->error;

    if (is_fw_loaded(session) != false)
        return -EBUSY;

    session->watchdog_timeout = timeout*1000;

    return 0;
}

int mvx_session_set_profiling(struct mvx_session *session, int enable)
{
    if (session->error != 0)
        return session->error;

    if (is_fw_loaded(session) != false)
        return -EBUSY;

    session->enable_profiling = enable;

    return 0;
}
