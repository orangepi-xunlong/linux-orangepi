#ifndef __CVI_CMD_H
#define __CVI_CMD_H

#include <linux/ioctl.h>
#define IOC_MAGIC 'k' /* defines the magic number */

#define CVI_DRD_SET_A_IDLE		_IO(IOC_MAGIC, 0)
#define CVI_DRD_SET_B_SRP_INIT		_IO(IOC_MAGIC, 1)
#define CVI_DRD_GET_STATE		_IOR(IOC_MAGIC, 2, int)
#define CVI_DRD_SET_HNP_REQ		_IO(IOC_MAGIC, 3)
#define CVI_DRD_STB_ALLOWED		_IOR(IOC_MAGIC, 4, int)
#define CVI_DRD_SET_STB		_IOR(IOC_MAGIC, 5, int)
#define CVI_DRD_CLEAR_STB		_IOR(IOC_MAGIC, 6, int)

#define CVI_DEV_STB_ALLOWED		_IOR(IOC_MAGIC, 7, int)
#define CVI_DEV_SET_STB		_IOR(IOC_MAGIC, 8, int)
#define CVI_DEV_CLEAR_STB		_IOR(IOC_MAGIC, 9, int)

#endif /* __CVI_CMD_H */
