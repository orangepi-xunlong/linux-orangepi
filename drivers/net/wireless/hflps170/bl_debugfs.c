/**
 ****************************************************************************************
 *
 * @file bl_utils.c
 *
 * @brief Miscellaneous utility function definitions
 *
 * Copyright (C) BouffaloLab 2017-2018
 *
 ****************************************************************************************
 */


#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/kmod.h>
#include <linux/debugfs.h>
#include <linux/string.h>
#include <linux/sort.h>

#include "bl_debugfs.h"
#include "bl_msg_tx.h"
#include "bl_tx.h"
#include "bl_utils.h"
#include "bl_sdio.h"
#include "bl_platform.h"
#include "bl_v7.h"

/* some macros taken from iwlwifi */
/* TODO: replace with generic read and fill read buffer in open to avoid double
 * reads */
#define DEBUGFS_ADD_FILE(name, parent, mode) do {               \
    if (!debugfs_create_file(#name, mode, parent, bl_hw,      \
                &bl_dbgfs_##name##_ops))                      \
    goto err;                                                   \
} while (0)

#define DEBUGFS_ADD_BOOL(name, parent, ptr) do {                \
    struct dentry *__tmp;                                       \
    __tmp = debugfs_create_bool(#name, S_IWUSR | S_IRUSR,       \
            parent, ptr);                                       \
    if (IS_ERR(__tmp) || !__tmp)                                \
    goto err;                                                   \
} while (0)

#define DEBUGFS_ADD_X64(name, parent, ptr) do {                 \
    struct dentry *__tmp;                                       \
    __tmp = debugfs_create_x64(#name, S_IWUSR | S_IRUSR,        \
            parent, ptr);                                       \
    if (IS_ERR(__tmp) || !__tmp)                                \
    goto err;                                                   \
} while (0)

#define DEBUGFS_ADD_U64(name, parent, ptr, mode) do {           \
    struct dentry *__tmp;                                       \
    __tmp = debugfs_create_u64(#name, mode,                     \
            parent, ptr);                                       \
    if (IS_ERR(__tmp) || !__tmp)                                \
    goto err;                                                   \
} while (0)

#define DEBUGFS_ADD_X32(name, parent, ptr) do {                 \
    struct dentry *__tmp;                                       \
    __tmp = debugfs_create_x32(#name, S_IWUSR | S_IRUSR,        \
            parent, ptr);                                       \
    if (IS_ERR(__tmp) || !__tmp)                                \
    goto err;                                                   \
} while (0)

#define DEBUGFS_ADD_U32(name, parent, ptr, mode) do {           \
    struct dentry *__tmp;                                       \
    __tmp = debugfs_create_u32(#name, mode,                     \
            parent, ptr);                                       \
    if (IS_ERR(__tmp) || !__tmp)                                \
    goto err;                                                   \
} while (0)

/* file operation */
#define DEBUGFS_READ_FUNC(name)                                 \
    static ssize_t bl_dbgfs_##name##_read(struct file *file,  \
            char __user *user_buf,                              \
            size_t count, loff_t *ppos);

#define DEBUGFS_WRITE_FUNC(name)                                \
    static ssize_t bl_dbgfs_##name##_write(struct file *file, \
            const char __user *user_buf,                        \
            size_t count, loff_t *ppos);


#define DEBUGFS_READ_FILE_OPS(name)                             \
    DEBUGFS_READ_FUNC(name);                                    \
static const struct file_operations bl_dbgfs_##name##_ops = { \
    .read   = bl_dbgfs_##name##_read,                         \
    .open   = simple_open,                                      \
    .llseek = generic_file_llseek,                              \
};

#define DEBUGFS_WRITE_FILE_OPS(name)                            \
    DEBUGFS_WRITE_FUNC(name);                                   \
static const struct file_operations bl_dbgfs_##name##_ops = { \
    .write  = bl_dbgfs_##name##_write,                        \
    .open   = simple_open,                                      \
    .llseek = generic_file_llseek,                              \
};


#define DEBUGFS_READ_WRITE_FILE_OPS(name)                       \
    DEBUGFS_READ_FUNC(name);                                    \
DEBUGFS_WRITE_FUNC(name);                                       \
static const struct file_operations bl_dbgfs_##name##_ops = { \
    .write  = bl_dbgfs_##name##_write,                        \
    .read   = bl_dbgfs_##name##_read,                         \
    .open   = simple_open,                                      \
    .llseek = generic_file_llseek,                              \
};

#define DBG_TIME_HDR "sdiotxtime"
#define DBG_TIME_HDR_FMT "%10lld"
#define DBG_TIME_HDR_LEN sizeof(DBG_TIME_HDR)
static ssize_t bl_dbgfs_dbg_time_read(struct file *file ,
                                   char __user *user_buf,
                                   size_t count, loff_t *ppos)
{
    struct bl_hw *bl_hw = file->private_data;
    char *buf;
    int idx, res;
    ssize_t read;
	int i;
	size_t bufsz = sizeof(bl_hw->dbg_time)+DBG_TIME_HDR_LEN + 3000;

	bufsz = min_t(size_t, bufsz, count);
    buf = kmalloc(bufsz, GFP_ATOMIC);
    if (buf == NULL)
        return 0;

	bufsz--;
    idx = 0;

	res = scnprintf(&buf[idx], bufsz, DBG_TIME_HDR);
    idx += res;
    bufsz -= res;

    res = scnprintf(&buf[idx], bufsz, "\n");
    idx += res;
    bufsz -= res;

    for (i = 0; i < 49; i++) {

	    res = scnprintf(&buf[idx], bufsz, DBG_TIME_HDR_FMT, 
								bl_hw->dbg_time[i].sdio_tx
								);
	    idx += res;
	    bufsz -= res;
        res = scnprintf(&buf[idx], bufsz, "\n");
        idx += res;
        bufsz -= res;
    }

    res = scnprintf(&buf[idx], bufsz, "\n");

	read = simple_read_from_buffer(user_buf, count, ppos, buf, idx);
    kfree(buf);

    return read;
}

static ssize_t bl_dbgfs_dbg_time_write(struct file *file,
                                      const char __user *user_buf,
                                      size_t count, loff_t *ppos)
{
	return count;
}

DEBUGFS_READ_WRITE_FILE_OPS(dbg_time);

#define DBG_CDT_HDR "sdioport|txqcredit|hwqcredit|credit|ready|txqidx"
#define DBG_CDT_HDR_FMT "%8d|%9d|%9d|%6d|%5d|%6d"
#define DBG_CDT_HDR_LEN sizeof(DBG_CDT_HDR)

static ssize_t bl_dbgfs_dbg_cdt_read(struct file *file ,
                                   char __user *user_buf,
                                   size_t count, loff_t *ppos)
{
    struct bl_hw *bl_hw = file->private_data;
    char *buf;
    int idx, res;
    ssize_t read;
	int i;
	size_t bufsz = sizeof(bl_hw->dbg_credit)+DBG_CDT_HDR_LEN + 3000;

	bufsz = min_t(size_t, bufsz, count);
    buf = kmalloc(bufsz, GFP_ATOMIC);
    if (buf == NULL)
        return 0;

	bufsz--;
    idx = 0;

	res = scnprintf(&buf[idx], bufsz, DBG_CDT_HDR);
    idx += res;
    bufsz -= res;

    res = scnprintf(&buf[idx], bufsz, "\n");
    idx += res;
    bufsz -= res;

    for (i = 0; i < 50; i++) {

	    res = scnprintf(&buf[idx], bufsz, DBG_CDT_HDR_FMT, 
								bl_hw->dbg_credit[i].sdio_port,
								bl_hw->dbg_credit[i].txq_credit,
								bl_hw->dbg_credit[i].hwq_credit,
								bl_hw->dbg_credit[i].credit,
								bl_hw->dbg_credit[i].nb_ready,
								bl_hw->dbg_credit[i].txq_idx);
	    idx += res;
	    bufsz -= res;
        res = scnprintf(&buf[idx], bufsz, "\n");
        idx += res;
        bufsz -= res;
    }

    res = scnprintf(&buf[idx], bufsz, "\n");

	read = simple_read_from_buffer(user_buf, count, ppos, buf, idx);
    kfree(buf);

    return read;

}


static ssize_t bl_dbgfs_dbg_cdt_write(struct file *file,
                                      const char __user *user_buf,
                                      size_t count, loff_t *ppos)
{
	return count;
}

DEBUGFS_READ_WRITE_FILE_OPS(dbg_cdt);

