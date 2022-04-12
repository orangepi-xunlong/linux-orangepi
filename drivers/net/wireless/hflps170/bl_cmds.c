/**
 ****************************************************************************************
 *
 * @file bl_cmds.c
 *
 * @brief Handles queueing (push to IPC, ack/cfm from IPC) of commands issued to LMAC FW
 *
 * Copyright (C) BouffaloLab 2017-2018
 *
 ****************************************************************************************
 */

#include <linux/list.h>

#include "bl_cmds.h"
#include "bl_defs.h"
#include "bl_msg_tx.h"
#include "bl_strs.h"
#define CREATE_TRACE_POINTS
#include "bl_events.h"
#include "bl_debugfs.h"
#include "bl_sdio.h"
#include "bl_v7.h"
#include "bl_irqs.h"


static void cmd_mgr_print(struct bl_cmd_mgr *cmd_mgr);

void cmd_dump(const struct bl_cmd *cmd)
{
    BL_DBG("tkn[%d]  flags:%04x  result:%3d  cmd:%4d-%-24s - reqcfm(%4d-%-s)\n",
           cmd->tkn, cmd->flags, cmd->result, cmd->id, BL_ID2STR(cmd->id),
           cmd->reqid, cmd->reqid != (lmac_msg_id_t)-1 ? BL_ID2STR(cmd->reqid) : "none");
}

static void cmd_complete(struct bl_cmd_mgr *cmd_mgr, struct bl_cmd *cmd)
{
    lockdep_assert_held(&cmd_mgr->lock);

    list_del(&cmd->list);
    cmd_mgr->queue_sz--;

    cmd->flags |= BL_CMD_FLAG_DONE;
    if (cmd->flags & BL_CMD_FLAG_NONBLOCK) {
        kfree(cmd);
    } else {
        if (BL_CMD_WAIT_COMPLETE(cmd->flags)) {
            cmd->result = 0;
            complete(&cmd->complete);
        }
    }
}

static int cmd_mgr_queue(struct bl_cmd_mgr *cmd_mgr, struct bl_cmd *cmd)
{
    struct bl_hw *bl_hw = container_of(cmd_mgr, struct bl_hw, cmd_mgr);
    struct bl_cmd *last;
    unsigned long tout;
    bool defer_push = false;

    BL_DBG(BL_FN_ENTRY_STR);
    trace_msg_send(cmd->id);

    spin_lock_bh(&cmd_mgr->lock);

    if (cmd_mgr->state == BL_CMD_MGR_STATE_CRASHED) {
        printk(KERN_CRIT"cmd queue crashed\n");
        cmd->result = -EPIPE;
        spin_unlock_bh(&cmd_mgr->lock);
        return -EPIPE;
    }

    if (!list_empty(&cmd_mgr->cmds)) {
        if (cmd_mgr->queue_sz == cmd_mgr->max_queue_sz) {
            printk(KERN_CRIT"Too many cmds (%d) already queued\n",
                   cmd_mgr->max_queue_sz);
            cmd->result = -ENOMEM;
            spin_unlock_bh(&cmd_mgr->lock);
            return -ENOMEM;
        }
        last = list_entry(cmd_mgr->cmds.prev, struct bl_cmd, list);
        if (last->flags & (BL_CMD_FLAG_WAIT_ACK | BL_CMD_FLAG_WAIT_PUSH)) {
            cmd->flags |= BL_CMD_FLAG_WAIT_PUSH;
            defer_push = true;
        }
    }

    cmd->flags |= BL_CMD_FLAG_WAIT_ACK;
    if (cmd->flags & BL_CMD_FLAG_REQ_CFM)
        cmd->flags |= BL_CMD_FLAG_WAIT_CFM;

    cmd->tkn    = cmd_mgr->next_tkn++;
    cmd->result = -EINTR;

    if (!(cmd->flags & BL_CMD_FLAG_NONBLOCK))
        init_completion(&cmd->complete);

    list_add_tail(&cmd->list, &cmd_mgr->cmds);
    cmd_mgr->queue_sz++;
    tout = msecs_to_jiffies(BL_80211_CMD_TIMEOUT_MS * cmd_mgr->queue_sz);
    spin_unlock_bh(&cmd_mgr->lock);

    if (!defer_push) {
		ASSERT_ERR(!(bl_hw->ipc_env->msga2e_hostid));
		bl_hw->ipc_env->msga2e_hostid = (void *)cmd;
		spin_lock_bh(&bl_hw->cmd_lock);
		bl_hw->cmd_sent = true;
		spin_unlock_bh(&bl_hw->cmd_lock);
		bl_queue_main_work(bl_hw);
    }

    BL_DBG("send: cmd:%4d-%-24s\n", cmd->id, BL_ID2STR(cmd->id));

    if (!(cmd->flags & BL_CMD_FLAG_NONBLOCK)) {
        if (!wait_for_completion_timeout(&cmd->complete, tout)) {
			printk("wait for cmd cfm timeout!\n");
            cmd_dump(cmd);
            spin_lock_bh(&cmd_mgr->lock);
            cmd_mgr->state = BL_CMD_MGR_STATE_CRASHED;
            if (!(cmd->flags & BL_CMD_FLAG_DONE)) {
                cmd->result = -ETIMEDOUT;
                cmd_complete(cmd_mgr, cmd);
            }
            spin_unlock_bh(&cmd_mgr->lock);
        }
    } else {
        cmd->result = 0;
    }

    return 0;
}

