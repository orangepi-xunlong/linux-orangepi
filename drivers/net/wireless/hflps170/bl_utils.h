/**
 ******************************************************************************
 *
 * @file bl_ipc_utils.h
 *
 * @brief IPC utility function declarations
 *
 * Copyright (C) BouffaloLab 2017-2018
 *
 ******************************************************************************
 */

#ifndef _BL_IPC_UTILS_H_
#define _BL_IPC_UTILS_H_

#include <linux/dma-mapping.h>
#include <linux/skbuff.h>

#include "lmac_msg.h"
#include "ipc_host.h"
#include "bl_cmds.h"


#ifdef CONFIG_BL_DBG
#define BL_DBG printk
//#define BL_DBG(f, args...) trace_printk(f, ##args)
#else
#define BL_DBG(a...) do {} while (0)
#endif

#define LOG_BUF_SIZE (32*PAGE_SIZE)
#define LOG_NEWLINE_SIGN                    "\n"
/* buffer size for every line's log */
#define LOG_LINE_BUF_SIZE                   256
#define LOG_BUF_OUTPUT_BUF_SIZE             (LOG_LINE_BUF_SIZE*4*4*32)

// Prefix used for module filtering
// If you modify any value, modify also DBG_MOD_ macros below
#define D_INI        	"\x80"   ///< Prefix for Init
#define D_MSG      	 	"\x81"   ///< Prefix for MSG
#define D_TX       		"\x82"   ///< Prefix for TX
#define D_RX       		"\x83"   ///< Prefix for RX
#define D_CREDIT       	"\x84"   ///< Prefix for CREDIT
#define D_AGGR        	"\x85"   ///< Prefix for AGGR
#define D_XX0        	"\x86"   ///< Prefix unused
#define D_XX1       	"\x87"   ///< Prefix unused
#define D_XX2       	"\x88"   ///< Prefix unused
#define D_XX3       	"\x89"   ///< Prefix unused
#define D_XX4      	 	"\x8A"   ///< Prefix unused
#define D_XX5       	"\x8B"   ///< Prefix unused
#define D_XX6       	"\x8C"   ///< Prefix unused

// Prefix used for severity filtering
// If you modify any value, modify also DBG_SEV_ macros below
#define D_CRT       "\x9A"   ///< Prefix for critical
#define D_ERR       "\x9B"   ///< Prefix for error
#define D_WRN       "\x9C"   ///< Prefix for warning
#define D_INF       "\x9D"   ///< Prefix for info
#define D_VRB       "\x9E"   ///< Prefix for verbose debug

/// Module filtering macros, used only by debug module
enum dbg_mod_tag
{
    DBG_MOD_IDX_INI = 0,   		///< Bit index for init
    DBG_MOD_IDX_MSG,      		///< Bit index for msg
    DBG_MOD_IDX_TX,      		///< Bit index for tx
    DBG_MOD_IDX_RX,      		///< Bit index for rx
    DBG_MOD_IDX_CREDIT,       	///< Bit index for credit
    DBG_MOD_IDX_AGGR,       	///< Bit index for aggr
    DBG_MOD_IDX_MAX,      		///< Number of modules
};

#define DBG_MOD_MIN     0x80
#define DBG_MOD_MAX     (DBG_MOD_MIN + DBG_MOD_IDX_MAX)

#define DBG_MOD_ALL         0xFFFFFFFF

/// Severity filtering macros, used only by debug module
enum dbg_sev_tag
{
    DBG_SEV_IDX_NONE = 0,   ///< No print allowed
    DBG_SEV_IDX_CRT,        ///< Critical and unspecified allowed only
    DBG_SEV_IDX_ERR,        ///< Error allowed and above
    DBG_SEV_IDX_WRN,        ///< Warning allowed and above
    DBG_SEV_IDX_INF,        ///< Info allowed and above
    DBG_SEV_IDX_VRB,        ///< All allowed
    DBG_SEV_IDX_MAX,        ///< Number of severity levels
    DBG_SEV_ALL             ///< Convenient macro
};

#define DBG_SEV_MIN     0x9A
#define DBG_SEV_MAX     0xA0

#ifdef CONFIG_BL_MOD_LEV_DBG
    void dbg_test_print(struct bl_hw *bl_hw, const char *func, const long line, const char *fmt, ...);
    #define BL_DBG_MOD_LEVEL(bl_hw, fmt, ...)   dbg_test_print(bl_hw, __func__, __LINE__, fmt, ## __VA_ARGS__)
#else
    #define BL_DBG_MOD_LEVEL(bl_hw, fmt, ...)   do {} while (0)
#endif

#define BL_FN_ENTRY_STR ">>> %s()\n", __func__

enum bl_dev_flag {
    BL_DEV_RESTARTING,
    BL_DEV_STACK_RESTARTING,
    BL_DEV_STARTED,
};

struct bl_preq_ie_elem {
    u8 *buf;
    dma_addr_t dma_addr;
    int bufsz;
};

struct bl_dma_elem {
    u8 *buf;
    dma_addr_t dma_addr;
    int len;
};

