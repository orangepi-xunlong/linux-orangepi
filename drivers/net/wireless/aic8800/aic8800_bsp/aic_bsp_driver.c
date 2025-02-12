/**
 ******************************************************************************
 *
 * aic_bsp_driver.c
 *
 * Copyright (C) RivieraWaves 2014-2019
 *
 ******************************************************************************
 */

#include <linux/list.h>
#include <linux/version.h>
#include <linux/firmware.h>
#include "aic_bsp_txrxif.h"
#include "aicsdio.h"
#include "aicusb.h"
#include "aic_bsp_driver.h"

#ifdef AICWF_USB_SUPPORT
static u32 sys_reboot_tbl[][2] = {
	{0x50017000, 0x0001ffff},
	{0x50017008, 0x00000002},
};
#endif

static void rwnx_set_cmd_tx(void *dev, struct lmac_msg *msg, uint len);
u8 binding_enc_data[16];
bool need_binding_verify;

#ifndef CONFIG_PLATFORM_ALLWINNER
int wcn_bind_verify_calculate_verify_data(uint8_t *din, uint8_t *dout)
{
	return 0;
}
#endif

static void cmd_dump(const struct rwnx_cmd *cmd)
{
	printk(KERN_CRIT "tkn[%d]  flags:%04x  result:%3d  cmd:%4d - reqcfm(%4d)\n",
		   cmd->tkn, cmd->flags, cmd->result, cmd->id, cmd->reqid);
}

static void cmd_complete(struct rwnx_cmd_mgr *cmd_mgr, struct rwnx_cmd *cmd)
{
	//printk("cmdcmp\n");
	lockdep_assert_held(&cmd_mgr->lock);

	list_del(&cmd->list);
	cmd_mgr->queue_sz--;

	cmd->flags |= RWNX_CMD_FLAG_DONE;
	if (cmd->flags & RWNX_CMD_FLAG_NONBLOCK) {
		kfree(cmd);
	} else {
		if (RWNX_CMD_WAIT_COMPLETE(cmd->flags)) {
			cmd->result = 0;
			complete(&cmd->complete);
		}
	}
}

static int cmd_mgr_queue(struct rwnx_cmd_mgr *cmd_mgr, struct rwnx_cmd *cmd)
{
	bool defer_push = false;
	int err = 0;

	spin_lock_bh(&cmd_mgr->lock);

	if (cmd_mgr->state == RWNX_CMD_MGR_STATE_CRASHED) {
		printk(KERN_CRIT"cmd queue crashed\n");
		cmd->result = -EPIPE;
		spin_unlock_bh(&cmd_mgr->lock);
		return -EPIPE;
	}

	if (!list_empty(&cmd_mgr->cmds)) {
		struct rwnx_cmd *last;

		if (cmd_mgr->queue_sz == cmd_mgr->max_queue_sz) {
			printk(KERN_CRIT"Too many cmds (%d) already queued\n",
				   cmd_mgr->max_queue_sz);
			cmd->result = -ENOMEM;
			spin_unlock_bh(&cmd_mgr->lock);
			return -ENOMEM;
		}
		last = list_entry(cmd_mgr->cmds.prev, struct rwnx_cmd, list);
		if (last->flags & (RWNX_CMD_FLAG_WAIT_ACK | RWNX_CMD_FLAG_WAIT_PUSH)) {
			cmd->flags |= RWNX_CMD_FLAG_WAIT_PUSH;
			defer_push = true;
		}
	}

	if (cmd->flags & RWNX_CMD_FLAG_REQ_CFM)
		cmd->flags |= RWNX_CMD_FLAG_WAIT_CFM;

	cmd->tkn    = cmd_mgr->next_tkn++;
	cmd->result = -EINTR;

	if (!(cmd->flags & RWNX_CMD_FLAG_NONBLOCK))
		init_completion(&cmd->complete);

	list_add_tail(&cmd->list, &cmd_mgr->cmds);
	cmd_mgr->queue_sz++;
	spin_unlock_bh(&cmd_mgr->lock);

	if (!defer_push) {
		rwnx_set_cmd_tx((void *)(cmd_mgr->aicdev), cmd->a2e_msg, sizeof(struct lmac_msg) + cmd->a2e_msg->param_len);
		kfree(cmd->a2e_msg);
	} else {
		printk("ERR: never defer push!!!!");
		return 0;
	}

	if (!(cmd->flags & RWNX_CMD_FLAG_NONBLOCK)) {
		unsigned long tout = msecs_to_jiffies(RWNX_CMD_TIMEOUT_MS * cmd_mgr->queue_sz);
		if (!wait_for_completion_timeout(&cmd->complete, tout)) {
			printk(KERN_CRIT"cmd timed-out\n");
			cmd_dump(cmd);
			spin_lock_bh(&cmd_mgr->lock);
			cmd_mgr->state = RWNX_CMD_MGR_STATE_CRASHED;
			if (!(cmd->flags & RWNX_CMD_FLAG_DONE)) {
				cmd->result = -ETIMEDOUT;
				cmd_complete(cmd_mgr, cmd);
			}
			spin_unlock_bh(&cmd_mgr->lock);
			err = -ETIMEDOUT;
		} else {
			kfree(cmd);
		}
	} else {
		cmd->result = 0;
	}

	return err;
}

