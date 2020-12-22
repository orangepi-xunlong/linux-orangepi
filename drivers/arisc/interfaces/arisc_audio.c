/*
 *  drivers/arisc/interfaces/arisc_audio.c
 *
 * Copyright (c) 2013 Allwinner.
 * 2013-07-01 Written by superm (superm@allwinnertech.com).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "../arisc_i.h"

static audio_cb_t audio_cb_node[2];

/**
 * start audio play or capture.
 * @mode:    start audio in which mode ; 0:play, 1;capture.
 *
 * return: result, 0 - start audio play or capture successed,
 *                !0 - start audio play or capture failed.
 */
int arisc_audio_start(int mode)
{
	int                   ret;
	int                   result;
	struct arisc_message *pmessage;

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/*
	 * |para[0]   |
	 * |mode      |
	 */
	/* initialize message */
	pmessage->type       = ARISC_AUDIO_START;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;
	pmessage->paras[0]   = mode;

	/* send message use hwmsgbox */
	ret = arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);
	if (ret == 0) {
		result = pmessage->result;
	} else {
		result = ret;
	}

	/* free message */
	arisc_message_free(pmessage);

	return result;
}
EXPORT_SYMBOL(arisc_audio_start);

/**
 * stop audio play or capture.
 * @mode:    stop audio in which mode ; 0:play, 1;capture.
 *
 * return: result, 0 - stop audio play or capture successed,
 *                !0 - stop audio play or capture failed.
 */
int arisc_audio_stop(int mode)
{
	int                   ret;
	int                   result;
	struct arisc_message *pmessage;

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/*
	 * |para[0]   |
	 * |mode      |
	 */
	/* initialize message */
	pmessage->type       = ARISC_AUDIO_STOP;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;
	pmessage->paras[0]   = mode;

	/* send message use hwmsgbox */
	ret = arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);
	if (ret == 0) {
		result = pmessage->result;
	} else {
		result = ret;
	}

	/* free message */
	arisc_message_free(pmessage);

	return result;
}
EXPORT_SYMBOL(arisc_audio_stop);

/**
 * set audio buffer and period parameters.
 * @audio_mem:
 *             mode          :which mode be set; 0:paly, 1:capture;
 *             sram_base_addr:sram base addr of buffer;
 *             buffer_size   :the size of buffer;
 *             period_size   :the size of period;
 *
 * |period|period|period|period|...|period|period|period|period|...|
 * | paly                   buffer | capture                buffer |
 * |                               |
 * 1                               2
 * 1:paly sram_base_addr,          2:capture sram_base_addr;
 * buffer size = capture sram_base_addr - paly sram_base_addr.
 *
 * return: result, 0 - set buffer and period parameters successed,
 *                !0 - set buffer and period parameters failed.
 *
 */
int arisc_buffer_period_paras(struct arisc_audio_mem audio_mem)
{
	int                   ret;
	int                   result;
	struct arisc_message *pmessage;

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/*
	 * |para[0]   |para[1]       |para[2]    |para[3]    |
	 * |mode      |sram_base_addr|buffer_size|period_size|
	 */
	/* initialize message */
	pmessage->type       = ARISC_AUDIO_SET_BUF_PER_PARAS;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;
	pmessage->paras[0]   = audio_mem.mode;
	pmessage->paras[1]   = audio_mem.sram_base_addr;
	pmessage->paras[2]   = audio_mem.buffer_size;
	pmessage->paras[3]   = audio_mem.period_size;

	/* send message use hwmsgbox */
	ret = arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);
	if (ret == 0) {
		result = pmessage->result;
	} else {
		result = ret;
	}

	/* free message */
	arisc_message_free(pmessage);

	return result;
}
EXPORT_SYMBOL(arisc_buffer_period_paras);

/**
 * get audio play or capture real-time address.
 * @mode:    in which mode; 0:play, 1;capture;
 * @addr:    real-time address in which mode.
 *
 * return: result, 0 - get real-time address successed,
 *                !0 - get real-time address failed.
 */
int arisc_get_position(int mode, unsigned int *addr)
{
	int                   ret;
	int                   result;
	struct arisc_message *pmessage;

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/*
	 * |para[0]   |para[1]       |
	 * |mode      |psrc / pdst   |
	 */
	/* initialize message */
	pmessage->type       = ARISC_AUDIO_GET_POSITION;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;
	pmessage->paras[0]   = mode;

	/* send message use hwmsgbox */
	ret = arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);
	if (ret == 0) {
		result = pmessage->result;
	} else {
		result = ret;
	}

	if (!result) {
		*addr = pmessage->paras[1];
	}

	/* free message */
	arisc_message_free(pmessage);

	return result;
}
EXPORT_SYMBOL(arisc_get_position);

