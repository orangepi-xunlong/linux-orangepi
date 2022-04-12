/**
 ****************************************************************************************
 *
 * @file bl_txq.h
 *
 * Copyright (C) BouffaloLab 2017-2018
 *
 ****************************************************************************************
 */
#ifndef _BL_TXQ_H_
#define _BL_TXQ_H_

#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/ieee80211.h>

/**
 * Fullmac TXQ configuration:
 *  - STA: 1 TXQ per TID (limited to 8)
 *         1 txq for bufferable MGT frames
 *  - VIF: 1 tXQ for Multi/Broadcast +
 *         1 TXQ for MGT for unknown STAs or non-bufferable MGT frames
 *  - 1 TXQ for offchannel transmissions
 *
 *
 * Txq mapping looks like
 * for NX_REMOTE_STA_MAX=10 and NX_VIRT_DEV_MAX=4
 *
 * | TXQ | NDEV_ID | VIF |   STA |  TID | HWQ |
 * |-----+---------+-----+-------+------+-----|-
 * |   0 |       0 |     |     0 |    0 |   1 | 9 TXQ per STA
 * |   1 |       1 |     |     0 |    1 |   0 | (8 data + 1 mgmt)
 * |   2 |       2 |     |     0 |    2 |   0 |
 * |   3 |       3 |     |     0 |    3 |   1 |
 * |   4 |       4 |     |     0 |    4 |   2 |
 * |   5 |       5 |     |     0 |    5 |   2 |
 * |   6 |       6 |     |     0 |    6 |   3 |
 * |   7 |       7 |     |     0 |    7 |   3 |
 * |   8 |     N/A |     |     0 | MGMT |   3 |
 * |-----+---------+-----+-------+------+-----|-
 * | ... |         |     |       |      |     | Same for all STAs
 * |-----+---------+-----+-------+------+-----|-
 * |  90 |      80 |   0 | BC/MC |    0 | 1/4 | 1 TXQ for BC/MC per VIF
 * | ... |         |     |       |      |     |
 * |  93 |      80 |   3 | BC/MC |    0 | 1/4 |
 * |-----+---------+-----+-------+------+-----|-
 * |  94 |     N/A |   0 |   N/A | MGMT |   3 | 1 TXQ for unknown STA per VIF
 * | ... |         |     |       |      |     |
 * |  97 |     N/A |   3 |   N/A | MGMT |   3 |
 * |-----+---------+-----+-------+------+-----|-
 * |  98 |     N/A |     |   N/A | MGMT |   3 | 1 TXQ for offchannel frame
 */
#define NX_NB_TID_PER_STA 8
#define NX_NB_TXQ_PER_STA (NX_NB_TID_PER_STA + 1)
#define NX_NB_TXQ_PER_VIF 2
#define NX_NB_TXQ ((NX_NB_TXQ_PER_STA * NX_REMOTE_STA_MAX) +    \
                   (NX_NB_TXQ_PER_VIF * NX_VIRT_DEV_MAX) + 1)

#define NX_FIRST_VIF_TXQ_IDX (NX_REMOTE_STA_MAX * NX_NB_TXQ_PER_STA)
#define NX_FIRST_BCMC_TXQ_IDX  NX_FIRST_VIF_TXQ_IDX
#define NX_FIRST_UNK_TXQ_IDX  (NX_FIRST_BCMC_TXQ_IDX + NX_VIRT_DEV_MAX)

#define NX_OFF_CHAN_TXQ_IDX (NX_FIRST_VIF_TXQ_IDX +                     \
                             (NX_VIRT_DEV_MAX * NX_NB_TXQ_PER_VIF))
#define NX_BCMC_TXQ_TYPE 0
#define NX_UNK_TXQ_TYPE  1

/**
 * Each data TXQ is a netdev queue. TXQ to send MGT are not data TXQ as
 * they did not recieved buffer from netdev interface.
 * Need to allocate the maximum case.
 * AP : all STAs + 1 BC/MC
 */
