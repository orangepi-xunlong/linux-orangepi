/**
 ******************************************************************************
 *
 * @file bl_tx.h
 *
 * Copyright (C) BouffaloLab 2017-2018
 *
 ******************************************************************************
 */
#ifndef _BL_TX_H_
#define _BL_TX_H_

#include <linux/version.h>
#include <linux/ieee80211.h>
#include <net/cfg80211.h>
#include <linux/netdevice.h>
#include "lmac_types.h"
#include "lmac_msg.h"
#include "ipc_shared.h"
#include "bl_txq.h"
#include "hal_desc.h"

#define BL_HWQ_BK                     0
#define BL_HWQ_BE                     1
#define BL_HWQ_VI                     2
#define BL_HWQ_VO                     3
#define BL_HWQ_BCMC                   4
#define BL_HWQ_NB                     NX_TXQ_CNT
#define BL_HWQ_ALL_ACS (BL_HWQ_BK | BL_HWQ_BE | BL_HWQ_VI | BL_HWQ_VO)
#define BL_HWQ_ALL_ACS_BIT ( BIT(BL_HWQ_BK) | BIT(BL_HWQ_BE) |    \
                               BIT(BL_HWQ_VI) | BIT(BL_HWQ_VO) )

#define BL_TX_LIFETIME_MS             100

#define BL_SWTXHDR_ALIGN_SZ           4
#define BL_SWTXHDR_ALIGN_MSK (BL_SWTXHDR_ALIGN_SZ - 1)
#define BL_SWTXHDR_ALIGN_PADS(x) \
                    ((BL_SWTXHDR_ALIGN_SZ - ((x) & BL_SWTXHDR_ALIGN_MSK)) \
                     & BL_SWTXHDR_ALIGN_MSK)
#if BL_SWTXHDR_ALIGN_SZ & BL_SWTXHDR_ALIGN_MSK
#error bad BL_SWTXHDR_ALIGN_SZ
#endif

#define AMSDU_PADDING(x) ((4 - ((x) & 0x3)) & 0x3)

#define TXU_CNTRL_RETRY        BIT(0)
#define TXU_CNTRL_MORE_DATA    BIT(2)
#define TXU_CNTRL_MGMT         BIT(3)
#define TXU_CNTRL_MGMT_NO_CCK  BIT(4)
#define TXU_CNTRL_AMSDU        BIT(6)
#define TXU_CNTRL_MGMT_ROBUST  BIT(7)
#define TXU_CNTRL_USE_4ADDR    BIT(8)
#define TXU_CNTRL_EOSP         BIT(9)
#define TXU_CNTRL_MESH_FWD     BIT(10)
#define TXU_CNTRL_TDLS         BIT(11)

extern const int bl_tid2hwq[IEEE80211_NUM_TIDS];

/**
 * struct bl_amsdu_txhdr - Structure added in skb headroom (instead of
 * bl_txhdr) for amsdu subframe buffer (except for the first subframe
 * that has a normal bl_txhdr)
 *
 * @list     List of other amsdu subframe (bl_sw_txhdr.amsdu.hdrs)
 * @map_len  Length to be downloaded for this subframe
 * @dma_addr Buffer address form embedded point of view
 * @skb      skb
 * @pad      padding added before this subframe
 *           (only use when amsdu must be dismantled)
 */
struct bl_amsdu_txhdr {
    struct list_head list;
    size_t map_len;
    dma_addr_t dma_addr;
    struct sk_buff *skb;
    u16 pad;
};

/**
 * struct bl_amsdu - Structure to manage creation of an A-MSDU, updated
 * only In the first subframe of an A-MSDU
 *
 * @hdrs List of subframe of bl_amsdu_txhdr
 * @len  Current size for this A-MDSU (doesn't take padding into account)
 *       0 means that no amsdu is in progress
 * @nb   Number of subframe in the amsdu
 * @pad  Padding to add before adding a new subframe
 */
struct bl_amsdu {
    struct list_head hdrs;
    u16 len;
    u8 nb;
    u8 pad;
};

/**
 * struct bl_sw_txhdr - Software part of tx header
 *
 * @bl_sta sta to which this buffer is addressed
 * @bl_vif vif that send the buffer
 * @txq pointer to TXQ used to send the buffer
 * @hw_queue Index of the HWQ used to push the buffer.
 *           May be different than txq->hwq->id on confirmation.
 * @frame_len Size of the frame (doesn't not include mac header)
 *            (Only used to update stat, can't we use skb->len instead ?)
 * @headroom Headroom added in skb to add bl_txhdr
 *           (Only used to remove it before freeing skb, is it needed ?)
 * @amsdu Description of amsdu whose first subframe is this buffer
 *        (amsdu.nb = 0 means this buffer is not part of amsdu)
 * @skb skb received from transmission
 * @map_len  Length mapped for DMA (only bl_hw_txhdr and data are mapped)
 * @dma_addr DMA address after mapping
 * @desc Buffer description that will be copied in shared mem for FW
 */
struct bl_sw_txhdr {
    struct bl_sta *bl_sta;
    struct bl_vif *bl_vif;
    struct bl_txq *txq;
    u8 hw_queue;
    u16 frame_len;
    u16 headroom;
#ifdef CONFIG_BL_AMSDUS_TX
    struct bl_amsdu amsdu;
#endif
    struct sk_buff *skb;

    size_t map_len;
    dma_addr_t dma_addr;
	struct sdio_hdr hdr;
    struct txdesc_api desc;
};

/**
 * struct bl_txhdr - Stucture to control transimission of packet
 * (Added in skb headroom)
 *
 * @sw_hdr: Information from driver
 * @cache_guard:
 * @hw_hdr: Information for/from hardware
 */
struct bl_txhdr {
    struct bl_sw_txhdr *sw_hdr;
    char cache_guard[L1_CACHE_BYTES];
    struct bl_hw_txhdr hw_hdr;
};

int bl_start_xmit(struct sk_buff *skb, struct net_device *dev);
int bl_requeue_multicast_skb(struct sk_buff *skb, struct bl_vif *bl_vif);
int bl_start_mgmt_xmit(struct bl_vif *vif, struct bl_sta *sta,
                         struct cfg80211_mgmt_tx_params *params, bool offchan,
                         u64 *cookie);
int bl_txdatacfm(void *pthis, void *host_id, void *data1, void **data2);

struct bl_hw;
struct bl_sta;
void bl_set_traffic_status(struct bl_hw *bl_hw,
                             struct bl_sta *sta,
                             bool available,
                             u8 ps_id);
void bl_ps_bh_enable(struct bl_hw *bl_hw, struct bl_sta *sta,
                       bool enable);
void bl_ps_bh_traffic_req(struct bl_hw *bl_hw, struct bl_sta *sta,
                            u16 pkt_req, u8 ps_id);

void bl_switch_vif_sta_txq(struct bl_sta *sta, struct bl_vif *old_vif,
                             struct bl_vif *new_vif);

int bl_dbgfs_print_sta(char *buf, size_t size, struct bl_sta *sta,
                         struct bl_hw *bl_hw);
void bl_txq_credit_update(struct bl_hw *bl_hw, int sta_idx, u8 tid,
                            s8 update);
void bl_tx_push(struct bl_hw *bl_hw, struct bl_txhdr *txhdr, int flags);
void bl_tx_multi_pkt_push(struct bl_hw *bl_hw, struct sk_buff_head *sk_list_push);

void bl_downgrade_ac(struct bl_sta *sta, struct sk_buff *skb);

#endif /* _BL_TX_H_ */