static ssize_t bl_dbgfs_stats_read(struct file *file,
                                     char __user *user_buf,
                                     size_t count, loff_t *ppos)
{
    struct bl_hw *priv = file->private_data;
    char *buf;
    int ret;
    int i, skipped;
    ssize_t read;
#ifdef CONFIG_BL_SPLIT_TX_BUF
	int per;
#endif
		
    int bufsz = (NX_TXQ_CNT) * 20 + (ARRAY_SIZE(priv->stats.amsdus_rx) + 1) * 40
        + (ARRAY_SIZE(priv->stats.ampdus_tx) * 30);

    if (*ppos)
        return 0;

    buf = kmalloc(bufsz, GFP_ATOMIC);
    if (buf == NULL)
        return 0;

    ret = scnprintf(buf, bufsz, "TXQs CFM balances ");
    for (i = 0; i < NX_TXQ_CNT; i++)
        ret += scnprintf(&buf[ret], bufsz - ret,
                         "  [%1d]:%3d", i,
                         priv->stats.cfm_balance[i]);

    ret += scnprintf(&buf[ret], bufsz - ret, "\n");

#ifdef CONFIG_BL_SPLIT_TX_BUF
    ret += scnprintf(&buf[ret], bufsz - ret,
                     "\nAMSDU[len]       done         failed   received\n");
    for (i = skipped = 0; i < NX_TX_PAYLOAD_MAX; i++) {
        if (priv->stats.amsdus[i].done) {
            per = DIV_ROUND_UP((priv->stats.amsdus[i].failed) *
                               100, priv->stats.amsdus[i].done);
        } else if (priv->stats.amsdus_rx[i]) {
            per = 0;
        } else {
            per = 0;
            skipped = 1;
            continue;
        }
        if (skipped) {
            ret += scnprintf(&buf[ret], bufsz - ret, "   ...\n");
            skipped = 0;
        }

        ret += scnprintf(&buf[ret], bufsz - ret,
                         "   [%2d]    %10d %8d(%3d%%) %10d\n",  i ? i + 1 : i,
                         priv->stats.amsdus[i].done,
                         priv->stats.amsdus[i].failed, per,
                         priv->stats.amsdus_rx[i]);
    }

    for (; i < ARRAY_SIZE(priv->stats.amsdus_rx); i++) {
        if (!priv->stats.amsdus_rx[i]) {
            skipped = 1;
            continue;
        }
        if (skipped) {
            ret += scnprintf(&buf[ret], bufsz - ret, "   ...\n");
            skipped = 0;
        }

        ret += scnprintf(&buf[ret], bufsz - ret,
                         "   [%2d]                              %10d\n",
                         i + 1, priv->stats.amsdus_rx[i]);
    }
#else
    ret += scnprintf(&buf[ret], bufsz - ret,
                     "\nAMSDU[len]   received\n");
    for (i = skipped = 0; i < ARRAY_SIZE(priv->stats.amsdus_rx); i++) {
        if (!priv->stats.amsdus_rx[i]) {
            skipped = 1;
            continue;
        }
        if (skipped) {
            ret += scnprintf(&buf[ret], bufsz - ret,
                             "   ...\n");
            skipped = 0;
        }

        ret += scnprintf(&buf[ret], bufsz - ret,
                         "   [%2d]    %10d\n",
                         i + 1, priv->stats.amsdus_rx[i]);
    }

#endif /* CONFIG_BL_SPLIT_TX_BUF */

    ret += scnprintf(&buf[ret], bufsz - ret,
                     "\nAMPDU[len]     done  received\n");
    for (i = skipped = 0; i < ARRAY_SIZE(priv->stats.ampdus_tx); i++) {
        if (!priv->stats.ampdus_tx[i] && !priv->stats.ampdus_rx[i]) {
            skipped = 1;
            continue;
        }
        if (skipped) {
            ret += scnprintf(&buf[ret], bufsz - ret,
                             "    ...\n");
            skipped = 0;
        }

        ret += scnprintf(&buf[ret], bufsz - ret,
                         "   [%2d]   %9d %9d\n", i ? i + 1 : i,
                         priv->stats.ampdus_tx[i], priv->stats.ampdus_rx[i]);
    }

    ret += scnprintf(&buf[ret], bufsz - ret,
                     "#mpdu missed        %9d\n",
                     priv->stats.ampdus_rx_miss);
    read = simple_read_from_buffer(user_buf, count, ppos, buf, ret);

    kfree(buf);

    return read;
}

static ssize_t bl_dbgfs_stats_write(struct file *file,
                                      const char __user *user_buf,
                                      size_t count, loff_t *ppos)
{
    struct bl_hw *priv = file->private_data;

    /* Prevent from interrupt preemption as these statistics are updated under
     * interrupt */
    spin_lock_bh(&priv->tx_lock);

    memset(&priv->stats, 0, sizeof(priv->stats));

    spin_unlock_bh(&priv->tx_lock);

    return count;
}

DEBUGFS_READ_WRITE_FILE_OPS(stats);

#define TXQ_STA_PREF "tid|"
#define TXQ_STA_PREF_FMT "%3d|"

#ifdef CONFIG_BL_FULLMAC
#define TXQ_VIF_PREF "type|"
#define TXQ_VIF_PREF_FMT "%4s|"
#else
#define TXQ_VIF_PREF "AC|"
#define TXQ_VIF_PREF_FMT "%2s|"
#endif /* CONFIG_BL_FULLMAC */

#define TXQ_HDR "idx| status|credit|ready|retry"
#define TXQ_HDR_FMT "%3d|%s%s%s%s%s%s%s|%6d|%5d|%5d"

#ifdef CONFIG_BL_AMSDUS_TX
#ifdef CONFIG_BL_FULLMAC
#define TXQ_HDR_SUFF "|amsdu"
#define TXQ_HDR_SUFF_FMT "|%5d"
#else
#define TXQ_HDR_SUFF "|amsdu-ht|amdsu-vht"
#define TXQ_HDR_SUFF_FMT "|%8d|%9d"
#endif /* CONFIG_BL_FULLMAC */
#else
#define TXQ_HDR_SUFF ""
#define TXQ_HDR_SUF_FMT ""
#endif /* CONFIG_BL_AMSDUS_TX */

#define TXQ_HDR_MAX_LEN (sizeof(TXQ_STA_PREF) + sizeof(TXQ_HDR) + sizeof(TXQ_HDR_SUFF) + 1)

#ifdef CONFIG_BL_FULLMAC
#define PS_HDR  "Legacy PS: ready=%d, sp=%d / UAPSD: ready=%d, sp=%d"
#define PS_HDR_LEGACY "Legacy PS: ready=%d, sp=%d"
#define PS_HDR_UAPSD  "UAPSD: ready=%d, sp=%d"
#define PS_HDR_MAX_LEN  sizeof("Legacy PS: ready=xxx, sp=xxx / UAPSD: ready=xxx, sp=xxx\n")
#else
#define PS_HDR ""
#define PS_HDR_MAX_LEN 0
#endif /* CONFIG_BL_FULLMAC */

#define STA_HDR "** STA %d (%pM)\n"
#define STA_HDR_MAX_LEN sizeof("- STA xx (xx:xx:xx:xx:xx:xx)\n") + PS_HDR_MAX_LEN

#ifdef CONFIG_BL_FULLMAC
#define VIF_HDR "* VIF [%d] %s\n"
#define VIF_HDR_MAX_LEN sizeof(VIF_HDR) + IFNAMSIZ
#else
#define VIF_HDR "* VIF [%d]\n"
#define VIF_HDR_MAX_LEN sizeof(VIF_HDR)
#endif


#ifdef CONFIG_BL_AMSDUS_TX

#ifdef CONFIG_BL_FULLMAC
#define VIF_SEP "---------------------------------------\n"
#else
#define VIF_SEP "----------------------------------------------------\n"
#endif /* CONFIG_BL_FULLMAC */

#else /* ! CONFIG_BL_AMSDUS_TX */
#define VIF_SEP "---------------------------------\n"
#endif /* CONFIG_BL_AMSDUS_TX*/

#define VIF_SEP_LEN sizeof(VIF_SEP)

#define CAPTION "status: L=in hwq list, F=stop full, P=stop sta PS, V=stop vif PS, C=stop channel, S=stop CSA, M=stop MU"
#define CAPTION_LEN sizeof(CAPTION)

#define STA_TXQ 0
#define VIF_TXQ 1

static int bl_dbgfs_txq(char *buf, size_t size, struct bl_txq *txq, int type, int tid, char *name)
{
    int res, idx = 0;

    if (type == STA_TXQ) {
        res = scnprintf(&buf[idx], size, TXQ_STA_PREF_FMT, tid);
        idx += res;
        size -= res;
    } else {
        res = scnprintf(&buf[idx], size, TXQ_VIF_PREF_FMT, name);
        idx += res;
        size -= res;
    }

    res = scnprintf(&buf[idx], size, TXQ_HDR_FMT, txq->idx,
                    (txq->status & BL_TXQ_IN_HWQ_LIST) ? "L" : " ",
                    (txq->status & BL_TXQ_STOP_FULL) ? "F" : " ",
                    (txq->status & BL_TXQ_STOP_STA_PS) ? "P" : " ",
                    (txq->status & BL_TXQ_STOP_VIF_PS) ? "V" : " ",
                    (txq->status & BL_TXQ_STOP_CHAN) ? "C" : " ",
                    (txq->status & BL_TXQ_STOP_CSA) ? "S" : " ",
                    (txq->status & BL_TXQ_STOP_MU_POS) ? "M" : " ",
                    txq->credits, skb_queue_len(&txq->sk_list),
                    txq->nb_retry);
    idx += res;
    size -= res;

#ifdef CONFIG_BL_AMSDUS_TX
    if (type == STA_TXQ) {
        res = scnprintf(&buf[idx], size, TXQ_HDR_SUFF_FMT,
#ifdef CONFIG_BL_FULLMAC
                        txq->amsdu_len
#else
                        txq->amsdu_ht_len_cap, txq->amsdu_vht_len_cap
#endif /* CONFIG_BL_FULLMAC */
                        );
        idx += res;
        size -= res;
    }
#endif

    res = scnprintf(&buf[idx], size, "\n");
    idx += res;
    size -= res;

    return idx;
}

static int bl_dbgfs_txq_sta(char *buf, size_t size, struct bl_sta *bl_sta,
                              struct bl_hw *bl_hw)
{
    int tid, res, idx = 0;
    struct bl_txq *txq;

    res = scnprintf(&buf[idx], size, "\n" STA_HDR,
                    bl_sta->sta_idx,
                    bl_sta->mac_addr
                    );
    idx += res;
    size -= res;

#ifdef CONFIG_BL_FULLMAC
    if (bl_sta->ps.active) {
        if (bl_sta->uapsd_tids &&
            (bl_sta->uapsd_tids == ((1 << NX_NB_TXQ_PER_STA) - 1)))
            res = scnprintf(&buf[idx], size, PS_HDR_UAPSD "\n",
                            bl_sta->ps.pkt_ready[UAPSD_ID],
                            bl_sta->ps.sp_cnt[UAPSD_ID]);
        else if (bl_sta->uapsd_tids)
            res = scnprintf(&buf[idx], size, PS_HDR "\n",
                            bl_sta->ps.pkt_ready[LEGACY_PS_ID],
                            bl_sta->ps.sp_cnt[LEGACY_PS_ID],
                            bl_sta->ps.pkt_ready[UAPSD_ID],
                            bl_sta->ps.sp_cnt[UAPSD_ID]);
        else
            res = scnprintf(&buf[idx], size, PS_HDR_LEGACY "\n",
                            bl_sta->ps.pkt_ready[LEGACY_PS_ID],
                            bl_sta->ps.sp_cnt[LEGACY_PS_ID]);
        idx += res;
        size -= res;
    } else {
        res = scnprintf(&buf[idx], size, "\n");
        idx += res;
        size -= res;
    }
#endif /* CONFIG_BL_FULLMAC */


