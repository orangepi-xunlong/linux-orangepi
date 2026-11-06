#ifndef _VI_PROC_H_
#define _VI_PROC_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <vi_defines.h>

int vi_proc_init(struct sop_vi_dev *_vdev, void *shm);
int vi_proc_remove(void);

#ifdef __cplusplus
}
#endif

#endif // _VI_PROC_H_
