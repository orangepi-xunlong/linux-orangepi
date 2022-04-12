/**
 ******************************************************************************
 *
 * @file ipc_host.c
 *
 * @brief IPC module.
 *
 * Copyright (C) BouffaloLab 2017-2018
 *
 ******************************************************************************
 */

/*
 * INCLUDE FILES
 ******************************************************************************
 */
#include <linux/spinlock.h>
#include "bl_defs.h"

#include "ipc_host.h"

/*
 * TYPES DEFINITION
 ******************************************************************************
 */

const int nx_txdesc_cnt[] =
{
    NX_TXDESC_CNT0,
    NX_TXDESC_CNT1,
    NX_TXDESC_CNT2,
    NX_TXDESC_CNT3,
    #if NX_TXQ_CNT == 5
    NX_TXDESC_CNT4,
    #endif
};

const int nx_txdesc_cnt_msk[] =
{
    NX_TXDESC_CNT0 - 1,
    NX_TXDESC_CNT1 - 1,
    NX_TXDESC_CNT2 - 1,
    NX_TXDESC_CNT3 - 1,
    #if NX_TXQ_CNT == 5
    NX_TXDESC_CNT4 - 1,
    #endif
};

const int nx_txuser_cnt[] =
{
    CONFIG_USER_MAX,
    CONFIG_USER_MAX,
    CONFIG_USER_MAX,
    CONFIG_USER_MAX,
    #if NX_TXQ_CNT == 5
    1,
    #endif
};

/**
 ******************************************************************************
 */
void *ipc_host_tx_flush(struct ipc_host_env_tag *env, const int queue_idx, const int user_pos)
{
    uint32_t used_idx = env->txdesc_used_idx[queue_idx][user_pos];
    void *host_id = env->tx_host_id[queue_idx][user_pos][used_idx & nx_txdesc_cnt_msk[queue_idx]];

    // call the external function to indicate that a TX packet is freed
    if (host_id != 0)
    {
        // Reset the host id in the array
        env->tx_host_id[queue_idx][user_pos][used_idx & nx_txdesc_cnt_msk[queue_idx]] = 0;

        // Increment the used index
        env->txdesc_used_idx[queue_idx][user_pos]++;
    }

    return (host_id);
}

/**
 ******************************************************************************
 */
void ipc_host_tx_cfm_handler(struct ipc_host_env_tag *env, const int queue_idx, const int user_pos, struct bl_hw_txhdr *hw_hdr, struct bl_txq **txq_saved)
{
	void *host_id = NULL;
	struct sk_buff *skb;
	uint32_t used_idx = env->txdesc_used_idx[queue_idx][user_pos] & nx_txdesc_cnt_msk[queue_idx];
	uint32_t free_idx = env->txdesc_free_idx[queue_idx][user_pos] & nx_txdesc_cnt_msk[queue_idx];
							
	host_id = env->tx_host_id[queue_idx][user_pos][used_idx];
	//ASSERT_ERR(host_id != NULL);
	if(!host_id){
        printk("%s: host id is null \n",__func__);
        goto exit;
	}
	env->tx_host_id[queue_idx][user_pos][used_idx] = 0;
	skb=host_id;

	BL_DBG("cfm skb=%p in %d of buffer, pkt_sn=%u\n", host_id, used_idx & nx_txdesc_cnt_msk[queue_idx], ((struct bl_txhdr *)(skb->data))->sw_hdr->hdr.reserved);

	if (env->cb.send_data_cfm(env->pthis, host_id, hw_hdr, (void **)txq_saved) != 0) {
		BL_DBG("send_data_cfm!=0, so break, used_idx=%d\n", used_idx);
		env->tx_host_id[queue_idx][user_pos][used_idx] = host_id;
	} else {
		env->txdesc_used_idx[queue_idx][user_pos]++;
	}

exit:
	if ((env->txdesc_used_idx[queue_idx][user_pos]&nx_txdesc_cnt_msk[queue_idx]) == free_idx) {
		BL_DBG("ipc_host_tx_cfm_handler: used_idx=free_idx=%d\n", free_idx);
		env->rb_len[queue_idx] = nx_txdesc_cnt[queue_idx];
	} else {
		BL_DBG("ipc_host_tx_cfm_handler: used_idx=%d, free_idx=%d\n", env->txdesc_used_idx[queue_idx][user_pos]&nx_txdesc_cnt_msk[queue_idx], free_idx);
		env->rb_len[queue_idx] = ((env->txdesc_used_idx[queue_idx][user_pos]&nx_txdesc_cnt_msk[queue_idx])-free_idx + nx_txdesc_cnt[queue_idx]) % nx_txdesc_cnt[queue_idx];
	}
	
	BL_DBG("ipc_host_tx_cfm_handler: env->rb_len[%d]=%d\n", queue_idx, env->rb_len[queue_idx]);
	BL_DBG("used_idx=%d--->%d\n", env->txdesc_used_idx[queue_idx][user_pos]-1, env->txdesc_used_idx[queue_idx][user_pos]);
}

