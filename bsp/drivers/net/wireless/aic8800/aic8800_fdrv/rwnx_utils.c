/**
 * rwnx_utils.c
 *
 * IPC utility function definitions
 *
 * Copyright (C) RivieraWaves 2012-2019
 */
#include "rwnx_utils.h"
#include "rwnx_defs.h"
#include "rwnx_rx.h"
#include "rwnx_tx.h"
#include "rwnx_msg_rx.h"
#include "rwnx_debugfs.h"
#include "rwnx_prof.h"
#include "ipc_host.h"

int rwnx_init_aic(struct rwnx_hw *rwnx_hw)
{
	RWNX_DBG(RWNX_FN_ENTRY_STR);
#ifdef AICWF_SDIO_SUPPORT
	aicwf_sdio_host_init(&(rwnx_hw->sdio_env), NULL, NULL, rwnx_hw);
#else
	aicwf_usb_host_init(&(rwnx_hw->usb_env), NULL, NULL, rwnx_hw);
#endif
	rwnx_cmd_mgr_init(rwnx_hw->cmd_mgr);

	return 0;
}

