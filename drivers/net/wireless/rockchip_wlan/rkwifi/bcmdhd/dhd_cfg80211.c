/*
 * Linux cfg80211 driver - Dongle Host Driver (DHD) related
 *
 * Copyright (C) 2022, Broadcom.
 *
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 *
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 *
 *
 * <<Broadcom-WL-IPTag/Open:>>
 *
 * $Id$
 */

#include <linux/vmalloc.h>
#include <net/rtnetlink.h>

#include <bcmutils.h>
#include <wldev_common.h>
#include <wl_cfg80211.h>
#include <dhd_cfg80211.h>

#ifdef PKT_FILTER_SUPPORT
#include <dngl_stats.h>
#include <dhd.h>
#endif

#ifdef PKT_FILTER_SUPPORT
extern uint dhd_pkt_filter_enable;
extern uint dhd_master_mode;
extern void dhd_pktfilter_offload_enable(dhd_pub_t * dhd, char *arg, int enable, int master_mode);
#endif

static int dhd_dongle_up = FALSE;
#define PKT_FILTER_BUF_SIZE 64

#if defined(BCMDONGLEHOST)
#include <dngl_stats.h>
#include <dhd.h>
#include <dhdioctl.h>
#include <wlioctl.h>
#include <brcm_nl80211.h>
#include <dhd_cfg80211.h>
#endif /* defined(BCMDONGLEHOST) */

static s32 wl_dongle_up(struct net_device *ndev);
static s32 wl_dongle_down(struct net_device *ndev);

/**
 * Function implementations
 */

s32 dhd_cfg80211_init(struct bcm_cfg80211 *cfg)
{
	dhd_dongle_up = FALSE;
	return 0;
}

s32 dhd_cfg80211_deinit(struct bcm_cfg80211 *cfg)
{
	dhd_dongle_up = FALSE;
	return 0;
}

s32 dhd_cfg80211_down(struct bcm_cfg80211 *cfg)
{
	struct net_device *ndev;
	s32 err = 0;

	WL_TRACE(("In\n"));
	if (!dhd_dongle_up) {
		WL_INFORM_MEM(("Dongle is already down\n"));
		err = 0;
		goto done;
	}
	ndev = bcmcfg_to_prmry_ndev(cfg);
	wl_dongle_down(ndev);
done:
	return err;
}

s32 dhd_cfg80211_set_p2p_info(struct bcm_cfg80211 *cfg, int val)
{
	dhd_pub_t *dhd =  (dhd_pub_t *)(cfg->pub);
	dhd->op_mode |= val;
	WL_ERR(("Set : op_mode=0x%04x\n", dhd->op_mode));

	return 0;
}

s32 dhd_cfg80211_clean_p2p_info(struct bcm_cfg80211 *cfg)
{
	dhd_pub_t *dhd =  (dhd_pub_t *)(cfg->pub);
	dhd->op_mode &= ~(DHD_FLAG_P2P_GC_MODE | DHD_FLAG_P2P_GO_MODE);
	WL_ERR(("Clean : op_mode=0x%04x\n", dhd->op_mode));

	return 0;
}
#ifdef WL_STATIC_IF
int32
wl_cfg80211_update_iflist_info(struct bcm_cfg80211 *cfg, struct net_device *ndev,
	int ifidx, uint8 *addr, int bssidx, char *name, int if_state)
{
		return dhd_update_iflist_info(cfg->pub, ndev, ifidx, addr, bssidx, name, if_state);
}
#endif /* WL_STATIC_IF */
struct net_device* wl_cfg80211_allocate_if(struct bcm_cfg80211 *cfg, int ifidx, const char *name,
	uint8 *mac, uint8 bssidx, const char *dngl_name)
{
	return dhd_allocate_if(cfg->pub, ifidx, name, mac, bssidx, FALSE, dngl_name);
}

int wl_cfg80211_register_if(struct bcm_cfg80211 *cfg,
	int ifidx, struct net_device* ndev, bool rtnl_lock_reqd)
{
	return dhd_register_if(cfg->pub, ifidx, rtnl_lock_reqd);
}

int wl_cfg80211_remove_if(struct bcm_cfg80211 *cfg,
	int ifidx, struct net_device* ndev, bool rtnl_lock_reqd)
{
	return dhd_remove_if(cfg->pub, ifidx, rtnl_lock_reqd);
}

void wl_cfg80211_cleanup_if(struct net_device *net)
{
	struct bcm_cfg80211 *cfg = wl_get_cfg(net);
	BCM_REFERENCE(cfg);
	dhd_cleanup_if(net);
}

struct net_device * dhd_cfg80211_netdev_free(struct net_device *ndev)
{
	struct bcm_cfg80211 *cfg;

	if (ndev) {
		cfg = wl_get_cfg(ndev);
		if (ndev->ieee80211_ptr) {
			MFREE(cfg->osh, ndev->ieee80211_ptr, sizeof(struct wireless_dev));
			ndev->ieee80211_ptr = NULL;
		}
		free_netdev(ndev);
		return NULL;
	}

	return ndev;
}

void dhd_netdev_free(struct net_device *ndev)
{
#ifdef WL_CFG80211
	ndev = dhd_cfg80211_netdev_free(ndev);
#endif
	if (ndev)
		free_netdev(ndev);
}

