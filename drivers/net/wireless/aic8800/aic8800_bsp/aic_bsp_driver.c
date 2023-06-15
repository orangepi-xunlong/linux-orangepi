/**
 ******************************************************************************
 *
 * rwnx_cmds.c
 *
 * Handles queueing (push to IPC, ack/cfm from IPC) of commands issued to
 * LMAC FW
 *
 * Copyright (C) RivieraWaves 2014-2019
 *
 ******************************************************************************
 */

#include <linux/list.h>
#include <linux/version.h>
#include <linux/firmware.h>
#include "aicsdio_txrxif.h"
#include "aicsdio.h"
#include "aic_bsp_driver.h"

static u8 binding_enc_data[16];
static bool need_binding_verify;

int wcn_bind_verify_calculate_verify_data(uint8_t *din, uint8_t *dout);
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
		//printk("queue:id=%x, param_len=%u\n", cmd->a2e_msg->id, cmd->a2e_msg->param_len);
		rwnx_set_cmd_tx((void *)(cmd_mgr->sdiodev), cmd->a2e_msg, sizeof(struct lmac_msg) + cmd->a2e_msg->param_len);
		//rwnx_ipc_msg_push(rwnx_hw, cmd, RWNX_CMD_A2EMSG_LEN(cmd->a2e_msg));
		kfree(cmd->a2e_msg);
	} else {
		//WAKE_CMD_WORK(cmd_mgr);
		printk("ERR: never defer push!!!!");
		return 0;
	}

	if (!(cmd->flags & RWNX_CMD_FLAG_NONBLOCK)) {
		unsigned long tout = msecs_to_jiffies(RWNX_80211_CMD_TIMEOUT_MS * cmd_mgr->queue_sz);
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

void rwnx_cmd_mgr_init(struct rwnx_cmd_mgr *cmd_mgr)
{
	cmd_mgr->max_queue_sz = RWNX_CMD_MAX_QUEUED;
	INIT_LIST_HEAD(&cmd_mgr->cmds);
	cmd_mgr->state = RWNX_CMD_MGR_STATE_INITED;
	spin_lock_init(&cmd_mgr->lock);
	spin_lock_init(&cmd_mgr->cb_lock);
	cmd_mgr->queue  = &cmd_mgr_queue;
	cmd_mgr->print  = &cmd_mgr_print;
	cmd_mgr->drain  = &cmd_mgr_drain;
	cmd_mgr->llind  = NULL;//&cmd_mgr_llind;
	cmd_mgr->msgind = &cmd_mgr_msgind;

#if 0
	INIT_WORK(&cmd_mgr->cmdWork, cmd_mgr_task_process);
	cmd_mgr->cmd_wq = create_singlethread_workqueue("cmd_wq");
	if (!cmd_mgr->cmd_wq) {
		txrx_err("insufficient memory to create cmd workqueue.\n");
		return;
	}
#endif
}

void rwnx_cmd_mgr_deinit(struct rwnx_cmd_mgr *cmd_mgr)
{
	cmd_mgr->print(cmd_mgr);
	cmd_mgr->drain(cmd_mgr);
	cmd_mgr->print(cmd_mgr);
	memset(cmd_mgr, 0, sizeof(*cmd_mgr));
}

void rwnx_set_cmd_tx(void *dev, struct lmac_msg *msg, uint len)
{
	struct aic_sdio_dev *sdiodev = (struct aic_sdio_dev *)dev;
	struct aicwf_bus *bus = sdiodev->bus_if;
	u8 *buffer = bus->cmd_buf;
	u16 index = 0;

	memset(buffer, 0, CMD_BUF_MAX);
	buffer[0] = (len+4) & 0x00ff;
	buffer[1] = ((len+4) >> 8) &0x0f;
	buffer[2] = 0x11;
	buffer[3] = 0x0;
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


static int rwnx_send_msg(struct aic_sdio_dev *sdiodev, const void *msg_params,
						 int reqcfm, lmac_msg_id_t reqid, void *cfm)
{
	struct lmac_msg *msg;
	struct rwnx_cmd *cmd;
	bool nonblock;
	int ret = 0;

	msg = container_of((void *)msg_params, struct lmac_msg, param);
	if (sdiodev->bus_if->state == BUS_DOWN_ST) {
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
		ret = sdiodev->cmd_mgr.queue(&sdiodev->cmd_mgr, cmd);
	} else {
		rwnx_set_cmd_tx((void *)(sdiodev), cmd->a2e_msg, sizeof(struct lmac_msg) + cmd->a2e_msg->param_len);
	}

	if (!reqcfm)
		kfree(cmd);

	return ret;
}


int rwnx_send_dbg_mem_block_write_req(struct aic_sdio_dev *sdiodev, u32 mem_addr,
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
	return rwnx_send_msg(sdiodev, mem_blk_write_req, 1, DBG_MEM_BLOCK_WRITE_CFM, NULL);
}

int rwnx_send_dbg_mem_read_req(struct aic_sdio_dev *sdiodev, u32 mem_addr,
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
	return rwnx_send_msg(sdiodev, mem_read_req, 1, DBG_MEM_READ_CFM, cfm);
}


int rwnx_send_dbg_mem_write_req(struct aic_sdio_dev *sdiodev, u32 mem_addr, u32 mem_data)
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
	return rwnx_send_msg(sdiodev, mem_write_req, 1, DBG_MEM_WRITE_CFM, NULL);
}

int rwnx_send_dbg_mem_mask_write_req(struct aic_sdio_dev *sdiodev, u32 mem_addr,
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
	return rwnx_send_msg(sdiodev, mem_mask_write_req, 1, DBG_MEM_MASK_WRITE_CFM, NULL);
}

int rwnx_send_dbg_binding_req(struct aic_sdio_dev *sdiodev, u8 *dout, u8 *binding_status)
{
	struct dbg_binding_req *binding_req;

	/* Build the DBG_BINDING_REQ message */
	binding_req = rwnx_msg_zalloc(DBG_BINDING_REQ, TASK_DBG, DRV_TASK_ID,
									sizeof(struct dbg_binding_req));
	if (!binding_req)
		return -ENOMEM;

	memcpy(binding_req->driver_data, dout, 16);

	/* Send the DBG_MEM_MASK_WRITE_REQ message to LMAC FW */
	return rwnx_send_msg(sdiodev, binding_req, 1, DBG_BINDING_CFM, binding_status);
}

int rwnx_send_dbg_start_app_req(struct aic_sdio_dev *sdiodev, u32 boot_addr, u32 boot_type, struct dbg_start_app_cfm *start_app_cfm)
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
	return rwnx_send_msg(sdiodev, start_app_req, 1, DBG_START_APP_CFM, start_app_cfm);
}

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

