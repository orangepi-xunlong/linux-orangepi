/**
 ******************************************************************************
 *
 * @file bl_defs.h
 *
 * @brief Main driver structure declarations for fullmac driver
 *
 * Copyright (C) BouffaloLab 2017-2018
 *
 ******************************************************************************
 */

#ifndef _BL_DEFS_H_
#define _BL_DEFS_H_

#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/dmapool.h>
#include <linux/skbuff.h>
#include <net/cfg80211.h>
#include <linux/slab.h>

#include "bl_mod_params.h"
#include "bl_debugfs.h"
#include "bl_tx.h"
#include "bl_rx.h"
#include "bl_utils.h"
#include "bl_platform.h"

#define WPI_HDR_LEN    18
#define WPI_PN_LEN     16
#define WPI_PN_OFST     2
#define WPI_MIC_LEN    16
#define WPI_KEY_LEN    32
#define WPI_SUBKEY_LEN 16 // WPI key is actually two 16bytes key

#define LEGACY_PS_ID   0
#define UAPSD_ID       1


/**
 * struct bl_bcn - Information of the beacon in used (AP mode)
 *
 * @head: head portion of beacon (before TIM IE)
 * @tail: tail portion of beacon (after TIM IE)
 * @ies: extra IEs (not used ?)
 * @head_len: length of head data
 * @tail_len: length of tail data
 * @ies_len: length of extra IEs data
 * @tim_len: length of TIM IE
 * @len: Total beacon len (head + tim + tail + extra)
 * @dtim: dtim period
 */
struct bl_bcn {
    u8 *head;
    u8 *tail;
    u8 *ies;
    size_t head_len;
    size_t tail_len;
    size_t ies_len;
    size_t tim_len;
    size_t len;
    u8 dtim;
};

/**
 * struct bl_key - Key information
 *
 * @hw_idx: Idx of the key from hardware point of view
 */
struct bl_key {
    u8 hw_idx;
};

/**
 * struct bl_csa - Information for CSA (Channel Switch Announcement)
 *
 * @vif: Pointer to the vif doing the CSA
 * @bcn: Beacon to use after CSA
 * @dma: DMA descriptor to send the new beacon to the fw
 * @chandef: defines the channel to use after the switch
 * @count: Current csa counter
 * @status: Status of the CSA at fw level
 * @ch_idx: Index of the new channel context
 * @work: work scheduled at the end of CSA
 */
struct bl_csa {
    struct bl_vif *vif;
    struct bl_bcn bcn;
    struct bl_dma_elem dma;
    struct cfg80211_chan_def chandef;
    int count;
    int status;
    int ch_idx;
    struct work_struct work;
};

/**
 * enum bl_ap_flags - AP flags
 *
 * @BL_AP_ISOLATE Isolate clients (i.e. Don't brige packets transmitted by
 *                                   one client for another one)
 */
enum bl_ap_flags {
    BL_AP_ISOLATE = BIT(0),
};

/*
 * Structure used to save information relative to the managed interfaces.
 * This is also linked within the bl_hw vifs list.
 *
 */
struct bl_vif {
    struct list_head list;
    struct bl_hw *bl_hw;
    struct wireless_dev wdev;
    struct net_device *ndev;
    struct net_device_stats net_stats;
    struct bl_key key[6];
    u8 drv_vif_index;           /* Identifier of the VIF in driver */
    u8 vif_index;               /* Identifier of the station in FW */
    u8 ch_index;                /* Channel context identifier */
    bool up;                    /* Indicate if associated netdev is up
                                   (i.e. Interface is created at fw level) */
    bool use_4addr;             /* Should we use 4addresses mode */
    bool is_resending;          /* Indicate if a frame is being resent on this interface */
    bool user_mpm;              /* In case of Mesh Point VIF, indicate if MPM is handled by userspace */
    bool roc_tdls;              /* Indicate if the ROC has been called by a
                                   TDLS station */
    u8 tdls_status;             /* Status of the TDLS link */
    union
    {
        struct
        {
            struct bl_sta *ap; /* Pointer to the peer STA entry allocated for
                                    the AP */
            struct bl_sta *tdls_sta; /* Pointer to the TDLS station */
        } sta;
        struct
        {
            u16 flags;                 /* see bl_ap_flags */
            struct list_head sta_list; /* List of STA connected to the AP */
            struct bl_bcn bcn;       /* beacon */
            u8 bcmc_index;             /* Index of the BCMC sta to use */
            struct bl_csa *csa;