/**
 * register audio callback function.
 * @mode:    in which mode; 0:play, 1;capture;
 * @handler: audio callback handler which need be register;
 * @arg    : the pointer of audio callback arguments.
 *
 * return: result, 0 - register audio callback function successed,
 *                !0 - register audio callback function failed.
 */
int arisc_audio_cb_register(int mode, arisc_cb_t handler, void *arg)
{
	if (audio_cb_node[mode].handler) {
		if(handler == audio_cb_node[mode].handler) {
			ARISC_WRN("audio period done callback register already\n");
			return 0;
		}
		/* just output warning message, overlay handler */
		ARISC_WRN("audio period done callback register already\n");
		return -EINVAL;
	}
	audio_cb_node[mode].handler = handler;
	audio_cb_node[mode].arg     = arg;

	return 0;
}
EXPORT_SYMBOL(arisc_audio_cb_register);

/**
 * unregister audio callback function.
 * @mode:    in which mode; 0:play, 1;capture;
 * @handler: audio callback handler which need be register;
 * @arg    : the pointer of audio callback arguments.
 *
 * return: result, 0 - unregister audio callback function successed,
 *                !0 - unregister audio callback function failed.
 */
int arisc_audio_cb_unregister(int mode, arisc_cb_t handler)
{
	if ((u32)(audio_cb_node[mode].handler) != (u32)(handler)) {
		/* invalid handler */
		ARISC_WRN("invalid handler for unreg\n\n");
		return -EINVAL;
	}
	audio_cb_node[mode].handler = NULL;
	audio_cb_node[mode].arg     = NULL;

	return 0;
}
EXPORT_SYMBOL(arisc_audio_cb_unregister);

int arisc_audio_perdone_notify(struct arisc_message *pmessage)
{
	/*
	 * |para[0]   |
	 * |mode      |
	 */
	u32 mode = pmessage->paras[0];

	/* call audio cb handler */
	if (audio_cb_node[mode].handler == NULL) {
		ARISC_WRN("audio perdone callback handler not install\n");
		return -EINVAL;
	}
	return (*(audio_cb_node[mode].handler))(audio_cb_node[mode].arg);
}

/**
 * set audio tdm parameters.
 * @tdm_cfg: audio tdm struct
 *           mode      :in which mode; 0:play, 1;capture;
 *           samplerate:tdm samplerate depend on audio data;
 *           channel   :audio channel number, 1 or 2.

 * return: result, 0 - set buffer and period parameters successed,
 *                !0 - set buffer and period parameters failed.
 *
 */
int arisc_tdm_paras(struct arisc_audio_tdm tdm_cfg)
{
	int                   ret;
	int                   result;
	struct arisc_message *pmessage;

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/*
	 * package address and data to message->paras,
	 * message->paras data layout:
	 * |para[0]   |para[1]       |para[2]    |
	 * |mode      |samplerate    |channel    |
	 */
	/* initialize message */
	pmessage->type       = ARISC_AUDIO_SET_TDM_PARAS;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;
	pmessage->paras[0]   = tdm_cfg.mode;
	pmessage->paras[1]   = tdm_cfg.samplerate;
	pmessage->paras[2]   = tdm_cfg.channum;

	/* send message use hwmsgbox */
	ret = arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);
	if (ret == 0) {
		result = pmessage->result;
	} else {
		result = ret;
	}

	/* free message */
	arisc_message_free(pmessage);

	return result;
}
EXPORT_SYMBOL(arisc_tdm_paras);

/**
 * add audio period.
 * @mode:    start audio in which mode ; 0:play, 1;capture.
 * @addr:    period address which will be add in buffer
 *
 * return: result, 0 - add audio period successed,
 *                !0 - add audio period failed.
 *
 */
int arisc_add_period(u32 mode, u32 addr)
{
	int                   ret;
	int                   result;
	struct arisc_message *pmessage;

	/* allocate a message frame */
	pmessage = arisc_message_allocate(ARISC_MESSAGE_ATTR_HARDSYN);
	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/*
	 * |para[0]|para[1]|
	 * |mode   |address|
	 */
	/* initialize message */
	pmessage->type       = ARISC_AUDIO_ADD_PERIOD;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;
	pmessage->paras[0]   = mode;
	pmessage->paras[1]   = addr;

	/* send message use hwmsgbox */
	ret = arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);
	if (ret == 0) {
		result = pmessage->result;
	} else {
		result = ret;
	}

	/* free message */
	arisc_message_free(pmessage);

	return result;
}
EXPORT_SYMBOL(arisc_add_period);