    res = scnprintf(&buf[idx], size, TXQ_STA_PREF TXQ_HDR TXQ_HDR_SUFF "\n");
    idx += res;
    size -= res;

#ifdef CONFIG_BL_FULLMAC
    txq = bl_txq_sta_get(bl_sta, 0, NULL, bl_hw);
#else
    txq = bl_txq_sta_get(bl_sta, 0, NULL);
#endif /* CONFIG_BL_FULLMAC */

    for (tid = 0; tid < NX_NB_TXQ_PER_STA; tid++, txq++) {
        res = bl_dbgfs_txq(&buf[idx], size, txq, STA_TXQ, tid, NULL);
        idx += res;
        size -= res;
    }

    return idx;
}

static int bl_dbgfs_txq_vif(char *buf, size_t size, struct bl_vif *bl_vif,
                              struct bl_hw *bl_hw)
{
    int res, idx = 0;
    struct bl_txq *txq;
    struct bl_sta *bl_sta;

#ifdef CONFIG_BL_FULLMAC
    res = scnprintf(&buf[idx], size, VIF_HDR, bl_vif->vif_index, bl_vif->ndev->name);
    idx += res;
    size -= res;
    if (!bl_vif->up || bl_vif->ndev == NULL)
        return idx;

#else
    int i;
    char ac_name[2] = {'0', '\0'};

    res = scnprintf(&buf[idx], size, VIF_HDR, bl_vif->vif_index);
    idx += res;
    size -= res;
#endif /* CONFIG_BL_FULLMAC */

    #ifdef CONFIG_BL_FULLMAC
    if (BL_VIF_TYPE(bl_vif) ==  NL80211_IFTYPE_AP ||
        BL_VIF_TYPE(bl_vif) ==  NL80211_IFTYPE_P2P_GO) {
        res = scnprintf(&buf[idx], size, TXQ_VIF_PREF TXQ_HDR "\n");
        idx += res;
        size -= res;
        txq = bl_txq_vif_get(bl_vif, NX_UNK_TXQ_TYPE, NULL);
        res = bl_dbgfs_txq(&buf[idx], size, txq, VIF_TXQ, 0, "UNK");
        idx += res;
        size -= res;
        txq = bl_txq_vif_get(bl_vif, NX_BCMC_TXQ_TYPE, NULL);
        res = bl_dbgfs_txq(&buf[idx], size, txq, VIF_TXQ, 0, "BCMC");
        idx += res;
        size -= res;
        bl_sta = &bl_hw->sta_table[bl_vif->ap.bcmc_index];
        if (bl_sta->ps.active) {
            res = scnprintf(&buf[idx], size, PS_HDR_LEGACY "\n",
                            bl_sta->ps.sp_cnt[LEGACY_PS_ID],
                            bl_sta->ps.sp_cnt[LEGACY_PS_ID]);
            idx += res;
            size -= res;
        } else {
            res = scnprintf(&buf[idx], size, "\n");
            idx += res;
            size -= res;
        }

        list_for_each_entry(bl_sta, &bl_vif->ap.sta_list, list) {
            res = bl_dbgfs_txq_sta(&buf[idx], size, bl_sta, bl_hw);
            idx += res;
            size -= res;
        }
    } else if (BL_VIF_TYPE(bl_vif) ==  NL80211_IFTYPE_STATION ||
               BL_VIF_TYPE(bl_vif) ==  NL80211_IFTYPE_P2P_CLIENT) {
        if (bl_vif->sta.ap) {
            res = bl_dbgfs_txq_sta(&buf[idx], size, bl_vif->sta.ap, bl_hw);
            idx += res;
            size -= res;
        }
    }

    #else
    res = scnprintf(&buf[idx], size, TXQ_VIF_PREF TXQ_HDR "\n");
    idx += res;
    size -= res;
    txq = bl_txq_vif_get(bl_vif, 0, NULL);
    for (i = 0; i < NX_NB_TXQ_PER_VIF; i++, txq++) {
        ac_name[0]++;
        res = bl_dbgfs_txq(&buf[idx], size, txq, VIF_TXQ, 0, ac_name);
        idx += res;
        size -= res;
    }

    list_for_each_entry(bl_sta, &bl_vif->stations, list) {
        res = bl_dbgfs_txq_sta(&buf[idx], size, bl_sta, bl_hw);
        idx += res;
        size -= res;
    }
    #endif /* CONFIG_BL_FULLMAC */
    return idx;
}

static ssize_t bl_dbgfs_txq_read(struct file *file ,
                                   char __user *user_buf,
                                   size_t count, loff_t *ppos)
{
    struct bl_hw *bl_hw = file->private_data;
    struct bl_vif *vif;
    char *buf;
    int idx, res;
    ssize_t read;
    size_t bufsz = ((NX_VIRT_DEV_MAX * (VIF_HDR_MAX_LEN + 2 * VIF_SEP_LEN)) +
                    (NX_REMOTE_STA_MAX * STA_HDR_MAX_LEN) +
                    ((NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX + NX_NB_TXQ) *
                     TXQ_HDR_MAX_LEN) + CAPTION_LEN);

    /* everything is read in one go */
    if (*ppos)
        return 0;

    bufsz = min_t(size_t, bufsz, count);
    buf = kmalloc(bufsz, GFP_ATOMIC);
    if (buf == NULL)
        return 0;

    bufsz--;
    idx = 0;

    res = scnprintf(&buf[idx], bufsz, CAPTION);
    idx += res;
    bufsz -= res;

    //spin_lock_bh(&bl_hw->tx_lock);
    list_for_each_entry(vif, &bl_hw->vifs, list) {
        res = scnprintf(&buf[idx], bufsz, "\n"VIF_SEP);
        idx += res;
        bufsz -= res;
        res = bl_dbgfs_txq_vif(&buf[idx], bufsz, vif, bl_hw);
        idx += res;
        bufsz -= res;
        res = scnprintf(&buf[idx], bufsz, VIF_SEP);
        idx += res;
        bufsz -= res;
    }
    //spin_unlock_bh(&bl_hw->tx_lock);

    read = simple_read_from_buffer(user_buf, count, ppos, buf, idx);
    kfree(buf);

    return read;
}
DEBUGFS_READ_FILE_OPS(txq);

static ssize_t bl_dbgfs_rhd_read(struct file *file,
                                   char __user *user_buf,
                                   size_t count, loff_t *ppos)
{
    struct bl_hw *priv = file->private_data;
    ssize_t read;

    mutex_lock(&priv->dbginfo.mutex);
    if (!priv->debugfs.trace_prst) {
        mutex_unlock(&priv->dbginfo.mutex);
        return 0;
    }

    read = simple_read_from_buffer(user_buf, count, ppos,
                                   priv->dbginfo.buf->rhd_mem,
                                   priv->dbginfo.buf->dbg_info.rhd_len);

    mutex_unlock(&priv->dbginfo.mutex);
    return read;
}

DEBUGFS_READ_FILE_OPS(rhd);

static ssize_t bl_dbgfs_rbd_read(struct file *file,
                                   char __user *user_buf,
                                   size_t count, loff_t *ppos)
{
    struct bl_hw *priv = file->private_data;
    ssize_t read;

    mutex_lock(&priv->dbginfo.mutex);
    if (!priv->debugfs.trace_prst) {
        mutex_unlock(&priv->dbginfo.mutex);
        return 0;
    }

    read = simple_read_from_buffer(user_buf, count, ppos,
                                   priv->dbginfo.buf->rbd_mem,
                                   priv->dbginfo.buf->dbg_info.rbd_len);

    mutex_unlock(&priv->dbginfo.mutex);
    return read;
}

DEBUGFS_READ_FILE_OPS(rbd);

static ssize_t bl_dbgfs_thdx_read(struct file *file, char __user *user_buf,
                                    size_t count, loff_t *ppos, int idx)
{
    struct bl_hw *priv = file->private_data;
    ssize_t read;

    mutex_lock(&priv->dbginfo.mutex);
    if (!priv->debugfs.trace_prst) {
        mutex_unlock(&priv->dbginfo.mutex);
        return 0;
    }

    read = simple_read_from_buffer(user_buf, count, ppos,
                                   &priv->dbginfo.buf->thd_mem[idx],
                                   priv->dbginfo.buf->dbg_info.thd_len[idx]);

    mutex_unlock(&priv->dbginfo.mutex);
    return read;
}

static ssize_t bl_dbgfs_thd0_read(struct file *file,
                                    char __user *user_buf,
                                    size_t count, loff_t *ppos)
{
    return bl_dbgfs_thdx_read(file, user_buf, count, ppos, 0);
}
DEBUGFS_READ_FILE_OPS(thd0);

static ssize_t bl_dbgfs_thd1_read(struct file *file,
                                    char __user *user_buf,
                                    size_t count, loff_t *ppos)
{
    return bl_dbgfs_thdx_read(file, user_buf, count, ppos, 1);
}
DEBUGFS_READ_FILE_OPS(thd1);

static ssize_t bl_dbgfs_thd2_read(struct file *file,
                                    char __user *user_buf,
                                    size_t count, loff_t *ppos)
{
    return bl_dbgfs_thdx_read(file, user_buf, count, ppos, 2);
}
DEBUGFS_READ_FILE_OPS(thd2);

static ssize_t bl_dbgfs_thd3_read(struct file *file,
                                    char __user *user_buf,
                                    size_t count, loff_t *ppos)
{
    return bl_dbgfs_thdx_read(file, user_buf, count, ppos, 3);
}
DEBUGFS_READ_FILE_OPS(thd3);

#if (NX_TXQ_CNT == 5)
static ssize_t bl_dbgfs_thd4_read(struct file *file,
                                    char __user *user_buf,
                                    size_t count, loff_t *ppos)
{
    return bl_dbgfs_thdx_read(file, user_buf, count, ppos, 4);
}
DEBUGFS_READ_FILE_OPS(thd4);
#endif

