#ifndef __CMDQU_CB_H__
#define __CMDQU_CB_H__

#ifdef __cplusplus
	extern "C" {
#endif

enum CMDQU_CB_CMD {
	CMDQU_CB_RGN_COMPRESS,
	CMDQU_CB_RGN_GET_COMPRESS_SIZE,
	CMDQU_CB_RGN_COMPRESS_DONE,
	CMDQU_CB_MAX
};

struct cmdqu_rgn_cb_param {
	unsigned int u32RgnCmprParamPAddr;
	unsigned char u8CmdId;
	bool bStatus;
};

#ifdef __cplusplus
}
#endif

#endif /* __CMDQU_CB_H__ */