#define NX_NB_NDEV_TXQ ((NX_NB_TID_PER_STA * NX_REMOTE_STA_MAX) + 1 )
#define NX_BCMC_TXQ_NDEV_IDX (NX_NB_TID_PER_STA * NX_REMOTE_STA_MAX)
#define NX_STA_NDEV_IDX(tid, sta_idx) ((tid) + (sta_idx) * NX_NB_TID_PER_STA)
#define NDEV_NO_TXQ 0xffff
#if (NX_NB_NDEV_TXQ >= NDEV_NO_TXQ)
#error("Need to increase struct bl_txq->ndev_idx size")
#endif

/* stop netdev queue when number of queued buffers if greater than this  */
#define BL_NDEV_FLOW_CTRL_STOP    200//32
/* restart netdev queue when number of queued buffers is lower than this */
#define BL_NDEV_FLOW_CTRL_RESTART 32//16

#define TXQ_INACTIVE 0xffff
#if (NX_NB_TXQ >= TXQ_INACTIVE)
#error("Need to increase struct bl_txq->idx size")
#endif

#define NX_TXQ_INITIAL_CREDITS 32

/**
 * struct bl_hwq - Structure used to save information relative to
 *                   an AC TX queue (aka HW queue)
 * @list: List of TXQ, that have buffers ready for this HWQ
 * @credits: available credit for the queue (i.e. nb of buffers that
 *           can be pushed to FW )
 * @id Id of the HWQ among BL_HWQ_....
 * @size size of the queue
 * @need_processing Indicate if hwq should be processed
 * @len number of packet ready to be pushed to fw for this HW queue
 * @len_stop threshold to stop mac80211(i.e. netdev) queues. Stop queue when
 *           driver has more than @len_stop packets ready.
 * @len_start threshold to wake mac8011 queues. Wake queue when driver has
 *            less than @len_start packets ready.
 */
struct bl_hwq {
    struct list_head list;
    u8 credits[CONFIG_USER_MAX];
    u8 size;
    u8 id;
    bool need_processing;
};

/**
 * enum bl_push_flags - Flags of pushed buffer
 *
 * @BL_PUSH_RETRY Pushing a buffer for retry
 * @BL_PUSH_IMMEDIATE Pushing a buffer without queuing it first
 */
enum bl_push_flags {
    BL_PUSH_RETRY  = BIT(0),
    BL_PUSH_IMMEDIATE = BIT(1),
};

/**
 * enum bl_txq_flags - TXQ status flag
 *
 * @BL_TXQ_IN_HWQ_LIST The queue is scheduled for transmission
 * @BL_TXQ_STOP_FULL No more credits for the queue
 * @BL_TXQ_STOP_CSA CSA is in progress
 * @BL_TXQ_STOP_STA_PS Destiniation sta is currently in power save mode
 * @BL_TXQ_STOP_VIF_PS Vif owning this queue is currently in power save mode
 * @BL_TXQ_STOP_CHAN Channel of this queue is not the current active channel
 * @BL_TXQ_STOP_MU_POS TXQ is stopped waiting for all the buffers pushed to
 *                       fw to be confirmed
 * @BL_TXQ_STOP All possible reason to have a txq stopped
 * @BL_TXQ_NDEV_FLOW_CTRL associated netdev queue is currently stopped.
 *                          Note: when a TXQ is flowctrl it is NOT stopped
 */
enum bl_txq_flags {
    BL_TXQ_IN_HWQ_LIST  = BIT(0),
    BL_TXQ_STOP_FULL    = BIT(1),
    BL_TXQ_STOP_CSA     = BIT(2),
    BL_TXQ_STOP_STA_PS  = BIT(3),
    BL_TXQ_STOP_VIF_PS  = BIT(4),
    BL_TXQ_STOP_CHAN    = BIT(5),
    BL_TXQ_STOP_MU_POS  = BIT(6),
    BL_TXQ_STOP         = (BL_TXQ_STOP_FULL | BL_TXQ_STOP_CSA |
                             BL_TXQ_STOP_STA_PS | BL_TXQ_STOP_VIF_PS |
                             BL_TXQ_STOP_CHAN) ,
    BL_TXQ_NDEV_FLOW_CTRL = BIT(7),
};


