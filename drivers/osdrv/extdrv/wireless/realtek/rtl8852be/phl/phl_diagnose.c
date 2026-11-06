/*
 * This module is designed to collect internal diagnostic information.
 *
 *	Author: Cosa
 *	History: Created at 2023/02/03
 */
#define _PHL_DIAGNOSE_C_
#include "phl_headers.h"

#ifdef CONFIG_PHL_DIAGNOSE

static void _diag_evt_done(void* priv, struct phl_msg* msg)
{
	struct phl_info_t *phl_info = (struct phl_info_t *)priv;

	if (msg->inbuf && msg->inlen) {
		_os_kmem_free(phl_to_drvpriv(phl_info),
			msg->inbuf, msg->inlen);
	}
}

bool rtw_phl_send_diag_hub_msg(struct rtw_phl_com_t *phl_com,
		u16 phl_evt, u8 sub_evt, u8 level, u8 ver, u8 *buf, u32 len)
{
	struct phl_msg msg = {0};
	struct phl_msg_attribute attr = {0};
	struct phl_info_t *phl = NULL;
	struct rtw_phl_diag_msg *dmsg = NULL;
	void *d = NULL;

	if (!phl_com || !buf)
		return false;

	phl = phl_com->phl_priv;

	if ((phl_evt >= PHL_DIAG_EVT_MAX) ||(level >= PHL_DIAG_LVL_MAX) ||
	    (len > MAX_PHL_DIAG_MSG_LEN) || !phl)  {
		return false;
	}

	d = phl_to_drvpriv(phl);
	if (!d)
		return false;

	dmsg = (struct rtw_phl_diag_msg *)_os_kmem_alloc(
			d, sizeof(struct rtw_phl_diag_msg));
	if (!dmsg)
		return false;

	SET_MSG_MDL_ID_FIELD(msg.msg_id, PHL_MDL_GENERAL);
	SET_MSG_EVT_ID_FIELD(msg.msg_id, MSG_EVT_DIAGNOSTIC);

	dmsg->type = phl_evt;
	dmsg->level = level;
	if (phl_evt == PHL_DIAG_EVT_MAC || phl_evt == PHL_DIAG_EVT_BB ||
	    phl_evt == PHL_DIAG_EVT_RF) {
		dmsg->sub_evt = sub_evt;
	} else {
		dmsg->sub_evt = INVALID_SUBMODULE_DIAG_EVT;
	}
	dmsg->ver = ver;
	dmsg->len = len;
	_os_mem_cpy(d, dmsg->buf, buf, len);

	msg.inbuf = (u8 *)dmsg;
	msg.inlen = sizeof(struct rtw_phl_diag_msg);
	attr.completion.completion = _diag_evt_done;
	attr.completion.priv = phl;

	if (phl_msg_hub_send(phl, &attr, &msg) != RTW_PHL_STATUS_SUCCESS) {
		_os_kmem_free(d, dmsg, sizeof(struct rtw_phl_diag_msg));
		return false;
	} else {
		return true;
	}
}

#endif