            struct list_head mpath_list; /* List of Mesh Paths used on this interface */
            struct list_head proxy_list; /* List of Proxies Information used on this interface */
            bool create_path;            /* Indicate if we are waiting for a MESH_CREATE_PATH_CFM
                                            message */
            int generation;              /* Increased each time the list of Mesh Paths is updated */
        } ap;
        struct
        {
            struct bl_vif *master;   /* pointer on master interface */
            struct bl_sta *sta_4a;
        } ap_vlan;
    };
};

#define BL_VIF_TYPE(bl_vif) (bl_vif->wdev.iftype)

/**
 * Structure used to store information relative to PS mode.
 *
 * @active: True when the sta is in PS mode.
 *          If false, other values should be ignored
 * @pkt_ready: Number of packets buffered for the sta in drv's txq
 *             (1 counter for Legacy PS and 1 for U-APSD)
 * @sp_cnt: Number of packets that remain to be pushed in the service period.
 *          0 means that no service period is in progress
 *          (1 counter for Legacy PS and 1 for U-APSD)
 */
struct bl_sta_ps {
    bool active;
    u16 pkt_ready[2];
    u16 sp_cnt[2];
};

/**
 * struct bl_rx_rate_stats - Store statistics for RX rates
 *
 * @table: Table indicating how many frame has been receive which each
 * rate index. Rate index is the same as the one used by RC algo for TX
 * @size: Size of the table array
 * @cpt: number of frames received
 */
struct bl_rx_rate_stats {
    int *table;
    int size;
    int cpt;
};

/**
 * struct bl_sta_stats - Structure Used to store statistics specific to a STA
 *
 * @last_rx: Hardware vector of the last received frame
 * @rx_rate: Statistics of the received rates
 */
struct bl_sta_stats {
#ifdef CONFIG_BL_DEBUGFS
    struct hw_vect last_rx;
    struct bl_rx_rate_stats rx_rate;
#endif
};

/*
 * Structure used to save information relative to the managed stations.
 */
struct bl_sta {
    struct list_head list;
    u16 aid;                /* association ID */
    u8 sta_idx;             /* Identifier of the station */
    u8 vif_idx;             /* Identifier of the VIF (fw id) the station
                               belongs to */
    u8 vlan_idx;            /* Identifier of the VLAN VIF (fw id) the station
                               belongs to (= vif_idx if no vlan in used) */
    enum nl80211_band band; /* Band */
    enum nl80211_chan_width width; /* Channel width */
    u16 center_freq;        /* Center frequency */
    u32 center_freq1;       /* Center frequency 1 */
    u32 center_freq2;       /* Center frequency 2 */
    u8 ch_idx;              /* Identifier of the channel
                               context the station belongs to */
    bool qos;               /* Flag indicating if the station
                               supports QoS */
    u8 acm;                 /* Bitfield indicating which queues
                               have AC mandatory */
    u16 uapsd_tids;         /* Bitfield indicating which tids are subject to
                               UAPSD */
    u8 mac_addr[ETH_ALEN];  /* MAC address of the station */
    struct bl_key key;
    bool valid;             /* Flag indicating if the entry is valid */
    struct bl_sta_ps ps;  /* Information when STA is in PS (AP only) */
    bool ht;               /* Flag indicating if the station
                               supports HT */
    bool vht;               /* Flag indicating if the station
                               supports VHT */
    u32 ac_param[AC_MAX];  /* EDCA parameters */
    struct bl_sta_stats stats;
};