static ssize_t bl_dbgfs_mactrace_read(struct file *file,
                                        char __user *user_buf,
                                        size_t count, loff_t *ppos)
{
    struct bl_hw *priv = file->private_data;
    ssize_t read;

    mutex_lock(&priv->dbginfo.mutex);
    read = simple_read_from_buffer(user_buf, count, ppos,
                                   priv->dbginfo.buf->la_mem,
                                   priv->dbginfo.buf->dbg_info.la_conf.trace_len);

    mutex_unlock(&priv->dbginfo.mutex);
    return read;
}
DEBUGFS_READ_FILE_OPS(mactrace);

static ssize_t bl_dbgfs_mactctrig_read(struct file *file,
                                        char __user *user_buf,
                                        size_t count, loff_t *ppos)
{
    struct bl_hw *priv = file->private_data;
    ssize_t read;

    mutex_lock(&priv->dbginfo.mutex);

	priv->debugfs.trace_prst = false;

    if (!priv->debugfs.trace_prst) {
        char msg[64];

		printk("sent trace trigger msg\n");

        scnprintf(msg, sizeof(msg), "Force trigger\n");
        bl_send_dbg_trigger_req(priv, msg);

        mutex_unlock(&priv->dbginfo.mutex);
        return 0;
    }

    mutex_unlock(&priv->dbginfo.mutex);
    return read;
}
DEBUGFS_READ_FILE_OPS(mactctrig);


static ssize_t bl_dbgfs_macdiags_read(struct file *file,
                                        char __user *user_buf,
                                        size_t count, loff_t *ppos)
{
    struct bl_hw *priv = file->private_data;
    ssize_t read;

    mutex_lock(&priv->dbginfo.mutex);
    if (!priv->debugfs.trace_prst) {
        mutex_unlock(&priv->dbginfo.mutex);
        return 0;
    }

    read = simple_read_from_buffer(user_buf, count, ppos,
                                   priv->dbginfo.buf->dbg_info.diags_mac,
                                   DBG_DIAGS_MAC_MAX * 2);

    mutex_unlock(&priv->dbginfo.mutex);
    return read;
}

DEBUGFS_READ_FILE_OPS(macdiags);

static ssize_t bl_dbgfs_phydiags_read(struct file *file,
                                        char __user *user_buf,
                                        size_t count, loff_t *ppos)
{
    struct bl_hw *priv = file->private_data;
    ssize_t read;

    mutex_lock(&priv->dbginfo.mutex);
    if (!priv->debugfs.trace_prst) {
        mutex_unlock(&priv->dbginfo.mutex);
        return 0;
    }

    read = simple_read_from_buffer(user_buf, count, ppos,
                                   priv->dbginfo.buf->dbg_info.diags_phy,
                                   DBG_DIAGS_PHY_MAX * 2);

    mutex_unlock(&priv->dbginfo.mutex);
    return read;
}

DEBUGFS_READ_FILE_OPS(phydiags);

static ssize_t bl_dbgfs_hwdiags_read(struct file *file,
                                       char __user *user_buf,
                                       size_t count, loff_t *ppos)
{
    struct bl_hw *priv = file->private_data;
    char buf[16];
    int ret;

    mutex_lock(&priv->dbginfo.mutex);
    if (!priv->debugfs.trace_prst) {
        mutex_unlock(&priv->dbginfo.mutex);
        return 0;
    }

    ret = scnprintf(buf, min_t(size_t, sizeof(buf) - 1, count),
                    "%08X\n", priv->dbginfo.buf->dbg_info.hw_diag);

    mutex_unlock(&priv->dbginfo.mutex);
    return simple_read_from_buffer(user_buf, count, ppos, buf, ret);
}

DEBUGFS_READ_FILE_OPS(hwdiags);

static ssize_t bl_dbgfs_plfdiags_read(struct file *file,
                                       char __user *user_buf,
                                       size_t count, loff_t *ppos)
{
    struct bl_hw *priv = file->private_data;
    char buf[16];
    int ret;

    mutex_lock(&priv->dbginfo.mutex);
    if (!priv->debugfs.trace_prst) {
        mutex_unlock(&priv->dbginfo.mutex);
        return 0;
    }

    ret = scnprintf(buf, min_t(size_t, sizeof(buf) - 1, count),
                    "%08X\n", priv->dbginfo.buf->dbg_info.la_conf.diag_conf);

    mutex_unlock(&priv->dbginfo.mutex);
    return simple_read_from_buffer(user_buf, count, ppos, buf, ret);
}

DEBUGFS_READ_FILE_OPS(plfdiags);

static ssize_t bl_dbgfs_swdiags_read(struct file *file,
                                      char __user *user_buf,
                                      size_t count, loff_t *ppos)
{
    struct bl_hw *priv = file->private_data;
    ssize_t read;

    mutex_lock(&priv->dbginfo.mutex);
    if (!priv->debugfs.trace_prst) {
        mutex_unlock(&priv->dbginfo.mutex);
        return 0;
    }

    read = simple_read_from_buffer(user_buf, count, ppos,
                                   &priv->dbginfo.buf->dbg_info.sw_diag,
                                   priv->dbginfo.buf->dbg_info.sw_diag_len);

    mutex_unlock(&priv->dbginfo.mutex);
    return read;
}

DEBUGFS_READ_FILE_OPS(swdiags);

static ssize_t bl_dbgfs_error_read(struct file *file,
                                     char __user *user_buf,
                                     size_t count, loff_t *ppos)
{
    struct bl_hw *priv = file->private_data;
    ssize_t read;

    mutex_lock(&priv->dbginfo.mutex);
    if (!priv->debugfs.trace_prst) {
        mutex_unlock(&priv->dbginfo.mutex);
        return 0;
    }

    read = simple_read_from_buffer(user_buf, count, ppos,
                                   priv->dbginfo.buf->dbg_info.error,
                                   strlen((char *)priv->dbginfo.buf->dbg_info.error));

    mutex_unlock(&priv->dbginfo.mutex);
    return read;
}

DEBUGFS_READ_FILE_OPS(error);

static ssize_t bl_dbgfs_rxdesc_read(struct file *file,
                                      char __user *user_buf,
                                      size_t count, loff_t *ppos)
{
    struct bl_hw *priv = file->private_data;
    char buf[32];
    int ret;
    ssize_t read;

    mutex_lock(&priv->dbginfo.mutex);
    if (!priv->debugfs.trace_prst) {
        mutex_unlock(&priv->dbginfo.mutex);
        return 0;
    }

    ret = scnprintf(buf, min_t(size_t, sizeof(buf) - 1, count),
                    "%08X\n%08X\n", priv->dbginfo.buf->dbg_info.rhd,
                    priv->dbginfo.buf->dbg_info.rbd);
    read = simple_read_from_buffer(user_buf, count, ppos, buf, ret);

    mutex_unlock(&priv->dbginfo.mutex);
    return read;
}

DEBUGFS_READ_FILE_OPS(rxdesc);

static ssize_t bl_dbgfs_txdesc_read(struct file *file,
                                      char __user *user_buf,
                                      size_t count, loff_t *ppos)
{
    struct bl_hw *priv = file->private_data;
    char buf[64];
    int len = 0;
    int i;

    mutex_lock(&priv->dbginfo.mutex);
    if (!priv->debugfs.trace_prst) {
        mutex_unlock(&priv->dbginfo.mutex);
        return 0;
    }

    for (i = 0; i < NX_TXQ_CNT; i++) {
        len += scnprintf(&buf[len], min_t(size_t, sizeof(buf) - len - 1, count),
                         "%08X\n", priv->dbginfo.buf->dbg_info.thd[i]);
    }

    mutex_unlock(&priv->dbginfo.mutex);
    return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

DEBUGFS_READ_FILE_OPS(txdesc);

static ssize_t bl_dbgfs_macrxptr_read(struct file *file,
                                        char __user *user_buf,
                                        size_t count, loff_t *ppos)
{
    struct bl_hw *priv = file->private_data;
    ssize_t read;

    mutex_lock(&priv->dbginfo.mutex);
    if (!priv->debugfs.trace_prst) {
        mutex_unlock(&priv->dbginfo.mutex);
        return 0;
    }

    read = simple_read_from_buffer(user_buf, count, ppos,
                                   &priv->dbginfo.buf->dbg_info.rhd_hw_ptr,
                            2 * sizeof(priv->dbginfo.buf->dbg_info.rhd_hw_ptr));

    mutex_unlock(&priv->dbginfo.mutex);
    return read;
}

DEBUGFS_READ_FILE_OPS(macrxptr);

static ssize_t bl_dbgfs_lamacconf_read(struct file *file,
                                         char __user *user_buf,
                                         size_t count, loff_t *ppos)
{
    struct bl_hw *priv = file->private_data;
    ssize_t read;

    mutex_lock(&priv->dbginfo.mutex);
    if (!priv->debugfs.trace_prst) {
        mutex_unlock(&priv->dbginfo.mutex);
        return 0;
    }

    read = simple_read_from_buffer(user_buf, count, ppos,
                                   priv->dbginfo.buf->dbg_info.la_conf.conf,
                                   LA_CONF_LEN * 4);

    mutex_unlock(&priv->dbginfo.mutex);
    return read;
}
DEBUGFS_READ_FILE_OPS(lamacconf);

static ssize_t bl_dbgfs_chaninfo_read(struct file *file,
                                        char __user *user_buf,
                                        size_t count, loff_t *ppos)
{
    struct bl_hw *priv = file->private_data;
    char buf[4 * 32];
    int ret;

    mutex_lock(&priv->dbginfo.mutex);
    if (!priv->debugfs.trace_prst) {
        mutex_unlock(&priv->dbginfo.mutex);
        return 0;
    }

    ret = scnprintf(buf, min_t(size_t, sizeof(buf) - 1, count),
                    "type:          %d\n"
                    "prim20_freq:   %d MHz\n"
                    "center1_freq:  %d MHz\n"
                    "center2_freq:  %d MHz\n",
                    (priv->dbginfo.buf->dbg_info.chan_info.info1 >> 8) & 0xFF,
                    (priv->dbginfo.buf->dbg_info.chan_info.info1 >> 16) & 0xFFFF,
                    (priv->dbginfo.buf->dbg_info.chan_info.info2 >> 0) & 0xFFFF,
                    (priv->dbginfo.buf->dbg_info.chan_info.info2 >> 16) & 0xFFFF);

    mutex_unlock(&priv->dbginfo.mutex);
    return simple_read_from_buffer(user_buf, count, ppos, buf, ret);
}

DEBUGFS_READ_FILE_OPS(chaninfo);

static ssize_t bl_dbgfs_acsinfo_read(struct file *file,
                                           char __user *user_buf,
                                           size_t count, loff_t *ppos)
{
    struct bl_hw *priv = file->private_data;
    struct wiphy *wiphy = priv->wiphy;
    int survey_cnt = 0;
    int len = 0;
    int band, chan_cnt;
    char *buf = NULL;
    int  buf_len = (SCAN_CHANNEL_MAX + 1) * 43;
	size_t ret = 0;

    buf = kzalloc(buf_len, GFP_KERNEL);
    if(!buf)
		return ret;
	
    mutex_lock(&priv->dbginfo.mutex);

    len += scnprintf(buf, min_t(size_t, buf_len - 1, count),
                     "FREQ    TIME(ms)    BUSY(ms)    NOISE(dBm)\n");

    for (band = NL80211_BAND_2GHZ; band <= NL80211_BAND_5GHZ; band++) {
        for (chan_cnt = 0; chan_cnt < wiphy->bands[band]->n_channels; chan_cnt++) {
            struct bl_survey_info *p_survey_info = &priv->survey[survey_cnt];
            struct ieee80211_channel *p_chan = &wiphy->bands[band]->channels[chan_cnt];

            if (p_survey_info->filled) {
                len += scnprintf(buf + len, min_t(size_t, buf_len - len - 1, count),
                                 "%d    %03d         %03d         %d\n",
                                 p_chan->center_freq,
                                 p_survey_info->chan_time_ms,
                                 p_survey_info->chan_time_busy_ms,
                                 p_survey_info->noise_dbm);
            } else {
                len += scnprintf(buf + len, min_t(size_t, buf_len -len -1, count),
                                 "%d    NOT AVAILABLE\n",
                                 p_chan->center_freq);
            }

            survey_cnt++;
        }
    }

    mutex_unlock(&priv->dbginfo.mutex);
	
    ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);

    kfree(buf);
    buf = NULL;
    return ret;
}