struct bl_patternbuf {
    u32 *buf;
    dma_addr_t dma_addr;
    int bufsz;
};

struct bl_labuf {
    u8 *buf;
    dma_addr_t dma_addr;
    int bufsz;
};

struct bl_dbginfo {
    struct mutex mutex;
    struct dbg_debug_dump_tag *buf;
    dma_addr_t dma_addr;
    int bufsz;
};

#define BL_RXBUFF_PATTERN     (0xCAFEFADE)

/* Maximum number of rx buffer the fw may use at the same time */
#define BL_RXBUFF_MAX (64 * 10)

/**
 * struct bl_skb_cb - Control Buffer structure for RX buffer
 *
 * @dma_addr: DMA address of the data buffer
 * @pattern: Known pattern (used to check pointer on skb)
 * @idx: Index in bl_hw.rxbuff_table to contians address of this buffer
 */
struct bl_skb_cb {
    dma_addr_t dma_addr;
    uint32_t pattern;
    uint32_t idx;
};

#define BL_RXBUFF_DMA_ADDR_SET(skbuff, addr)          \
    ((struct bl_skb_cb *)(skbuff->cb))->dma_addr = addr
#define BL_RXBUFF_DMA_ADDR_GET(skbuff)                \
    ((struct bl_skb_cb *)(skbuff->cb))->dma_addr

#define BL_RXBUFF_PATTERN_SET(skbuff, pat)                \
    ((struct bl_skb_cb *)(skbuff->cb))->pattern = pat
#define BL_RXBUFF_PATTERN_GET(skbuff)         \
    ((struct bl_skb_cb *)(skbuff->cb))->pattern

#define BL_RXBUFF_IDX_SET(skbuff, val)                \
    ((struct bl_skb_cb *)(skbuff->cb))->idx = val
#define BL_RXBUFF_IDX_GET(skbuff)             \
    ((struct bl_skb_cb *)(skbuff->cb))->idx

#define BL_RXBUFF_VALID_IDX(idx) ((idx) < BL_RXBUFF_MAX)

/* Used to ensure that hostid set to fw is never 0 */
#define BL_RXBUFF_IDX_TO_HOSTID(idx) ((idx) + 1)
#define BL_RXBUFF_HOSTID_TO_IDX(hostid) ((hostid) - 1)

struct bl_e2arxdesc_elem {
    struct rxdesc_tag *rxdesc_ptr;
    dma_addr_t dma_addr;
};

/*
 * Structure used to store information regarding E2A radar events in the driver
 */
struct bl_e2aradar_elem {
    struct radar_pulse_array_desc *radarbuf_ptr;
    dma_addr_t dma_addr;
};

/*
 * Structure used to store information regarding E2A msg buffers in the driver
 */
struct bl_e2amsg_elem {
    struct ipc_e2a_msg *msgbuf_ptr;
    dma_addr_t dma_addr;
};

/*
 * Structure used to store information regarding Debug msg buffers in the driver
 */
struct bl_dbg_elem {
    struct ipc_dbg_msg *dbgbuf_ptr;
    dma_addr_t dma_addr;
};

/**
 ******************************************************************************
 * @brief Initialize IPC interface.
 *
 * This function initializes IPC interface by registering callbacks, setting
 * shared memory area and calling IPC Init function.
 *
 * This function should be called only once during driver's lifetime.
 *
 * @param[in]   bl_hw        Pointer to main structure storing all the
 *                             relevant information
 * @param[in]   ipc_shared_mem
 *
 ******************************************************************************
 */
int bl_ipc_init(struct bl_hw *bl_hw);

/**
 ******************************************************************************
 * @brief Release IPC interface.
 *
 * @param[in]   bl_hw   Pointer to main structure storing all the relevant
 *                        information
 *
 ******************************************************************************
 */
void bl_ipc_deinit(struct bl_hw *bl_hw);

/**
 ******************************************************************************
 * @brief Flush IPC .
 *
 * @param[in]   bl_hw   Pointer to main structure storing all the relevant
 *                        information
 *
 ******************************************************************************
 */
void bl_ipc_tx_drain(struct bl_hw *bl_hw);

/**
 ******************************************************************************
 * @brief Function called upon DBG_ERROR_IND message reception.
 * This function triggers the UMH script call that will indicate to the user
 * space the error that occurred and stored the debug dump. Once the UMH script
 *  is executed, the bl_umh_done function has to be called.
 *
 * @param[in]   bl_hw   Pointer to main structure storing all the relevant
 *                        information
 *
 ******************************************************************************
 */
void bl_error_ind(struct bl_hw *bl_hw);

/**
 ******************************************************************************
 * @brief Function called once UMH script execution is finished.
 *
 * @param[in]   bl_hw   Pointer to main structure storing all the relevant
 *                        information
 *
 ******************************************************************************
 */
void bl_umh_done(struct bl_hw *bl_hw);

struct bl_sta *bl_get_sta(struct bl_hw *bl_hw, const u8 *mac_addr);


#endif /* _BL_IPC_UTILS_H_ */