static int cmd_mgr_run_callback(struct rwnx_cmd_mgr *cmd_mgr, struct rwnx_cmd *cmd,
								struct rwnx_cmd_e2amsg *msg, msg_cb_fct cb)
{
	int res;

	if (!cb) {
		return 0;
	}
	spin_lock(&cmd_mgr->cb_lock);
	res = cb(cmd, msg);
	spin_unlock(&cmd_mgr->cb_lock);

	return res;
}

static int cmd_mgr_msgind(struct rwnx_cmd_mgr *cmd_mgr, struct rwnx_cmd_e2amsg *msg,
						  msg_cb_fct cb)
{
	struct rwnx_cmd *cmd;
	bool found = false;

	//printk("cmd->id=%x\n", msg->id);
	spin_lock(&cmd_mgr->lock);
	list_for_each_entry(cmd, &cmd_mgr->cmds, list) {
		if (cmd->reqid == msg->id &&
			(cmd->flags & RWNX_CMD_FLAG_WAIT_CFM)) {

			if (!cmd_mgr_run_callback(cmd_mgr, cmd, msg, cb)) {
				found = true;
				cmd->flags &= ~RWNX_CMD_FLAG_WAIT_CFM;

				if (WARN((msg->param_len > RWNX_CMD_E2AMSG_LEN_MAX),
						 "Unexpect E2A msg len %d > %d\n", msg->param_len,
						 RWNX_CMD_E2AMSG_LEN_MAX)) {
					msg->param_len = RWNX_CMD_E2AMSG_LEN_MAX;
				}

				if (cmd->e2a_msg && msg->param_len)
					memcpy(cmd->e2a_msg, &msg->param, msg->param_len);

				if (RWNX_CMD_WAIT_COMPLETE(cmd->flags))
					cmd_complete(cmd_mgr, cmd);

				break;
			}
		}
	}
	spin_unlock(&cmd_mgr->lock);

	if (!found)
		cmd_mgr_run_callback(cmd_mgr, NULL, msg, cb);

	return 0;
}

static void cmd_mgr_print(struct rwnx_cmd_mgr *cmd_mgr)
{
	struct rwnx_cmd *cur;

	spin_lock_bh(&cmd_mgr->lock);
	list_for_each_entry(cur, &cmd_mgr->cmds, list) {
		cmd_dump(cur);
	}
	spin_unlock_bh(&cmd_mgr->lock);
}

static void cmd_mgr_drain(struct rwnx_cmd_mgr *cmd_mgr)
{
	struct rwnx_cmd *cur, *nxt;

	spin_lock_bh(&cmd_mgr->lock);
	list_for_each_entry_safe(cur, nxt, &cmd_mgr->cmds, list) {
		list_del(&cur->list);
		cmd_mgr->queue_sz--;
		if (!(cur->flags & RWNX_CMD_FLAG_NONBLOCK))
			complete(&cur->complete);
	}
	spin_unlock_bh(&cmd_mgr->lock);
}

