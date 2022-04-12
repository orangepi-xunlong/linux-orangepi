/**
 ******************************************************************************
 *
 * @file ipc_host.h
 *
 * @brief IPC module.
 *
 * Copyright (C) BouffaloLab 2017-2018
 *
 ******************************************************************************
 */
#ifndef _IPC_HOST_H_
#define _IPC_HOST_H_

/*
 * INCLUDE FILES
 ******************************************************************************
 */
#include "ipc_shared.h"
#ifndef __KERNEL__
#include "arch.h"
#else
#include "ipc_compat.h"
#endif

#include "bl_txq.h"
#include "hal_desc.h"

/**
 ******************************************************************************
 * @brief This structure is used to initialize the MAC SW
 *
 * The WLAN device driver provides functions call-back with this structure
 ******************************************************************************
 */
struct ipc_host_cb_tag
{
    /// WLAN driver call-back function: send_data_cfm
    int (*send_data_cfm)(void *pthis, void *host_id, void *data1, void **data2);

    /// WLAN driver call-back function: recv_data_ind
    uint8_t (*recv_data_ind)(void *pthis, void *host_id);

    /// WLAN driver call-back function: recv_radar_ind
    uint8_t (*recv_radar_ind)(void *pthis, void *host_id);

    /// WLAN driver call-back function: recv_msg_ind
    uint8_t (*recv_msg_ind)(void *pthis, void *host_id);

    /// WLAN driver call-back function: recv_msgack_ind
    uint8_t (*recv_msgack_ind)(void *pthis, void *host_id);

    /// WLAN driver call-back function: recv_dbg_ind
    uint8_t (*recv_dbg_ind)(void *pthis, void *host_id);

    /// WLAN driver call-back function: prim_tbtt_ind
    void (*prim_tbtt_ind)(void *pthis);

    /// WLAN driver call-back function: sec_tbtt_ind
    void (*sec_tbtt_ind)(void *pthis);

};

/*
 * Struct used to store information about host buffers (DMA Address and local pointer)
 */
struct ipc_hostbuf
{
    void    *hostid;     ///< ptr to hostbuf client (ipc_host client) structure
    uint32_t dma_addr;   ///< ptr to real hostbuf dma address
};

/// Definition of the IPC Host environment structure.
struct ipc_host_env_tag
{
    /// Structure containing the callback pointers
    struct ipc_host_cb_tag cb;

    #ifdef CONFIG_BL_FULLMAC
    // Array used to store the descriptor addresses
    struct ipc_hostbuf ipc_host_rxdesc_array[IPC_RXDESC_CNT];
    // Index of the host RX descriptor array (ipc_shared environment)
    uint8_t ipc_host_rxdesc_idx;
    /// Store the number of RX Descriptors
    uint8_t rxdesc_nb;
    #endif //(CONFIG_BL_FULLMAC)

    /// Fields for Data Rx handling
    // Index used for ipc_host_rxbuf_array to point to current buffer
    uint8_t ipc_host_rxbuf_idx;
    // Store the number of Rx Data buffers
    uint32_t rx_bufnb;
    // Store the size of the Rx Data buffers
    uint32_t rx_bufsz;

	uint32_t rb_len[NX_TXQ_CNT];

    // Index used that points to the first free TX desc
    uint32_t txdesc_free_idx[IPC_TXQUEUE_CNT][CONFIG_USER_MAX];
    // Index used that points to the first used TX desc
    uint32_t txdesc_used_idx[IPC_TXQUEUE_CNT][CONFIG_USER_MAX];
    // Array storing the currently pushed host ids for the BK queue
    void *tx_host_id0[CONFIG_USER_MAX][NX_TXDESC_CNT0];
    // Array storing the currently pushed host ids for the BE queue
    void *tx_host_id1[CONFIG_USER_MAX][NX_TXDESC_CNT1];
    // Array storing the currently pushed host ids for the VI queue
    void *tx_host_id2[CONFIG_USER_MAX][NX_TXDESC_CNT2];
    // Array storing the currently pushed host ids for the VO queue
    void *tx_host_id3[CONFIG_USER_MAX][NX_TXDESC_CNT3];
    #if NX_TXQ_CNT == 5
    // Array storing the currently pushed host ids for the BCN queue
    void *tx_host_id4[1][NX_TXDESC_CNT4];
    #endif
    // Pointer to the different host ids arrays, per IPC queue
    void **tx_host_id[IPC_TXQUEUE_CNT][CONFIG_USER_MAX];
    // Pointer to the different TX descriptor arrays, per IPC queue
    volatile struct txdesc_host *txdesc[IPC_TXQUEUE_CNT][CONFIG_USER_MAX];

