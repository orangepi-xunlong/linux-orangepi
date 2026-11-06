#ifndef _VO_PROC_H_
#define _VO_PROC_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <vo_defines.h>
#include "vo_ctx.h"

int vo_proc_init(struct vo_ctx *ctx);
int vo_proc_remove(void);

#ifdef __cplusplus
}
#endif

#endif // _VO_PROC_H_