static void rwnx_cmd_mgr_init(struct rwnx_cmd_mgr *cmd_mgr)
{
	cmd_mgr->max_queue_sz = RWNX_CMD_MAX_QUEUED;
	INIT_LIST_HEAD(&cmd_mgr->cmds);
	cmd_mgr->state = RWNX_CMD_MGR_STATE_INITED;
	spin_lock_init(&cmd_mgr->lock);
	spin_lock_init(&cmd_mgr->cb_lock);
	cmd_mgr->queue  = &cmd_mgr_queue;
	cmd_mgr->print  = &cmd_mgr_print;
	cmd_mgr->drain  = &cmd_mgr_drain;
	cmd_mgr->llind  = NULL;
	cmd_mgr->msgind = &cmd_mgr_msgind;
}

void rwnx_cmd_mgr_deinit(struct rwnx_cmd_mgr *cmd_mgr)
{
	cmd_mgr->print(cmd_mgr);
	cmd_mgr->drain(cmd_mgr);
	cmd_mgr->print(cmd_mgr);
	memset(cmd_mgr, 0, sizeof(*cmd_mgr));
}

static void rwnx_set_cmd_tx(void *dev, struct lmac_msg *msg, uint len)
{
	struct priv_dev *aicdev = (struct priv_dev *)dev;
	struct aicwf_bus *bus = aicdev->bus_if;
	u8 *buffer = bus->cmd_buf;
	u16 index = 0;

	memset(buffer, 0, CMD_BUF_MAX);
	buffer[0] = (len+4) & 0x00ff;
	buffer[1] = ((len+4) >> 8) &0x0f;
	buffer[2] = 0x11;
#if defined(AICWF_SDIO_SUPPORT)
	if (aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800D || aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800DC ||
		aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800DW)
		buffer[3] = 0x0;
	else if (aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800D80)
		buffer[3] = crc8_ponl_107(&buffer[0], 3); // crc8
#elif defined(AICWF_USB_SUPPORT)
	buffer[3] = 0x0;
#endif
	index += 4;
	//there is a dummy word
	index += 4;

	//make sure little endian
	put_u16(&buffer[index], msg->id);
	index += 2;
	put_u16(&buffer[index], msg->dest_id);
	index += 2;
	put_u16(&buffer[index], msg->src_id);
	index += 2;
	put_u16(&buffer[index], msg->param_len);
	index += 2;
	memcpy(&buffer[index], (u8 *)msg->param, msg->param_len);

	aicwf_bus_txmsg(bus, buffer, len + 8);
}

static inline void *rwnx_msg_zalloc(lmac_msg_id_t const id,
									lmac_task_id_t const dest_id,
									lmac_task_id_t const src_id,
									uint16_t const param_len)
{
	struct lmac_msg *msg;
	gfp_t flags;

	if (in_softirq())
		flags = GFP_ATOMIC;
	else
		flags = GFP_KERNEL;

	msg = (struct lmac_msg *)kzalloc(sizeof(struct lmac_msg) + param_len,
									 flags);
	if (msg == NULL) {
		printk(KERN_CRIT "%s: msg allocation failed\n", __func__);
		return NULL;
	}
	msg->id = id;
	msg->dest_id = dest_id;
	msg->src_id = src_id;
	msg->param_len = param_len;

	return msg->param;
}

static void rwnx_msg_free(struct lmac_msg *msg, const void *msg_params)
{
	kfree(msg);
}

