#ifndef __BM_CMD_H
#define __BM_CMD_H

#include <linux/ioctl.h>
#define IOC_MAGIC 'k' /* defines the magic number */

#define BM_DRD_SET_A_IDLE		_IO(IOC_MAGIC, 0)
#define BM_DRD_SET_B_SRP_INIT		_IO(IOC_MAGIC, 1)
#define BM_DRD_GET_STATE		_IOR(IOC_MAGIC, 2, int)
#define BM_DRD_SET_HNP_REQ		_IO(IOC_MAGIC, 3)
#define BM_DRD_STB_ALLOWED		_IOR(IOC_MAGIC, 4, int)
#define BM_DRD_SET_STB		_IOR(IOC_MAGIC, 5, int)
#define BM_DRD_CLEAR_STB		_IOR(IOC_MAGIC, 6, int)

#define BM_DEV_STB_ALLOWED		_IOR(IOC_MAGIC, 7, int)
#define BM_DEV_SET_STB		_IOR(IOC_MAGIC, 8, int)
#define BM_DEV_CLEAR_STB		_IOR(IOC_MAGIC, 9, int)

#endif /* __BM_CMD_H */