#ifdef CONFIG_BL_SPLIT_TX_BUF
struct bl_amsdu_stats {
    int done;
    int failed;
};
#endif

struct bl_stats {
    int cfm_balance[NX_TXQ_CNT];
    unsigned long last_rx, last_tx; /* jiffies */
    int ampdus_tx[IEEE80211_MAX_AMPDU_BUF];
    int ampdus_rx[IEEE80211_MAX_AMPDU_BUF];
    int ampdus_rx_map[4];
    int ampdus_rx_miss;
#ifdef CONFIG_BL_SPLIT_TX_BUF
    struct bl_amsdu_stats amsdus[NX_TX_PAYLOAD_MAX];
#endif
    int amsdus_rx[64];
};

struct bl_sec_phy_chan {
    u16 prim20_freq;
    u16 center_freq1;
    u16 center_freq2;
    enum nl80211_band band;
    u8 type;
};

/* Structure that will contains all RoC information received from cfg80211 */
struct bl_roc_elem {
    struct wireless_dev *wdev;
    struct ieee80211_channel *chan;
    unsigned int duration;
    /* Used to avoid call of CFG80211 callback upon expiration of RoC */
    bool mgmt_roc;
    /* Indicate if we have switch on the RoC channel */
    bool on_chan;
};

/* Structure containing channel survey information received from MAC */
struct bl_survey_info {
    // Filled
    u32 filled;
    // Amount of time in ms the radio spent on the channel
    u32 chan_time_ms;
    // Amount of time the primary channel was sensed busy
    u32 chan_time_busy_ms;
    // Noise in dbm
    s8 noise_dbm;
};

struct bl_agg_reord_pkt {
	struct list_head list;
	struct sk_buff *skb;
	u16 sn;
};

#define BL_CH_NOT_SET 0xFF

/* Structure containing channel context information */
struct bl_chanctx {
    struct cfg80211_chan_def chan_def; /* channel description */
    u8 count;                          /* number of vif using this ctxt */
};

struct bl_dbg_credit {
	int sdio_port;
	int txq_credit;
	int hwq_credit;
	int credit;
	int nb_ready;
	int txq_idx;
};

struct bl_dbg_time {
	s64 sdio_tx;
};

/** 2K buf size */
#define BL_TX_DATA_BUF_SIZE_16K        16*1024
#define BL_RX_DATA_BUF_SIZE_16K        16*1024

#define BL_SDIO_MP_AGGR_PKT_LIMIT_MAX  8
#define BL_SDIO_MPA_ADDR_BASE                  0x1000

typedef struct _sdio_mpa_tx {
               u8 *buf;
               u32 buf_len;
               u32 pkt_cnt;
               u32 ports;
               u16 start_port;
               u16 mp_wr_info[BL_SDIO_MP_AGGR_PKT_LIMIT_MAX];
}sdio_mpa_tx;

typedef struct _sdio_mpa_rx {
               u8 *buf;
               u32 buf_len;
               u32 pkt_cnt;
               u32 ports;
               u16 start_port;
               struct sk_buff *buf_arr[BL_SDIO_MP_AGGR_PKT_LIMIT_MAX];
               u32 len_arr[BL_SDIO_MP_AGGR_PKT_LIMIT_MAX];
               u16 mp_rd_info[BL_SDIO_MP_AGGR_PKT_LIMIT_MAX];
               u16 mp_rd_port[BL_SDIO_MP_AGGR_PKT_LIMIT_MAX];
}sdio_mpa_rx;

struct bl_hw {
    struct bl_mod_params *mod_params;
	void *log_buff;
	int  buf_write_size;
    bool use_phy_bw_tweaks;
    struct device *dev;
    struct wiphy *wiphy;
    struct list_head vifs;
    struct bl_vif *vif_table[NX_VIRT_DEV_MAX + NX_REMOTE_STA_MAX]; /* indexed with fw id */
    struct bl_sta sta_table[NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX];
    struct bl_survey_info survey[SCAN_CHANNEL_MAX];
    struct cfg80211_scan_request *scan_request;
    struct bl_chanctx chanctx_table[NX_CHAN_CTXT_CNT];
    u8 cur_chanctx;


