#ifndef __VI_CB_H__
#define __VI_CB_H__

#ifdef __cplusplus
	extern "C" {
#endif
#include <ldc_cb.h>

enum VI_CB_CMD {
	VI_CB_QBUF_TRIGGER,
	VI_CB_SC_FRM_DONE,
	VI_CB_SET_VIVPSSMODE,
	VI_CB_GDC_OP_DONE = LDC_CB_GDC_OP_DONE,
	VI_CB_MAX
};

struct vi_vpss_online {
	__u8   raw_num;
	__u8   is_vpss_online;
};

#ifdef __cplusplus
}
#endif

#endif /* __VI_CB_H__ */