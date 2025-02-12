/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_X1_PLAT_CAM_H
#define _UAPI_X1_PLAT_CAM_H

#define PLAT_CAM_NAME "platform-cam"

/*
 * Base number ranges for x1 entity functions
 * belong to either MEDIA_ENT_F_OLD_BASE or MEDIA_ENT_F_TUNER
 * ranges
 */
#define MEDIA_ENT_F_X1_BASE		0x00010000

#define MEDIA_ENT_F_X1_VI		(MEDIA_ENT_F_X1_BASE + 0x1001)
#define MEDIA_ENT_F_X1_CPP		(MEDIA_ENT_F_X1_BASE + 0x1002)
#define MEDIA_ENT_F_X1_VBE		(MEDIA_ENT_F_X1_BASE + 0x1003)
#define MEDIA_ENT_F_X1_SENSOR	(MEDIA_ENT_F_X1_BASE + 0x1004)

#endif /* _UAPI_X1_PLAT_CAM_H */