static int rwnx_send_msg(struct priv_dev *aicdev, const void *msg_params,
						 int reqcfm, lmac_msg_id_t reqid, void *cfm)
{
	struct lmac_msg *msg;
	struct rwnx_cmd *cmd;
	bool nonblock;
	int ret = 0;

	msg = container_of((void *)msg_params, struct lmac_msg, param);
	if (aicdev->bus_if->state == BUS_DOWN_ST) {
		rwnx_msg_free(msg, msg_params);
		printk("bus is down\n");
		return 0;
	}

	nonblock = 0;
	cmd = kzalloc(sizeof(struct rwnx_cmd), nonblock ? GFP_ATOMIC : GFP_KERNEL);
	cmd->result  = -EINTR;
	cmd->id      = msg->id;
	cmd->reqid   = reqid;
	cmd->a2e_msg = msg;
	cmd->e2a_msg = cfm;
	if (nonblock)
		cmd->flags = RWNX_CMD_FLAG_NONBLOCK;
	if (reqcfm)
		cmd->flags |= RWNX_CMD_FLAG_REQ_CFM;

	if (reqcfm) {
		cmd->flags &= ~RWNX_CMD_FLAG_WAIT_ACK; // we don't need ack any more
		ret = aicdev->cmd_mgr.queue(&aicdev->cmd_mgr, cmd);
	} else {
		rwnx_set_cmd_tx((void *)(aicdev), cmd->a2e_msg, sizeof(struct lmac_msg) + cmd->a2e_msg->param_len);
	}

	if (!reqcfm)
		kfree(cmd);

	return ret;
}

int rwnx_send_dbg_mem_block_write_req(struct priv_dev *aicdev, u32 mem_addr,
									  u32 mem_size, u32 *mem_data)
{
	struct dbg_mem_block_write_req *mem_blk_write_req;

	/* Build the DBG_MEM_BLOCK_WRITE_REQ message */
	mem_blk_write_req = rwnx_msg_zalloc(DBG_MEM_BLOCK_WRITE_REQ, TASK_DBG, DRV_TASK_ID,
										sizeof(struct dbg_mem_block_write_req));
	if (!mem_blk_write_req)
		return -ENOMEM;

	/* Set parameters for the DBG_MEM_BLOCK_WRITE_REQ message */
	mem_blk_write_req->memaddr = mem_addr;
	mem_blk_write_req->memsize = mem_size;
	memcpy(mem_blk_write_req->memdata, mem_data, mem_size);

	/* Send the DBG_MEM_BLOCK_WRITE_REQ message to LMAC FW */
	return rwnx_send_msg(aicdev, mem_blk_write_req, 1, DBG_MEM_BLOCK_WRITE_CFM, NULL);
}

int rwnx_send_dbg_mem_read_req(struct priv_dev *aicdev, u32 mem_addr,
							   struct dbg_mem_read_cfm *cfm)
{
	struct dbg_mem_read_req *mem_read_req;

	/* Build the DBG_MEM_READ_REQ message */
	mem_read_req = rwnx_msg_zalloc(DBG_MEM_READ_REQ, TASK_DBG, DRV_TASK_ID,
								   sizeof(struct dbg_mem_read_req));
	if (!mem_read_req)
		return -ENOMEM;

	/* Set parameters for the DBG_MEM_READ_REQ message */
	mem_read_req->memaddr = mem_addr;

	/* Send the DBG_MEM_READ_REQ message to LMAC FW */
	return rwnx_send_msg(aicdev, mem_read_req, 1, DBG_MEM_READ_CFM, cfm);
}

int rwnx_send_dbg_mem_write_req(struct priv_dev *aicdev, u32 mem_addr, u32 mem_data)
{
	struct dbg_mem_write_req *mem_write_req;

	/* Build the DBG_MEM_WRITE_REQ message */
	mem_write_req = rwnx_msg_zalloc(DBG_MEM_WRITE_REQ, TASK_DBG, DRV_TASK_ID,
									sizeof(struct dbg_mem_write_req));
	if (!mem_write_req)
		return -ENOMEM;

	/* Set parameters for the DBG_MEM_WRITE_REQ message */
	mem_write_req->memaddr = mem_addr;
	mem_write_req->memdata = mem_data;

	/* Send the DBG_MEM_WRITE_REQ message to LMAC FW */
	return rwnx_send_msg(aicdev, mem_write_req, 1, DBG_MEM_WRITE_CFM, NULL);
}