/**
 * struct bl_txq - Structure used to save information relative to
 *                   a RA/TID TX queue
 *
 * @idx: Unique txq idx. Set to TXQ_INACTIVE if txq is not used.
 * @status: bitfield of @bl_txq_flags.
 * @credits: available credit for the queue (i.e. nb of buffers that
 *           can be pushed to FW).
 * @pkt_sent: number of consecutive pkt sent without leaving HW queue list
 * @pkt_pushed: number of pkt currently pending for transmission confirmation
 * @sched_list: list node for HW queue schedule list (bl_hwq.list)
 * @sk_list: list of buffers to push to fw
 * @last_retry_skb: pointer on the last skb in @sk_list that is a retry.
 *                  (retry skb are stored at the beginning of the list)
 *                  NULL if no retry skb is queued in @sk_list
 * @nb_retry: Number of retry packet queued.
 * @hwq: Pointer on the associated HW queue.
 *
 * SOFTMAC specific:
 * @baw: Block Ack window information
 * @amsdu_anchor: pointer to bl_sw_txhdr of the first subframe of the A-MSDU.
 *                NULL if no A-MSDU frame is in construction
 * @amsdu_ht_len_cap:
 * @amsdu_vht_len_cap:
 * @tid:
 *
 * FULLMAC specific
 * @ps_id: Index to use for Power save mode (LEGACY or UAPSD)
 * @push_limit: number of packet to push before removing the txq from hwq list.
 *              (we always have push_limit < skb_queue_len(sk_list))
 * @ndev_idx: txq idx from netdev point of view (0xFF for non netdev queue)
 * @ndev: pointer to ndev of the corresponding vif
 * @amsdu: pointer to bl_sw_txhdr of the first subframe of the A-MSDU.
 *         NULL if no A-MSDU frame is in construction
 * @amsdu_len: Maximum size allowed for an A-MSDU. 0 means A-MSDU not allowed
 */
struct bl_txq {
    u16 idx;
    u8 status;
    s8 credits;
    u8 pkt_sent;
    u8 pkt_pushed[CONFIG_USER_MAX];
    struct list_head sched_list;
    struct sk_buff_head sk_list;
    struct sk_buff *last_retry_skb;
    struct bl_hwq *hwq;
    int nb_retry;
    struct bl_sta *sta;
    u8 ps_id;
    u8 push_limit;
    u16 ndev_idx;
    struct net_device *ndev;
    #ifdef CONFIG_BL_AMSDUS_TX
    struct bl_sw_txhdr *amsdu;
     u16 amsdu_len;
    #endif /* CONFIG_BL_AMSDUS_TX */
};

struct bl_sta;
struct bl_vif;
struct bl_hw;
struct bl_sw_txhdr;

#define BL_TXQ_GROUP_ID(txq) 0
#define BL_TXQ_POS_ID(txq)   0

static inline bool bl_txq_is_stopped(struct bl_txq *txq)
{
    return (txq->status & BL_TXQ_STOP);
}

static inline bool bl_txq_is_full(struct bl_txq *txq)
{
    return (txq->status & BL_TXQ_STOP_FULL);
}

static inline bool bl_txq_is_scheduled(struct bl_txq *txq)
{
    return (txq->status & BL_TXQ_IN_HWQ_LIST);
}

/*
 * if
 * - txq is not stopped
 * - hwq has credits
 * - there is no buffer queued
 * then a buffer can be immediately pushed without having to queue it first
 */