    /* RoC Management */
    struct bl_roc_elem *roc_elem;             /* Information provided by cfg80211 in its remain on channel request */
    u32 roc_cookie_cnt;                         /* Counter used to identify RoC request sent by cfg80211 */

    struct bl_cmd_mgr cmd_mgr;

    unsigned long drv_flags;
    struct bl_plat *plat;
    struct ipc_host_env_tag *ipc_env;           /* store the IPC environment */

    spinlock_t tx_lock;
	spinlock_t main_proc_lock;
	spinlock_t int_lock;
	spinlock_t cmd_lock;
	spinlock_t resend_lock;
	spinlock_t txq_lock;
	spinlock_t rd_int_lock;
    struct mutex mutex;                         /* per-device perimeter lock */
	struct semaphore sem;
	u8 bl_processing;
	u8 more_task_flag;
	u8 lost_int_flag;

    struct tasklet_struct task;
	struct work_struct main_work;
	struct workqueue_struct *workqueue;
    struct mm_version_cfm version_cfm;          /* Lower layers versions - obtained via MM_VERSION_REQ */
	u32 int_status;
	u32 cmd_sent;
	u32 resend;

    int rxbuff_idx;
    struct sk_buff *rxbuff_table[BL_RXBUFF_MAX];
    struct bl_e2amsg_elem    *msg_elems;      /* pointer to the E2A msg useful addresses */
    struct bl_dbg_elem       *dbg_elems;      /* pointer to the Debug msg useful addresses */
    struct bl_e2aradar_elem  *radar_elems;    /* pointer to the E2A radar useful addresses */
    struct bl_e2arxdesc_elem *rxdesc_elems;   /* pointer to the E2A RX Desc useful addresses */
    struct kmem_cache      *sw_txhdr_cache;
    struct kmem_cache      *agg_reodr_pkt_cache;
    sdio_mpa_tx mpa_tx_data;
    sdio_mpa_rx mpa_rx_data;

    struct bl_dbginfo     dbginfo;            /* Debug information from FW */

    struct bl_debugfs     debugfs;
    struct bl_stats       stats;

    struct bl_txq txq[NX_NB_TXQ];
    struct bl_hwq hwq[NX_TXQ_CNT];
    struct bl_preq_ie_elem preq_ie;
    struct bl_sec_phy_chan sec_phy_chan;
    u8 phy_cnt;
    u8 chan_ctxt_req;
    u8 avail_idx_map;
    u8 vif_started;
    bool adding_sta;
    struct phy_cfg_tag phy_config;

    /* extended capabilities supported */
    u8 ext_capa[8];

	u8 dbg_dump_start;
	u32 la_buf_idx;

	u8 recovery_flag; //temp add for fw recovery flag

	u16 msg_idx;

	struct bl_dbg_credit dbg_credit[50];
	int dbg_credit_idx;

	struct bl_dbg_time dbg_time[50];
	int dbg_time_idx;

	struct list_head reorder_list[NX_REMOTE_STA_MAX*NX_NB_TID_PER_STA];
};

u8 *bl_build_bcn(struct bl_bcn *bcn, struct cfg80211_beacon_data *new);

void bl_chanctx_link(struct bl_vif *vif, u8 idx,
                        struct cfg80211_chan_def *chandef);
void bl_chanctx_unlink(struct bl_vif *vif);
int  bl_chanctx_valid(struct bl_hw *bl_hw, u8 idx);

static inline bool is_multicast_sta(int sta_idx)
{
    return (sta_idx >= NX_REMOTE_STA_MAX);
}

static inline uint8_t master_vif_idx(struct bl_vif *vif)
{
    if (unlikely(vif->wdev.iftype == NL80211_IFTYPE_AP_VLAN)) {
        return vif->ap_vlan.master->vif_index;
    } else {
        return vif->vif_index;
    }
}


#endif /* _BL_DEFS_H_ */