int rwnx_send_dbg_mem_mask_write_req(struct priv_dev *aicdev, u32 mem_addr,
									 u32 mem_mask, u32 mem_data)
{
	struct dbg_mem_mask_write_req *mem_mask_write_req;

	/* Build the DBG_MEM_MASK_WRITE_REQ message */
	mem_mask_write_req = rwnx_msg_zalloc(DBG_MEM_MASK_WRITE_REQ, TASK_DBG, DRV_TASK_ID,
										 sizeof(struct dbg_mem_mask_write_req));
	if (!mem_mask_write_req)
		return -ENOMEM;

	/* Set parameters for the DBG_MEM_MASK_WRITE_REQ message */
	mem_mask_write_req->memaddr = mem_addr;
	mem_mask_write_req->memmask = mem_mask;
	mem_mask_write_req->memdata = mem_data;

	/* Send the DBG_MEM_MASK_WRITE_REQ message to LMAC FW */
	return rwnx_send_msg(aicdev, mem_mask_write_req, 1, DBG_MEM_MASK_WRITE_CFM, NULL);
}

int rwnx_send_dbg_binding_req(struct priv_dev *aicdev, u8 *dout, u8 *binding_status)
{
	struct dbg_binding_req *binding_req;

	/* Build the DBG_BINDING_REQ message */
	binding_req = rwnx_msg_zalloc(DBG_BINDING_REQ, TASK_DBG, DRV_TASK_ID,
									sizeof(struct dbg_binding_req));
	if (!binding_req)
		return -ENOMEM;

	memcpy(binding_req->driver_data, dout, 16);

	/* Send the DBG_MEM_MASK_WRITE_REQ message to LMAC FW */
	return rwnx_send_msg(aicdev, binding_req, 1, DBG_BINDING_CFM, binding_status);
}

int rwnx_send_dbg_start_app_req(struct priv_dev *aicdev, u32 boot_addr, u32 boot_type, struct dbg_start_app_cfm *start_app_cfm)
{
	struct dbg_start_app_req *start_app_req;

	/* Build the DBG_START_APP_REQ message */
	start_app_req = rwnx_msg_zalloc(DBG_START_APP_REQ, TASK_DBG, DRV_TASK_ID,
									sizeof(struct dbg_start_app_req));
	if (!start_app_req) {
		printk("start app nomen\n");
		return -ENOMEM;
	}

	/* Set parameters for the DBG_START_APP_REQ message */
	start_app_req->bootaddr = boot_addr;
	start_app_req->boottype = boot_type;

	/* Send the DBG_START_APP_REQ message to LMAC FW */
#if defined(AICWF_SDIO_SUPPORT)
	return rwnx_send_msg(aicdev, start_app_req, 1, DBG_START_APP_CFM, start_app_cfm);
#elif defined(AICWF_USB_SUPPORT)
	return rwnx_send_msg(aicdev, start_app_req, 0, 0, NULL);
#endif
}

int rwnx_send_rf_config_req(struct priv_dev *aicdev, u8 ofst, u8 sel, u8 *tbl, u16 len)
{
	struct mm_set_rf_config_req *rf_config_req;
	int error;

	/* Build the MM_SET_RF_CONFIG_REQ message */
	rf_config_req = rwnx_msg_zalloc(MM_SET_RF_CONFIG_REQ, TASK_MM, DRV_TASK_ID,
								  sizeof(struct mm_set_rf_config_req));

	if (!rf_config_req) {
		return -ENOMEM;
	}

	rf_config_req->table_sel = sel;
	rf_config_req->table_ofst = ofst;
	rf_config_req->table_num = 16;
	rf_config_req->deft_page = 0;

	memcpy(rf_config_req->data, tbl, len);

	/* Send the MM_SET_RF_CONFIG_REQ message to UMAC FW */
	error = rwnx_send_msg(aicdev, rf_config_req, 1, MM_SET_RF_CONFIG_CFM, NULL);

	return error;
};

static inline int dbg_binding_ind(struct rwnx_cmd *cmd, struct ipc_e2a_msg *msg)
{
	struct dbg_binding_ind *ind = (struct dbg_binding_ind *)msg->param;
	memcpy(binding_enc_data, ind->enc_data, 16);
	need_binding_verify = true;

	return 0;
}