static inline bool bl_txq_is_ready_for_push(struct bl_txq *txq)
{
    return (!bl_txq_is_stopped(txq) &&
            txq->hwq->credits[BL_TXQ_POS_ID(txq)] > 0 &&
            skb_queue_empty(&txq->sk_list));
}


/**
 * extract the first @nb_elt of @list and append them to @head
 * It is assume that:
 * - @list contains more that @nb_elt
 * - There is no need to take @list nor @head lock to modify them
 */
static inline void skb_queue_extract(struct sk_buff_head *list,
                                     struct sk_buff_head *head, int nb_elt)
{
    int i;
    struct sk_buff *first, *last, *ptr;

    first = ptr = list->next;
    for (i = 0; i < nb_elt; i++) {
        ptr = ptr->next;
    }
    last = ptr->prev;

    /* unlink nb_elt in list */
    list->qlen -= nb_elt;
    list->next = ptr;
    ptr->prev = (struct sk_buff *)list;

    /* append nb_elt at end of head */
    head->qlen += nb_elt;
    last->next = (struct sk_buff *)head;
    head->prev->next = first;
    first->prev = head->prev;
    head->prev = last;
}

struct bl_txq *bl_txq_sta_get(struct bl_sta *sta, u8 tid, int *idx,
                                  struct bl_hw * bl_hw);
struct bl_txq *bl_txq_vif_get(struct bl_vif *vif, u8 type, int *idx);

/* return status bits related to the vif */
static inline u8 bl_txq_vif_get_status(struct bl_vif *bl_vif)
{
    struct bl_txq *txq = bl_txq_vif_get(bl_vif, 0, NULL);
    return (txq->status & (BL_TXQ_STOP_CHAN | BL_TXQ_STOP_VIF_PS));
}

void bl_txq_vif_init(struct bl_hw * bl_hw, struct bl_vif *vif,
                       u8 status);
void bl_txq_vif_deinit(struct bl_hw * bl_hw, struct bl_vif *vif);
void bl_txq_sta_init(struct bl_hw * bl_hw, struct bl_sta *bl_sta,
                       u8 status);
void bl_txq_sta_deinit(struct bl_hw * bl_hw, struct bl_sta *bl_sta);
#ifdef CONFIG_BL_FULLMAC
void bl_txq_offchan_init(struct bl_vif *bl_vif);
void bl_txq_offchan_deinit(struct bl_vif *bl_vif);
#endif


void bl_txq_add_to_hw_list(struct bl_txq *txq);
void bl_txq_del_from_hw_list(struct bl_txq *txq);
void bl_txq_stop(struct bl_txq *txq, u16 reason);
void bl_txq_start(struct bl_txq *txq, u16 reason);
void bl_txq_vif_start(struct bl_vif *vif, u16 reason,
                        struct bl_hw *bl_hw);
void bl_txq_vif_stop(struct bl_vif *vif, u16 reason,
                       struct bl_hw *bl_hw);

void bl_txq_sta_start(struct bl_sta *sta, u16 reason,
                        struct bl_hw *bl_hw);
void bl_txq_sta_stop(struct bl_sta *sta, u16 reason,
                       struct bl_hw *bl_hw);
void bl_txq_offchan_start(struct bl_hw *bl_hw);
void bl_txq_sta_switch_vif(struct bl_sta *sta, struct bl_vif *old_vif,
                             struct bl_vif *new_vif);

int bl_txq_queue_skb(struct sk_buff *skb, struct bl_txq *txq,
                       struct bl_hw *bl_hw,  bool retry);
void bl_txq_confirm_any(struct bl_hw *bl_hw, struct bl_txq *txq,
                          struct bl_hwq *hwq, struct bl_sw_txhdr *sw_txhdr);


void bl_hwq_init(struct bl_hw *bl_hw);
void bl_hwq_process(struct bl_hw *bl_hw, struct bl_hwq *hwq);
void bl_hwq_process_all(struct bl_hw *bl_hw);

#endif /* _BL_TXQ_H_ */
