
#ifdef WL_EXT_IAPSTA
#include <net/rtnetlink.h>
#include <bcmendian.h>
#include <dhd_linux.h>
#include <wlioctl_utils.h>
#include <wl_android.h>
#include <dhd_config.h>
#ifdef WL_CFG80211
#include <wl_cfg80211.h>
#include <wl_cfgscan.h>
#endif /* WL_CFG80211 */
#ifdef WL_ESCAN
#include <wl_escan.h>
#endif /* WL_ESCAN */

#ifdef ARP_DETECTION
#include <net/arp.h>
#include <net/route.h>
#include <linux/skbuff.h>
#include <linux/etherdevice.h>
#include <bcmarp.h>
#endif /* ARP_DETECTION */

#ifndef WL_CFG80211
#ifdef WL_EXT_RECONNECT
#error "WL_EXT_RECONNECT should be defined based on WL_CFG80211"
#endif /* WL_EXT_RECONNECT */
#ifdef SCAN_SUPPRESS
#error "SCAN_SUPPRESS should be defined based on WL_CFG80211"
#endif /* SCAN_SUPPRESS */
#ifdef KEY_INSTALL_CHECK
#error "KEY_INSTALL_CHECK should be defined based on WL_CFG80211"
#endif /* KEY_INSTALL_CHECK */
#ifdef WL_EXT_REASSOC
#error "WL_EXT_REASSOC should be defined based on WL_CFG80211"
#endif /* WL_EXT_REASSOC */
#endif /* !WL_CFG80211 */

#ifdef BTC_WAR
/* btc_war:
 * -1(don't apply btc)
 *   0(disable btc war): btc_mode=1; txchain=3; rxchain=3
 *   1(enable btc war): txchain=2; rxchain=2; btc_mode=5
 */
int btc_war = -1;
module_param(btc_war, int, 0644);
#endif /* BTC_WAR */

#define IAPSTA_ERROR(name, arg1, args...) \
	do { \
		if (android_msg_level & ANDROID_ERROR_LEVEL) { \
			printf("[%s] IAPSTA-ERROR) %s : " arg1, name, __func__, ## args); \
		} \
	} while (0)
#define IAPSTA_TRACE(name, arg1, args...) \
	do { \
		if (android_msg_level & ANDROID_TRACE_LEVEL) { \
			printf("[%s] IAPSTA-TRACE) %s : " arg1, name, __func__, ## args); \
		} \
	} while (0)
#define IAPSTA_INFO(name, arg1, args...) \
	do { \
		if (android_msg_level & ANDROID_INFO_LEVEL) { \
			printf("[%s] IAPSTA-INFO) %s : " arg1, name, __func__, ## args); \
		} \
	} while (0)
#define IAPSTA_DBG(name, arg1, args...) \
	do { \
		if (android_msg_level & ANDROID_DBG_LEVEL) { \
			printf("[%s] IAPSTA-DBG) %s : " arg1, name, __func__, ## args); \
		} \
	} while (0)

#ifdef PROP_TXSTATUS
#include <dhd_wlfc.h>
#ifdef PROP_TXSTATUS_VSDB
extern int disable_proptx;
#endif /* PROP_TXSTATUS_VSDB */
#endif /* PROP_TXSTATUS */

#ifndef WL_CFG80211
#define htod32(i) i
#define htod16(i) i
#define dtoh32(i) i
#define dtoh16(i) i
#define IEEE80211_BAND_2GHZ 0
#define IEEE80211_BAND_5GHZ 1
#endif /* WL_CFG80211 */

#define WL_DUMP_BUF_LEN (127 * 1024)

#define MAX_AP_LINK_WAIT_TIME   3000
#define MAX_STA_LINK_WAIT_TIME   15000
#define STA_LINKDOWN_TIMEOUT	10000
#define STA_EAP_CONNECT_TIMEOUT	70500
#define REASSOC_SCAN_RETRY_MS 1000
#define STA_CONNECT_FULL_CHAN_TIMEOUT	3000
#define STA_CONNECT_RETRY_TIMEOUT	600
#define STA_RECONNECT_RETRY_TIMEOUT	300
#define STA_DISCONNECT_RECONNECT_TIMEOUT	30000
#define STA_DISCONNECT_RECONNECT_MAX	3
#define STA_4WAY_TIMEOUT	3000
#define STA_EAP_TIMEOUT	10000
#define STA_EAPOL_TIMEOUT	100
#define STA_KEY_INSTALL_INTERVAL	100
#ifdef EAPOL_RESEND_M4
#define STA_KEY_INSTALL_MAX	10
#else
#define STA_KEY_INSTALL_MAX	5
#endif
#define STA_EMPTY_SCAN_MAX	6
#define AP_RESTART_TIMEOUT	1500
#define AP_TXBCNFRM_TIMEOUT	10000
#ifdef RXF0OVFL_REINIT_WAR
#define RXF0OVFL_POLLING_TIMEOUT	1000
#define RXF0OVFL_THRESHOLD	100
#endif /* RXF0OVFL_REINIT_WAR */
#ifdef ARP_DETECTION
#define ARP_DETECTION_INTERVAL	1000
#define ARP_DETECTION_START_MS	3000
#define ARP_DETECTION_TIMEOUT	3000
#define ARP_DETECTION_LOWRSSI	-75
#endif /* ARP_DETECTION */
#define CSA_RESET_CNT_MAX	3
#define CSA_RESET_INTERVAL_MS	1000

#define MAX_DWDS_IF_NUM 4

enum wl_if_list {
	IF_PIF,
	IF_VIF,
	IF_VIF2,
	MAX_IF_NUM
};

typedef enum WL_PRIO {
	PRIO_AP = 1,
	PRIO_MESH = 2,
	PRIO_GO = 3,
	PRIO_GC = 4,
	PRIO_STA = 4
} wl_prio_t;

typedef enum APSTAMODE {
	IUNKNOWN_MODE = 0,
	ISTAONLY_MODE = 1,
	IAPONLY_MODE = 2,
	ISTAAP_MODE = 3,
	ISTAGO_MODE = 4,
	ISTASTA_MODE = 5,
	IDUALAP_MODE = 6,
	ISTAAPAP_MODE = 7,
	IMESHONLY_MODE = 8,
	ISTAMESH_MODE = 9,
	IMESHAP_MODE = 10,
	ISTAAPMESH_MODE = 11,
	IMESHAPAP_MODE = 12
} apstamode_t;

#ifdef ISAM_CONFIG
typedef enum BGNMODE {
	IEEE80211B = 1,
	IEEE80211G,
	IEEE80211BG,
	IEEE80211BGN,
	IEEE80211BGNAC
} bgnmode_t;

typedef enum AUTHMODE {
	AUTH_OPEN,
	AUTH_SHARED,
	AUTH_WPAPSK,
	AUTH_WPA2PSK,
	AUTH_WPAWPA2PSK,
	AUTH_SAE
} authmode_t;

typedef enum ENCMODE {
	ENC_NONE,
	ENC_WEP,
	ENC_TKIP,
	ENC_AES,
	ENC_TKIPAES
} encmode_t;
#endif

typedef struct mchan_cap_t {
	int band_x;
	int band_y;
	bool mcc;
} mchan_cap_t;

/* Same Channel Concurrent */
const mchan_cap_t mchan_scc_map[] = {
	{WLC_BAND_2G,	WLC_BAND_2G,	FALSE},
	{WLC_BAND_2G,	WLC_BAND_5G,	FALSE},
	{WLC_BAND_2G,	WLC_BAND_6G,	FALSE},
	{WLC_BAND_5G,	WLC_BAND_5G,	FALSE},
	{WLC_BAND_5G,	WLC_BAND_6G,	FALSE},
	{WLC_BAND_6G,	WLC_BAND_6G,	FALSE},
};

/* Multiple Channel Concurrent */
const mchan_cap_t mchan_mcc_map[] = {
	{WLC_BAND_2G,	WLC_BAND_2G,	TRUE},
	{WLC_BAND_2G,	WLC_BAND_5G,	TRUE},
	{WLC_BAND_2G,	WLC_BAND_6G,	TRUE},
	{WLC_BAND_5G,	WLC_BAND_5G,	TRUE},
	{WLC_BAND_5G,	WLC_BAND_6G,	TRUE},
	{WLC_BAND_6G,	WLC_BAND_6G,	TRUE},
};

/* Same Band Same Channel Concurrent */
const mchan_cap_t mchan_sbsc_map[] = {
	{WLC_BAND_2G,	WLC_BAND_2G,	FALSE},
	{WLC_BAND_2G,	WLC_BAND_5G,	TRUE},
	{WLC_BAND_2G,	WLC_BAND_6G,	TRUE},
	{WLC_BAND_5G,	WLC_BAND_5G,	FALSE},
	{WLC_BAND_5G,	WLC_BAND_6G,	TRUE},
	{WLC_BAND_6G,	WLC_BAND_6G,	FALSE},
};

/* RSDB Channel Concurrent */
const mchan_cap_t mchan_rsdb_map[] = {
	{WLC_BAND_2G,	WLC_BAND_2G,	FALSE},
	{WLC_BAND_2G,	WLC_BAND_5G,	TRUE},
	{WLC_BAND_2G,	WLC_BAND_6G,	TRUE},
	{WLC_BAND_5G,	WLC_BAND_5G,	FALSE},
	{WLC_BAND_5G,	WLC_BAND_6G,	FALSE},
	{WLC_BAND_6G,	WLC_BAND_6G,	FALSE},
};
#define MCHAN_CAP_NUM (sizeof(mchan_mcc_map)/sizeof(mchan_mcc_map[0]))

#define MCHANMODE2STR(mchan_mode) ((mchan_mode == MCHAN_AUTO) ? "AUTO" : \
	(mchan_mode == MCHAN_SCC) ? "SCC" : \
	(mchan_mode == MCHAN_SBSC) ? "SBSC" : \
	(mchan_mode == MCHAN_MCC) ? "MCC" : \
	(mchan_mode == MCHAN_RSDB) ? "RSDB" : "??")

#ifdef STA_MGMT
typedef struct wl_sta_info {
	int ifidx;
	struct ether_addr bssid;
	struct list_head list;
} wl_sta_info_t;
#endif /* STA_MGMT */

#ifdef TPUT_MONITOR
typedef struct wl_tput_info {
	unsigned long last_tx;
	unsigned long last_rx;
	struct osl_timespec tput_ts;
	int32 tput_tx;
	int32 tput_rx;
	int32 tput_tx_kb;
	int32 tput_rx_kb;
} wl_tput_info_t;
#endif /* TPUT_MONITOR */

#ifdef WLDWDS
typedef struct wl_dwds_info {
	struct net_device *dev;
	int ifidx;
	uint8 bssidx;
#ifdef TPUT_MONITOR
	struct wl_tput_info tput_info;
#endif /* TPUT_MONITOR */
} wl_dwds_info_t;
#endif /* WLDWDS */

typedef struct wl_if_info {
	struct net_device *dev;
	ifmode_t ifmode;
	unsigned long status;
	wl_prio_t prio;
	int ifidx;
	uint8 bssidx;
	char ifname[IFNAMSIZ+1];
	char ssid[DOT11_MAX_SSID_LEN];
	struct ether_addr bssid;
	struct wl_ext_chan_info chan_info;
#ifdef ISAM_CONFIG
	bgnmode_t bgnmode;
	int hidden;
	authmode_t amode;
	encmode_t emode;
	int maxassoc;
	char key[100];
#endif
	bool csa_complete;
	wait_queue_head_t csa_complete_event;
	int mchan_mode;
	mchan_cap_t mchan_cap[MCHAN_CAP_NUM];
#ifdef WL_ESCAN
#if (defined(WLMESH) || defined(ACS_MONITOR))
	struct wl_escan_info *escan;
#ifdef WLMESH
	timer_list_compat_t delay_scan;
#endif /* WLMESH */
#ifdef ACS_MONITOR
	timer_list_compat_t acs_timer;
#endif /* ACS_MONITOR */
#endif /* WLMESH || ACS_MONITOR */
#endif /* WL_ESCAN */
	struct delayed_work pm_enable_work;
	struct mutex pm_sync;
#ifdef PROPTX_MAXCOUNT
	int transit_maxcount;
#endif /* PROPTX_MAXCOUNT */
	uint conn_state;
	struct osl_timespec sta_disc_ts;
	struct osl_timespec sta_conn_ts;
	bool ap_recon_sta;
	wait_queue_head_t ap_recon_sta_event;
	struct ether_addr ap_disc_sta_bssid;
	struct osl_timespec ap_disc_sta_ts;
#ifdef TPUT_MONITOR
	struct wl_tput_info tput_info;
#endif /* TPUT_MONITOR */
	timer_list_compat_t connect_timer;
#ifdef WL_EXT_REASSOC
	int reassoc_scan_type;
	timer_list_compat_t reassoc_scan_timer;
#endif /* WL_EXT_REASSOC */
#ifdef WL_EXT_RECONNECT
	timer_list_compat_t reconnect_timer;
#endif /* WL_EXT_RECONNECT */
#ifdef EAPOL_RESEND
	void *pend_eapol_pkt;
	timer_list_compat_t eapol_timer;
#ifdef EAPOL_DYNAMATIC_RESEND
	struct osl_timespec eapol_tx_ts;
	bool eapol_retry;
	int eapol_cnt;
	int eapol_avg_intvl;
	int eapol_min_intvl;
	int eapol_max_intvl;
	int eapol_resend_intvl;
#endif /* EAPOL_DYNAMATIC_RESEND */
#endif /* EAPOL_RESEND */
#ifdef KEY_INSTALL_CHECK
	timer_list_compat_t key_install_timer;
	int key_install_cnt;
#endif /* KEY_INSTALL_CHECK */
#if defined(WL_CFG80211) && defined(WL_ROAM_PMK_WAR)
	wsec_pmk_t pmk;
#endif /* WL_CFG80211 && WL_ROAM_PMK_WAR */
	int empty_scan;
#ifdef RESTART_AP_WAR
	timer_list_compat_t restart_ap_timer;
#endif /* RESTART_AP_WAR */
#ifdef RESET_AP_WAR
	timer_list_compat_t reset_ap_timer;
	uint32 txbcnfrm;
#endif /* RESET_AP_WAR */
#ifdef ARP_DETECTION
	timer_list_compat_t arp_timer;
	u32 gw_ipaddr; /* Saved gateway for Wlan0 */
	u32 local_ipaddr;
	struct osl_timespec ip_ready_ts;
	struct osl_timespec arp_rx_ts;
#endif /* ARP_DETECTION */
#ifdef WL_EXT_RECONNECT
#ifdef WL_EXT_DISCONNECT_RECONNECT
	struct osl_timespec sta_disc_conn_ts;
	int sta_disc_recon_cnt;
#endif /* WL_EXT_DISCONNECT_RECONNECT */
#endif /* WL_EXT_RECONNECT */
} wl_if_info_t;

u8 *g_ioctl_buf = NULL;

typedef struct wl_apsta_params {
	struct wl_if_info if_info[MAX_IF_NUM];
#ifdef WLDWDS
	struct wl_dwds_info dwds_info[MAX_DWDS_IF_NUM];
#endif /* WLDWDS */
	bool init;
	bool rsdb;
	bool mcc;
	bool csa;
	uint acs;
#ifdef ACS_MONITOR
	uint acs_tmo;
#endif /* ACS_MONITOR */
	bool radar;
#ifdef WL_PASSIVE_CHAN_UPDATE
	bool wiphy_update;
#endif /* WL_PASSIVE_CHAN_UPDATE */
	apstamode_t apstamode;
	wait_queue_head_t netif_change_event;
	struct mutex usr_sync;
#if defined(WLMESH) && defined(WL_ESCAN)
	int macs;
	struct wl_mesh_params mesh_info;
#endif /* WLMESH && WL_ESCAN */
	struct mutex in4way_sync;
	int sta_btc_mode;
#ifdef TPUT_MONITOR
	timer_list_compat_t monitor_timer;
	int32 tput_sum;
	int32 tput_sum_kb;
#endif /* TPUT_MONITOR */
#ifdef SCAN_SUPPRESS
	struct osl_timespec scan_busy_ts;
	int scan_busy_cnt;
#endif /* SCAN_SUPPRESS */
	uint32 linkdown_reason;
#ifdef EAPOL_RESEND
	spinlock_t eapol_lock;
#endif /* EAPOL_RESEND */
#ifdef STA_MGMT
	struct list_head sta_list;
#endif /* STA_MGMT */
#ifdef RXF0OVFL_REINIT_WAR
	timer_list_compat_t rxf0ovfl_timer;
	uint32 rxbeaconmbss;
	uint32 rxf0ovfl;
	int war_reason;
#endif /* RXF0OVFL_REINIT_WAR */
	timer_list_compat_t reset_itf_timer;
} wl_apsta_params_t;

enum wifi_isam_status {
	ISAM_STATUS_IF_ADDING = 0,
	ISAM_STATUS_IF_READY,
	ISAM_STATUS_STA_CONNECTING,
	ISAM_STATUS_STA_CONNECTED,
	ISAM_STATUS_AP_CREATING,
	ISAM_STATUS_AP_CREATED
};

#define wl_get_isam_status(cur_if, stat) \
	(test_bit(ISAM_STATUS_ ## stat, &(cur_if)->status))
#define wl_set_isam_status(cur_if, stat) \
	(set_bit(ISAM_STATUS_ ## stat, &(cur_if)->status))
#define wl_clr_isam_status(cur_if, stat) \
	(clear_bit(ISAM_STATUS_ ## stat, &(cur_if)->status))
#define wl_chg_isam_status(cur_if, stat) \
	(change_bit(ISAM_STATUS_ ## stat, &(cur_if)->status))

#ifdef ISAM_CONFIG
static int wl_ext_enable_iface(struct net_device *dev, char *ifname,
	int wait_up, bool lock);
static int wl_ext_disable_iface(struct net_device *dev, char *ifname);
#endif

static struct wl_apsta_params *
wl_get_apsta_params(struct net_device *dev)
{
	dhd_pub_t *dhd = dhd_get_pub(dev);
	return (struct wl_apsta_params *)dhd->iapsta_params;
}

static struct wl_if_info *
wl_get_cur_if(struct net_device *dev)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(dev);
	struct wl_if_info *tmp_if = NULL;
	int i;

	for (i=0; i<MAX_IF_NUM; i++) {
		tmp_if = &apsta_params->if_info[i];
		if (tmp_if->dev && tmp_if->dev == dev) {
			return tmp_if;
		}
	}

	return NULL;
}

static bool
wl_ext_slave_if(struct wl_if_info *cur_if)
{
	if (cur_if->ifmode == ISTA_MODE || cur_if->ifmode == IGC_MODE)
		return TRUE;
	else
		return FALSE;
}

static bool
wl_ext_master_if(struct wl_if_info *cur_if)
{
	if (cur_if->ifmode == IAP_MODE || cur_if->ifmode == IGO_MODE ||
			cur_if->ifmode == IMESH_MODE)
		return TRUE;
	else
		return FALSE;
}

#ifdef WL_PM_ENABLE_TIMEOUT
#undef WL_PM_ENABLE_TIMEOUT
#endif
#define WL_PM_ENABLE_TIMEOUT 10000u
#if defined(STRICT_GCC_WARNINGS) && defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == \
	4 && __GNUC_MINOR__ >= 6))
#define BCM_SET_CONTAINER_OF(entry, ptr, type, member) \
_Pragma("GCC diagnostic push") \
_Pragma("GCC diagnostic ignored \"-Wcast-qual\"") \
entry = container_of((ptr), type, member); \
_Pragma("GCC diagnostic pop")
#else
#define BCM_SET_CONTAINER_OF(entry, ptr, type, member) \
entry = container_of((ptr), type, member);
#endif /* STRICT_GCC_WARNINGS */

static void
wl_ext_pm_work_handler(struct work_struct *work)
{
	struct wl_if_info *cur_if;
	s32 pm = PM_FAST;
	dhd_pub_t *dhd;

	BCM_SET_CONTAINER_OF(cur_if, work, struct wl_if_info, pm_enable_work.work);

	IAPSTA_TRACE("wlan", "%s: Enter\n", __FUNCTION__);

	if (cur_if->dev == NULL)
		return;

#if defined(STRICT_GCC_WARNINGS) && defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == \
	4 && __GNUC_MINOR__ >= 6))
_Pragma("GCC diagnostic push")
_Pragma("GCC diagnostic ignored \"-Wcast-qual\"")
#endif

	dhd = dhd_get_pub(cur_if->dev);

	if (!dhd || !dhd->up) {
		IAPSTA_TRACE(cur_if->ifname, "dhd is null or not up\n");
		return;
	}
	if (dhd_conf_get_pm(dhd) >= 0)
		pm = dhd_conf_get_pm(dhd);
	wl_ext_ioctl(cur_if->dev, WLC_SET_PM, &pm, sizeof(pm), 1);
#if defined(STRICT_GCC_WARNINGS) && defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == \
	4 && __GNUC_MINOR__ >= 6))
_Pragma("GCC diagnostic pop")
#endif
	DHD_PM_WAKE_UNLOCK(dhd);

}

void
wl_ext_add_remove_pm_enable_work(struct net_device *dev, bool add)
{
	dhd_pub_t *dhd = dhd_get_pub(dev);
	struct wl_if_info *cur_if = NULL;
	u16 wq_duration = 0;
	s32 pm = PM_OFF;

	cur_if = wl_get_cur_if(dev);
	if (!cur_if)
		return;

	mutex_lock(&cur_if->pm_sync);
	/*
	 * Make cancel and schedule work part mutually exclusive
	 * so that while cancelling, we are sure that there is no
	 * work getting scheduled.
	 */

	if (delayed_work_pending(&cur_if->pm_enable_work)) {
		cancel_delayed_work_sync(&cur_if->pm_enable_work);
		DHD_PM_WAKE_UNLOCK(dhd);
	}

	if (add) {
		wq_duration = (WL_PM_ENABLE_TIMEOUT);
	}

	/* It should schedule work item only if driver is up */
	if (dhd->up) {
		if (add) {
			if (dhd_conf_get_pm(dhd) >= 0)
				pm = dhd_conf_get_pm(dhd);
			wl_ext_ioctl(cur_if->dev, WLC_SET_PM, &pm, sizeof(pm), 1);
		}
		if (wq_duration) {
			if (schedule_delayed_work(&cur_if->pm_enable_work,
					msecs_to_jiffies((const unsigned int)wq_duration))) {
				DHD_PM_WAKE_LOCK_TIMEOUT(dhd, wq_duration);
			} else {
				IAPSTA_ERROR(cur_if->ifname, "Can't schedule pm work handler\n");
			}
		}
	}
	mutex_unlock(&cur_if->pm_sync);

}

static void
wl_ext_set_chan_info(struct wl_if_info *cur_if, uint band, uint16 chan)
{
	cur_if->chan_info.band = band;
	cur_if->chan_info.chan = chan;
}

bool
wl_ext_associated(struct net_device *dev, struct ether_addr *bssid)
{
	struct ether_addr addr;
	int ret = 0;

	ret = wldev_ioctl(dev, WLC_GET_BSSID, &addr, sizeof(addr), 0);
	if (ret != BCME_NOTASSOCIATED && memcmp(&ether_null, &addr, ETHER_ADDR_LEN)) {
		if (bssid)
			memcpy(bssid, &addr, ETHER_ADDR_LEN);
		return TRUE;
	}

	return FALSE;
}

static void
wl_phy_rssi_ant(struct net_device *dev, char *mac, char *rssi_buf, int len)
{
	char buf[WLC_IOCTL_SMLEN];
	wl_rssi_ant_t *rssi_ant_p;
	int ret, bytes_w = 0, i;
	scb_val_t scb_val;

	memset(buf, 0, sizeof(buf));
	ret = wldev_iovar_getbuf(dev, "phy_rssi_ant",
		mac, mac ? ETHER_ADDR_LEN : 0, buf, sizeof(buf), NULL);
	rssi_ant_p = (wl_rssi_ant_t *)buf;
	rssi_ant_p->version = dtoh32(rssi_ant_p->version);
	rssi_ant_p->count = dtoh32(rssi_ant_p->count);
	if (ret < 0 || rssi_ant_p->count == 0) {
		ret = wldev_ioctl_get(dev, WLC_GET_RSSI, &scb_val, sizeof(scb_val_t));
		if (ret) {
			IAPSTA_ERROR(dev->name, "WLC_GET_RSSI err %d\n", ret);
			return;
		}
		rssi_ant_p->count = 1;
		rssi_ant_p->rssi_ant[0] = dtoh32(scb_val.val);
	}
	for (i=0; i<rssi_ant_p->count && rssi_ant_p->rssi_ant[i]; i++) {
		bytes_w += snprintf(rssi_buf+bytes_w, len-bytes_w,
			"[%2d]", rssi_ant_p->rssi_ant[i]);
	}
}

u32
wl_ext_get_chanspec(struct net_device *dev, struct wl_ext_chan_info *chan_info)
{
	struct dhd_pub *dhd = dhd_get_pub(dev);
	u32 chanspec = 0;

	if (wl_ext_associated(dev, NULL)) {
		if (wl_ext_iovar_getint(dev, "chanspec", (s32 *)&chanspec) == BCME_OK) {
			chanspec = wl_ext_chspec_driver_to_host(dhd, chanspec);
			if (chan_info) {
				chan_info->band = CHSPEC2WLC_BAND(chanspec);
				chan_info->chan = wf_chspec_ctlchan(chanspec);
			}
		}
	}

	return chanspec;
}

#ifdef WLMESH
static uint16
wl_ext_get_chan(struct net_device *dev, struct wl_ext_chan_info *chan_info)
{
	uint16 chan = 0, ctl_chan;
	u32 chanspec = 0;

	chanspec = wl_ext_get_chanspec(dev, chan_info);
	if (chanspec) {
		ctl_chan = wf_chspec_ctlchan(chanspec);
		chan = (u16)(ctl_chan & 0x00FF);
	}

	return chan;
}
#endif /* WLMESH */

void
wl_ext_get_chan_str(struct net_device *dev, char *chan_str, int total_len)
{
	struct wl_ext_chan_info chan_info;
	u32 chanspec = 0;

	memset(chan_str, 0, total_len);
	chanspec = wl_ext_get_chanspec(dev, &chan_info);
	if (chanspec) {
		snprintf(chan_str, total_len, "%s-%d %sMHz",
			WLCBAND2STR(chan_info.band), chan_info.chan,
			wf_chspec_to_bw_str(chanspec));
	}
}

static chanspec_t
wl_ext_chan_to_chanspec(struct net_device *dev, struct wl_ext_chan_info *chan_info)
{
	struct dhd_pub *dhd = dhd_get_pub(dev);
	chanspec_band_t chanspec_band;
	chanspec_t chspec = 0, fw_chspec = 0;
	u32 bw = WL_CHANSPEC_BW_20;
	s32 err = BCME_OK, bw_cap = 0;
	s8 iovar_buf[WLC_IOCTL_SMLEN];
	struct {
		u32 band;
		u32 bw_cap;
	} param = {0, 0};

	if ((chan_info->band != WLC_BAND_2G) && (chan_info->band != WLC_BAND_5G) &&
			(chan_info->band != WLC_BAND_6G)) {
		IAPSTA_ERROR(dev->name, "bad band %d\n", chan_info->band);
		return BCME_BADBAND;
	}

	param.band = chan_info->band;
	err = wl_ext_iovar_getbuf(dev, "bw_cap", &param, sizeof(param),
		iovar_buf, WLC_IOCTL_SMLEN, NULL);
	if (err) {
		if (err != BCME_UNSUPPORTED) {
			IAPSTA_ERROR(dev->name, "bw_cap failed, %d\n", err);
			return err;
		} else {
			err = wl_ext_iovar_getint(dev, "mimo_bw_cap", &bw_cap);
			if (bw_cap != WLC_N_BW_20ALL)
				bw = WL_CHANSPEC_BW_40;
		}
	} else {
		if (WL_BW_CAP_80MHZ(iovar_buf[0]))
			bw = WL_CHANSPEC_BW_80;
		else if (WL_BW_CAP_40MHZ(iovar_buf[0]))
			bw = WL_CHANSPEC_BW_40;
		else
			bw = WL_CHANSPEC_BW_20;
	}

	if (chan_info->band == WLC_BAND_5G && chan_info->chan == 165)
		bw = WL_CHANSPEC_BW_20;

set_channel:
	chanspec_band = wl_ext_wlcband_to_chanspec_band(chan_info->band);
	chspec = wf_create_chspec_from_primary(chan_info->chan, bw, chanspec_band);
	if (wf_chspec_valid(chspec)) {
		fw_chspec = wl_ext_chspec_host_to_driver(dhd, chspec);
		if (fw_chspec == INVCHANSPEC) {
			IAPSTA_ERROR(dev->name, "failed to convert host chanspec to fw chanspec\n");
			fw_chspec = 0;
		}
	} else {
		if (bw == WL_CHANSPEC_BW_80)
			bw = WL_CHANSPEC_BW_40;
		else if (bw == WL_CHANSPEC_BW_40)
			bw = WL_CHANSPEC_BW_20;
		else
			bw = 0;
		if (bw)
			goto set_channel;
		IAPSTA_ERROR(dev->name, "Invalid chanspec 0x%x\n", chspec);
		err = BCME_ERROR;
	}

	return fw_chspec;
}

static bool
wl_ext_radar_detect(struct net_device *dev)
{
	int ret = BCME_OK;
	bool radar = FALSE;
	s32 val = 0;

	if ((ret = wldev_ioctl(dev, WLC_GET_RADAR, &val, sizeof(int), false) == 0)) {
		radar = TRUE;
	}
	IAPSTA_INFO(dev->name, "radar=%d\n", radar);

	return radar;
}

static int
wl_ext_assoclist(struct net_device *dev, char *data, char *command,
	int total_len)
{
	int ret = 0, i, maxassoc = 0, bytes_w = 0;
	char mac_buf[MAX_NUM_OF_ASSOCLIST *
		sizeof(struct ether_addr) + sizeof(uint)] = {0};
	struct maclist *assoc_maclist = (struct maclist *)mac_buf;

	assoc_maclist->count = htod32(MAX_NUM_OF_ASSOCLIST);
	ret = wl_ext_ioctl(dev, WLC_GET_ASSOCLIST, assoc_maclist, sizeof(mac_buf), 0);
	if (ret)
		return 0;
	maxassoc = dtoh32(assoc_maclist->count);
	for (i=0; i<maxassoc; i++) {
		bytes_w += snprintf(command+bytes_w, total_len-bytes_w,
			"\n%s#%02d: %pM", DHD_LOG_PREFIXS, i, &assoc_maclist->ea[i]);
	}

	return bytes_w;
}

void
wl_ext_send_event_msg(struct net_device *dev, int event, int status,
	int reason)
{
	struct dhd_pub *dhd = dhd_get_pub(dev);
	struct wl_if_info *cur_if;
	wl_event_msg_t msg;

	if (dhd && dhd->up) {
		cur_if = wl_get_cur_if(dev);
		if (!cur_if)
			return;
		bzero(&msg, sizeof(wl_event_msg_t));
		msg.ifidx = dhd_net2idx(dhd->info, dev);
		msg.event_type = hton32(event);
		msg.status = hton32(status);
		msg.reason = hton32(reason);
		memcpy(&msg.addr, &cur_if->bssid, ETHER_ADDR_LEN);
#ifdef WL_EVENT
		wl_ext_event_send(dhd->event_params, &msg, NULL);
#endif /* WL_EVENT */
#ifdef WL_CFG80211
		wl_cfg80211_event(dev, &msg, NULL);
#endif /* WL_CFG80211 */
	}
}

#ifdef BTC_WAR
static bool
wl_ext_iapsta_if_2g_enabled(struct net_device *net)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(net);
	struct wl_if_info *tmp_if;
	struct wl_ext_chan_info chan_info;
	bool enabled = FALSE;
	uint16 cur_chan;
	int i;

	for (i=0; i<MAX_IF_NUM; i++) {
		tmp_if = &apsta_params->if_info[i];
		if (tmp_if && wl_get_isam_status(tmp_if, IF_READY)) {
			if (wl_ext_get_chanspec(tmp_if->dev, &chan_info) &&
					chan_info.band == WLC_BAND_2G) {
				enabled = TRUE;
			}
			if (enabled)
				break;
		}
	}

	return enabled;
}

void
wl_ext_btc_config(struct net_device *dev, bool enable)
{
	struct dhd_pub *dhd = dhd_get_pub(dev);
	bool enab = FALSE;

	if (dhd->conf->chip == BCM4356_CHIP_ID || dhd->conf->chip == BCM43752_CHIP_ID) {
		IAPSTA_INFO(dev->name, "btc_war=%d, enable=%d\n", btc_war, enable);
		if (btc_war >= 0) {
			if (enable && btc_war > 0) {
				if (wl_ext_iapsta_if_2g_enabled(dev))
					enab = TRUE;
			}
			if (enab) {
				IAPSTA_INFO(dev->name, "enable\n");
				wl_ext_iovar_setint(dev, "txchain", 2);
				wl_ext_iovar_setint(dev, "rxchain", 2);
				wl_ext_iovar_setint(dev, "btc_mode", 5);
			} else {
				IAPSTA_INFO(dev->name, "disable\n");
				wl_ext_iovar_setint(dev, "btc_mode", 1);
				wl_ext_iovar_setint(dev, "txchain", 3);
				wl_ext_iovar_setint(dev, "rxchain", 3);
			}
		}
	}
}
#endif /* BTC_WAR */

static void
wl_ext_connect_timeout(unsigned long data)
{
	struct net_device *dev = (struct net_device *)data;
	struct wl_if_info *cur_if;
	struct bcm_cfg80211 *cfg = wl_get_cfg(dev);
	u8 *curbssid = NULL;

	if (!dev) {
		IAPSTA_ERROR("wlan", "dev is not ready\n");
		return;
	}

	cur_if = wl_get_cur_if(dev);
	if (!cur_if)
		return;

	curbssid = wl_read_prof(cfg, dev, WL_PROF_BSSID);
	if (curbssid)
		(void)memcpy_s(&cur_if->bssid, ETH_ALEN, curbssid, ETH_ALEN);
	IAPSTA_ERROR(dev->name, "timer expired\n");
#ifdef WL_EXT_REASSOC
	cur_if->reassoc_scan_type = SCAN_NONE;
#endif /* WL_EXT_REASSOC */
	wl_ext_send_event_msg(dev, WLC_E_SET_SSID, WLC_E_STATUS_NO_NETWORKS, WLC_E_STATUS_SUCCESS);
}

#if defined(WL_CFG80211) || (defined(WLMESH) && defined(WL_ESCAN))
static struct wl_if_info *
wl_ext_if_enabled(struct wl_apsta_params *apsta_params, ifmode_t ifmode)
{
	struct wl_if_info *tmp_if, *tgt_if = NULL;
	int i;

	for (i=0; i<MAX_IF_NUM; i++) {
		tmp_if = &apsta_params->if_info[i];
		if (tmp_if && tmp_if->ifmode == ifmode &&
				wl_get_isam_status(tmp_if, IF_READY) &&
				wl_ext_associated(tmp_if->dev, NULL)) {
			tgt_if = tmp_if;
			break;
		}
	}

	return tgt_if;
}
#endif

#ifdef WLMESH
static int
wl_mesh_print_peer_info(mesh_peer_info_ext_t *mpi_ext,
	uint32 peer_results_count, char *command, int total_len)
{
	char *peering_map[] = MESH_PEERING_STATE_STRINGS;
	uint32 count = 0;
	int bytes_written = 0;

	bytes_written += snprintf(command+bytes_written, total_len,
		"%2s: %12s : %6s : %-6s : %6s :"
		" %5s : %4s : %4s : %11s : %4s",
		"no", "------addr------ ", "l.aid", "state", "p.aid",
		"mppid", "llid", "plid", "entry_state", "rssi");
	for (count=0; count < peer_results_count; count++) {
		if (mpi_ext->entry_state != MESH_SELF_PEER_ENTRY_STATE_TIMEDOUT) {
			bytes_written += snprintf(command+bytes_written, total_len,
				"\n%2d: %pM : 0x%4x : %6s : 0x%4x :"
				" %5d : %4d : %4d : %11s : %4d",
				count, &mpi_ext->ea, mpi_ext->local_aid,
				peering_map[mpi_ext->peer_info.state],
				mpi_ext->peer_info.peer_aid,
				mpi_ext->peer_info.mesh_peer_prot_id,
				mpi_ext->peer_info.local_link_id,
				mpi_ext->peer_info.peer_link_id,
				(mpi_ext->entry_state == MESH_SELF_PEER_ENTRY_STATE_ACTIVE) ?
				"ACTIVE" :
				"EXTERNAL",
				mpi_ext->rssi);
		} else {
			bytes_written += snprintf(command+bytes_written, total_len,
				"\n%2d: %pM : %6s : %5s : %6s :"
				" %5s : %4s : %4s : %11s : %4s",
				count, &mpi_ext->ea, "  NA  ", "  NA  ", "  NA  ",
				"  NA ", " NA ", " NA ", "  TIMEDOUT ", " NA ");
		}
		mpi_ext++;
	}

	return bytes_written;
}

static int
wl_mesh_get_peer_results(struct net_device *dev, char *buf, int len)
{
	int indata, inlen;
	mesh_peer_info_dump_t *peer_results;
	int ret;

	memset(buf, 0, len);
	peer_results = (mesh_peer_info_dump_t *)buf;
	indata = htod32(len);
	inlen = 4;
	ret = wl_ext_iovar_getbuf(dev, "mesh_peer_status", &indata, inlen, buf, len, NULL);
	if (!ret) {
		peer_results = (mesh_peer_info_dump_t *)buf;
		ret = peer_results->count;
	}

	return ret;
}

int
wl_ext_mesh_peer_status(struct net_device *dev, char *data, char *command,
	int total_len)
{
	struct wl_if_info *cur_if;
	mesh_peer_info_dump_t *peer_results;
	mesh_peer_info_ext_t *mpi_ext;
	char *peer_buf = NULL;
	int peer_len = WLC_IOCTL_MAXLEN;
	int dump_written = 0, ret;

	if (!data) {
		peer_buf = kmalloc(peer_len, GFP_KERNEL);
		if (peer_buf == NULL) {
			IAPSTA_ERROR(dev->name, "Failed to allocate buffer of %d bytes\n",
				peer_len);
			return -1;
		}
		cur_if = wl_get_cur_if(dev);
		if (cur_if && cur_if->ifmode == IMESH_MODE) {
			memset(peer_buf, 0, peer_len);
			ret = wl_mesh_get_peer_results(dev, peer_buf, peer_len);
			if (ret >= 0) {
				peer_results = (mesh_peer_info_dump_t *)peer_buf;
				mpi_ext = (mesh_peer_info_ext_t *)peer_results->mpi_ext;
				dump_written += wl_mesh_print_peer_info(mpi_ext,
					peer_results->count, command+dump_written,
					total_len-dump_written);
			}
		} else if (cur_if) {
			IAPSTA_ERROR(dev->name, "[%s] is not mesh interface\n",
				cur_if->ifname);
		}
	}

	if (peer_buf)
		kfree(peer_buf);
	return dump_written;
}

#ifdef WL_ESCAN
#define WL_MESH_DELAY_SCAN_TMO	3000
static void
wl_mesh_timer(unsigned long data)
{
	struct net_device *dev = (struct net_device *)data;
	struct dhd_pub *dhd;
	wl_event_msg_t msg;

	if (!dev) {
		IAPSTA_ERROR("wlan", "dev is not ready\n");
		return;
	}

	dhd = dhd_get_pub(dev);

	bzero(&msg, sizeof(wl_event_msg_t));
	IAPSTA_TRACE(dev->name, "timer expired\n");

	msg.ifidx = dhd_net2idx(dhd->info, dev);
	msg.event_type = hton32(WLC_E_RESERVED);
	msg.reason = hton32(ISAM_RC_MESH_ACS);
	wl_ext_event_send(dhd->event_params, &msg, NULL);
}

static int
wl_mesh_clear_vndr_ie(struct net_device *dev, uchar *oui)
{
	char *vndr_ie_buf = NULL;
	vndr_ie_setbuf_t *vndr_ie = NULL;
	ie_getbuf_t vndr_ie_tmp;
	char *iovar_buf = NULL;
	int err = -1, i;
	vndr_ie_buf_t *vndr_ie_dump = NULL;
	uchar *iebuf;
	vndr_ie_info_t *ie_info;
	vndr_ie_t *ie;

	vndr_ie_buf = kzalloc(WLC_IOCTL_SMLEN, GFP_KERNEL);
	if (!vndr_ie_buf) {
		IAPSTA_ERROR(dev->name, "IE memory alloc failed\n");
		err = -ENOMEM;
		goto exit;
	}

	iovar_buf = kzalloc(WLC_IOCTL_MEDLEN, GFP_KERNEL);
	if (!iovar_buf) {
		IAPSTA_ERROR(dev->name, "iovar_buf alloc failed\n");
		err = -ENOMEM;
		goto exit;
	}

	memset(iovar_buf, 0, WLC_IOCTL_MEDLEN);
	vndr_ie_tmp.pktflag = (uint32) -1;
	vndr_ie_tmp.id = (uint8) DOT11_MNG_PROPR_ID;
	err = wl_ext_iovar_getbuf(dev, "vndr_ie", &vndr_ie_tmp, sizeof(vndr_ie_tmp),
		iovar_buf, WLC_IOCTL_MEDLEN, NULL);
	if (err)
		goto exit;

	vndr_ie_dump = (vndr_ie_buf_t *)iovar_buf;
	if (!vndr_ie_dump->iecount)
		goto exit;

	iebuf = (uchar *)&vndr_ie_dump->vndr_ie_list[0];
	for (i=0; i<vndr_ie_dump->iecount; i++) {
		ie_info = (vndr_ie_info_t *) iebuf;
		ie = &ie_info->vndr_ie_data;
		if (memcmp(ie->oui, oui, 3))
			memset(ie->oui, 0, 3);
		iebuf += sizeof(uint32) + ie->len + VNDR_IE_HDR_LEN;
	}

	vndr_ie = (vndr_ie_setbuf_t *) vndr_ie_buf;
	strncpy(vndr_ie->cmd, "del", VNDR_IE_CMD_LEN - 1);
	vndr_ie->cmd[VNDR_IE_CMD_LEN - 1] = '\0';
	memcpy(&vndr_ie->vndr_ie_buffer, vndr_ie_dump, WLC_IOCTL_SMLEN-VNDR_IE_CMD_LEN-1);

	memset(iovar_buf, 0, WLC_IOCTL_MEDLEN);
	err = wl_ext_iovar_setbuf(dev, "vndr_ie", vndr_ie, WLC_IOCTL_SMLEN, iovar_buf,
		WLC_IOCTL_MEDLEN, NULL);

exit:
	if (vndr_ie) {
		kfree(vndr_ie);
	}
	if (iovar_buf) {
		kfree(iovar_buf);
	}
	return err;
}

static int
wl_mesh_clear_mesh_info(struct wl_if_info *mesh_if, bool scan)
{
	struct dhd_pub *dhd = dhd_get_pub(mesh_if->dev);
	struct wl_apsta_params *apsta_params = dhd->iapsta_params;
	struct wl_mesh_params *mesh_info = &apsta_params->mesh_info;
	uchar mesh_oui[]={0x00, 0x22, 0xf4};
	int ret;

	IAPSTA_TRACE(mesh_if->dev->name, "Enter\n");

	ret = wl_mesh_clear_vndr_ie(mesh_if->dev, mesh_oui);
	memset(mesh_info, 0, sizeof(struct wl_mesh_params));
	if (scan) {
		mesh_info->scan_channel = wl_ext_get_chan(mesh_if->dev,
			&mesh_if->chan_info);
		wl_timer_mod(dhd, &mesh_if->delay_scan, 100);
	}

	return ret;
}

static int
wl_mesh_update_vndr_ie(struct wl_apsta_params *apsta_params,
	struct wl_if_info *mesh_if)
{
	struct wl_mesh_params *mesh_info = &apsta_params->mesh_info;
	char *vndr_ie;
	uchar mesh_oui[]={0x00, 0x22, 0xf4};
	int bytes_written = 0;
	int ret = 0, i, vndr_ie_len;
	uint8 *peer_bssid;

	wl_mesh_clear_vndr_ie(mesh_if->dev, mesh_oui);

	vndr_ie_len = WLC_IOCTL_MEDLEN;
	vndr_ie = kmalloc(vndr_ie_len, GFP_KERNEL);
	if (vndr_ie == NULL) {
		IAPSTA_ERROR(mesh_if->dev->name, "Failed to allocate buffer of %d bytes\n",
			WLC_IOCTL_MEDLEN);
		ret = -1;
		goto exit;
	}

	bytes_written += snprintf(vndr_ie+bytes_written, vndr_ie_len,
		"0x%02x%02x%02x", mesh_oui[0], mesh_oui[1], mesh_oui[2]);

	bytes_written += snprintf(vndr_ie+bytes_written, vndr_ie_len,
		"%02x%02x%02x%02x%02x%02x%02x%02x", MESH_INFO_MASTER_BSSID, ETHER_ADDR_LEN,
		((u8 *)(&mesh_info->master_bssid))[0], ((u8 *)(&mesh_info->master_bssid))[1],
		((u8 *)(&mesh_info->master_bssid))[2], ((u8 *)(&mesh_info->master_bssid))[3],
		((u8 *)(&mesh_info->master_bssid))[4], ((u8 *)(&mesh_info->master_bssid))[5]);

	bytes_written += snprintf(vndr_ie+bytes_written, vndr_ie_len,
		"%02x%02x%02x", MESH_INFO_MASTER_CHANNEL, 1, mesh_info->master_channel);

	bytes_written += snprintf(vndr_ie+bytes_written, vndr_ie_len,
		"%02x%02x%02x", MESH_INFO_HOP_CNT, 1, mesh_info->hop_cnt);

	bytes_written += snprintf(vndr_ie+bytes_written, vndr_ie_len,
		"%02x%02x", MESH_INFO_PEER_BSSID, mesh_info->hop_cnt*ETHER_ADDR_LEN);
	for (i=0; i<mesh_info->hop_cnt && i<MAX_HOP_LIST; i++) {
		peer_bssid = (uint8 *)&mesh_info->peer_bssid[i];
		bytes_written += snprintf(vndr_ie+bytes_written, vndr_ie_len,
			"%02x%02x%02x%02x%02x%02x",
			peer_bssid[0], peer_bssid[1], peer_bssid[2],
			peer_bssid[3], peer_bssid[4], peer_bssid[5]);
	}

	ret = wl_ext_add_del_ie(mesh_if->dev, VNDR_IE_BEACON_FLAG|VNDR_IE_PRBRSP_FLAG,
		vndr_ie, "add");
	if (!ret) {
		IAPSTA_INFO(mesh_if->dev->name, "mbssid=%pM, mchannel=%d, hop=%d, pbssid=%pM\n",
			&mesh_info->master_bssid, mesh_info->master_channel, mesh_info->hop_cnt,
			mesh_info->peer_bssid);
	}

exit:
	if (vndr_ie)
		kfree(vndr_ie);
	return ret;
}

static bool
wl_mesh_update_master_info(struct wl_apsta_params *apsta_params,
	struct wl_if_info *mesh_if)
{
	struct wl_mesh_params *mesh_info = &apsta_params->mesh_info;
	struct wl_if_info *sta_if = NULL;
	bool updated = FALSE;

	sta_if = wl_ext_if_enabled(apsta_params, ISTA_MODE);
	if (sta_if) {
		wl_ext_associated(mesh_if->dev, &mesh_info->master_bssid);
		mesh_info->master_channel = wl_ext_get_chan(mesh_if->dev, &mesh_if->chan_info);
		mesh_info->hop_cnt = 0;
		memset(mesh_info->peer_bssid, 0, MAX_HOP_LIST*ETHER_ADDR_LEN);
		if (!wl_mesh_update_vndr_ie(apsta_params, mesh_if))
			updated = TRUE;
	}

	return updated;
}

static bool
wl_mesh_update_mesh_info(struct wl_apsta_params *apsta_params,
	struct wl_if_info *mesh_if)
{
	struct wl_mesh_params *mesh_info = &apsta_params->mesh_info, peer_mesh_info;
	uint32 count = 0;
	char *dump_buf = NULL;
	mesh_peer_info_dump_t *peer_results;
	mesh_peer_info_ext_t *mpi_ext;
	struct ether_addr bssid;
	bool updated = FALSE, bss_found = FALSE;
	uint16 cur_chan;

	dump_buf = kmalloc(WLC_IOCTL_MAXLEN, GFP_KERNEL);
	if (dump_buf == NULL) {
		IAPSTA_ERROR(mesh_if->dev->name, "Failed to allocate buffer of %d bytes\n",
			WLC_IOCTL_MAXLEN);
		return FALSE;
	}
	count = wl_mesh_get_peer_results(mesh_if->dev, dump_buf, WLC_IOCTL_MAXLEN);
	if (count > 0) {
		wl_ext_associated(mesh_if->dev, &bssid);
		peer_results = (mesh_peer_info_dump_t *)dump_buf;
		mpi_ext = (mesh_peer_info_ext_t *)peer_results->mpi_ext;
		for (count = 0; count < peer_results->count; count++) {
			if (mpi_ext->entry_state != MESH_SELF_PEER_ENTRY_STATE_TIMEDOUT &&
					mpi_ext->peer_info.state == MESH_PEERING_ESTAB) {
				memset(&peer_mesh_info, 0, sizeof(struct wl_mesh_params));
				bss_found = wl_escan_mesh_info(mesh_if->dev, mesh_if->escan,
					&mpi_ext->ea, &peer_mesh_info);
				if (bss_found && (mesh_info->master_channel == 0 ||
						peer_mesh_info.hop_cnt <= mesh_info->hop_cnt) &&
						memcmp(&peer_mesh_info.peer_bssid, &bssid, ETHER_ADDR_LEN)) {
					memcpy(&mesh_info->master_bssid, &peer_mesh_info.master_bssid,
						ETHER_ADDR_LEN);
					mesh_info->master_channel = peer_mesh_info.master_channel;
					mesh_info->hop_cnt = peer_mesh_info.hop_cnt+1;
					memset(mesh_info->peer_bssid, 0, MAX_HOP_LIST*ETHER_ADDR_LEN);
					memcpy(&mesh_info->peer_bssid, &mpi_ext->ea, ETHER_ADDR_LEN);
					memcpy(&mesh_info->peer_bssid[1], peer_mesh_info.peer_bssid,
						(MAX_HOP_LIST-1)*ETHER_ADDR_LEN);
					updated = TRUE;
				}
			}
			mpi_ext++;
		}
		if (updated) {
			if (wl_mesh_update_vndr_ie(apsta_params, mesh_if)) {
				IAPSTA_ERROR(mesh_if->dev->name, "update failed\n");
				mesh_info->master_channel = 0;
				updated = FALSE;
				goto exit;
			}
		}
	}

	if (!mesh_info->master_channel) {
		wlc_ssid_t cur_ssid;
		char sec[64];
		bool sae = FALSE;
		memset(&peer_mesh_info, 0, sizeof(struct wl_mesh_params));
		wl_ext_ioctl(mesh_if->dev, WLC_GET_SSID, &cur_ssid, sizeof(cur_ssid), 0);
		wl_ext_get_sec(mesh_if->dev, sec, sizeof(sec), FALSE);
		if (strnicmp(sec, "sae/sae", strlen("sae/sae")) == 0)
			sae = TRUE;
		cur_chan = wl_ext_get_chan(mesh_if->dev, &mesh_if->chan_info);
		bss_found = wl_escan_mesh_peer(mesh_if->dev, mesh_if->escan, &cur_ssid, cur_chan,
			sae, &peer_mesh_info);

		if (bss_found && peer_mesh_info.master_channel&&
				(cur_chan != peer_mesh_info.master_channel)) {
			WL_MSG(mesh_if->ifname, "moving channel %d -> %d\n",
				cur_chan, peer_mesh_info.master_channel);
			wl_ext_disable_iface(mesh_if->dev, mesh_if->ifname);
			mesh_if->chan_info.chan = peer_mesh_info.master_channel;
			wl_ext_enable_iface(mesh_if->dev, mesh_if->ifname, 500, TRUE);
		}
	}

exit:
	if (dump_buf)
		kfree(dump_buf);
	return updated;
}

static void
wl_mesh_event_handler(struct wl_if_info *cur_if,
	const wl_event_msg_t *e, void *data)
{
	struct dhd_pub *dhd = dhd_get_pub(cur_if->dev);
	struct wl_apsta_params *apsta_params = dhd->iapsta_params;
	struct wl_mesh_params *mesh_info = &apsta_params->mesh_info;
	uint32 event_type = ntoh32(e->event_type);
	uint32 status = ntoh32(e->status);
	uint32 reason = ntoh32(e->reason);
	uint16 flags =  ntoh16(e->flags);
	struct wl_if_info *mesh_if = NULL, *tmp_if = NULL;
	int ret, i;

	if (cur_if->ifmode == IMESH_MODE)
		mesh_if = cur_if;
	else {
		for (i=0; i<MAX_IF_NUM; i++) {
			tmp_if = &apsta_params->if_info[i];
			if (tmp_if->dev && tmp_if->ifmode == IMESH_MODE) {
				mesh_if = tmp_if;
				break;
			}
		}
	}

	if (cur_if->ifmode == ISTA_MODE || cur_if->ifmode == IGC_MODE) {
		if (event_type == WLC_E_LINK) {
			if (!(flags & WLC_EVENT_MSG_LINK)) {
				if (mesh_if && apsta_params->macs)
					wl_mesh_clear_mesh_info(mesh_if, TRUE);
			} else {
				if (mesh_if && apsta_params->macs)
					wl_mesh_update_master_info(apsta_params, mesh_if);
			}
		}
		else if (event_type == WLC_E_SET_SSID && status != WLC_E_STATUS_SUCCESS) {
			if (mesh_if && apsta_params->macs)
				wl_mesh_clear_mesh_info(mesh_if, TRUE);
		}
		else if (event_type == WLC_E_DEAUTH || event_type == WLC_E_DEAUTH_IND ||
				event_type == WLC_E_DISASSOC || event_type == WLC_E_DISASSOC_IND) {
			if (mesh_if && apsta_params->macs)
				wl_mesh_clear_mesh_info(mesh_if, TRUE);
		}
	}
	else if (cur_if->ifmode == IMESH_MODE && apsta_params->macs) {
		if (wl_get_isam_status(mesh_if, AP_CREATED) &&
				((event_type == WLC_E_SET_SSID && status == WLC_E_STATUS_SUCCESS) ||
				(event_type == WLC_E_LINK && status == WLC_E_STATUS_SUCCESS &&
				reason == WLC_E_REASON_INITIAL_ASSOC))) {
			if (!wl_mesh_update_master_info(apsta_params, mesh_if)) {
				mesh_info->scan_channel = wl_ext_get_chan(mesh_if->dev,
					&mesh_if->chan_info);
				wl_timer_mod(dhd, &mesh_if->delay_scan, WL_MESH_DELAY_SCAN_TMO);
			}
		}
		else if ((event_type == WLC_E_LINK && reason == WLC_E_LINK_BSSCFG_DIS) ||
				(event_type == WLC_E_LINK && status == WLC_E_STATUS_SUCCESS &&
				reason == WLC_E_REASON_DEAUTH)) {
			wl_mesh_clear_mesh_info(mesh_if, FALSE);
		}
		else if (wl_get_isam_status(mesh_if, AP_CREATED) &&
				(event_type == WLC_E_ASSOC_IND || event_type == WLC_E_REASSOC_IND) &&
				reason == DOT11_SC_SUCCESS) {
			mesh_info->scan_channel = wl_ext_get_chan(mesh_if->dev,
				&mesh_if->chan_info);
			wl_timer_mod(dhd, &mesh_if->delay_scan, 100);
		}
		else if (event_type == WLC_E_DISASSOC_IND || event_type == WLC_E_DEAUTH_IND ||
				(event_type == WLC_E_DEAUTH && reason != DOT11_RC_RESERVED)) {
			if (!memcmp(&mesh_info->peer_bssid, &e->addr, ETHER_ADDR_LEN))
				wl_mesh_clear_mesh_info(mesh_if, TRUE);
		}
		else if (wl_get_isam_status(mesh_if, AP_CREATED) &&
				event_type == WLC_E_RESERVED && reason == ISAM_RC_MESH_ACS) {
			if (!wl_mesh_update_master_info(apsta_params, mesh_if)) {
				wl_scan_info_t scan_info;
				memset(&scan_info, 0, sizeof(wl_scan_info_t));
				wl_ext_ioctl(mesh_if->dev, WLC_GET_SSID, &scan_info.ssid, sizeof(wlc_ssid_t), 0);
				if (mesh_info->scan_channel) {
					scan_info.channels.count = 1;
					// fix me: need to convert channel to host chanspec
					scan_info.channels.chanspec[0] = mesh_info->scan_channel;
				}
				ret = wl_escan_set_scan(mesh_if->dev, &scan_info);
				if (ret)
					wl_timer_mod(dhd, &mesh_if->delay_scan, WL_MESH_DELAY_SCAN_TMO);
			}
		}
		else if (wl_get_isam_status(mesh_if, AP_CREATED) &&
				((event_type == WLC_E_ESCAN_RESULT && status == WLC_E_STATUS_SUCCESS) ||
				(event_type == WLC_E_ESCAN_RESULT &&
				(status == WLC_E_STATUS_ABORT || status == WLC_E_STATUS_NEWSCAN ||
				status == WLC_E_STATUS_11HQUIET || status == WLC_E_STATUS_CS_ABORT ||
				status == WLC_E_STATUS_NEWASSOC || status == WLC_E_STATUS_TIMEOUT)))) {
			if (!wl_mesh_update_master_info(apsta_params, mesh_if)) {
				if (!wl_mesh_update_mesh_info(apsta_params, mesh_if)) {
					mesh_info->scan_channel = 0;
					wl_timer_mod(dhd, &mesh_if->delay_scan, WL_MESH_DELAY_SCAN_TMO);
				}
			}
		}
	}
}

static void
wl_mesh_escan_detach(dhd_pub_t *dhd, struct wl_if_info *mesh_if)
{
	IAPSTA_TRACE(mesh_if->dev->name, "Enter\n");
	wl_timer_deregister(mesh_if->dev, &mesh_if->delay_scan);
	if (mesh_if->escan) {
		mesh_if->escan = NULL;
	}
}

static int
wl_mesh_escan_attach(dhd_pub_t *dhd, struct wl_if_info *mesh_if)
{
	IAPSTA_TRACE(mesh_if->dev->name, "Enter\n");

	mesh_if->escan = dhd->escan;
	wl_timer_register(mesh_if->dev, &mesh_if->delay_scan, wl_mesh_timer);

	return 0;
}

static uint
wl_mesh_update_peer_path(struct wl_if_info *mesh_if, char *command,
	int total_len)
{
	struct wl_mesh_params peer_mesh_info;
	uint32 count = 0;
	char *dump_buf = NULL;
	mesh_peer_info_dump_t *peer_results;
	mesh_peer_info_ext_t *mpi_ext;
	int bytes_written = 0, j, k;
	bool bss_found = FALSE;

	dump_buf = kmalloc(WLC_IOCTL_MAXLEN, GFP_KERNEL);
	if (dump_buf == NULL) {
		IAPSTA_ERROR(mesh_if->dev->name, "Failed to allocate buffer of %d bytes\n",
			WLC_IOCTL_MAXLEN);
		return FALSE;
	}
	count = wl_mesh_get_peer_results(mesh_if->dev, dump_buf, WLC_IOCTL_MAXLEN);
	if (count > 0) {
		peer_results = (mesh_peer_info_dump_t *)dump_buf;
		mpi_ext = (mesh_peer_info_ext_t *)peer_results->mpi_ext;
		for (count = 0; count < peer_results->count; count++) {
			if (mpi_ext->entry_state != MESH_SELF_PEER_ENTRY_STATE_TIMEDOUT &&
					mpi_ext->peer_info.state == MESH_PEERING_ESTAB) {
				memset(&peer_mesh_info, 0, sizeof(struct wl_mesh_params));
				bss_found = wl_escan_mesh_info(mesh_if->dev, mesh_if->escan,
					&mpi_ext->ea, &peer_mesh_info);
				if (bss_found) {
					bytes_written += snprintf(command+bytes_written, total_len,
						"\npeer=%pM, hop=%d",
						&mpi_ext->ea, peer_mesh_info.hop_cnt);
					for (j=1; j<peer_mesh_info.hop_cnt; j++) {
						bytes_written += snprintf(command+bytes_written,
							total_len, "\n");
						for (k=0; k<j; k++) {
							bytes_written += snprintf(command+bytes_written,
								total_len, " ");
						}
						bytes_written += snprintf(command+bytes_written, total_len,
							"%pM", &peer_mesh_info.peer_bssid[j]);
					}
				}
			}
			mpi_ext++;
		}
	}

	if (dump_buf)
		kfree(dump_buf);
	return bytes_written;
}

int
wl_ext_isam_peer_path(struct net_device *dev, char *command, int total_len)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(dev);
	struct wl_mesh_params *mesh_info = &apsta_params->mesh_info;
	struct wl_if_info *tmp_if;
	char *dump_buf = NULL;
	int dump_len = WLC_IOCTL_MEDLEN;
	int dump_written = 0;
	int i;

	if (command || android_msg_level & ANDROID_INFO_LEVEL) {
		if (command) {
			dump_buf = command;
			dump_len = total_len;
		} else {
			dump_buf = kmalloc(dump_len, GFP_KERNEL);
			if (dump_buf == NULL) {
				IAPSTA_ERROR(dev->name, "Failed to allocate buffer of %d bytes\n",
					dump_len);
				return -1;
			}
		}
		for (i=0; i<MAX_IF_NUM; i++) {
			tmp_if = &apsta_params->if_info[i];
			if (tmp_if->dev && tmp_if->ifmode == IMESH_MODE && apsta_params->macs &&
					wl_ext_associated(tmp_if->dev, NULL)) {
				dump_written += snprintf(dump_buf+dump_written, dump_len,
					DHD_LOG_PREFIXS "[%s] mbssid=%pM, mchan=%d, hop=%d, pbssid=%pM",
					tmp_if->ifname, &mesh_info->master_bssid,
					mesh_info->master_channel, mesh_info->hop_cnt,
					&mesh_info->peer_bssid);
				dump_written += wl_mesh_update_peer_path(tmp_if,
					dump_buf+dump_written, dump_len-dump_written);
			}
		}
		IAPSTA_INFO(dev->name, "%s\n", dump_buf);
	}

	if (!command && dump_buf)
		kfree(dump_buf);
	return dump_written;
}
#endif /* WL_ESCAN */
#endif /* WLMESH */

static bool
wl_ext_mcc_chan(struct wl_if_info *cur_if, struct wl_ext_chan_info *cur_chan_info,
	struct wl_ext_chan_info *tgt_chan_info)
{
	uint band_x, band_y;
	int i;

	for (i=0; i<MCHAN_CAP_NUM; i++) {
		band_x = cur_if->mchan_cap[i].band_x;
		band_y = cur_if->mchan_cap[i].band_y;
		if ((band_x == cur_chan_info->band && band_y == tgt_chan_info->band) ||
				(band_x == tgt_chan_info->band && band_y == cur_chan_info->band)) {
			return cur_if->mchan_cap[i].mcc;
		}
	}
	return FALSE;
}

static bool
wl_ext_same_chan(struct wl_ext_chan_info *cur_chan_info,
	struct wl_ext_chan_info *tgt_chan_info)
{
	if (cur_chan_info->band == tgt_chan_info->band &&
			cur_chan_info->chan == tgt_chan_info->chan) {
		return TRUE;
	}
	return FALSE;
}

static bool
wl_ext_same_band(struct wl_ext_chan_info *cur_chan_info,
	struct wl_ext_chan_info *tgt_chan_info)
{
	if (cur_chan_info->band == tgt_chan_info->band &&
			cur_chan_info->band != WLC_BAND_AUTO) {
		return TRUE;
	}
	return FALSE;
}

static bool
wl_ext_rsdb_band(uint band_x, uint band_y)
{
	if ((band_x == WLC_BAND_2G && band_y != WLC_BAND_2G) ||
		(band_y == WLC_BAND_2G && band_x != WLC_BAND_2G)) {
		return TRUE;
	}
	return FALSE;
}

static uint16
wl_ext_get_same_band_chan(struct wl_if_info *cur_if, bool same_chan)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);
	struct wl_if_info *tmp_if;
	struct wl_ext_chan_info tmp_chan_info;
	wl_prio_t max_prio;
	uint16 chan = 0;
	int i;

	// find the max prio
	max_prio = cur_if->prio;
	for (i=0; i<MAX_IF_NUM; i++) {
		tmp_if = &apsta_params->if_info[i];
		if (cur_if != tmp_if && wl_get_isam_status(tmp_if, IF_READY) &&
				tmp_if->prio > max_prio) {
			if (wl_ext_get_chanspec(tmp_if->dev, &tmp_chan_info)) {
				if (wl_ext_same_chan(&cur_if->chan_info, &tmp_chan_info) ||
						(!same_chan &&
						wl_ext_same_band(&cur_if->chan_info, &tmp_chan_info))) {
					max_prio = tmp_if->prio;
					chan = tmp_chan_info.chan;
				}
			}
		}
	}

	return chan;
}

static int
wl_ext_if_down(struct wl_if_info *cur_if)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);
	s8 iovar_buf[WLC_IOCTL_SMLEN];
	scb_val_t scbval;
	struct {
		s32 cfg;
		s32 val;
	} bss_setbuf;
	apstamode_t apstamode = apsta_params->apstamode;
	char mac_buf[MAX_NUM_OF_ASSOCIATED_DEV *
		sizeof(struct ether_addr) + sizeof(uint)] = {0};
	struct maclist *assoc_maclist = (struct maclist *)mac_buf;
	int num_associated = 0, err;

	WL_MSG(cur_if->ifname, "Turning off...\n");

	if (cur_if->ifmode == ISTA_MODE) {
		wl_ext_ioctl(cur_if->dev, WLC_DISASSOC, NULL, 0, 1);
		return 0;
	} else if (cur_if->ifmode == IAP_MODE || cur_if->ifmode == IMESH_MODE) {
		// deauthenticate all STA first
		assoc_maclist->count = MAX_NUM_OF_ASSOCIATED_DEV;
		err = wldev_ioctl_get(cur_if->dev, WLC_GET_ASSOCLIST,
			assoc_maclist, sizeof(mac_buf));
		if (err < 0)
			IAPSTA_ERROR(cur_if->ifname, "WLC_GET_ASSOCLIST error %d\n", err);
		else
			num_associated = assoc_maclist->count;
		memcpy(scbval.ea.octet, &ether_bcast, ETHER_ADDR_LEN);
		wl_ext_ioctl(cur_if->dev, WLC_SCB_DEAUTHENTICATE, &scbval.ea, ETHER_ADDR_LEN, 1);
		if (num_associated > 0)
			OSL_SLEEP(400);
	}

	if (apstamode == IAPONLY_MODE || apstamode == IMESHONLY_MODE) {
		wl_ext_ioctl(cur_if->dev, WLC_DOWN, NULL, 0, 1);
	} else {
		bss_setbuf.cfg = htod32(-1);
		bss_setbuf.val = htod32(0);
		wl_ext_iovar_setbuf(cur_if->dev, "bss", &bss_setbuf, sizeof(bss_setbuf),
			iovar_buf, WLC_IOCTL_SMLEN, NULL);
	}
	wl_clr_isam_status(cur_if, AP_CREATED);

	return 0;
}

static bool
wl_ext_if_up(struct wl_if_info *cur_if, bool force_enable, int wait_up)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);
	struct wl_ext_chan_info *chan_info = &cur_if->chan_info;
	s8 iovar_buf[WLC_IOCTL_SMLEN];
	struct {
		s32 cfg;
		s32 val;
	} bss_setbuf;
	apstamode_t apstamode = apsta_params->apstamode;
	u32 timeout;
	wlc_ssid_t ssid = { 0, {0} };
	uint32 chanspec = 0;

	if (cur_if->ifmode != IAP_MODE && cur_if->ifmode != IGO_MODE) {
		IAPSTA_ERROR(cur_if->ifname, "Wrong ifmode\n");
		return FALSE;
	}

	chanspec = wf_create_chspec_from_primary(chan_info->chan,
		WL_CHANSPEC_BW_20, wl_ext_wlcband_to_chanspec_band(chan_info->band));

	if (wl_ext_dfs_chan(chanspec)) {
		if (!apsta_params->radar && !force_enable &&
				!wl_ext_get_same_band_chan(cur_if, TRUE)) {
			WL_MSG(cur_if->ifname, "skip DFS channel %s-%d\n",
				WLCBAND2STR(chan_info->band), chan_info->chan);
			return FALSE;
		}
	} else if (wl_ext_passive_chan(cur_if->dev, chanspec)) {
		WL_MSG(cur_if->ifname, "skip PASSIVE channel %s-%d\n",
			WLCBAND2STR(chan_info->band), chan_info->chan);
		return FALSE;
	} else if (!chan_info->chan) {
		WL_MSG(cur_if->ifname, "no valid channel\n");
		return FALSE;
	}

	WL_MSG(cur_if->ifname, "Turning on...\n");

	wl_ext_set_chanspec(cur_if->dev, chan_info, NULL);

	wl_clr_isam_status(cur_if, AP_CREATED);
	wl_set_isam_status(cur_if, AP_CREATING);
	if (apstamode == IAPONLY_MODE) {
		wl_ext_ioctl(cur_if->dev, WLC_UP, NULL, 0, 1);
	} else {
		bss_setbuf.cfg = htod32(-1);
		bss_setbuf.val = htod32(1);
		wl_ext_iovar_setbuf(cur_if->dev, "bss", &bss_setbuf,
			sizeof(bss_setbuf), iovar_buf, WLC_IOCTL_SMLEN, NULL);
	}

	if (wait_up) {
		OSL_SLEEP(wait_up);
	} else {
		timeout = wait_event_interruptible_timeout(apsta_params->netif_change_event,
			wl_get_isam_status(cur_if, AP_CREATED),
			msecs_to_jiffies(MAX_AP_LINK_WAIT_TIME));
		if (timeout <= 0 || !wl_get_isam_status(cur_if, AP_CREATED)) {
			wl_ext_if_down(cur_if);
			WL_MSG(cur_if->ifname, "failed to up with SSID: \"%s\"\n",
				cur_if->ssid);
		}
	}

	wl_ext_ioctl(cur_if->dev, WLC_GET_SSID, &ssid, sizeof(ssid), 0);
	chanspec = wl_ext_get_chanspec(cur_if->dev, chan_info);
	WL_MSG(cur_if->ifname, "enabled with SSID: \"%s\" on channel %s-%d(0x%x)\n",
		ssid.SSID, WLCBAND2STR(chan_info->band), chan_info->chan, chanspec);

	wl_clr_isam_status(cur_if, AP_CREATING);
	wl_ext_isam_status(cur_if->dev, NULL, 0);

	return TRUE;
}

static bool
wl_ext_rsdb_core_conflict(struct wl_if_info *cur_if)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);
	struct wl_if_info *tmp_if;
	struct wl_ext_chan_info cur_chan_info, *tgt_chan_info, tmp_chan_info;
	int i;

	if (apsta_params->rsdb && wl_ext_get_chanspec(cur_if->dev, &cur_chan_info)) {
		tgt_chan_info = &cur_if->chan_info;
		for (i=0; i<MAX_IF_NUM; i++) {
			tmp_if = &apsta_params->if_info[i];
			if (tmp_if != cur_if && wl_get_isam_status(tmp_if, IF_READY) &&
					tmp_if->prio > cur_if->prio) {
				if (wl_ext_get_chanspec(tmp_if->dev, &tmp_chan_info)) {
					if (wl_ext_rsdb_band(cur_chan_info.band, tmp_chan_info.band) &&
							wl_ext_rsdb_band(cur_chan_info.band, tgt_chan_info->band))
						return TRUE;
					else if (!wl_ext_rsdb_band(cur_chan_info.band, tmp_chan_info.band) &&
							wl_ext_rsdb_band(cur_chan_info.band, tgt_chan_info->band))
						return TRUE;
				}
			}
		}
	}
	return FALSE;
}

static bool
wl_ext_trigger_csa(struct wl_if_info *cur_if)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);
	struct dhd_pub *dhd = dhd_get_pub(cur_if->dev);
	wl_chan_switch_t csa_arg;
	s8 iovar_buf[WLC_IOCTL_SMLEN];
	bool core_conflict = FALSE;
	uint32 chanspec = 0;
	bool passive = FALSE;
	int wpa_auth = 0, ret = 0;
	char sec[64];
	u32 timeout;
	static int reset_cnt = CSA_RESET_CNT_MAX;

	if (wl_ext_master_if(cur_if) && apsta_params->csa) {
		chanspec = wf_create_chspec_from_primary(cur_if->chan_info.chan,
			WL_CHANSPEC_BW_20, wl_ext_wlcband_to_chanspec_band(cur_if->chan_info.band));
		passive = wl_ext_passive_chan(cur_if->dev, chanspec);
		wl_ext_iovar_getint(cur_if->dev, "wpa_auth", &wpa_auth);
		if (!cur_if->chan_info.chan) {
			WL_MSG(cur_if->ifname, "not valid channel\n");
		}
		else if (passive) {
			WL_MSG(cur_if->ifname, "skip passive channel %s-%d\n",
				WLCBAND2STR(cur_if->chan_info.band), cur_if->chan_info.chan);
		}
		else if ((cur_if->chan_info.band == WLC_BAND_6G) &&
				!(wpa_auth & WPA3_AUTH_SAE_PSK)) {
			wl_ext_get_sec(cur_if->dev, sec, sizeof(sec), TRUE);
			WL_MSG(cur_if->ifname, "skip %s-%d for non-WPA3 sec=%-20s\n",
				WLCBAND2STR(cur_if->chan_info.band), cur_if->chan_info.chan, sec);
		} else {
			memset(&csa_arg, 0, sizeof(csa_arg));
			csa_arg.mode = DOT11_CSA_MODE_ADVISORY;
			csa_arg.count = 3;
			csa_arg.frame_type = CSA_UNICAST_ACTION_FRAME;
			csa_arg.chspec = wl_ext_chan_to_chanspec(cur_if->dev, &cur_if->chan_info);
			core_conflict = wl_ext_rsdb_core_conflict(cur_if);
			if (core_conflict) {
				WL_MSG(cur_if->ifname, "skip CSA due to rsdb core conflict\n");
			} else if (csa_arg.chspec) {
				WL_MSG(cur_if->ifname, "CSA(%d/%d/%d) to channel %s-%d(%sMHz)\n",
					csa_arg.mode, csa_arg.count, csa_arg.frame_type,
					WLCBAND2STR(cur_if->chan_info.band), cur_if->chan_info.chan,
					wf_chspec_to_bw_str(csa_arg.chspec));
				wl_set_isam_status(cur_if, AP_CREATING);
				cur_if->csa_complete = FALSE;
				ret = wl_ext_iovar_setbuf(cur_if->dev, "csa", &csa_arg, sizeof(csa_arg),
					iovar_buf, sizeof(iovar_buf), NULL);
				if (ret && ret != BCME_UNSUPPORTED) {
					if (reset_cnt > 0) {
						WL_MSG(cur_if->ifname, "CSA %dms later cnt=%d\n",
							CSA_RESET_INTERVAL_MS, reset_cnt);
						wl_timer_mod(dhd, &apsta_params->reset_itf_timer,
							CSA_RESET_INTERVAL_MS);
					} else {
						IAPSTA_ERROR(cur_if->ifname, "reset_cnt=%d\n", reset_cnt);
					}
					reset_cnt--;
				} else if (!ret) {
					reset_cnt = CSA_RESET_CNT_MAX;
					timeout = wait_event_interruptible_timeout(cur_if->csa_complete_event,
						cur_if->csa_complete,
						msecs_to_jiffies(((csa_arg.count+2) * 100)));
					if (timeout <= 0 || !cur_if->csa_complete) {
						IAPSTA_INFO(cur_if->ifname, "CSA not complete?\n");
					}
				}
				wl_ext_isam_status(cur_if->dev, NULL, 0);
				wl_clr_isam_status(cur_if, AP_CREATING);
			} else {
				IAPSTA_ERROR(cur_if->ifname, "fail to get chanspec\n");
			}
		}
	}

	if (ret == BCME_UNSUPPORTED) {
		WL_MSG(cur_if->ifname, "CSA not supported\n");
		return FALSE;
	}

	return TRUE;
}

static bool
wl_ext_passive(struct net_device *net, struct wl_ext_chan_info *tgt_chan_info)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(net);
	uint32 chanspec = 0;
	bool passive = FALSE;

	chanspec = wf_create_chspec_from_primary(tgt_chan_info->chan,
		WL_CHANSPEC_BW_20, wl_ext_wlcband_to_chanspec_band(tgt_chan_info->band));
	if (wl_ext_dfs_chan(chanspec)) {
		if (!apsta_params->radar)
			passive = TRUE;
	}
	else if (wl_ext_passive_chan(net, chanspec))
		passive = TRUE;

	return passive;
}

static bool
wl_ext_move_chan(struct wl_if_info *cur_if,
	struct wl_ext_chan_info *cur_chan_info, struct wl_ext_chan_info *tgt_chan_info)
{
	bool mcc = FALSE, scc = FALSE, move_chan = FALSE;

	if (wl_ext_same_chan(cur_chan_info, tgt_chan_info))
		scc = TRUE;
	else if (wl_ext_mcc_chan(cur_if, cur_chan_info, tgt_chan_info))
		mcc = TRUE;
	else
		move_chan = TRUE;
	IAPSTA_INFO(cur_if->ifname, "[%s] cur_chan=%s-%d, tgt_chan=%s-%d, %s=%d\n",
		MCHANMODE2STR(cur_if->mchan_mode),
		WLCBAND2STR(cur_chan_info->band), cur_chan_info->chan,
		WLCBAND2STR(tgt_chan_info->band), tgt_chan_info->chan,
		scc ? "scc" : "mcc", scc ? 1 : mcc);

	return move_chan;
}

#ifdef AVOID_MASTER_6G
static bool
wl_ext_move_cur_6g_channel(struct wl_if_info *cur_if,
	struct wl_if_info *tgt_if, struct wl_ext_chan_info *tgt_chan_info)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);
	struct wl_ext_chan_info cur_chan_info;
	int ret = 0, mapsta_mode = MCHAN_APSTA_NOT_ALLOW;
	uint16 new_chan = 0;
	bool move_chan = TRUE;

	/* We should avoid AP mode moving to 6g channel if it is not configured to WPA3 only mode.
	 * But how to check it will be configured to WPA3 only mode if cur_if is not connected yet?
	*/

	if (tgt_chan_info->band == WLC_BAND_6G &&
			cur_if->chan_info.band != WLC_BAND_6G && wl_ext_master_if(cur_if)) {
		if (apsta_params->rsdb) {
			memcpy(&cur_chan_info, &cur_if->chan_info, sizeof(struct wl_ext_chan_info));
			wl_ext_set_chan_info(cur_if, WLC_BAND_2G, 6);
			/* We need to check if it is configured to mcc between 2g and 6g before moving to 2g */
			if (wl_ext_mcc_chan(cur_if, &cur_if->chan_info, tgt_chan_info)) {
				new_chan = wl_ext_get_same_band_chan(cur_if, FALSE);
				if (new_chan)
					cur_if->chan_info.chan = new_chan;
				IAPSTA_INFO(cur_if->ifname, "move %s-%d => %s-%d\n",
					WLCBAND2STR(cur_chan_info.band), cur_chan_info.chan,
					WLCBAND2STR(cur_if->chan_info.band), cur_if->chan_info.chan);
			}
		} else {
			/* For non-RSDB chip, don't move to 6g and chip should support VSDB and
			 * set wl_preinit=mapsta_mode=2 (MCHAN_APSTA_NO_RESTRICT).
			*/
			if (cur_if->ifmode == IAP_MODE)
				ret = wldev_iovar_getint(cur_if->dev, "mapsta_mode", &mapsta_mode);
			else
				mapsta_mode = MCHAN_APSTA_NO_RESTRICT;

			if (ret || mapsta_mode == MCHAN_APSTA_NOT_ALLOW)
				IAPSTA_INFO(cur_if->ifname, "mapsta_mode=%d, ret=%d\n",
					mapsta_mode, ret);
			else {
				move_chan = FALSE;
				IAPSTA_INFO(cur_if->ifname, "keep on %s-%d\n",
					WLCBAND2STR(tgt_chan_info->band), tgt_chan_info->chan);
			}
		}
	}

	return move_chan;
}

static bool
wl_ext_move_other_6g_channel(struct wl_if_info *cur_if,
	struct wl_if_info *tgt_if, struct wl_ext_chan_info *tgt_chan_info)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);
	int ret = 0, mapsta_mode = MCHAN_APSTA_NOT_ALLOW;
	int wpa_auth = 0;
	uint16 new_chan = 0;
	bool move_chan = TRUE;

	/* We should avoid AP mode moving to 6g channel if it is not configured to WPA3 only mode. */

	if (cur_if->chan_info.band == WLC_BAND_6G && wl_ext_master_if(tgt_if)) {
		wl_ext_iovar_getint(tgt_if->dev, "wpa_auth", &wpa_auth);
		if (wpa_auth != WPA3_AUTH_SAE_PSK) {
			if (apsta_params->rsdb) {
				/* For RSDB chip to get better performance, we need to move tgt_if to 2g if tgt_if
				 * channel is on 5g.
				*/
				if (tgt_chan_info->chan && tgt_chan_info->band == WLC_BAND_5G) {
					wl_ext_set_chan_info(tgt_if, WLC_BAND_2G, 6);
					/* We need to check if it is configured to mcc between 2g and 6g before
					 * moving to 2g
					 */
					if (wl_ext_mcc_chan(tgt_if, &tgt_if->chan_info, &cur_if->chan_info)) {
						new_chan = wl_ext_get_same_band_chan(tgt_if, FALSE);
						if (new_chan)
							tgt_if->chan_info.chan = new_chan;
						IAPSTA_INFO(tgt_if->ifname, "move %s-%d => %s-%d\n",
							WLCBAND2STR(tgt_chan_info->band), tgt_chan_info->chan,
							WLCBAND2STR(tgt_if->chan_info.band), tgt_if->chan_info.chan);
					}
				}
			} else {
				/* For non-RSDB chip, don't move to 6g and chip should support mapsta_mode and
				 * set wl_preinit=mapsta_mode=2 (MCHAN_APSTA_NO_RESTRICT).
				*/
				if (cur_if->ifmode == IAP_MODE)
					ret = wldev_iovar_getint(tgt_if->dev, "mapsta_mode", &mapsta_mode);
				else
					mapsta_mode = MCHAN_APSTA_NO_RESTRICT;

				if (ret || mapsta_mode == MCHAN_APSTA_NOT_ALLOW)
					IAPSTA_INFO(tgt_if->ifname, "mapsta_mode=%d, ret=%d\n",
						mapsta_mode, ret);
				else {
					memcpy(&tgt_if->chan_info, tgt_chan_info, sizeof(struct wl_ext_chan_info));
					move_chan = FALSE;
					IAPSTA_INFO(tgt_if->ifname, "keep on %s-%d\n",
						WLCBAND2STR(tgt_chan_info->band), tgt_chan_info->chan);
				}
			}
		}
	}

	return move_chan;
}
#endif /* AVOID_MASTER_6G */

static void
wl_ext_move_passive_channel(struct wl_if_info *cur_if)
{
#ifdef NOT_USED
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);
#endif
	struct wl_ext_chan_info *cur_chan_info = &cur_if->chan_info;
	uint cur_band = cur_chan_info->band;
	uint16 cur_chan = cur_chan_info->chan, new_chan = 0;
	uint16 chan_2g = 0, chan_5g = 0, chan_6g = 0;

	if (wl_ext_master_if(cur_if) && wl_ext_passive(cur_if->dev, &cur_if->chan_info)) {
		wl_ext_get_default_chan(cur_if->dev, &chan_2g, &chan_5g, &chan_6g);
		if (cur_band == WLC_BAND_5G && chan_5g)
			wl_ext_set_chan_info(cur_if, WLC_BAND_5G, chan_5g);
		else if (chan_2g)
			wl_ext_set_chan_info(cur_if, WLC_BAND_2G, chan_2g);
		else {
			cur_chan_info->chan = 0;
			WL_MSG(cur_if->ifname, "no valid channel on cur_band=%d\n", cur_band);
			return;
		}

		new_chan = wl_ext_get_same_band_chan(cur_if, FALSE);
#ifdef NOT_USED
		if (!new_chan) {
			mutex_unlock(&apsta_params->in4way_sync);
			new_chan = wl_ext_autochannel(cur_if->dev, ACS_DRV_BIT,
				cur_chan_info->band, NULL);
			mutex_lock(&apsta_params->in4way_sync);
			new_chan = wf_chspec_ctlchan(new_chan);
		}
		cur_chan_info->chan = new_chan;
#else
		if (new_chan)
			cur_chan_info->chan = new_chan;
#endif
		WL_MSG(cur_if->ifname, "cur_chan = %s-%d => %s-%d\n",
			WLCBAND2STR(cur_band), cur_chan,
			WLCBAND2STR(cur_chan_info->band), cur_chan_info->chan);
	}
}

static void
wl_ext_move_cur_channel(struct wl_if_info *cur_if)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);
	struct wl_if_info *tmp_if, *tgt_if = NULL;
	struct wl_ext_chan_info cur_chan_info, tgt_chan_info, tmp_chan_info;
	wl_prio_t max_prio;
	int i;

	if (apsta_params->mcc) {
		goto exit;
	}

	memcpy(&cur_chan_info, &cur_if->chan_info, sizeof(struct wl_ext_chan_info));
	memset(&tgt_chan_info, 0, sizeof(struct wl_ext_chan_info));

	/* find the max prio and the prio is bigger than cur_if */
	max_prio = cur_if->prio;
	for (i=0; i<MAX_IF_NUM; i++) {
		tmp_if = &apsta_params->if_info[i];
		if (cur_if != tmp_if && wl_get_isam_status(tmp_if, IF_READY) &&
				tmp_if->prio > max_prio) {
			if (wl_ext_get_chanspec(tmp_if->dev, &tmp_chan_info) &&
					wl_ext_move_chan(cur_if, &cur_chan_info, &tmp_chan_info)) {
				tgt_if = tmp_if;
				max_prio = tmp_if->prio;
				memcpy(&tgt_chan_info, &tmp_chan_info, sizeof(struct wl_ext_chan_info));
			}
		}
	}

	if (tgt_if) {
#ifdef AVOID_MASTER_6G
		if (!wl_ext_move_cur_6g_channel(cur_if, tgt_if, &tgt_chan_info)) {
			WL_MSG(cur_if->ifname, "keep on %s-%d\n",
				WLCBAND2STR(cur_chan_info.band), cur_chan_info.chan);
			return;
		}
		else if (!wl_ext_same_chan(&cur_if->chan_info, &cur_chan_info)) {
			WL_MSG(cur_if->ifname, "move %s-%d => %s-%d\n",
				WLCBAND2STR(cur_chan_info.band), cur_chan_info.chan,
				WLCBAND2STR(cur_if->chan_info.band), cur_if->chan_info.chan);
		} else
#endif /* AVOID_MASTER_6G */
		{
			WL_MSG(cur_if->ifname, "cur_chan=%s-%d => [%s] tgt_chan=%s-%d\n",
				WLCBAND2STR(cur_chan_info.band), cur_chan_info.chan,
				tgt_if->ifname,
				WLCBAND2STR(tgt_chan_info.band), tgt_chan_info.chan);
			wl_ext_set_chan_info(cur_if, tgt_chan_info.band, tgt_chan_info.chan);
		}
	}

exit:
	if (!tgt_if)
		wl_ext_move_passive_channel(cur_if);
}

static void
wl_ext_move_other_channel(struct wl_if_info *cur_if)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);
	struct wl_if_info *tmp_if, *tgt_if = NULL;
	struct wl_ext_chan_info cur_chan_info, tgt_chan_info, tmp_chan_info;
	wl_prio_t max_prio = 0, cur_prio;
	int i;

	if (apsta_params->mcc || !cur_if->chan_info.chan) {
		return;
	}

	memcpy(&cur_chan_info, &cur_if->chan_info, sizeof(struct wl_ext_chan_info));
	memset(&tgt_chan_info, 0, sizeof(struct wl_ext_chan_info));

	/* find the max prio, but lower or equal than cur_if */
	cur_prio = cur_if->prio;
	for (i=0; i<MAX_IF_NUM; i++) {
		tmp_if = &apsta_params->if_info[i];
		if (cur_if != tmp_if && wl_get_isam_status(tmp_if, IF_READY) &&
				tmp_if->prio >= max_prio && tmp_if->prio <= cur_prio) {
			if (wl_ext_get_chanspec(tmp_if->dev, &tmp_chan_info) &&
					wl_ext_move_chan(tmp_if, &tmp_chan_info, &cur_chan_info)) {
				tgt_if = tmp_if;
				max_prio = tmp_if->prio;
				memcpy(&tgt_chan_info, &tmp_chan_info, sizeof(struct wl_ext_chan_info));
			}
		}
	}

	if (tgt_if) {
		/* skip moving tgt_if while there is other interface's prio is higher than cur_if */
		max_prio = cur_if->prio;
		for (i=0; i<MAX_IF_NUM; i++) {
			tmp_if = &apsta_params->if_info[i];
			if (tgt_if != tmp_if && wl_get_isam_status(tmp_if, IF_READY) &&
					tmp_if->prio > max_prio) {
				if (wl_ext_get_chanspec(tmp_if->dev, &tmp_chan_info) &&
						wl_ext_same_chan(&tgt_chan_info, &tmp_chan_info)) {
					WL_MSG(tgt_if->ifname, "skip moving channel to [%s] chan=%s-%d\n",
						cur_if->ifname,
						WLCBAND2STR(cur_if->chan_info.band), cur_if->chan_info.chan);
					tgt_if = NULL;
				}
				if (!tgt_if)
					break;
			}
		}
	}

	if (tgt_if && !wl_ext_associated(cur_if->dev, NULL) &&
			wl_ext_passive(tgt_if->dev, &cur_if->chan_info)) {
		WL_MSG(tgt_if->ifname, "move to passive chan=%s-%d after %s enabled\n",
			WLCBAND2STR(cur_if->chan_info.band), cur_if->chan_info.chan,
			cur_if->ifname);
		tgt_if = NULL;
	}

	if (tgt_if) {
#ifdef AVOID_MASTER_6G
		if (!wl_ext_move_other_6g_channel(cur_if, tgt_if, &tgt_chan_info)) {
			WL_MSG(tgt_if->ifname, "keep on %s-%d\n",
				WLCBAND2STR(tgt_chan_info.band), tgt_chan_info.chan);
			return;
		}
		else if (!wl_ext_same_chan(&tgt_if->chan_info, &tgt_chan_info)) {
			WL_MSG(tgt_if->ifname, "move %s-%d => %s-%d\n",
				WLCBAND2STR(tgt_chan_info.band), tgt_chan_info.chan,
				WLCBAND2STR(tgt_if->chan_info.band), tgt_if->chan_info.chan);
		} else
#endif /* AVOID_MASTER_6G */
		{
			WL_MSG(tgt_if->ifname, "cur_chan=%s-%d => [%s] tgt_chan=%s-%d\n",
				WLCBAND2STR(tgt_chan_info.band), tgt_chan_info.chan,
				cur_if->ifname,
				WLCBAND2STR(cur_if->chan_info.band), cur_if->chan_info.chan);
			wl_ext_set_chan_info(tgt_if, cur_if->chan_info.band, cur_if->chan_info.chan);
		}

		if (apsta_params->csa && wl_ext_trigger_csa(tgt_if)) {
			wl_ext_move_other_channel(tgt_if);
		} else {
			wl_ext_if_down(tgt_if);
			wl_ext_move_other_channel(tgt_if);
			if (tgt_if->ifmode == IAP_MODE || tgt_if->ifmode == IGO_MODE)
				wl_ext_if_up(tgt_if, FALSE, 0);
#ifdef ISAM_CONFIG
			else if (tgt_if->ifmode == IMESH_MODE)
				wl_ext_enable_iface(tgt_if->dev, tgt_if->ifname, 0, FALSE);
#endif
		}
	}
}

bool
wl_ext_iapsta_other_if_associated(struct net_device *net)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(net);
	struct wl_if_info *tmp_if;
	bool enabled = FALSE;
	int i;

	for (i=0; i<MAX_IF_NUM; i++) {
		tmp_if = &apsta_params->if_info[i];
		if (tmp_if && wl_get_isam_status(tmp_if, IF_READY) &&
				wl_ext_associated(tmp_if->dev, NULL)) {
			enabled = TRUE;
			break;
		}
	}

	return enabled;
}

bool
wl_ext_iapsta_other_if_enabled(struct net_device *net)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(net);
	struct wl_if_info *tmp_if;
	bool enabled = FALSE;
	int i;

	for (i=0; i<MAX_IF_NUM; i++) {
		tmp_if = &apsta_params->if_info[i];
		if (tmp_if && wl_get_isam_status(tmp_if, IF_READY)) {
			if ((wl_ext_slave_if(tmp_if) && wl_get_isam_status(tmp_if, STA_CONNECTED)) ||
					(wl_ext_master_if(tmp_if) && wl_get_isam_status(tmp_if, AP_CREATED))) {
				enabled = TRUE;
				break;
			}
		}
	}

	return enabled;
}

bool
wl_ext_sta_connecting(struct net_device *dev)
{
	struct wl_if_info *cur_if = NULL;
	bool connecting = FALSE;
	int state;

	cur_if = wl_get_cur_if(dev);
	if (!cur_if)
		return FALSE;

	if (!wl_ext_slave_if(cur_if))
		return FALSE;

	state = cur_if->conn_state;
	if (state >= CONN_STATE_CONNECTING && state < CONN_STATE_CONNECTED) {
		connecting = TRUE;
		IAPSTA_TRACE(dev->name, "conn_state %d\n", state);
	}

	return connecting;
}

bool
wl_ext_sta_handshaking(struct net_device *dev)
{
	struct wl_if_info *cur_if = NULL;
	bool connecting = FALSE;
	int state;

	cur_if = wl_get_cur_if(dev);
	if (!cur_if)
		return FALSE;

	if (!wl_ext_slave_if(cur_if))
		return FALSE;

	state = cur_if->conn_state;
	if (state >= CONN_STATE_4WAY_M1 && state < CONN_STATE_CONNECTED) {
		connecting = TRUE;
		IAPSTA_TRACE(dev->name, "conn_state %d\n", state);
	}

	return connecting;
}

bool
wl_ext_sta_connected(struct net_device *dev)
{
	struct wl_if_info *cur_if = NULL;
	bool connected = FALSE;
	int state;

	cur_if = wl_get_cur_if(dev);
	if (!cur_if)
		return FALSE;

	if (!wl_ext_slave_if(cur_if))
		return FALSE;

	state = cur_if->conn_state;
	if (state >= CONN_STATE_CONNECTED) {
		connected = TRUE;
		IAPSTA_TRACE(dev->name, "conn_state %d\n", state);
	}

	return connected;
}

#ifdef DHD_LOSSLESS_ROAMING
int
wl_ext_any_sta_handshaking(struct dhd_pub *dhd)
{
	struct wl_apsta_params *apsta_params = dhd->iapsta_params;
	struct wl_if_info *cur_if;
	int state = 0, i;

	for (i=0; i<MAX_IF_NUM; i++) {
		cur_if = &apsta_params->if_info[i];
		if (cur_if->dev && cur_if->ifmode == ISTA_MODE) {
			state = cur_if->conn_state;
			if (state >= CONN_STATE_4WAY_M1 && state < CONN_STATE_CONNECTED) {
				return state;
			}
		}
	}
	return 0;
}
#endif /* DHD_LOSSLESS_ROAMING */

#ifdef PROPTX_MAXCOUNT
int
wl_ext_get_wlfc_maxcount(struct dhd_pub *dhd, int ifidx)
{
	struct wl_apsta_params *apsta_params = dhd->iapsta_params;
	struct wl_if_info *tmp_if, *cur_if = NULL;
	int i, maxcount = WL_TXSTATUS_FREERUNCTR_MASK;

	if (!apsta_params->rsdb)
		return maxcount;

	for (i=0; i<MAX_IF_NUM; i++) {
		tmp_if = &apsta_params->if_info[i];
		if (tmp_if->dev && tmp_if->ifidx == ifidx) {
			cur_if = tmp_if;
			maxcount = cur_if->transit_maxcount;
		}
	}

	if (cur_if)
		IAPSTA_INFO(cur_if->ifname, "update maxcount %d\n", maxcount);
	else
		IAPSTA_INFO("wlan", "update maxcount %d for ifidx %d\n", maxcount, ifidx);
	return maxcount;
}

static void
wl_ext_update_wlfc_maxcount(struct dhd_pub *dhd)
{
	struct wl_apsta_params *apsta_params = dhd->iapsta_params;
	struct wl_if_info *tmp_if;
	struct wl_ext_chan_info chan_info;
	bool band_5g = FALSE;
	int i, ret;

	if (!apsta_params->rsdb)
		return;

	for (i=0; i<MAX_IF_NUM; i++) {
		tmp_if = &apsta_params->if_info[i];
		if (tmp_if->dev) {
			if (wl_ext_get_chanspec(tmp_if->dev, &chan_info)) {
				if (chan_info.band == WLC_BAND_5G || chan_info.band == WLC_BAND_6G) {
					tmp_if->transit_maxcount = dhd->conf->proptx_maxcnt_5g;
					ret = dhd_wlfc_update_maxcount(dhd, tmp_if->ifidx,
						tmp_if->transit_maxcount);
					if (ret == 0)
						IAPSTA_INFO(tmp_if->ifname, "updated maxcount %d\n",
							tmp_if->transit_maxcount);
					band_5g = TRUE;
				}
			}
		}
	}

	for (i=0; i<MAX_IF_NUM; i++) {
		tmp_if = &apsta_params->if_info[i];
		if (tmp_if->dev) {
			wl_ext_get_chanspec(tmp_if->dev, &chan_info);
			if ((chan_info.chan == 0) || (chan_info.band == WLC_BAND_2G)) {
				if (chan_info.chan == 0) {
					tmp_if->transit_maxcount = WL_TXSTATUS_FREERUNCTR_MASK;
				} else if (band_5g) {
					tmp_if->transit_maxcount = dhd->conf->proptx_maxcnt_2g;
				} else {
					tmp_if->transit_maxcount = dhd->conf->proptx_maxcnt_5g;
				}
				ret = dhd_wlfc_update_maxcount(dhd, tmp_if->ifidx,
					tmp_if->transit_maxcount);
				if (ret == 0)
					IAPSTA_INFO(tmp_if->ifname, "updated maxcount %d\n",
						tmp_if->transit_maxcount);
			}
		}
	}
}
#endif /* PROPTX_MAXCOUNT */

#ifdef WL_CFG80211
struct net_device *
wl_ext_max_prio_enabled_if(struct net_device *dev)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(dev);
	struct wl_if_info *tgt_if = NULL, *tmp_if = NULL;
	struct net_device *tgt_dev = NULL;
	wl_prio_t max_prio = 0;
	int i;

	for (i=0; i<MAX_IF_NUM; i++) {
		tmp_if = &apsta_params->if_info[i];
		if (tmp_if && tgt_if != tmp_if && wl_get_isam_status(tmp_if, IF_READY) &&
				tmp_if->prio > max_prio && wl_ext_associated(tmp_if->dev, NULL)) {
			tgt_if = tmp_if;
			max_prio = tmp_if->prio;
			tgt_dev = tmp_if->dev;
		}
	}

	return tgt_dev;
}

static void
wl_ext_reset_master(struct wl_if_info *cur_if)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);
	struct wl_ext_chan_info cur_chan_info;

	memcpy(&cur_chan_info, &cur_if->chan_info, sizeof(struct wl_ext_chan_info));
	wl_ext_move_cur_channel(cur_if);
	if (cur_if->chan_info.chan &&
			!wl_ext_same_chan(&cur_chan_info, &cur_if->chan_info)) {
		if (apsta_params->csa && wl_ext_trigger_csa(cur_if)) {
			wl_ext_move_other_channel(cur_if);
		} else {
			wl_ext_if_down(cur_if);
			wl_ext_move_other_channel(cur_if);
			wl_ext_if_up(cur_if, TRUE, 0);
		}
	} else {
		wl_ext_move_other_channel(cur_if);
	}
}

static void
wl_ext_sync_interface_timeout(unsigned long data)
{
	struct net_device *dev = (struct net_device *)data;
	struct dhd_pub *dhd;
	wl_event_msg_t msg;

	if (!dev) {
		IAPSTA_ERROR("wlan", "dev is not ready\n");
		return;
	}

	dhd = dhd_get_pub(dev);

	bzero(&msg, sizeof(wl_event_msg_t));
	IAPSTA_TRACE(dev->name, "timer expired\n");

	msg.ifidx = dhd_net2idx(dhd->info, dev);
	msg.event_type = hton32(WLC_E_RESERVED);
	msg.reason = hton32(ISAM_RC_RESET_ITF);
	wl_ext_event_send(dhd->event_params, &msg, NULL);
}

static void
wl_ext_sync_interface(struct net_device *dev)
{
	struct net_device *cur_dev = NULL;
	struct wl_if_info *cur_if = NULL;

	cur_dev = wl_ext_max_prio_enabled_if(dev);
	if (!cur_dev)
		return;
	cur_if = wl_get_cur_if(cur_dev);
	if (!cur_if)
		return;

	if (wl_ext_get_chanspec(cur_if->dev, &cur_if->chan_info)) {
		if (wl_ext_slave_if(cur_if)) {
			wl_ext_move_other_channel(cur_if);
		} else if (wl_ext_master_if(cur_if)) {
			wl_ext_reset_master(cur_if);
		}
	}
}

static void
wl_ext_sync_interface_handler(struct wl_if_info *cur_if,
	const wl_event_msg_t *e, void *data)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);
	uint32 etype = ntoh32(e->event_type);
	uint32 reason = ntoh32(e->reason);

	if (etype == WLC_E_RESERVED && reason == ISAM_RC_RESET_ITF) {
		mutex_lock(&apsta_params->in4way_sync);
		WL_MSG(cur_if->ifname, "sync up interfaces\n");
		wl_ext_sync_interface(cur_if->dev);
		mutex_unlock(&apsta_params->in4way_sync);
	}
	return;
}

static void
wl_ext_43598_ap_restart(struct wl_if_info *cur_if, u32 channel)
{
	dhd_pub_t *dhd = dhd_get_pub(cur_if->dev);
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);
	struct wl_if_info *tmp_if;
	bool band_a = FALSE, band_g = FALSE, rsdb_ap = FALSE;
	int i;

	/* this is a WAR to fix 43598 fw trap issue as below procedure:
	 * step1: enable wlan1 on channel 1
	 * step2: enable wlan2 on channel 36
	 * step3: enable wlan0 to connect channel 1 AP, then it will fw trap
	*/

	if (dhd->conf->chip != BCM4359_CHIP_ID || cur_if->ifmode != ISTA_MODE) {
		return;
	}

	for (i=0; i<MAX_IF_NUM; i++) {
		tmp_if = &apsta_params->if_info[i];
		if (tmp_if && tmp_if->ifmode == IAP_MODE &&
				wl_get_isam_status(tmp_if, IF_READY)) {
			if (wl_ext_get_chanspec(tmp_if->dev, &tmp_if->chan_info)) {
				if (tmp_if->chan_info.band == WLC_BAND_2G)
					band_g = TRUE;
				else if (tmp_if->chan_info.band == WLC_BAND_5G)
					band_a = TRUE;
			}
			if(band_a && band_g)
				rsdb_ap = TRUE;
		}
	}

	if (!rsdb_ap)
		return;

	for (i=0; i<MAX_IF_NUM; i++) {
		tmp_if = &apsta_params->if_info[i];
		if (tmp_if && tmp_if->ifmode == IAP_MODE &&
				wl_get_isam_status(tmp_if, IF_READY)) {
			if (wl_ext_get_chanspec(tmp_if->dev, &tmp_if->chan_info) &&
					wl_ext_same_band(&tmp_if->chan_info, &cur_if->chan_info)) {
				WL_MSG(tmp_if->ifname, "restart chan %s-%d\n",
					WLCBAND2STR(tmp_if->chan_info.band), tmp_if->chan_info.chan);
				wl_ext_if_down(tmp_if);
				wl_ext_if_up(tmp_if, FALSE, 0);
				break;
			}
		}
	}
}

u32
wl_ext_iapsta_update_channel(struct net_device *dev, u32 chanspec)
{
	struct dhd_pub *dhd = dhd_get_pub(dev);
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(dev);
	struct wl_if_info *cur_if = NULL;
	struct dhd_conf *conf = dhd->conf;
	struct wl_ext_chan_info *chan_info;

	cur_if = wl_get_cur_if(dev);
	if (!cur_if)
		goto exit;

	mutex_lock(&apsta_params->usr_sync);
	chan_info = &cur_if->chan_info;
	wl_ext_isam_status(cur_if->dev, NULL, 0);
	wl_ext_set_chan_info(cur_if, CHSPEC2WLC_BAND(chanspec),
		wf_chspec_ctlchan(chanspec));
	if (wl_ext_master_if(cur_if)) {
		if ((cur_if->ifmode == IAP_MODE || cur_if->ifmode == IMESH_MODE) &&
				apsta_params->acs) {
			/* Don't trigger ACS while GO bringup since it could be at WPS stage */
			chan_info->chan = wl_ext_autochannel(cur_if->dev, ACS_DRV_BIT,
				chan_info->band, NULL);
			chan_info->chan = wf_chspec_ctlchan(chan_info->chan);
		}
		wl_ext_move_cur_channel(cur_if);
	}
	wl_ext_move_other_channel(cur_if);
	wl_ext_43598_ap_restart(cur_if, chan_info->chan);
	if (cur_if->ifmode == ISTA_MODE) {
		if (conf->war & SET_CHAN_INCONN && chan_info->chan) {
			wl_ext_set_chanspec(cur_if->dev, chan_info, NULL);
		}
		wl_set_isam_status(cur_if, STA_CONNECTING);
	}
	chanspec = wf_create_chspec_from_primary(chan_info->chan,
		CHSPEC_BW(chanspec), wl_ext_wlcband_to_chanspec_band(chan_info->band));
	mutex_unlock(&apsta_params->usr_sync);

exit:
	return chanspec;
}

static int
wl_ext_iftype_to_ifmode(struct net_device *net, int wl_iftype, ifmode_t *ifmode)
{
	switch (wl_iftype) {
		case WL_IF_TYPE_STA:
			*ifmode = ISTA_MODE;
			break;
		case WL_IF_TYPE_AP:
			*ifmode = IAP_MODE;
			break;
		case WL_IF_TYPE_P2P_GO:
			*ifmode = IGO_MODE;
			break;
		case WL_IF_TYPE_P2P_GC:
			*ifmode = IGC_MODE;
			break;
		default:
			IAPSTA_ERROR(net->name, "Unknown interface wl_iftype:0x%x\n", wl_iftype);
			return BCME_ERROR;
	}
	return BCME_OK;
}

static void
wl_ext_mchan_dump(struct wl_if_info *cur_if)
{
	char scc_str[64], mcc_str[64];
	int scc_len = 0, mcc_len = 0, i;

	if (android_msg_level & ANDROID_INFO_LEVEL) {
		memset(scc_str, 0, sizeof(scc_str));
		memset(mcc_str, 0, sizeof(mcc_str));
		for (i=0; i<MCHAN_CAP_NUM; i++) {
			if (cur_if->mchan_cap[i].mcc)
				mcc_len += snprintf(mcc_str+mcc_len, sizeof(mcc_str)-mcc_len,
					"[%s-%s]",
					WLCBAND2STR(cur_if->mchan_cap[i].band_x),
					WLCBAND2STR(cur_if->mchan_cap[i].band_y));
			else
				scc_len += snprintf(scc_str+scc_len, sizeof(scc_str)-scc_len,
					"[%s-%s]",
					WLCBAND2STR(cur_if->mchan_cap[i].band_x),
					WLCBAND2STR(cur_if->mchan_cap[i].band_y));
		}
		if (scc_len)
			IAPSTA_INFO(cur_if->ifname, "SCC: %s\n", scc_str);
		if (mcc_len)
			IAPSTA_INFO(cur_if->ifname, "MCC: %s\n", mcc_str);
	}
}

static void
wl_ext_init_mchan_mode(struct wl_if_info *cur_if)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);
	struct dhd_pub *dhd = dhd_get_pub(cur_if->dev);

	if (apsta_params->mcc) {
		cur_if->mchan_mode = MCHAN_MCC;
	} else if (cur_if->ifmode == ISTA_MODE || cur_if->ifmode == IGC_MODE) {
		cur_if->mchan_mode = MCHAN_MCC;
	} else if (cur_if->ifmode == IAP_MODE) {
		cur_if->mchan_mode = MCHAN_SCC;
		if (apsta_params->rsdb)
			cur_if->mchan_mode = MCHAN_RSDB;
		if (dhd->conf->ap_mchan_mode != MCHAN_AUTO)
			cur_if->mchan_mode = dhd->conf->ap_mchan_mode;
	} else if (cur_if->ifmode == IGO_MODE) {
		cur_if->mchan_mode = MCHAN_MCC;
		if (dhd->conf->go_mchan_mode != MCHAN_AUTO)
			cur_if->mchan_mode = dhd->conf->go_mchan_mode;
	} else
		cur_if->mchan_mode = MCHAN_MCC;

	if (cur_if->mchan_mode == MCHAN_SCC)
		memcpy(cur_if->mchan_cap, mchan_scc_map, sizeof(mchan_scc_map));
	else if (cur_if->mchan_mode == MCHAN_SBSC)
		memcpy(cur_if->mchan_cap, mchan_sbsc_map, sizeof(mchan_sbsc_map));
	else if (cur_if->mchan_mode == MCHAN_MCC)
		memcpy(cur_if->mchan_cap, mchan_mcc_map, sizeof(mchan_mcc_map));
	else if (cur_if->mchan_mode == MCHAN_RSDB)
		memcpy(cur_if->mchan_cap, mchan_rsdb_map, sizeof(mchan_rsdb_map));

	WL_MSG(cur_if->dev->name, "%s\n", MCHANMODE2STR(cur_if->mchan_mode));
	wl_ext_mchan_dump(cur_if);
}

#if defined(SET_XPS_CPUS) || defined(SET_RPS_CPUS)
static void
wl_ext_xps(struct net_device *dev, char *data, char *ifmode, bool xps)
{
	char *pch, *pick_tmp, *buf = NULL;
	int len;

	IAPSTA_TRACE(dev->name, "data=%s\n", data);

	if (!data)
		return;

	len = strlen(data);
	buf = kmalloc(len+1, GFP_KERNEL);
	strcpy(buf, data);
	pick_tmp = buf;

	pch = bcmstrstr(pick_tmp, ifmode); // pick ifmode
	if (!pch)
		goto exit;
	pch = bcmstrtok(&pch, ", ", 0);
	if (!pch) {
		goto exit;
	}
	pch += strlen(ifmode);
	IAPSTA_INFO(dev->name, "%s = %s\n", xps ? "xps" : "rps", pch);
	if (xps)
		custom_xps_map_set(dev, pch, strlen(pch));
	else
		custom_rps_map_set(dev, pch, strlen(pch));
exit:
	if (buf)
		kfree(buf);
	return;
}

/* config.txt:
  * xps_cpus=sta=1, ap=3, gc=5, go=7
  * rps_cpus=sta=2, ap=4, gc=6, go=8
  * cmd:
  * cat /sys/class/net/wlan0/queues/tx-0/xps_cpus
  * cat /sys/class/net/wlan0/queues/rx-0/rps_cpus
 */
void
wl_ext_iapsta_set_xps(struct wl_if_info *cur_if)
{
	struct dhd_pub *dhd = dhd_get_pub(cur_if->dev);
#if defined(SET_XPS_CPUS)
	char *xps_cpus = dhd->conf->xps_cpus;
#endif /* SET_XPS_CPUS  */
#if defined(SET_RPS_CPUS)
	char *rps_cpus = dhd->conf->rps_cpus;
#endif /* SET_RPS_CPUS  */

	IAPSTA_TRACE(cur_if->dev->name, "Enter\n");

#if defined(SET_XPS_CPUS)
	if (cur_if && wl_get_isam_status(cur_if, IF_READY)) {
		if (cur_if->ifmode == ISTA_MODE)
			wl_ext_xps(cur_if->dev, xps_cpus, "sta=", TRUE);
		else if (cur_if->ifmode == IAP_MODE)
			wl_ext_xps(cur_if->dev, xps_cpus, "ap=", TRUE);
		else if (cur_if->ifmode == IGC_MODE)
			wl_ext_xps(cur_if->dev, xps_cpus, "gc=", TRUE);
		else if (cur_if->ifmode == IGO_MODE)
			wl_ext_xps(cur_if->dev, xps_cpus, "go=", TRUE);
#endif /* SET_XPS_CPUS  */
#if defined(SET_RPS_CPUS)
		if (cur_if->ifmode == ISTA_MODE)
			wl_ext_xps(cur_if->dev, rps_cpus, "sta=", FALSE);
		else if (cur_if->ifmode == IAP_MODE)
			wl_ext_xps(cur_if->dev, rps_cpus, "ap=", FALSE);
		else if (cur_if->ifmode == IGC_MODE)
			wl_ext_xps(cur_if->dev, rps_cpus, "gc=", FALSE);
		else if (cur_if->ifmode == IGO_MODE)
			wl_ext_xps(cur_if->dev, rps_cpus, "go=", FALSE);
#endif /* SET_RPS_CPUS  */
	}
}
#endif /* SET_XPS_CPUS ||SET_RPS_CPUS  */

void
wl_ext_iapsta_update_iftype(struct net_device *net, int wl_iftype)
{
#ifdef WL_STATIC_IF
	struct dhd_pub *dhd = dhd_get_pub(net);
#endif /* WL_STATIC_IF */
	struct wl_if_info *cur_if = NULL;

	cur_if = wl_get_cur_if(net);
	if (cur_if) {
		IAPSTA_INFO(net->name, "ifidx=%d(%s)\n",
			cur_if->ifidx, wl_iftype_to_str(wl_iftype));
		if (wl_iftype == WL_IF_TYPE_STA) {
			cur_if->ifmode = ISTA_MODE;
			cur_if->prio = PRIO_STA;
#ifdef WL_STATIC_IF
			dhd_conf_preinit_ioctls_sta(dhd, cur_if->ifidx);
#endif /* WL_STATIC_IF */
		}
		else if (wl_iftype == WL_IF_TYPE_AP && cur_if->ifmode != IMESH_MODE) {
			cur_if->ifmode = IAP_MODE;
			cur_if->prio = PRIO_AP;
		}
		else if (wl_iftype == WL_IF_TYPE_P2P_GO) {
			cur_if->ifmode = IGO_MODE;
			cur_if->prio = PRIO_GO;
		}
		else if (wl_iftype == WL_IF_TYPE_P2P_GC) {
			cur_if->ifmode = IGC_MODE;
			cur_if->prio = PRIO_GC;
		}
		else if (wl_iftype == WL_IF_TYPE_IBSS) {
			cur_if->ifmode = IAP_MODE;
			cur_if->prio = PRIO_AP;
			wl_ext_iovar_setint(cur_if->dev, "assoc_retry_max", 3);
		}
		wl_ext_init_mchan_mode(cur_if);
#if defined(SET_XPS_CPUS) || defined(SET_RPS_CPUS)
		wl_ext_iapsta_set_xps(cur_if);
#endif /* SET_XPS_CPUS ||SET_RPS_CPUS  */
	}
}

void
wl_ext_iapsta_ifadding(struct net_device *net, int ifidx)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(net);
	struct wl_if_info *cur_if = NULL;

	IAPSTA_TRACE(net->name, "ifidx=%d\n", ifidx);
	if (ifidx < MAX_IF_NUM) {
		cur_if = &apsta_params->if_info[ifidx];
		wl_set_isam_status(cur_if, IF_ADDING);
	}
}

bool
wl_ext_iapsta_iftype_enabled(struct net_device *net, int wl_iftype)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(net);
	struct wl_if_info *cur_if = NULL;
	ifmode_t ifmode = 0;

	wl_ext_iftype_to_ifmode(net, wl_iftype, &ifmode);
	cur_if = wl_ext_if_enabled(apsta_params, ifmode);
	if (cur_if)
		return TRUE;

	return FALSE;
}

#ifdef WLMESH_CFG80211
bool
wl_ext_iapsta_mesh_creating(struct net_device *net)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(net);
	struct wl_if_info *cur_if;
	int i;

	if (apsta_params) {
		for (i=0; i<MAX_IF_NUM; i++) {
			cur_if = &apsta_params->if_info[i];
			if (cur_if->ifmode==IMESH_MODE && wl_get_isam_status(cur_if, IF_ADDING))
				return TRUE;
		}
	}
	return FALSE;
}
#endif /* WLMESH_CFG80211 */

void
wl_ext_fw_reinit_incsa(struct net_device *dev)
{
	struct dhd_pub *dhd = dhd_get_pub(dev);
	struct dhd_conf *conf = dhd->conf;
	struct wl_if_info *cur_if = NULL;

	cur_if = wl_get_cur_if(dev);
	if (!cur_if)
		return;

	if (conf->war & FW_REINIT_INCSA) {
		if (cur_if->ifmode == ISTA_MODE &&
				wl_ext_iapsta_iftype_enabled(dev, WL_IF_TYPE_AP)) {
			IAPSTA_INFO(dev->name, "wl reinit\n");
			wl_ext_ioctl(dev, WLC_INIT, NULL, 0, 1);
		}
	}
}

#ifdef WL_ESCAN
static void
wl_ext_get_scan_channels(struct net_device *dev, wl_scan_info_t *scan_info)
{
#ifdef WL_CFG80211
	struct bcm_cfg80211 *cfg = wl_get_cfg(dev);
	wlc_ssid_t *ssid = NULL;
	int ret = BCME_ERROR;
#endif /* WL_CFG80211 */
	wlcfg_assoc_info_t assoc_info;
	uint16 chan_2g = 0, chan_5g = 0, chan_6g = 0;
	int i, cnt = 0;

	memset(&assoc_info, 0, sizeof(wlcfg_assoc_info_t));
	wl_ext_get_default_chan(dev, &chan_2g, &chan_5g, &chan_6g);
	if (chan_2g) {
		assoc_info.chanspecs[0] =
			wf_create_chspec_from_primary(chan_2g, WL_CHANSPEC_BW_20,
				wl_ext_wlcband_to_chanspec_band(WLC_BAND_2G));
	} else if (chan_5g) {
		assoc_info.chanspecs[0] =
			wf_create_chspec_from_primary(chan_5g, WL_CHANSPEC_BW_20,
				wl_ext_wlcband_to_chanspec_band(WLC_BAND_5G));
	}
	assoc_info.chan_cnt = 1;
#ifdef WL_CFG80211
	ssid = (struct wlc_ssid *)wl_read_prof(cfg, dev, WL_PROF_SSID);
	if (ssid) {
		assoc_info.ssid_len = ssid->SSID_len;
		ret = memcpy_s(assoc_info.ssid, sizeof(assoc_info.ssid),
			ssid->SSID, ssid->SSID_len);
		if (!ret) {
			ret = wl_get_assoc_channels(cfg, dev, &assoc_info);
			if (ret)
				IAPSTA_ERROR(dev->name, "get assoc channels failed\n");
			for (i = 0; i < assoc_info.chan_cnt; i++) {
				if (CHSPEC2WLC_BAND(assoc_info.chanspecs[i]) == WLC_BAND_5G)
					chan_5g = 0;
			}
		}
	}
#endif /* WL_CFG80211 */
	// add one more channel to make sure there is one non-home channel to scan
	cnt = assoc_info.chan_cnt;
	if (cnt < (MAX_ROAM_CHANNEL - 1)) {
		if (chan_2g && !chan_5g && cnt == 1) {
			assoc_info.chanspecs[cnt] =
				wf_create_chspec_from_primary(6, WL_CHANSPEC_BW_20,
					wl_ext_wlcband_to_chanspec_band(WLC_BAND_2G));
			cnt++;
		} else if (chan_5g) {
			assoc_info.chanspecs[cnt] =
				wf_create_chspec_from_primary(chan_5g, WL_CHANSPEC_BW_20,
					wl_ext_wlcband_to_chanspec_band(WLC_BAND_5G));
			cnt++;
		}
		assoc_info.chan_cnt = cnt;
	}
	for (i = 0; i < assoc_info.chan_cnt; i++) {
		scan_info->channels.chanspec[i] = assoc_info.chanspecs[i];
	}
	scan_info->channels.count = assoc_info.chan_cnt;
}

static int
wl_iapsta_trigger_scan(struct net_device *dev, int scan_type)
{
	struct wl_if_info *cur_if = wl_get_cur_if(dev);
	struct bcm_cfg80211 *cfg = wl_get_cfg(dev);
	wlc_ssid_t *ssid = (struct wlc_ssid *)wl_read_prof(cfg, dev, WL_PROF_SSID);
	wl_scan_info_t *scan_info = NULL;
	int ret = BCME_OK;

	if (cur_if->conn_state >= CONN_STATE_CONNECTING && cur_if->conn_state < CONN_STATE_4WAY_M4) {
		IAPSTA_ERROR(dev->name, "connecting... %d\n", cur_if->conn_state);
		return BCME_BUSY;
	}

	if (scan_type) {
		scan_info = kzalloc(sizeof(wl_scan_info_t), GFP_KERNEL);
		if (scan_info) {
			if (ssid)
				memcpy(&scan_info->ssid, ssid, sizeof(wlc_ssid_t));
			scan_info->bcast_ssid = TRUE;
			IAPSTA_INFO(dev->name, "scan_type %d\n", scan_type);
			if (scan_type == SCAN_RCC)
				wl_ext_get_scan_channels(dev, scan_info);
#ifdef WL_ESCAN_ROAM_CACHE
			else if (scan_type == SCAN_ERCC)
				wl_escan_roam_channel_list(dev, scan_info,
					sizeof(scan_info->channels.chanspec));
#endif /* WL_ESCAN_ROAM_CACHE */
			if (scan_type == SCAN_ERCC || scan_type == SCAN_FULLCHAN)
				scan_info->scan_time = DHD_SCAN_UNASSOC_ACTIVE_TIME;
			ret = wl_escan_set_scan(dev, scan_info);
			if (ret)
				IAPSTA_ERROR(dev->name, "scan failed %d\n", ret);
			kfree(scan_info);
		} else {
			IAPSTA_ERROR(dev->name, "kzalloc failed\n");
			ret = BCME_NOMEM;
		}
	} else {
		IAPSTA_ERROR(dev->name, "unknow scan_type %d\n", scan_type);
	}

	return ret;
}
#endif /* WL_ESCAN */

#ifdef WL_EXT_REASSOC
static void
wl_ext_reassoc_scan_timeout(unsigned long data)
{
	struct net_device *dev = (struct net_device *)data;

	if (!dev) {
		IAPSTA_ERROR("wlan", "dev is not ready\n");
		return;
	}
	IAPSTA_ERROR(dev->name, "timer expired\n");
	wl_ext_send_event_msg(dev, WLC_E_RESERVED, WLC_E_STATUS_SUCCESS,
		ISAM_RC_REASSOC_SCAN);
}

static void
wl_ext_cancel_reassoc(struct wl_if_info *cur_if)
{
	struct dhd_pub *dhd = dhd_get_pub(cur_if->dev);

	if (!cur_if->dev) {
		IAPSTA_DBG(cur_if->dev->name, "not ready\n");
		return;
	}

	IAPSTA_TRACE(cur_if->dev->name, "Enter\n");
	cur_if->reassoc_scan_type = SCAN_NONE;
	wl_timer_mod(dhd, &cur_if->reassoc_scan_timer, 0);
}

static int
wl_ext_trigger_reassoc(struct wl_if_info *cur_if, int rescan_type)
{
	struct net_device *dev = cur_if->dev;
	struct bcm_cfg80211 *cfg = wl_get_cfg(dev);
	dhd_pub_t *dhd = dhd_get_pub(dev);
#ifdef WL_EXT_RECONNECT
	int max_wait_time = 0;
#endif /* WL_EXT_RECONNECT */
	wlcfg_assoc_info_t assoc_info;
	wlc_ssid_t *ssid = (struct wlc_ssid *)wl_read_prof(cfg, dev, WL_PROF_SSID);
	struct ether_addr bssid = {{0, 0, 0, 0, 0, 0}};
	chanspec_t chanspec = 0;
	int ret = 0;

	memset(&assoc_info, 0, sizeof(wlcfg_assoc_info_t));

	if (ssid) {
		assoc_info.ssid_len = ssid->SSID_len;
		ret = memcpy_s(assoc_info.ssid, sizeof(assoc_info.ssid), ssid->SSID, ssid->SSID_len);
	}
	if (!ssid || ret) {
		IAPSTA_ERROR(dev->name, "ssid cpy failed %d\n", ret);
		goto exit;
	}

	assoc_info.reassoc = wl_ext_associated(dev, &bssid);
#ifdef WL_ESCAN
	chanspec = wl_escan_candidate_bss(dev, ssid, &bssid);
#endif /* WL_ESCAN */
	if (chanspec) {
		assoc_info.chanspecs[0] = chanspec;
		assoc_info.chan_cnt = 1;
	} else {
		WL_MSG(dev->name, "try scan_type %d again\n", rescan_type);
		wl_cfgext_trigger_reassoc_scan(dev, NULL, rescan_type, 0);
		ret = BCME_OK;
		goto exit;
	}

	ret = wl_get_assoc_channels(cfg, dev, &assoc_info);
	if (ret)
		assoc_info.chan_cnt = 0;

	if (!ETHER_ISNULLADDR(&bssid))
		(void)memcpy_s(&assoc_info.bssid, ETH_ALEN, &bssid, ETH_ALEN);
	else
		memcpy(&assoc_info.bssid, &ether_bcast, ETHER_ADDR_LEN);

	if (assoc_info.reassoc) {
		/* cleanup key_install to perform OPEN AUTH instead of FT AUTH */
		ret = wldev_iovar_setint(dev, "buf_key_b4_m4_installed", 0);
		if (ret)
			IAPSTA_INFO(dev->name, "buf_key_b4_m4_installed unsupported %d\n", ret);
		else
			IAPSTA_INFO(dev->name, "wl buf_key_b4_m4_installed 0\n");
		ret = wl_cfg80211_reassoc(dev, &assoc_info);
	} else {
		wl_ext_send_event_msg(dev, WLC_E_ROAM_PREP, WLC_E_STATUS_SUCCESS, WLC_E_REASON_DEAUTH);
		ret = wl_handle_join(cfg, dev, &assoc_info);
	}
	if (!ret) {
		wl_ext_in4way_sync(dev, STA_NO_BTC_IN4WAY, WL_EXT_STATUS_RECONNECTING, NULL);
#ifdef WL_EXT_RECONNECT
		if (assoc_info.reassoc)
			max_wait_time = STA_RECONNECT_RETRY_TIMEOUT;
		else
			max_wait_time = STA_CONNECT_RETRY_TIMEOUT;
		max_wait_time += (WL_BCAST_SCAN_JOIN_ACTIVE_DWELL_TIME_MS * assoc_info.chan_cnt);
		if (assoc_info.chan_cnt == 0)
			max_wait_time = STA_CONNECT_FULL_CHAN_TIMEOUT;
		max_wait_time = min(max_wait_time, STA_CONNECT_FULL_CHAN_TIMEOUT);
		wl_timer_mod(dhd, &cur_if->reconnect_timer, max_wait_time);
		IAPSTA_INFO(dev->name, "reconnect %dms later\n", max_wait_time);
#endif /* WL_EXT_RECONNECT */
	}

exit:
	if (ret) {
		wl_cfg80211_disassoc(dev, WLAN_REASON_DEAUTH_LEAVING);
		wl_timer_mod(dhd, &cur_if->connect_timer, 100);
	}
	return ret;
}

void
wl_cfgext_trigger_reassoc_scan(struct net_device *dev,
	struct ether_addr *dirty_bssid, int scan_type, int conn_tmo)
{
	struct dhd_pub *dhd = dhd_get_pub(dev);
	struct wl_if_info *cur_if = NULL;

	cur_if = wl_get_cur_if(dev);
	if (!cur_if || !cur_if->dev) {
		IAPSTA_DBG(dev->name, "not ready\n");
		return;
	}

	if (cur_if->reassoc_scan_type) {
		IAPSTA_INFO(dev->name, "reassoc scan is running already\n");
	} else {
		wl_timer_mod(dhd, &cur_if->reassoc_scan_timer, 0);
		cur_if->reassoc_scan_type = scan_type;
#ifdef WL_ESCAN_ROAM_CACHE
		if (dirty_bssid && !ETHER_ISNULLADDR(dirty_bssid))
			wl_escan_dirty_roam_cache(dev, dirty_bssid, TRUE);
#endif /* WL_ESCAN_ROAM_CACHE */
		wl_ext_send_event_msg(cur_if->dev, WLC_E_RESERVED, WLC_E_STATUS_SUCCESS,
			ISAM_RC_REASSOC_SCAN);
	}
	if (conn_tmo) {
		osl_do_gettimeofday(&cur_if->sta_conn_ts);
		wl_timer_mod(dhd, &cur_if->connect_timer, STA_CONNECT_TIMEOUT);
	}
}

void
wl_cfgext_event_handler(struct net_device *dev, const wl_event_msg_t *e,
	void *data)
{
	struct bcm_cfg80211 *cfg = wl_get_cfg(dev);
	struct dhd_pub *dhd = dhd_get_pub(dev);
	struct wl_if_info *cur_if = NULL;
	uint32 event_type = ntoh32(e->event_type);
	uint32 reason =  ntoh32(e->reason);
	int rescan_type = SCAN_RCC;

	cur_if = wl_get_cur_if(dev);
	if (!cur_if || !cur_if->dev) {
		IAPSTA_DBG(dev->name, "ifidx %d is not ready\n", e->ifidx);
		return;
	}

	if (cur_if->ifmode != ISTA_MODE)
		return;

	if (event_type == WLC_E_RESERVED &&
			(reason == ISAM_RC_REASSOC_SCAN || reason == ISAM_RC_REASSOC)) {
		mutex_lock(&cfg->connect_sync);
		if (wl_get_drv_status(cfg, DISCONNECTING, dev) ||
				!wl_get_drv_status(cfg, CONNECTED, dev)) {
			IAPSTA_INFO(dev->name, "cfg not connected\n");
			mutex_unlock(&cfg->connect_sync);
			return;
		}
		mutex_unlock(&cfg->connect_sync);
		IAPSTA_INFO(dev->name, "etype %d, reason %d\n", event_type, reason);
		if (reason == ISAM_RC_REASSOC_SCAN && cur_if->reassoc_scan_type) {
			if (wl_iapsta_trigger_scan(cur_if->dev, cur_if->reassoc_scan_type)) {
				IAPSTA_INFO(dev->name, "trigger reassoc scan %dms later\n",
					REASSOC_SCAN_RETRY_MS);
				wl_timer_mod(dhd, &cur_if->reassoc_scan_timer, REASSOC_SCAN_RETRY_MS);
			}
		}
		else if (reason == ISAM_RC_REASSOC) {
			if (cur_if->reassoc_scan_type == SCAN_RCC)
				rescan_type = SCAN_ERCC;
			else if (cur_if->reassoc_scan_type == SCAN_ERCC ||
					cur_if->reassoc_scan_type == SCAN_FULLCHAN)
				rescan_type = SCAN_FULLCHAN;
			wl_ext_cancel_reassoc(cur_if);
			wl_ext_trigger_reassoc(cur_if, rescan_type);
		}
	}
}

static void
wl_ext_reassoc_scan_event(struct wl_if_info *cur_if,
	const wl_event_msg_t *e, void *data)
{
	uint32 event_type = ntoh32(e->event_type);
	uint32 status =  ntoh32(e->status);

	if (cur_if->reassoc_scan_type && event_type == WLC_E_ESCAN_RESULT &&
			(status == WLC_E_STATUS_SUCCESS || status == WLC_E_STATUS_ABORT)) {
		IAPSTA_INFO(cur_if->dev->name, "reassoc scan complete\n");
		wl_ext_send_event_msg(cur_if->dev, WLC_E_RESERVED, WLC_E_STATUS_SUCCESS,
			ISAM_RC_REASSOC);
	}
}
#endif /* WL_EXT_REASSOC */

#ifdef WL_EXT_RECONNECT
static void
wl_ext_reconnect_timeout(unsigned long data)
{
	struct net_device *dev = (struct net_device *)data;

	if (!dev) {
		IAPSTA_ERROR("wlan", "dev is not ready\n");
		return;
	}
	IAPSTA_ERROR(dev->name, "timer expired\n");
	wl_ext_send_event_msg(dev, WLC_E_SET_SSID, WLC_E_STATUS_NO_NETWORKS, WLC_E_STATUS_SUCCESS);
}

#ifdef WL_EXT_DISCONNECT_RECONNECT
static bool
wl_ext_disc_recon_retry(struct wl_if_info *cur_if)
{
	int sta_disc_recon_cnt = cur_if->sta_disc_recon_cnt;
	struct osl_timespec *sta_disc_conn_ts = &cur_if->sta_disc_conn_ts;
	struct osl_timespec cur_ts;
	uint32 diff_ms = 0;
	bool retry = FALSE;

	if (sta_disc_recon_cnt == 0)
		osl_do_gettimeofday(sta_disc_conn_ts);

	osl_do_gettimeofday(&cur_ts);
	diff_ms = osl_do_gettimediff(&cur_ts, sta_disc_conn_ts)/1000;

	IAPSTA_INFO(cur_if->ifname, "sta_disc_recon_cnt=%d, diff_ms = %dms\n",
		sta_disc_recon_cnt, diff_ms);
	if (sta_disc_recon_cnt >= STA_DISCONNECT_RECONNECT_MAX) {
		if (diff_ms >= STA_DISCONNECT_RECONNECT_TIMEOUT) {
			osl_do_gettimeofday(sta_disc_conn_ts);
			cur_if->sta_disc_recon_cnt = 0;
			retry = TRUE;
		} else {
			retry = FALSE;
		}
	} else {
		retry = TRUE;
	}

	if (retry)
		cur_if->sta_disc_recon_cnt++;

	return retry;
}
#endif /* WL_EXT_DISCONNECT_RECONNECT */

static int
wl_ext_connect_retry(struct net_device *dev, wl_event_msg_t *e)
{
	struct dhd_pub *dhd = dhd_get_pub(dev);
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(dev);
	struct wl_if_info *cur_if = wl_get_cur_if(dev);
	struct bcm_cfg80211 *cfg = wl_get_cfg(dev);
	struct osl_timespec cur_ts, *sta_conn_ts;
	uint32 etype = ntoh32(e->event_type);
	uint32 status = ntoh32(e->status);
	uint32 diff_ms = 0;
	int ret = 0;

	if (!cur_if) {
		return 0;
	}

	wl_timer_mod(dhd, &cur_if->reconnect_timer, 0);

	mutex_unlock(&apsta_params->in4way_sync);
	mutex_lock(&cfg->connect_sync);

	if (wl_get_drv_status(cfg, DISCONNECTING, dev)) {
		WL_MSG(dev->name, "skip connect retry due to disconnecting\n");
		ret = BCME_BADADDR;
		goto exit;
	}

	if (cur_if->conn_state == CONN_STATE_ASSOCIATED) {
		WL_MSG(dev->name, "skip connect retry due to no 4-way M1 received\n");
		ret = BCME_BADADDR;
		goto exit;
	}

	sta_conn_ts = &cur_if->sta_conn_ts;
	osl_do_gettimeofday(&cur_ts);
	diff_ms = osl_do_gettimediff(&cur_ts, sta_conn_ts) / 1000;

	if (wl_ext_sta_connecting(dev) && diff_ms < STA_CONNECT_TIMEOUT) {
		if (etype == WLC_E_SET_SSID && (status == WLC_E_STATUS_NO_NETWORKS ||
				status == WLC_E_STATUS_NO_ACK)) {
			if (wl_ext_associated(dev, NULL))
				bzero(&cfg->last_roamed_addr, ETHER_ADDR_LEN);
			WL_MSG(dev->name, "retry reassoc\n");
			wl_ext_send_event_msg(cur_if->dev, WLC_E_RESERVED, WLC_E_STATUS_SUCCESS,
				ISAM_RC_REASSOC);
		}
		ret = BCME_ERROR;
	}
#ifdef WL_EXT_DISCONNECT_RECONNECT
	else if (cur_if->conn_state >= CONN_STATE_CONNECTED &&
			wl_get_drv_status(cfg, CONNECTED, dev)) {
		if (etype == WLC_E_DISASSOC_IND || etype == WLC_E_DEAUTH_IND) {
			if (wl_ext_disc_recon_retry(cur_if)) {
				bzero(&cfg->last_roamed_addr, ETHER_ADDR_LEN);
				WL_MSG(dev->name, "retry join cnt %d\n", cur_if->sta_disc_recon_cnt);
				wl_cfgext_trigger_reassoc_scan(dev, (struct ether_addr *)&e->addr, SCAN_RCC, STA_CONNECT_TIMEOUT);
				ret = BCME_ERROR;
			}
			else {
				WL_MSG(dev->name, "out of retry cnt %d within %dms\n",
					cur_if->sta_disc_recon_cnt, STA_DISCONNECT_RECONNECT_TIMEOUT);
			}
		}
	}
#endif /* WL_EXT_DISCONNECT_RECONNECT */

exit:
	mutex_unlock(&cfg->connect_sync);
	mutex_lock(&apsta_params->in4way_sync);
	return ret;
}

static void
wl_ext_set_eapol_timeout(struct wl_if_info *cur_if, int wpa_auth)
{
	struct net_device *dev = cur_if->dev;
	struct bcm_cfg80211 *cfg = wl_get_cfg(dev);
	struct dhd_pub *dhd = dhd_get_pub(dev);
	int max_wait_time = 0;

	if ((cfg->auth_type == DOT11_FAST_BSS) && (wpa_auth & WPA2_AUTH_FT))
		max_wait_time = 0;
	else if ((cfg->auth_type == DOT11_SAE) && wpa_auth & (WPA3_AUTH_SAE_PSK|0x20))
		max_wait_time = 0;
	else if (wpa_auth >= WPA_AUTH_UNSPECIFIED) {
		if (wpa_auth & WPA2_AUTH_UNSPECIFIED)
			max_wait_time = STA_EAP_TIMEOUT;
		else
			max_wait_time = STA_4WAY_TIMEOUT;
	}

	if (max_wait_time)
		IAPSTA_INFO(dev->name, "reconnect %dms later\n", max_wait_time);
	wl_timer_mod(dhd, &cur_if->reconnect_timer, max_wait_time);
}

static void
wl_ext_set_connect_retry(struct net_device *dev, void *context)
{
	struct dhd_pub *dhd = dhd_get_pub(dev);
	wlcfg_assoc_info_t *assoc_info = (wlcfg_assoc_info_t *)context;
	struct wl_if_info *cur_if;
	int max_wait_time;
	int wpa_auth = 0;

	cur_if = wl_get_cur_if(dev);
	if (!cur_if)
		return;

	wl_timer_mod(dhd, &cur_if->reconnect_timer, 0);
	wl_ext_iovar_getint(dev, "wpa_auth", &wpa_auth);
	if (!(wpa_auth & (WPA3_AUTH_SAE_PSK|0x20) && assoc_info)) {
		memcpy(&cur_if->bssid, assoc_info->bssid, ETHER_ADDR_LEN);
		if (assoc_info->chan_cnt == 0)
			max_wait_time = STA_CONNECT_FULL_CHAN_TIMEOUT;
		else if (assoc_info->reassoc)
			max_wait_time = STA_RECONNECT_RETRY_TIMEOUT;
		else
			max_wait_time = STA_CONNECT_RETRY_TIMEOUT;
		IAPSTA_INFO(dev->name, "reconnect %dms later\n", max_wait_time);
		wl_timer_mod(dhd, &cur_if->reconnect_timer, max_wait_time);
	}
}
#endif /* WL_EXT_RECONNECT */

#ifdef STA_MGMT
static void
wl_ext_flush_sta_list(struct net_device *net, int ifidx)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(net);
	wl_sta_info_t *node, *next;

	list_for_each_entry_safe(node, next, &apsta_params->sta_list, list) {
		if (node->ifidx == ifidx || ifidx == 0xFF) {
			IAPSTA_INFO(net->name, "Del BSSID %pM\n", &node->bssid);
			list_del(&node->list);
			kfree(node);
		}
	}
}

bool
wl_ext_del_sta_info(struct net_device *net, u8 *bssid)
{
	struct dhd_pub *dhd = dhd_get_pub(net);
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(net);
	int ifidx = dhd_net2idx(dhd->info, net);
	wl_sta_info_t *node, *next;
	bool in_list = FALSE;

	list_for_each_entry_safe(node, next, &apsta_params->sta_list, list) {
		if (node->ifidx == ifidx && !memcmp(&node->bssid, bssid, ETHER_ADDR_LEN)) {
			IAPSTA_INFO(net->name, "Del BSSID %pM\n", &node->bssid);
			in_list = TRUE;
			list_del(&node->list);
			kfree(node);
		}
	}
	return in_list;
}

bool
wl_ext_add_sta_info(struct net_device *net, u8 *bssid)
{
	struct dhd_pub *dhd = dhd_get_pub(net);
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(net);
	int ifidx = dhd_net2idx(dhd->info, net);
	wl_sta_info_t *node, *next, *leaf;

	list_for_each_entry_safe(node, next, &apsta_params->sta_list, list) {
		if (node->ifidx == ifidx && !memcmp(&node->bssid, bssid, ETHER_ADDR_LEN)) {
			IAPSTA_INFO(net->name, "BSSID %pM already in list\n", bssid);
			return FALSE;
		}
	}

	leaf = kmalloc(sizeof(wl_sta_info_t), GFP_KERNEL);
	if (!leaf) {
		IAPSTA_ERROR(net->name, "Memory alloc failure %d\n",
			(int)sizeof(wl_sta_info_t));
		return FALSE;
	}
	IAPSTA_INFO(net->name, "Add BSSID %pM in the leaf\n", bssid);
	leaf->ifidx = ifidx;
	memcpy(&leaf->bssid, bssid, ETHER_ADDR_LEN);
	list_add_tail(&leaf->list, &apsta_params->sta_list);

	return TRUE;
}
#endif /* STA_MGMT */
#endif /* WL_CFG80211 */

static int
wlc_ver_major(struct net_device *dev)
{
	wl_wlc_version_t wlc_ver;
	int ret;

	memset(&wlc_ver, 0, sizeof(wlc_ver));
	ret = wldev_iovar_getbuf(dev, "wlc_ver", NULL, 0,
		&wlc_ver, sizeof(wl_wlc_version_t), NULL);
	if (ret != BCME_OK) {
		/* old firmware may not support "wlc_ver" */
		return 0;
	}
	return wlc_ver.wlc_ver_major;
}

void
wl_ext_update_conn_state(dhd_pub_t *dhd, int ifidx, uint conn_state)
{
	struct wl_apsta_params *apsta_params = dhd->iapsta_params;
	struct wl_if_info *cur_if = NULL;
#ifdef EAPOL_RESEND
	unsigned long flags = 0;
#endif /* EAPOL_RESEND */

	if (ifidx < MAX_IF_NUM) {
		cur_if = &apsta_params->if_info[ifidx];
#ifdef EAPOL_RESEND
		spin_lock_irqsave(&apsta_params->eapol_lock, flags);
#endif /* EAPOL_RESEND */
		if (wl_ext_slave_if(cur_if)) {
			if (wl_ext_sta_connecting(cur_if->dev) ||
#ifdef EAPOL_REQID_UPDATE
					conn_state == CONN_STATE_REQID ||
#endif /* EAPOL_REQID_UPDATE */
					conn_state >= CONN_STATE_CONNECTED ||
					conn_state <= CONN_STATE_CONNECTING)
				apsta_params->if_info[ifidx].conn_state = conn_state;
			else
				IAPSTA_INFO(cur_if->dev->name, "skip update %d\n", conn_state);
		} else {
			apsta_params->if_info[ifidx].conn_state = conn_state;
		}
#ifdef EAPOL_RESEND
		spin_unlock_irqrestore(&apsta_params->eapol_lock, flags);
#endif /* EAPOL_RESEND */
	}
}

#ifdef EAPOL_RESEND
#ifdef EAPOL_DYNAMATIC_RESEND
static void
wl_ext_calc_eapol_intvl(struct wl_if_info *cur_if, bool rx)
{
	struct osl_timespec cur_ts;
	uint32 diff_ms;

	if (rx && cur_if->pend_eapol_pkt && !cur_if->eapol_retry) {
		osl_do_gettimeofday(&cur_ts);
		diff_ms = osl_do_gettimediff(&cur_ts, &cur_if->eapol_tx_ts)/1000;
		if (diff_ms > STA_EAPOL_TIMEOUT)
			diff_ms = STA_EAPOL_TIMEOUT;
		if (diff_ms > cur_if->eapol_max_intvl)
			cur_if->eapol_max_intvl = diff_ms;
		if (!cur_if->eapol_cnt || diff_ms < cur_if->eapol_min_intvl ||
				cur_if->eapol_min_intvl == 0)
			cur_if->eapol_min_intvl = diff_ms;

		if (cur_if->eapol_cnt)
			cur_if->eapol_avg_intvl =
				(cur_if->eapol_avg_intvl * cur_if->eapol_cnt + diff_ms) /
				(cur_if->eapol_cnt+1);
		else
			cur_if->eapol_avg_intvl = (diff_ms + STA_EAPOL_TIMEOUT) / 2;
		cur_if->eapol_cnt++;

		if (cur_if->eapol_avg_intvl <= (cur_if->eapol_min_intvl + 2) ||
				cur_if->eapol_avg_intvl <= 10) {
			cur_if->eapol_avg_intvl = (cur_if->eapol_max_intvl+STA_EAPOL_TIMEOUT)/2;
			cur_if->eapol_cnt = 1;
		}
	}
}
#endif /* EAPOL_DYNAMATIC_RESEND */

static void
wl_ext_free_eapol_txpkt(struct wl_if_info *cur_if, bool rx)
{
	struct dhd_pub *dhd = dhd_get_pub(cur_if->dev);

#ifdef BCMDBUS
	if (!rx)
#endif /* BCMDBUS */
	wl_timer_mod(dhd, &cur_if->eapol_timer, 0);

	if (cur_if->pend_eapol_pkt) {
		PKTCFREE(dhd->osh, cur_if->pend_eapol_pkt, TRUE);
		cur_if->pend_eapol_pkt = NULL;
		IAPSTA_TRACE(cur_if->dev->name, "release eapol pkt\n");
	}
}

void
wl_ext_release_eapol_txpkt(dhd_pub_t *dhd, int ifidx, bool rx)
{
	struct wl_apsta_params *apsta_params = dhd->iapsta_params;
	struct wl_if_info *cur_if = NULL;
	unsigned long flags = 0;

	if (ifidx < MAX_IF_NUM && (dhd->conf->war & RESEND_EAPOL_PKT)) {
		cur_if = &apsta_params->if_info[ifidx];
		spin_lock_irqsave(&apsta_params->eapol_lock, flags);
#ifdef EAPOL_DYNAMATIC_RESEND
		wl_ext_calc_eapol_intvl(cur_if, rx);
		if (rx)
			cur_if->eapol_retry = FALSE;
#endif /* EAPOL_DYNAMATIC_RESEND */
		wl_ext_free_eapol_txpkt(cur_if, rx);
		spin_unlock_irqrestore(&apsta_params->eapol_lock, flags);
	}
}

void
wl_ext_backup_eapol_txpkt(dhd_pub_t *dhd, int ifidx, void *pkt)
{
	struct wl_apsta_params *apsta_params = dhd->iapsta_params;
	struct wl_if_info *cur_if = NULL;
	unsigned long flags = 0;
	int interval;

	if (ifidx < MAX_IF_NUM && (dhd->conf->war & RESEND_EAPOL_PKT)) {
		cur_if = &apsta_params->if_info[ifidx];
		if (cur_if->dev && cur_if->ifmode == ISTA_MODE &&
				wl_ext_sta_connecting(cur_if->dev)) {
			spin_lock_irqsave(&apsta_params->eapol_lock, flags);
			wl_ext_free_eapol_txpkt(cur_if, TRUE);
			cur_if->pend_eapol_pkt = skb_copy(pkt, GFP_ATOMIC);
			if (cur_if->pend_eapol_pkt) {
#ifdef EAPOL_DYNAMATIC_RESEND
				osl_do_gettimeofday(&cur_if->eapol_tx_ts);
				if (cur_if->eapol_retry)
					interval = cur_if->eapol_max_intvl;
				else
					interval = (cur_if->eapol_avg_intvl + cur_if->eapol_max_intvl) / 2;
				if (interval <= 20) {
					cur_if->eapol_avg_intvl = (cur_if->eapol_max_intvl+STA_EAPOL_TIMEOUT)/2;
					cur_if->eapol_cnt = 1;
				}
				cur_if->eapol_resend_intvl = interval;
#else
				interval = STA_EAPOL_TIMEOUT;
#endif /* EAPOL_DYNAMATIC_RESEND */
				wl_timer_mod(dhd, &cur_if->eapol_timer, interval);
				IAPSTA_TRACE(cur_if->dev->name, "backup eapol pkt\n");
			}
			spin_unlock_irqrestore(&apsta_params->eapol_lock, flags);
		}
	}
}

static void
wl_resend_eapol_handler(struct wl_if_info *cur_if,
	const wl_event_msg_t *e, void *data)
{
	struct dhd_pub *dhd = dhd_get_pub(cur_if->dev);
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);
	struct net_device *dev = cur_if->dev;
	uint32 etype = ntoh32(e->event_type);
	uint32 reason = ntoh32(e->reason);
	unsigned long flags = 0;
	bool pending = FALSE;
	void *pend_eapol_pkt = NULL;

	if (etype == WLC_E_RESERVED && reason == ISAM_RC_EAPOL_RESEND) {
		spin_lock_irqsave(&apsta_params->eapol_lock, flags);
		if (cur_if->pend_eapol_pkt && wl_ext_sta_connecting(dev)) {
			pend_eapol_pkt = skb_copy(cur_if->pend_eapol_pkt, GFP_ATOMIC);
			if (pend_eapol_pkt) {
#ifdef EAPOL_DYNAMATIC_RESEND
				cur_if->eapol_retry = TRUE;
				IAPSTA_INFO(dev->name, "resend eapol pkt %d(%d/%d/%d/%d), cnt=%d\n",
					cur_if->eapol_resend_intvl,
					cur_if->eapol_min_intvl, cur_if->eapol_avg_intvl,
					cur_if->eapol_max_intvl, STA_EAPOL_TIMEOUT,
					cur_if->eapol_cnt);
#else
				IAPSTA_INFO(dev->name, "resend eapol pkt %dms\n", STA_EAPOL_TIMEOUT);
#endif /* EAPOL_DYNAMATIC_RESEND */
				pending = TRUE;
			}
		}
		spin_unlock_irqrestore(&apsta_params->eapol_lock, flags);
		if (pending) {
			dhd_sendpkt(dhd, cur_if->ifidx, pend_eapol_pkt);
		}
	}
}

static void
wl_eapol_timer(unsigned long data)
{
	struct net_device *dev = (struct net_device *)data;
	struct dhd_pub *dhd;
	wl_event_msg_t msg;

	if (!dev) {
		IAPSTA_ERROR("wlan", "dev is not ready\n");
		return;
	}

	dhd = dhd_get_pub(dev);

	bzero(&msg, sizeof(wl_event_msg_t));
	IAPSTA_TRACE(dev->name, "timer expired\n");

	msg.ifidx = dhd_net2idx(dhd->info, dev);
	msg.event_type = hton32(WLC_E_RESERVED);
	msg.reason = hton32(ISAM_RC_EAPOL_RESEND);
	wl_ext_event_send(dhd->event_params, &msg, NULL);
}
#endif /* EAPOL_RESEND */

#ifdef KEY_INSTALL_CHECK
static void
wl_ext_key_install_timeout(unsigned long data)
{
	struct net_device *dev = (struct net_device *)data;
	struct dhd_pub *dhd;
	wl_event_msg_t msg;

	if (!dev) {
		IAPSTA_ERROR("wlan", "dev is not ready\n");
		return;
	}

	dhd = dhd_get_pub(dev);

	bzero(&msg, sizeof(wl_event_msg_t));
	IAPSTA_TRACE(dev->name, "timer expired\n");

	msg.ifidx = dhd_net2idx(dhd->info, dev);
	msg.event_type = hton32(WLC_E_RESERVED);
	msg.reason = hton32(ISAM_RC_KEY_INSTALL);
	wl_ext_event_send(dhd->event_params, &msg, NULL);
}

static void
wl_ext_key_install_war(struct wl_if_info *cur_if)
{
	struct net_device *dev = cur_if->dev;
	struct dhd_pub *dhd = dhd_get_pub(dev);
	int wait_tmo = STA_KEY_INSTALL_INTERVAL;
#ifdef KEY_INSTALL_REASSOC
	struct ether_addr bssid;
#endif /* KEY_INSTALL_REASSOC */

	cur_if->key_install_cnt++;
	if (cur_if->key_install_cnt >= STA_KEY_INSTALL_MAX) {
#ifdef KEY_INSTALL_REASSOC
		if ((dhd->conf->war & STA_REASSOC_WAR) &&
				cur_if->key_install_cnt == STA_KEY_INSTALL_MAX) {
			IAPSTA_ERROR(dev->name, "key not installed, trigger reassoc\n");
#ifdef EAPOL_RESEND
			wl_ext_release_eapol_txpkt(dhd, cur_if->ifidx, FALSE);
#endif /* EAPOL_RESEND */
			wl_ext_update_conn_state(dhd, cur_if->ifidx, CONN_STATE_CONNECTED);
			wl_ext_associated(dev, &bssid);
			wl_cfgext_trigger_reassoc_scan(dev, &bssid, SCAN_RCC, STA_CONNECT_TIMEOUT);
			wl_timer_mod(dhd, &cur_if->key_install_timer, STA_CONNECT_TIMEOUT);
		} else if (cur_if->key_install_cnt < (STA_KEY_INSTALL_MAX * 2)) {
			IAPSTA_INFO(dev->name, "check key installed %dms later, cnt=%d\n",
				wait_tmo, cur_if->key_install_cnt);
			wl_timer_mod(dhd, &cur_if->key_install_timer, wait_tmo);
		} else
#endif /* KEY_INSTALL_REASSOC */
		{
			IAPSTA_ERROR(dev->name, "key not installed, trigger disconnect\n");
#ifdef EAPOL_RESEND
			wl_ext_release_eapol_txpkt(dhd, cur_if->ifidx, FALSE);
#endif /* EAPOL_RESEND */
			wl_ext_update_conn_state(dhd, cur_if->ifidx, CONN_STATE_CONNECTED);
#ifdef KEY_INSTALL_REASSOC
			wl_ext_cancel_reassoc(cur_if);
#endif /* KEY_INSTALL_REASSOC */
			wl_ext_ioctl(dev, WLC_DISASSOC, NULL, 0, 1);
			wl_timer_mod(dhd, &cur_if->key_install_timer, 0);
			cur_if->key_install_cnt = 0;
			wl_ext_sync_interface(dev);
		}
	}
	else {
		IAPSTA_INFO(dev->name, "check key installed %dms later, cnt=%d\n",
			wait_tmo, cur_if->key_install_cnt);
		wl_timer_mod(dhd, &cur_if->key_install_timer, wait_tmo);
	}
}

static void
wl_ext_key_install_handler(struct wl_if_info *cur_if,
	const wl_event_msg_t *e, void *data)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);
	struct net_device *dev = cur_if->dev;
	struct dhd_pub *dhd = dhd_get_pub(dev);
	struct bcm_cfg80211 *cfg = wl_get_cfg(dev);
	uint32 etype = ntoh32(e->event_type);
	uint32 reason = ntoh32(e->reason);
	int err, key_installed = 0;

	if (etype == WLC_E_RESERVED && reason == ISAM_RC_KEY_INSTALL) {
		mutex_lock(&apsta_params->in4way_sync);
		if (cur_if->conn_state >= CONN_STATE_4WAY_M4 &&
				!wl_get_drv_status(cfg, DISCONNECTING, dev) &&
				wl_get_drv_status(cfg, CONNECTED, dev)) {
			err = wldev_iovar_getint(dev, "buf_key_b4_m4_installed", &key_installed);
			if (!err && !key_installed) {
				wl_ext_key_install_war(cur_if);
			} else {
#ifdef EAPOL_RESEND
				wl_ext_release_eapol_txpkt(dhd, cur_if->ifidx, FALSE);
#endif /* EAPOL_RESEND */
				IAPSTA_INFO(dev->name, "key installed\n");
				wl_timer_mod(dhd, &cur_if->connect_timer, 0);
				wl_ext_update_conn_state(dhd, cur_if->ifidx, CONN_STATE_CONNECTED);
#ifdef ARP_DETECTION
				wl_ext_trigger_arp(dev);
#endif /* ARP_DETECTION */
#ifdef WL_ESCAN_ROAM_CACHE
				wl_escan_add_roam_cache(cur_if->dev);
#endif /* WL_ESCAN_ROAM_CACHE */
				cur_if->key_install_cnt = 0;
				wl_ext_sync_interface(dev);
			}
		}
		mutex_unlock(&apsta_params->in4way_sync);
	}
	return;
}

static int
wl_key_installed(struct wl_if_info *cur_if)
{
	struct net_device *dev = cur_if->dev;
	dhd_pub_t *dhd = dhd_get_pub(dev);
	int err, key_installed = 0;

	err = wldev_iovar_getint(dev, "buf_key_b4_m4_installed", &key_installed);
	if (err) {
		IAPSTA_INFO(dev->name, "not supported %d\n", err);
		key_installed = 1;
	} else if (key_installed)
		IAPSTA_INFO(dev->name, "key installed\n");

	if (key_installed) {
		wl_timer_mod(dhd, &cur_if->key_install_timer, 0);
		cur_if->key_install_cnt = 0;
	}
	else {
		IAPSTA_INFO(dev->name, "check key installed %dms later\n", STA_KEY_INSTALL_INTERVAL);
		wl_timer_mod(dhd, &cur_if->key_install_timer, STA_KEY_INSTALL_INTERVAL);
	}

	return key_installed;
}
#endif /* KEY_INSTALL_CHECK */

#if defined(WL_CFG80211) && defined(WL_ROAM_PMK_WAR)
void
wl_ext_pmk_set(struct net_device *dev, wsec_pmk_t *pmk, bool set)
{
	struct wl_if_info *cur_if;

	cur_if = wl_get_cur_if(dev);
	if (cur_if && cur_if->ifmode == ISTA_MODE) {
		if (set) {
			memcpy(&cur_if->pmk, pmk, sizeof(wsec_pmk_t));
			IAPSTA_INFO(dev->name, "backup pmk\n");
		} else {
			memset(&cur_if->pmk, 0, sizeof(wsec_pmk_t));
			IAPSTA_INFO(dev->name, "clear pmk\n");
		}
	}
}

static void
wl_ext_pmk_update(struct wl_if_info *cur_if)
{
	struct net_device *dev = cur_if->dev;
	struct bcm_cfg80211 *cfg = wl_get_cfg(dev);
	int wpa_auth = 0;

	wl_ext_iovar_getint(dev, "wpa_auth", &wpa_auth);
	if ((cfg->auth_type == DOT11_OPEN_SYSTEM) && (wpa_auth & WPA2_AUTH_FT) &&
			cur_if->pmk.key_len) {
		wl_ext_ioctl(dev, WLC_SET_WSEC_PMK, &cur_if->pmk, sizeof(wsec_pmk_t), 1);
		IAPSTA_INFO(dev->name, "pmk updated\n");
	}
}
#endif /* WL_CFG80211 && WL_ROAM_PMK_WAR */

#ifdef SCAN_SUPPRESS
static void
wl_ext_light_scan_prep(struct net_device *dev, void *scan_params, bool scan_v2)
{
	wl_scan_params_v1_t *params = NULL;
	wl_scan_params_v2_t *params_v2 = NULL;

	if (!scan_params) {
		IAPSTA_ERROR(dev->name, "NULL scan_params\n");
		return;
	}
	IAPSTA_INFO(dev->name, "Enter\n");

	if (scan_v2) {
		params_v2 = (wl_scan_params_v2_t *)scan_params;
	} else {
		params = (wl_scan_params_v1_t *)scan_params;
	}

	if (params_v2) {
		/* scan params ver2 */
		params_v2->nprobes = 1;
		params_v2->active_time = 20;
		params_v2->home_time = 150;
	} else {
		/* scan params ver 1 */
		if (!params) {
			ASSERT(0);
			return;
		}
		params->nprobes = 1;
		params->active_time = 20;
		params->home_time = 150;
	}

	return;
}

static uint16
wl_ext_max_tput_chan(struct wl_apsta_params *apsta_params,
	struct wl_ext_chan_info *chan_info)
{
	struct wl_if_info *tmp_if, *max_tput_if = NULL;
	struct wl_ext_chan_info tmp_chan_info;
	int32 tput_sum = 0;
	int i;

	for (i=0; i<MAX_IF_NUM; i++) {
		tmp_if = &apsta_params->if_info[i];
		if (tmp_if->dev && (tmp_if->tput_info.tput_tx + tmp_if->tput_info.tput_rx) > tput_sum) {
			if (wl_ext_get_chanspec(tmp_if->dev, &tmp_chan_info)) {
				max_tput_if = tmp_if;
				tput_sum = tmp_if->tput_info.tput_tx + tmp_if->tput_info.tput_rx;
				memcpy(chan_info, &tmp_chan_info, sizeof(struct wl_ext_chan_info));
			}
		}
	}

	if (max_tput_if)
		IAPSTA_INFO(max_tput_if->dev->name, "chan=%s-%d\n",
			WLCBAND2STR(chan_info->band), chan_info->chan);

	return chan_info->chan;
}

uint16
wl_ext_scan_suppress(struct net_device *dev, void *scan_params, bool scan_v2,
	struct wl_ext_chan_info *chan_info)
{
	struct dhd_pub *dhd = dhd_get_pub(dev);
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(dev);
	struct dhd_conf *conf = dhd->conf;

	if (!(conf->scan_intput & (SCAN_CURCHAN_INTPUT|SCAN_LIGHT_INTPUT)))
		return 0;

	memset(chan_info, 0, sizeof(struct wl_ext_chan_info));
	if (apsta_params->tput_sum >= conf->scan_tput_thresh) {
		IAPSTA_INFO(dev->name, "tput %dMbps >= %dMbps (busy cnt/thresh %d/%d)\n",
			apsta_params->tput_sum, conf->scan_tput_thresh,
			apsta_params->scan_busy_cnt, conf->scan_busy_thresh);
		if (apsta_params->scan_busy_cnt >= conf->scan_busy_thresh) {
			apsta_params->scan_busy_cnt = 0;
		} else if (conf->scan_intput & SCAN_CURCHAN_INTPUT) {
			wl_ext_max_tput_chan(apsta_params, chan_info);
		}
		if ((conf->scan_intput & SCAN_LIGHT_INTPUT) && !chan_info->chan)
			wl_ext_light_scan_prep(dev, scan_params, scan_v2);
		apsta_params->scan_busy_cnt++;
	} else {
		apsta_params->scan_busy_cnt = 0;
	}

	return chan_info->chan;
}

void
wl_ext_reset_scan_busy(dhd_pub_t *dhd)
{
	struct wl_apsta_params *apsta_params = (struct wl_apsta_params *)dhd->iapsta_params;
	apsta_params->scan_busy_cnt = 0;
}
#endif /* SCAN_SUPPRESS */

static int
wl_ext_scan_busy(struct wl_if_info *cur_if, uint action)
{
	struct net_device *dev = cur_if->dev;
	dhd_pub_t *dhd = dhd_get_pub(dev);
#ifdef WL_CFG80211
	struct bcm_cfg80211 *cfg = wl_get_cfg(dev);
#endif /* WL_CFG80211 */
#ifdef SCAN_SUPPRESS
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(dev);
	struct dhd_conf *conf = dhd->conf;
#endif /* SCAN_SUPPRESS */
	struct osl_timespec *sta_conn_ts = &cur_if->sta_conn_ts;
	struct osl_timespec cur_ts;
	uint32 diff_ms;
	int ret = 0, suppressed = 0;
	int cur_conn_state = cur_if->conn_state;
	bool connecting = FALSE;

	connecting = wl_ext_sta_connecting(dev);

	wldev_ioctl(dev, WLC_GET_SCANSUPPRESS, &suppressed, sizeof(int), false);
	if (suppressed) {
		IAPSTA_ERROR(dev->name, "scan suppressed\n");
		ret = -EBUSY;
		goto exit;
	}
#ifdef WL_ESCAN
	if (dhd->escan->escan_state == ESCAN_STATE_SCANING) {
		IAPSTA_ERROR(dev->name, "escan scanning\n");
		ret = -EBUSY;
		goto exit;
	}
#endif /* WL_ESCAN */
#ifdef WL_CFG80211
	if (wl_get_drv_status_all(cfg, SCANNING) && cfg->scan_request) {
		IAPSTA_ERROR(dev->name, "cfg80211 scanning\n");
		ret = -EAGAIN;
		goto exit;
	}
#endif /* WL_CFG80211 */

#ifdef SCAN_SUPPRESS
	if (conf->scan_intput & NO_SCAN_INTPUT) {
		if (apsta_params->tput_sum >= conf->scan_tput_thresh) {
			if (apsta_params->scan_busy_cnt) {
				osl_do_gettimeofday(&cur_ts);
				diff_ms = osl_do_gettimediff(&cur_ts, &apsta_params->scan_busy_ts) / 1000;
				if ((diff_ms / 1000) >= conf->scan_busy_tmo) {
					apsta_params->scan_busy_cnt = 0;
					IAPSTA_INFO(dev->name, "reset scan_busy_cnt\n");
					goto exit;
				}
			}
			if (apsta_params->scan_busy_cnt >= conf->scan_busy_thresh) {
				apsta_params->scan_busy_cnt = 0;
			} else if (conf->scan_intput & NO_SCAN_INTPUT) {
				IAPSTA_INFO(dev->name,
					"no scan in tput %dMbps >= %dMbps(busy cnt/thresh %d/%d)\n",
					apsta_params->tput_sum, conf->scan_tput_thresh,
					apsta_params->scan_busy_cnt, conf->scan_busy_thresh);
				apsta_params->scan_busy_cnt++;
				if (apsta_params->scan_busy_cnt == 1)
					osl_do_gettimeofday(&apsta_params->scan_busy_ts);
				ret = -EBUSY;
				goto exit;
			}
		}
		else {
			apsta_params->scan_busy_cnt = 0;
		}
	}
#endif /* SCAN_SUPPRESS */

	if (action & STA_NO_SCAN_IN4WAY) {
		osl_do_gettimeofday(&cur_ts);
		diff_ms = osl_do_gettimediff(&cur_ts, sta_conn_ts) / 1000;
		if (connecting && diff_ms <= STA_CONNECT_TIMEOUT) {
			IAPSTA_ERROR(dev->name, "connecting... %d\n", cur_conn_state);
			ret = -EBUSY;
			goto exit;
		}
	}

exit:
	return ret;
}

#ifdef SET_CARRIER
static void
wl_ext_net_setcarrier(struct wl_if_info *cur_if, bool on, bool force)
{
	IAPSTA_TRACE(cur_if->ifname, "carrier=%d\n", on);
	if (on) {
		if (!netif_carrier_ok(cur_if->dev) || force)
			netif_carrier_on(cur_if->dev);
	} else {
		if (netif_carrier_ok(cur_if->dev) || force)
			netif_carrier_off(cur_if->dev);
	}
}
#endif /* SET_CARRIER */

static void
wl_set_btc_in4way(struct wl_if_info *cur_if, enum wl_ext_status status, bool disable)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);
	struct net_device *dev = cur_if->dev;
	int err;

	if (cur_if->ifidx == 0) {
		if (disable) {
			err = wldev_iovar_getint(dev, "btc_mode", &apsta_params->sta_btc_mode);
			if (!err && apsta_params->sta_btc_mode) {
				IAPSTA_INFO(dev->name, "status=%d, disable current btc_mode %d\n",
					status, apsta_params->sta_btc_mode);
				wldev_iovar_setint(dev, "btc_mode", 0);
			}
		} else {
			if (apsta_params->sta_btc_mode) {
				IAPSTA_INFO(dev->name, "status=%d, restore btc_mode %d\n",
					status, apsta_params->sta_btc_mode);
				wldev_iovar_setint(dev, "btc_mode", apsta_params->sta_btc_mode);
				apsta_params->sta_btc_mode = 0;
			}
		}
	}
}

static void
wl_set_wl_in4way(struct wl_if_info *cur_if, bool enable)
{
	dhd_pub_t *dhd = dhd_get_pub(cur_if->dev);
	struct dhd_conf *conf = dhd->conf;

	if (cur_if->ifmode == ISTA_MODE) {
		/* Process wl_pre_in4way/wl_post_in4way:
		 * wl_pre_in4way=[cmd]=[val], [cmd]=[val]
		  * wl_pre_in4way=set_wificrt=1
		  * wl_post_in4way=set_wificrt=0
		 */
		if (enable && conf->wl_pre_in4way) {
			IAPSTA_INFO(cur_if->ifname, "%s\n", conf->wl_pre_in4way);
			dhd_conf_set_wl_cmd(dhd, conf->wl_pre_in4way, FALSE);
		} else if (!enable && conf->wl_post_in4way) {
			IAPSTA_INFO(cur_if->ifname, "%s\n", conf->wl_post_in4way);
			dhd_conf_set_wl_cmd(dhd, conf->wl_post_in4way, FALSE);
		}
	}
}

static void
wl_wait_disconnect(struct wl_if_info *cur_if, enum wl_ext_status status)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);
	struct net_device *dev = cur_if->dev;
	struct osl_timespec cur_ts, *sta_disc_ts = &cur_if->sta_disc_ts;
	int max_wait_time = 200, max_wait_cnt = 20;
	int cur_conn_state = cur_if->conn_state;
	uint32 diff_ms = 0;

	if (cur_conn_state > CONN_STATE_IDLE)
		osl_do_gettimeofday(sta_disc_ts);
	osl_do_gettimeofday(&cur_ts);
	diff_ms = osl_do_gettimediff(&cur_ts, sta_disc_ts)/1000;
	while (diff_ms < max_wait_time && max_wait_cnt) {
		IAPSTA_INFO(dev->name, "status=%d, max_wait_cnt=%d waiting...\n",
			status, max_wait_cnt);
		mutex_unlock(&apsta_params->in4way_sync);
		OSL_SLEEP(50);
		mutex_lock(&apsta_params->in4way_sync);
		max_wait_cnt--;
		osl_do_gettimeofday(&cur_ts);
		diff_ms = osl_do_gettimediff(&cur_ts, sta_disc_ts)/1000;
	}

}

void
wl_iapsta_wait_event_complete(struct dhd_pub *dhd)
{
	struct wl_apsta_params *apsta_params = dhd->iapsta_params;
	struct wl_if_info *cur_if;
	int i;

	for (i=0; i<MAX_IF_NUM; i++) {
		cur_if = &apsta_params->if_info[i];
		if (cur_if->dev && cur_if->ifmode == ISTA_MODE) {
			wl_ext_wait_event_complete(dhd, cur_if->ifidx);
		}
	}
}

static int
wl_iapsta_suspend_resume_ap(dhd_pub_t *dhd, struct wl_if_info *cur_if,
	int suspend)
{
	uint insuspend = 0;

	insuspend = dhd_conf_get_insuspend(dhd, ALL_IN_SUSPEND);
	if (insuspend)
		WL_MSG(cur_if->ifname, "suspend %d\n", suspend);

	if (suspend) {
		if (insuspend & AP_DOWN_IN_SUSPEND) {
			if (wl_ext_get_chanspec(cur_if->dev, &cur_if->chan_info))
				wl_ext_if_down(cur_if);
		}
	} else {
		if (insuspend & AP_DOWN_IN_SUSPEND) {
			if (cur_if->chan_info.chan)
				wl_ext_if_up(cur_if, FALSE, 0);
		}
	}

	return 0;
}

int
wl_iapsta_suspend_resume(dhd_pub_t *dhd, int suspend)
{
	struct wl_apsta_params *apsta_params = dhd->iapsta_params;
	struct wl_if_info *cur_if;
	int i;

#ifdef TPUT_MONITOR
	if (suspend)
		wl_timer_mod(dhd, &apsta_params->monitor_timer, 0);
#endif /* TPUT_MONITOR */
#ifdef RXF0OVFL_REINIT_WAR
	if (suspend)
		wl_timer_mod(dhd, &apsta_params->rxf0ovfl_timer, 0);
#endif

	for (i=0; i<MAX_IF_NUM; i++) {
		cur_if = &apsta_params->if_info[i];
		if (cur_if->dev && cur_if->ifmode == ISTA_MODE) {
			if (!suspend)
				memcpy(&dhd->conf->bssid_insuspend, &cur_if->bssid, ETHER_ADDR_LEN);
			dhd_conf_suspend_resume_sta(dhd, cur_if->ifidx, suspend);
			if (suspend)
				memcpy(&cur_if->bssid, &dhd->conf->bssid_insuspend, ETHER_ADDR_LEN);
		}
		else if (cur_if->dev && cur_if->ifmode == IAP_MODE) {
			wl_iapsta_suspend_resume_ap(dhd, cur_if, suspend);
		}
	}

#ifdef TPUT_MONITOR
	if (!suspend)
		wl_timer_mod(dhd, &apsta_params->monitor_timer, dhd->conf->tput_monitor_ms);
#endif /* TPUT_MONITOR */
#ifdef RXF0OVFL_REINIT_WAR
	if (!suspend) {
		if (dhd->conf->war & FW_REINIT_RXF0OVFL) {
			wl_timer_mod(dhd, &apsta_params->rxf0ovfl_timer, RXF0OVFL_POLLING_TIMEOUT);
		}
	}
#endif

	return 0;
}

static int
wl_ext_in4way_sync_sta(struct wl_if_info *cur_if, uint action,
	enum wl_ext_status status, void *context)
{
	dhd_pub_t *dhd = dhd_get_pub(cur_if->dev);
	struct wl_apsta_params *apsta_params = dhd->iapsta_params;
	struct dhd_conf *conf = dhd->conf;
	struct net_device *dev = cur_if->dev;
	struct osl_timespec cur_ts, *sta_disc_ts = &cur_if->sta_disc_ts;
	struct osl_timespec *sta_conn_ts = &cur_if->sta_conn_ts;
	uint32 diff_ms = 0;
	int ret = 0, wpa_auth = 0, key_installed = 1;
	uint cur_conn_state = cur_if->conn_state, conn_state;
	bool connecting = FALSE;
	wl_event_msg_t *e = (wl_event_msg_t *)context;
#ifdef WL_CFG80211
	struct bcm_cfg80211 *cfg = wl_get_cfg(dev);
#ifdef WL_ESCAN_ROAM_CACHE
	wlc_ssid_t *ssid = (struct wlc_ssid *)wl_read_prof(cfg, dev, WL_PROF_SSID);
#endif /* WL_ESCAN_ROAM_CACHE */
#endif /* WL_CFG80211 */

	action = action & conf->in4way;
#ifdef WL_CFG80211
	if ((conf->in4way & STA_FAKE_SCAN_IN_CONNECT) && (action & STA_NO_SCAN_IN4WAY))
		action &= ~(STA_NO_SCAN_IN4WAY);
#endif /* WL_CFG80211 */
#ifdef WL_EXT_RECONNECT
	if (cur_if->ifmode != ISTA_MODE)
		action &= ~(STA_RECONNECT_RETRY);
	if (!(dhd->conf->war & STA_REASSOC_WAR))
		action &= ~(STA_RECONNECT_RETRY);
#endif /* WL_EXT_RECONNECT */

	IAPSTA_TRACE(dev->name, "status=%d, action=0x%x, in4way=0x%x\n",
		status, action, conf->in4way);

	connecting = wl_ext_sta_connecting(dev);

	switch (status) {
		case WL_EXT_STATUS_SCAN:
			ret = wl_ext_scan_busy(cur_if, action);
			break;
#ifdef WL_CFG80211
		case WL_EXT_STATUS_SCANNING:
			if (action & STA_FAKE_SCAN_IN_CONNECT) {
				osl_do_gettimeofday(&cur_ts);
				diff_ms = osl_do_gettimediff(&cur_ts, sta_conn_ts)/1000;
				if (wl_get_drv_status(cfg, CONNECTING, dev) ||
						(connecting && diff_ms <= STA_CONNECT_TIMEOUT) ||
						(cur_if->empty_scan >= STA_EMPTY_SCAN_MAX)) {
					cur_if->empty_scan = 0;
					WL_MSG(dev->name, "FAKE SCAN\n");
					ret = -EBUSY;
				}
			}
			break;
		case WL_EXT_STATUS_SCAN_COMPLETE:
			if ((conf->war & FW_REINIT_EMPTY_SCAN) &&
					cfg->bss_list->count == 0 && !p2p_scan(cfg)) {
				osl_do_gettimeofday(&cur_ts);
				diff_ms = osl_do_gettimediff(&cur_ts, sta_disc_ts)/1000;
				cur_if->empty_scan++;
				if ((wl_ext_associated(dev, NULL) && cur_if->empty_scan >= STA_EMPTY_SCAN_MAX) ||
						(diff_ms < STA_LINKDOWN_TIMEOUT &&
						apsta_params->linkdown_reason == WLC_E_LINK_BCN_LOSS)) {
					if (conf->chip == BCM43569_CHIP_ID) {
						if (wl_ext_associated(dev, NULL)) {
							IAPSTA_INFO(dev->name, "wl disassoc for empty scan\n");
							wl_ext_ioctl(dev, WLC_DISASSOC, NULL, 0, 1);
						}
					} else {
						IAPSTA_INFO(dev->name, "wl reinit for empty scan\n");
						wl_ext_ioctl(dev, WLC_INIT, NULL, 0, 1);
					}
				}
			} else {
				cur_if->empty_scan = 0;
			}
			break;
#endif /* WL_CFG80211 */
		case WL_EXT_STATUS_PRE_DISCONNECTING:
#ifdef EAPOL_RESEND
			wl_ext_release_eapol_txpkt(dhd, cur_if->ifidx, FALSE);
#endif /* EAPOL_RESEND */
#ifdef KEY_INSTALL_CHECK
			wl_timer_mod(dhd, &cur_if->key_install_timer, 0);
			cur_if->key_install_cnt = 0;
#endif /* KEY_INSTALL_CHECK */
			wl_timer_mod(dhd, &cur_if->connect_timer, 0);
#ifdef WL_EXT_REASSOC
			wl_ext_cancel_reassoc(cur_if);
#endif /* WL_EXT_REASSOC */
#ifdef WL_EXT_RECONNECT
			wl_timer_mod(dhd, &cur_if->reconnect_timer, 0);
#ifdef WL_EXT_DISCONNECT_RECONNECT
			cur_if->sta_disc_recon_cnt = 0;
#endif /* WL_EXT_DISCONNECT_RECONNECT */
#endif /* WL_EXT_RECONNECT */
#ifdef SCAN_SUPPRESS
			apsta_params->scan_busy_cnt = 0;
#endif /* SCAN_SUPPRESS */
#if defined(WL_CFG80211) && defined(WL_ROAM_PMK_WAR)
			wl_ext_pmk_set(dev, NULL, FALSE);
#endif /* WL_CFG80211 && WL_ROAM_PMK_WAR */
			break;
		case WL_EXT_STATUS_DISCONNECTING:
			if (connecting) {
				IAPSTA_ERROR(dev->name, "connect failed at %d\n", cur_conn_state);
				wl_ext_update_conn_state(dhd, cur_if->ifidx, CONN_STATE_IDLE);
			}
			if (action & STA_NO_BTC_IN4WAY) {
				wl_set_btc_in4way(cur_if, status, FALSE);
			}
			wl_set_wl_in4way(cur_if, FALSE);
#ifdef BTC_WAR
			wl_ext_btc_config(dev, FALSE);
#endif /* BTC_WAR */
			if (action & STA_WAIT_DISCONNECTED) {
				wl_wait_disconnect(cur_if, status);
				wake_up_interruptible(&conf->event_complete);
			}
			break;
		case WL_EXT_STATUS_CONNECTING:
			wl_ext_iovar_getint(dev, "wpa_auth", &wpa_auth);
#ifdef WL_EXT_RECONNECT
			if (action & STA_RECONNECT_RETRY) {
				wl_ext_set_connect_retry(dev, context);
			}
#ifdef WL_EXT_DISCONNECT_RECONNECT
			cur_if->sta_disc_recon_cnt = 0;
#endif /* WL_EXT_DISCONNECT_RECONNECT */
#endif /* WL_EXT_RECONNECT */
			if (wpa_auth & WPA2_AUTH_UNSPECIFIED)
				wl_timer_mod(dhd, &cur_if->connect_timer, STA_EAP_CONNECT_TIMEOUT);
			else
				wl_timer_mod(dhd, &cur_if->connect_timer, STA_CONNECT_TIMEOUT);
			osl_do_gettimeofday(sta_conn_ts);
			wl_ext_update_conn_state(dhd, cur_if->ifidx, CONN_STATE_CONNECTING);
#ifdef WL_ESCAN_ROAM_CACHE
			wl_escan_init_roam_cache(cur_if->dev, ssid);
#endif /* WL_ESCAN_ROAM_CACHE */
			if (wpa_auth >= WPA_AUTH_UNSPECIFIED) {
				if (action & STA_NO_BTC_IN4WAY) {
					wl_set_btc_in4way(cur_if, status, TRUE);
				}
				wl_set_wl_in4way(cur_if, TRUE);
			}
			break;
		case WL_EXT_STATUS_RECONNECTING:
			wl_ext_update_conn_state(dhd, cur_if->ifidx, CONN_STATE_CONNECTING);
#ifdef EAPOL_RESEND
			wl_ext_release_eapol_txpkt(dhd, cur_if->ifidx, FALSE);
#endif /* EAPOL_RESEND */
			break;
		case WL_EXT_STATUS_CONNECTED:
			wl_ext_iovar_getint(dev, "wpa_auth", &wpa_auth);
#ifdef WL_EXT_REASSOC
			wl_ext_cancel_reassoc(cur_if);
#endif /* WL_EXT_REASSOC */
#ifdef WL_EXT_RECONNECT
			if (action & STA_RECONNECT_RETRY)
				wl_ext_set_eapol_timeout(cur_if, wpa_auth);
#endif /* WL_EXT_RECONNECT */
			if (wpa_auth >= WPA_AUTH_UNSPECIFIED) {
				wl_ext_update_conn_state(dhd, cur_if->ifidx, CONN_STATE_ASSOCIATED);
			} else {
				wl_timer_mod(dhd, &cur_if->connect_timer, 0);
				wl_ext_update_conn_state(dhd, cur_if->ifidx, CONN_STATE_CONNECTED);
#ifdef WL_ESCAN_ROAM_CACHE
				wl_escan_add_roam_cache(cur_if->dev);
#endif /* WL_ESCAN_ROAM_CACHE */
#ifdef BTC_WAR
				wl_ext_btc_config(dev, TRUE);
#endif /* BTC_WAR */
#ifdef WL_CFG80211
				wl_ext_sync_interface(dev);
#endif /* WL_CFG80211 */
			}
			if (cur_if->ifmode == ISTA_MODE) {
				dhd_conf_set_wme(dhd, cur_if->ifidx, 0);
				wake_up_interruptible(&conf->event_complete);
			}
			else if (cur_if->ifmode == IGC_MODE) {
				dhd_conf_set_mchan_bw(dhd, WL_P2P_IF_CLIENT, -1);
			}
			break;
		case WL_EXT_STATUS_ROAMED:
			wl_ext_iovar_getint(dev, "wpa_auth", &wpa_auth);
#ifdef WL_EXT_REASSOC
			wl_ext_cancel_reassoc(cur_if);
#endif /* WL_EXT_REASSOC */
#ifdef WL_EXT_RECONNECT
			if (action & STA_RECONNECT_RETRY)
				wl_ext_set_eapol_timeout(cur_if, wpa_auth);
#endif /* WL_EXT_RECONNECT */
#ifdef KEY_INSTALL_CHECK
			wl_timer_mod(dhd, &cur_if->key_install_timer, 0);
#endif /* KEY_INSTALL_CHECK */
			if ((cfg->auth_type == DOT11_FAST_BSS) && (wpa_auth & WPA2_AUTH_FT)) {
				wl_ext_update_conn_state(dhd, cur_if->ifidx, CONN_STATE_CONNECTED);
				wl_timer_mod(dhd, &cur_if->connect_timer, 0);
#ifdef WL_ESCAN_ROAM_CACHE
				wl_escan_add_roam_cache(cur_if->dev);
#endif /* WL_ESCAN_ROAM_CACHE */
#ifdef ARP_DETECTION
				wl_ext_trigger_arp(dev);
#endif /* ARP_DETECTION */
#ifdef WL_CFG80211
				wl_ext_sync_interface(dev);
#endif /* WL_CFG80211 */
			}
			else if (wpa_auth >= WPA_AUTH_UNSPECIFIED) {
				if (cur_if->conn_state >= CONN_STATE_CONNECTED) {
					if (wpa_auth & WPA2_AUTH_UNSPECIFIED)
						wl_timer_mod(dhd, &cur_if->connect_timer, STA_EAP_CONNECT_TIMEOUT);
					else
						wl_timer_mod(dhd, &cur_if->connect_timer, STA_CONNECT_TIMEOUT);
					osl_do_gettimeofday(sta_conn_ts);
				}
				if (cur_conn_state < CONN_STATE_ASSOCIATED)
					wl_ext_update_conn_state(dhd, cur_if->ifidx, CONN_STATE_ASSOCIATED);
#if defined(WL_CFG80211) && defined(WL_ROAM_PMK_WAR)
				wl_ext_pmk_update(cur_if);
#endif /* WL_CFG80211 && WL_ROAM_PMK_WAR */
				wl_set_wl_in4way(cur_if, TRUE);
#ifdef BTC_WAR
				wl_ext_btc_config(dev, TRUE);
#endif /* BTC_WAR */
			} else {
				wl_ext_update_conn_state(dhd, cur_if->ifidx, CONN_STATE_CONNECTED);
				wl_timer_mod(dhd, &cur_if->connect_timer, 0);
#ifdef WL_ESCAN_ROAM_CACHE
				wl_escan_add_roam_cache(cur_if->dev);
#endif /* WL_ESCAN_ROAM_CACHE */
#ifdef WL_CFG80211
				wl_ext_sync_interface(dev);
#endif /* WL_CFG80211 */
			}
			if (cur_if->ifmode == ISTA_MODE) {
				dhd_conf_set_wme(dhd, cur_if->ifidx, 0);
				wake_up_interruptible(&conf->event_complete);
			}
			break;
#ifdef WL_EXT_RECONNECT
		case WL_EXT_STATUS_RECONNECT:
#ifdef EAPOL_RESEND
			wl_ext_release_eapol_txpkt(dhd, cur_if->ifidx, FALSE);
#endif /* EAPOL_RESEND */
			if (action & STA_RECONNECT_RETRY) {
				ret = wl_ext_connect_retry(dev, e);
			}
			break;
#endif /* WL_EXT_RECONNECT */
		case WL_EXT_STATUS_DISCONNECTED:
#ifdef EAPOL_RESEND
			wl_ext_release_eapol_txpkt(dhd, cur_if->ifidx, FALSE);
#endif /* EAPOL_RESEND */
#ifdef KEY_INSTALL_CHECK
			wl_timer_mod(dhd, &cur_if->key_install_timer, 0);
			cur_if->key_install_cnt = 0;
#endif /* KEY_INSTALL_CHECK */
#ifdef WL_EXT_REASSOC
			wl_ext_cancel_reassoc(cur_if);
#endif /* WL_EXT_REASSOC */
#ifdef WL_EXT_RECONNECT
			wl_timer_mod(dhd, &cur_if->reconnect_timer, 0);
#ifdef WL_EXT_DISCONNECT_RECONNECT
			cur_if->sta_disc_recon_cnt = 0;
#endif /* WL_EXT_DISCONNECT_RECONNECT */
#endif /* WL_EXT_RECONNECT */
#ifdef SCAN_SUPPRESS
			apsta_params->scan_busy_cnt = 0;
#endif /* SCAN_SUPPRESS */
			if (e && ntoh32(e->event_type) == WLC_E_LINK &&
					!(ntoh16(e->flags) & WLC_EVENT_MSG_LINK)) {
				apsta_params->linkdown_reason = ntoh32(e->reason);
			}
			wl_timer_mod(dhd, &cur_if->connect_timer, 0);
			if (connecting) {
				IAPSTA_ERROR(dev->name, "connect failed at %d\n", cur_conn_state);
			}
			wl_ext_update_conn_state(dhd, cur_if->ifidx, CONN_STATE_IDLE);
#if defined(WL_CFG80211) && defined(WL_ROAM_PMK_WAR)
			wl_ext_pmk_set(dev, NULL, FALSE);
#endif /* WL_CFG80211 && WL_ROAM_PMK_WAR */
			if (action & STA_NO_BTC_IN4WAY) {
				wl_set_btc_in4way(cur_if, status, FALSE);
			}
			wl_set_wl_in4way(cur_if, FALSE);
#ifdef WL_CFG80211
			wl_ext_sync_interface(dev);
#endif /* WL_CFG80211 */
#ifdef BTC_WAR
			wl_ext_btc_config(dev, FALSE);
#endif /* BTC_WAR */
			osl_do_gettimeofday(sta_disc_ts);
			wake_up_interruptible(&conf->event_complete);
			break;
		case WL_EXT_STATUS_ADD_KEY:
			conn_state = CONN_STATE_ADD_KEY;
			wl_timer_mod(dhd, &cur_if->connect_timer, 0);
#ifdef WL_EXT_REASSOC
			wl_ext_cancel_reassoc(cur_if);
#endif /* WL_EXT_REASSOC */
#ifdef WL_EXT_RECONNECT
			wl_timer_mod(dhd, &cur_if->reconnect_timer, 0);
#endif /* WL_EXT_RECONNECT */
#ifdef KEY_INSTALL_CHECK
			wl_ext_iovar_getint(dev, "wpa_auth", &wpa_auth);
			if (wpa_auth != WPA_AUTH_DISABLED && wpa_auth != WPA_AUTH_PSK) {
				key_installed = wl_key_installed(cur_if);
			}
#endif /* KEY_INSTALL_CHECK */
			if (key_installed) {
				conn_state = CONN_STATE_CONNECTED;
#ifdef WL_ESCAN_ROAM_CACHE
				wl_escan_add_roam_cache(cur_if->dev);
#endif /* WL_ESCAN_ROAM_CACHE */
#ifdef ARP_DETECTION
				wl_ext_trigger_arp(dev);
#endif /* ARP_DETECTION */
#ifdef WL_CFG80211
				wl_ext_sync_interface(dev);
#endif /* WL_CFG80211 */
			}
			IAPSTA_INFO(dev->name, "WPA 4-WAY complete %d => %d\n",
				cur_conn_state, conn_state);
			wl_ext_update_conn_state(dhd, cur_if->ifidx, conn_state);
#ifdef EAPOL_RESEND
			if (key_installed)
				wl_ext_release_eapol_txpkt(dhd, cur_if->ifidx, FALSE);
#endif /* EAPOL_RESEND */
			if (action & STA_NO_BTC_IN4WAY) {
				wl_set_btc_in4way(cur_if, status, FALSE);
			}
			wl_set_wl_in4way(cur_if, FALSE);
#ifdef BTC_WAR
			wl_ext_btc_config(dev, TRUE);
#endif /* BTC_WAR */
			wake_up_interruptible(&conf->event_complete);
			break;
		default:
			IAPSTA_INFO(dev->name, "Unknown action=0x%x, status=%d\n", action, status);
	}

	return ret;
}

#ifdef WL_CFG80211
static int
wl_ext_in4way_sync_ap(struct wl_if_info *cur_if,
	uint action, enum wl_ext_status status, void *context)
{
	dhd_pub_t *dhd = dhd_get_pub(cur_if->dev);
	struct wl_apsta_params *apsta_params = dhd->iapsta_params;
	struct net_device *dev = cur_if->dev;
	struct osl_timespec cur_ts, *ap_disc_sta_ts = &cur_if->ap_disc_sta_ts;
	u8 *ap_disc_sta_bssid = (u8*)&cur_if->ap_disc_sta_bssid;
	uint32 diff_ms = 0, timeout, max_wait_time = 600;
	int ret = 0, suppressed = 0;
	u8* mac_addr = context;
	bool wait = FALSE;

	action = action & dhd->conf->in4way;
	IAPSTA_TRACE(dev->name, "status=%d, action=0x%x, in4way=0x%x\n",
		status, action, dhd->conf->in4way);

	switch (status) {
		case WL_EXT_STATUS_SCAN:
			wldev_ioctl(dev, WLC_GET_SCANSUPPRESS, &suppressed, sizeof(int), false);
			if (suppressed) {
				IAPSTA_ERROR(dev->name, "scan suppressed\n");
				ret = -EBUSY;
				break;
			}
			break;
		case WL_EXT_STATUS_AP_ENABLING:
#ifdef RESTART_AP_WAR
			wl_timer_mod(dhd, &cur_if->restart_ap_timer, AP_RESTART_TIMEOUT);
#endif /* RESTART_AP_WAR */
			break;
		case WL_EXT_STATUS_AP_ENABLED:
#ifdef RESTART_AP_WAR
			wl_timer_mod(dhd, &cur_if->restart_ap_timer, 0);
#endif /* RESTART_AP_WAR */
			if (cur_if->ifmode == IAP_MODE)
				dhd_conf_set_wme(dhd, cur_if->ifidx, 1);
			else if (cur_if->ifmode == IGO_MODE)
				dhd_conf_set_mchan_bw(dhd, WL_P2P_IF_GO, -1);
			break;
		case WL_EXT_STATUS_AP_DISABLING:
#ifdef RESTART_AP_WAR
			wl_timer_mod(dhd, &cur_if->restart_ap_timer, 0);
#endif /* RESTART_AP_WAR */
			break;
		case WL_EXT_STATUS_DELETE_STA:
			if (action & AP_WAIT_STA_RECONNECT) {
				osl_do_gettimeofday(&cur_ts);
				diff_ms = osl_do_gettimediff(&cur_ts, ap_disc_sta_ts)/1000;
				if (cur_if->ifmode == IAP_MODE &&
						mac_addr && diff_ms < max_wait_time &&
						!memcmp(ap_disc_sta_bssid, mac_addr, ETHER_ADDR_LEN)) {
					wait = TRUE;
				} else if (cur_if->ifmode == IGO_MODE &&
						cur_if->conn_state == CONN_STATE_WSC_DONE &&
						memcmp(&ether_bcast, mac_addr, ETHER_ADDR_LEN)) {
					int auth, wpa_auth = 0;
					wl_ext_iovar_getint(dev, "auth", &auth);
					wl_ext_iovar_getint(dev, "wpa_auth", &wpa_auth);
					if (auth == WL_AUTH_SAE_KEY || wpa_auth&(WPA3_AUTH_SAE_PSK|0x20)) {
						IAPSTA_INFO(dev->name, "skip delete STA %pM\n", mac_addr);
						ret = -1;
						break;
					}
					wait = TRUE;
				}
				if (wait) {
					IAPSTA_INFO(dev->name, "status=%d, ap_recon_sta=%d, waiting %dms ...\n",
						status, cur_if->ap_recon_sta, max_wait_time);
					mutex_unlock(&apsta_params->in4way_sync);
					timeout = wait_event_interruptible_timeout(cur_if->ap_recon_sta_event,
						cur_if->ap_recon_sta, msecs_to_jiffies(max_wait_time));
					mutex_lock(&apsta_params->in4way_sync);
					IAPSTA_INFO(dev->name, "status=%d, ap_recon_sta=%d, timeout=%d\n",
						status, cur_if->ap_recon_sta, timeout);
					if (timeout > 0) {
						IAPSTA_INFO(dev->name, "skip delete STA %pM\n", mac_addr);
						ret = -1;
						break;
					}
				} else {
					IAPSTA_INFO(dev->name, "status=%d, ap_recon_sta=%d => 0\n",
						status, cur_if->ap_recon_sta);
					cur_if->ap_recon_sta = FALSE;
					if (cur_if->ifmode == IGO_MODE)
						wl_ext_update_conn_state(dhd, cur_if->ifidx, CONN_STATE_IDLE);
				}
			}
			break;
		case WL_EXT_STATUS_STA_DISCONNECTED:
			if (action & AP_WAIT_STA_RECONNECT) {
				IAPSTA_INFO(dev->name, "latest disc STA %pM ap_recon_sta=%d\n",
					ap_disc_sta_bssid, cur_if->ap_recon_sta);
				osl_do_gettimeofday(ap_disc_sta_ts);
				memcpy(ap_disc_sta_bssid, mac_addr, ETHER_ADDR_LEN);
				cur_if->ap_recon_sta = FALSE;
			}
			break;
		case WL_EXT_STATUS_STA_CONNECTED:
			if (action & AP_WAIT_STA_RECONNECT) {
				osl_do_gettimeofday(&cur_ts);
				diff_ms = osl_do_gettimediff(&cur_ts, ap_disc_sta_ts)/1000;
				if (diff_ms < max_wait_time &&
						!memcmp(ap_disc_sta_bssid, mac_addr, ETHER_ADDR_LEN)) {
					IAPSTA_INFO(dev->name, "status=%d, ap_recon_sta=%d => 1\n",
						status, cur_if->ap_recon_sta);
					cur_if->ap_recon_sta = TRUE;
					wake_up_interruptible(&cur_if->ap_recon_sta_event);
				} else {
					cur_if->ap_recon_sta = FALSE;
				}
			}
			break;
		default:
			IAPSTA_INFO(dev->name, "Unknown action=0x%x, status=%d\n", action, status);
	}

	return ret;
}

int
wl_ext_in4way_sync(struct net_device *dev, uint action,
	enum wl_ext_status status, void *context)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(dev);
	struct wl_if_info *cur_if = NULL;
	int ret = 0;

	mutex_lock(&apsta_params->in4way_sync);
	cur_if = wl_get_cur_if(dev);
	if (cur_if) {
		if (wl_ext_slave_if(cur_if))
			ret = wl_ext_in4way_sync_sta(cur_if, action, status, context);
		else if (cur_if->ifmode == IAP_MODE || cur_if->ifmode == IGO_MODE)
			ret = wl_ext_in4way_sync_ap(cur_if, action, status, context);
		else
			IAPSTA_INFO(dev->name, "Unknown mode %d\n", cur_if->ifmode);
	}
	mutex_unlock(&apsta_params->in4way_sync);

	return ret;
}

void
wl_ext_update_extsae_4way(struct net_device *dev,
	const struct ieee80211_mgmt *mgmt, bool tx)
{
	dhd_pub_t *dhd = dhd_get_pub(dev);
	struct wl_if_info *cur_if = NULL;
	uint32 auth_alg, auth_seq, status_code;
	uint conn_state = 0;
	char sae_type[32] = "";

	cur_if = wl_get_cur_if(dev);
	if (!cur_if)
		return;

	auth_alg = mgmt->u.auth.auth_alg;
	auth_seq = mgmt->u.auth.auth_transaction;
	status_code = mgmt->u.auth.status_code;
	if (auth_alg == WLAN_AUTH_SAE) {
		if (wl_ext_slave_if(cur_if)) {
			if (auth_seq == 1) {
				if (tx)
					conn_state = CONN_STATE_AUTH_SAE_M1;
				else
					conn_state = CONN_STATE_AUTH_SAE_M2;
			} else if (auth_seq == 2) {
				if (tx)
					conn_state = CONN_STATE_AUTH_SAE_M3;
				else
					conn_state = CONN_STATE_AUTH_SAE_M4;
			}
		} else if (cur_if->ifmode == IAP_MODE || cur_if->ifmode == IGO_MODE) {
			if (auth_seq == 1) {
				if (tx)
					conn_state = CONN_STATE_AUTH_SAE_M2;
				else
					conn_state = CONN_STATE_AUTH_SAE_M1;
			} else if (auth_seq == 2) {
				if (tx)
					conn_state = CONN_STATE_AUTH_SAE_M4;
				else
					conn_state = CONN_STATE_AUTH_SAE_M3;
			}
		}
		if (status_code == 76) {
			snprintf(sae_type, sizeof(sae_type), "%d(Anti-clogging)", status_code);
		} else if (status_code == 126) {
			snprintf(sae_type, sizeof(sae_type), "%d(R3-H2E)", status_code);
		} else {
			snprintf(sae_type, sizeof(sae_type), "%d", status_code);
		}
	}
	if (conn_state) {
		wl_ext_update_conn_state(dhd, cur_if->ifidx, conn_state);
		if (dump_msg_level & DUMP_EAPOL_VAL) {
			if (tx) {
				printf("[%s] WPA3_AUTH SAE M%d [TX] : (%pM) -> (%pM), status=%s\n",
					dev->name, conn_state-CONN_STATE_AUTH_SAE_M1+1,
					mgmt->sa, mgmt->da, sae_type);
			} else {
				printf("[%s] WPA3_AUTH SAE M%d [RX] : (%pM) <- (%pM), status=%s\n",
					dev->name, conn_state-CONN_STATE_AUTH_SAE_M1+1,
					mgmt->da, mgmt->sa, sae_type);
			}
		}
	} else {
		if (tx) {
			IAPSTA_ERROR(dev->name,
				"AUTH [TX] : (%pM) -> (%pM), alg/seq/status = %d/%d/%d\n",
				mgmt->sa, mgmt->da, auth_alg, auth_seq, status_code);
		} else {
			IAPSTA_ERROR(dev->name,
				"AUTH [RX] : (%pM) <- (%pM), alg/seq/status = %d/%d/%d\n",
				mgmt->da, mgmt->sa, auth_alg, auth_seq, status_code);
		}
	}

	return;
}
#endif /* WL_CFG80211 */

#ifdef WL_WIRELESS_EXT
int
wl_ext_in4way_sync_wext(struct net_device *dev, uint action,
	enum wl_ext_status status, void *context)
{
	int ret = 0;
#ifndef WL_CFG80211
	dhd_pub_t *dhd = dhd_get_pub(dev);
	struct wl_apsta_params *apsta_params;
	struct wl_if_info *cur_if = NULL;

	if (!dhd)
		return 0;

	apsta_params = dhd->iapsta_params;

	mutex_lock(&apsta_params->in4way_sync);
	cur_if = wl_get_cur_if(dev);
	if (cur_if && cur_if->ifmode == ISTA_MODE) {
		if (status == WL_EXT_STATUS_DISCONNECTING) {
			wl_ext_add_remove_pm_enable_work(dev, FALSE);
		} else if (status == WL_EXT_STATUS_CONNECTING) {
			wl_ext_add_remove_pm_enable_work(dev, TRUE);
		}
		ret = wl_ext_in4way_sync_sta(cur_if, 0, status, NULL);
	}
	mutex_unlock(&apsta_params->in4way_sync);
#endif
	return ret;
}
#endif /* WL_WIRELESS_EXT */

#ifdef TPUT_MONITOR
static void
wl_tput_monitor_timer(unsigned long data)
{
	struct net_device *dev = (struct net_device *)data;
	struct dhd_pub *dhd;
	wl_event_msg_t msg;

	if (!dev) {
		IAPSTA_ERROR("wlan", "dev is not ready\n");
		return;
	}

	dhd = dhd_get_pub(dev);

	bzero(&msg, sizeof(wl_event_msg_t));
	IAPSTA_TRACE(dev->name, "timer expired\n");

	msg.ifidx = 0;
	msg.event_type = hton32(WLC_E_RESERVED);
	msg.reason = hton32(ISAM_RC_TPUT_MONITOR);
	wl_ext_event_send(dhd->event_params, &msg, NULL);
}

static int
wl_ext_assoclist_num(struct net_device *dev)
{
	int ret = 0, maxassoc = 0;
	char mac_buf[MAX_NUM_OF_ASSOCLIST *
		sizeof(struct ether_addr) + sizeof(uint)] = {0};
	struct maclist *assoc_maclist = (struct maclist *)mac_buf;

	assoc_maclist->count = htod32(MAX_NUM_OF_ASSOCLIST);
	ret = wl_ext_ioctl(dev, WLC_GET_ASSOCLIST, assoc_maclist, sizeof(mac_buf), 0);
	if (ret)
		return ret;
	maxassoc = dtoh32(assoc_maclist->count);

	return maxassoc;
}

static int
dev_wlc_ioctl(struct net_device *dev, int cmd, void *arg, int len)
{
	struct dhd_pub *dhd = dhd_get_pub(dev);
	dhd_ioctl_t ioc;
	int8 index;
	int ret;

	memset(&ioc, 0, sizeof(ioc));
	ioc.cmd = cmd;
	ioc.buf = arg;
	ioc.len = len;

	index = dhd_net2idx(dhd->info, dev);
	if (index == DHD_BAD_IF) {
		IAPSTA_ERROR(dev->name, "Bad ifidx from dev\n");
		return -ENODEV;
	}
	ret = dhd_ioctl_process(dhd, index, &ioc, arg);

	return ret;
}

static int
wl_string_fill(char *buf, int offset, char *ioctl_buf, int remain,
	char *str_start, char *str_end)
{
	char *pch_start = NULL, *pch_end = NULL, *pch_offset;
	int len = 0;

	pch_start = strstr(ioctl_buf, str_start);
	if (pch_start)
		pch_end = strstr(pch_start, str_end);
	if (pch_end) {
		len = pch_end - pch_start;
		pch_offset = pch_end;
		pch_end = strchr(pch_offset, '\n');
		if (pch_end) {
			len += (pch_end - pch_offset + 1);
			pch_offset = pch_end + 1;
			if (*pch_offset == ' ') {
				pch_end = strchr(pch_offset, '\n');
				if (pch_end)
					len += (pch_end - pch_offset + 1);
			}
			if (remain > len)
				memcpy(buf+offset, pch_start, len);
		} else {
			len = 0;
		}
	}

	return len;
}

static void
wl_ampdu_dump(struct net_device *dev)
{
	char *ioctl_buf = g_ioctl_buf, *buf = NULL;
	int ret = 0, len = 0, offset = 0, remain;

	if (!(android_msg_level & ANDROID_AMPDU_LEVEL))
		return;

	memset(ioctl_buf, 0, WL_DUMP_BUF_LEN);
	ret = bcm_mkiovar("dump", "ampdu", strlen("ampdu"), ioctl_buf, WL_DUMP_BUF_LEN);
	if (ret == 0) {
		goto exit;
	}
	ret = dev_wlc_ioctl(dev, WLC_GET_VAR, (void *)ioctl_buf, WL_DUMP_BUF_LEN);
	if (ret) {
		goto exit;
	}

	buf = kmalloc(WLC_IOCTL_MEDLEN, GFP_KERNEL);
	if (buf == NULL) {
		IAPSTA_ERROR(dev->name, "Failed to allocate buffer of %d bytes\n", WLC_IOCTL_MEDLEN);
		goto exit;
	}
	memset(buf, 0, WLC_IOCTL_MEDLEN);
	ret = bcm_mkiovar("dump_clear", "ampdu", strlen("ampdu"), buf, WLC_IOCTL_MEDLEN);
	if (ret == 0) {
		goto exit;
	}
	ret = dev_wlc_ioctl(dev, WLC_SET_VAR, (void *)buf, WLC_IOCTL_MEDLEN);
	if (ret) {
		goto exit;
	}

	remain = WLC_IOCTL_MEDLEN;
	memset(buf, 0, WLC_IOCTL_MEDLEN);

	len = wl_string_fill(buf, offset, ioctl_buf, remain, "TX MCS", "HE PER");
	remain -= len;
	offset += len;

	len = wl_string_fill(buf, offset, ioctl_buf, remain, "TX EHT", "EHT PER");
	remain -= len;
	offset += len;

	len = wl_string_fill(buf, offset, ioctl_buf, remain, "RX MCS", "RX HE");
	remain -= len;
	offset += len;

	len = wl_string_fill(buf, offset, ioctl_buf, remain, "RX EHT", "RX EHT");
	remain -= len;
	offset += len;

	WL_MSG(dev->name,"\n%s\n", buf);

exit:
	if (buf)
		kfree(buf);
}

static void
wl_btc_dump(struct net_device *dev)
{
	struct dhd_pub *dhd = dhd_get_pub(dev);
	char btc_str[64];
	int ret, val, byte_w = 0;

	if (!(android_msg_level & ANDROID_BTC_LEVEL))
		return;

	memset(btc_str, 0, sizeof(btc_str));
	byte_w += snprintf(btc_str+byte_w, sizeof(btc_str)-byte_w, "btc_params");
	ret = dhd_conf_reg2args(dhd, "btc_params", FALSE, 15, &val);
	if (!ret)
		byte_w += snprintf(btc_str+byte_w, sizeof(btc_str)-byte_w, "[15:%d]", val);
	ret = dhd_conf_reg2args(dhd, "btc_params", FALSE, 39, &val);
	if (!ret)
		byte_w += snprintf(btc_str+byte_w, sizeof(btc_str)-byte_w, "[39:%d]", val);
	WL_MSG(dev->name,"%s\n", btc_str);
}

static void
wl_tvpm_dump(struct net_device *dev)
{
	wl_tvpm_req_t* tvpm_req = NULL;
	size_t reqlen = sizeof(wl_tvpm_req_t) + sizeof(wl_tvpm_status_t);
	uint8 *outbuf = g_ioctl_buf;
	size_t outlen = WLC_IOCTL_MEDLEN;
	wl_tvpm_status_t* status;
	int ret, phy_temp = 0;
	bool tvpm = FALSE, sense = FALSE;

	if (!(android_msg_level & ANDROID_TVPM_LEVEL))
		return;

	tvpm_req = kmalloc(reqlen, GFP_KERNEL);
	if (tvpm_req == NULL) {
		IAPSTA_ERROR(dev->name, "MALLOC failed\n");
		goto exit;
	}
	memset(tvpm_req, 0, reqlen);

	tvpm_req->version = TVPM_REQ_VERSION_1;
	tvpm_req->length = reqlen;
	tvpm_req->req_type = WL_TVPM_REQ_STATUS;
	ret = wldev_iovar_getbuf(dev, "tvpm", tvpm_req, reqlen, outbuf, outlen, NULL);
	status = (wl_tvpm_status_t*)outbuf;
	if (!ret && status->enable) {
		tvpm = TRUE;
	} else {
		ret = wldev_iovar_getbuf(dev, "phy_tempsense", &phy_temp, sizeof(phy_temp),
			outbuf, outlen, NULL);
		if (!ret) {
			phy_temp = dtoh32(*(int*)outbuf);
			sense = TRUE;
		}
	}

	if (tvpm) {
		WL_MSG(dev->name,"temp=%3d, duty=%3d, pwrbko=%d, chains=%d\n",
			status->temp, status->tx_dutycycle, status->tx_power_backoff,
			status->num_active_chains);
	}
	else if (sense) {
		WL_MSG(dev->name,"phy_temp=%3d\n", phy_temp);
	}

exit:
	if (tvpm_req)
		kfree(tvpm_req);
}

static void
wl_tput_dump(struct net_device *dev, wl_tput_info_t *tput_info)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(dev);

	if (!(android_msg_level & ANDROID_TPUT_LEVEL))
		return;

	WL_MSG(dev->name,
		"tx=%3d.%d%d%d Mbps, rx=%3d.%d%d%d Mbps, tput_sum=%3d.%d%d%d Mbps\n",
		tput_info->tput_tx, (tput_info->tput_tx_kb/100)%10,
		(tput_info->tput_tx_kb/10)%10, (tput_info->tput_tx_kb)%10,
		tput_info->tput_rx, (tput_info->tput_rx_kb/100)%10,
		(tput_info->tput_rx_kb/10)%10, (tput_info->tput_rx_kb)%10,
		apsta_params->tput_sum, (apsta_params->tput_sum_kb/100)%10,
		(apsta_params->tput_sum_kb/10)%10, (apsta_params->tput_sum_kb)%10);
}

static void
wl_rate_dump(struct net_device *dev, char *mac)
{
	void *buf = g_ioctl_buf;
	sta_info_v4_t *sta = NULL;
	char chan_str[64], rssi_str[16], tx_rate_str[16], rx_rate_str[16];
	s32 rate = 0;
	int i, bytes_w = 0, ret = BCME_ERROR;
	bool alive = TRUE;

	memset(rssi_str, 0, sizeof(rssi_str));
	if (alive) {
		ret = wldev_iovar_getbuf(dev, "sta_info", mac, ETHER_ADDR_LEN,
			buf, WLC_IOCTL_MEDLEN, NULL);
		if (ret == BCME_OK) {
			sta = (sta_info_v4_t *)buf;
			if (!IS_STA_INFO_VER(sta))
				sta = NULL;
		}
		if (sta) {
			memset(tx_rate_str, 0, sizeof(tx_rate_str));
			if (dtoh32(sta->tx_rate) == -1)
				snprintf(tx_rate_str, sizeof(tx_rate_str), "-1");
			else
				snprintf(tx_rate_str, sizeof(tx_rate_str), "%4d.%d",
					dtoh32(sta->tx_rate)/1000, ((dtoh32(sta->tx_rate)/100)%10));
			memset(rx_rate_str, 0, sizeof(rx_rate_str));
			if (dtoh32(sta->rx_rate) == -1)
				snprintf(rx_rate_str, sizeof(rx_rate_str), "-1");
			else
				snprintf(rx_rate_str, sizeof(rx_rate_str), "%4d.%d",
					dtoh32(sta->rx_rate)/1000, ((dtoh32(sta->rx_rate)/100)%10));
			for (i = 0; i < WL_RSSI_ANT_MAX; i++) {
				if (dtoh32(sta->rssi[i])) {
					bytes_w += snprintf(rssi_str+bytes_w, sizeof(rssi_str)-bytes_w,
						"[%2d]", dtoh32(sta->rssi[i]));
				}
			}
		} else {
			wl_phy_rssi_ant(dev, mac, rssi_str, sizeof(rssi_str));
		}
	}

	{
		wl_ext_get_chan_str(dev, chan_str, sizeof(chan_str));
		if (sta) {
			WL_MSG(dev->name, "mac=%pM, chan=%s, rssi=%s, tx_rate:%s, rx_rate:%s\n",
				mac, chan_str, rssi_str, tx_rate_str, rx_rate_str);
		} else {
			ret = wl_ext_ioctl(dev, WLC_GET_RATE, &rate, sizeof(rate), FALSE);
			if (!ret) {
				rate = dtoh32(rate);
				WL_MSG(dev->name, "mac=%pM, chan=%s, rssi=%s, tx_rate:%4d%2s\n",
					mac, chan_str, rssi_str, rate/2, (rate & 1) ? ".5" : "");
			}
		}
	}
}

static void
wl_swdiv_dump(struct wl_if_info *cur_if)
{
	void *buf = g_ioctl_buf;
	struct wlc_swdiv_stats_v1 *cnt_v1;
	struct wlc_swdiv_stats_v2 *cnt;
	struct ether_addr bssid;
	char rssi_buf[16];
	int ret = 0, swdiv_en = 0;

	if (!(android_msg_level & ANDROID_SWDIV_LEVEL))
		return;

	if (cur_if->ifmode != ISTA_MODE)
		return;

	ret = wl_ext_iovar_getint(cur_if->dev, "swdiv_en", &swdiv_en);
	if (ret != 0 || swdiv_en == 0)
		return;

	if (!wl_ext_associated(cur_if->dev, &bssid))
		return;

	ret = wldev_iovar_getbuf(cur_if->dev, "swdiv_stats", NULL, 0,
		buf, WLC_IOCTL_MEDLEN, NULL);
	if (ret < 0)
		return;

	memset(rssi_buf, 0, sizeof(rssi_buf));
	wl_phy_rssi_ant(cur_if->dev, (char *)&bssid, rssi_buf, sizeof(rssi_buf));

	if (wlc_ver_major(cur_if->dev) <= 5) {
		cnt_v1 = (struct wlc_swdiv_stats_v1 *)buf;
		WL_MSG(cur_if->dev->name,
			"active_ant/rssi/snr_ant=%d/%s/[%2d][%2d]\n",
			cnt_v1->active_ant, rssi_buf,
			cnt_v1->avg_snr_per_ant0, cnt_v1->avg_snr_per_ant1);

	} else {
		cnt = (struct wlc_swdiv_stats_v2 *)buf;
		WL_MSG(cur_if->dev->name,
			"active_ant/rssi/snr_ant=%d/%s/[%2d][%2d]\n",
			cnt->active_ant, rssi_buf,
			cnt->avg_snr_per_ant0, cnt->avg_snr_per_ant1);
	}
}

#ifdef WLDWDS
static void
wl_dwds_tput_dump(struct wl_if_info *cur_if)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);
	struct wl_dwds_info *dwds_if;
	int i;

	if (!(android_msg_level & ANDROID_TPUT_LEVEL))
		return;

	if (cur_if->ifmode == IAP_MODE) {
		for (i=0; i<MAX_DWDS_IF_NUM; i++) {
			dwds_if = &apsta_params->dwds_info[i];
			if (dwds_if->dev && cur_if->bssidx == dwds_if->bssidx) {
				wl_tput_dump(dwds_if->dev, &dwds_if->tput_info);
			}
		}
	}
}
#endif /* WLDWDS */

static void
wl_sta_info_dump(struct wl_if_info *cur_if)
{
	struct ether_addr bssid;
	int ret = 0;
	int i;

	if (!(android_msg_level & ANDROID_TPUT_LEVEL))
		return;

	if (wl_ext_slave_if(cur_if)) {
		if (wl_ext_associated(cur_if->dev, &bssid)) {
			wl_rate_dump(cur_if->dev, (char *)&bssid);
		}
	}
	else if (cur_if->ifmode == IAP_MODE || cur_if->ifmode == IGO_MODE) {
		int maxassoc = 0;
		char mac_buf[MAX_NUM_OF_ASSOCLIST *
			sizeof(struct ether_addr) + sizeof(uint)] = {0};
		struct maclist *assoc_maclist = (struct maclist *)mac_buf;

		assoc_maclist->count = htod32(MAX_NUM_OF_ASSOCLIST);
		ret = wl_ext_ioctl(cur_if->dev, WLC_GET_ASSOCLIST, assoc_maclist, sizeof(mac_buf), 0);
		if (ret)
			goto exit;
		maxassoc = dtoh32(assoc_maclist->count);
		for (i=0; i<maxassoc; i++) {
			wl_rate_dump(cur_if->dev, (char *)&assoc_maclist->ea[i]);
		}
	}

exit:
	return;
}

static void
wl_tput_monitor(struct net_device *dev, int ifidx, wl_tput_info_t *tput_info)
{
	struct dhd_pub *dhd = dhd_get_pub(dev);
	dhd_if_t *ifp = NULL;

	ifp = dhd_get_ifp(dhd, ifidx);
	if (!ifp)
		return;

	if (tput_info->tput_ts.tv_sec == 0 && tput_info->tput_ts.tv_nsec == 0) {
		osl_do_gettimeofday(&tput_info->tput_ts);
		tput_info->last_tx = ifp->stats.tx_bytes;
		tput_info->last_rx = ifp->stats.rx_bytes;
	} else {
		struct osl_timespec cur_ts;
		uint32 diff_ms;

		osl_do_gettimeofday(&cur_ts);
		diff_ms = osl_do_gettimediff(&cur_ts, &tput_info->tput_ts)/1000;
		memcpy(&tput_info->tput_ts, &cur_ts, sizeof(struct osl_timespec));
		tput_info->tput_tx = (int32)(((ifp->stats.tx_bytes-tput_info->last_tx)/1024/1024)*8)*1000/diff_ms;
		if (tput_info->tput_tx == 0) {
			tput_info->tput_tx = (int32)((ifp->stats.tx_bytes-tput_info->last_tx)*8*1000/1024/1024)/diff_ms;
			tput_info->tput_tx_kb = (int32)((ifp->stats.tx_bytes-tput_info->last_tx)*8*1000/1024)/diff_ms;
			tput_info->tput_tx_kb = tput_info->tput_tx_kb % 1000;
		} else
			tput_info->tput_tx_kb = 0;
		tput_info->tput_rx = (int32)(((ifp->stats.rx_bytes-tput_info->last_rx)/1024/1024)*8)*1000/diff_ms;
		if (tput_info->tput_rx == 0) {
			tput_info->tput_rx = (int32)((ifp->stats.rx_bytes-tput_info->last_rx)*8*1000/1024/1024)/diff_ms;
			tput_info->tput_rx_kb = (int32)((ifp->stats.rx_bytes-tput_info->last_rx)*8*1000/1024)/diff_ms;
			tput_info->tput_rx_kb = tput_info->tput_rx_kb % 1000;
		} else
			tput_info->tput_rx_kb = 0;
		tput_info->last_tx = ifp->stats.tx_bytes;
		tput_info->last_rx = ifp->stats.rx_bytes;
	}
}

static void
wl_tput_monitor_handler(struct wl_if_info *cur_if,
	const wl_event_msg_t *e, void *data)
{
	struct dhd_pub *dhd = dhd_get_pub(cur_if->dev);
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);
	wl_tput_info_t *tput_info;
	struct wl_if_info *tmp_if;
#ifdef WLDWDS
	struct wl_dwds_info *dwds_if;
#endif /* WLDWDS */
	uint32 etype = ntoh32(e->event_type);
	uint32 status =  ntoh32(e->status);
	uint32 reason = ntoh32(e->reason);
	uint16 flags =  ntoh16(e->flags);
	uint timeout = dhd->conf->tput_monitor_ms;
	int32 tput_sum = 0, tput_sum_kb = 0;
	bool monitor_if[MAX_IF_NUM] = {FALSE}, monitor = FALSE;
	int i;

	if (etype == WLC_E_RESERVED && reason == ISAM_RC_TPUT_MONITOR) {
		tput_sum = 0;
		for (i=0; i<MAX_IF_NUM; i++) {
			tmp_if = &apsta_params->if_info[i];
			if (tmp_if->dev && wl_ext_slave_if(tmp_if) &&
					wl_get_isam_status(tmp_if, STA_CONNECTED)) {
				wl_tput_monitor(tmp_if->dev, tmp_if->ifidx, &tmp_if->tput_info);
				monitor_if[i] = TRUE;
			}
			else if (tmp_if->dev &&
					(tmp_if->ifmode == IAP_MODE || tmp_if->ifmode == IGO_MODE) &&
					wl_ext_assoclist_num(tmp_if->dev)) {
				wl_tput_monitor(tmp_if->dev, tmp_if->ifidx, &tmp_if->tput_info);
				monitor_if[i] = TRUE;
			}
			if (monitor_if[i] == TRUE) {
				tput_info = &tmp_if->tput_info;
				tput_sum += (tput_info->tput_tx + tput_info->tput_rx);
				tput_sum_kb += (tput_info->tput_tx_kb + tput_info->tput_rx_kb);
			}
		}
#ifdef WLDWDS
		for (i=0; i<MAX_DWDS_IF_NUM; i++) {
			dwds_if = &apsta_params->dwds_info[i];
			if (dwds_if->dev) {
				wl_tput_monitor(dwds_if->dev, dwds_if->ifidx, &dwds_if->tput_info);
				tput_info = &dwds_if->tput_info;
				tput_sum += (tput_info->tput_tx + tput_info->tput_rx);
				tput_sum_kb += (tput_info->tput_tx_kb + tput_info->tput_rx_kb);
			}
		}
#endif /* WLDWDS */
		apsta_params->tput_sum = tput_sum + (tput_sum_kb/1000);
		apsta_params->tput_sum_kb = tput_sum_kb % 1000;
		for (i=0; i<MAX_IF_NUM; i++) {
			if (monitor_if[i]) {
				tmp_if = &apsta_params->if_info[i];
				wl_tput_dump(tmp_if->dev, &tmp_if->tput_info);
#ifdef WLDWDS
				wl_dwds_tput_dump(tmp_if);
#endif /* WLDWDS */
				monitor = TRUE;
			}
		}
		for (i=0; i<MAX_IF_NUM; i++) {
			if (monitor_if[i]) {
				tmp_if = &apsta_params->if_info[i];
				wl_sta_info_dump(tmp_if);
				wl_swdiv_dump(tmp_if);
			}
		}
		if (monitor) {
			wl_btc_dump(cur_if->dev);
			wl_tvpm_dump(cur_if->dev);
			wl_ampdu_dump(cur_if->dev);
			wl_timer_mod(dhd, &apsta_params->monitor_timer, timeout);
		}
#ifdef BCMSDIO
		if (apsta_params->tput_sum >= dhd->conf->doflow_tput_thresh && dhd_doflow) {
			dhd_doflow = FALSE;
			dhd_txflowcontrol(dhd, ALL_INTERFACES, OFF);
			IAPSTA_INFO("wlan", "dhd_doflow=%d\n", dhd_doflow);
		}
		else if (apsta_params->tput_sum < dhd->conf->doflow_tput_thresh && !dhd_doflow) {
			dhd_doflow = TRUE;
			IAPSTA_INFO("wlan", "dhd_doflow=%d\n", dhd_doflow);
		}
#endif
	}
	else if (wl_ext_slave_if(cur_if)) {
		if (etype == WLC_E_LINK) {
			if (flags & WLC_EVENT_MSG_LINK) {
				wl_timer_mod(dhd, &apsta_params->monitor_timer, timeout);
			} else if (!wl_ext_iapsta_other_if_associated(cur_if->dev)) {
				wl_timer_mod(dhd, &apsta_params->monitor_timer, 0);
			}
		}
	}
	else if (cur_if->ifmode == IAP_MODE || cur_if->ifmode == IGO_MODE) {
		if ((etype == WLC_E_SET_SSID && status == WLC_E_STATUS_SUCCESS) ||
				(etype == WLC_E_LINK && status == WLC_E_STATUS_SUCCESS &&
				reason == WLC_E_REASON_INITIAL_ASSOC)) {
			wl_timer_mod(dhd, &apsta_params->monitor_timer, timeout);
		} else if ((etype == WLC_E_LINK && reason == WLC_E_LINK_BSSCFG_DIS) ||
				(etype == WLC_E_LINK && status == WLC_E_STATUS_SUCCESS &&
				reason == WLC_E_REASON_DEAUTH)) {
			if (!wl_ext_iapsta_other_if_associated(cur_if->dev)) {
				wl_timer_mod(dhd, &apsta_params->monitor_timer, 0);
			}
		} else if ((etype == WLC_E_ASSOC_IND || etype == WLC_E_REASSOC_IND) &&
				reason == DOT11_SC_SUCCESS) {
			wl_timer_mod(dhd, &apsta_params->monitor_timer, timeout);
		}
	}
}
#endif /* TPUT_MONITOR */

#ifdef ACS_MONITOR
static bool
wl_ext_max_prio_if(struct wl_if_info *cur_if)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);
	struct wl_if_info *tmp_if;
	wl_prio_t max_prio;
	int i;

	if (apsta_params->mcc) {
		goto exit;
	}

	// find the max prio
	max_prio = cur_if->prio;
	for (i=0; i<MAX_IF_NUM; i++) {
		tmp_if = &apsta_params->if_info[i];
		if (cur_if != tmp_if && wl_get_isam_status(tmp_if, IF_READY) &&
				tmp_if->prio > max_prio && wl_ext_associated(tmp_if->dev, NULL)) {
			return TRUE;
		}
	}
exit:
	return FALSE;
}

static void
wl_ext_acs_scan(struct wl_if_info *cur_if)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);

	if ((apsta_params->acs & ACS_DRV_BIT) && wl_ext_associated(cur_if->dev, NULL)) {
		int ret, cur_scan_time;
		cur_if->escan->autochannel = 1;
		cur_scan_time = wl_ext_set_scan_time(cur_if->dev, 80,
			WLC_GET_SCAN_CHANNEL_TIME, WLC_SET_SCAN_CHANNEL_TIME);
		WL_MSG(cur_if->dev->name, "ACS_SCAN\n");
		wl_escan_drv_acs_scan(cur_if->dev, WLC_BAND_2G|WLC_BAND_5G, NULL);
		if (cur_scan_time) {
			ret = wl_ext_ioctl(cur_if->dev, WLC_SET_SCAN_CHANNEL_TIME,
				&cur_scan_time, sizeof(cur_scan_time), 1);
		}
	}
}

static void
wl_ext_acs(struct wl_if_info *cur_if)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);
	struct wl_ext_chan_info chan_info;
	struct wl_escan_info *escan = cur_if->escan;

	if (apsta_params->acs & ACS_DRV_BIT) {
		mutex_lock(&apsta_params->usr_sync);
		if (wl_ext_get_chanspec(cur_if->dev, &chan_info)) {
			if (chan_info.band == WLC_BAND_5G && escan->best_5g_ch)
				cur_if->chan_info.chan = escan->best_5g_ch;
			else if (chan_info.band == WLC_BAND_6G && escan->best_6g_ch)
				cur_if->chan_info.chan = escan->best_6g_ch;
			else if (escan->best_2g_ch)
				wl_ext_set_chan_info(cur_if, WLC_BAND_2G, escan->best_2g_ch);
			else {
				cur_if->chan_info.chan = 0;
				WL_MSG(cur_if->ifname, "no valid channel on cur_band=%d\n",
					cur_if->chan_info.band);
			}
			if (cur_if->chan_info.chan)
				wl_ext_reset_master(cur_if);
		}
		mutex_unlock(&apsta_params->usr_sync);
	}
}

static void
wl_acs_timer(unsigned long data)
{
	struct net_device *dev = (struct net_device *)data;
	struct dhd_pub *dhd;
	wl_event_msg_t msg;

	if (!dev) {
		IAPSTA_ERROR("wlan", "dev is not ready\n");
		return;
	}

	dhd = dhd_get_pub(dev);

	bzero(&msg, sizeof(wl_event_msg_t));
	IAPSTA_TRACE(dev->name, "timer expired\n");

	msg.ifidx = dhd_net2idx(dhd->info, dev);
	msg.event_type = hton32(WLC_E_RESERVED);
	msg.reason = hton32(ISAM_RC_AP_ACS);
	wl_ext_event_send(dhd->event_params, &msg, NULL);
}

static void
wl_acs_handler(struct wl_if_info *cur_if, const wl_event_msg_t *e, void *data)
{
	struct dhd_pub *dhd = dhd_get_pub(cur_if->dev);
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);
	uint acs_tmo = apsta_params->acs_tmo;
	uint32 etype = ntoh32(e->event_type);
	uint32 status = ntoh32(e->status);
	uint32 reason = ntoh32(e->reason);

	if (wl_get_isam_status(cur_if, AP_CREATED)) {
		if ((etype == WLC_E_SET_SSID && status == WLC_E_STATUS_SUCCESS) ||
				(etype == WLC_E_LINK && status == WLC_E_STATUS_SUCCESS &&
				reason == WLC_E_REASON_INITIAL_ASSOC)) {
			// Link up
			wl_timer_mod(dhd, &cur_if->acs_timer, acs_tmo*1000);
		}
		else if ((etype == WLC_E_LINK && reason == WLC_E_LINK_BSSCFG_DIS) ||
				(etype == WLC_E_LINK && status == WLC_E_STATUS_SUCCESS &&
				reason == WLC_E_REASON_DEAUTH)) {
			// Link down
			wl_timer_mod(dhd, &cur_if->acs_timer, 0);
			cur_if->escan->autochannel = 0;
		}
		else if ((etype == WLC_E_ASSOC_IND || etype == WLC_E_REASSOC_IND) &&
				reason == DOT11_SC_SUCCESS) {
			// external STA connected
			wl_timer_mod(dhd, &cur_if->acs_timer, 0);
		}
		else if (etype == WLC_E_DISASSOC_IND ||
				etype == WLC_E_DEAUTH_IND ||
				(etype == WLC_E_DEAUTH && reason != DOT11_RC_RESERVED)) {
			// external STA disconnected
			wl_timer_mod(dhd, &cur_if->acs_timer, acs_tmo*1000);
		}
		else if (etype == WLC_E_RESERVED && reason == ISAM_RC_AP_ACS) {
			// acs_tmo expired
			if (!wl_ext_assoclist_num(cur_if->dev) &&
					!wl_ext_max_prio_if(cur_if)) {
				wl_ext_acs_scan(cur_if);
				wl_timer_mod(dhd, &cur_if->acs_timer, acs_tmo*1000);
			} else {
				wl_timer_mod(dhd, &cur_if->acs_timer, 0);
			}
		}
		else if (((etype == WLC_E_ESCAN_RESULT && status == WLC_E_STATUS_SUCCESS) ||
				(etype == WLC_E_ESCAN_RESULT &&
				(status == WLC_E_STATUS_ABORT || status == WLC_E_STATUS_NEWSCAN ||
				status == WLC_E_STATUS_11HQUIET || status == WLC_E_STATUS_CS_ABORT ||
				status == WLC_E_STATUS_NEWASSOC || status == WLC_E_STATUS_TIMEOUT)))) {
			// scan complete
			cur_if->escan->autochannel = 0;
			if (!wl_ext_assoclist_num(cur_if->dev) &&
					!wl_ext_max_prio_if(cur_if)) {
				wl_ext_acs(cur_if);
			} else {
				wl_timer_mod(dhd, &cur_if->acs_timer, 0);
			}
		}
	}
}

static void
wl_acs_detach(struct wl_if_info *cur_if)
{
	IAPSTA_TRACE(cur_if->dev->name, "Enter\n");
	wl_timer_deregister(cur_if->dev, &cur_if->acs_timer);
	if (cur_if->escan) {
		cur_if->escan = NULL;
	}
}

static void
wl_acs_attach(dhd_pub_t *dhd, struct wl_if_info *cur_if)
{
	IAPSTA_TRACE(cur_if->dev->name, "Enter\n");
	cur_if->escan = dhd->escan;
	wl_timer_register(cur_if->dev, &cur_if->acs_timer, wl_acs_timer);
}
#endif /* ACS_MONITOR */

#ifdef RESTART_AP_WAR
static void
wl_ext_restart_ap_timeout(unsigned long data)
{
	struct net_device *dev = (struct net_device *)data;
	struct dhd_pub *dhd;
	wl_event_msg_t msg;

	if (!dev) {
		IAPSTA_ERROR("wlan", "dev is not ready\n");
		return;
	}

	dhd = dhd_get_pub(dev);

	bzero(&msg, sizeof(wl_event_msg_t));
	IAPSTA_TRACE(dev->name, "timer expired\n");

	msg.ifidx = dhd_net2idx(dhd->info, dev);
	msg.event_type = hton32(WLC_E_RESERVED);
	msg.reason = hton32(ISAM_RC_AP_RESTART);
	wl_ext_event_send(dhd->event_params, &msg, NULL);
}

static void
wl_ext_restart_ap_handler(struct wl_if_info *cur_if,
	const wl_event_msg_t *e, void *data)
{
	struct dhd_pub *dhd = dhd_get_pub(cur_if->dev);
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);
	struct wl_ext_chan_info chan_info;
	uint32 etype = ntoh32(e->event_type);
	uint32 reason = ntoh32(e->reason);

	if (etype == WLC_E_RESERVED && reason == ISAM_RC_AP_RESTART) {
		if (!wl_get_isam_status(cur_if, AP_CREATED)) {
			mutex_lock(&apsta_params->usr_sync);
			memset(&chan_info, 0, sizeof(struct wl_ext_chan_info));
			if (!wl_ext_get_chanspec(cur_if->dev, &chan_info) ||
					wl_ext_passive(cur_if->dev, &chan_info)) {
				WL_MSG(cur_if->ifname, "restart AP\n");
				wl_ext_if_down(cur_if);
				if (chan_info.chan)
					wl_ext_set_chan_info(cur_if, chan_info.band, chan_info.chan);
				wl_ext_move_cur_channel(cur_if);
				wl_ext_move_other_channel(cur_if);
				wl_ext_if_up(cur_if, TRUE, 0);
				wl_timer_mod(dhd, &cur_if->restart_ap_timer, AP_RESTART_TIMEOUT);
			} else {
				WL_MSG(cur_if->ifname, "skip restart AP\n");
			}
			mutex_unlock(&apsta_params->usr_sync);
		}
	}
	return;
}
#endif /* RESTART_AP_WAR */

#if defined(RESET_AP_WAR) || defined(RXF0OVFL_REINIT_WAR)
static int
wl_ext_counters_cbfn(void *ctx, const uint8 *data, uint16 type, uint16 len)
{
	struct wl_if_info *cur_if = ctx;
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);
	int res = BCME_OK;

	switch (type) {
		case WL_CNT_XTLV_CNTV_LE10_UCODE: {
			wl_cnt_v_le10_mcst_t *cnt = (wl_cnt_v_le10_mcst_t *)data;
			if (len != sizeof(wl_cnt_v_le10_mcst_t)) {
				printf("type %d: cnt struct length mismatch! %d != %d\n",
					type, len, (int)sizeof(wl_cnt_v_le10_mcst_t));
			}
#ifdef RESET_AP_WAR
			if (apsta_params->war_reason == ISAM_RC_AP_RESET)
				cur_if->txbcnfrm = dtoh32(cnt->txbcnfrm);
#endif /* RESET_AP_WAR */
#ifdef RXF0OVFL_REINIT_WAR
			if (apsta_params->war_reason == ISAM_RC_RXF0OVFL_REINIT) {
				apsta_params->rxbeaconmbss = dtoh32(cnt->rxbeaconmbss);
				apsta_params->rxf0ovfl = dtoh32(cnt->rxf0ovfl);
			}
#endif /* RXF0OVFL_REINIT_WAR */
			break;
		}
		case WL_CNT_XTLV_GE40_UCODE_V1:
		{
			wl_cnt_ge40mcst_v1_t *cnt = (wl_cnt_ge40mcst_v1_t *)data;
			if (len != sizeof(wl_cnt_ge40mcst_v1_t)) {
				IAPSTA_ERROR(cur_if->ifname,
					"type 0x%x, cnt struct length mismatch! %d != %d\n",
					type, len, (int)sizeof(wl_cnt_ge40mcst_v1_t));
			}
#ifdef RESET_AP_WAR
			if (apsta_params->war_reason == ISAM_RC_AP_RESET)
				cur_if->txbcnfrm = dtoh32(cnt->txbcnfrm);
#endif /* RESET_AP_WAR */
#ifdef RXF0OVFL_REINIT_WAR
			if (apsta_params->war_reason == ISAM_RC_RXF0OVFL_REINIT) {
				apsta_params->rxbeaconmbss = dtoh32(cnt->rxbeaconmbss);
				apsta_params->rxf0ovfl = dtoh32(cnt->rxf0ovfl);
			}
#endif /* RXF0OVFL_REINIT_WAR */
			break;
		}
		case WL_CNT_XTLV_GE80_UCODE_V1:
		{
			wl_cnt_ge80mcst_v1_t *cnt = (wl_cnt_ge80mcst_v1_t *)data;
			if (len != sizeof(wl_cnt_ge80mcst_v1_t)) {
				IAPSTA_ERROR(cur_if->ifname,
					"type 0x%x, cnt struct length mismatch! %d != %d\n",
					type, len, (int)sizeof(wl_cnt_ge80mcst_v1_t));
			}
#ifdef RESET_AP_WAR
			if (apsta_params->war_reason == ISAM_RC_AP_RESET)
				cur_if->txbcnfrm = dtoh32(cnt->txbcnfrm);
#endif /* RESET_AP_WAR */
#ifdef RXF0OVFL_REINIT_WAR
			if (apsta_params->war_reason == ISAM_RC_RXF0OVFL_REINIT) {
				apsta_params->rxbeaconmbss = dtoh32(cnt->rxbeaconmbss);
				apsta_params->rxf0ovfl = dtoh32(cnt->rxf0ovfl);
			}
#endif /* RXF0OVFL_REINIT_WAR */
			break;
		}
		default:
			break;
	}
	return res;
}

static int
wl_ext_counters_update(struct wl_if_info *cur_if, int war_reason)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);
	char *iovar_buf = g_ioctl_buf;
	uint32 corerev = 0;
	wl_cnt_info_t *cntinfo;
	uint16 ver;
	int ret = 0;

	ret = wldev_iovar_getbuf(cur_if->dev, "counters", NULL, 0,
		iovar_buf, WLC_IOCTL_MEDLEN, NULL);
	if (unlikely(ret)) {
		IAPSTA_ERROR(cur_if->ifname,
			"counters error (%d) - size = %zu\n", ret, sizeof(wl_cnt_wlc_t));
		goto exit;
	}
	cntinfo = (wl_cnt_info_t *)iovar_buf;
	cntinfo->version = dtoh16(cntinfo->version);
	cntinfo->datalen = dtoh16(cntinfo->datalen);
	ver = cntinfo->version;
	CHK_CNTBUF_DATALEN(iovar_buf, WLC_IOCTL_MEDLEN);
	if (ver > WL_CNT_VERSION_XTLV) {
		IAPSTA_ERROR(cur_if->ifname,
			"Incorrect version of counters struct: expected %d; got %d\n",
			WL_CNT_VERSION_XTLV, ver);
		goto exit;
	}

	if (ver == WL_CNT_VERSION_11) {
		wlc_rev_info_t revinfo;
		memset(&revinfo, 0, sizeof(revinfo));
		ret = wl_ext_ioctl(cur_if->dev, WLC_GET_REVINFO, &revinfo, sizeof(revinfo), 0);
		if (ret) {
			IAPSTA_ERROR(cur_if->ifname, "WLC_GET_REVINFO failed %d\n", ret);
			goto exit;
		}
		corerev = dtoh32(revinfo.corerev);
	}
	ret = wl_cntbuf_to_xtlv_format(NULL, cntinfo, WLC_IOCTL_MEDLEN, corerev);
	if (ret) {
		IAPSTA_ERROR(cur_if->ifname, "wl_cntbuf_to_xtlv_format failed %d\n", ret);
		goto exit;
	}

	apsta_params->war_reason = war_reason;
	if ((ret = bcm_unpack_xtlv_buf(cur_if, cntinfo->data, cntinfo->datalen,
			BCM_XTLV_OPTION_ALIGN32, wl_ext_counters_cbfn))) {
		IAPSTA_ERROR(cur_if->ifname, "bcm_unpack_xtlv_buf failed %d\n", ret);
		goto exit;
	}

exit:
	return ret;
}
#endif /* RESET_AP_WAR | RXF0OVFL_REINIT_WAR */

#ifdef RESET_AP_WAR
static void
wl_ext_reset_ap_timeout(unsigned long data)
{
	struct net_device *dev = (struct net_device *)data;
	struct dhd_pub *dhd;
	wl_event_msg_t msg;

	if (!dev) {
		IAPSTA_ERROR("wlan", "dev is not ready\n");
		return;
	}

	dhd = dhd_get_pub(dev);

	bzero(&msg, sizeof(wl_event_msg_t));
	IAPSTA_TRACE(dev->name, "timer expired\n");

	msg.ifidx = dhd_net2idx(dhd->info, dev);
	msg.event_type = hton32(WLC_E_RESERVED);
	msg.reason = hton32(ISAM_RC_AP_RESET);
	wl_ext_event_send(dhd->event_params, &msg, NULL);
}

static void
wl_ext_reset_ap_handler(struct wl_if_info *cur_if,
	const wl_event_msg_t *e, void *data)
{
	struct dhd_pub *dhd = dhd_get_pub(cur_if->dev);
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);
	uint32 etype = ntoh32(e->event_type);
	uint32 status = ntoh32(e->status);
	uint32 reason = ntoh32(e->reason);
	uint32 txbcnfrm;
	int ret = 0;

	if (wl_get_isam_status(cur_if, AP_CREATED)) {
		if ((etype == WLC_E_SET_SSID && status == WLC_E_STATUS_SUCCESS) ||
				(etype == WLC_E_LINK && status == WLC_E_STATUS_SUCCESS &&
				reason == WLC_E_REASON_INITIAL_ASSOC)) {
			// Link up
			wl_ext_counters_update(cur_if, ISAM_RC_AP_RESET);
			wl_timer_mod(dhd, &cur_if->reset_ap_timer, AP_TXBCNFRM_TIMEOUT);
		}
		else if (etype == WLC_E_RESERVED && reason == ISAM_RC_AP_RESET) {
			txbcnfrm = cur_if->txbcnfrm;
			ret = wl_ext_counters_update(cur_if, ISAM_RC_AP_RESET);
			if (ret)
				goto done;
			if ((cur_if->txbcnfrm != 0) && (txbcnfrm == cur_if->txbcnfrm)) {
				WL_MSG(cur_if->ifname, "reset AP mode\n");
				wl_ext_if_down(cur_if);
				wl_ext_if_up(cur_if, FALSE, 0);
			}
done:
			wl_timer_mod(dhd, &cur_if->reset_ap_timer, AP_TXBCNFRM_TIMEOUT);
		}
	}
	return;
}
#endif /* RESET_AP_WAR */

#ifdef RXF0OVFL_REINIT_WAR
static void
wl_ext_rxf0ovfl_reinit_timeout(unsigned long data)
{
	struct net_device *dev = (struct net_device *)data;
	struct dhd_pub *dhd;
	wl_event_msg_t msg;

	if (!dev) {
		IAPSTA_ERROR("wlan", "dev is not ready\n");
		return;
	}

	dhd = dhd_get_pub(dev);

	bzero(&msg, sizeof(wl_event_msg_t));
	IAPSTA_TRACE(dev->name, "timer expired\n");

	msg.ifidx = dhd_net2idx(dhd->info, dev);
	msg.event_type = hton32(WLC_E_RESERVED);
	msg.reason = hton32(ISAM_RC_RXF0OVFL_REINIT);
	wl_ext_event_send(dhd->event_params, &msg, NULL);
}

static void
wl_ext_rxf0ovfl_reinit_handler(struct wl_if_info *cur_if,
	const wl_event_msg_t *e, void *data)
{
	struct dhd_pub *dhd = dhd_get_pub(cur_if->dev);
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);
	uint32 etype = ntoh32(e->event_type);
	uint32 reason = ntoh32(e->reason);
	uint32 status = ntoh32(e->status);
	uint16 flags =  ntoh16(e->flags);
	uint32 rxbeaconmbss, rxbeaconmbss_diff = 0, rxf0ovfl, rxf0ovfl_diff = 0;
	int ret = 0;
	bool reinit = FALSE, updated = TRUE;

	if (!(dhd->conf->war & FW_REINIT_RXF0OVFL)) {
		return;
	}

	if ((cur_if->ifmode == ISTA_MODE) &&
			(etype == WLC_E_LINK) && (flags & WLC_EVENT_MSG_LINK)) {
		// Link up
		wl_ext_counters_update(cur_if, ISAM_RC_RXF0OVFL_REINIT);
		wl_timer_mod(dhd, &apsta_params->rxf0ovfl_timer, RXF0OVFL_POLLING_TIMEOUT);
	}
	else if ((cur_if->ifmode == IAP_MODE) &&
			((etype == WLC_E_SET_SSID && status == WLC_E_STATUS_SUCCESS) ||
			(etype == WLC_E_LINK && status == WLC_E_STATUS_SUCCESS &&
			reason == WLC_E_REASON_INITIAL_ASSOC))) {
		// Link up
		wl_ext_counters_update(cur_if, ISAM_RC_RXF0OVFL_REINIT);
		wl_timer_mod(dhd, &apsta_params->rxf0ovfl_timer, RXF0OVFL_POLLING_TIMEOUT);
	}
	else if ((etype == WLC_E_RESERVED) && (reason == ISAM_RC_RXF0OVFL_REINIT) &&
			(wl_ext_iapsta_other_if_enabled(cur_if->dev))) {
		if (apsta_params->tput_sum >= 1 || apsta_params->tput_sum_kb >= 1) {
			if (apsta_params->rxf0ovfl)
				updated = TRUE;
			rxbeaconmbss = apsta_params->rxbeaconmbss;
			rxf0ovfl = apsta_params->rxf0ovfl;
			ret = wl_ext_counters_update(cur_if, ISAM_RC_RXF0OVFL_REINIT);
			if (ret)
				goto done;
			rxf0ovfl_diff = apsta_params->rxf0ovfl - rxf0ovfl;
			rxbeaconmbss_diff = apsta_params->rxbeaconmbss - rxbeaconmbss;
			if (rxf0ovfl_diff > 0) {
				IAPSTA_INFO(cur_if->ifname,
					"rxf0ovfl diff = %d, rxbeaconmbss diff = %d\n",
					rxf0ovfl_diff, rxbeaconmbss_diff);
			}
			if (wl_ext_if_enabled(apsta_params, ISTA_MODE)) {
				if (rxbeaconmbss_diff < 5 && rxf0ovfl_diff > RXF0OVFL_THRESHOLD)
					reinit = TRUE;
			}
#if 0
			if (wl_ext_if_enabled(apsta_params, IAP_MODE)) {
				if (rxf0ovfl_diff > RXF0OVFL_THRESHOLD)
					reinit = TRUE;
			}
#endif
			if (reinit && updated) {
				WL_MSG(cur_if->ifname, "wl reinit\n");
				wl_ext_ioctl(cur_if->dev, WLC_INIT, NULL, 0, 1);
			}
		}
		else {
			apsta_params->rxf0ovfl = 0;
		}
done:
		wl_timer_mod(dhd, &apsta_params->rxf0ovfl_timer, RXF0OVFL_POLLING_TIMEOUT);
	}

	return;
}
#endif /* RXF0OVFL_REINIT_WAR */

#ifdef EAP_FAILURE_REASSOC
#include <eapol.h>
#define EAP_CODE_FAILURE    4
typedef struct eap_header_fmt {
	uint8 code;
	uint8 id;
	uint16 len;
	uint8 type;
	uint8 data[1];
} PACKED_STRUCT eap_header_fmt_t;

static bool
wl_ext_check_eap_fail_type(uint8 *pktdata, uint32 pktlen)
{
	struct ether_header *eh = (struct ether_header *)pktdata;
	eapol_header_t *eapol_hdr = (eapol_header_t *)pktdata;
	eap_header_fmt_t *eap_hdr;
	uint16 ether_type;

	if (!pktdata || pktlen < ETHER_HDR_LEN) {
		return FALSE;
	}

	/* validation 802.1x */
	ether_type = ntoh16(eh->ether_type);
	if (ether_type != ETHER_TYPE_802_1X) {
		return FALSE;
	}

	/* validation EAP Failure type */
	eap_hdr = (eap_header_fmt_t *)(eapol_hdr->body);
	if (eapol_hdr->type == EAP_PACKET && eap_hdr->code == EAP_CODE_FAILURE) {
		return TRUE;
	}

	return FALSE;
}

bool
wl_ext_eap_fail_reassoc(dhd_pub_t *dhd, int ifidx, uint8 *pktdata, uint32 pktlen)
{
	struct wl_apsta_params *apsta_params = dhd->iapsta_params;
	struct wl_if_info *cur_if = NULL;

	if (ifidx < MAX_IF_NUM) {
		cur_if = &apsta_params->if_info[ifidx];
		if (cur_if->dev && wl_ext_sta_connected(cur_if->dev)) {
			if (wl_ext_check_eap_fail_type(pktdata, pktlen)) {
				IAPSTA_ERROR(cur_if->ifname, "EAP Packet, Failure received, trigger reassoc\n");
				wl_cfgext_trigger_reassoc_scan(cur_if->dev, NULL, SCAN_RCC, STA_CONNECT_TIMEOUT);
				return TRUE;
			}
		}
	}

	return FALSE;
}
#endif /* EAP_FAILURE_REASSOC */

#ifdef ARP_DETECTION
static bool
wl_ext_check_arp_type(uint8 *pktdata, uint32 pktlen, uint16 oper)
{
	struct ether_header *eh;
	struct bcmarp *arph;
	uint16 ether_type;
	uint16 opcode;

	if (!pktdata || pktlen < ETHER_HDR_LEN) {
		return FALSE;
	}

	eh = (struct ether_header *)pktdata;
	ether_type = ntoh16(eh->ether_type);

	/* validation ARP */
	if (ether_type != ETHER_TYPE_ARP) {
		return FALSE;
	}

	/* validation ARP type */
	arph = (struct bcmarp *)&pktdata[ETHER_HDR_LEN];
	opcode = ntoh16(arph->oper);
	if ((arph->htype != hton16(HTYPE_ETHERNET)) ||
		(arph->hlen != ETHER_ADDR_LEN) ||
		(arph->plen != 4) ||
		(opcode != oper)) {
		return FALSE;
	}

	return TRUE;
}

int
wl_ext_arp_receive(dhd_pub_t *dhd, int ifidx, uint8 *pktdata, uint32 pktlen)
{
	struct wl_apsta_params *apsta_params = dhd->iapsta_params;
	struct wl_if_info *cur_if = NULL;

	if (!(dhd->conf->war & ARP_DETECTION_WAR)) {
		return 0;
	}

	if (ifidx < MAX_IF_NUM) {
		cur_if = &apsta_params->if_info[ifidx];
		if (cur_if->dev && (cur_if->ifmode == ISTA_MODE) &&
				wl_get_isam_status(cur_if, STA_CONNECTED)) {
			if (wl_ext_check_arp_type(pktdata, pktlen, ARP_OPC_REPLY)) {
				IAPSTA_TRACE(cur_if->ifname, "ARP REPLY received\n");
				osl_do_gettimeofday(&cur_if->arp_rx_ts);
			}
		}
	}

	return 0;
}

void
wl_ext_trigger_arp(struct net_device *dev)
{
	struct dhd_pub *dhd = dhd_get_pub(dev);
	struct wl_if_info *cur_if = NULL;

	if (!(dhd->conf->war & ARP_DETECTION_WAR)) {
		return;
	}

	cur_if = wl_get_cur_if(dev);
	if (!cur_if)
		return;

	if ((cur_if->ifmode == ISTA_MODE) && wl_get_isam_status(cur_if, STA_CONNECTED)) {
		IAPSTA_INFO(dev->name, "Enter\n");
		cur_if->gw_ipaddr = 0;
		wl_timer_mod(dhd, &cur_if->arp_timer, 10);
	}
}

static void
wl_ext_arp_detection_timeout(unsigned long data)
{
	struct net_device *dev = (struct net_device *)data;
	struct dhd_pub *dhd;
	wl_event_msg_t msg;

	if (!dev) {
		IAPSTA_ERROR("wlan", "dev is not ready\n");
		return;
	}

	dhd = dhd_get_pub(dev);

	bzero(&msg, sizeof(wl_event_msg_t));
	IAPSTA_TRACE(dev->name, "timer expired\n");

	msg.ifidx = dhd_net2idx(dhd->info, dev);
	msg.event_type = hton32(WLC_E_RESERVED);
	msg.reason = hton32(ISAM_RC_ARP_DETECTION);
	wl_ext_event_send(dhd->event_params, &msg, NULL);
}

static void
wl_ext_update_default_gateway_ip(struct wl_if_info *cur_if)
{
	struct net_device *dev = cur_if->dev;
	struct rtable *rt;
	struct flowi4 fl4;

	if (dev->flags & IFF_UP) {
		memset(&fl4, 0, sizeof(fl4));
		/* fill ARP structure */
		fl4.flowi4_oif = dev->ifindex;
		fl4.daddr = htonl(0x08080808); /* destination IP 8.8.8.8 */
		fl4.saddr = 0;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 18, 0))
		fl4.__fl_common.flowic_dscp = inet_dsfield_to_dscp(0);
#else
		fl4.__fl_common.flowic_tos = 0;
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(6, 18, 0) */
		fl4.__fl_common.flowic_scope = RT_SCOPE_UNIVERSE;
		fl4.__fl_common.flowic_proto = IPPROTO_IP;

		/* get gate way items */
		rt = ip_route_output_key(dev_net(dev), &fl4);
		if (!IS_ERR(rt)) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 2, 0))
			if (rt->rt_gw_family == AF_INET && rt->rt_gw4) {
				cur_if->gw_ipaddr = rt->rt_gw4;
				cur_if->local_ipaddr = fl4.saddr;
			}
#else
			cur_if->gw_ipaddr = rt->rt_gateway;
			cur_if->local_ipaddr = fl4.saddr;
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 2, 0) */
		}
	}
}

static void
wl_ext_arp_detection_handler(struct wl_if_info *cur_if,
	const wl_event_msg_t *e, void *data)
{
	struct net_device *dev = cur_if->dev;
	struct dhd_pub *dhd = dhd_get_pub(dev);
	struct wl_ext_chan_info chan_info;
	struct osl_timespec cur_ts;
	uint32 diff_ms = 0;
	uint32 etype = ntoh32(e->event_type);
	uint32 status =  ntoh32(e->status);
	uint32 reason = ntoh32(e->reason);
	uint16 flags =  ntoh16(e->flags);
	int roam[2] = {0, 0}, roam_off = 1, cur_rssi, ret = 0;
	scb_val_t scb_val;
	bool send_later = FALSE;
	int tmo_start;
#ifdef WL_EXT_REASSOC
	struct ether_addr bssid = {{0, 0, 0, 0, 0, 0}};
#endif /* WL_EXT_REASSOC */

	if (!(dhd->conf->war & ARP_DETECTION_WAR) || (cur_if->ifmode != ISTA_MODE)) {
		return;
	}

	if (etype == WLC_E_LINK) {
		cur_if->gw_ipaddr = 0;
		if (flags & WLC_EVENT_MSG_LINK) {
			// Link up
#ifdef TPUT_MONITOR
			struct wl_apsta_params *apsta_params = wl_get_apsta_params(dev);
			uint tput_ms = 1000;
			dhd->conf->tput_monitor_ms = tput_ms;
			wl_timer_mod(dhd, &apsta_params->monitor_timer, tput_ms);
			IAPSTA_INFO(dev->name, "change tput_monitor_ms to %d\n", tput_ms);
#endif /* TPUT_MONITOR */
			IAPSTA_INFO(dev->name, "start detection %dms later(etype=%d)\n",
				ARP_DETECTION_START_MS, etype);
			wl_timer_mod(dhd, &cur_if->arp_timer, ARP_DETECTION_START_MS);
		} else {
			// Link down
			wl_timer_mod(dhd, &cur_if->arp_timer, 0);
		}
	}
	else if (etype == WLC_E_SET_SSID || etype == WLC_E_BSSID) {
		cur_if->gw_ipaddr = 0;
		if (status == WLC_E_STATUS_SUCCESS) {
			IAPSTA_INFO(dev->name, "start detection %dms later(etype=%d)\n",
				ARP_DETECTION_START_MS, etype);
			wl_timer_mod(dhd, &cur_if->arp_timer, ARP_DETECTION_START_MS);
		} else {
			wl_timer_mod(dhd, &cur_if->arp_timer, 0);
		}
	}
	else if (etype == WLC_E_ROAM_PREP) {
		IAPSTA_INFO(dev->name, "start detection %dms later(etype=%d)\n",
			ARP_DETECTION_START_MS, etype);
		wl_timer_mod(dhd, &cur_if->arp_timer, ARP_DETECTION_START_MS);
		cur_if->gw_ipaddr = 0;
	}
	else if ((etype == WLC_E_RESERVED) && (reason == ISAM_RC_ARP_DETECTION)) {
		osl_do_gettimeofday(&cur_ts);
		diff_ms = osl_do_gettimediff(&cur_ts, &cur_if->ip_ready_ts) / 1000;
		if (!wl_ext_sta_connected(dev)) {
			IAPSTA_INFO(dev->name, "skip (dhd not connected %d)\n", cur_if->conn_state);
			cur_if->gw_ipaddr = 0;
			send_later = TRUE;
		}
		else if (cur_if->gw_ipaddr == 0) {
			wl_ext_update_default_gateway_ip(cur_if);
			if (cur_if->gw_ipaddr) {
				IAPSTA_INFO(dev->name, "gateway=%pI4, local=%pI4\n",
					&cur_if->gw_ipaddr, &cur_if->local_ipaddr);
				osl_do_gettimeofday(&cur_if->ip_ready_ts);
				osl_do_gettimeofday(&cur_if->arp_rx_ts);
			} else {
				IAPSTA_INFO(dev->name, "Skip (gateway ip not updated yet)\n");
				send_later = TRUE;
			}
		}
#ifdef TPUT_MONITOR
		else if (cur_if->tput_info.tput_rx && diff_ms > (ARP_DETECTION_START_MS * 2)) {
			// Don't send ARP if there is Rx traffic more than 1Mbps
			send_later = TRUE;
		}
#endif /* TPUT_MONITOR */
		else if (!wl_ext_associated(dev, NULL)) {
			IAPSTA_INFO(dev->name, "skip (fw not connected)\n");
			cur_if->gw_ipaddr = 0;
			send_later = TRUE;
		}
		else {
			ret = wldev_ioctl_get(dev, WLC_GET_RSSI, &scb_val, sizeof(scb_val_t));
			cur_rssi = dtoh32(scb_val.val);
			if (!ret && (cur_rssi < ARP_DETECTION_LOWRSSI)) {
				// Don't send ARP if RSSI is too low
				IAPSTA_INFO(dev->name, "skip (low rssi %d < %d)\n",
					cur_rssi, ARP_DETECTION_LOWRSSI);
				send_later = TRUE;
			} else if (!ret) {
				ret = wl_ext_iovar_getint(dev, "roam_off", &roam_off);
				if (!ret && roam_off == 0) {
					// Don't send ARP if RSSI lower than roam_trigger
					if (wl_ext_get_chanspec(dev, &chan_info)) {
						roam[1] = chan_info.band;
						ret = wl_ext_ioctl(dev, WLC_GET_ROAM_TRIGGER, roam, sizeof(roam), 0);
						if (!ret && (cur_rssi < roam[0])) {
							IAPSTA_INFO(dev->name,
								"skip (rssi %d < roam_trigger %d %s)\n",
								cur_rssi, roam[0], WLCBAND2STR(chan_info.band));
							send_later = TRUE;
						}
					}
				}
			}
		}
		if (send_later) {
			osl_do_gettimeofday(&cur_if->arp_rx_ts);
			wl_timer_mod(dhd, &cur_if->arp_timer, ARP_DETECTION_INTERVAL);
			goto exit;
		}

		// Send ARP
		IAPSTA_INFO(dev->name, "Send ARP\n");
		arp_send(ARPOP_REQUEST, ETH_P_ARP, cur_if->gw_ipaddr, dev,
			cur_if->local_ipaddr, NULL, dev->dev_addr, NULL);

		osl_do_gettimeofday(&cur_ts);
		diff_ms = osl_do_gettimediff(&cur_ts, &cur_if->arp_rx_ts) / 1000;
#ifdef ARP_DETECTION_REINIT
		tmo_start = ARP_DETECTION_TIMEOUT;
		if (diff_ms > tmo_start &&
				diff_ms < (tmo_start + ARP_DETECTION_INTERVAL)) {
			// 1st trigger fw reinit
			IAPSTA_ERROR(dev->name, "ARP timeout(%dms), trigger reinit\n", diff_ms);
			wl_ext_ioctl(dev, WLC_INIT, NULL, 0, 1);
		} else
#else
		tmo_start = ARP_DETECTION_TIMEOUT - (2 * ARP_DETECTION_INTERVAL);
#endif /* ARP_DETECTION_REINIT */
#ifdef WL_EXT_REASSOC
		if ((dhd->conf->war & STA_REASSOC_WAR) &&
				diff_ms > (tmo_start + 2 * ARP_DETECTION_INTERVAL) &&
				diff_ms < (tmo_start + 3 * ARP_DETECTION_INTERVAL)) {
			// 2rd trigger reassoc
			IAPSTA_ERROR(dev->name, "ARP timeout(%dms), trigger reassoc\n", diff_ms);
			wl_ext_associated(dev, &bssid);
			wl_cfgext_trigger_reassoc_scan(dev, &bssid, SCAN_RCC, STA_CONNECT_TIMEOUT);
		} else
#endif /* WL_EXT_REASSOC */
#ifdef WL_ESCAN
		if (diff_ms > (tmo_start + 2 * ARP_DETECTION_INTERVAL) &&
				diff_ms < (tmo_start + 3 * ARP_DETECTION_INTERVAL)) {
			// 2nd trigger scan
			IAPSTA_ERROR(dev->name, "ARP timeout(%dms), trigger scan\n", diff_ms);
			wl_iapsta_trigger_scan(dev, SCAN_RCC);
		} else
#endif /* WL_ESCAN */
		if (diff_ms > (tmo_start + 20 * ARP_DETECTION_INTERVAL)) {
			// 3th trigger disconnect
			IAPSTA_ERROR(dev->name, "ARP timeout(%dms), trigger disconnect\n", diff_ms);
#ifdef WL_CFG80211
			wl_cfg80211_disassoc(dev, WLAN_REASON_DEAUTH_LEAVING);
#else
			wl_ext_ioctl(dev, WLC_DISASSOC, NULL, 0, 1);
#endif /* WL_CFG80211 */
			goto exit;
		}
		wl_timer_mod(dhd, &cur_if->arp_timer, ARP_DETECTION_INTERVAL);
	}

exit:
	return;
}
#endif /* ARP_DETECTION */

void
wl_ext_iapsta_link(struct wl_if_info *cur_if,
	const wl_event_msg_t *e, void *data)
{
#ifdef PROPTX_MAXCOUNT
	struct dhd_pub *dhd = dhd_get_pub(cur_if->dev);
#endif /* PROPTX_MAXCOUNT */
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);
	uint32 event_type = ntoh32(e->event_type);
	uint32 status =  ntoh32(e->status);
	uint32 reason =  ntoh32(e->reason);
	uint16 flags =  ntoh16(e->flags);

	if (wl_ext_slave_if(cur_if)) {
		if (event_type == WLC_E_LINK) {
			if (!(flags & WLC_EVENT_MSG_LINK)) {
				WL_MSG(cur_if->ifname,
					"Link down with %pM, %s(%d), reason %d\n",
					&e->addr, bcmevent_get_name(event_type),
					event_type, reason);
#ifdef SET_CARRIER
				wl_ext_net_setcarrier(cur_if, FALSE, FALSE);
#endif /* SET_CARRIER */
				wl_clr_isam_status(cur_if, STA_CONNECTED);
			} else {
				WL_MSG(cur_if->ifname, "Link UP with %pM\n", &e->addr);
				wl_ext_isam_status(cur_if->dev, NULL, 0);
#ifdef SET_CARRIER
				wl_ext_net_setcarrier(cur_if, TRUE, FALSE);
#endif /* SET_CARRIER */
				wl_set_isam_status(cur_if, STA_CONNECTED);
			}
			wl_clr_isam_status(cur_if, STA_CONNECTING);
			wake_up_interruptible(&apsta_params->netif_change_event);
#ifdef PROPTX_MAXCOUNT
			wl_ext_update_wlfc_maxcount(dhd);
#endif /* PROPTX_MAXCOUNT */
		}
		else if (event_type == WLC_E_SET_SSID && status != WLC_E_STATUS_SUCCESS) {
			WL_MSG(cur_if->ifname,
				"connect failed event=%d, reason=%d, status=%d\n",
				event_type, reason, status);
			wl_clr_isam_status(cur_if, STA_CONNECTING);
			wl_clr_isam_status(cur_if, STA_CONNECTED);
			wake_up_interruptible(&apsta_params->netif_change_event);
#ifdef PROPTX_MAXCOUNT
			wl_ext_update_wlfc_maxcount(dhd);
#endif /* PROPTX_MAXCOUNT */
		}
		else if (event_type == WLC_E_DEAUTH || event_type == WLC_E_DEAUTH_IND ||
				event_type == WLC_E_DISASSOC || event_type == WLC_E_DISASSOC_IND) {
			WL_MSG(cur_if->ifname, "Link down with %pM, %s(%d), reason %d\n",
				&e->addr, bcmevent_get_name(event_type),
				event_type, reason);
#ifdef SET_CARRIER
			wl_ext_net_setcarrier(cur_if, FALSE, FALSE);
#endif /* SET_CARRIER */
		}
	}
	else if (wl_ext_master_if(cur_if)) {
		if ((event_type == WLC_E_SET_SSID && status == WLC_E_STATUS_SUCCESS) ||
				(event_type == WLC_E_LINK && status == WLC_E_STATUS_SUCCESS &&
				reason == WLC_E_REASON_INITIAL_ASSOC)) {
			if (wl_get_isam_status(cur_if, AP_CREATING)) {
				WL_MSG(cur_if->ifname, "Link up (etype=%d)\n", event_type);
				wl_set_isam_status(cur_if, AP_CREATED);
				wl_ext_isam_status(cur_if->dev, NULL, 0);
				wake_up_interruptible(&apsta_params->netif_change_event);
			} else {
				wl_set_isam_status(cur_if, AP_CREATED);
				WL_MSG(cur_if->ifname, "Link up w/o creating? (etype=%d)\n",
					event_type);
				wl_ext_isam_status(cur_if->dev, NULL, 0);
			}
#ifdef SET_CARRIER
			wl_ext_net_setcarrier(cur_if, TRUE, FALSE);
#endif /* SET_CARRIER */
#ifdef PROPTX_MAXCOUNT
			wl_ext_update_wlfc_maxcount(dhd);
#endif /* PROPTX_MAXCOUNT */
		}
		else if ((event_type == WLC_E_LINK && reason == WLC_E_LINK_BSSCFG_DIS) ||
				(event_type == WLC_E_LINK && status == WLC_E_STATUS_SUCCESS &&
				reason == WLC_E_REASON_DEAUTH)) {
			wl_clr_isam_status(cur_if, AP_CREATED);
			WL_MSG(cur_if->ifname, "Link down, reason=%d\n", reason);
#ifdef SET_CARRIER
			wl_ext_net_setcarrier(cur_if, FALSE, FALSE);
#endif /* SET_CARRIER */
#ifdef PROPTX_MAXCOUNT
			wl_ext_update_wlfc_maxcount(dhd);
#endif /* PROPTX_MAXCOUNT */
		}
		else if ((event_type == WLC_E_ASSOC_IND || event_type == WLC_E_REASSOC_IND) &&
				reason == DOT11_SC_SUCCESS) {
			WL_MSG(cur_if->ifname, "connected device %pM\n", &e->addr);
			wl_ext_isam_status(cur_if->dev, NULL, 0);
		}
		else if (event_type == WLC_E_DISASSOC_IND ||
				event_type == WLC_E_DEAUTH_IND ||
				(event_type == WLC_E_DEAUTH && reason != DOT11_RC_RESERVED)) {
			WL_MSG_RLMT(cur_if->ifname, &e->addr, ETHER_ADDR_LEN,
				"disconnected device %pM, %s(%d), reason=%d\n",
				&e->addr, bcmevent_get_name(event_type),
				event_type, reason);
			wl_ext_isam_status(cur_if->dev, NULL, 0);
		}
	}
}

#ifdef WL_PASSIVE_CHAN_UPDATE
void
wl_ext_set_wiphy_update(struct net_device *dev, bool set)
{
	struct dhd_pub *dhd = dhd_get_pub(dev);
	struct wl_apsta_params *apsta_params = dhd->iapsta_params;

	IAPSTA_INFO(dev->name, "%s\n", set ? "SET" : "CLR");
	apsta_params->wiphy_update = set;
}

bool
wl_ext_get_wiphy_update(struct net_device *dev)
{
	struct dhd_pub *dhd = dhd_get_pub(dev);
	struct wl_apsta_params *apsta_params = dhd->iapsta_params;

	if (apsta_params->wiphy_update)
		return TRUE;

	return FALSE;
}
#endif /* WL_PASSIVE_CHAN_UPDATE */

static void
wl_ext_iapsta_ccode(struct wl_if_info *cur_if,
	const wl_event_msg_t *e, void *data)
{
	struct dhd_pub *dhd = dhd_get_pub(cur_if->dev);
#ifdef WL_PASSIVE_CHAN_UPDATE
	struct wl_apsta_params *apsta_params = dhd->iapsta_params;
	struct bcm_cfg80211 *cfg = wl_get_cfg(cur_if->dev);
	uint32 status =  ntoh32(e->status);
#endif /* WL_PASSIVE_CHAN_UPDATE */
	uint32 event_type = ntoh32(e->event_type);
	char country_str[WLC_CNTRY_BUF_SZ] = { 0 };
	wl_country_t cspec = {{0}, 0, {0}};

	if (event_type == WLC_E_COUNTRY_CODE_CHANGED) {
		if (strlcpy(country_str, data, WL_CCODE_LEN + 1) >= WLC_CNTRY_BUF_SZ) {
			return;
		}
		WL_MSG(cur_if->ifname, "Updated new country %s\n", country_str);
		dhd_conf_get_country(dhd, &cspec);
		WL_MSG(cur_if->dev->name, "Country code: %s (%s/%d)\n",
			cspec.country_abbrev, cspec.ccode, cspec.rev);
#ifdef WL_PASSIVE_CHAN_UPDATE
		if (!wl_escan_passive_chan_scan(cur_if->dev))
			wl_ext_set_wiphy_update(cur_if->dev, TRUE);
#endif /* WL_PASSIVE_CHAN_UPDATE */
	}
#ifdef WL_PASSIVE_CHAN_UPDATE
	else if (event_type == WLC_E_ESCAN_RESULT && status == WLC_E_STATUS_SUCCESS &&
			apsta_params->wiphy_update) {
		wl_update_wiphybands(cfg, FALSE);
		wl_ext_set_wiphy_update(cur_if->dev, FALSE);
	}
#endif /* WL_PASSIVE_CHAN_UPDATE */
}

void
wl_ext_iapsta_csa_event(struct net_device *dev)
{
	struct wl_if_info *cur_if = NULL;

	cur_if = wl_get_cur_if(dev);
	if (!cur_if)
		return;

	if (wl_ext_master_if(cur_if) && wl_ext_associated(cur_if->dev, NULL)) {
		IAPSTA_INFO(cur_if->dev->name, "complete\n");
		cur_if->csa_complete = TRUE;
		wake_up_interruptible(&cur_if->csa_complete_event);
	}
}

static void
wl_ext_csa_event(struct wl_if_info *cur_if,
	const wl_event_msg_t *e, void *data)
{
	uint32 event_type = ntoh32(e->event_type);

	if (event_type == WLC_E_CSA_COMPLETE_IND)
		wl_ext_iapsta_csa_event(cur_if->dev);
}

static void
wl_ext_iapsta_event(struct net_device *dev, void *argu,
	const wl_event_msg_t *e, void *data)
{
#ifdef ACS_MONITOR
	struct wl_apsta_params *apsta_params = (struct wl_apsta_params *)argu;
#endif /* ACS_MONITOR */
	struct dhd_pub *dhd = dhd_get_pub(dev);
	struct wl_if_info *cur_if = NULL;

	cur_if = wl_get_cur_if(dev);
	if (!cur_if || !cur_if->dev) {
		IAPSTA_DBG(dev->name, "ifidx %d is not ready\n", e->ifidx);
		return;
	}

	wl_ext_iapsta_link(cur_if, e, data);
	wl_ext_iapsta_ccode(cur_if, e, data);
	wl_ext_csa_event(cur_if, e, data);
#ifdef WL_EXT_REASSOC
	wl_ext_reassoc_scan_event(cur_if, e, data);
#endif /* WL_EXT_REASSOC */
#ifdef TPUT_MONITOR
	if (dhd->conf->tput_monitor_ms)
		wl_tput_monitor_handler(cur_if, e, data);
#endif /* TPUT_MONITOR */

#if defined(WLMESH) && defined(WL_ESCAN)
	wl_mesh_event_handler(cur_if, e, data);
#endif /* WLMESH && WL_ESCAN */
#ifdef ACS_MONITOR
	if ((apsta_params->acs & ACS_DRV_BIT) && apsta_params->acs_tmo)
		wl_acs_handler(cur_if, e, data);
#endif /* ACS_MONITOR */
#ifdef EAPOL_RESEND
	wl_resend_eapol_handler(cur_if, e, data);
#endif /* EAPOL_RESEND */
#ifdef KEY_INSTALL_CHECK
	wl_ext_key_install_handler(cur_if, e, data);
#endif /* KEY_INSTALL_CHECK */
	wl_ext_sync_interface_handler(cur_if, e, data);
#ifdef RESTART_AP_WAR
	wl_ext_restart_ap_handler(cur_if, e, data);
#endif /* RESTART_AP_WAR */
#ifdef RESET_AP_WAR
	wl_ext_reset_ap_handler(cur_if, e, data);
#endif /* RESET_AP_WAR */
#ifdef RXF0OVFL_REINIT_WAR
	wl_ext_rxf0ovfl_reinit_handler(cur_if, e, data);
#endif /* RXF0OVFL_REINIT_WAR */
#ifdef ARP_DETECTION
	wl_ext_arp_detection_handler(cur_if, e, data);
#endif /* ARP_DETECTION */

	return;
}

static void
wl_ext_get_slave_rssi_str(struct net_device *dev, void *link, char *mac,
	char *rssi_buf, int buf_len)
{
	struct wl_if_info *cur_if = wl_get_cur_if(dev);

	memset(rssi_buf, 0, buf_len);
	if (wl_ext_slave_if(cur_if)) {
		strcpy(rssi_buf, ", rssi=");
		wl_phy_rssi_ant(dev, mac, rssi_buf+strlen(rssi_buf), buf_len-strlen(rssi_buf));
	}
}

static int
wl_ext_link_status(struct net_device *dev, char *dump_buf, int dump_len)
{
	struct ether_addr bssid;
	struct wl_ext_chan_info chan_info;
	u32 chanspec = 0;
	char rssi_buf[32], sec_str[64];
	int dump_written = 0;
	wlc_ssid_t ssid = { 0, {0} };

	wl_ext_ioctl(dev, WLC_GET_SSID, &ssid, sizeof(ssid), 0);
	wl_ext_get_sec(dev, sec_str, sizeof(sec_str), FALSE);
	wl_ext_get_slave_rssi_str(dev, NULL, (char *)&bssid, rssi_buf, sizeof(rssi_buf));
	wl_ext_associated(dev, &bssid);
	chanspec = wl_ext_get_chanspec(dev, &chan_info);
	dump_written += snprintf(dump_buf+dump_written, dump_len,
		"\n" DHD_LOG_PREFIXS "[%s] bssid=%pM, "
		"chan=%s-%-3d(%sMHz)%s, sec=%-20s, SSID=\"%s\"",
		dev->name, &bssid, WLCBAND2STR(chan_info.band), chan_info.chan,
		wf_chspec_to_bw_str(chanspec), rssi_buf, sec_str, ssid.SSID);

	return dump_written;
}

static int
wl_ext_isam_dev_status(struct net_device *dev, ifmode_t ifmode,
	char *dump_buf, int dump_len)
{
	int dump_written = 0;

	if (wl_ext_associated(dev, NULL)) {
		dump_written += wl_ext_link_status(dev,
			dump_buf+dump_written, dump_len-dump_written);
		if (ifmode == IAP_MODE || ifmode == IGO_MODE) {
			dump_written += wl_ext_assoclist(dev, NULL,
				dump_buf+dump_written, dump_len-dump_written);
		}
#ifdef WLMESH
		else if (ifmode == IMESH_MODE) {
			dump_written += snprintf(dump_buf+dump_written, dump_len, "\n");
			dump_written += wl_ext_mesh_peer_status(dev, NULL,
				dump_buf+dump_written, dump_len-dump_written);
		}
#endif /* WLMESH */
	} else {
		dump_written += snprintf(dump_buf+dump_written, dump_len,
			"\n" DHD_LOG_PREFIXS "[%s]", dev->name);
	}

	return dump_written;
}

int
wl_ext_isam_status(struct net_device *dev, char *command, int total_len)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(dev);
	struct wl_if_info *cur_if = NULL;
#ifdef WLDWDS
	struct wl_dwds_info *dwds_info = NULL;
#endif /* WLDWDS */
	char *dump_buf = NULL;
	int dump_len = WL_DUMP_BUF_LEN, dump_written = 0;
	int i;

	if (command || android_msg_level & ANDROID_INFO_LEVEL) {
		if (command) {
			dump_buf = command;
			dump_len = total_len;
		} else {
			dump_buf = g_ioctl_buf;
			memset(dump_buf, 0, dump_len);
		}
		for (i=0; i<MAX_IF_NUM; i++) {
			cur_if = &apsta_params->if_info[i];
			if (cur_if->dev) {
				dump_written += wl_ext_isam_dev_status(cur_if->dev, cur_if->ifmode,
					dump_buf+dump_written, dump_len-dump_written);
			}
		}
#ifdef WLDWDS
		for (i=0; i<MAX_DWDS_IF_NUM; i++) {
			dwds_info = &apsta_params->dwds_info[i];
			if (dwds_info->dev) {
				dump_written += wl_ext_isam_dev_status(dwds_info->dev, IAP_MODE,
					dump_buf+dump_written, dump_len-dump_written);
			}
		}
#endif /* WLDWDS */
		IAPSTA_INFO(dev->name, "%s\n", dump_buf);
	}

	return dump_written;
}

int
wl_ext_isam_param(struct net_device *dev, char *command, int total_len)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(dev);
	int ret = -1;
	char *pick_tmp, *data, *param;
	int bytes_written=-1;

	IAPSTA_TRACE(dev->name, "command=%s, len=%d\n", command, total_len);

	pick_tmp = command;
	param = bcmstrtok(&pick_tmp, " ", 0); // pick isam_param
	param = bcmstrtok(&pick_tmp, " ", 0); // pick cmd
	while (param != NULL) {
		data = bcmstrtok(&pick_tmp, " ", 0); // pick data
		if (!strcmp(param, "acs")) {
			if (data) {
				apsta_params->acs = simple_strtol(data, NULL, 0);
				ret = 0;
			} else {
				bytes_written = snprintf(command, total_len, "%d", apsta_params->acs);
				ret = bytes_written;
				goto exit;
			}
		}
#ifdef ACS_MONITOR
		else if (!strcmp(param, "acs_tmo")) {
			if (data) {
				struct dhd_pub *dhd = dhd_get_pub(dev);
				struct wl_if_info *cur_if = NULL;
				uint acs_tmo;
				cur_if = wl_get_cur_if(dev);
				if (!cur_if)
					goto exit;
				acs_tmo = simple_strtol(data, NULL, 0);
				if (apsta_params->acs_tmo != acs_tmo) {
					apsta_params->acs_tmo = acs_tmo;
					WL_MSG(dev->name, "acs_timer reset to %d\n", acs_tmo);
					wl_timer_mod(dhd, &cur_if->acs_timer, 0);
				}
				ret = 0;
			} else {
				bytes_written = snprintf(command, total_len, "%d", apsta_params->acs_tmo);
				ret = bytes_written;
				goto exit;
			}
		}
#endif /* ACS_MONITOR */
		param = bcmstrtok(&pick_tmp, " ", 0); // pick cmd
	}

exit:
	return ret;
}

#ifdef ISAM_CONFIG
#ifndef WL_STATIC_IF
s32
wl_ext_add_del_bss(struct net_device *ndev, s32 bsscfg_idx,
	int iftype, s32 del, u8 *addr)
{
	s32 ret = BCME_OK;
	s32 val = 0;
	u8 ioctl_buf[WLC_IOCTL_SMLEN];
	struct {
		s32 cfg;
		s32 val;
		struct ether_addr ea;
	} bss_setbuf;

	IAPSTA_TRACE(ndev->name, "wl_iftype:%d del:%d \n", iftype, del);

	bzero(&bss_setbuf, sizeof(bss_setbuf));

	/* AP=2, STA=3, up=1, down=0, val=-1 */
	if (del) {
		val = WLC_AP_IOV_OP_DELETE;
	} else if (iftype == WL_INTERFACE_TYPE_AP) {
		/* Add/role change to AP Interface */
		IAPSTA_TRACE(ndev->name, "Adding AP Interface\n");
		val = WLC_AP_IOV_OP_MANUAL_AP_BSSCFG_CREATE;
	} else if (iftype == WL_INTERFACE_TYPE_STA) {
		/* Add/role change to STA Interface */
		IAPSTA_TRACE(ndev->name, "Adding STA Interface\n");
		val = WLC_AP_IOV_OP_MANUAL_STA_BSSCFG_CREATE;
	} else {
		IAPSTA_ERROR(ndev->name, "add_del_bss NOT supported for IFACE type:0x%x", iftype);
		return -EINVAL;
	}

	if (!del) {
		wl_ext_bss_iovar_war(ndev, &val);
	}

	bss_setbuf.cfg = htod32(bsscfg_idx);
	bss_setbuf.val = htod32(val);

	if (addr) {
		memcpy(&bss_setbuf.ea.octet, addr, ETH_ALEN);
	}

	IAPSTA_INFO(ndev->name, "wl bss %d bssidx:%d\n", val, bsscfg_idx);
	ret = wl_ext_iovar_setbuf(ndev, "bss", &bss_setbuf, sizeof(bss_setbuf),
		ioctl_buf, WLC_IOCTL_SMLEN, NULL);
	if (ret != 0)
		IAPSTA_ERROR(ndev->name, "'bss %d' failed with %d\n", val, ret);

	return ret;
}

static int
wl_ext_interface_ops(struct net_device *dev,
	struct wl_apsta_params *apsta_params, int iftype, u8 *addr)
{
	s32 ret;
	struct wl_interface_create_v2 iface;
	wl_interface_create_v3_t iface_v3;
	struct wl_interface_info_v1 *info;
	wl_interface_info_v2_t *info_v2;
	uint32 ifflags = 0;
	bool use_iface_info_v2 = false;
	u8 ioctl_buf[WLC_IOCTL_SMLEN];

	/* Interface create */
	bzero(&iface, sizeof(iface));

	if (addr) {
		ifflags |= WL_INTERFACE_MAC_USE;
	}

	if (wlc_ver_major(dev) >= 5) {
		ret = wldev_iovar_getbuf(dev, "interface_create",
			&iface, sizeof(struct wl_interface_create_v2),
			ioctl_buf, sizeof(ioctl_buf), NULL);
		if ((ret == BCME_OK) && (*((uint32 *)ioctl_buf) == WL_INTERFACE_CREATE_VER_3)) {
			use_iface_info_v2 = true;
			bzero(&iface_v3, sizeof(wl_interface_create_v3_t));
			iface_v3.ver = WL_INTERFACE_CREATE_VER_3;
			iface_v3.iftype = iftype;
			iface_v3.flags = ifflags;
			if (addr) {
				memcpy(&iface_v3.mac_addr.octet, addr, ETH_ALEN);
			}
			ret = wl_ext_iovar_getbuf(dev, "interface_create",
				&iface_v3, sizeof(wl_interface_create_v3_t),
				ioctl_buf, sizeof(ioctl_buf), NULL);
			if (unlikely(ret)) {
				IAPSTA_ERROR(dev->name, "Interface v3 create failed!! ret %d\n", ret);
				return ret;
			}
		}
	}

	/* success case */
	if (use_iface_info_v2 == true) {
		info_v2 = (wl_interface_info_v2_t *)ioctl_buf;
		ret = info_v2->bsscfgidx;
	} else {
		/* Use v1 struct */
		iface.ver = WL_INTERFACE_CREATE_VER_2;
		iface.iftype = iftype;
		iface.flags = iftype | ifflags;
		if (addr) {
			memcpy(&iface.mac_addr.octet, addr, ETH_ALEN);
		}
		ret = wldev_iovar_getbuf(dev, "interface_create",
			&iface, sizeof(struct wl_interface_create_v2),
			ioctl_buf, sizeof(ioctl_buf), NULL);
		if (ret == BCME_OK) {
			info = (struct wl_interface_info_v1 *)ioctl_buf;
			ret = info->bsscfgidx;
		}
	}

	IAPSTA_INFO(dev->name, "wl interface create success!! bssidx:%d \n", ret);
	return ret;
}

static void
wl_ext_wait_netif_change(struct wl_apsta_params *apsta_params,
	struct wl_if_info *cur_if)
{
	rtnl_unlock();
	wait_event_interruptible_timeout(apsta_params->netif_change_event,
		wl_get_isam_status(cur_if, IF_READY),
		msecs_to_jiffies(MAX_AP_LINK_WAIT_TIME));
	rtnl_lock();
}

static void
wl_ext_interface_create(struct net_device *dev, struct wl_apsta_params *apsta_params,
	struct wl_if_info *cur_if, int iftype, u8 *addr)
{
	s32 ret;

	wl_set_isam_status(cur_if, IF_ADDING);
	ret = wl_ext_interface_ops(dev, apsta_params, iftype, addr);
	if (ret == BCME_UNSUPPORTED) {
		wl_ext_add_del_bss(dev, 1, iftype, 0, addr);
	}
	wl_ext_wait_netif_change(apsta_params, cur_if);
}

static void
wl_ext_iapsta_intf_add(struct net_device *dev, struct wl_apsta_params *apsta_params)
{
	apstamode_t apstamode = apsta_params->apstamode;
	struct wl_if_info *cur_if;
	s8 iovar_buf[WLC_IOCTL_SMLEN];
	wl_p2p_if_t ifreq;
	struct ether_addr mac_addr;

	bzero(&mac_addr, sizeof(mac_addr));

	if (apstamode == ISTAAP_MODE) {
		cur_if = &apsta_params->if_info[IF_VIF];
		wl_ext_interface_create(dev, apsta_params, cur_if, WL_INTERFACE_TYPE_AP, NULL);
	}
	else if (apstamode == ISTAGO_MODE) {
		bzero(&ifreq, sizeof(wl_p2p_if_t));
		ifreq.type = htod32(WL_P2P_IF_GO);
		cur_if = &apsta_params->if_info[IF_VIF];
		wl_set_isam_status(cur_if, IF_ADDING);
		wl_ext_iovar_setbuf(dev, "p2p_ifadd", &ifreq, sizeof(ifreq),
			iovar_buf, WLC_IOCTL_SMLEN, NULL);
		wl_ext_wait_netif_change(apsta_params, cur_if);
	}
	else if (apstamode == ISTASTA_MODE) {
		cur_if = &apsta_params->if_info[IF_VIF];
		memcpy(&mac_addr, dev->dev_addr, ETHER_ADDR_LEN);
		mac_addr.octet[0] |= 0x02;
		wl_ext_interface_create(dev, apsta_params, cur_if, WL_INTERFACE_TYPE_STA,
			(u8*)&mac_addr);
	}
	else if (apstamode == IDUALAP_MODE) {
		cur_if = &apsta_params->if_info[IF_VIF];
		wl_ext_interface_create(dev, apsta_params, cur_if, WL_INTERFACE_TYPE_AP, NULL);
	}
	else if (apstamode == ISTAAPAP_MODE) {
		u8 rand_bytes[2] = {0, };
		get_random_bytes(&rand_bytes, sizeof(rand_bytes));
		cur_if = &apsta_params->if_info[IF_VIF];
		memcpy(&mac_addr, dev->dev_addr, ETHER_ADDR_LEN);
		mac_addr.octet[0] |= 0x02;
		mac_addr.octet[5] += 0x01;
		memcpy(&mac_addr.octet[3], rand_bytes, sizeof(rand_bytes));
		wl_ext_interface_create(dev, apsta_params, cur_if, WL_INTERFACE_TYPE_AP,
			(u8*)&mac_addr);
		cur_if = &apsta_params->if_info[IF_VIF2];
		memcpy(&mac_addr, dev->dev_addr, ETHER_ADDR_LEN);
		mac_addr.octet[0] |= 0x02;
		mac_addr.octet[5] += 0x02;
		memcpy(&mac_addr.octet[3], rand_bytes, sizeof(rand_bytes));
		wl_ext_interface_create(dev, apsta_params, cur_if, WL_INTERFACE_TYPE_AP,
			(u8*)&mac_addr);
	}
#ifdef WLMESH
	else if (apstamode == ISTAMESH_MODE) {
		cur_if = &apsta_params->if_info[IF_VIF];
		wl_ext_interface_create(dev, apsta_params, cur_if, WL_INTERFACE_TYPE_STA, NULL);
	}
	else if (apstamode == IMESHAP_MODE) {
		cur_if = &apsta_params->if_info[IF_VIF];
		wl_ext_interface_create(dev, apsta_params, cur_if, WL_INTERFACE_TYPE_AP, NULL);
	}
	else if (apstamode == ISTAAPMESH_MODE) {
		cur_if = &apsta_params->if_info[IF_VIF];
		wl_ext_interface_create(dev, apsta_params, cur_if, WL_INTERFACE_TYPE_AP, NULL);
		cur_if = &apsta_params->if_info[IF_VIF2];
		wl_ext_interface_create(dev, apsta_params, cur_if, WL_INTERFACE_TYPE_STA, NULL);
	}
	else if (apstamode == IMESHAPAP_MODE) {
		cur_if = &apsta_params->if_info[IF_VIF];
		wl_ext_interface_create(dev, apsta_params, cur_if, WL_INTERFACE_TYPE_AP, NULL);
		cur_if = &apsta_params->if_info[IF_VIF2];
		wl_ext_interface_create(dev, apsta_params, cur_if, WL_INTERFACE_TYPE_AP, NULL);
	}
#endif /* WLMESH */
}
#endif /* !WL_STATIC_IF */

static bool
wl_ext_wait_other_enabling(struct wl_if_info *cur_if)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);
	struct wl_if_info *tmp_if;
	bool enabling = FALSE;
	u32 timeout = 1;
	int i;

	for (i=0; i<MAX_IF_NUM; i++) {
		tmp_if = &apsta_params->if_info[i];
		if (tmp_if->dev && tmp_if->dev != cur_if->dev) {
			if (tmp_if->ifmode == ISTA_MODE)
				enabling = wl_get_isam_status(tmp_if, STA_CONNECTING);
			else if (tmp_if->ifmode == IAP_MODE || tmp_if->ifmode == IMESH_MODE)
				enabling = wl_get_isam_status(tmp_if, AP_CREATING);
			if (enabling)
				WL_MSG(cur_if->ifname, "waiting for [%s] enabling...\n",
					tmp_if->ifname);
			if (enabling && tmp_if->ifmode == ISTA_MODE) {
				timeout = wait_event_interruptible_timeout(
					apsta_params->netif_change_event,
					!wl_get_isam_status(tmp_if, STA_CONNECTING),
					msecs_to_jiffies(MAX_STA_LINK_WAIT_TIME));
			} else if (enabling &&
					(tmp_if->ifmode == IAP_MODE || tmp_if->ifmode == IMESH_MODE)) {
				timeout = wait_event_interruptible_timeout(
					apsta_params->netif_change_event,
					!wl_get_isam_status(tmp_if, AP_CREATING),
					msecs_to_jiffies(MAX_STA_LINK_WAIT_TIME));
			}
			if (tmp_if->ifmode == ISTA_MODE)
				enabling = wl_get_isam_status(tmp_if, STA_CONNECTING);
			else if (tmp_if->ifmode == IAP_MODE || tmp_if->ifmode == IMESH_MODE)
				enabling = wl_get_isam_status(tmp_if, AP_CREATING);
			if (timeout <= 0 || enabling) {
				WL_MSG(cur_if->ifname, "[%s] is still enabling...\n",
					tmp_if->ifname);
			}
		}
	}

	return enabling;
}

static int
wl_ext_parse_wep(char *key, struct wl_wsec_key *wsec_key)
{
	char hex[] = "XX";
	unsigned char *data = wsec_key->data;
	char *keystr = key;

	switch (strlen(keystr)) {
	case 5:
	case 13:
	case 16:
		wsec_key->len = strlen(keystr);
		memcpy(data, keystr, wsec_key->len + 1);
		break;
	case 12:
	case 28:
	case 34:
	case 66:
		/* strip leading 0x */
		if (!strnicmp(keystr, "0x", 2))
			keystr += 2;
		else
			return -1;
		/* fall through */
		fallthrough;
	case 10:
	case 26:
	case 32:
	case 64:
		wsec_key->len = strlen(keystr) / 2;
		while (*keystr) {
			strncpy(hex, keystr, 2);
			*data++ = (char) strtoul(hex, NULL, 16);
			keystr += 2;
		}
		break;
	default:
		return -1;
	}

	switch (wsec_key->len) {
	case 5:
		wsec_key->algo = CRYPTO_ALGO_WEP1;
		break;
	case 13:
		wsec_key->algo = CRYPTO_ALGO_WEP128;
		break;
	case 16:
		/* default to AES-CCM */
		wsec_key->algo = CRYPTO_ALGO_AES_CCM;
		break;
	case 32:
		wsec_key->algo = CRYPTO_ALGO_TKIP;
		break;
	default:
		return -1;
	}

	/* Set as primary wsec_key by default */
	wsec_key->flags |= WL_PRIMARY_KEY;

	return 0;
}

static int
wl_ext_set_bgnmode(struct wl_if_info *cur_if)
{
	struct net_device *dev = cur_if->dev;
	bgnmode_t bgnmode = cur_if->bgnmode;
	int val;

	if (bgnmode == 0)
		return 0;

	wl_ext_ioctl(dev, WLC_DOWN, NULL, 0, 1);
	if (bgnmode == IEEE80211B) {
		wl_ext_iovar_setint(dev, "nmode", 0);
		val = 0;
		wl_ext_ioctl(dev, WLC_SET_GMODE, &val, sizeof(val), 1);
		IAPSTA_TRACE(dev->name, "Network mode: B only\n");
	} else if (bgnmode == IEEE80211G) {
		wl_ext_iovar_setint(dev, "nmode", 0);
		val = 2;
		wl_ext_ioctl(dev, WLC_SET_GMODE, &val, sizeof(val), 1);
		IAPSTA_TRACE(dev->name, "Network mode: G only\n");
	} else if (bgnmode == IEEE80211BG) {
		wl_ext_iovar_setint(dev, "nmode", 0);
		val = 1;
		wl_ext_ioctl(dev, WLC_SET_GMODE, &val, sizeof(val), 1);
		IAPSTA_TRACE(dev->name, "Network mode: B/G mixed\n");
	} else if (bgnmode == IEEE80211BGN) {
		wl_ext_iovar_setint(dev, "nmode", 0);
		wl_ext_iovar_setint(dev, "nmode", 1);
		wl_ext_iovar_setint(dev, "vhtmode", 0);
		val = 1;
		wl_ext_ioctl(dev, WLC_SET_GMODE, &val, sizeof(val), 1);
		IAPSTA_TRACE(dev->name, "Network mode: B/G/N mixed\n");
	} else if (bgnmode == IEEE80211BGNAC) {
		wl_ext_iovar_setint(dev, "nmode", 0);
		wl_ext_iovar_setint(dev, "nmode", 1);
		wl_ext_iovar_setint(dev, "vhtmode", 1);
		val = 1;
		wl_ext_ioctl(dev, WLC_SET_GMODE, &val, sizeof(val), 1);
		IAPSTA_TRACE(dev->name, "Network mode: B/G/N/AC mixed\n");
	}
	wl_ext_ioctl(dev, WLC_UP, NULL, 0, 1);

	return 0;
}

static int
wl_ext_set_amode(struct wl_if_info *cur_if)
{
	struct net_device *dev = cur_if->dev;
	authmode_t amode = cur_if->amode;
	int auth=0, wpa_auth=0;

#ifdef WLMESH
	if (cur_if->ifmode == IMESH_MODE) {
		if (amode == AUTH_SAE) {
			auth = WL_AUTH_OPEN_SYSTEM;
			wpa_auth = WPA2_AUTH_PSK;
			IAPSTA_INFO(dev->name, "SAE\n");
		} else {
			auth = WL_AUTH_OPEN_SYSTEM;
			wpa_auth = WPA_AUTH_DISABLED;
			IAPSTA_INFO(dev->name, "Open System\n");
		}
	} else
#endif /* WLMESH */
	if (amode == AUTH_OPEN) {
		auth = WL_AUTH_OPEN_SYSTEM;
		wpa_auth = WPA_AUTH_DISABLED;
		IAPSTA_INFO(dev->name, "Open System\n");
	} else if (amode == AUTH_SHARED) {
		auth = WL_AUTH_SHARED_KEY;
		wpa_auth = WPA_AUTH_DISABLED;
		IAPSTA_INFO(dev->name, "Shared Key\n");
	} else if (amode == AUTH_WPAPSK) {
		auth = WL_AUTH_OPEN_SYSTEM;
		wpa_auth = WPA_AUTH_PSK;
		IAPSTA_INFO(dev->name, "WPA-PSK\n");
	} else if (amode == AUTH_WPA2PSK) {
		auth = WL_AUTH_OPEN_SYSTEM;
		wpa_auth = WPA2_AUTH_PSK;
		IAPSTA_INFO(dev->name, "WPA2-PSK\n");
	} else if (amode == AUTH_WPAWPA2PSK) {
		auth = WL_AUTH_OPEN_SYSTEM;
		wpa_auth = WPA2_AUTH_PSK | WPA_AUTH_PSK;
		IAPSTA_INFO(dev->name, "WPA/WPA2-PSK\n");
	}
#ifdef WLMESH
	if (cur_if->ifmode == IMESH_MODE) {
		s32 val = WL_BSSTYPE_MESH;
		wl_ext_ioctl(dev, WLC_SET_INFRA, &val, sizeof(val), 1);
	} else
#endif /* WLMESH */
	if (cur_if->ifmode == ISTA_MODE) {
		s32 val = WL_BSSTYPE_INFRA;
		wl_ext_ioctl(dev, WLC_SET_INFRA, &val, sizeof(val), 1);
	}
	wl_ext_iovar_setint(dev, "auth", auth);

	wl_ext_iovar_setint(dev, "wpa_auth", wpa_auth);

	return 0;
}

static int
wl_ext_set_emode(struct wl_if_info *cur_if)
{
	struct net_device *dev = cur_if->dev;
	struct wl_wsec_key wsec_key;
	wsec_pmk_t psk;
	authmode_t amode = cur_if->amode;
	encmode_t emode = cur_if->emode;
	int wsec=0;
	char *key = cur_if->key;

	memset(&wsec_key, 0, sizeof(wsec_key));
	memset(&psk, 0, sizeof(psk));

#ifdef WLMESH
	if (cur_if->ifmode == IMESH_MODE) {
		if (amode == AUTH_SAE) {
			wsec = AES_ENABLED;
		} else {
			wsec = WSEC_NONE;
		}
	} else
#endif /* WLMESH */
	if (emode == ENC_NONE) {
		wsec = WSEC_NONE;
		IAPSTA_INFO(dev->name, "No securiy\n");
	} else if (emode == ENC_WEP) {
		wsec = WEP_ENABLED;
		wl_ext_parse_wep(key, &wsec_key);
		IAPSTA_INFO(dev->name, "WEP key \"%s\"\n", wsec_key.data);
	} else if (emode == ENC_TKIP) {
		wsec = TKIP_ENABLED;
		psk.key_len = strlen(key);
		psk.flags = WSEC_PASSPHRASE;
		memcpy(psk.key, key, strlen(key));
		IAPSTA_INFO(dev->name, "TKIP key \"%s\"\n", psk.key);
	} else if (emode == ENC_AES || amode == AUTH_SAE) {
		wsec = AES_ENABLED;
		psk.key_len = strlen(key);
		psk.flags = WSEC_PASSPHRASE;
		memcpy(psk.key, key, strlen(key));
		IAPSTA_INFO(dev->name, "AES key \"%s\"\n", psk.key);
	} else if (emode == ENC_TKIPAES) {
		wsec = TKIP_ENABLED | AES_ENABLED;
		psk.key_len = strlen(key);
		psk.flags = WSEC_PASSPHRASE;
		memcpy(psk.key, key, strlen(key));
		IAPSTA_INFO(dev->name, "TKIP/AES key \"%s\"\n", psk.key);
	}

	wl_ext_iovar_setint(dev, "wsec", wsec);

#ifdef WLMESH
	if (cur_if->ifmode == IMESH_MODE) {
		if (amode == AUTH_SAE) {
			s8 iovar_buf[WLC_IOCTL_SMLEN];
			IAPSTA_INFO(dev->name, "AES key \"%s\"\n", key);
			wl_ext_iovar_setint(dev, "mesh_auth_proto", 1);
			wl_ext_iovar_setint(dev, "mfp", WL_MFP_REQUIRED);
			wl_ext_iovar_setbuf(dev, "sae_password", key, strlen(key),
				iovar_buf, WLC_IOCTL_SMLEN, NULL);
		} else {
			IAPSTA_INFO(dev->name, "No securiy\n");
			wl_ext_iovar_setint(dev, "mesh_auth_proto", 0);
			wl_ext_iovar_setint(dev, "mfp", WL_MFP_NONE);
		}
	} else
#endif /* WLMESH */
	if (emode == ENC_WEP) {
		wl_ext_ioctl(dev, WLC_SET_KEY, &wsec_key, sizeof(wsec_key), 1);
	} else if (emode == ENC_TKIP || emode == ENC_AES || emode == ENC_TKIPAES) {
		if (cur_if->ifmode == ISTA_MODE)
			wl_ext_iovar_setint(dev, "sup_wpa", 1);
		wl_ext_ioctl(dev, WLC_SET_WSEC_PMK, &psk, sizeof(psk), 1);
	}

	return 0;
}

static int
wl_ext_parse_config(struct wl_if_info *cur_if, char *command, char **pick_next)
{
	char *pch, *pick_tmp;
	char name[20], data[100];
	int i, j, len;
	char *ifname_head = NULL;

	typedef struct config_map_t {
		char name[20];
		char *head;
		char *tail;
	} config_map_t;

	config_map_t config_map [] = {
		{" ifname ",	NULL, NULL},
		{" ssid ",		NULL, NULL},
		{" bssid ", 	NULL, NULL},
		{" bgnmode ",	NULL, NULL},
		{" hidden ",	NULL, NULL},
		{" maxassoc ",	NULL, NULL},
		{" band ",		NULL, NULL},
		{" chan ",		NULL, NULL},
		{" amode ", 	NULL, NULL},
		{" emode ", 	NULL, NULL},
		{" key ",		NULL, NULL},
	};
	config_map_t *row, *row_prev;

	pick_tmp = command;

	// reset head and tail
	for (i = 0; i < sizeof(config_map)/sizeof(config_map[0]); i++) {
		row = &config_map[i];
		row->head = NULL;
		row->tail = pick_tmp + strlen(pick_tmp);
	}

	// pick head
	for (i = 0; i < sizeof(config_map)/sizeof(config_map[0]); i++) {
		row = &config_map[i];
		pch = strstr(pick_tmp, row->name);
		if (pch) {
			row->head = pch;
		}
	}

	// sort by head
	for (i = 0; i < sizeof(config_map)/sizeof(config_map[0]) - 1; i++) {
		row_prev = &config_map[i];
		for (j = i+1; j < sizeof(config_map)/sizeof(config_map[0]); j++) {
			row = &config_map[j];
			if (row->head < row_prev->head) {
				strcpy(name, row_prev->name);
				strcpy(row_prev->name, row->name);
				strcpy(row->name, name);
				pch = row_prev->head;
				row_prev->head = row->head;
				row->head = pch;
			}
		}
	}

	// pick tail
	for (i = 0; i < sizeof(config_map)/sizeof(config_map[0]) - 1; i++) {
		row_prev = &config_map[i];
		row = &config_map[i+1];
		if (row_prev->head) {
			row_prev->tail = row->head;
		}
	}

	// remove name from head
	for (i = 0; i < sizeof(config_map)/sizeof(config_map[0]); i++) {
		row = &config_map[i];
		if (row->head) {
			if (!strcmp(row->name, " ifname ")) {
				ifname_head = row->head + 1;
				break;
			}
			row->head += strlen(row->name);
		}
	}

	for (i = 0; i < sizeof(config_map)/sizeof(config_map[0]); i++) {
		row = &config_map[i];
		if (row->head) {
			memset(data, 0, sizeof(data));
			if (row->tail && row->tail > row->head) {
				strncpy(data, row->head, row->tail-row->head);
			} else {
				strcpy(data, row->head);
			}
			pick_tmp = data;

			if (!strcmp(row->name, " ifname ")) {
				break;
			} else if (!strcmp(row->name, " ssid ")) {
				len = strlen(pick_tmp);
				memset(cur_if->ssid, 0, sizeof(cur_if->ssid));
				if (pick_tmp[0] == '"' && pick_tmp[len-1] == '"')
					strncpy(cur_if->ssid, &pick_tmp[1], len-2);
				else
					strcpy(cur_if->ssid, pick_tmp);
			} else if (!strcmp(row->name, " bssid ")) {
				pch = bcmstrtok(&pick_tmp, ": ", 0);
				for (j=0; j<6 && pch; j++) {
					((u8 *)&cur_if->bssid)[j] = (int)simple_strtol(pch, NULL, 16);
					pch = bcmstrtok(&pick_tmp, ": ", 0);
				}
			} else if (!strcmp(row->name, " bgnmode ")) {
				if (!strcmp(pick_tmp, "b"))
					cur_if->bgnmode = IEEE80211B;
				else if (!strcmp(pick_tmp, "g"))
					cur_if->bgnmode = IEEE80211G;
				else if (!strcmp(pick_tmp, "bg"))
					cur_if->bgnmode = IEEE80211BG;
				else if (!strcmp(pick_tmp, "bgn"))
					cur_if->bgnmode = IEEE80211BGN;
				else if (!strcmp(pick_tmp, "bgnac"))
					cur_if->bgnmode = IEEE80211BGNAC;
				else {
					IAPSTA_ERROR(cur_if->dev->name, "bgnmode [b|g|bg|bgn|bgnac]\n");
					return -1;
				}
			} else if (!strcmp(row->name, " hidden ")) {
				if (!strcmp(pick_tmp, "n"))
					cur_if->hidden = 0;
				else if (!strcmp(pick_tmp, "y"))
					cur_if->hidden = 1;
				else {
					IAPSTA_ERROR(cur_if->dev->name, "hidden [y|n]\n");
					return -1;
				}
			} else if (!strcmp(row->name, " maxassoc ")) {
				cur_if->maxassoc = (int)simple_strtol(pick_tmp, NULL, 10);
			} else if (!strcmp(row->name, " band ")) {
				if (!strcmp(pick_tmp, "2g"))
					cur_if->amode = WLC_BAND_2G;
				else if (!strcmp(pick_tmp, "5g"))
					cur_if->amode = WLC_BAND_5G;
#ifdef WL_6G_BAND
				else if (!strcmp(pick_tmp, "6g"))
					cur_if->amode = WLC_BAND_6G;
#endif /* WL_6G_BAND */
				else {
					IAPSTA_ERROR(cur_if->dev->name, "band [2g|5g|6g]\n");
					return -1;
				}
			} else if (!strcmp(row->name, " chan ")) {
				cur_if->chan_info.chan = (int)simple_strtol(pick_tmp, NULL, 10);
				cur_if->chan_info.band = WL_GET_BAND(cur_if->chan_info.chan);
			} else if (!strcmp(row->name, " amode ")) {
				if (!strcmp(pick_tmp, "open"))
					cur_if->amode = AUTH_OPEN;
				else if (!strcmp(pick_tmp, "shared"))
					cur_if->amode = AUTH_SHARED;
				else if (!strcmp(pick_tmp, "wpapsk"))
					cur_if->amode = AUTH_WPAPSK;
				else if (!strcmp(pick_tmp, "wpa2psk"))
					cur_if->amode = AUTH_WPA2PSK;
				else if (!strcmp(pick_tmp, "wpawpa2psk"))
					cur_if->amode = AUTH_WPAWPA2PSK;
				else if (!strcmp(pick_tmp, "sae"))
					cur_if->amode = AUTH_SAE;
				else {
					IAPSTA_ERROR(cur_if->dev->name, "amode [open|shared|wpapsk|wpa2psk|wpawpa2psk]\n");
					return -1;
				}
			} else if (!strcmp(row->name, " emode ")) {
				if (!strcmp(pick_tmp, "none"))
					cur_if->emode = ENC_NONE;
				else if (!strcmp(pick_tmp, "wep"))
					cur_if->emode = ENC_WEP;
				else if (!strcmp(pick_tmp, "tkip"))
					cur_if->emode = ENC_TKIP;
				else if (!strcmp(pick_tmp, "aes"))
					cur_if->emode = ENC_AES;
				else if (!strcmp(pick_tmp, "tkipaes"))
					cur_if->emode = ENC_TKIPAES;
				else {
					IAPSTA_ERROR(cur_if->dev->name, "emode [none|wep|tkip|aes|tkipaes]\n");
					return -1;
				}
			} else if (!strcmp(row->name, " key ")) {
				len = strlen(pick_tmp);
				memset(cur_if->key, 0, sizeof(cur_if->key));
				if (pick_tmp[0] == '"' && pick_tmp[len-1] == '"')
					strncpy(cur_if->key, &pick_tmp[1], len-2);
				else
					strcpy(cur_if->key, pick_tmp);
			}
		}
	}

	*pick_next = ifname_head;
	return 0;
}

static void
wl_ext_iapsta_preinit(struct net_device *dev, struct wl_apsta_params *apsta_params)
{
	struct dhd_pub *dhd;
	apstamode_t apstamode = apsta_params->apstamode;
	struct wl_if_info *cur_if;
	s8 iovar_buf[WLC_IOCTL_SMLEN];
	s32 val = 0;
	int i;

	dhd = dhd_get_pub(dev);

	for (i=0; i<MAX_IF_NUM; i++) {
		cur_if = &apsta_params->if_info[i];
		if (i >= 1 && !strlen(cur_if->ifname))
			snprintf(cur_if->ifname, IFNAMSIZ, "wlan%d", i);
		if (cur_if->ifmode == ISTA_MODE) {
			wl_ext_set_chan_info(cur_if, WLC_BAND_2G, 0);
			cur_if->maxassoc = -1;
			cur_if->prio = PRIO_STA;
			snprintf(cur_if->ssid, DOT11_MAX_SSID_LEN, "ttt_sta");
		} else if (cur_if->ifmode == IAP_MODE) {
			wl_ext_set_chan_info(cur_if, WLC_BAND_2G, 1);
			cur_if->maxassoc = -1;
			cur_if->prio = PRIO_AP;
			snprintf(cur_if->ssid, DOT11_MAX_SSID_LEN, "ttt_ap");
#ifdef WLMESH
		} else if (cur_if->ifmode == IMESH_MODE) {
			wl_ext_set_chan_info(cur_if, WLC_BAND_2G, 1);
			cur_if->maxassoc = -1;
			cur_if->prio = PRIO_MESH;
			snprintf(cur_if->ssid, DOT11_MAX_SSID_LEN, "ttt_mesh");
#endif /* WLMESH */
		}
	}

	if (FW_SUPPORTED(dhd, rsdb)) {
		if (apstamode == IDUALAP_MODE)
			apsta_params->rsdb = TRUE;
		else if (apstamode == ISTAAPAP_MODE)
			apsta_params->rsdb = FALSE;
		if (apstamode == ISTAAPAP_MODE || apstamode == IDUALAP_MODE ||
				apstamode == IMESHONLY_MODE || apstamode == ISTAMESH_MODE ||
				apstamode == IMESHAP_MODE || apstamode == ISTAAPMESH_MODE ||
				apstamode == IMESHAPAP_MODE) {
			wl_config_t rsdb_mode_cfg = {0, 0};
			rsdb_mode_cfg.config = apsta_params->rsdb ? 1 : 0;
			IAPSTA_INFO(dev->name, "set rsdb_mode %d\n", rsdb_mode_cfg.config);
			wl_ext_ioctl(dev, WLC_DOWN, NULL, 0, 1);
			wl_ext_iovar_setbuf(dev, "rsdb_mode", &rsdb_mode_cfg,
				sizeof(rsdb_mode_cfg), iovar_buf, sizeof(iovar_buf), NULL);
			wl_ext_ioctl(dev, WLC_UP, NULL, 0, 1);
		}
	} else {
		apsta_params->rsdb = FALSE;
	}

	if (apstamode == ISTAONLY_MODE) {
		wl_ext_ioctl(dev, WLC_DOWN, NULL, 0, 1);
		wl_ext_iovar_setint(dev, "apsta", 1); // keep 1 as we set in dhd_preinit_ioctls
		// don't set WLC_SET_AP to 0, some parameters will be reset, such as bcn_timeout and roam_off
		wl_ext_ioctl(dev, WLC_UP, NULL, 0, 1);
	} else if (apstamode == IAPONLY_MODE) {
		wl_ext_ioctl(dev, WLC_DOWN, NULL, 0, 1);
#ifdef ARP_OFFLOAD_SUPPORT
		/* IF SoftAP is enabled, disable arpoe */
		dhd_arp_offload_set(dhd, 0);
		dhd_arp_offload_enable(dhd, FALSE);
#endif /* ARP_OFFLOAD_SUPPORT */
		wl_ext_iovar_setint(dev, "mpc", 0);
		wl_ext_iovar_setint(dev, "apsta", 0);
		val = 1;
		wl_ext_ioctl(dev, WLC_SET_AP, &val, sizeof(val), 1);
#ifdef PROP_TXSTATUS_VSDB
#if defined(BCMSDIO)
		if (!(FW_SUPPORTED(dhd, rsdb)) && !disable_proptx) {
			bool enabled;
			dhd_wlfc_get_enable(dhd, &enabled);
			if (!enabled) {
				dhd_wlfc_init(dhd);
				wl_ext_ioctl(dev, WLC_UP, NULL, 0, 1);
			}
		}
#endif /* BCMSDIO */
#endif /* PROP_TXSTATUS_VSDB */
	}
	else if (apstamode == ISTAAP_MODE) {
		wl_ext_ioctl(dev, WLC_DOWN, NULL, 0, 1);
		wl_ext_iovar_setint(dev, "mpc", 0);
		wl_ext_iovar_setint(dev, "apsta", 1);
		wl_ext_ioctl(dev, WLC_UP, NULL, 0, 1);
	}
	else if (apstamode == ISTAGO_MODE) {
		wl_ext_ioctl(dev, WLC_DOWN, NULL, 0, 1);
		wl_ext_iovar_setint(dev, "apsta", 1);
		wl_ext_ioctl(dev, WLC_UP, NULL, 0, 1);
	}
	else if (apstamode == ISTASTA_MODE) {
	}
	else if (apstamode == IDUALAP_MODE) {
		wl_ext_ioctl(dev, WLC_DOWN, NULL, 0, 1);
		/* IF SoftAP is enabled, disable arpoe or wlan1 will ping fail */
#ifdef ARP_OFFLOAD_SUPPORT
		/* IF SoftAP is enabled, disable arpoe */
		dhd_arp_offload_set(dhd, 0);
		dhd_arp_offload_enable(dhd, FALSE);
#endif /* ARP_OFFLOAD_SUPPORT */
		wl_ext_iovar_setint(dev, "mpc", 0);
		wl_ext_iovar_setint(dev, "mbcn", 1);
		wl_ext_iovar_setint(dev, "apsta", 0);
		wl_ext_ioctl(dev, WLC_UP, NULL, 0, 1);
		val = 1;
		wl_ext_ioctl(dev, WLC_SET_AP, &val, sizeof(val), 1);
	}
	else if (apstamode == ISTAAPAP_MODE) {
		wl_ext_ioctl(dev, WLC_DOWN, NULL, 0, 1);
		wl_ext_iovar_setint(dev, "mpc", 0);
		wl_ext_iovar_setint(dev, "mbss", 1);
		wl_ext_iovar_setint(dev, "apsta", 1); // keep 1 as we set in dhd_preinit_ioctls
		wl_ext_ioctl(dev, WLC_UP, NULL, 0, 1);
		// don't set WLC_SET_AP to 0, some parameters will be reset, such as bcn_timeout and roam_off
	}
#ifdef WLMESH
	else if (apstamode == IMESHONLY_MODE || apstamode == ISTAMESH_MODE ||
			apstamode == IMESHAP_MODE || apstamode == ISTAAPMESH_MODE ||
			apstamode == IMESHAPAP_MODE) {
		int pm = 0;
		wl_ext_ioctl(dev, WLC_DOWN, NULL, 0, 1);
		wl_ext_iovar_setint(dev, "mpc", 0);
		if (apstamode == IMESHONLY_MODE)
			wl_ext_ioctl(dev, WLC_SET_PM, &pm, sizeof(pm), 1);
		else
			wl_ext_iovar_setint(dev, "mbcn", 1);
		wl_ext_iovar_setint(dev, "apsta", 1); // keep 1 as we set in dhd_preinit_ioctls
		wl_ext_ioctl(dev, WLC_UP, NULL, 0, 1);
		// don't set WLC_SET_AP to 0, some parameters will be reset, such as bcn_timeout and roam_off
	}
#endif /* WLMESH */

	apsta_params->init = TRUE;

	WL_MSG(dev->name, "apstamode=%d\n", apstamode);
}

static int
wl_ext_disable_iface(struct net_device *dev, char *ifname)
{
	struct dhd_pub *dhd = dhd_get_pub(dev);
	int i;
	s8 iovar_buf[WLC_IOCTL_SMLEN];
	wlc_ssid_t ssid = { 0, {0} };
	scb_val_t scbval;
	struct {
		s32 cfg;
		s32 val;
	} bss_setbuf;
	struct wl_apsta_params *apsta_params = dhd->iapsta_params;
	apstamode_t apstamode = apsta_params->apstamode;
	struct wl_if_info *cur_if = NULL, *tmp_if = NULL;

	for (i=0; i<MAX_IF_NUM; i++) {
		tmp_if = &apsta_params->if_info[i];
		if (tmp_if->dev && !strcmp(tmp_if->dev->name, ifname)) {
			cur_if = tmp_if;
			break;
		}
	}
	if (!cur_if) {
		IAPSTA_ERROR(dev->name, "wrong ifname=%s or dev not ready\n", ifname);
		return -1;
	}

	mutex_lock(&apsta_params->usr_sync);
	WL_MSG(ifname, "Disabling...\n");

	if (cur_if->ifmode == ISTA_MODE) {
		wl_ext_ioctl(cur_if->dev, WLC_DISASSOC, NULL, 0, 1);
		wl_ext_add_remove_pm_enable_work(dev, FALSE);
	} else if (cur_if->ifmode == IAP_MODE || cur_if->ifmode == IMESH_MODE) {
		// deauthenticate all STA first
		memcpy(scbval.ea.octet, &ether_bcast, ETHER_ADDR_LEN);
		wl_ext_ioctl(cur_if->dev, WLC_SCB_DEAUTHENTICATE, &scbval.ea, ETHER_ADDR_LEN, 1);
	}

	if (apstamode == IAPONLY_MODE || apstamode == IMESHONLY_MODE) {
		wl_ext_ioctl(dev, WLC_DOWN, NULL, 0, 1);
		wl_ext_ioctl(dev, WLC_SET_SSID, &ssid, sizeof(ssid), 1); // reset ssid
		wl_ext_iovar_setint(dev, "mpc", 1);
	} else if ((apstamode==ISTAAP_MODE || apstamode==ISTAGO_MODE) &&
			cur_if->ifmode == IAP_MODE) {
		bss_setbuf.cfg = 0xffffffff;
		bss_setbuf.val = htod32(0);
		wl_ext_iovar_setbuf(cur_if->dev, "bss", &bss_setbuf, sizeof(bss_setbuf),
			iovar_buf, WLC_IOCTL_SMLEN, NULL);
		wl_ext_iovar_setint(dev, "mpc", 1);
#ifdef ARP_OFFLOAD_SUPPORT
		/* IF SoftAP is disabled, enable arpoe back for STA mode. */
		dhd_arp_offload_set(dhd, dhd_arp_mode);
		dhd_arp_offload_enable(dhd, TRUE);
#endif /* ARP_OFFLOAD_SUPPORT */
#ifdef PROP_TXSTATUS_VSDB
#if defined(BCMSDIO)
		if (dhd->conf->disable_proptx!=0) {
			bool enabled;
			dhd_wlfc_get_enable(dhd, &enabled);
			if (enabled) {
				dhd_wlfc_deinit(dhd);
			}
		}
#endif /* BCMSDIO */
#endif /* PROP_TXSTATUS_VSDB */
	}
	else if (apstamode == IDUALAP_MODE || apstamode == ISTAAPAP_MODE) {
		bss_setbuf.cfg = 0xffffffff;
		bss_setbuf.val = htod32(0);
		wl_ext_iovar_setbuf(cur_if->dev, "bss", &bss_setbuf, sizeof(bss_setbuf),
			iovar_buf, WLC_IOCTL_SMLEN, NULL);
#ifdef WLMESH
	} else if (apstamode == ISTAMESH_MODE || apstamode == IMESHAP_MODE ||
			apstamode == ISTAAPMESH_MODE || apstamode == IMESHAPAP_MODE) {
		bss_setbuf.cfg = 0xffffffff;
		bss_setbuf.val = htod32(0);
		wl_ext_iovar_setbuf(cur_if->dev, "bss", &bss_setbuf, sizeof(bss_setbuf),
			iovar_buf, WLC_IOCTL_SMLEN, NULL);
		if (cur_if->ifmode == IMESH_MODE) {
			int scan_assoc_time = DHD_SCAN_ASSOC_ACTIVE_TIME;
			for (i=0; i<MAX_IF_NUM; i++) {
				tmp_if = &apsta_params->if_info[i];
				if (tmp_if->dev && tmp_if->ifmode == ISTA_MODE) {
					wl_ext_ioctl(tmp_if->dev, WLC_SET_SCAN_CHANNEL_TIME,
						&scan_assoc_time, sizeof(scan_assoc_time), 1);
				}
			}
		}
#endif /* WLMESH */
	}

	wl_clr_isam_status(cur_if, AP_CREATED);

	WL_MSG(ifname, "Exit\n");
	mutex_unlock(&apsta_params->usr_sync);
	return 0;
}

static int
wl_ext_enable_iface(struct net_device *dev, char *ifname, int wait_up, bool lock)
{
	struct dhd_pub *dhd = dhd_get_pub(dev);
	int i, ret = 0;
	s8 iovar_buf[WLC_IOCTL_SMLEN];
	wlc_ssid_t ssid = { 0, {0} };
	chanspec_t fw_chspec;
	struct {
		s32 cfg;
		s32 val;
	} bss_setbuf;
	struct wl_apsta_params *apsta_params = dhd->iapsta_params;
	apstamode_t apstamode = apsta_params->apstamode;
	struct wl_if_info *cur_if = NULL, *tmp_if = NULL;
	struct wl_ext_chan_info chan_info;
	struct wl_conn_info conn_info;
	u32 timeout;

	for (i=0; i<MAX_IF_NUM; i++) {
		tmp_if = &apsta_params->if_info[i];
		if (tmp_if->dev && !strcmp(tmp_if->dev->name, ifname)) {
			cur_if = tmp_if;
			break;
		}
	}
	if (!cur_if) {
		IAPSTA_ERROR(dev->name, "wrong ifname=%s or dev not ready\n", ifname);
		return -1;
	}

	if (lock)
		mutex_lock(&apsta_params->usr_sync);

	if (cur_if->ifmode == ISTA_MODE) {
		wl_set_isam_status(cur_if, STA_CONNECTING);
	} else if (cur_if->ifmode == IAP_MODE || cur_if->ifmode == IMESH_MODE) {
		wl_set_isam_status(cur_if, AP_CREATING);
	}

	wl_ext_isam_status(cur_if->dev, NULL, 0);
	WL_MSG(ifname, "Enabling...\n");

	wl_ext_wait_other_enabling(cur_if);

	memset(&conn_info, 0, sizeof(struct wl_conn_info));
	if (wl_ext_master_if(cur_if) && apsta_params->acs) {
		uint16 chan_2g, chan_5g, chan_6g;
		wl_ext_get_default_chan(cur_if->dev, &chan_2g, &chan_5g, &chan_6g);
		if ((chan_2g && cur_if->chan_info.band == WLC_BAND_2G) ||
				(chan_5g && cur_if->chan_info.band == WLC_BAND_5G) ||
				(chan_6g && cur_if->chan_info.band == WLC_BAND_6G)) {
			cur_if->chan_info.chan = wl_ext_autochannel(cur_if->dev, ACS_DRV_BIT,
				cur_if->chan_info.band, NULL);
			cur_if->chan_info.chan = wf_chspec_ctlchan(cur_if->chan_info.chan);
		} else {
			IAPSTA_ERROR(ifname, "invalid channel\n");
			ret = -1;
			goto exit;
		}
	}

	wl_ext_move_cur_channel(cur_if);

	if (wl_ext_master_if(cur_if) && !cur_if->chan_info.chan) {
		IAPSTA_ERROR(ifname, "skip channel 0\n");
		ret = -1;
		goto exit;
	}

	if (wl_ext_get_chanspec(cur_if->dev, &chan_info)) {
		IAPSTA_INFO(cur_if->ifname, "Associated\n");
		if (!wl_ext_same_chan(&cur_if->chan_info, &chan_info)) {
			wl_ext_trigger_csa(cur_if);
		}
		goto exit;
	}
	if (cur_if->ifmode == ISTA_MODE) {
		wl_clr_isam_status(cur_if, STA_CONNECTED);
	} else if (cur_if->ifmode == IAP_MODE || cur_if->ifmode == IMESH_MODE) {
		wl_clr_isam_status(cur_if, AP_CREATED);
	}

	wl_ext_move_other_channel(cur_if);

	if (cur_if->ifidx > 0) {
		wl_ext_iovar_setbuf(cur_if->dev, "cur_etheraddr", (u8 *)cur_if->dev->dev_addr,
			ETHER_ADDR_LEN, iovar_buf, WLC_IOCTL_SMLEN, NULL);
	}

	// set ssid for AP
	ssid.SSID_len = strlen(cur_if->ssid);
	memcpy(ssid.SSID, cur_if->ssid, ssid.SSID_len);
	if (cur_if->ifmode == IAP_MODE || cur_if->ifmode == IMESH_MODE) {
		wl_ext_iovar_setint(dev, "mpc", 0);
		if (apstamode == IAPONLY_MODE || apstamode == IMESHONLY_MODE) {
			wl_ext_ioctl(dev, WLC_UP, NULL, 0, 1);
		} else if (apstamode==ISTAAP_MODE || apstamode==ISTAGO_MODE) {
			wl_ext_iovar_setbuf_bsscfg(cur_if->dev, "ssid", &ssid, sizeof(ssid),
				iovar_buf, WLC_IOCTL_SMLEN, cur_if->bssidx, NULL);
		}
	}

	if (wl_ext_master_if(cur_if)) {
		wl_ext_set_bgnmode(cur_if);
		if (!cur_if->chan_info.chan) {
			wl_ext_set_chan_info(cur_if, WLC_BAND_2G, 1);
		}
		ret = wl_ext_set_chanspec(cur_if->dev, &cur_if->chan_info, &fw_chspec);
		if (ret)
			goto exit;
	}

	wl_ext_set_amode(cur_if);
	wl_ext_set_emode(cur_if);

	if (cur_if->ifmode == ISTA_MODE) {
		conn_info.bssidx = cur_if->bssidx;
		conn_info.channel = cur_if->chan_info.chan;
		memcpy(conn_info.ssid.SSID, cur_if->ssid, strlen(cur_if->ssid));
		conn_info.ssid.SSID_len = strlen(cur_if->ssid);
		memcpy(&conn_info.bssid, &cur_if->bssid, ETHER_ADDR_LEN);
	}
	if (cur_if->ifmode == IAP_MODE) {
		if (cur_if->maxassoc >= 0)
			wl_ext_iovar_setint(dev, "maxassoc", cur_if->maxassoc);
		// terence: fix me, hidden does not work in dualAP mode
		if (cur_if->hidden > 0) {
			wl_ext_ioctl(cur_if->dev, WLC_SET_CLOSED, &cur_if->hidden,
				sizeof(cur_if->hidden), 1);
			WL_MSG(ifname, "Broadcast SSID: %s\n", cur_if->hidden ? "OFF":"ON");
		}
	}

	if (apstamode == ISTAONLY_MODE) {
		wl_ext_connect(cur_if->dev, &conn_info);
	} else if (apstamode == IAPONLY_MODE) {
		wl_ext_ioctl(cur_if->dev, WLC_SET_SSID, &ssid, sizeof(ssid), 1);
		wl_ext_ioctl(dev, WLC_UP, NULL, 0, 1);
	} else if (apstamode == ISTAAP_MODE || apstamode == ISTAGO_MODE) {
		if (cur_if->ifmode == ISTA_MODE) {
			wl_ext_connect(cur_if->dev, &conn_info);
		} else {
			if (FW_SUPPORTED(dhd, rsdb)) {
				wl_ext_ioctl(cur_if->dev, WLC_SET_SSID, &ssid, sizeof(ssid), 1);
			} else {
				bss_setbuf.cfg = htod32(cur_if->bssidx);
				bss_setbuf.val = htod32(1);
				wl_ext_iovar_setbuf(cur_if->dev, "bss", &bss_setbuf,
					sizeof(bss_setbuf), iovar_buf, WLC_IOCTL_SMLEN, NULL);
			}
#ifdef ARP_OFFLOAD_SUPPORT
			/* IF SoftAP is enabled, disable arpoe */
			dhd_arp_offload_set(dhd, 0);
			dhd_arp_offload_enable(dhd, FALSE);
#endif /* ARP_OFFLOAD_SUPPORT */
#ifdef PROP_TXSTATUS_VSDB
#if defined(BCMSDIO)
			if (!(FW_SUPPORTED(dhd, rsdb)) && !disable_proptx) {
				bool enabled;
				dhd_wlfc_get_enable(dhd, &enabled);
				if (!enabled) {
					dhd_wlfc_init(dhd);
					wl_ext_ioctl(dev, WLC_UP, NULL, 0, 1);
				}
			}
#endif /* BCMSDIO */
#endif /* PROP_TXSTATUS_VSDB */
		}
	}
	else if (apstamode == IDUALAP_MODE) {
		wl_ext_ioctl(cur_if->dev, WLC_SET_SSID, &ssid, sizeof(ssid), 1);
	} else if (apstamode == ISTAAPAP_MODE) {
		if (cur_if->ifmode == ISTA_MODE) {
			wl_ext_connect(cur_if->dev, &conn_info);
		} else if (cur_if->ifmode == IAP_MODE) {
			wl_ext_ioctl(cur_if->dev, WLC_SET_SSID, &ssid, sizeof(ssid), 1);
		} else {
			IAPSTA_ERROR(cur_if->ifname, "wrong ifmode %d\n", cur_if->ifmode);
		}
#ifdef WLMESH
	} else if (apstamode == IMESHONLY_MODE ||
			apstamode == ISTAMESH_MODE || apstamode == IMESHAP_MODE ||
			apstamode == ISTAAPMESH_MODE || apstamode == IMESHAPAP_MODE) {
		if (cur_if->ifmode == ISTA_MODE) {
			wl_ext_connect(cur_if->dev, &conn_info);
		} else if (cur_if->ifmode == IAP_MODE) {
			wl_ext_ioctl(cur_if->dev, WLC_SET_SSID, &ssid, sizeof(ssid), 1);
		} else if (cur_if->ifmode == IMESH_MODE) {
			struct wl_join_params join_params;
			// need to up before setting ssid
			memset(&join_params, 0, sizeof(join_params));
			join_params.ssid.SSID_len = strlen(cur_if->ssid);
			memcpy((void *)join_params.ssid.SSID, cur_if->ssid, strlen(cur_if->ssid));
			join_params.params.chanspec_list[0] = fw_chspec;
			join_params.params.chanspec_num = 1;
			wl_ext_ioctl(cur_if->dev, WLC_SET_SSID, &join_params, sizeof(join_params), 1);
		} else {
			IAPSTA_ERROR(cur_if->ifname, "wrong ifmode %d\n", cur_if->ifmode);
		}
#endif /* WLMESH */
	}

	if (wait_up) {
		OSL_SLEEP(wait_up);
	} else if (cur_if->ifmode == IAP_MODE || cur_if->ifmode == IMESH_MODE) {
		timeout = wait_event_interruptible_timeout(apsta_params->netif_change_event,
			wl_get_isam_status(cur_if, AP_CREATED),
			msecs_to_jiffies(MAX_AP_LINK_WAIT_TIME));
		if (timeout <= 0 || !wl_get_isam_status(cur_if, AP_CREATED)) {
			if (lock)
				mutex_unlock(&apsta_params->usr_sync);
			wl_ext_disable_iface(dev, cur_if->ifname);
			WL_MSG(ifname, "failed to enable with SSID: \"%s\"\n", cur_if->ssid);
			ret = -1;
		}
	}

	if (wl_get_isam_status(cur_if, AP_CREATED) &&
			(cur_if->ifmode == IMESH_MODE || cur_if->ifmode == IAP_MODE) &&
			(apstamode == ISTAAP_MODE || apstamode == ISTAAPAP_MODE ||
			apstamode == ISTAMESH_MODE || apstamode == IMESHAP_MODE ||
			apstamode == ISTAAPMESH_MODE || apstamode == IMESHAPAP_MODE)) {
		wl_ext_set_scan_time(cur_if->dev, 80,
			WLC_GET_SCAN_CHANNEL_TIME, WLC_SET_SCAN_CHANNEL_TIME);
	}

	wl_ext_isam_status(cur_if->dev, NULL, 0);

exit:
	if (cur_if->ifmode == IAP_MODE || cur_if->ifmode == IMESH_MODE) {
		wl_clr_isam_status(cur_if, AP_CREATING);
	}
	WL_MSG(ifname, "Exit ret=%d\n", ret);
	if (lock)
		mutex_unlock(&apsta_params->usr_sync);
	return ret;
}

int
wl_ext_iapsta_disable(struct net_device *dev, char *command, int total_len)
{
	int ret = 0;
	char *pch, *pick_tmp, *param;
	char ifname[IFNAMSIZ+1];

	IAPSTA_TRACE(dev->name, "command=%s, len=%d\n", command, total_len);

	pick_tmp = command;
	param = bcmstrtok(&pick_tmp, " ", 0); // skip iapsta_disable
	param = bcmstrtok(&pick_tmp, " ", 0);
	while (param != NULL) {
		if (!strcmp(param, "ifname")) {
			pch = bcmstrtok(&pick_tmp, " ", 0);
			if (pch) {
				strcpy(ifname, pch);
				ret = wl_ext_disable_iface(dev, ifname);
				if (ret)
					return ret;
			}
			else {
				IAPSTA_ERROR(dev->name, "ifname [wlanX]\n");
				return -1;
			}
		}
		param = bcmstrtok(&pick_tmp, " ", 0);
	}

	return ret;
}

int
wl_ext_iapsta_enable(struct net_device *dev, char *command, int total_len)
{
	int ret = 0;
	char *pch, *pick_tmp, *param;
	char ifname[IFNAMSIZ+1];

	IAPSTA_TRACE(dev->name, "command=%s, len=%d\n", command, total_len);

	pick_tmp = command;
	param = bcmstrtok(&pick_tmp, " ", 0); // skip iapsta_enable
	param = bcmstrtok(&pick_tmp, " ", 0);
	while (param != NULL) {
		if (!strcmp(param, "ifname")) {
			pch = bcmstrtok(&pick_tmp, " ", 0);
			if (pch) {
				strcpy(ifname, pch);
				ret = wl_ext_enable_iface(dev, ifname, 0, TRUE);
				if (ret)
					return ret;
			} else {
				IAPSTA_ERROR(dev->name, "ifname [wlanX]\n");
				return -1;
			}
		}
		param = bcmstrtok(&pick_tmp, " ", 0);
	}

	return ret;
}

int
wl_ext_iapsta_config(struct net_device *dev, char *command, int total_len)
{
	struct dhd_pub *dhd = dhd_get_pub(dev);
	int ret=0, i;
	char *pch, *pch2, *pick_tmp, *pick_next=NULL;
	struct wl_apsta_params *apsta_params = dhd->iapsta_params;
	char ifname[IFNAMSIZ+1];
	struct wl_if_info *cur_if = NULL, *tmp_if = NULL;

	if (!apsta_params->init) {
		IAPSTA_ERROR(dev->name, "please init first\n");
		return -1;
	}

	IAPSTA_TRACE(dev->name, "command=%s, len=%d\n", command, total_len);

	pick_tmp = command;
	bcmstrtok(&pick_tmp, " ", 0); // skip iapsta_config

	mutex_lock(&apsta_params->usr_sync);

	while (pick_tmp != NULL) {
		memset(ifname, 0, IFNAMSIZ+1);
		if (!strncmp(pick_tmp, "ifname ", strlen("ifname "))) {
			pch = pick_tmp + strlen("ifname ");
			pch2 = strchr(pch, ' ');
			if (pch && pch2) {
				strncpy(ifname, pch, pch2-pch);
			} else {
				IAPSTA_ERROR(dev->name, "ifname [wlanX]\n");
				ret = -1;
				break;
			}
			for (i=0; i<MAX_IF_NUM; i++) {
				tmp_if = &apsta_params->if_info[i];
				if (tmp_if->dev && !strcmp(tmp_if->dev->name, ifname)) {
					cur_if = tmp_if;
					break;
				}
			}
			if (!cur_if) {
				IAPSTA_ERROR(dev->name, "wrong ifname=%s in apstamode=%d\n",
					ifname, apsta_params->apstamode);
				ret = -1;
				break;
			}
			ret = wl_ext_parse_config(cur_if, pick_tmp, &pick_next);
			if (ret)
				break;
			pick_tmp = pick_next;
		} else {
			IAPSTA_ERROR(dev->name, "first arg must be ifname\n");
			ret = -1;
			break;
		}

	}

	mutex_unlock(&apsta_params->usr_sync);

	return ret;
}

int
wl_ext_isam_init(struct net_device *dev, char *command, int total_len)
{
	struct dhd_pub *dhd = dhd_get_pub(dev);
	char *pch, *pick_tmp, *pick_tmp2, *param;
	struct wl_apsta_params *apsta_params = dhd->iapsta_params;
	int i;

	if (apsta_params->init) {
		IAPSTA_ERROR(dev->name, "don't init twice\n");
		return -1;
	}
	IAPSTA_TRACE(dev->name, "command=%s, len=%d\n", command, total_len);

	pick_tmp = command;
	param = bcmstrtok(&pick_tmp, " ", 0); // skip iapsta_init
	param = bcmstrtok(&pick_tmp, " ", 0);
	while (param != NULL) {
		pick_tmp2 = bcmstrtok(&pick_tmp, " ", 0);
		if (!pick_tmp2) {
			IAPSTA_ERROR(dev->name, "wrong param %s\n", param);
			return -1;
		}
		if (!strcmp(param, "mode")) {
			pch = NULL;
			if (!strcmp(pick_tmp2, "sta")) {
				apsta_params->apstamode = ISTAONLY_MODE;
			} else if (!strcmp(pick_tmp2, "ap")) {
				apsta_params->apstamode = IAPONLY_MODE;
			} else if (!strcmp(pick_tmp2, "sta-ap")) {
				apsta_params->apstamode = ISTAAP_MODE;
			} else if (!strcmp(pick_tmp2, "sta-sta")) {
				apsta_params->apstamode = ISTASTA_MODE;
				apsta_params->mcc = TRUE;
			} else if (!strcmp(pick_tmp2, "ap-ap")) {
				apsta_params->apstamode = IDUALAP_MODE;
			} else if (!strcmp(pick_tmp2, "sta-ap-ap")) {
				apsta_params->apstamode = ISTAAPAP_MODE;
			} else if (!strcmp(pick_tmp2, "apsta")) {
				apsta_params->apstamode = ISTAAP_MODE;
				apsta_params->if_info[IF_PIF].ifmode = ISTA_MODE;
				apsta_params->if_info[IF_VIF].ifmode = IAP_MODE;
			} else if (!strcmp(pick_tmp2, "dualap")) {
				apsta_params->apstamode = IDUALAP_MODE;
				apsta_params->if_info[IF_PIF].ifmode = IAP_MODE;
				apsta_params->if_info[IF_VIF].ifmode = IAP_MODE;
			} else if (!strcmp(pick_tmp2, "sta-go") ||
					!strcmp(pick_tmp2, "gosta")) {
				if (!FW_SUPPORTED(dhd, p2p)) {
					return -1;
				}
				apsta_params->apstamode = ISTAGO_MODE;
				apsta_params->if_info[IF_PIF].ifmode = ISTA_MODE;
				apsta_params->if_info[IF_VIF].ifmode = IAP_MODE;
#ifdef WLMESH
			} else if (!strcmp(pick_tmp2, "mesh")) {
				apsta_params->apstamode = IMESHONLY_MODE;
			} else if (!strcmp(pick_tmp2, "sta-mesh")) {
				apsta_params->apstamode = ISTAMESH_MODE;
			} else if (!strcmp(pick_tmp2, "sta-ap-mesh")) {
				apsta_params->apstamode = ISTAAPMESH_MODE;
			} else if (!strcmp(pick_tmp2, "mesh-ap")) {
				apsta_params->apstamode = IMESHAP_MODE;
			} else if (!strcmp(pick_tmp2, "mesh-ap-ap")) {
				apsta_params->apstamode = IMESHAPAP_MODE;
#endif /* WLMESH */
			} else {
				IAPSTA_ERROR(dev->name, "mode [sta|ap|sta-ap|ap-ap]\n");
				return -1;
			}
			pch = bcmstrtok(&pick_tmp2, " -", 0);
			for (i=0; i<MAX_IF_NUM && pch; i++) {
				if (!strcmp(pch, "sta"))
					apsta_params->if_info[i].ifmode = ISTA_MODE;
				else if (!strcmp(pch, "ap"))
					apsta_params->if_info[i].ifmode = IAP_MODE;
#ifdef WLMESH
				else if (!strcmp(pch, "mesh")) {
					if (dhd->conf->fw_type != FW_TYPE_MESH) {
						IAPSTA_ERROR(dev->name, "wrong fw type\n");
						return -1;
					}
					apsta_params->if_info[i].ifmode = IMESH_MODE;
				}
#endif /* WLMESH */
				pch = bcmstrtok(&pick_tmp2, " -", 0);
			}
		}
		else if (!strcmp(param, "rsdb")) {
			apsta_params->rsdb = (int)simple_strtol(pick_tmp2, NULL, 0) ? TRUE : FALSE;
		} else if (!strcmp(param, "mcc")) {
			apsta_params->mcc = (int)simple_strtol(pick_tmp2, NULL, 0) ? TRUE : FALSE;
		} else if (!strcmp(param, "acs")) {
			apsta_params->acs = (int)simple_strtol(pick_tmp2, NULL, 0);
#if defined(WLMESH) && defined(WL_ESCAN)
		} else if (!strcmp(param, "macs")) {
			apsta_params->macs = (int)simple_strtol(pick_tmp2, NULL, 0);
#endif /* WLMESH && WL_ESCAN */
		} else if (!strcmp(param, "ifname")) {
			pch = NULL;
			pch = bcmstrtok(&pick_tmp2, " -", 0);
			for (i=0; i<MAX_IF_NUM && pch; i++) {
				strcpy(apsta_params->if_info[i].ifname, pch);
				pch = bcmstrtok(&pick_tmp2, " -", 0);
			}
		} else if (!strcmp(param, "vifname")) {
			strcpy(apsta_params->if_info[IF_VIF].ifname, pick_tmp2);
		}
		param = bcmstrtok(&pick_tmp, " ", 0);
	}

	if (apsta_params->apstamode == 0) {
		IAPSTA_ERROR(dev->name, "mode [sta|ap|sta-ap|ap-ap]\n");
		return -1;
	}

	wl_ext_iapsta_preinit(dev, apsta_params);
#ifndef WL_STATIC_IF
	wl_ext_iapsta_intf_add(dev, apsta_params);
#endif /* WL_STATIC_IF */

	return 0;
}

int
wl_ext_iapsta_alive_preinit(struct net_device *dev)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(dev);

	if (apsta_params->init == TRUE) {
		IAPSTA_ERROR(dev->name, "don't init twice\n");
		return -1;
	}

	IAPSTA_TRACE(dev->name, "Enter\n");

	apsta_params->init = TRUE;

	return 0;
}

int
wl_ext_iapsta_alive_postinit(struct net_device *dev)
{
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(dev);
	s32 apsta = 0, ap = 0;
	struct wl_if_info *cur_if;
	int i;

	wl_ext_iovar_getint(dev, "apsta", &apsta);
	wl_ext_ioctl(dev, WLC_GET_AP, &ap, sizeof(ap), 0);
	if (apsta == 1 || ap == 0) {
		apsta_params->apstamode = ISTAONLY_MODE;
		apsta_params->if_info[IF_PIF].ifmode = ISTA_MODE;
		op_mode = DHD_FLAG_STA_MODE;
	} else {
		apsta_params->apstamode = IAPONLY_MODE;
		apsta_params->if_info[IF_PIF].ifmode = IAP_MODE;
		op_mode = DHD_FLAG_HOSTAP_MODE;
	}
	// fix me: how to check it's ISTAAP_MODE or IDUALAP_MODE?

	WL_MSG(dev->name, "apstamode=%d\n", apsta_params->apstamode);

	for (i=0; i<MAX_IF_NUM; i++) {
		cur_if = &apsta_params->if_info[i];
		if (i == 1 && !strlen(cur_if->ifname))
			strcpy(cur_if->ifname, "wlan1");
		if (i == 2 && !strlen(cur_if->ifname))
			strcpy(cur_if->ifname, "wlan2");
		if (cur_if->ifmode == ISTA_MODE) {
			wl_ext_set_chan_info(cur_if, WLC_BAND_2G, 0);
			cur_if->maxassoc = -1;
			wl_set_isam_status(cur_if, IF_READY);
			cur_if->prio = PRIO_STA;
			snprintf(cur_if->ssid, DOT11_MAX_SSID_LEN, "ttt_sta");
		}
		else if (cur_if->ifmode == IAP_MODE) {
			wl_ext_set_chan_info(cur_if, WLC_BAND_2G, 1);
			cur_if->maxassoc = -1;
			wl_set_isam_status(cur_if, IF_READY);
			cur_if->prio = PRIO_AP;
			snprintf(cur_if->ssid, DOT11_MAX_SSID_LEN, "ttt_ap");
		}
#ifdef WLMESH
		else if (cur_if->ifmode == IMESH_MODE) {
			wl_ext_set_chan_info(cur_if, WLC_BAND_2G, 1);
			cur_if->maxassoc = -1;
			wl_set_isam_status(cur_if, IF_READY);
			cur_if->prio = PRIO_MESH;
			snprintf(cur_if->ssid, DOT11_MAX_SSID_LEN, "ttt_mesh");
		}
#endif /* WLMESH */
	}

	return op_mode;
}
#endif

static void
wl_ext_iapsta_postinit(struct wl_if_info *cur_if)
{
	struct dhd_pub *dhd = dhd_get_pub(cur_if->dev);
	struct wl_apsta_params *apsta_params = wl_get_apsta_params(cur_if->dev);

	IAPSTA_TRACE(cur_if->ifname, "ifidx=%d\n", cur_if->ifidx);
	if (cur_if->ifidx == 0) {
		apsta_params->rsdb = dhd_conf_rsdb_chip(dhd);
		apsta_params->mcc = dhd_conf_mcc_chip(dhd);
		apsta_params->csa = dhd_conf_csa_chip(dhd);
		apsta_params->acs = 0;
		apsta_params->radar = wl_ext_radar_detect(cur_if->dev);
		if (dhd->conf->fw_type == FW_TYPE_MESH)
			apsta_params->csa = TRUE;
		cur_if->ifmode = ISTA_MODE;
		cur_if->prio = PRIO_STA;
		if (dhd->conf->vndr_ie_assocreq && strlen(dhd->conf->vndr_ie_assocreq))
			wl_ext_add_del_ie(cur_if->dev, VNDR_IE_ASSOCREQ_FLAG, dhd->conf->vndr_ie_assocreq, "add");
	} else {
		if (cur_if->ifmode == ISTA_MODE) {
			dhd_conf_preinit_ioctls_sta(dhd, cur_if->ifidx);
		}
#ifdef WLMESH
		else if (cur_if->ifmode == IMESH_MODE) {
			int pm = 0;
			wl_ext_ioctl(cur_if->dev, WLC_SET_PM, &pm, sizeof(pm), 1);
		}
#endif /* WLMESH */
	}
#ifdef PROPTX_MAXCOUNT
	wl_ext_update_wlfc_maxcount(dhd);
#endif /* PROPTX_MAXCOUNT */
}

int
wl_ext_iapsta_attach_name(struct net_device *net, int ifidx)
{
	struct dhd_pub *dhd = dhd_get_pub(net);
	struct wl_apsta_params *apsta_params = dhd->iapsta_params;
	struct wl_if_info *cur_if = NULL;

	if (ifidx < MAX_IF_NUM) {
		IAPSTA_TRACE(net->name, "ifidx=%d\n", ifidx);
		cur_if = &apsta_params->if_info[ifidx];
	}
	if (ifidx == 0) {
		strcpy(cur_if->ifname, net->name);
		wl_set_isam_status(cur_if, IF_READY);
		wl_ext_iapsta_postinit(cur_if);
		wl_ext_init_mchan_mode(cur_if);
#if defined(SET_XPS_CPUS) || defined(SET_RPS_CPUS)
		wl_ext_iapsta_set_xps(cur_if);
#endif /* SET_XPS_CPUS ||SET_RPS_CPUS  */
	} else if (cur_if && wl_get_isam_status(cur_if, IF_ADDING)) {
		strcpy(cur_if->ifname, net->name);
		wl_ext_iapsta_postinit(cur_if);
		wl_clr_isam_status(cur_if, IF_ADDING);
		wl_set_isam_status(cur_if, IF_READY);
#ifndef WL_STATIC_IF
		wake_up_interruptible(&apsta_params->netif_change_event);
#endif /* WL_STATIC_IF */
	}

	return 0;
}

int
wl_ext_iapsta_update_net_device(struct net_device *net, int ifidx)
{
	struct dhd_pub *dhd = dhd_get_pub(net);
	struct wl_apsta_params *apsta_params = dhd->iapsta_params;
	struct wl_if_info *cur_if = NULL;
#ifndef WL_STATIC_IF
	struct wl_if_info *primary_if;
#endif /* WL_STATIC_IF */

	if (ifidx < MAX_IF_NUM) {
		IAPSTA_TRACE(net->name, "ifidx=%d\n", ifidx);
		cur_if = &apsta_params->if_info[ifidx];
	}
	if (cur_if && wl_get_isam_status(cur_if, IF_ADDING)) {
		if (strlen(cur_if->ifname)) {
			memset(net->name, 0, sizeof(IFNAMSIZ));
			strcpy(net->name, cur_if->ifname);
			net->name[IFNAMSIZ-1] = '\0';
		}
#ifndef WL_STATIC_IF
		primary_if = &apsta_params->if_info[IF_PIF];
		if (apsta_params->apstamode != IUNKNOWN_MODE &&
				apsta_params->apstamode != ISTAAPAP_MODE &&
				apsta_params->apstamode != ISTASTA_MODE) {
			u8 mac_addr[ETH_ALEN];
			memcpy(mac_addr, primary_if->dev->dev_addr, ETHER_ADDR_LEN);
			mac_addr[0] |= 0x02;
			wl_ext_get_vif_macaddr(primary_if->dev, WL_IF_TYPE_AP, ifidx, mac_addr);
			DEV_ADDR_SET(net, mac_addr);
		}
#endif /* WL_STATIC_IF */
	}

	return 0;
}

#ifdef WLDWDS
int
wl_ext_iapsta_attach_dwds_netdev(struct net_device *net, int ifidx, uint8 bssidx)
{
	struct dhd_pub *dhd = dhd_get_pub(net);
	struct wl_apsta_params *apsta_params = dhd->iapsta_params;
	struct wl_if_info *cur_if = NULL;
	int i;

	for (i=0; i<MAX_IF_NUM; i++) {
		cur_if = &apsta_params->if_info[ifidx];
		if (cur_if->bssidx == bssidx) {
			break;
		}
	}

	if (cur_if) {
		IAPSTA_TRACE(net->name, "ifidx=%d, bssidx=%d\n", ifidx, bssidx);
		for (i=0; i<MAX_DWDS_IF_NUM; i++) {
			if (apsta_params->dwds_info[i].dev == NULL) {
				apsta_params->dwds_info[i].dev = net;
				apsta_params->dwds_info[i].ifidx = ifidx;
				apsta_params->dwds_info[i].bssidx = bssidx;
				break;
			}
		}
	}

	return 0;
}

int
wl_ext_iapsta_dettach_dwds_netdev(struct net_device *net, int ifidx, uint8 bssidx)
{
	struct dhd_pub *dhd = dhd_get_pub(net);
	struct wl_apsta_params *apsta_params = dhd->iapsta_params;
	struct wl_if_info *cur_if = NULL;
	int i;

	for (i=0; i<MAX_IF_NUM; i++) {
		cur_if = &apsta_params->if_info[ifidx];
		if (cur_if->bssidx == bssidx) {
			break;
		}
	}

	if (cur_if) {
		IAPSTA_TRACE(net->name, "ifidx=%d, bssidx=%d\n", ifidx, bssidx);
		for (i=0; i<MAX_DWDS_IF_NUM; i++) {
			if (apsta_params->dwds_info[i].dev == net) {
				memset(&apsta_params->dwds_info[i], 0, sizeof(struct wl_dwds_info));
			}
		}
	}

	return 0;
}
#endif /* WLDWDS */

static void
wl_ext_iapsta_init_priv(struct net_device *net)
{
	struct dhd_pub *dhd = dhd_get_pub(net);
	struct wl_apsta_params *apsta_params = dhd->iapsta_params;

	init_waitqueue_head(&apsta_params->netif_change_event);
	mutex_init(&apsta_params->usr_sync);
	mutex_init(&apsta_params->in4way_sync);
#ifdef STA_MGMT
	INIT_LIST_HEAD(&apsta_params->sta_list);
#endif /* STA_MGMT */
#ifdef EAPOL_RESEND
	spin_lock_init(&apsta_params->eapol_lock);
#endif /* EAPOL_RESEND */
}

static void
wl_ext_iapsta_deinit_priv(struct net_device *net)
{
	struct dhd_pub *dhd = dhd_get_pub(net);
	struct wl_apsta_params *apsta_params = dhd->iapsta_params;

	memset(apsta_params, 0, sizeof(struct wl_apsta_params));
}

int
wl_ext_iapsta_attach_netdev(struct net_device *net, int ifidx, uint8 bssidx)
{
	struct dhd_pub *dhd = dhd_get_pub(net);
	struct wl_apsta_params *apsta_params = dhd->iapsta_params;
	struct wl_if_info *cur_if = NULL;

	if (ifidx < MAX_IF_NUM) {
		IAPSTA_TRACE(net->name, "ifidx=%d, bssidx=%d\n", ifidx, bssidx);
		cur_if = &apsta_params->if_info[ifidx];
	}

	if (ifidx == 0) {
		wl_ext_iapsta_init_priv(net);
		strcpy(cur_if->ifname, net->name);
#ifdef TPUT_MONITOR
		wl_timer_register(net, &apsta_params->monitor_timer, wl_tput_monitor_timer);
#endif /* TPUT_MONITOR */
#ifdef RXF0OVFL_REINIT_WAR
		wl_timer_register(net, &apsta_params->rxf0ovfl_timer, wl_ext_rxf0ovfl_reinit_timeout);
#endif /* RXF0OVFL_REINIT_WAR */
		wl_timer_register(net, &apsta_params->reset_itf_timer, wl_ext_sync_interface_timeout);
	}

	if (ifidx == 0 || (cur_if && wl_get_isam_status(cur_if, IF_ADDING))) {
		cur_if->dev = net;
		cur_if->ifidx = ifidx;
		cur_if->bssidx = bssidx;
		mutex_init(&cur_if->pm_sync);
		INIT_DELAYED_WORK(&cur_if->pm_enable_work, wl_ext_pm_work_handler);
		init_waitqueue_head(&cur_if->ap_recon_sta_event);
		init_waitqueue_head(&cur_if->csa_complete_event);
		wl_ext_event_register(net, dhd, WLC_E_LAST, wl_ext_iapsta_event,
			apsta_params, PRIO_EVENT_IAPSTA);
#if defined(WLMESH) && defined(WL_ESCAN)
		wl_mesh_escan_attach(dhd, cur_if);
#endif /* WLMESH && WL_ESCAN */
#ifdef ACS_MONITOR
		wl_acs_attach(dhd, cur_if);
#endif /* ACS_MONITOR */
#ifdef SET_CARRIER
		wl_ext_net_setcarrier(cur_if, FALSE, TRUE);
#endif /* SET_CARRIER */
		wl_timer_register(net, &cur_if->connect_timer, wl_ext_connect_timeout);
#ifdef WL_EXT_RECONNECT
		wl_timer_register(net, &cur_if->reconnect_timer, wl_ext_reconnect_timeout);
#endif /* WL_EXT_RECONNECT */
#ifdef WL_EXT_REASSOC
		wl_timer_register(net, &cur_if->reassoc_scan_timer, wl_ext_reassoc_scan_timeout);
#endif /* WL_EXT_REASSOC */
#ifdef RESTART_AP_WAR
		wl_timer_register(net, &cur_if->restart_ap_timer, wl_ext_restart_ap_timeout);
#endif /* RESTART_AP_WAR */
#ifdef RESET_AP_WAR
		wl_timer_register(net, &cur_if->reset_ap_timer, wl_ext_reset_ap_timeout);
#endif /* RESET_AP_WAR */
#ifdef EAPOL_RESEND
		wl_timer_register(net, &cur_if->eapol_timer, wl_eapol_timer);
#endif /* EAPOL_RESEND */
#ifdef KEY_INSTALL_CHECK
		wl_timer_register(net, &cur_if->key_install_timer, wl_ext_key_install_timeout);
#endif /* KEY_INSTALL_CHECK */
#ifdef ARP_DETECTION
		wl_timer_register(net, &cur_if->arp_timer, wl_ext_arp_detection_timeout);
#endif /* ARP_DETECTION */
	}

	return 0;
}

int
wl_ext_iapsta_dettach_netdev(struct net_device *net, int ifidx)
{
	struct dhd_pub *dhd = dhd_get_pub(net);
	struct wl_apsta_params *apsta_params = dhd->iapsta_params;
	struct wl_if_info *cur_if = NULL;

	if (!apsta_params)
		return 0;

	if (ifidx < MAX_IF_NUM) {
		IAPSTA_TRACE(net->name, "ifidx=%d\n", ifidx);
		cur_if = &apsta_params->if_info[ifidx];
	}

	if (ifidx == 0 || (cur_if && (wl_get_isam_status(cur_if, IF_READY) ||
			wl_get_isam_status(cur_if, IF_ADDING)))) {
#ifdef EAPOL_RESEND
		wl_ext_release_eapol_txpkt(dhd, ifidx, FALSE);
#endif /* EAPOL_RESEND */
#ifdef ARP_DETECTION
		wl_timer_deregister(net, &cur_if->arp_timer);
#endif /* ARP_DETECTION */
#ifdef KEY_INSTALL_CHECK
		wl_timer_deregister(net, &cur_if->key_install_timer);
#endif /* KEY_INSTALL_CHECK */
		wl_timer_deregister(net, &cur_if->connect_timer);
#ifdef WL_EXT_RECONNECT
		wl_timer_deregister(net, &cur_if->reconnect_timer);
#endif /* WL_EXT_RECONNECT */
#ifdef RESTART_AP_WAR
		wl_timer_deregister(net, &cur_if->restart_ap_timer);
#endif /* RESTART_AP_WAR */
#ifdef RESET_AP_WAR
		wl_timer_deregister(net, &cur_if->reset_ap_timer);
#endif /* RESET_AP_WAR */
#ifdef SET_CARRIER
		wl_ext_net_setcarrier(cur_if, FALSE, FALSE);
#endif /* SET_CARRIER */
		wl_ext_add_remove_pm_enable_work(net, FALSE);
#ifdef ACS_MONITOR
		wl_acs_detach(cur_if);
#endif /* ACS_MONITOR */
#if defined(WLMESH) && defined(WL_ESCAN)
		wl_mesh_escan_detach(dhd, cur_if);
#endif /* WLMESH && WL_ESCAN */
		wl_ext_event_deregister(net, dhd, WLC_E_LAST, wl_ext_iapsta_event);
#ifdef STA_MGMT
		wl_ext_flush_sta_list(net, ifidx);
#endif /* STA_MGMT */
		memset(cur_if, 0, sizeof(struct wl_if_info));
	}

	if (ifidx == 0) {
		wl_timer_deregister(net, &apsta_params->reset_itf_timer);
#ifdef RXF0OVFL_REINIT_WAR
		wl_timer_deregister(net, &apsta_params->rxf0ovfl_timer);
#endif /* RXF0OVFL_REINIT_WAR */
#ifdef TPUT_MONITOR
		wl_timer_deregister(net, &apsta_params->monitor_timer);
#endif /* TPUT_MONITOR */
		wl_ext_iapsta_deinit_priv(net);
	}

	return 0;
}

void
wl_ext_iapsta_dettach(struct net_device *net)
{
	struct dhd_pub *dhd = dhd_get_pub(net);

	IAPSTA_TRACE(net->name, "Enter\n");

	if (g_ioctl_buf) {
		KVMFREE(dhd->osh, g_ioctl_buf, WL_DUMP_BUF_LEN);
		g_ioctl_buf = NULL;
	}

	if (dhd->iapsta_params) {
		wl_ext_iapsta_deinit_priv(net);
		MFREE(dhd->osh, dhd->iapsta_params, sizeof(struct wl_apsta_params));
		dhd->iapsta_params = NULL;
	}
}

int
wl_ext_iapsta_attach(struct net_device *net)
{
	struct dhd_pub *dhd = dhd_get_pub(net);
	struct wl_apsta_params *iapsta_params;
	int ret = 0;

	IAPSTA_TRACE(net->name, "Enter\n");

	iapsta_params = MALLOCZ(dhd->osh, sizeof(struct wl_apsta_params));
	if (unlikely(!iapsta_params)) {
		IAPSTA_ERROR(net->name, "Can not allocate apsta_params\n");
		ret = -ENOMEM;
		goto exit;
	}
	dhd->iapsta_params = (void *)iapsta_params;

	if (!g_ioctl_buf) {
		g_ioctl_buf = KVMALLOC(dhd->osh, WL_DUMP_BUF_LEN);
		if (unlikely(!g_ioctl_buf)) {
			IAPSTA_ERROR(net->name, "Can not allocate g_ioctl_buf\n");
			ret = -ENOMEM;
			goto exit;
		}
	}

exit:
	if (ret)
		wl_ext_iapsta_dettach(net);
	return ret;
}
#endif /* WL_EXT_IAPSTA */