static msg_cb_fct dbg_hdlrs[MSG_I(DBG_MAX)] = {
	[MSG_I(DBG_BINDING_IND)] = (msg_cb_fct)dbg_binding_ind,
};

static msg_cb_fct mm_hdlrs[MSG_I(MM_MAX)] = {
};

static msg_cb_fct *msg_hdlrs[] = {
	[TASK_MM]  = mm_hdlrs,
	[TASK_DBG] = dbg_hdlrs,
};

void rwnx_rx_handle_msg(struct priv_dev *aicdev, struct ipc_e2a_msg *msg)
{
	aicdev->cmd_mgr.msgind(&aicdev->cmd_mgr, msg,
							msg_hdlrs[MSG_T(msg->id)][MSG_I(msg->id)]);
}

int rwnx_plat_bin_fw_upload_android(struct priv_dev *aicdev, u32 fw_addr,
							   const char *filename)
{
	unsigned int i = 0;
	int size;
	u32 *dst = NULL;
	int err = 0;

	const struct firmware *fw = NULL;
	int ret = request_firmware(&fw, filename, NULL);

	printk("rwnx_request_firmware, name: %s\n", filename);
	if (ret < 0) {
		printk("Load %s fail\n", filename);
		return ret;
	}

	size = fw->size;
	dst = (u32 *)fw->data;

	if (size <= 0) {
		printk("wrong size of firmware file\n");
		release_firmware(fw);
		return -1;
	}

	/* Copy the file on the Embedded side */
	if (size > 1024) {// > 1KB data
		for (i = 0; i < (size - 1024); i += 1024) {//each time write 1KB
			err = rwnx_send_dbg_mem_block_write_req(aicdev, fw_addr + i, 1024, dst + i / 4);
			if (err) {
				printk("bin upload fail: %x, err:%d\r\n", fw_addr + i, err);
				break;
			}
		}
	}

	if (!err && (i < size)) {// <1KB data
		err = rwnx_send_dbg_mem_block_write_req(aicdev, fw_addr + i, size - i, dst + i / 4);
		if (err) {
			printk("bin upload fail: %x, err:%d\r\n", fw_addr + i, err);
		}
	}

	release_firmware(fw);
	return err;
}

int aicbt_patch_table_free(struct aicbt_patch_table **head)
{
	struct aicbt_patch_table *p = *head, *n = NULL;
	while (p) {
		n = p->next;
		kfree(p->name);
		kfree(p->data);
		kfree(p);
		p = n;
	}
	*head = NULL;
	return 0;
}

struct aicbt_patch_table *aicbt_patch_table_alloc(const char *filename)
{
	uint8_t *rawdata = NULL, *p;
	int size;
	struct aicbt_patch_table *head = NULL, *new = NULL, *cur = NULL;

	const struct firmware *fw = NULL;
	int ret = request_firmware(&fw, filename, NULL);

	printk("rwnx_request_firmware, name: %s\n", filename);
	if (ret < 0) {
		printk("Load %s fail\n", filename);
		return NULL;
	}

	rawdata = (uint8_t *)fw->data;
	size = fw->size;

	if (size <= 0) {
		printk("wrong size of firmware file\n");
		goto err;
	}

	p = rawdata;
	if (memcmp(p, AICBT_PT_TAG, sizeof(AICBT_PT_TAG) < 16 ? sizeof(AICBT_PT_TAG) : 16)) {
		printk("TAG err\n");
		goto err;
	}
	p += 16;

	while (p - rawdata < size) {
		new = (struct aicbt_patch_table *)kzalloc(sizeof(struct aicbt_patch_table), GFP_KERNEL);
		memset(new, 0, sizeof(struct aicbt_patch_table));
		if (head == NULL) {
			head = new;
			cur  = new;
		} else {
			cur->next = new;
			cur = cur->next;
		}

		cur->name = (char *)kzalloc(sizeof(char) * 16, GFP_KERNEL);
		memcpy(cur->name, p, 16);
		p += 16;

		cur->type = *(uint32_t *)p;
		p += 4;

		cur->len = *(uint32_t *)p;
		p += 4;

		cur->data = (uint32_t *)kzalloc(sizeof(uint8_t) * cur->len * 8, GFP_KERNEL);
		memcpy(cur->data, p, cur->len * 8);
		p += cur->len * 8;
	}
	release_firmware(fw);
	return head;

err:
	aicbt_patch_table_free(&head);
	release_firmware(fw);
	return NULL;
}