static s32
wl_dongle_up(struct net_device *ndev)
{
	s32 err = 0;
	u32 local_up = 0;
#ifdef WLAN_ACCEL_BOOT
	u32 bus_host_access = 1;
	err = wldev_iovar_setint(ndev, "bus:host_access", bus_host_access);
	if (unlikely(err)) {
		WL_ERR(("bus:host_access(%d) error (%d)\n", bus_host_access, err));
	}
#endif /* WLAN_ACCEL_BOOT */
	err = wldev_ioctl_set(ndev, WLC_UP, &local_up, sizeof(local_up));
	if (unlikely(err)) {
		WL_ERR(("WLC_UP error (%d)\n", err));
	} else {
		WL_INFORM_MEM(("wl up\n"));
		dhd_dongle_up = TRUE;
	}
	return err;
}

static s32
wl_dongle_down(struct net_device *ndev)
{
	s32 err = 0;
	u32 local_down = 0;
#ifdef WLAN_ACCEL_BOOT
	u32 bus_host_access = 0;
#endif /* WLAN_ACCEL_BOOT */

	err = wldev_ioctl_set(ndev, WLC_DOWN, &local_down, sizeof(local_down));
	if (unlikely(err)) {
		WL_ERR(("WLC_DOWN error (%d)\n", err));
	}
#ifdef WLAN_ACCEL_BOOT
	err = wldev_iovar_setint(ndev, "bus:host_access", bus_host_access);
	if (unlikely(err)) {
		WL_ERR(("bus:host_access(%d) error (%d)\n", bus_host_access, err));
	}
#endif /* WLAN_ACCEL_BOOT */
	WL_INFORM_MEM(("wl down\n"));
	dhd_dongle_up = FALSE;

	return err;
}

s32
wl_dongle_roam(struct net_device *ndev, u32 roamvar, u32 bcn_timeout)
{
	s32 err = 0;

	/* Setup timeout if Beacons are lost and roam is off to report link down */
	if (roamvar) {
		err = wldev_iovar_setint(ndev, "bcn_timeout", bcn_timeout);
		if (unlikely(err)) {
			WL_ERR(("bcn_timeout error (%d)\n", err));
			goto dongle_rom_out;
		}
	}
	/* Enable/Disable built-in roaming to allow supplicant to take care of roaming */
	err = wldev_iovar_setint(ndev, "roam_off", roamvar);
	if (unlikely(err)) {
		WL_ERR(("roam_off error (%d)\n", err));
		goto dongle_rom_out;
	}
	ROAMOFF_DBG_SAVE(ndev, SET_ROAM_FILS_TOGGLE, roamvar);
dongle_rom_out:
	return err;
}

s32 dhd_config_dongle(struct bcm_cfg80211 *cfg)
{
#ifndef DHD_SDALIGN
#define DHD_SDALIGN	32
#endif
	struct net_device *ndev;
	s32 err = 0;
	dhd_pub_t *dhd = NULL;

	WL_TRACE(("In\n"));

	ndev = bcmcfg_to_prmry_ndev(cfg);
	dhd = (dhd_pub_t *)(cfg->pub);

	err = wl_dongle_up(ndev);
	if (unlikely(err)) {
		WL_ERR(("wl_dongle_up failed\n"));
		goto default_conf_out;
	}

	if (dhd && dhd->fw_preinit) {
		/* Init config will be done by fw preinit context */
		return BCME_OK;
	}

default_conf_out:

	return err;

}

int dhd_cfgvendor_priv_string_handler(struct bcm_cfg80211 *cfg, struct wireless_dev *wdev,
	const struct bcm_nlmsg_hdr *nlioc, void *buf)
{
	struct net_device *ndev = NULL;
	dhd_pub_t *dhd;
	dhd_ioctl_t ioc = { 0, NULL, 0, 0, 0, 0, 0};
	int ret = 0;
	int8 index;

	WL_TRACE(("entry: cmd = %d\n", nlioc->cmd));

	dhd = cfg->pub;
	DHD_OS_WAKE_LOCK(dhd);

	ndev = wdev_to_wlc_ndev(wdev, cfg);
	index = dhd_net2idx(dhd->info, ndev);
	if (index == DHD_BAD_IF) {
		WL_ERR(("Bad ifidx from wdev:%p\n", wdev));
		ret = BCME_ERROR;
		goto done;
	}

	ioc.cmd = nlioc->cmd;
	ioc.len = nlioc->len;
	ioc.set = nlioc->set;
	ioc.driver = nlioc->magic;
	ioc.buf = buf;
	ret = dhd_ioctl_process(dhd, index, &ioc, buf);
	if (ret) {
		WL_TRACE(("dhd_ioctl_process return err %d\n", ret));
		ret = OSL_ERROR(ret);
		goto done;
	}

done:
	DHD_OS_WAKE_UNLOCK(dhd);
	return ret;
}

int
dhd_set_wsec_info(dhd_pub_t *dhd, uint32 data, int tag)
{
	struct net_device *ndev;
	uint32  wsec_info = data;
	int ret = BCME_OK;

	ndev = dhd_linux_get_primary_netdev(dhd);
	if (!ndev) {
		WL_ERR(("Cannot find primary netdev\n"));
		return -ENODEV;
	}
	BCM_REFERENCE(wsec_info);
#if defined(WL_CFG80211)
	ret = wl_cfg80211_set_wsec_info(ndev, &wsec_info, sizeof(wsec_info), tag);
	if (unlikely(ret)) {
		WL_ERR(("Set wsec_info tag 0x%04x failed \n", tag));
	}
#endif /* WL_CFG80211 */

	return ret;
}

#ifdef RPM_FAST_TRIGGER
void
dhd_trigger_rpm_fast(struct bcm_cfg80211 *cfg)
{
	dhd_pub_t *dhd = (dhd_pub_t *)cfg->pub;
	dhdpcie_trigger_rpm_fast(dhd);
}
#endif /* RPM_FAST_TRIGGER */