static msg_cb_fct *msg_hdlrs[] = {
	[TASK_DBG] = dbg_hdlrs,
};

void rwnx_rx_handle_msg(struct aic_sdio_dev *sdiodev, struct ipc_e2a_msg *msg)
{
	sdiodev->cmd_mgr.msgind(&sdiodev->cmd_mgr, msg,
							msg_hdlrs[MSG_T(msg->id)][MSG_I(msg->id)]);
}

int rwnx_plat_bin_fw_upload_android(struct aic_sdio_dev *sdiodev, u32 fw_addr,
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
			err = rwnx_send_dbg_mem_block_write_req(sdiodev, fw_addr + i, 1024, dst + i / 4);
			if (err) {
				printk("bin upload fail: %x, err:%d\r\n", fw_addr + i, err);
				break;
			}
		}
	}

	if (!err && (i < size)) {// <1KB data
		err = rwnx_send_dbg_mem_block_write_req(sdiodev, fw_addr + i, size - i, dst + i / 4);
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

int aicbt_patch_trap_data_load(struct aic_sdio_dev *sdiodev)
{
	uint32_t fw_ram_adid_base_addr = FW_RAM_ADID_BASE_ADDR;
	if (aicbsp_info.chip_rev != CHIP_REV_U02)
		fw_ram_adid_base_addr = FW_RAM_ADID_BASE_ADDR_U03;

	if (rwnx_plat_bin_fw_upload_android(sdiodev, fw_ram_adid_base_addr, aicbsp_firmware_list[aicbsp_info.cpmode].bt_adid))
		return -1;
	if (rwnx_plat_bin_fw_upload_android(sdiodev, FW_RAM_PATCH_BASE_ADDR, aicbsp_firmware_list[aicbsp_info.cpmode].bt_patch))
		return -1;
	return 0;
}

static struct aicbt_info_t aicbt_info = {
	.btmode        = AICBT_BTMODE_DEFAULT,
	.btport        = AICBT_BTPORT_DEFAULT,
	.uart_baud     = AICBT_UART_BAUD_DEFAULT,
	.uart_flowctrl = AICBT_UART_FC_DEFAULT,
};

int aicbt_patch_table_load(struct aic_sdio_dev *sdiodev)
{
	struct aicbt_patch_table *head, *p;
	int ret = 0, i;
	uint32_t *data = NULL;
	head = aicbt_patch_table_alloc(aicbsp_firmware_list[aicbsp_info.cpmode].bt_table);
	for (p = head; p != NULL; p = p->next) {
		data = p->data;
		if (AICBT_PT_BTMODE == p->type) {
			*(data + 1)  = aicbsp_info.hwinfo < 0;
			*(data + 3)  = aicbsp_info.hwinfo;
			*(data + 5)  = aicbsp_info.cpmode;

			*(data + 7)  = aicbt_info.btmode;
			*(data + 9)  = aicbt_info.btport;
			*(data + 11) = aicbt_info.uart_baud;
			*(data + 13) = aicbt_info.uart_flowctrl;
		}

		if (AICBT_PT_VER == p->type) {
			printk("aicbsp: bt patch version: %s\n", (char *)p->data);
			continue;
		}

		for (i = 0; i < p->len; i++) {
			ret = rwnx_send_dbg_mem_write_req(sdiodev, *data, *(data + 1));
			if (ret != 0)
				return ret;
			data += 2;
		}
		if (p->type == AICBT_PT_PWRON)
			udelay(500);
	}
	aicbt_patch_table_free(&head);
	return 0;
}

int aicbt_init(struct aic_sdio_dev *sdiodev)
{
	if (aicbt_patch_trap_data_load(sdiodev)) {
		printk("aicbt_patch_trap_data_load fail\n");
		return -1;
	}

	if (aicbt_patch_table_load(sdiodev)) {
		 printk("aicbt_patch_table_load fail\n");
		return -1;
	}

	return 0;
}

static int aicwifi_start_from_bootrom(struct aic_sdio_dev *sdiodev)
{
	int ret = 0;

	/* memory access */
	const u32 fw_addr = RAM_FMAC_FW_ADDR;
	struct dbg_start_app_cfm start_app_cfm;

	/* fw start */
	ret = rwnx_send_dbg_start_app_req(sdiodev, fw_addr, HOST_START_APP_AUTO, &start_app_cfm);
	if (ret) {
		return -1;
	}
	aicbsp_info.hwinfo_r = start_app_cfm.bootstatus & 0xFF;

	return 0;
}

u32 patch_tbl[][2] = {
};

u32 syscfg_tbl_masked[][3] = {
	{0x40506024, 0x000000FF, 0x000000DF}, // for clk gate lp_level
};

u32 rf_tbl_masked[][3] = {
	{0x40344058, 0x00800000, 0x00000000},// pll trx
};

static int aicwifi_sys_config(struct aic_sdio_dev *sdiodev)
{
	int ret, cnt;
	int syscfg_num = sizeof(syscfg_tbl_masked) / sizeof(u32) / 3;
	for (cnt = 0; cnt < syscfg_num; cnt++) {
		ret = rwnx_send_dbg_mem_mask_write_req(sdiodev,
			syscfg_tbl_masked[cnt][0], syscfg_tbl_masked[cnt][1], syscfg_tbl_masked[cnt][2]);
		if (ret) {
			printk("%x mask write fail: %d\n", syscfg_tbl_masked[cnt][0], ret);
			return ret;
		}
	}

	ret = rwnx_send_dbg_mem_mask_write_req(sdiodev,
				rf_tbl_masked[0][0], rf_tbl_masked[0][1], rf_tbl_masked[0][2]);
	if (ret) {
		printk("rf config %x write fail: %d\n", rf_tbl_masked[0][0], ret);
		return ret;
	}

	return 0;
}

static int aicwifi_patch_config(struct aic_sdio_dev *sdiodev)
{
	const u32 rd_patch_addr = RAM_FMAC_FW_ADDR + 0x0180;
	u32 config_base;
	uint32_t start_addr = 0x1e6000;
	u32 patch_addr = start_addr;
	u32 patch_num = sizeof(patch_tbl)/4;
	struct dbg_mem_read_cfm rd_patch_addr_cfm;
	int ret = 0;
	u16 cnt = 0;
	u32 patch_addr_reg = 0x1e4d80;
	u32 patch_num_reg = 0x1e4d84;

	if (aicbsp_info.cpmode == AICBSP_CPMODE_TEST) {
		patch_addr_reg = 0x1e4d74;
		patch_num_reg = 0x1e4d78;
	}

	ret = rwnx_send_dbg_mem_read_req(sdiodev, rd_patch_addr, &rd_patch_addr_cfm);
	if (ret) {
		printk("patch rd fail\n");
		return ret;
	}

	config_base = rd_patch_addr_cfm.memdata;

	ret = rwnx_send_dbg_mem_write_req(sdiodev, patch_addr_reg, patch_addr);
	if (ret) {
		printk("0x%x write fail\n", patch_addr_reg);
		return ret;
	}

	ret = rwnx_send_dbg_mem_write_req(sdiodev, patch_num_reg, patch_num);
	if (ret) {
		printk("0x%x write fail\n", patch_num_reg);
		return ret;
	}

	for (cnt = 0; cnt < patch_num/2; cnt += 1) {
		ret = rwnx_send_dbg_mem_write_req(sdiodev, start_addr+8*cnt, patch_tbl[cnt][0]+config_base);
		if (ret) {
			printk("%x write fail\n", start_addr+8*cnt);
			return ret;
		}

		ret = rwnx_send_dbg_mem_write_req(sdiodev, start_addr+8*cnt+4, patch_tbl[cnt][1]);
		if (ret) {
			printk("%x write fail\n", start_addr+8*cnt+4);
			return ret;
		}
	}

	return 0;
}

int aicwifi_init(struct aic_sdio_dev *sdiodev)
{
	if (rwnx_plat_bin_fw_upload_android(sdiodev, RAM_FMAC_FW_ADDR, aicbsp_firmware_list[aicbsp_info.cpmode].wl_fw)) {
		printk("download wifi fw fail\n");
		return -1;
	}

	if (aicwifi_patch_config(sdiodev)) {
		printk("aicwifi_patch_config fail\n");
		return -1;
	}

	if (aicwifi_sys_config(sdiodev)) {
		printk("aicwifi_sys_config fail\n");
		return -1;
	}

	if (aicwifi_start_from_bootrom(sdiodev)) {
		printk("wifi start fail\n");
		return -1;
	}

	return 0;
}

u32 aicbsp_syscfg_tbl[][2] = {
	{0x40500014, 0x00000101}, // 1)
	{0x40500018, 0x00000109}, // 2)
	{0x40500004, 0x00000010}, // 3) the order should not be changed

	// def CONFIG_PMIC_SETTING
	// U02 bootrom only
	{0x40040000, 0x00001AC8}, // 1) fix panic
	{0x40040084, 0x00011580},
	{0x40040080, 0x00000001},
	{0x40100058, 0x00000000},

	{0x50000000, 0x03220204}, // 2) pmic interface init
	{0x50019150, 0x00000002}, // 3) for 26m xtal, set div1
	{0x50017008, 0x00000000}, // 4) stop wdg
};

static int aicbsp_system_config(struct aic_sdio_dev *sdiodev)
{
	int syscfg_num = sizeof(aicbsp_syscfg_tbl) / sizeof(u32) / 2;
	int ret, cnt;
	for (cnt = 0; cnt < syscfg_num; cnt++) {
		ret = rwnx_send_dbg_mem_write_req(sdiodev, aicbsp_syscfg_tbl[cnt][0], aicbsp_syscfg_tbl[cnt][1]);
		if (ret) {
			sdio_err("%x write fail: %d\n", aicbsp_syscfg_tbl[cnt][0], ret);
			return ret;
		}
	}
	return 0;
}

int aicbsp_platform_init(struct aic_sdio_dev *sdiodev)
{
	rwnx_cmd_mgr_init(&sdiodev->cmd_mgr);
	sdiodev->cmd_mgr.sdiodev = (void *)sdiodev;
	return 0;
}

void aicbsp_platform_deinit(struct aic_sdio_dev *sdiodev)
{
	(void)sdiodev;
}

int aicbsp_driver_fw_init(struct aic_sdio_dev *sdiodev)
{
	const u32 mem_addr = 0x40500000;
	struct dbg_mem_read_cfm rd_mem_addr_cfm;

	uint8_t binding_status;
	uint8_t dout[16];

	need_binding_verify = false;

	if (rwnx_send_dbg_mem_read_req(sdiodev, mem_addr, &rd_mem_addr_cfm))
		return -1;

	aicbsp_info.chip_rev = (u8)(rd_mem_addr_cfm.memdata >> 16);
	if (aicbsp_info.chip_rev != CHIP_REV_U02 &&
		aicbsp_info.chip_rev != CHIP_REV_U03 &&
		aicbsp_info.chip_rev != CHIP_REV_U04) {
		pr_err("aicbsp: %s, unsupport chip rev: %d\n", __func__, aicbsp_info.chip_rev);
		return -1;
	}

	printk("aicbsp: %s, chip rev: %d\n", __func__, aicbsp_info.chip_rev);

	if (aicbsp_info.chip_rev != CHIP_REV_U02)
		aicbsp_firmware_list = fw_u03;

	if (aicbsp_system_config(sdiodev))
		return -1;

	if (aicbt_init(sdiodev))
		return -1;

	if (aicwifi_init(sdiodev))
		return -1;

	if (need_binding_verify) {
		printk("aicbsp: crypto data %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
						binding_enc_data[0],  binding_enc_data[1],  binding_enc_data[2],  binding_enc_data[3],
						binding_enc_data[4],  binding_enc_data[5],  binding_enc_data[6],  binding_enc_data[7],
						binding_enc_data[8],  binding_enc_data[9],  binding_enc_data[10], binding_enc_data[11],
						binding_enc_data[12], binding_enc_data[13], binding_enc_data[14], binding_enc_data[15]);

		/* calculate verify data from crypto data */
		if (wcn_bind_verify_calculate_verify_data(binding_enc_data, dout)) {
			pr_err("aicbsp: %s, binding encrypt data incorrect\n", __func__);
			return -1;
		}

		printk("aicbsp: verify data %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",
						dout[0],  dout[1],  dout[2],  dout[3],
						dout[4],  dout[5],  dout[6],  dout[7],
						dout[8],  dout[9],  dout[10], dout[11],
						dout[12], dout[13], dout[14], dout[15]);

		if (rwnx_send_dbg_binding_req(sdiodev, dout, &binding_status)) {
			pr_err("aicbsp: %s, send binding request failn", __func__);
			return -1;
		}

		if (binding_status) {
			pr_err("aicbsp: %s, binding verify fail\n", __func__);
			return -1;
		}
	}

	if (aicwf_sdio_writeb(sdiodev, SDIOWIFI_WAKEUP_REG, 4)) {
		sdio_err("reg:%d write failed!\n", SDIOWIFI_WAKEUP_REG);
		return -1;
	}

	return 0;
}

int aicbsp_get_feature(struct aicbsp_feature_t *feature)
{
	feature->sdio_clock = FEATURE_SDIO_CLOCK;
	feature->sdio_phase = FEATURE_SDIO_PHASE;
	feature->hwinfo     = aicbsp_info.hwinfo;
	feature->fwlog_en   = aicbsp_info.fwlog_en;
	return 0;
}
EXPORT_SYMBOL_GPL(aicbsp_get_feature);