int aicbt_patch_info_unpack(struct aicbt_patch_table *head, struct aicbt_patch_info_t *patch_info)
{
	struct aicbt_patch_table *p;
	int ret = -1;
	u8 *p_adid = (u8 *)&patch_info->adid_addrinf;

	for (p = head; p != NULL; p = p->next) {
		if (AICBT_PT_INF == p->type) {
			patch_info->info_len = p->len;
			if (patch_info->info_len > 0) {
				memcpy(p_adid, p->data, patch_info->info_len * sizeof(uint32_t) * 2);
				ret = 0;
			}
		}

		if (AICBT_PT_VER == p->type) {
			printk("%s bt patch version: %s\n", __func__, (char *)p->data);
		}
	}

	return ret;
}

int aicbt_patch_table_load(struct priv_dev *aicdev, struct aicbt_info_t *aicbt_info, struct aicbt_patch_table *head)
{
	struct aicbt_patch_table *p;
	int ret = 0, i;
	uint32_t *data = NULL;

	printk("%s bt uart baud: %d, flowctrl: %d, lpm_enable: %d, tx_pwr: %d, bt mode:%d.\n", __func__,
			aicbt_info->uart_baud, aicbt_info->uart_flowctrl, aicbt_info->lpm_enable, aicbt_info->txpwr_lvl, aicbt_info->btmode);

	for (p = head; p != NULL; p = p->next) {
		data = p->data;
		if (AICBT_PT_BTMODE == p->type) {
			*(data + 1)  = aicbsp_info.hwinfo < 0;
			*(data + 3)  = aicbsp_info.hwinfo;
			*(data + 5)  = 0;//aicbsp_info.cpmode;

			*(data + 7)  = aicbt_info->btmode;
			*(data + 9)  = aicbt_info->btport;
			*(data + 11) = aicbt_info->uart_baud;
			*(data + 13) = aicbt_info->uart_flowctrl;
			*(data + 15) = aicbt_info->lpm_enable;
			*(data + 17) = aicbt_info->txpwr_lvl;
		}

		if (AICBT_PT_VER == p->type) {
			continue;
		}

		for (i = 0; i < p->len; i++) {
			ret = rwnx_send_dbg_mem_write_req(aicdev, *data, *(data + 1));
			if (ret != 0)
				return ret;
			data += 2;
		}
		if (p->type == AICBT_PT_PWRON)
			udelay(500);
	}
	return 0;
}

#ifdef AICWF_USB_SUPPORT
int aicbsp_system_reboot(struct priv_dev *aicdev)
{
	int syscfg_num;
	int ret, cnt;

	syscfg_num = sizeof(sys_reboot_tbl) / sizeof(u32) / 2;
	for (cnt = 0; cnt < syscfg_num; cnt++) {
		ret = rwnx_send_dbg_mem_write_req(aicdev, sys_reboot_tbl[cnt][0], sys_reboot_tbl[cnt][1]);
		if (ret) {
			printk("%x write fail: %d\n", sys_reboot_tbl[cnt][0], ret);
			return ret;
		}
	}
	return 0;
}

int rwnx_send_reboot(struct priv_dev *aicdev)
{
	int ret = 0;
	u32 delay = 2 *1000; //1s

	printk("%s enter \r\n", __func__);

	ret = rwnx_send_dbg_start_app_req(aicdev, delay, HOST_START_APP_REBOOT, NULL);
	return ret;
}
#endif

int aicbsp_platform_init(struct priv_dev *aicdev)
{
	rwnx_cmd_mgr_init(&aicdev->cmd_mgr);
	aicdev->cmd_mgr.aicdev = (void *)aicdev;
	return 0;
}

void aicbsp_platform_deinit(struct priv_dev *aicdev)
{
	(void)aicdev;
}