DEBUGFS_READ_FILE_OPS(acsinfo);

static ssize_t bl_dbgfs_fw_dbg_read(struct file *file,
                                           char __user *user_buf,
                                           size_t count, loff_t *ppos)
{
    char help[]="usage: [MOD:<ALL|KE|DBG|IPC|DMA|MM|TX|RX|PHY>]* "
        "[DBG:<NONE|CRT|ERR|WRN|INF|VRB>]\n";

    return simple_read_from_buffer(user_buf, count, ppos, help, sizeof(help));
}


static ssize_t bl_dbgfs_fw_dbg_write(struct file *file,
                                            const char __user *user_buf,
                                            size_t count, loff_t *ppos)
{
	return 0;
}

DEBUGFS_READ_WRITE_FILE_OPS(fw_dbg);

static ssize_t bl_dbgfs_sys_stats_read(struct file *file,
                                         char __user *user_buf,
                                         size_t count, loff_t *ppos)
{
	return 0;
}

DEBUGFS_READ_FILE_OPS(sys_stats);

static ssize_t bl_dbgfs_um_helper_read(struct file *file,
                                         char __user *user_buf,
                                         size_t count, loff_t *ppos)
{
    struct bl_hw *priv = file->private_data;
    char buf[sizeof(priv->debugfs.helper_cmd)];
    int ret;

    ret = scnprintf(buf, min_t(size_t, sizeof(buf) - 1, count),
                    "%s", priv->debugfs.helper_cmd);

    return simple_read_from_buffer(user_buf, count, ppos, buf, ret);
}

static ssize_t bl_dbgfs_um_helper_write(struct file *file,
                                          const char __user *user_buf,
                                          size_t count, loff_t *ppos)
{
    struct bl_hw *priv = file->private_data;
    int eobuf = min_t(size_t, sizeof(priv->debugfs.helper_cmd) - 1, count);

    priv->debugfs.helper_cmd[eobuf] = '\0';
    if (copy_from_user(priv->debugfs.helper_cmd, user_buf, eobuf))
        return -EFAULT;

    return count;
}

DEBUGFS_READ_WRITE_FILE_OPS(um_helper);


static ssize_t bl_dbgfs_sdio_test_read(struct file *file,
                                         char __user *user_buf,
                                         size_t count, loff_t *ppos)
{
    struct bl_hw *priv = file->private_data;
    u8 *buf = NULL;
	int pkt_len;
    int ret = 0;
	u32 port;
	ssize_t read;
	int i = 0;

	bl_get_rd_port(priv, &port);
	bl_get_rd_len(priv, 0x8, 0x9, &pkt_len);
	buf = kzalloc(pkt_len, GFP_KERNEL);
	bl_read_data_sync(priv, buf, pkt_len, priv->plat->io_port + port);

	for(i=0; i<pkt_len; i++)
	{
		if(i%16 == 0)
			printk("\n");
		printk("0x%x ",buf[i]);
	}
	printk("\n");
 
	read = simple_read_from_buffer(user_buf, count, ppos, buf, ret);

	kfree(buf);
	buf = NULL;
	return read;
}

static ssize_t bl_dbgfs_sdio_test_write(struct file *file,
                                          const char __user *user_buf,
                                          size_t count, loff_t *ppos)
{
    struct bl_hw *priv = file->private_data;
	char buf[32];
	u8 *send_buf = NULL;
	int i;
	u32 port;
	int val;
	int pkt_len;
	int act_len;
    size_t len = min_t(size_t, count, sizeof(buf) - 1);

    if (copy_from_user(buf, user_buf, len))
        return -EFAULT;

    buf[len] = '\0';

	if (sscanf(buf, "0x%x-%d", &val, &pkt_len) > 0)
	{
		printk("val=0x%x, pkt_len=0x%x\n", val, pkt_len);
	}

	act_len = pkt_len;

	pkt_len = ((pkt_len + BL_SDIO_BLOCK_SIZE -1)/BL_SDIO_BLOCK_SIZE) * BL_SDIO_BLOCK_SIZE;

	printk("after adjust: alloc %d for pkt_buf\n", pkt_len);

	send_buf = kzalloc(pkt_len, GFP_KERNEL);

	for(i = 0; i < pkt_len; i++)
	{
		send_buf[i] = val;
	}

	bl_get_wr_port(priv, &port);

    bl_write_data_sync(priv, send_buf, act_len, priv->plat->io_port + port);

	kfree(send_buf);

	send_buf = NULL;

    return count;
}

DEBUGFS_READ_WRITE_FILE_OPS(sdio_test);

static ssize_t bl_dbgfs_rw_reg_read(struct file *file,
                                         char __user *user_buf,
                                         size_t count, loff_t *ppos)
{
	struct bl_hw *bl_hw = file->private_data;
	struct bl_device *bl_device;
	struct bl_plat *bl_plat;
	u8 sdio_ireg;

	bl_plat = bl_hw->plat;
	bl_device = (struct bl_device *)bl_plat->priv;

	if(bl_read_data_sync(bl_hw, bl_plat->mp_regs, bl_device->reg->max_mp_regs, REG_PORT | BL_SDIO_BYTE_MODE_MASK)) {
		printk("read mp_regs failed\n");
		return -1;
	}

	sdio_ireg = bl_plat->mp_regs[bl_device->reg->host_int_status_reg];

	printk("sdio_ireg=0x%x\n", sdio_ireg);
	return 0;
}

static ssize_t bl_dbgfs_rw_reg_write(struct file *file,
                                          const char __user *user_buf,
                                          size_t count, loff_t *ppos)
{
    struct bl_hw *priv = file->private_data;
	char buf[32];
	int reg;
	int wr_val;
	u8 data;
	int ret = -1;

	size_t len = min_t(size_t, count, sizeof(buf)-1);

	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;
	
	buf[len]  = '\0';

	if (sscanf(buf, "0x%x-%d", &reg, &wr_val) > 0)
	{
		printk("reg=0x%x, wr_val=0x%x\n", reg, wr_val);
	}

	ret = bl_read_reg(priv, reg, &data);

	if(ret)
		printk("11--read reg 0x%x failed\n", reg);
	else
		printk("11--read reg 0x%x success, val=0x%x\n", reg, data);


	ret = bl_write_reg(priv, reg, wr_val);

	if(ret)
		printk("22--write 0x%x to reg 0x%x failed!\n", wr_val, reg);
	else
		printk("22--write 0x%x to reg 0x%x success!\n", wr_val, reg);

	ret = bl_read_reg(priv, reg, &data);

	if(ret)
		printk("33--read reg 0x%x failed\n", reg);
	else
		printk("33--read reg 0x%x success, val=0x%x\n", reg, data);
	
	return count;

}


DEBUGFS_READ_WRITE_FILE_OPS(rw_reg);

static ssize_t bl_dbgfs_rdbitmap_read(struct file *file,
                                         char __user *user_buf,
                                         size_t count, loff_t *ppos)
{
    struct bl_hw *priv = file->private_data;
	u8 rd_bitmap_l;
	u8 rd_bitmap_u;
	u32 bitmap;

	bl_read_reg(priv, 0x04, &rd_bitmap_l);
	bl_read_reg(priv, 0x05, &rd_bitmap_u);
	bitmap = rd_bitmap_l;
	bitmap |= rd_bitmap_u << 8;

	printk("rd_bitmap=0x%08x\n", bitmap);

	return 0;
}

static ssize_t bl_dbgfs_rdbitmap_write(struct file *file,
                                          const char __user *user_buf,
                                          size_t count, loff_t *ppos)
{
	return 0;
}

DEBUGFS_READ_WRITE_FILE_OPS(rdbitmap);

