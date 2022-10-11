/*
 * Copyright (C) Fuzhou Rockchip Electronics Co.Ltd
 * Author: Yakir Yang <ykk@rock-chips.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <drm/drmP.h>
#include <drm/drm_crtc_helper.h>

#include "rockchip_drm_drv.h"
#include "rockchip_drm_psr.h"

#define PSR_FLUSH_TIMEOUT	msecs_to_jiffies(3000) /* 3 seconds */

static LIST_HEAD(psr_list);
static DEFINE_MUTEX(psr_list_mutex);

enum psr_state {
	PSR_FLUSH,
	PSR_ENABLE,
	PSR_DISABLE,
};

struct psr_drv {
	struct list_head list;
	enum psr_state state;
	struct mutex state_mutex;

	struct timer_list flush_timer;

	struct drm_encoder *encoder;
	void (*set)(struct drm_encoder *encoder, bool enable);
};

static struct psr_drv *find_psr_by_crtc(struct drm_crtc *crtc)
{
	struct rockchip_drm_private *drm_drv = crtc->dev->dev_private;
	struct psr_drv *psr;

	mutex_lock(&drm_drv->psr_list_mutex);
	list_for_each_entry(psr, &drm_drv->psr_list, list) {
		if (psr->encoder->crtc == crtc) {
			mutex_unlock(&drm_drv->psr_list_mutex);
			return psr;
		}
	}
	mutex_unlock(&drm_drv->psr_list_mutex);

	return ERR_PTR(-ENODEV);
}

static void psr_set_state(struct psr_drv *psr, enum psr_state state)
{
	mutex_lock(&psr->state_mutex);

	if (psr->state == state) {
		mutex_unlock(&psr->state_mutex);
		return;
	}

	psr->state = state;
	switch (state) {
	case PSR_ENABLE:
		psr->set(psr->encoder, true);
		break;

	case PSR_DISABLE:
	case PSR_FLUSH:
		psr->set(psr->encoder, false);
		break;
	};

	mutex_unlock(&psr->state_mutex);
}

static void psr_flush_handler(unsigned long data)
{
	struct psr_drv *psr = (struct psr_drv *)data;

	if (!psr || psr->state != PSR_FLUSH)
		return;

	psr_set_state(psr, PSR_ENABLE);
}

/**
 * rockchip_drm_psr_enable - enable the encoder PSR which bind to given CRTC
 * @crtc: CRTC to obtain the PSR encoder
 *
 * Returns:
 * Zero on success, negative errno on failure.
 */
int rockchip_drm_psr_enable(struct drm_crtc *crtc)
{
	struct psr_drv *psr = find_psr_by_crtc(crtc);

	if (IS_ERR(psr))
		return PTR_ERR(psr);

	psr_set_state(psr, PSR_ENABLE);
	return 0;
}
EXPORT_SYMBOL(rockchip_drm_psr_enable);

/**
 * rockchip_drm_psr_disable - disable the encoder PSR which bind to given CRTC
 * @crtc: CRTC to obtain the PSR encoder
 *
 * Returns:
 * Zero on success, negative errno on failure.
 */
int rockchip_drm_psr_disable(struct drm_crtc *crtc)
{
	struct psr_drv *psr = find_psr_by_crtc(crtc);

	if (IS_ERR(psr))
		return PTR_ERR(psr);

	psr_set_state(psr, PSR_DISABLE);
	return 0;
}
EXPORT_SYMBOL(rockchip_drm_psr_disable);

/**
 * rockchip_drm_psr_flush - force to flush all registered PSR encoders
 * @dev: drm device
 *
 * Disable the PSR function for all registered encoders, and then enable the
 * PSR function back after PSR_FLUSH_TIMEOUT. If encoder PSR state have been
 * changed during flush time, then keep the state no change after flush
 * timeout.
 *
 * Returns:
 * Zero on success, negative errno on failure.
 */
void rockchip_drm_psr_flush(struct drm_device *dev)
{
	struct rockchip_drm_private *drm_drv = dev->dev_private;
	struct psr_drv *psr;

	mutex_lock(&drm_drv->psr_list_mutex);
	list_for_each_entry(psr, &drm_drv->psr_list, list) {
		if (psr->state == PSR_DISABLE)
			continue;

		mod_timer(&psr->flush_timer,
			  round_jiffies_up(jiffies + PSR_FLUSH_TIMEOUT));

		psr_set_state(psr, PSR_FLUSH);
	}
	mutex_unlock(&drm_drv->psr_list_mutex);
}
EXPORT_SYMBOL(rockchip_drm_psr_flush);

/**
 * rockchip_drm_psr_register - register encoder to psr driver
 * @encoder: encoder that obtain the PSR function
 * @psr_set: call back to set PSR state
 *
 * Returns:
 * Zero on success, negative errno on failure.
 */
int rockchip_drm_psr_register(struct drm_encoder *encoder,
			void (*psr_set)(struct drm_encoder *, bool enable))
{
	struct rockchip_drm_private *drm_drv = encoder->dev->dev_private;
	struct psr_drv *psr;

	if (!encoder || !psr_set)
		return -EINVAL;

	psr = kzalloc(sizeof(struct psr_drv), GFP_KERNEL);
	if (!psr)
		return -ENOMEM;

	setup_timer(&psr->flush_timer, psr_flush_handler, (unsigned long)psr);

	mutex_init(&psr->state_mutex);

	psr->state = PSR_DISABLE;
	psr->encoder = encoder;
	psr->set = psr_set;

	mutex_lock(&drm_drv->psr_list_mutex);
	list_add_tail(&psr->list, &drm_drv->psr_list);
	mutex_unlock(&drm_drv->psr_list_mutex);

	return 0;
}
EXPORT_SYMBOL(rockchip_drm_psr_register);

/**
 * rockchip_drm_psr_unregister - unregister encoder to psr driver
 * @encoder: encoder that obtain the PSR function
 * @psr_set: call back to set PSR state
 *
 * Returns:
 * Zero on success, negative errno on failure.
 */
void rockchip_drm_psr_unregister(struct drm_encoder *encoder)
{
	struct rockchip_drm_private *drm_drv = encoder->dev->dev_private;
	struct psr_drv *psr;

	mutex_lock(&drm_drv->psr_list_mutex);
	list_for_each_entry(psr, &drm_drv->psr_list, list) {
		if (psr->encoder == encoder) {
			del_timer(&psr->flush_timer);
			list_del(&psr->list);
			kfree(psr);
		}
	}
	mutex_unlock(&drm_drv->psr_list_mutex);
}
EXPORT_SYMBOL(rockchip_drm_psr_unregister);