int aicbsp_driver_fw_init(struct priv_dev *aicdev)
{
	if (aicbsp_info.chipinfo == NULL)
		goto err;

	if (aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800D)
		return aicbsp_8800d_fw_init(aicdev);

	if (aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800DC || aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800DW)
		return aicbsp_8800dc_fw_init(aicdev);

	if (aicbsp_info.chipinfo->chipid == PRODUCT_ID_AIC8800D80)
		return aicbsp_8800d80_fw_init(aicdev);

err:
	pr_err("%s no matched chip found\n", __func__);
	return -1;
}

int aicbsp_driver_btmode_reinit(struct aicbt_info_t *aicbt_info)
{
	if (aicbsp_info.btmode >=  AICBT_BTMODE_BT_ONLY_SW && aicbsp_info.btmode != aicbt_info->btmode) {
		aicbt_info->btmode = aicbsp_info.btmode;
		printk("Using Android solution btmode = 0x%x\n", aicbt_info->btmode);
		return 0;
	}
	return -1;
}

int aicbsp_get_feature(struct aicbsp_feature_t *feature)
{
	feature->cpmode     = aicbsp_info.cpmode;
	feature->chipinfo   = aicbsp_info.chipinfo;
	feature->sdio_clock = aicbsp_info.sdio_clock;
	feature->sdio_phase = aicbsp_info.sdio_phase;
	feature->hwinfo     = aicbsp_info.hwinfo;
	feature->fwlog_en   = aicbsp_info.fwlog_en;

	return 0;
}
EXPORT_SYMBOL_GPL(aicbsp_get_feature);

#ifdef AICBSP_RESV_MEM_SUPPORT
static struct skb_buff_pool resv_skb[] = {
	{AIC_RESV_MEM_TXDATA, 1536*64, "resv_mem_txdata", 0, NULL},
};

int aicbsp_resv_mem_init(void)
{
	int i = 0;
	for (i = 0; i < sizeof(resv_skb) / sizeof(resv_skb[0]); i++) {
		resv_skb[i].skb = dev_alloc_skb(resv_skb[i].size);
	}
	return 0;
}

int aicbsp_resv_mem_deinit(void)
{
	int i = 0;
	for (i = 0; i < sizeof(resv_skb) / sizeof(resv_skb[0]); i++) {
		if (resv_skb[i].used == 0 && resv_skb[i].skb)
			dev_kfree_skb(resv_skb[i].skb);
	}
	return 0;
}

struct sk_buff *aicbsp_resv_mem_alloc_skb(unsigned int length, uint32_t id)
{
	if (resv_skb[id].size < length) {
		pr_err("aicbsp: %s, no enough mem\n", __func__);
		goto fail;
	}

	if (resv_skb[id].used) {
		pr_err("aicbsp: %s, mem in use\n", __func__);
		goto fail;
	}

	if (resv_skb[id].skb == NULL) {
		pr_err("aicbsp: %s, mem not initialazed\n", __func__);
		resv_skb[id].skb = dev_alloc_skb(resv_skb[id].size);
		if (resv_skb[id].skb == NULL) {
			pr_err("aicbsp: %s, mem reinitial still fail\n", __func__);
			goto fail;
		}
	}

	printk("aicbsp: %s, alloc %s succuss, id: %d, size: %d\n", __func__,
			resv_skb[id].name, resv_skb[id].id, resv_skb[id].size);

	resv_skb[id].used = 1;
	return resv_skb[id].skb;

fail:
	return NULL;
}
EXPORT_SYMBOL_GPL(aicbsp_resv_mem_alloc_skb);

void aicbsp_resv_mem_kfree_skb(struct sk_buff *skb, uint32_t id)
{
	resv_skb[id].used = 0;
	printk("aicbsp: %s, free %s succuss, id: %d, size: %d\n", __func__,
			resv_skb[id].name, resv_skb[id].id, resv_skb[id].size);
}
EXPORT_SYMBOL_GPL(aicbsp_resv_mem_kfree_skb);

#else

int aicbsp_resv_mem_init(void)
{
	return 0;
}

int aicbsp_resv_mem_deinit(void)
{
	return 0;
}

#endif