/**
 ******************************************************************************
 */
void ipc_host_init(struct ipc_host_env_tag *env,
                  struct ipc_host_cb_tag *cb,
                  void *pthis)
{
    unsigned int i;
    // Reset the IPC Host environment
    memset(env, 0, sizeof(struct ipc_host_env_tag));

    // Save the callbacks in our own environment
    env->cb = *cb;

    // Save the pointer to the register base
    env->pthis = pthis;

    // Initialize buffers numbers and buffers sizes needed for DMA Receptions
    env->rx_bufnb = IPC_RXBUF_CNT;
    env->rx_bufsz = IPC_RXBUF_SIZE;
    #ifdef CONFIG_BL_FULLMAC
    env->rxdesc_nb = IPC_RXDESC_CNT;
    #endif //(CONFIG_BL_FULLMAC)
    env->ipc_e2amsg_bufnb = IPC_MSGE2A_BUF_CNT;
    env->ipc_e2amsg_bufsz = sizeof(struct ipc_e2a_msg);
    env->ipc_dbg_bufnb = IPC_DBGBUF_CNT;
    env->ipc_dbg_bufsz = sizeof(struct ipc_dbg_msg);

	env->rb_len[0] = NX_TXDESC_CNT0;
	env->rb_len[1] = NX_TXDESC_CNT1;
	env->rb_len[2] = NX_TXDESC_CNT2;
	env->rb_len[3] = NX_TXDESC_CNT3;
	#if NX_TXQ_CNT == 5
	env->rb_len[4] = NX_TXDESC_CNT4;
	#endif

    for (i = 0; i < CONFIG_USER_MAX; i++)
    {
        // Initialize the pointers to the hostid arrays
        env->tx_host_id[0][i] = env->tx_host_id0[i];
        env->tx_host_id[1][i] = env->tx_host_id1[i];
        env->tx_host_id[2][i] = env->tx_host_id2[i];
        env->tx_host_id[3][i] = env->tx_host_id3[i];
        #if NX_TXQ_CNT == 5
        env->tx_host_id[4][i] = NULL;
        #endif
    }

    #if NX_TXQ_CNT == 5
    env->tx_host_id[4][0] = env->tx_host_id4[0];
    #endif
}

/**
 ******************************************************************************
 */
void ipc_host_txdesc_push(struct ipc_host_env_tag *env, const int queue_idx,
                          const int user_pos, void *host_id)
{
#ifdef CONFIG_BL_DBG
    struct sk_buff *skb = host_id;
#endif
    uint32_t free_idx = env->txdesc_free_idx[queue_idx][user_pos] & nx_txdesc_cnt_msk[queue_idx];
	uint32_t used_idx = env->txdesc_used_idx[queue_idx][user_pos] & nx_txdesc_cnt_msk[queue_idx];

	BL_DBG("save skb=%p in %d of buffer, pkt_sn=%u\n", host_id, free_idx, ((struct bl_txhdr *)((struct sk_buff *)skb->data))->sw_hdr->hdr.reserved);

    // Save the host id in the environment
    env->tx_host_id[queue_idx][user_pos][free_idx] = host_id;

    if((free_idx + 1) % nx_txdesc_cnt[queue_idx] == used_idx) {
		BL_DBG("queue is full: free_idx=%d, used_idx=%d\n", free_idx, used_idx);
		env->txdesc_free_idx[queue_idx][user_pos]++;
		env->rb_len[queue_idx] = 0;
    } else {
	   	env->txdesc_free_idx[queue_idx][user_pos]++;
		BL_DBG("ipc_host_txdesc_push: used_idx=%d, free_idx=%d\n", used_idx, env->txdesc_free_idx[queue_idx][user_pos]&nx_txdesc_cnt_msk[queue_idx]);
		env->rb_len[queue_idx] = (used_idx-(env->txdesc_free_idx[queue_idx][user_pos]&nx_txdesc_cnt_msk[queue_idx]) + nx_txdesc_cnt[queue_idx]) % nx_txdesc_cnt[queue_idx];
    }

	BL_DBG("ipc_host_txdesc_push: env->rb_len[%d]=%d\n", queue_idx, env->rb_len[queue_idx]);
	BL_DBG("queue_idx[%d], free_idx: %d--->%d\n", queue_idx, env->txdesc_free_idx[queue_idx][user_pos] - 1, env->txdesc_free_idx[queue_idx][user_pos]);
}
