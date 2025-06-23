// SPDX-License-Identifier: GPL-2.0
/*
 * (C) COPYRIGHT 2022-2023 Arm Technology (China) Co., Ltd.
 * ALL RIGHTS RESERVED
 *
 */

#ifndef _LINLON_AEU_DEV_H_
#define _LINLON_AEU_DEV_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mem2mem.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include "linlon_aeu_hw.h"

#define AEU_NAME    "linlon-aeu"

enum aeu_device_status {
    AEU_ACTIVE = 0,
    AEU_PAUSED,
};

struct linlon_aeu_device {
    struct device            *dev;

    struct v4l2_device        v4l2_dev;
    struct video_device        vdev;
    struct v4l2_m2m_dev        *m2mdev;
    struct iommu_domain        *iommu;
    enum aeu_device_status        status;
    struct device_dma_parameters    dma_parms;
    /* protect access in different instance */
    struct mutex            aeu_mutex;

    struct linlon_aeu_hw_device    *hw_dev;
    struct linlon_aeu_hw_info        hw_info;

    struct dentry            *dbg_folder;

#if defined(CONFIG_PM_SLEEP)
    struct notifier_block aeu_pm_nb;
#endif
};

int linlon_aeu_device_init(struct linlon_aeu_device *adev,
             struct platform_device *pdev, struct dentry *parent);
int linlon_aeu_device_destroy(struct linlon_aeu_device *adev);
void linlon_aeu_paused(struct linlon_aeu_device *adev);
void linlon_aeu_resume(struct linlon_aeu_device *adev);
#endif