static int cmd_mgr_llind(struct bl_cmd_mgr *cmd_mgr, struct bl_cmd *cmd)
{
    struct bl_cmd *cur, *acked = NULL, *next = NULL;
	struct bl_hw *bl_hw = container_of(cmd_mgr, struct bl_hw, cmd_mgr);
	
    BL_DBG(BL_FN_ENTRY_STR);

    spin_lock(&cmd_mgr->lock);
    list_for_each_entry(cur, &cmd_mgr->cmds, list) {
        if (!acked) {
            if (cur->tkn == cmd->tkn) {
                if (WARN_ON_ONCE(cur != cmd)) {
					cmd_mgr_print(cmd_mgr);
                    //cmd_dump(cmd);
                }
                acked = cur;
                continue;
            }
        }
        if (cur->flags & BL_CMD_FLAG_WAIT_PUSH) {
                next = cur;
                break;
        }
    }
    if (!acked) {
        printk(KERN_CRIT "Error: acked cmd not found\n");
    } else {
        cmd->flags &= ~BL_CMD_FLAG_WAIT_ACK;
        if (BL_CMD_WAIT_COMPLETE(cmd->flags)) {
            cmd_complete(cmd_mgr, cmd);
		}
    }
    if (next) {
		ASSERT_ERR(!(bl_hw->ipc_env->msga2e_hostid));
		bl_hw->ipc_env->msga2e_hostid = (void *)next;
		spin_lock_bh(&bl_hw->cmd_lock);
		bl_hw->cmd_sent = true;
		spin_unlock_bh(&bl_hw->cmd_lock);
		bl_queue_main_work(bl_hw);
    }
    spin_unlock(&cmd_mgr->lock);

    return 0;
}

static int cmd_mgr_msgind(struct bl_cmd_mgr *cmd_mgr, struct ipc_e2a_msg *msg, msg_cb_fct cb)
{
    struct bl_hw *bl_hw = container_of(cmd_mgr, struct bl_hw, cmd_mgr);
    struct bl_cmd *cmd;
    bool found = false;

    BL_DBG(BL_FN_ENTRY_STR);
    trace_msg_recv(msg->id);

    spin_lock(&cmd_mgr->lock);
    list_for_each_entry(cmd, &cmd_mgr->cmds, list) {
        if (cmd->reqid == msg->id &&
            (cmd->flags & BL_CMD_FLAG_WAIT_CFM)) {
            if (!cb || (cb && !cb(bl_hw, cmd, msg))) {
                found = true;
                cmd->flags &= ~BL_CMD_FLAG_WAIT_CFM;

                if (cmd->e2a_msg && msg->param_len)
                    memcpy(cmd->e2a_msg, &msg->param, msg->param_len);

                if (BL_CMD_WAIT_COMPLETE(cmd->flags)){
                    cmd_complete(cmd_mgr, cmd);
				}

                break;
            }
        }
    }
    spin_unlock(&cmd_mgr->lock);

    if (!found && cb)
	{
        cb(bl_hw, NULL, msg);
	}

    return 0;
}

static void cmd_mgr_print(struct bl_cmd_mgr *cmd_mgr)
{
    struct bl_cmd *cur;

    spin_lock_bh(&cmd_mgr->lock);
    BL_DBG("q_sz/max: %2d / %2d - next tkn: %d\n",
             cmd_mgr->queue_sz, cmd_mgr->max_queue_sz,
             cmd_mgr->next_tkn);
    list_for_each_entry(cur, &cmd_mgr->cmds, list) {
        cmd_dump(cur);
    }
    spin_unlock_bh(&cmd_mgr->lock);
}

static void cmd_mgr_drain(struct bl_cmd_mgr *cmd_mgr)
{
    struct bl_cmd *cur, *nxt;

    BL_DBG(BL_FN_ENTRY_STR);

    spin_lock_bh(&cmd_mgr->lock);
    list_for_each_entry_safe(cur, nxt, &cmd_mgr->cmds, list) {
        list_del(&cur->list);
        cmd_mgr->queue_sz--;
        if (!(cur->flags & BL_CMD_FLAG_NONBLOCK))
            complete(&cur->complete);
    }
    spin_unlock_bh(&cmd_mgr->lock);
}

void bl_cmd_mgr_init(struct bl_cmd_mgr *cmd_mgr)
{
    BL_DBG(BL_FN_ENTRY_STR);

    INIT_LIST_HEAD(&cmd_mgr->cmds);
    spin_lock_init(&cmd_mgr->lock);
    cmd_mgr->max_queue_sz = BL_CMD_MAX_QUEUED;
    cmd_mgr->queue  = &cmd_mgr_queue;
    cmd_mgr->print  = &cmd_mgr_print;
    cmd_mgr->drain  = &cmd_mgr_drain;
    cmd_mgr->llind  = &cmd_mgr_llind;
    cmd_mgr->msgind = &cmd_mgr_msgind;
}

void bl_cmd_mgr_deinit(struct bl_cmd_mgr *cmd_mgr)
{
    cmd_mgr->print(cmd_mgr);
    cmd_mgr->drain(cmd_mgr);
    cmd_mgr->print(cmd_mgr);
    memset(cmd_mgr, 0, sizeof(*cmd_mgr));
}