    /// Fields for Emb->App MSGs handling
    // Global array used to store the hostid and hostbuf addresses for msg/ind
    struct ipc_hostbuf ipc_host_msgbuf_array[IPC_MSGE2A_BUF_CNT];
    // Index of the MSG E2A buffers array to point to current buffer
    uint8_t ipc_host_msge2a_idx;
    // Store the number of E2A MSG buffers
    uint32_t ipc_e2amsg_bufnb;
    // Store the size of the E2A MSG buffers
    uint32_t ipc_e2amsg_bufsz;

    /// E2A ACKs of A2E MSGs
    uint8_t msga2e_cnt;
    void *msga2e_hostid;

    /// Fields for Debug MSGs handling
    // Global array used to store the hostid and hostbuf addresses for Debug messages
    struct ipc_hostbuf ipc_host_dbgbuf_array[IPC_DBGBUF_CNT];
    // Index of the Debug messages buffers array to point to current buffer
    uint8_t ipc_host_dbg_idx;
    // Store the number of Debug messages buffers
    uint32_t ipc_dbg_bufnb;
    // Store the size of the Debug messages buffers
    uint32_t ipc_dbg_bufsz;

    /// Pointer to the attached object (used in callbacks and register accesses)
    void *pthis;
};

extern const int nx_txdesc_cnt[];
extern const int nx_txdesc_cnt_msk[];
extern const int nx_txuser_cnt[];

/**
 ******************************************************************************
 * @brief Initialize the IPC running on the Application CPU.
 *
 * This function:
 *   - initializes the IPC software environments
 *   - enables the interrupts in the IPC block
 *
 * @param[in]   env   Pointer to the IPC host environment
 *
 * @warning Since this function resets the IPC Shared memory, it must be called
 * before the LMAC FW is launched because LMAC sets some init values in IPC
 * Shared memory at boot.
 *
 ******************************************************************************
 */
void ipc_host_init(struct ipc_host_env_tag *env,
                  struct ipc_host_cb_tag *cb,
                  void *pthis);

/**
 ******************************************************************************
 * @brief Push a filled Tx descriptor (host side).
 *
 * This function sets the next Tx descriptor available by the host side:
 * - as used for the host side
 * - as available for the emb side.
 * The Tx descriptor must be correctly filled before calling this function.
 *
 * This function may trigger an IRQ to the emb CPU depending on the interrupt
 * mitigation policy and on the push count.
 *
 * @param[in]   env   Pointer to the IPC host environment
 * @param[in]   queue_idx   Queue index. Same value than ipc_host_txdesc_get()
 * @param[in]   user_pos    User position. If MU-MIMO is not used, this value
 *                          shall be 0.
 * @param[in]   host_id     Parameter indicated by the IPC at TX confirmation,
 *                          that allows the driver finding the buffer
 *
 ******************************************************************************
 */
void ipc_host_txdesc_push(struct ipc_host_env_tag *env, const int queue_idx,
                          const int user_pos, void *host_id);

/**
 ******************************************************************************
 * @brief Get and flush a packet from the IPC queue passed as parameter.
 *
 * @param[in]   env        Pointer to the IPC host environment
 * @param[in]   queue_idx  Index of the queue to flush
 * @param[in]   user_pos   User position to flush
 *
 * @return The flushed hostid if there is one, 0 otherwise.
 *
 ******************************************************************************
 */
void *ipc_host_tx_flush(struct ipc_host_env_tag *env, const int queue_idx,
                        const int user_pos);
void ipc_host_tx_cfm_handler(struct ipc_host_env_tag *env, const int queue_idx, const int user_pos, struct bl_hw_txhdr *hw_hdr, struct bl_txq **txq_saved);


#endif // _IPC_HOST_H_
