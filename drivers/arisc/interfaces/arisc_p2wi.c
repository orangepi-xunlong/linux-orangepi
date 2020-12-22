/*
 *  drivers/arisc/interfaces/arisc_p2wi.c
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

#if defined CONFIG_ARCH_SUN8IW1P1
/**
 * p2wi read block data.
 * @cfg:    point of arisc_p2wi_block_cfg struct;
 *
 * return: result, 0 - read register successed,
 *                !0 - read register failed or the len more then max len;
 */
int arisc_p2wi_read_block_data(struct arisc_p2wi_block_cfg *cfg)
{
	int                   i;
	int                   result;
	struct arisc_message *pmessage;

	if ((cfg == NULL) || (cfg->addr == NULL) || (cfg->data == NULL) || (cfg->len > P2WI_TRANS_BYTE_MAX) ||
		((cfg->msgattr !=  ARISC_MESSAGE_ATTR_HARDSYN) && (cfg->msgattr !=  ARISC_MESSAGE_ATTR_SOFTSYN))) {
		ARISC_WRN("p2wi read reg paras error\n");
		return -EINVAL;
	}

	pmessage = arisc_message_allocate(cfg->msgattr);

	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}

	/* initialize message */
	pmessage->type       = ARISC_P2WI_READ_BLOCK_DATA;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;

	/*
	 * package address and data to message->paras,
	 * message->paras data layout:
	 * |para[0]|para[1]|para[2]|para[3]|para[4]|
	 * |len    |addr0~3|addr4~7|data0~3|data4~7|
	 */
	pmessage->paras[0] = 0;
	pmessage->paras[1] = 0;
	pmessage->paras[2] = 0;
	pmessage->paras[3] = 0;
	pmessage->paras[4] = 0;
	//memset(pmessage->paras, 0, sizeof(pmessage->paras));
	//memset(pmessage->paras, 0, sizeof(unsigned int) * 4);
	pmessage->paras[0] = cfg->len;
	for (i = 0; i < cfg->len; i++) {
		if (i < 4) {
			/* pack 8bit addr0~addr3 into 32bit paras[0] */
			pmessage->paras[1] |= (cfg->addr[i] << (i * 8));
		} else {
			/* pack 8bit addr4~addr7 into 32bit paras[1] */
			pmessage->paras[2] |= (cfg->addr[i] << ((i - 4) * 8));
		}
	}

	/* send message use hwmsgbox */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	/* copy message readout data to user data buffer */
	for (i = 0; i < cfg->len; i++) {
		if (i < 4) {
			cfg->data[i] = ((pmessage->paras[3]) >> (i * 8)) & 0xff;
		} else {
			cfg->data[i] = ((pmessage->paras[4]) >> ((i - 4) * 8)) & 0xff;
		}
	}

	/* free message */
	result = pmessage->result;
	arisc_message_free(pmessage);

	return result;
}
EXPORT_SYMBOL(arisc_p2wi_read_block_data);


/**
 * p2wi write block data.
 * @cfg:    point of arisc_p2wi_block_cfg struct;
 *
 * return: result, 0 - write register successed,
 *                !0 - write register failedor the len more then max len;
 */
int arisc_p2wi_write_block_data(struct arisc_p2wi_block_cfg *cfg)
{
	int                   i;
	int                   result;
	struct arisc_message *pmessage;

	if ((cfg == NULL) || (cfg->addr == NULL) || (cfg->data == NULL) || (cfg->len > P2WI_TRANS_BYTE_MAX) ||
		((cfg->msgattr !=  ARISC_MESSAGE_ATTR_HARDSYN) && (cfg->msgattr !=  ARISC_MESSAGE_ATTR_SOFTSYN))) {
		ARISC_WRN("p2wi write reg paras error\n");
		return -EINVAL;
	}

	pmessage = arisc_message_allocate(cfg->msgattr);

	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}
	/* initialize message */
	pmessage->type       = ARISC_P2WI_WRITE_BLOCK_DATA;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;

	/*
	 * package address and data to message->paras,
	 * message->paras data layout:
	 * |para[0]|para[1]|para[2]|para[3]|para[4]|
	 * |len    |addr0~3|addr4~7|data0~3|data4~7|
	 */
	pmessage->paras[0] = 0;
	pmessage->paras[1] = 0;
	pmessage->paras[2] = 0;
	pmessage->paras[3] = 0;
	pmessage->paras[4] = 0;
	//memset(pmessage->paras, 0, sizeof(pmessage->paras));
	//memset(pmessage->paras, 0, sizeof(unsigned int) * 4);
	pmessage->paras[0] = cfg->len;
	for (i = 0; i < cfg->len; i++) {
		if (i < 4) {
			/* pack 8bit addr0~addr3 into 32bit paras[0] */
			pmessage->paras[1] |= (cfg->addr[i] << (i * 8));

			/* pack 8bit data0~data3 into 32bit paras[2] */
			pmessage->paras[3] |= (cfg->data[i] << (i * 8));
		} else {
			/* pack 8bit addr4~addr7 into 32bit paras[1] */
			pmessage->paras[2] |= (cfg->addr[i] << ((i - 4) * 8));

			/* pack 8bit data4~data7 into 32bit paras[3] */
			pmessage->paras[4] |= (cfg->data[i] << ((i - 4) * 8));
		}
	}
	/* send message use hwmsgbox */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	/* free message */
	result = pmessage->result;
	arisc_message_free(pmessage);

	return result;
}
EXPORT_SYMBOL(arisc_p2wi_write_block_data);

