/*
 * btif_woble.c
 *
 *  Created on: 2020
 *      Author: Unisoc
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/tty.h>
#include <linux/vt_kern.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <linux/vmalloc.h>

#include <linux/file.h>
#include <linux/string.h>
#include "woble.h"
#include "tty.h"

#define MAX_WAKE_DEVICE_MAX_NUM 36
#define CONFIG_FILE_PATH "/data/misc/bluedroid/bt_config.conf"

static struct hci_cmd_t hci_cmd;
uint8_t device_count_db;
const woble_config_t s_woble_config_cust = {WOBLE_MOD_ENABLE, WOBLE_SLEEP_MOD_COULD_NOT_KNOW, 0, WOBLE_SLEEP_MOD_NOT_NEED_NOTITY};

mtty_bt_wake_t bt_wake_dev_db[MAX_WAKE_DEVICE_MAX_NUM];

int woble_init(void)
{
	memset(&hci_cmd, 0, sizeof(struct hci_cmd_t));
	sema_init(&hci_cmd.wait, 0);

	return 0;
}

int woble_data_recv(const unsigned char *buf, int count)
{
	unsigned short opcode = 0;
	unsigned char status = 0;
	const unsigned char *p = 0;
	const unsigned char *rxmsg = buf;
	int left_length = count;
	int pack_length = 0;
	int last = 0;
	int ret = -1;

	if (count < 0) {
		pr_err("%s count < 0!!!", __func__);
	}

	do {
		rxmsg = buf + (count - left_length);
		switch (rxmsg[PACKET_TYPE]) {
		case HCI_EVT:
			if (left_length < 3) {
				pr_err("%s left_length <3 !!!!!", __func__);
			}
			pack_length = rxmsg[EVT_HEADER_SIZE];
			pack_length += 3;

			if (left_length - pack_length < 0) {
				pr_err("%s left_length - pack_length <0 !!!!!", __func__);

			}
			switch (rxmsg[EVT_HEADER_EVENT]) {
			default:
			case BT_HCI_EVT_CMD_COMPLETE:
				p = rxmsg + 4;
				STREAM_TO_UINT16(opcode, p);
				STREAM_TO_UINT8(status, p);
				break;

			case BT_HCI_EVT_CMD_STATUS:
				p = rxmsg + 5;
				STREAM_TO_UINT16(opcode, p);
				status = rxmsg[3];
				break;
			}
			last = left_length;
			left_length -= pack_length;
			break;
		default:
			left_length = 0;
			break;
		}
	} while (left_length);

	if (hci_cmd.opcode == opcode && hci_cmd.opcode) {
		pr_info("%s opcode: 0x%04X, status: %d\n", __func__, opcode, status);
		up(&hci_cmd.wait);
		ret = 0;
	}
	return ret;
}

int hci_cmd_send_sync(unsigned short opcode, struct HC_BT_HDR *py,
		struct HC_BT_HDR *rsp)
{
	unsigned char msg_req[HCI_CMD_MAX_LEN], *p;
	int ret = 0;

	p = msg_req;
	UINT8_TO_STREAM(p, HCI_CMD);
	UINT16_TO_STREAM(p, opcode);

	if (py == NULL) {
		UINT8_TO_STREAM(p, 0);
	} else {
		UINT8_TO_STREAM(p, py->len);
		ARRAY_TO_STREAM(p, py->data, py->len);
	}

	hci_cmd.opcode = opcode;
	ret = marlin_sdio_write(msg_req, p - msg_req);
	if (!ret) {
		hci_cmd.opcode = 0;
		pr_err("%s marlin_sdio_write fail", __func__);
		return 0;
	}
	down(&hci_cmd.wait);
	hci_cmd.opcode = 0;

	return 0;
}

void hci_set_ap_sleep_mode(int is_shutdown, int is_resume)
{
	struct HC_BT_HDR *payload = (struct HC_BT_HDR *)vmalloc(sizeof(struct HC_BT_HDR) + 3);
	unsigned char *p;
	p = payload->data;

	payload->len = 6;
	if (is_resume && !is_shutdown) {
		UINT8_TO_STREAM(p, 0);
	} else {
		UINT8_TO_STREAM(p, s_woble_config_cust.woble_mod);
	}

	if (is_shutdown) {
		UINT8_TO_STREAM(p, WOBLE_SLEEP_MOD_COULD_KNOW);
	} else {
		UINT8_TO_STREAM(p, WOBLE_SLEEP_MOD_COULD_NOT_KNOW);
	}

	UINT16_TO_STREAM(p, s_woble_config_cust.timeout);
	UINT8_TO_STREAM(p, s_woble_config_cust.notify);
	UINT8_TO_STREAM(p, 0);

	hci_cmd_send_sync(BT_HCI_OP_SET_SLEEPMODE, payload, NULL);
	vfree(payload);
	return;
}

void hci_set_ap_start_sleep(void)
{
	struct HC_BT_HDR *payload = (struct HC_BT_HDR *)vmalloc(sizeof(struct HC_BT_HDR) + 3);

	hci_cmd_send_sync(BT_HCI_OP_SET_STARTSLEEP, NULL, NULL);
	vfree(payload);
	return;
}