static ssize_t bl_dbgfs_wrbitmap_read(struct file *file,
                                         char __user *user_buf,
                                         size_t count, loff_t *ppos)
{
    struct bl_hw *priv = file->private_data;
	u8 wr_bitmap_l;
	u8 wr_bitmap_u;
	u32 bitmap;

	bl_read_reg(priv, 0x06, &wr_bitmap_l);
	bl_read_reg(priv, 0x07, &wr_bitmap_u);
	bitmap = wr_bitmap_l;
	bitmap |= wr_bitmap_u << 8;

	printk("wr_bitmap=0x%08x\n", bitmap);

	return 0;
}

static ssize_t bl_dbgfs_wrbitmap_write(struct file *file,
                                          const char __user *user_buf,
                                          size_t count, loff_t *ppos)
{
	return 0;
}

DEBUGFS_READ_WRITE_FILE_OPS(wrbitmap);


static ssize_t bl_dbgfs_run_fw_read(struct file *file,
                                       char __user *user_buf,
                                       size_t count, loff_t *ppos)
{
    struct bl_hw *priv = file->private_data;
    char buf[32];
    int ret;
    ssize_t read;
	u8 data;

	bl_read_reg(priv, 0x60, &data);

	printk("data=0x%x\n", data);

    ret = scnprintf(buf, 4, "0x%x", data);

    read = simple_read_from_buffer(user_buf, count, ppos, buf, ret);

    return read;
}

static ssize_t bl_dbgfs_run_fw_write(struct file *file,
                                        const char __user *user_buf,
                                        size_t count, loff_t *ppos)
{
    struct bl_hw *priv = file->private_data;
    char buf[32];
    int val;
    size_t len = min_t(size_t, count, sizeof(buf) - 1);

    if (copy_from_user(buf, user_buf, len))
        return -EFAULT;

    buf[len] = '\0';

    if (sscanf(buf, "%d", &val) > 0)
    {
		printk("val=0x%x\n", val);
		bl_write_reg(priv, 0x60, val);
	}

    return count;
}


DEBUGFS_READ_WRITE_FILE_OPS(run_fw);

#ifdef CONFIG_BL_FULLMAC

#define LINE_MAX_SZ             150

struct st {
    char line[LINE_MAX_SZ + 1];
    unsigned int r_idx;
};

static int compare_idx(const void *st1, const void *st2)
{
    int index1 = ((struct st *)st1)->r_idx;
    int index2 = ((struct st *)st2)->r_idx;

    if (index1 > index2) return 1;
    if (index1 < index2) return -1;

    return 0;
}

static int print_rate(char *buf, int size, int format, int nss, int mcs, int bw,
                      int sgi, int pre, int *r_idx)
{
    int res = 0;
    int bitrates_cck[4] = { 10, 20, 55, 110 };
    int bitrates_ofdm[8] = { 6, 9, 12, 18, 24, 36, 48, 54};

    if (format <= FORMATMOD_NON_HT_DUP_OFDM) {
        if (mcs < 4) {
            if (r_idx) {
                *r_idx = (mcs * 2) + pre;
                res = scnprintf(buf, size - res, "%3d ", *r_idx);
            }
            res += scnprintf(&buf[res], size - res, "L-CCK/%cP   %2u.%1uM ",
                             pre > 0 ? 'L' : 'S',
                             bitrates_cck[mcs] / 10,
                             bitrates_cck[mcs] % 10);
        } else {
            if (r_idx) {
                *r_idx = N_CCK + (mcs - 4);
                res = scnprintf(buf, size - res, "%3d ", *r_idx);
            }
            res += scnprintf(&buf[res], size - res, "L-OFDM     %2u.0M ",
                             bitrates_ofdm[mcs]);
        }
    } else if (format <= FORMATMOD_HT_GF) {
        if (r_idx) {
            *r_idx = N_CCK + N_OFDM + nss * 32 + mcs * 4 + bw * 2 + sgi;
            res = scnprintf(buf, size - res, "%3d ", *r_idx);
        }
        res += scnprintf(&buf[res], size - res, "HT%d/%cGI   MCS%-2d ",
                         20 * (1 << bw), sgi ? 'S' : 'L', nss * 8 + mcs);
    } else {
        if (r_idx) {
            *r_idx = N_CCK + N_OFDM + N_HT + nss * 80 + mcs * 8 + bw * 2 + sgi;
            res = scnprintf(buf, size - res, "%3d ", *r_idx);
        }
        res += scnprintf(&buf[res], size - res, "VHT%d/%cGI%*cMCS%d/%1d",
                         20 * (1 << bw), sgi ? 'S' : 'L', bw > 2 ? 1 : 2, ' ',
                         mcs, nss + 1);

    }

    return res;
}

static int print_rate_from_cfg(char *buf, int size, u32 rate_config, int *r_idx)
{
    union bl_rate_ctrl_info *r_cfg = (union bl_rate_ctrl_info *)&rate_config;
    union bl_mcs_index *mcs_index = (union bl_mcs_index *)&rate_config;
    unsigned int ft, pre, gi, bw, nss, mcs, len;

    ft = r_cfg->formatModTx;
    pre = r_cfg->preTypeTx;
    if (ft == FORMATMOD_VHT) {
        mcs = mcs_index->vht.mcs;
        nss = mcs_index->vht.nss;
    } else if (ft >= FORMATMOD_HT_MF) {
        mcs = mcs_index->ht.mcs;
        nss = mcs_index->ht.nss;
    } else {
        mcs = mcs_index->legacy;
        nss = 0;
    }
    gi = r_cfg->shortGITx;
    bw = r_cfg->bwTx;

    len = print_rate(buf, size, ft, nss, mcs, bw, gi, pre, r_idx);
    return len;
}

static void idx_to_rate_cfg(int idx, union bl_rate_ctrl_info *r_cfg)
{
    r_cfg->value = 0;
    if (idx < N_CCK)
    {
        r_cfg->formatModTx = FORMATMOD_NON_HT;
        r_cfg->preTypeTx = idx & 1;
        r_cfg->mcsIndexTx = idx / 2;
    }
    else if (idx < (N_CCK + N_OFDM))
    {
        r_cfg->formatModTx = FORMATMOD_NON_HT;
        r_cfg->mcsIndexTx =  idx - N_CCK + 4;
    }
    else if (idx < (N_CCK + N_OFDM + N_HT))
    {
        union bl_mcs_index *r = (union bl_mcs_index *)r_cfg;

        idx -= (N_CCK + N_OFDM);
        r_cfg->formatModTx = FORMATMOD_HT_MF;
        r->ht.nss = idx / (8*2*2);
        r->ht.mcs = (idx % (8*2*2)) / (2*2);
        r_cfg->bwTx = ((idx % (8*2*2)) % (2*2)) / 2;
        r_cfg->shortGITx = idx & 1;
    }
    else
    {
        union bl_mcs_index *r = (union bl_mcs_index *)r_cfg;

        idx -= (N_CCK + N_OFDM + N_HT);
        r_cfg->formatModTx = FORMATMOD_VHT;
        r->vht.nss = idx / (10*4*2);
        r->vht.mcs = (idx % (10*4*2)) / (4*2);
        r_cfg->bwTx = ((idx % (10*4*2)) % (4*2)) / 2;
        r_cfg->shortGITx = idx & 1;
    }
}