/**
 * p2wi bits operation sync.
 * @cfg:    point of arisc_p2wi_bits_cfg struct;
 *
 * return: result, 0 - bits operation successed,
 *                !0 - bits operation failed, or the len more then max len;
 *
 * p2wi clear bits internal:
 * data = p2wi_read(addr);
 * data = data & (~mask);
 * p2wi_write(addr, data);
 *
 * p2wi set bits internal:
 * data = p2wi_read(addr);
 * data = data | mask;
 * p2wi_write(addr, data);
 *
 */
int p2wi_bits_ops_sync(struct arisc_p2wi_bits_cfg *cfg)
{
	int                   i;
	int                   result;
	struct arisc_message *pmessage;

	if ((cfg == NULL) || (cfg->addr == NULL) || (cfg->mask == NULL) || (cfg->delay == NULL) || (cfg->len > P2WI_TRANS_BYTE_MAX) ||
		((cfg->msgattr !=  ARISC_MESSAGE_ATTR_HARDSYN) && (cfg->msgattr !=  ARISC_MESSAGE_ATTR_SOFTSYN))) {
		ARISC_WRN("p2wi clear bits sync paras error\n");
		return -EINVAL;
	}

	pmessage = arisc_message_allocate(cfg->msgattr);

	if (pmessage == NULL) {
		ARISC_WRN("allocate message failed\n");
		return -ENOMEM;
	}
	/* initialize message */
	pmessage->type       = ARISC_P2WI_BITS_OPS_SYNC;
	pmessage->state      = ARISC_MESSAGE_INITIALIZED;
	pmessage->cb.handler = NULL;
	pmessage->cb.arg     = NULL;

	/*
	 * package address and data to message->paras,
	 * message->paras data layout:
	 * |para[0]|para[1]|para[2]|para[3]|para[4]|para[5] |para[6] |para[7]|
	 * |len    |addr0~3|addr4~7|mask0~3|mask4~7|delay0~3|delay4~7|ops    |
	 */
	pmessage->paras[0] = 0;
	pmessage->paras[1] = 0;
	pmessage->paras[2] = 0;
	pmessage->paras[3] = 0;
	pmessage->paras[4] = 0;
	pmessage->paras[5] = 0;
	pmessage->paras[6] = 0;
	pmessage->paras[7] = 0;
	//memset(pmessage->paras, 0, sizeof(pmessage->paras));
	//memset(pmessage->paras, 0, sizeof(unsigned int) * 4);
	pmessage->paras[0] = cfg->len;
	pmessage->paras[7] = cfg->ops;

	for (i = 0; i < cfg->len; i++) {
		if (i < 4) {
			/* pack 8bit addr0~addr3 into 32bit paras[0] */
			pmessage->paras[1] |= (cfg->addr[i] << (i * 8));

			/* pack 8bit mask0~mask3 into 32bit paras[2] */
			pmessage->paras[3] |= (cfg->mask[i] << (i * 8));

			/* pack 8bit delay0~delay3 into 32bit paras[3] */
			pmessage->paras[5] |= (cfg->delay[i] << (i * 8));
		} else {
			/* pack 8bit addr4~addr7 into 32bit paras[1] */
			pmessage->paras[2] |= (cfg->addr[i] << ((i - 4) * 8));

			/* pack 8bit mask4~mask7 into 32bit paras[3] */
			pmessage->paras[4] |= (cfg->mask[i] << ((i - 4) * 8));

			/* pack 8bit delay4~delay7 into 32bit paras[5] */
			pmessage->paras[6] |= (cfg->delay[i] << ((i - 4) * 8));
		}
	}
	/* send message use hwmsgbox */
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	/* free message */
	result = pmessage->result;
	arisc_message_free(pmessage);

	return result;
}
EXPORT_SYMBOL(p2wi_bits_ops_sync);
#endif
