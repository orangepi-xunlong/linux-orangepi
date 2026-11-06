#include "rtos_cmdqu.h"
#include <base_cb.h>
#include "cmdqu_cb.h"

int rtos_cmdqu_cb(void *dev, enum enum_modules_id caller, u32 cmd, void *arg)
{
	struct cvi_rtos_cmdqu_device *ndev = (struct cvi_rtos_cmdqu_device *)dev;
	int rc = -1;

	(void)ndev;
	(void)arg;

	if (caller == E_MODULE_RGN) {
		switch (cmd) {
		case CMDQU_CB_RGN_COMPRESS:
		case CMDQU_CB_RGN_GET_COMPRESS_SIZE:
		{
			cmdqu_t stRtosCmdqu;
			struct cmdqu_rgn_cb_param *rgn_cb_param;

			rgn_cb_param = (struct cmdqu_rgn_cb_param *)arg;
			stRtosCmdqu.ip_id = IP_RGN;
			stRtosCmdqu.cmd_id = rgn_cb_param->u8CmdId;
			stRtosCmdqu.block = 1;
			stRtosCmdqu.param_ptr = rgn_cb_param->u32RgnCmprParamPAddr;
			stRtosCmdqu.resv.mstime = 1000;
			rc = rtos_cmdqu_send_wait(&stRtosCmdqu, stRtosCmdqu.cmd_id);

			break;
		}

		default:
			break;
		}
	}

	return rc;
}

