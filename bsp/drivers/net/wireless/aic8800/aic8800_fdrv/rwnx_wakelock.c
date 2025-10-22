#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/platform_device.h>
#include <linux/pm_wakeirq.h>
#include "rwnx_defs.h"
#include "rwnx_wakelock.h"

struct wakeup_source *rwnx_wakeup_init(const char *name)
{
	struct wakeup_source *ws;
	ws = wakeup_source_create(name);
	wakeup_source_add(ws);
	return ws;
}

void rwnx_wakeup_deinit(struct wakeup_source *ws)
{
	if (ws && ws->active)
		__pm_relax(ws);
	wakeup_source_remove(ws);
	wakeup_source_destroy(ws);
}

struct wakeup_source *rwnx_wakeup_register(struct device *dev, const char *name)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
	return wakeup_source_register(dev, name);
#else
	return wakeup_source_register(name);
#endif
}

void rwnx_wakeup_unregister(struct wakeup_source *ws)
{
	if (ws && ws->active)
		__pm_relax(ws);
	wakeup_source_unregister(ws);
}

void rwnx_wakeup_lock(struct wakeup_source *ws)
{
	__pm_stay_awake(ws);
}

void rwnx_wakeup_unlock(struct wakeup_source *ws)
{
	__pm_relax(ws);
}

void rwnx_wakeup_lock_timeout(struct wakeup_source *ws, unsigned int msec)
{
	__pm_wakeup_event(ws, msec);
}

int aicwf_wakeup_lock_status(struct rwnx_hw *rwnx_hw)
{
	if (rwnx_hw->ws_tx && rwnx_hw->ws_tx->active)
		return -1;
	if (rwnx_hw->ws_rx && rwnx_hw->ws_rx->active)
		return -1;
	if (rwnx_hw->ws_pwrctrl && rwnx_hw->ws_pwrctrl->active)
		return -1;
	if (rwnx_hw->ws_irqrx && rwnx_hw->ws_irqrx->active)
		return -1;
	return 0;
}

void aicwf_wakeup_lock_init(struct rwnx_hw *rwnx_hw)
{
	rwnx_hw->ws_tx = rwnx_wakeup_init("rwnx_tx_wakelock");
	rwnx_hw->ws_rx = rwnx_wakeup_init("rwnx_rx_wakelock");
	rwnx_hw->ws_irqrx = rwnx_wakeup_init("rwnx_irqrx_wakelock");
	rwnx_hw->ws_pwrctrl = rwnx_wakeup_init("rwnx_pwrcrl_wakelock");
}

void aicwf_wakeup_lock_deinit(struct rwnx_hw *rwnx_hw)
{
	rwnx_wakeup_deinit(rwnx_hw->ws_tx);
	rwnx_wakeup_deinit(rwnx_hw->ws_rx);
	rwnx_wakeup_deinit(rwnx_hw->ws_irqrx);
	rwnx_wakeup_deinit(rwnx_hw->ws_pwrctrl);
}