static ssize_t bl_dbgfs_rc_stats_read(struct file *file,
                                        char __user *user_buf,
                                        size_t count, loff_t *ppos)
{
    struct bl_sta *sta = NULL;
    struct bl_hw *priv = file->private_data;
    char *buf;
    int bufsz, len = 0;
    ssize_t read;
    int i = 0;
    int error = 0;
    struct me_rc_stats_cfm me_rc_stats_cfm;
    unsigned int no_samples;
    struct st *st;
    u8 mac[6];

    BL_DBG(BL_FN_ENTRY_STR);

    /* everything should fit in one call */
    if (*ppos)
        return 0;

    /* Get the station index from MAC address */
    sscanf(file->f_path.dentry->d_parent->d_iname, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
            &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
    if (mac == NULL)
        return 0;
    sta = bl_get_sta(priv, mac);
    if (sta == NULL)
        return 0;

    /* Forward the information to the LMAC */
    if ((error = bl_send_me_rc_stats(priv, sta->sta_idx, &me_rc_stats_cfm)))
        return error;

    no_samples = me_rc_stats_cfm.no_samples;
    if (no_samples == 0)
        return 0;

    bufsz = no_samples * LINE_MAX_SZ + 500;

    buf = kmalloc(bufsz + 1, GFP_ATOMIC);
    if (buf == NULL)
        return 0;

    st = kmalloc(sizeof(struct st) * no_samples, GFP_ATOMIC);
    if (st == NULL)
    {
        kfree(buf);
        return 0;
    }

    for (i = 0; i < no_samples; i++)
    {
        unsigned int tp, eprob;
        len = print_rate_from_cfg(st[i].line, LINE_MAX_SZ,
                                  me_rc_stats_cfm.rate_stats[i].rate_config,
                                  &st[i].r_idx);

        if (me_rc_stats_cfm.sw_retry_step != 0)
        {
            len += scnprintf(&st[i].line[len], LINE_MAX_SZ - len,  "%c",
                    me_rc_stats_cfm.retry[me_rc_stats_cfm.sw_retry_step].idx == i ? '*' : ' ');
        }
        else
        {
            len += scnprintf(&st[i].line[len], LINE_MAX_SZ - len, " ");
        }
        len += scnprintf(&st[i].line[len], LINE_MAX_SZ - len, "%c",
                me_rc_stats_cfm.retry[0].idx == i ? 'T' : ' ');
        len += scnprintf(&st[i].line[len], LINE_MAX_SZ - len, "%c",
                me_rc_stats_cfm.retry[1].idx == i ? 't' : ' ');
        len += scnprintf(&st[i].line[len], LINE_MAX_SZ - len, "%c ",
                me_rc_stats_cfm.retry[2].idx == i ? 'P' : ' ');

        tp = me_rc_stats_cfm.tp[i] / 10;
        len += scnprintf(&st[i].line[len], LINE_MAX_SZ - len, "  %4u.%1u",
                         tp / 10, tp % 10);

        eprob = ((me_rc_stats_cfm.rate_stats[i].probability * 1000) >> 16) + 1;
        len += scnprintf(&st[i].line[len],LINE_MAX_SZ - len,
                         " %4u.%1u %6u(%6u)  %6u %6u",
                         eprob / 10, eprob % 10,
                         me_rc_stats_cfm.rate_stats[i].success,
                         me_rc_stats_cfm.rate_stats[i].attempts,
                         me_rc_stats_cfm.rate_stats[i].sample_skipped,
                         me_rc_stats_cfm.rate_stats[i].n_retry & 0x1F);
    }
    len = scnprintf(buf, bufsz ,
                     "\nTX rate info for %02X:%02X:%02X:%02X:%02X:%02X:\n",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    len += scnprintf(&buf[len], bufsz - len,
            " #  type       rate            tpt  eprob     ok(   tot) skipped nRetry\n");

    // add sorted statistics to the buffer
    sort(st, no_samples, sizeof(st[0]), compare_idx, NULL);
    for (i = 0; i < no_samples; i++)
    {
        len += scnprintf(&buf[len], bufsz - len, "%s\n", st[i].line);
    }
    len += scnprintf(&buf[len], bufsz - len, "\n MPDUs AMPDUs AvLen trialP");
    len += scnprintf(&buf[len], bufsz - len, "\n%6u %6u %3d.%1d %6u\n",
                     me_rc_stats_cfm.ampdu_len,
                     me_rc_stats_cfm.ampdu_packets,
                     me_rc_stats_cfm.avg_ampdu_len >> 16,
                     ((me_rc_stats_cfm.avg_ampdu_len * 10) >> 16) % 10,
                     me_rc_stats_cfm.sample_wait);

    read = simple_read_from_buffer(user_buf, count, ppos, buf, len);

    kfree(buf);
    kfree(st);

    return read;
}

DEBUGFS_READ_FILE_OPS(rc_stats);

static ssize_t bl_dbgfs_rc_fixed_rate_idx_write(struct file *file,
                                                  const char __user *user_buf,
                                                  size_t count, loff_t *ppos)
{
    struct bl_sta *sta = NULL;
    struct bl_hw *priv = file->private_data;
    u8 mac[6];
    char buf[10];
    int fixed_rate_idx = -1;
    union bl_rate_ctrl_info rate_config;
    int error = 0;
    size_t len = min_t(size_t, count, sizeof(buf) - 1);

    BL_DBG(BL_FN_ENTRY_STR);

    /* Get the station index from MAC address */
    sscanf(file->f_path.dentry->d_parent->d_iname, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
            &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
    if (mac == NULL)
        return 0;
    sta = bl_get_sta(priv, mac);
    if (sta == NULL)
        return 0;

    /* Get the content of the file */
    if (copy_from_user(buf, user_buf, len))
        return -EFAULT;
    buf[len] = '\0';
    sscanf(buf, "%i\n", &fixed_rate_idx);

    /* Convert rate index into rate configuration */
    if ((fixed_rate_idx < 0) || (fixed_rate_idx >= (N_CCK + N_OFDM + N_HT + N_VHT)))
    {
        // disable fixed rate
        rate_config.value = 0xFFFF;
    }
    else
    {
        idx_to_rate_cfg(fixed_rate_idx, &rate_config);
    }
    // Forward the request to the LMAC
    if ((error = bl_send_me_rc_set_rate(priv, sta->sta_idx,
                                          (u16)rate_config.value)) != 0)
    {
        return error;
    }

    return len;
}

DEBUGFS_WRITE_FILE_OPS(rc_fixed_rate_idx);

static ssize_t bl_dbgfs_last_rx_read(struct file *file,
                                       char __user *user_buf,
                                       size_t count, loff_t *ppos)
{
    struct bl_sta *sta = NULL;
    struct bl_hw *priv = file->private_data;
    struct bl_rx_rate_stats *rate_stats;
    char *buf;
    int bufsz, i, len = 0;
    ssize_t read;
    unsigned int fmt, pre, bw, nss, mcs, sgi;
    u8 mac[6];
    struct hw_vect *last_rx;
    char hist[] = "##################################################";
    int hist_len = sizeof(hist) - 1;

    BL_DBG(BL_FN_ENTRY_STR);

    /* everything should fit in one call */
    if (*ppos)
        return 0;

    /* Get the station index from MAC address */
    sscanf(file->f_path.dentry->d_parent->d_iname, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
            &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
    if (mac == NULL)
        return 0;
    sta = bl_get_sta(priv, mac);
    if (sta == NULL)
        return 0;

    rate_stats = &sta->stats.rx_rate;
    bufsz = (rate_stats->size * ( 30 * hist_len) + 200);
    buf = kmalloc(bufsz + 1, GFP_ATOMIC);
    if (buf == NULL)
        return 0;

    len += scnprintf(buf, bufsz,
                     "\nRX rate info for %02X:%02X:%02X:%02X:%02X:%02X:\n",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // Display Statistics
    for (i = 0 ; i < rate_stats->size ; i++ )
    {
        if (rate_stats->table[i]) {
            union bl_rate_ctrl_info rate_config;
            int percent = (rate_stats->table[i] * 1000) / rate_stats->cpt;
            int p;

            idx_to_rate_cfg(i, &rate_config);
            len += print_rate_from_cfg(&buf[len], bufsz - len,
                                       rate_config.value, NULL);
            p = (percent * hist_len) / 1000;
            len += scnprintf(&buf[len], bufsz - len, ": %6d(%3d.%1d%%)%.*s\n",
                             rate_stats->table[i],
                             percent / 10, percent % 10, p, hist);
        }
    }

    // Display detailled info of the last received rate
    last_rx = &sta->stats.last_rx;

    len += scnprintf(&buf[len], bufsz - len,"\nLast received rate\n"
                     "  type     rate   LDPC STBC BEAMFM rssi(dBm)\n");

    fmt = last_rx->format_mod;
    bw = last_rx->ch_bw;
    pre = last_rx->pre_type;
    sgi = last_rx->short_gi;
    if (fmt == FORMATMOD_VHT) {
        mcs = last_rx->mcs;
        nss = last_rx->stbc ? last_rx->n_sts/2 : last_rx->n_sts;
    } else if (fmt >= FORMATMOD_HT_MF) {
        mcs = last_rx->mcs;
        nss = last_rx->stbc ? last_rx->stbc : last_rx->n_sts;
    } else {
        BUG_ON((mcs = legrates_lut[last_rx->leg_rate]) == -1);
        nss = 0;
    }

    len += print_rate(&buf[len], bufsz - len, fmt, nss, mcs, bw, sgi, pre, NULL);

    /* flags for HT/VHT */
    if (fmt == FORMATMOD_VHT) {
        len += scnprintf(&buf[len], bufsz - len, "    %c    %c      %c",
                         last_rx->fec_coding ? 'L' : ' ',
                         last_rx->stbc ? 'S' : ' ',
                         last_rx->smoothing ? ' ' : 'B');
    } else if (fmt >= FORMATMOD_HT_MF) {
        len += scnprintf(&buf[len], bufsz - len, "    %c    %c       ",
                         last_rx->fec_coding ? 'L' : ' ',
                         last_rx->stbc ? 'S' : ' ');
    } else {
        len += scnprintf(&buf[len], bufsz - len, "                 ");
    }
    len += scnprintf(&buf[len], bufsz - len, " %d\n", last_rx->rssi1);

    read = simple_read_from_buffer(user_buf, count, ppos, buf, len);

    kfree(buf);
    return read;
}

static ssize_t bl_dbgfs_last_rx_write(struct file *file,
                                        const char __user *user_buf,
                                        size_t count, loff_t *ppos)
{
    struct bl_sta *sta = NULL;
    struct bl_hw *priv = file->private_data;
    u8 mac[6];

    /* Get the station index from MAC address */
    sscanf(file->f_path.dentry->d_parent->d_iname, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
    if (mac == NULL)
        return 0;
    sta = bl_get_sta(priv, mac);
    if (sta == NULL)
        return 0;

    /* Prevent from interrupt preemption as these statistics are updated under
     * interrupt */
    spin_lock_bh(&priv->tx_lock);
    memset(sta->stats.rx_rate.table, 0,
           sta->stats.rx_rate.size * sizeof(sta->stats.rx_rate.table[0]));
    sta->stats.rx_rate.cpt = 0;
    spin_unlock_bh(&priv->tx_lock);

    return count;
}

DEBUGFS_READ_WRITE_FILE_OPS(last_rx);

#endif /* CONFIG_BL_FULLMAC */

/*
 * Calls a userspace pgm
 */
int bl_um_helper(struct bl_debugfs *bl_debugfs, const char *cmd)
{
    char *envp[] = { "PATH=/sbin:/usr/sbin:/bin:/usr/bin", NULL };
    char **argv;
    int argc, ret;

    if (!bl_debugfs->dir ||
        !strlen((cmd = cmd ? cmd : bl_debugfs->helper_cmd)))
        return 0;
    argv = argv_split(in_interrupt() ? GFP_ATOMIC : GFP_KERNEL, cmd, &argc);
    if (!argc)
        return PTR_ERR(argv);

    if ((ret = call_usermodehelper(argv[0], argv, envp,
                                   UMH_WAIT_PROC | UMH_KILLABLE)))
        printk(KERN_CRIT "Failed to call %s (%s returned %d)\n",
               argv[0], cmd, ret);
    argv_free(argv);

    return ret;
}

int bl_trigger_um_helper(struct bl_debugfs *bl_debugfs)
{
    if (bl_debugfs->helper_scheduled == true) {
        printk(KERN_CRIT "%s: Already scheduled\n", __func__);
        return -EBUSY;
    }

    spin_lock_bh(&bl_debugfs->umh_lock);
    if (bl_debugfs->unregistering) {
        spin_unlock_bh(&bl_debugfs->umh_lock);
        printk(KERN_CRIT "%s: unregistering\n", __func__);
        return -ENOENT;
    }
    bl_debugfs->helper_scheduled = true;
    schedule_work(&bl_debugfs->helper_work);
    spin_unlock_bh(&bl_debugfs->umh_lock);

    return 0;
}

#ifdef CONFIG_BL_FULLMAC
static void bl_rc_stat_work(struct work_struct *ws)
{
    struct bl_debugfs *bl_debugfs = container_of(ws, struct bl_debugfs,
                                                     rc_stat_work);
    struct bl_hw *bl_hw = container_of(bl_debugfs, struct bl_hw,
                                           debugfs);
    struct bl_sta *sta;
    uint8_t ridx, sta_idx;

    ridx = bl_debugfs->rc_read;
    sta_idx = bl_debugfs->rc_sta[ridx];
    if (sta_idx > (NX_REMOTE_STA_MAX + NX_VIRT_DEV_MAX)) {
        WARN(1, "Invalid sta index %d", sta_idx);
        return;
    }

    bl_debugfs->rc_sta[ridx] = 0xFF;
    ridx = (ridx + 1) % ARRAY_SIZE(bl_debugfs->rc_sta);
    bl_debugfs->rc_read = ridx;
    sta = &bl_hw->sta_table[sta_idx];
    if (!sta) {
        WARN(1, "Invalid sta %d", sta_idx);
        return;
    }

    if (bl_debugfs->dir_sta[sta_idx] == NULL) {
        /* register the sta */
        struct dentry *dir_rc = bl_debugfs->dir_rc;
        struct dentry *dir_sta;
        struct dentry *file;
        char sta_name[18];
        struct bl_rx_rate_stats *rate_stats = &sta->stats.rx_rate;
        int nb_rx_rate = N_CCK + N_OFDM;


        if (sta->sta_idx >= NX_REMOTE_STA_MAX) {
            scnprintf(sta_name, sizeof(sta_name), "bc_mc");
        } else {
            scnprintf(sta_name, sizeof(sta_name), "%pM", sta->mac_addr);
        }

        if (!(dir_sta = debugfs_create_dir(sta_name, dir_rc)))
            goto error;

        bl_debugfs->dir_sta[sta->sta_idx] = dir_sta;

        file = debugfs_create_file("stats", S_IRUSR, dir_sta, bl_hw,
                                   &bl_dbgfs_rc_stats_ops);
        if (IS_ERR_OR_NULL(file))
            goto error_after_dir;

        file = debugfs_create_file("fixed_rate_idx", S_IWUSR , dir_sta, bl_hw,
                                   &bl_dbgfs_rc_fixed_rate_idx_ops);
        if (IS_ERR_OR_NULL(file))
            goto error_after_dir;

        file = debugfs_create_file("rx_rate", S_IRUSR | S_IWUSR, dir_sta, bl_hw,
                                   &bl_dbgfs_last_rx_ops);
        if (IS_ERR_OR_NULL(file))
            goto error_after_dir;

        if (bl_hw->mod_params->ht_on)
            nb_rx_rate += N_HT;

        if (bl_hw->mod_params->vht_on)
            nb_rx_rate += N_VHT;

        rate_stats->table = kzalloc(nb_rx_rate * sizeof(rate_stats->table[0]),
                                    GFP_KERNEL);
        if (!rate_stats->table)
            goto error_after_dir;

        rate_stats->size = nb_rx_rate;
        rate_stats->cpt = 0;

    } else {
        /* unregister the sta */
        if (sta->stats.rx_rate.table) {
            kfree(sta->stats.rx_rate.table);
            sta->stats.rx_rate.table = NULL;
        }
        sta->stats.rx_rate.size = 0;
        sta->stats.rx_rate.cpt  = 0;

        debugfs_remove_recursive(bl_debugfs->dir_sta[sta_idx]);
        bl_debugfs->dir_sta[sta->sta_idx] = NULL;
    }

    return;

  error_after_dir:
    debugfs_remove_recursive(bl_debugfs->dir_sta[sta_idx]);
    bl_debugfs->dir_sta[sta->sta_idx] = NULL;
  error:
    dev_err(bl_hw->dev,
            "Error while (un)registering debug entry for sta %d\n", sta_idx);
}

void _bl_dbgfs_rc_stat_write(struct bl_debugfs *bl_debugfs, uint8_t sta_idx)
{
    uint8_t widx = bl_debugfs->rc_write;
    if (bl_debugfs->rc_sta[widx] != 0XFF) {
        WARN(1, "Overlap in debugfs rc_sta table\n");
    }

    bl_debugfs->rc_sta[widx] = sta_idx;
    widx = (widx + 1) % ARRAY_SIZE(bl_debugfs->rc_sta);
    bl_debugfs->rc_write = widx;

    schedule_work(&bl_debugfs->rc_stat_work);
}

void bl_dbgfs_register_rc_stat(struct bl_hw *bl_hw, struct bl_sta *sta)
{
    _bl_dbgfs_rc_stat_write(&bl_hw->debugfs, sta->sta_idx);
}

void bl_dbgfs_unregister_rc_stat(struct bl_hw *bl_hw, struct bl_sta *sta)
{
    _bl_dbgfs_rc_stat_write(&bl_hw->debugfs, sta->sta_idx);
}
#endif


int bl_dbgfs_register(struct bl_hw *bl_hw, const char *name)
{
    struct dentry *phyd = bl_hw->wiphy->debugfsdir;
    struct dentry *dir_rc;
    struct bl_debugfs *bl_debugfs = &bl_hw->debugfs;
    struct dentry *dir_drv, *dir_diags;

    if (!(dir_drv = debugfs_create_dir(name, phyd)))
        return -ENOMEM;

    bl_debugfs->dir = dir_drv;
    bl_debugfs->unregistering = false;

    if (!(dir_diags = debugfs_create_dir("diags", dir_drv)))
        goto err;

#ifdef CONFIG_BL_FULLMAC
    if (!(dir_rc = debugfs_create_dir("rc", dir_drv)))
        goto err;
    bl_debugfs->dir_rc = dir_rc;
    INIT_WORK(&bl_debugfs->rc_stat_work, bl_rc_stat_work);
    bl_debugfs->rc_write = bl_debugfs->rc_read = 0;
    memset(bl_debugfs->rc_sta, 0xFF, sizeof(bl_debugfs->rc_sta));
#endif

    DEBUGFS_ADD_FILE(stats, dir_drv, S_IWUSR | S_IRUSR);
    DEBUGFS_ADD_FILE(sys_stats, dir_drv,  S_IRUSR);
    DEBUGFS_ADD_FILE(txq, dir_drv, S_IRUSR);
    DEBUGFS_ADD_FILE(acsinfo, dir_drv, S_IRUSR);

	DEBUGFS_ADD_FILE(run_fw, dir_drv, S_IWUSR | S_IRUSR);
	DEBUGFS_ADD_FILE(sdio_test, dir_drv, S_IWUSR | S_IRUSR);
	DEBUGFS_ADD_FILE(rdbitmap, dir_drv, S_IWUSR | S_IRUSR);
	DEBUGFS_ADD_FILE(wrbitmap, dir_drv, S_IWUSR | S_IRUSR);
	DEBUGFS_ADD_FILE(rw_reg, dir_drv, S_IWUSR | S_IRUSR);
	DEBUGFS_ADD_FILE(dbg_cdt, dir_drv, S_IWUSR | S_IRUSR);
	DEBUGFS_ADD_FILE(dbg_time, dir_drv, S_IWUSR | S_IRUSR);

    bl_debugfs->trace_prst = bl_debugfs->helper_scheduled = false;
    spin_lock_init(&bl_debugfs->umh_lock);
    DEBUGFS_ADD_FILE(rhd,        dir_diags,          S_IRUSR);
    DEBUGFS_ADD_FILE(rbd,        dir_diags,          S_IRUSR);
    DEBUGFS_ADD_FILE(thd0,       dir_diags,          S_IRUSR);
    DEBUGFS_ADD_FILE(thd1,       dir_diags,          S_IRUSR);
    DEBUGFS_ADD_FILE(thd2,       dir_diags,          S_IRUSR);
    DEBUGFS_ADD_FILE(thd3,       dir_diags,          S_IRUSR);
#if (NX_TXQ_CNT == 5)
    DEBUGFS_ADD_FILE(thd4,       dir_diags,          S_IRUSR);
#endif
    DEBUGFS_ADD_FILE(mactrace,   dir_diags,          S_IRUSR);
    DEBUGFS_ADD_FILE(mactctrig,   dir_diags,          S_IRUSR);
    DEBUGFS_ADD_FILE(macdiags,   dir_diags,          S_IRUSR);
    DEBUGFS_ADD_FILE(phydiags,   dir_diags,          S_IRUSR);
    DEBUGFS_ADD_FILE(plfdiags,   dir_diags,          S_IRUSR);
    DEBUGFS_ADD_FILE(hwdiags,    dir_diags,          S_IRUSR);
    DEBUGFS_ADD_FILE(swdiags,    dir_diags,          S_IRUSR);
    DEBUGFS_ADD_FILE(error,      dir_diags,          S_IRUSR);
    DEBUGFS_ADD_FILE(rxdesc,     dir_diags,          S_IRUSR);
    DEBUGFS_ADD_FILE(txdesc,     dir_diags,          S_IRUSR);
    DEBUGFS_ADD_FILE(macrxptr,   dir_diags,          S_IRUSR);
    DEBUGFS_ADD_FILE(lamacconf,  dir_diags,          S_IRUSR);
    DEBUGFS_ADD_FILE(chaninfo,   dir_diags,          S_IRUSR);
    DEBUGFS_ADD_FILE(fw_dbg,     dir_diags,          S_IWUSR | S_IRUSR);

    return 0;

err:
    bl_dbgfs_unregister(bl_hw);
    return -ENOMEM;
}

void bl_dbgfs_unregister(struct bl_hw *bl_hw)
{
    struct bl_debugfs *bl_debugfs = &bl_hw->debugfs;

    if (!bl_hw->debugfs.dir)
        return;

    bl_debugfs->unregistering = true;
    flush_work(&bl_debugfs->helper_work);
#ifdef CONFIG_BL_FULLMAC
    flush_work(&bl_debugfs->rc_stat_work);
#endif
    debugfs_remove_recursive(bl_hw->debugfs.dir);
    bl_hw->debugfs.dir = NULL;
}

