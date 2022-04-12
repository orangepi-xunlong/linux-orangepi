/**
 ****************************************************************************************
 *
 * @file bl_cmds.h
 *
 * Copyright (C) BouffaloLab 2017-2018
 *
 ****************************************************************************************
 */

#ifndef _BL_CMDS_H_
#define _BL_CMDS_H_

#include <linux/spinlock.h>
#include <linux/completion.h>
#include "lmac_msg.h"
#include "ipc_shared.h"

#ifdef CONFIG_BL_SDM
#define BL_80211_CMD_TIMEOUT_MS    (20 * 300)
#else
#define BL_80211_CMD_TIMEOUT_MS    2000
#endif

#define BL_CMD_FLAG_NONBLOCK      BIT(0)
#define BL_CMD_FLAG_REQ_CFM       BIT(1)
#define BL_CMD_FLAG_WAIT_PUSH     BIT(2)
#define BL_CMD_FLAG_WAIT_ACK      BIT(3)
#define BL_CMD_FLAG_WAIT_CFM      BIT(4)
#define BL_CMD_FLAG_DONE          BIT(5)
/* ATM IPC design makes it possible to get the CFM before the ACK,
 * otherwise this could have simply been a state enum */
#define BL_CMD_WAIT_COMPLETE(flags) \
    (!(flags & (BL_CMD_FLAG_WAIT_ACK | BL_CMD_FLAG_WAIT_CFM)))

#define BL_CMD_MAX_QUEUED         8

struct bl_hw;
struct bl_cmd;
typedef int (*msg_cb_fct)(struct bl_hw *bl_hw, struct bl_cmd *cmd,
                          struct ipc_e2a_msg *msg);

enum bl_cmd_mgr_state {
    BL_CMD_MGR_STATE_DEINIT,
    BL_CMD_MGR_STATE_INITED,
    BL_CMD_MGR_STATE_CRASHED,
};

struct bl_cmd {
    struct list_head list;
    lmac_msg_id_t id;
    lmac_msg_id_t reqid;
    struct lmac_msg *a2e_msg;
    char            *e2a_msg;
    u32 tkn;
    u16 flags;

    struct completion complete;
    u32 result;
};

struct bl_cmd_mgr {
    enum bl_cmd_mgr_state state;
    spinlock_t lock;
    u32 next_tkn;
    u32 queue_sz;
    u32 max_queue_sz;

    struct list_head cmds;

    int  (*queue)(struct bl_cmd_mgr *, struct bl_cmd *);
    int  (*llind)(struct bl_cmd_mgr *, struct bl_cmd *);
    int  (*msgind)(struct bl_cmd_mgr *, struct ipc_e2a_msg *, msg_cb_fct);
    void (*print)(struct bl_cmd_mgr *);
    void (*drain)(struct bl_cmd_mgr *);
};

void cmd_dump(const struct bl_cmd *cmd);
void bl_cmd_mgr_init(struct bl_cmd_mgr *cmd_mgr);
void bl_cmd_mgr_deinit(struct bl_cmd_mgr *cmd_mgr);

#endif /* _BL_CMDS_H_ */
