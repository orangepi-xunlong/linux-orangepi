/*
 *
 *  AicSemi Bluetooth USB driver
 *
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/skbuff.h>
#include <linux/usb.h>

#include <linux/ioctl.h>
#include <linux/io.h>
#include <linux/firmware.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/reboot.h>

#include "aic_btusb.h"

#define AICBT_RELEASE_NAME "202012_ANDROID"
#define VERSION            "2.1.0"
#define DRV_RELEASE_DATE   "20240919"
#define DRV_PATCH_LEVEL    "002"
#define DRV_RELEASE_TAG    "aic-btusb-" DRV_RELEASE_DATE "-" DRV_PATCH_LEVEL

#define SUSPNED_DW_FW 0


static spinlock_t queue_lock;
static spinlock_t dlfw_lock;
static volatile uint16_t    dlfw_dis_state;
static uint32_t usb_info;

struct btusb_data {
	struct hci_dev       *hdev;
	struct usb_device    *udev;
	struct usb_interface *intf;
	struct usb_interface *isoc;

	spinlock_t lock;

	unsigned long flags;

	struct work_struct work;
	struct work_struct waker;

	struct usb_anchor tx_anchor;
	struct usb_anchor intr_anchor;
	struct usb_anchor bulk_anchor;
	struct usb_anchor isoc_anchor;
	struct usb_anchor deferred;
	int tx_in_flight;
	spinlock_t txlock;

	struct usb_endpoint_descriptor *intr_ep;
	struct usb_endpoint_descriptor *bulk_tx_ep;
	struct usb_endpoint_descriptor *bulk_rx_ep;
	struct usb_endpoint_descriptor *isoc_tx_ep;
	struct usb_endpoint_descriptor *isoc_rx_ep;

	__u8 cmdreq_type;

	unsigned int sco_num;
	int isoc_altsetting;
	int suspend_count;
	uint16_t sco_handle;
//#ifdef CONFIG_HAS_EARLYSUSPEND
#if 0
	struct early_suspend early_suspend;
#else
	struct notifier_block pm_notifier;
	struct notifier_block reboot_notifier;
#endif
	firmware_info *fw_info;
};
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 1)
static bool reset_on_close;
#endif


static inline int check_set_dlfw_state_value(uint16_t change_value)
{
	spin_lock(&dlfw_lock);
	if (!dlfw_dis_state) {
		dlfw_dis_state = change_value;
	}
	spin_unlock(&dlfw_lock);
	return dlfw_dis_state;
}

static inline void set_dlfw_state_value(uint16_t change_value)
{
	spin_lock(&dlfw_lock);
	dlfw_dis_state = change_value;
	spin_unlock(&dlfw_lock);
}




static void aic_free(struct btusb_data *data)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 1)
	kfree(data);
#endif
	return;
}

static struct btusb_data *aic_alloc(struct usb_interface *intf)
{
	struct btusb_data *data;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 7, 1)
	data = kzalloc(sizeof(*data), GFP_KERNEL);
#else
	data = devm_kzalloc(&intf->dev, sizeof(*data), GFP_KERNEL);
#endif
	return data;
}

static void print_acl(struct sk_buff *skb, int direction)
{
#if PRINT_ACL_DATA
	uint wlength = skb->len;
	u16 *handle = (u16 *)(skb->data);
	u16 len = *(handle+1);
	u8 *acl_data = (u8 *)(skb->data);

	AICBT_INFO("aic %s: direction %d, handle %04x, len %d",
			__func__, direction, *handle, len);
#endif
}

static void print_sco(struct sk_buff *skb, int direction)
{
#if PRINT_SCO_DATA
	uint wlength = skb->len;
	u16 *handle = (u16 *)(skb->data);
	u8 len = *(u8 *)(handle+1);
	u8 *sco_data = (u8 *)(skb->data);

	AICBT_INFO("aic %s: direction %d, handle %04x, len %d",
			__func__, direction, *handle, len);
#endif
}

static void print_error_command(struct sk_buff *skb)
{
	u16 *opcode = (u16 *)(skb->data);
	u8 *cmd_data = (u8 *)(skb->data);
	u8 len = *(cmd_data + 2);

	switch (*opcode) {
	case HCI_OP_INQUIRY:
		printk("HCI_OP_INQUIRY");
		break;
	case HCI_OP_INQUIRY_CANCEL:
		printk("HCI_OP_INQUIRY_CANCEL");
		break;
	case HCI_OP_EXIT_PERIODIC_INQ:
		printk("HCI_OP_EXIT_PERIODIC_INQ");
		break;
	case HCI_OP_CREATE_CONN:
		printk("HCI_OP_CREATE_CONN");
		break;
	case HCI_OP_DISCONNECT:
		printk("HCI_OP_DISCONNECT");
		break;
	case HCI_OP_CREATE_CONN_CANCEL:
		printk("HCI_OP_CREATE_CONN_CANCEL");
		break;
	case HCI_OP_ACCEPT_CONN_REQ:
		printk("HCI_OP_ACCEPT_CONN_REQ");
		break;
	case HCI_OP_REJECT_CONN_REQ:
		printk("HCI_OP_REJECT_CONN_REQ");
		break;
	case HCI_OP_AUTH_REQUESTED:
		printk("HCI_OP_AUTH_REQUESTED");
		break;
	case HCI_OP_SET_CONN_ENCRYPT:
		printk("HCI_OP_SET_CONN_ENCRYPT");
		break;
	case HCI_OP_REMOTE_NAME_REQ:
		printk("HCI_OP_REMOTE_NAME_REQ");
		break;
	case HCI_OP_READ_REMOTE_FEATURES:
		printk("HCI_OP_READ_REMOTE_FEATURES");
		break;
	case HCI_OP_SNIFF_MODE:
		printk("HCI_OP_SNIFF_MODE");
		break;
	case HCI_OP_EXIT_SNIFF_MODE:
		printk("HCI_OP_EXIT_SNIFF_MODE");
		break;
	case HCI_OP_SWITCH_ROLE:
		printk("HCI_OP_SWITCH_ROLE");
		break;
	case HCI_OP_SNIFF_SUBRATE:
		printk("HCI_OP_SNIFF_SUBRATE");
		break;
	case HCI_OP_RESET:
		printk("HCI_OP_RESET");
		break;
	case HCI_OP_Write_Extended_Inquiry_Response:
		printk("HCI_Write_Extended_Inquiry_Response");
		break;

	case HCI_OP_Write_Simple_Pairing_Mode:
		printk("HCI_OP_Write_Simple_Pairing_Mode");
		break;
	case HCI_OP_Read_Buffer_Size:
		printk("HCI_OP_Read_Buffer_Size");
		break;
	case HCI_OP_Host_Buffer_Size:
		printk("HCI_OP_Host_Buffer_Size");
		break;
	case HCI_OP_Read_Local_Version_Information:
		printk("HCI_OP_Read_Local_Version_Information");
		break;
	case HCI_OP_Read_BD_ADDR:
		printk("HCI_OP_Read_BD_ADDR");
		break;
	case HCI_OP_Read_Local_Supported_Commands:
		printk("HCI_OP_Read_Local_Supported_Commands");
		break;
	case HCI_OP_Write_Scan_Enable:
		printk("HCI_OP_Write_Scan_Enable");
		break;
	case HCI_OP_Write_Current_IAC_LAP:
		printk("HCI_OP_Write_Current_IAC_LAP");
		break;
	case HCI_OP_Write_Inquiry_Scan_Activity:
		printk("HCI_OP_Write_Inquiry_Scan_Activity");
		break;
	case HCI_OP_Write_Class_of_Device:
		printk("HCI_OP_Write_Class_of_Device");
		break;
	case HCI_OP_LE_Rand:
		printk("HCI_OP_LE_Rand");
		break;
	case HCI_OP_LE_Set_Random_Address:
		printk("HCI_OP_LE_Set_Random_Address");
		break;
	case HCI_OP_LE_Set_Extended_Scan_Enable:
		printk("HCI_OP_LE_Set_Extended_Scan_Enable");
		break;
	case HCI_OP_LE_Set_Extended_Scan_Parameters:
		printk("HCI_OP_LE_Set_Extended_Scan_Parameters");
		break;
	case HCI_OP_Set_Event_Filter:
		printk("HCI_OP_Set_Event_Filter");
		break;
	case HCI_OP_Write_Voice_Setting:
		printk("HCI_OP_Write_Voice_Setting");
		break;
	case HCI_OP_Change_Local_Name:
		printk("HCI_OP_Change_Local_Name");
		break;
	case HCI_OP_Read_Local_Name:
		printk("HCI_OP_Read_Local_Name");
		break;
	case HCI_OP_Wirte_Page_Timeout:
		printk("HCI_OP_Wirte_Page_Timeout");
		break;
	case HCI_OP_LE_Clear_Resolving_List:
		printk("HCI_OP_LE_Clear_Resolving_List");
		break;
	case HCI_OP_LE_Set_Addres_Resolution_Enable_Command:
		printk("HCI_OP_LE_Set_Addres_Resolution_Enable_Command");
		break;
	case HCI_OP_Write_Inquiry_mode:
		printk("HCI_OP_Write_Inquiry_mode");
		break;
	case HCI_OP_Write_Page_Scan_Type:
		printk("HCI_OP_Write_Page_Scan_Type");
		break;
	case HCI_OP_Write_Inquiry_Scan_Type:
		printk("HCI_OP_Write_Inquiry_Scan_Type");
		break;
	case HCI_OP_Delete_Stored_Link_Key:
		printk("HCI_OP_Delete_Stored_Link_Key");
		break;
	case HCI_OP_LE_Read_Local_Resolvable_Address:
		printk("HCI_OP_LE_Read_Local_Resolvable_Address");
		break;
	case HCI_OP_LE_Extended_Create_Connection:
		printk("HCI_OP_LE_Extended_Create_Connection");
		break;
	case HCI_OP_Read_Remote_Version_Information:
		printk("HCI_OP_Read_Remote_Version_Information");
		break;
	case HCI_OP_LE_Start_Encryption:
		printk("HCI_OP_LE_Start_Encryption");
		break;
	case HCI_OP_LE_Add_Device_to_Resolving_List:
		printk("HCI_OP_LE_Add_Device_to_Resolving_List");
		break;
	case HCI_OP_LE_Set_Privacy_Mode:
		printk("HCI_OP_LE_Set_Privacy_Mode");
		break;
	case HCI_OP_LE_Connection_Update:
		printk("HCI_OP_LE_Connection_Update");
		break;
	default:
		printk("UNKNOW_HCI_COMMAND");
		break;
	}
	printk(" 0x%04x,len:%d,", *opcode, len);
}

static void print_command(struct sk_buff *skb)
{
#if PRINT_CMD_EVENT
	print_error_command(skb);
#endif
}

#if CONFIG_BLUEDROID
/* Global parameters for bt usb char driver */
#define BT_CHAR_DEVICE_NAME "aicbt_dev"
struct mutex btchr_mutex;
static struct sk_buff_head btchr_readq;
static wait_queue_head_t btchr_read_wait;
static wait_queue_head_t bt_dlfw_wait;
static int bt_char_dev_registered;
static dev_t bt_devid; /* bt char device number */
static struct cdev bt_char_dev; /* bt character device structure */
static struct class *bt_char_class; /* device class for usb char driver */
static int bt_reset;
/* HCI device & lock */
DEFINE_RWLOCK(hci_dev_lock);
struct hci_dev *ghdev;

static void print_event(struct sk_buff *skb)
{
#if PRINT_CMD_EVENT
	uint wlength = skb->len;
	uint icount = 0;
	u8 *opcode = (u8 *)(skb->data);
	u8 len = *(opcode + 1);

	printk("aic %s ", __func__);
	switch (*opcode) {
	case HCI_EV_INQUIRY_COMPLETE:
		printk("HCI_EV_INQUIRY_COMPLETE");
		break;
	case HCI_EV_INQUIRY_RESULT:
		printk("HCI_EV_INQUIRY_RESULT");
		break;
	case HCI_EV_CONN_COMPLETE:
		printk("HCI_EV_CONN_COMPLETE");
		break;
	case HCI_EV_CONN_REQUEST:
		printk("HCI_EV_CONN_REQUEST");
		break;
	case HCI_EV_DISCONN_COMPLETE:
		printk("HCI_EV_DISCONN_COMPLETE");
		break;
	case HCI_EV_AUTH_COMPLETE:
		printk("HCI_EV_AUTH_COMPLETE");
		break;
	case HCI_EV_REMOTE_NAME:
		printk("HCI_EV_REMOTE_NAME");
		break;
	case HCI_EV_ENCRYPT_CHANGE:
		printk("HCI_EV_ENCRYPT_CHANGE");
		break;
	case HCI_EV_CHANGE_LINK_KEY_COMPLETE:
		printk("HCI_EV_CHANGE_LINK_KEY_COMPLETE");
		break;
	case HCI_EV_REMOTE_FEATURES:
		printk("HCI_EV_REMOTE_FEATURES");
		break;
	case HCI_EV_REMOTE_VERSION:
		printk("HCI_EV_REMOTE_VERSION");
		break;
	case HCI_EV_QOS_SETUP_COMPLETE:
		printk("HCI_EV_QOS_SETUP_COMPLETE");
		break;
	case HCI_EV_CMD_COMPLETE:
		printk("HCI_EV_CMD_COMPLETE");
		break;
	case HCI_EV_CMD_STATUS:
		printk("HCI_EV_CMD_STATUS");
		break;
	case HCI_EV_ROLE_CHANGE:
		printk("HCI_EV_ROLE_CHANGE");
		break;
	case HCI_EV_NUM_COMP_PKTS:
		printk("HCI_EV_NUM_COMP_PKTS");
		break;
	case HCI_EV_MODE_CHANGE:
		printk("HCI_EV_MODE_CHANGE");
		break;
	case HCI_EV_PIN_CODE_REQ:
		printk("HCI_EV_PIN_CODE_REQ");
		break;
	case HCI_EV_LINK_KEY_REQ:
		printk("HCI_EV_LINK_KEY_REQ");
		break;
	case HCI_EV_LINK_KEY_NOTIFY:
		printk("HCI_EV_LINK_KEY_NOTIFY");
		break;
	case HCI_EV_CLOCK_OFFSET:
		printk("HCI_EV_CLOCK_OFFSET");
		break;
	case HCI_EV_PKT_TYPE_CHANGE:
		printk("HCI_EV_PKT_TYPE_CHANGE");
		break;
	case HCI_EV_PSCAN_REP_MODE:
		printk("HCI_EV_PSCAN_REP_MODE");
		break;
	case HCI_EV_INQUIRY_RESULT_WITH_RSSI:
		printk("HCI_EV_INQUIRY_RESULT_WITH_RSSI");
		break;
	case HCI_EV_REMOTE_EXT_FEATURES:
		printk("HCI_EV_REMOTE_EXT_FEATURES");
		break;
	case HCI_EV_SYNC_CONN_COMPLETE:
		printk("HCI_EV_SYNC_CONN_COMPLETE");
		break;
	case HCI_EV_SYNC_CONN_CHANGED:
		printk("HCI_EV_SYNC_CONN_CHANGED");
		break;
	case HCI_EV_SNIFF_SUBRATE:
		printk("HCI_EV_SNIFF_SUBRATE");
		break;
	case HCI_EV_EXTENDED_INQUIRY_RESULT:
		printk("HCI_EV_EXTENDED_INQUIRY_RESULT");
		break;
	case HCI_EV_IO_CAPA_REQUEST:
		printk("HCI_EV_IO_CAPA_REQUEST");
		break;
	case HCI_EV_SIMPLE_PAIR_COMPLETE:
		printk("HCI_EV_SIMPLE_PAIR_COMPLETE");
		break;
	case HCI_EV_REMOTE_HOST_FEATURES:
		printk("HCI_EV_REMOTE_HOST_FEATURES");
		break;
	default:
		printk("event");
		break;
	}
	printk(":%02x,len:%d,", *opcode, len);
	for (icount = 2; (icount < wlength) && (icount < 24); icount++)
		printk("%02x ", *(opcode+icount));
	printk("\n");
#endif
}

static inline ssize_t usb_put_user(struct sk_buff *skb,
		char __user *buf, int count)
{
	char __user *ptr = buf;
	int len = min_t(unsigned int, skb->len, count);

	if (copy_to_user(ptr, skb->data, len))
		return -EFAULT;

	return len;
}

static struct sk_buff *aic_skb_queue[QUEUE_SIZE];
static int aic_skb_queue_front;
static int aic_skb_queue_rear;

static void aic_enqueue(struct sk_buff *skb)
{
	spin_lock(&queue_lock);
	if (aic_skb_queue_front == (aic_skb_queue_rear + 1) % QUEUE_SIZE) {
		/*
		 * If queue is full, current solution is to drop
		 * the following entries.
		 */
		AICBT_WARN("%s: Queue is full, entry will be dropped", __func__);
	} else {
		aic_skb_queue[aic_skb_queue_rear] = skb;

		aic_skb_queue_rear++;
		aic_skb_queue_rear %= QUEUE_SIZE;

	}
	spin_unlock(&queue_lock);
}

static struct sk_buff *aic_dequeue_try(unsigned int deq_len)
{
	struct sk_buff *skb;
	struct sk_buff *skb_copy;

	if (aic_skb_queue_front == aic_skb_queue_rear) {
		AICBT_WARN("%s: Queue is empty", __func__);
		return NULL;
	}

	skb = aic_skb_queue[aic_skb_queue_front];
	if (deq_len >= skb->len) {

		aic_skb_queue_front++;
		aic_skb_queue_front %= QUEUE_SIZE;

		/*
		 * Return skb addr to be dequeued, and the caller
		 * should free the skb eventually.
		 */
		return skb;
	} else {
		skb_copy = pskb_copy(skb, GFP_ATOMIC);
		skb_pull(skb, deq_len);
		/* Return its copy to be freed */
		return skb_copy;
	}
}

static inline int is_queue_empty(void)
{
	return (aic_skb_queue_front == aic_skb_queue_rear) ? 1 : 0;
}

/*
 * AicSemi - Integrate from hci_core.c
 */

/* Get HCI device by index.
 * Device is held on return. */
static struct hci_dev *hci_dev_get(int index)
{
	if (index != 0)
		return NULL;

	return ghdev;
}

/* ---- HCI ioctl helpers ---- */
static int hci_dev_open(__u16 dev)
{
	struct hci_dev *hdev;
	int ret = 0;

	AICBT_DBG("%s: dev %d", __func__, dev);

	hdev = hci_dev_get(dev);
	if (!hdev) {
		AICBT_ERR("%s: Failed to get hci dev[Null]", __func__);
		return -ENODEV;
	}

	if (test_bit(HCI_UNREGISTER, &hdev->dev_flags)) {
		ret = -ENODEV;
		goto done;
	}

	if (test_bit(HCI_UP, &hdev->flags)) {
		ret = -EALREADY;
		goto done;
	}

done:
	return ret;
}

static int hci_dev_do_close(struct hci_dev *hdev)
{
	if (hdev->flush)
		hdev->flush(hdev);
	/* After this point our queues are empty
	 * and no tasks are scheduled. */
	hdev->close(hdev);
	/* Clear flags */
	hdev->flags = 0;
	return 0;
}

static int hci_dev_close(__u16 dev)
{
	struct hci_dev *hdev;
	int err;
	hdev = hci_dev_get(dev);
	if (!hdev) {
		AICBT_ERR("%s: failed to get hci dev[Null]", __func__);
		return -ENODEV;
	}

	err = hci_dev_do_close(hdev);

	return err;
}

static struct hci_dev *hci_alloc_dev(void)
{
	struct hci_dev *hdev;

	hdev = kzalloc(sizeof(struct hci_dev), GFP_KERNEL);
	if (!hdev)
		return NULL;

	return hdev;
}

/* Free HCI device */
static void hci_free_dev(struct hci_dev *hdev)
{
	kfree(hdev);
}

/* Register HCI device */
static int hci_register_dev(struct hci_dev *hdev)
{
	int i, id;

	AICBT_DBG("%s: %p name %s bus %d", __func__, hdev, hdev->name, hdev->bus);
	/* Do not allow HCI_AMP devices to register at index 0,
	 * so the index can be used as the AMP controller ID.
	 */
	id = (hdev->dev_type == HCI_BREDR) ? 0 : 1;

	write_lock(&hci_dev_lock);

	sprintf(hdev->name, "hci%d", id);
	hdev->id = id;
	hdev->flags = 0;
	hdev->dev_flags = 0;
	mutex_init(&hdev->lock);

	AICBT_DBG("%s: id %d, name %s", __func__, hdev->id, hdev->name);


	for (i = 0; i < NUM_REASSEMBLY; i++)
		hdev->reassembly[i] = NULL;

	memset(&hdev->stat, 0, sizeof(struct hci_dev_stats));
	atomic_set(&hdev->promisc, 0);

	if (ghdev) {
		write_unlock(&hci_dev_lock);
		AICBT_ERR("%s: Hci device has been registered already", __func__);
		return -1;
	} else
		ghdev = hdev;

	write_unlock(&hci_dev_lock);

	return id;
}

/* Unregister HCI device */
static void hci_unregister_dev(struct hci_dev *hdev)
{
	int i;

	AICBT_DBG("%s: hdev %p name %s bus %d", __func__, hdev, hdev->name, hdev->bus);
	set_bit(HCI_UNREGISTER, &hdev->dev_flags);

	write_lock(&hci_dev_lock);
	ghdev = NULL;
	write_unlock(&hci_dev_lock);

	hci_dev_do_close(hdev);
	for (i = 0; i < NUM_REASSEMBLY; i++)
		kfree_skb(hdev->reassembly[i]);
}

static void hci_send_to_stack(struct hci_dev *hdev, struct sk_buff *skb)
{
	struct sk_buff *aic_skb_copy = NULL;

	if (!hdev) {
		AICBT_ERR("%s: Frame for unknown HCI device", __func__);
		return;
	}

	if (!test_bit(HCI_RUNNING, &hdev->flags)) {
		AICBT_ERR("%s: HCI not running", __func__);
		return;
	}

	aic_skb_copy = pskb_copy(skb, GFP_ATOMIC);
	if (!aic_skb_copy) {
		AICBT_ERR("%s: Copy skb error", __func__);
		return;
	}

	memcpy(skb_push(aic_skb_copy, 1), &bt_cb(skb)->pkt_type, 1);
	aic_enqueue(aic_skb_copy);

	/* Make sure bt char device existing before wakeup read queue */
	hdev = hci_dev_get(0);
	if (hdev) {
		AICBT_DBG("%s", __func__);
		wake_up_interruptible(&btchr_read_wait);
	}

	return;
}

/* Receive frame from HCI drivers */
static int hci_recv_frame(struct sk_buff *skb)
{
	struct hci_dev *hdev = (struct hci_dev *) skb->dev;

	if (!hdev ||
		(!test_bit(HCI_UP, &hdev->flags) && !test_bit(HCI_INIT, &hdev->flags))) {
		kfree_skb(skb);
		return -ENXIO;
	}

	/* Incomming skb */
	bt_cb(skb)->incoming = 1;

	/* Time stamp */
	__net_timestamp(skb);

	if (atomic_read(&hdev->promisc)) {
		/* Send copy to the sockets */
		hci_send_to_stack(hdev, skb);
	}
	kfree_skb(skb);
	return 0;
}

static int hci_reassembly(struct hci_dev *hdev, int type, void *data,
						  int count, __u8 index)
{
	int len = 0;
	int hlen = 0;
	int remain = count;
	struct sk_buff *skb;
	struct bt_skb_cb *scb;

	if ((type < HCI_ACLDATA_PKT || type > HCI_EVENT_PKT) ||
			index >= NUM_REASSEMBLY)
		return -EILSEQ;

	skb = hdev->reassembly[index];

	if (!skb) {
		switch (type) {
		case HCI_ACLDATA_PKT:
			len = HCI_MAX_FRAME_SIZE;
			hlen = HCI_ACL_HDR_SIZE;
			break;
		case HCI_EVENT_PKT:
			len = HCI_MAX_EVENT_SIZE;
			hlen = HCI_EVENT_HDR_SIZE;
			break;
		case HCI_SCODATA_PKT:
			len = HCI_MAX_SCO_SIZE;
			hlen = HCI_SCO_HDR_SIZE;
			break;
		}

		skb = bt_skb_alloc(len, GFP_ATOMIC);
		if (!skb)
			return -ENOMEM;

		scb = (void *) skb->cb;
		scb->expect = hlen;
		scb->pkt_type = type;

		skb->dev = (void *) hdev;
		hdev->reassembly[index] = skb;
	}

	while (count) {
		scb = (void *) skb->cb;
		len = min_t(uint, scb->expect, count);

		memcpy(skb_put(skb, len), data, len);

		count -= len;
		data += len;
		scb->expect -= len;
		remain = count;

		switch (type) {
		case HCI_EVENT_PKT:
			if (skb->len == HCI_EVENT_HDR_SIZE) {
				struct hci_event_hdr *h = hci_event_hdr(skb);
				scb->expect = h->plen;

				if (skb_tailroom(skb) < scb->expect) {
					kfree_skb(skb);
					hdev->reassembly[index] = NULL;
					return -ENOMEM;
				}
			}
			break;

		case HCI_ACLDATA_PKT:
			if (skb->len  == HCI_ACL_HDR_SIZE) {
				struct hci_acl_hdr *h = hci_acl_hdr(skb);
				scb->expect = __le16_to_cpu(h->dlen);

				if (skb_tailroom(skb) < scb->expect) {
					kfree_skb(skb);
					hdev->reassembly[index] = NULL;
					return -ENOMEM;
				}
			}
			break;

		case HCI_SCODATA_PKT:
			if (skb->len == HCI_SCO_HDR_SIZE) {
				struct hci_sco_hdr *h = hci_sco_hdr(skb);
				scb->expect = h->dlen;

				if (skb_tailroom(skb) < scb->expect) {
					kfree_skb(skb);
					hdev->reassembly[index] = NULL;
					return -ENOMEM;
				}
			}
			break;
		}

		if (scb->expect == 0) {
			/* Complete frame */
			if (HCI_ACLDATA_PKT == type)
				print_acl(skb, 0);
			if (HCI_SCODATA_PKT == type)
				print_sco(skb, 0);
			if (HCI_EVENT_PKT == type)
				print_event(skb);

			bt_cb(skb)->pkt_type = type;
			hci_recv_frame(skb);

			hdev->reassembly[index] = NULL;
			return remain;
		}
	}

	return remain;
}

static int hci_recv_fragment(struct hci_dev *hdev, int type, void *data, int count)
{
	int rem = 0;

	if (type < HCI_ACLDATA_PKT || type > HCI_EVENT_PKT)
		return -EILSEQ;

	while (count) {
		rem = hci_reassembly(hdev, type, data, count, type - 1);
		if (rem < 0)
			return rem;

		data += (count - rem);
		count = rem;
	}

	return rem;
}

void hci_hardware_error(void)
{
	struct sk_buff *aic_skb_copy = NULL;
	int len = 3;
	uint8_t hardware_err_pkt[4] = {HCI_EVENT_PKT, 0x10, 0x01, HCI_VENDOR_USB_DISC_HARDWARE_ERROR};

	aic_skb_copy = alloc_skb(len, GFP_ATOMIC);
	if (!aic_skb_copy) {
		AICBT_ERR("%s: Failed to allocate mem", __func__);
		return;
	}

	memcpy(skb_put(aic_skb_copy, len), hardware_err_pkt, len);
	aic_enqueue(aic_skb_copy);

	wake_up_interruptible(&btchr_read_wait);
}

static int btchr_open(struct inode *inode_p, struct file  *file_p)
{
	struct btusb_data *data;
	struct hci_dev *hdev;

	AICBT_DBG("%s: BT usb char device is opening", __func__);
	/* Not open unless wanna tracing log */
	/* trace_printk("%s: open....\n", __func__); */

	hdev = hci_dev_get(0);
	if (!hdev) {
		AICBT_DBG("%s: Failed to get hci dev[NULL]", __func__);
		return -ENODEV;
	}
	data = GET_DRV_DATA(hdev);

	atomic_inc(&hdev->promisc);
	/*
	 * As bt device is not re-opened when hotplugged out, we cannot
	 * trust on file's private data(may be null) when other file ops
	 * are invoked.
	 */
	file_p->private_data = data;

	mutex_lock(&btchr_mutex);
	hci_dev_open(0);
	mutex_unlock(&btchr_mutex);

	return nonseekable_open(inode_p, file_p);
}

static int btchr_close(struct inode  *inode_p, struct file   *file_p)
{
	struct btusb_data *data;
	struct hci_dev *hdev;

	AICBT_INFO("%s: BT usb char device is closing", __func__);
	/* Not open unless wanna tracing log */
	/* trace_printk("%s: close....\n", __func__); */

	data = file_p->private_data;
	file_p->private_data = NULL;

#if CONFIG_BLUEDROID
	/*
	 * If the upper layer closes bt char interfaces, no reset
	 * action required even bt device hotplugged out.
	 */
	bt_reset = 0;
#endif

	hdev = hci_dev_get(0);
	if (hdev) {
		atomic_set(&hdev->promisc, 0);
		mutex_lock(&btchr_mutex);
		hci_dev_close(0);
		mutex_unlock(&btchr_mutex);
	}

	return 0;
}

static ssize_t btchr_read(struct file *file_p,
		char __user *buf_p,
		size_t count,
		loff_t *pos_p)
{
	struct hci_dev *hdev;
	struct sk_buff *skb;
	ssize_t ret = 0;

	while (count) {
		hdev = hci_dev_get(0);
		if (!hdev) {
			/*
			 * Note: Only when BT device hotplugged out, we wil get
			 * into such situation. In order to keep the upper layer
			 * stack alive (blocking the read), we should never return
			 * EFAULT or break the loop.
			 */
			AICBT_ERR("%s: Failed to get hci dev[Null]", __func__);
		}

		ret = wait_event_interruptible(btchr_read_wait, !is_queue_empty());
		if (ret < 0) {
			AICBT_ERR("%s: wait event is signaled %d", __func__, (int)ret);
			break;
		}

		skb = aic_dequeue_try(count);
		if (skb) {
			ret = usb_put_user(skb, buf_p, count);
			if (ret < 0)
				AICBT_ERR("%s: Failed to put data to user space", __func__);
			kfree_skb(skb);
			break;
		}
	}

	return ret;
}

static ssize_t btchr_write(struct file *file_p,
		const char __user *buf_p,
		size_t count,
		loff_t *pos_p)
{
	struct btusb_data *data = file_p->private_data;
	struct hci_dev *hdev;
	struct sk_buff *skb;

	AICBT_DBG("%s", __func__);

	hdev = hci_dev_get(0);
	if (!hdev) {
		AICBT_WARN("%s: Failed to get hci dev[Null]", __func__);
		/*
		 * Note: we bypass the data from the upper layer if bt device
		 * is hotplugged out. Fortunatelly, H4 or H5 HCI stack does
		 * NOT check btchr_write's return value. However, returning
		 * count instead of EFAULT is preferable.
		 */
		/* return -EFAULT; */
		return count;
	}

	/* Never trust on btusb_data, as bt device may be hotplugged out */
	data = GET_DRV_DATA(hdev);
	if (!data) {
		AICBT_WARN("%s: Failed to get bt usb driver data[Null]", __func__);
		return count;
	}

	if (count > HCI_MAX_FRAME_SIZE)
		return -EINVAL;

	skb = bt_skb_alloc(count, GFP_ATOMIC);
	if (!skb)
		return -ENOMEM;
	skb_reserve(skb, -1); // Add this line

	if (copy_from_user(skb_put(skb, count), buf_p, count)) {
		AICBT_ERR("%s: Failed to get data from user space", __func__);
		kfree_skb(skb);
		return -EFAULT;
	}

	skb->dev = (void *)hdev;
	bt_cb(skb)->pkt_type = *((__u8 *)skb->data);
	skb_pull(skb, 1);
	data->hdev->send(skb);

	return count;
}

static unsigned int btchr_poll(struct file *file_p, poll_table *wait)
{
	struct btusb_data *data = file_p->private_data;
	struct hci_dev *hdev;

	if (!bt_char_dev_registered) {
		AICBT_ERR("%s: char device has not registered!", __func__);
		return POLLERR | POLLHUP;
	}

	poll_wait(file_p, &btchr_read_wait, wait);

	hdev = hci_dev_get(0);
	if (!hdev) {
		AICBT_ERR("%s: Failed to get hci dev[Null]", __func__);
		mdelay(URB_CANCELING_DELAY_MS);
		return POLLERR | POLLHUP;
		return POLLOUT | POLLWRNORM;
	}

	/* Never trust on btusb_data, as bt device may be hotplugged out */
	data = GET_DRV_DATA(hdev);
	if (!data) {
		/*
		 * When bt device is hotplugged out, btusb_data will
		 * be freed in disconnect.
		 */
		AICBT_ERR("%s: Failed to get bt usb driver data[Null]", __func__);
		mdelay(URB_CANCELING_DELAY_MS);
		return POLLOUT | POLLWRNORM;
	}

	if (!is_queue_empty())
		return POLLIN | POLLRDNORM;

	return POLLOUT | POLLWRNORM;
}

static long btchr_ioctl(struct file *file_p, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct hci_dev *hdev;
	struct btusb_data *data;
	firmware_info *fw_info;

	if (!bt_char_dev_registered) {
		return -ENODEV;
	}

	if (check_set_dlfw_state_value(1) != 1) {
		AICBT_ERR("%s bt controller is disconnecting!", __func__);
		return 0;
	}

	hdev = hci_dev_get(0);
	if (!hdev) {
		AICBT_ERR("%s device is NULL!", __func__);
		set_dlfw_state_value(0);
		return 0;
	}
	data = GET_DRV_DATA(hdev);
	fw_info = data->fw_info;

	AICBT_INFO(" btchr_ioctl DOWN_FW_CFG with Cmd:%d", cmd);
	switch (cmd) {
	case DOWN_FW_CFG:
		AICBT_INFO(" btchr_ioctl DOWN_FW_CFG");
		ret = usb_autopm_get_interface(data->intf);
		if (ret < 0) {
			goto failed;
		}

		//ret = download_patch(fw_info,1);
		usb_autopm_put_interface(data->intf);
		if (ret < 0) {
			AICBT_ERR("%s:Failed in download_patch with ret:%d", __func__, ret);
			goto failed;
		}

		ret = hdev->open(hdev);
		if (ret < 0) {
			AICBT_ERR("%s:Failed in hdev->open(hdev):%d", __func__, ret);
			goto failed;
		}
		set_bit(HCI_UP, &hdev->flags);
		set_dlfw_state_value(0);
		wake_up_interruptible(&bt_dlfw_wait);
		return 1;
	case DWFW_CMPLT:
		AICBT_INFO(" btchr_ioctl DWFW_CMPLT");
	case SET_ISO_CFG:
		AICBT_INFO("btchr_ioctl SET_ISO_CFG");
	case GET_USB_INFO:
		AICBT_INFO(" btchr_ioctl GET_USB_INFO");
		ret = hdev->open(hdev);
		if (ret < 0) {
			AICBT_ERR("%s:Failed in hdev->open(hdev):%d", __func__, ret);
			//goto done;
		}
		put_user(usb_info, (__u32 __user *)arg);
		set_bit(HCI_UP, &hdev->flags);
		set_dlfw_state_value(0);
		wake_up_interruptible(&bt_dlfw_wait);
		return 1;
	case RESET_CONTROLLER:
		AICBT_INFO(" btchr_ioctl RESET_CONTROLLER");
		//reset_controller(fw_info);
		return 1;
	default:
		AICBT_ERR("%s:Failed with wrong Cmd:%d", __func__, cmd);
		goto failed;
	}

failed:
	set_dlfw_state_value(0);
	wake_up_interruptible(&bt_dlfw_wait);
	return ret;
}

#ifdef CONFIG_COMPAT
static long compat_btchr_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
	AICBT_DBG("%s: enter", __func__);
	return btchr_ioctl(filp, cmd, (unsigned long) compat_ptr(arg));
}
#endif

static struct file_operations bt_chrdev_ops  = {
	.open    = btchr_open,
	.release = btchr_close,
	.read    = btchr_read,
	.write   = btchr_write,
	.poll    = btchr_poll,
	.unlocked_ioctl = btchr_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl   = compat_btchr_ioctl,
#endif
};

static int btchr_init(void)
{
	int res = 0;
	struct device *dev;

	AICBT_INFO("Register usb char device interface for BT driver");
	/*
	 * btchr mutex is used to sync between
	 * 1) downloading patch and opening bt char driver
	 * 2) the file operations of bt char driver
	 */
	mutex_init(&btchr_mutex);

	skb_queue_head_init(&btchr_readq);
	init_waitqueue_head(&btchr_read_wait);
	init_waitqueue_head(&bt_dlfw_wait);

	bt_char_class = class_create(THIS_MODULE, BT_CHAR_DEVICE_NAME);
	if (IS_ERR(bt_char_class)) {
		AICBT_ERR("Failed to create bt char class");
		return PTR_ERR(bt_char_class);
	}

	res = alloc_chrdev_region(&bt_devid, 0, 1, BT_CHAR_DEVICE_NAME);
	if (res < 0) {
		AICBT_ERR("Failed to allocate bt char device");
		goto err_alloc;
	}

	dev = device_create(bt_char_class, NULL, bt_devid, NULL, BT_CHAR_DEVICE_NAME);
	if (IS_ERR(dev)) {
		AICBT_ERR("Failed to create bt char device");
		res = PTR_ERR(dev);
		goto err_create;
	}

	cdev_init(&bt_char_dev, &bt_chrdev_ops);
	res = cdev_add(&bt_char_dev, bt_devid, 1);
	if (res < 0) {
		AICBT_ERR("Failed to add bt char device");
		goto err_add;
	}

	return 0;

err_add:
	device_destroy(bt_char_class, bt_devid);
err_create:
	unregister_chrdev_region(bt_devid, 1);
err_alloc:
	class_destroy(bt_char_class);
	return res;
}

static void btchr_exit(void)
{
	AICBT_INFO("Unregister usb char device interface for BT driver");

	device_destroy(bt_char_class, bt_devid);
	cdev_del(&bt_char_dev);
	unregister_chrdev_region(bt_devid, 1);
	class_destroy(bt_char_class);

	return;
}
#endif

int send_hci_cmd(firmware_info *fw_info)
{
	int len = 0;
	int ret_val = -1;

	ret_val = usb_bulk_msg(fw_info->udev, fw_info->pipe_out, fw_info->send_pkt, fw_info->pkt_len,
			&len, 3000);
	if (ret_val || (len != fw_info->pkt_len)) {
		AICBT_INFO("Error in send hci cmd = %d,"
				"len = %d, size = %d", ret_val, len, fw_info->pkt_len);
	}

	return ret_val;
}

int rcv_hci_evt(firmware_info *fw_info)
{
	int ret_len = 0, ret_val = 0;
	int i;

	while (1) {
		for (i = 0; i < 5; i++) {
			ret_val = usb_interrupt_msg(
					fw_info->udev, fw_info->pipe_in,
					(void *)(fw_info->rcv_pkt), PKT_LEN,
					&ret_len, MSG_TO);
			if (ret_val >= 0)
				break;
		}

		if (ret_val < 0)
			return ret_val;

		if (CMD_CMP_EVT == fw_info->evt_hdr->evt) {
			if (fw_info->cmd_hdr->opcode == fw_info->cmd_cmp->opcode)
				return ret_len;
		}
	}
}

int set_bt_onoff(firmware_info *fw_info, uint8_t onoff)
{
	int ret_val;

	AICBT_INFO("%s: %s", __func__, onoff != 0 ? "on" : "off");

	fw_info->cmd_hdr->opcode = cpu_to_le16(BTOFF_OPCODE);
	fw_info->cmd_hdr->plen = 1;
	fw_info->pkt_len = CMD_HDR_LEN + 1;
	fw_info->send_pkt[CMD_HDR_LEN] = onoff;

	ret_val = send_hci_cmd(fw_info);
	if (ret_val < 0) {
		AICBT_ERR("%s: Failed to send bt %s cmd, errno %d",
				__func__, onoff != 0 ? "on" : "off", ret_val);
		return ret_val;
	}

	ret_val = rcv_hci_evt(fw_info);
	if (ret_val < 0) {
		AICBT_ERR("%s: Failed to receive bt %s event, errno %d",
				__func__, onoff != 0 ? "on" : "off", ret_val);
		return ret_val;
	}

	return ret_val;
}

firmware_info *firmware_info_init(struct usb_interface *intf)
{
	struct usb_device *udev = interface_to_usbdev(intf);
	firmware_info *fw_info;

	AICBT_DBG("%s: start", __func__);

	fw_info = kzalloc(sizeof(*fw_info), GFP_KERNEL);
	if (!fw_info)
		return NULL;

	fw_info->send_pkt = kzalloc(PKT_LEN, GFP_KERNEL);
	if (!fw_info->send_pkt) {
		kfree(fw_info);
		return NULL;
	}

	fw_info->rcv_pkt = kzalloc(PKT_LEN, GFP_KERNEL);
	if (!fw_info->rcv_pkt) {
		kfree(fw_info->send_pkt);
		kfree(fw_info);
		return NULL;
	}

	fw_info->intf = intf;
	fw_info->udev = udev;
	fw_info->pipe_in = usb_rcvbulkpipe(fw_info->udev, BULK_EP);
	fw_info->pipe_out = usb_rcvbulkpipe(fw_info->udev, CTRL_EP);
	fw_info->cmd_hdr = (struct hci_command_hdr *)(fw_info->send_pkt);
	fw_info->evt_hdr = (struct hci_event_hdr *)(fw_info->rcv_pkt);
	fw_info->cmd_cmp = (struct hci_ev_cmd_complete *)(fw_info->rcv_pkt + EVT_HDR_LEN);
	fw_info->req_para = fw_info->send_pkt + CMD_HDR_LEN;
	fw_info->rsp_para = fw_info->rcv_pkt + EVT_HDR_LEN + CMD_CMP_LEN;

#if BTUSB_RPM
	AICBT_INFO("%s: Auto suspend is enabled", __func__);
	usb_enable_autosuspend(udev);
	pm_runtime_set_autosuspend_delay(&(udev->dev), 2000);
#else
	AICBT_INFO("%s: Auto suspend is disabled", __func__);
	usb_disable_autosuspend(udev);
#endif

#if BTUSB_WAKEUP_HOST
	device_wakeup_enable(&udev->dev);
#endif

	return fw_info;
}

void firmware_info_destroy(struct usb_interface *intf)
{
	firmware_info *fw_info;
	struct usb_device *udev;
	struct btusb_data *data;

	udev = interface_to_usbdev(intf);
	data = usb_get_intfdata(intf);

	fw_info = data->fw_info;
	if (!fw_info)
		return;

#if BTUSB_RPM
	usb_disable_autosuspend(udev);
#endif

	/*
	 * In order to reclaim fw data mem, we free fw_data immediately
	 * after download patch finished instead of here.
	 */
	kfree(fw_info->rcv_pkt);
	kfree(fw_info->send_pkt);
	kfree(fw_info);
}

static struct usb_driver btusb_driver;

static struct usb_device_id btusb_table[] = {
	//{USB_DEVICE_AND_INTERFACE_INFO(0xa69c, 0x8801, 0xe0, 0x01, 0x01)},
	{USB_DEVICE_AND_INTERFACE_INFO(USB_VENDOR_ID_AIC, USB_DEVICE_ID_AIC_8801, 0xff, 0xff, 0xff)},
	{USB_DEVICE_AND_INTERFACE_INFO(USB_VENDOR_ID_AIC, USB_DEVICE_ID_AIC_8800D81, 0xff, 0xff, 0xff)},
	{USB_DEVICE_AND_INTERFACE_INFO(USB_VENDOR_ID_AIC, USB_DEVICE_ID_AIC_8800D41, 0xff, 0xff, 0xff)},
	{}
};

MODULE_DEVICE_TABLE(usb, btusb_table);

static int inc_tx(struct btusb_data *data)
{
	unsigned long flags;
	int rv;

	spin_lock_irqsave(&data->txlock, flags);
	rv = test_bit(BTUSB_SUSPENDING, &data->flags);
	if (!rv)
		data->tx_in_flight++;
	spin_unlock_irqrestore(&data->txlock, flags);

	return rv;
}

void check_sco_event(struct urb *urb)
{
	u8 *opcode = (u8 *)(urb->transfer_buffer);
	u8 status;
	static uint16_t sco_handle;
	uint16_t handle;
	struct hci_dev *hdev = urb->context;
	struct btusb_data *data = GET_DRV_DATA(hdev);

	switch (*opcode) {
	case HCI_EV_SYNC_CONN_COMPLETE:
		AICBT_INFO("%s: HCI_EV_SYNC_CONN_COMPLETE(0x%02x)", __func__, *opcode);
		status = *(opcode + 2);
		sco_handle = *(opcode + 3) | *(opcode + 4) << 8;
		if (status == 0) {
			hdev->conn_hash.sco_num++;
			schedule_work(&data->work);
		}
		break;
	case HCI_EV_DISCONN_COMPLETE:
		AICBT_INFO("%s: HCI_EV_DISCONN_COMPLETE(0x%02x)", __func__, *opcode);
		status = *(opcode + 2);
		handle = *(opcode + 3) | *(opcode + 4) << 8;
		if (status == 0 && sco_handle == handle) {
			hdev->conn_hash.sco_num--;
			schedule_work(&data->work);
		}
		break;
	default:
		AICBT_DBG("%s: event 0x%02x", __func__, *opcode);
		break;
	}
}

static void btusb_intr_complete(struct urb *urb)
{
	struct hci_dev *hdev = urb->context;
	struct btusb_data *data = GET_DRV_DATA(hdev);
	int err;

	AICBT_DBG("%s: urb %p status %d count %d ", __func__,
			urb, urb->status, urb->actual_length);

	if (!test_bit(HCI_RUNNING, &hdev->flags))
		return;


	if (urb->status == 0) {
		hdev->stat.byte_rx += urb->actual_length;

		if (hci_recv_fragment(hdev, HCI_EVENT_PKT,
						urb->transfer_buffer,
						urb->actual_length) < 0) {
			AICBT_ERR("%s: Corrupted event packet", __func__);
			hdev->stat.err_rx++;
		}
	} else if (urb->status == -ENOENT) { /* Avoid suspend failed when usb_kill_urb */
		return;
	}

	if (!test_bit(BTUSB_INTR_RUNNING, &data->flags))
		return;

	usb_mark_last_busy(data->udev);
	usb_anchor_urb(urb, &data->intr_anchor);

	err = usb_submit_urb(urb, GFP_ATOMIC);
	if (err < 0) {
		if (err != -EPERM && err != -ENODEV)
			AICBT_ERR("%s: Failed to re-submit urb %p, err %d",
					__func__, urb, err);
		usb_unanchor_urb(urb);
	}
}

static int btusb_submit_intr_urb(struct hci_dev *hdev, gfp_t mem_flags)
{
	struct btusb_data *data = GET_DRV_DATA(hdev);
	struct urb *urb;
	unsigned char *buf;
	unsigned int pipe;
	int err, size;

	if (!data->intr_ep)
		return -ENODEV;

	urb = usb_alloc_urb(0, mem_flags);
	if (!urb)
		return -ENOMEM;

	size = le16_to_cpu(data->intr_ep->wMaxPacketSize);

	buf = kmalloc(size, mem_flags);
	if (!buf) {
		usb_free_urb(urb);
		return -ENOMEM;
	}

	AICBT_DBG("%s: mMaxPacketSize %d, bEndpointAddress 0x%02x",
			__func__, size, data->intr_ep->bEndpointAddress);

	pipe = usb_rcvintpipe(data->udev, data->intr_ep->bEndpointAddress);

	usb_fill_int_urb(urb, data->udev, pipe, buf, size,
						btusb_intr_complete, hdev,
						data->intr_ep->bInterval);

	urb->transfer_flags |= URB_FREE_BUFFER;

	usb_anchor_urb(urb, &data->intr_anchor);

	err = usb_submit_urb(urb, mem_flags);
	if (err < 0) {
		AICBT_ERR("%s: Failed to submit urb %p, err %d",
				__func__, urb, err);
		usb_unanchor_urb(urb);
	}

	usb_free_urb(urb);

	return err;
}

static void btusb_bulk_complete(struct urb *urb)
{
	struct hci_dev *hdev = urb->context;
	struct btusb_data *data = GET_DRV_DATA(hdev);
	int err;
	uint8_t *data_;
	(void)data_;

	AICBT_DBG("%s: urb %p status %d count %d",
			__func__, urb, urb->status, urb->actual_length);

	if (!test_bit(HCI_RUNNING, &hdev->flags)) {
		printk("%s HCI_RUNNING\n", __func__);
		return;
	}
	if (urb->status == 0) {
		hdev->stat.byte_rx += urb->actual_length;

		#if 1
		if (hci_recv_fragment(hdev, HCI_ACLDATA_PKT,
						urb->transfer_buffer,
						urb->actual_length) < 0) {
			AICBT_ERR("%s: Corrupted ACL packet", __func__);
			hdev->stat.err_rx++;
		}
		#else
		data_ = urb->transfer_buffer;
		if (hci_recv_fragment(hdev, data_[0],
						&data_[1],
						urb->actual_length - 1) < 0) {
			AICBT_ERR("%s: Corrupted ACL packet", __func__);
			hdev->stat.err_rx++;
		}
		#endif
	} else if (urb->status == -ENOENT) {
		/* Avoid suspend failed when usb_kill_urb */
		printk("%s ENOENT\n", __func__);
		return;
	}
	AICBT_DBG("%s: OUT", __func__);

	if (!test_bit(BTUSB_BULK_RUNNING, &data->flags)) {
		printk("%s BTUSB_BULK_RUNNING\n", __func__);
		return;
	}
	usb_anchor_urb(urb, &data->bulk_anchor);
	usb_mark_last_busy(data->udev);

	err = usb_submit_urb(urb, GFP_ATOMIC);
	if (err < 0) {
		/* -EPERM: urb is being killed;
		 * -ENODEV: device got disconnected */
		if (err != -EPERM && err != -ENODEV)
			AICBT_ERR("btusb_bulk_complete %s urb %p failed to resubmit (%d)",
						hdev->name, urb, -err);
		usb_unanchor_urb(urb);
	}
}

static int btusb_submit_bulk_urb(struct hci_dev *hdev, gfp_t mem_flags)
{
	struct btusb_data *data = GET_DRV_DATA(hdev);
	struct urb *urb;
	unsigned char *buf;
	unsigned int pipe;
	int err, size = HCI_MAX_FRAME_SIZE;

	AICBT_DBG("%s: hdev name %s", __func__, hdev->name);
	AICBT_DBG("%s: mMaxPacketSize %d, bEndpointAddress 0x%02x",
			__func__, size, data->bulk_rx_ep->bEndpointAddress);

	if (!data->bulk_rx_ep)
		return -ENODEV;

	urb = usb_alloc_urb(0, mem_flags);
	if (!urb)
		return -ENOMEM;

	buf = kmalloc(size, mem_flags);
	if (!buf) {
		usb_free_urb(urb);
		return -ENOMEM;
	}

	pipe = usb_rcvbulkpipe(data->udev, data->bulk_rx_ep->bEndpointAddress);

	usb_fill_bulk_urb(urb, data->udev, pipe,
					buf, size, btusb_bulk_complete, hdev);

	urb->transfer_flags |= URB_FREE_BUFFER;

	usb_mark_last_busy(data->udev);
	usb_anchor_urb(urb, &data->bulk_anchor);

	err = usb_submit_urb(urb, mem_flags);
	if (err < 0) {
		AICBT_ERR("%s: Failed to submit urb %p, err %d", __func__, urb, err);
		usb_unanchor_urb(urb);
	}

	usb_free_urb(urb);

	return err;
}

static void btusb_isoc_complete(struct urb *urb)
{
	struct hci_dev *hdev = urb->context;
	struct btusb_data *data = GET_DRV_DATA(hdev);
	int i, err;


	AICBT_DBG("%s: urb %p status %d count %d",
			__func__, urb, urb->status, urb->actual_length);

	if (!test_bit(HCI_RUNNING, &hdev->flags))
		return;

	if (urb->status == 0) {
		for (i = 0; i < urb->number_of_packets; i++) {
			unsigned int offset = urb->iso_frame_desc[i].offset;
			unsigned int length = urb->iso_frame_desc[i].actual_length;

			if (urb->iso_frame_desc[i].status)
				continue;

			hdev->stat.byte_rx += length;

			if (hci_recv_fragment(hdev, HCI_SCODATA_PKT,
						urb->transfer_buffer + offset,
								length) < 0) {
				AICBT_ERR("%s: Corrupted SCO packet", __func__);
				hdev->stat.err_rx++;
			}
		}
	} else if (urb->status == -ENOENT) {
		/* Avoid suspend failed when usb_kill_urb */
		return;
	}

	if (!test_bit(BTUSB_ISOC_RUNNING, &data->flags))
		return;

	usb_anchor_urb(urb, &data->isoc_anchor);
	i = 0;
retry:
	err = usb_submit_urb(urb, GFP_ATOMIC);
	if (err < 0) {
		/* -EPERM: urb is being killed;
		 * -ENODEV: device got disconnected */
		if (err != -EPERM && err != -ENODEV)
			AICBT_ERR("%s: Failed to re-sumbit urb %p, retry %d, err %d",
					__func__, urb, i, err);
		if (i < 10) {
			i++;
			mdelay(1);
			goto retry;
		}

		usb_unanchor_urb(urb);
	}
}

static inline void fill_isoc_descriptor(struct urb *urb, int len, int mtu)
{
	int i, offset = 0;

	AICBT_DBG("%s: len %d mtu %d", __func__, len, mtu);

	for (i = 0; i < BTUSB_MAX_ISOC_FRAMES && len >= mtu;
					i++, offset += mtu, len -= mtu) {
		urb->iso_frame_desc[i].offset = offset;
		urb->iso_frame_desc[i].length = mtu;
	}

	if (len && i < BTUSB_MAX_ISOC_FRAMES) {
		urb->iso_frame_desc[i].offset = offset;
		urb->iso_frame_desc[i].length = len;
		i++;
	}

	urb->number_of_packets = i;
}

static int btusb_submit_isoc_urb(struct hci_dev *hdev, gfp_t mem_flags)
{
	struct btusb_data *data = GET_DRV_DATA(hdev);
	struct urb *urb;
	unsigned char *buf;
	unsigned int pipe;
	int err, size;

	if (!data->isoc_rx_ep)
		return -ENODEV;
	AICBT_DBG("%s: mMaxPacketSize %d, bEndpointAddress 0x%02x",
			__func__, size, data->isoc_rx_ep->bEndpointAddress);

	urb = usb_alloc_urb(BTUSB_MAX_ISOC_FRAMES, mem_flags);
	if (!urb)
		return -ENOMEM;

	size = le16_to_cpu(data->isoc_rx_ep->wMaxPacketSize) *
						BTUSB_MAX_ISOC_FRAMES;

	buf = kmalloc(size, mem_flags);
	if (!buf) {
		usb_free_urb(urb);
		return -ENOMEM;
	}

	pipe = usb_rcvisocpipe(data->udev, data->isoc_rx_ep->bEndpointAddress);

	urb->dev      = data->udev;
	urb->pipe     = pipe;
	urb->context  = hdev;
	urb->complete = btusb_isoc_complete;
	urb->interval = data->isoc_rx_ep->bInterval;

	urb->transfer_flags  = URB_FREE_BUFFER | URB_ISO_ASAP;
	urb->transfer_buffer = buf;
	urb->transfer_buffer_length = size;

	fill_isoc_descriptor(urb, size,
			le16_to_cpu(data->isoc_rx_ep->wMaxPacketSize));

	usb_anchor_urb(urb, &data->isoc_anchor);

	err = usb_submit_urb(urb, mem_flags);
	if (err < 0) {
		AICBT_ERR("%s: Failed to submit urb %p, err %d", __func__, urb, err);
		usb_unanchor_urb(urb);
	}

	usb_free_urb(urb);

	return err;
}

static void btusb_tx_complete(struct urb *urb)
{
	struct sk_buff *skb = urb->context;
	struct hci_dev *hdev = (struct hci_dev *) skb->dev;
	struct btusb_data *data = GET_DRV_DATA(hdev);

	if (!test_bit(HCI_RUNNING, &hdev->flags))
		goto done;

	if (!urb->status)
		hdev->stat.byte_tx += urb->transfer_buffer_length;
	else
		hdev->stat.err_tx++;

done:
	spin_lock(&data->txlock);
	data->tx_in_flight--;
	spin_unlock(&data->txlock);

	kfree(urb->setup_packet);

	kfree_skb(skb);
}

static void btusb_isoc_tx_complete(struct urb *urb)
{
	struct sk_buff *skb = urb->context;
	struct hci_dev *hdev = (struct hci_dev *) skb->dev;

	AICBT_DBG("%s: urb %p status %d count %d",
			__func__, urb, urb->status, urb->actual_length);

	if (skb && hdev) {
		if (!test_bit(HCI_RUNNING, &hdev->flags))
			goto done;

		if (!urb->status)
			hdev->stat.byte_tx += urb->transfer_buffer_length;
		else
			hdev->stat.err_tx++;
	} else
		AICBT_ERR("%s: skb 0x%p hdev 0x%p", __func__, skb, hdev);

done:
	kfree(urb->setup_packet);

	kfree_skb(skb);
}

static int btusb_open(struct hci_dev *hdev)
{
	struct btusb_data *data = GET_DRV_DATA(hdev);
	int err = 0;

	AICBT_INFO("%s: Start", __func__);

	err = usb_autopm_get_interface(data->intf);
	if (err < 0)
		return err;

	data->intf->needs_remote_wakeup = 1;

	if (test_and_set_bit(HCI_RUNNING, &hdev->flags))
		goto done;

	if (test_and_set_bit(BTUSB_INTR_RUNNING, &data->flags))
		goto done;

	err = btusb_submit_intr_urb(hdev, GFP_KERNEL);
	if (err < 0)
		goto failed;

	err = btusb_submit_bulk_urb(hdev, GFP_KERNEL);
	if (err < 0) {
		mdelay(URB_CANCELING_DELAY_MS);
		usb_kill_anchored_urbs(&data->intr_anchor);
		goto failed;
	}

	set_bit(BTUSB_BULK_RUNNING, &data->flags);
	btusb_submit_bulk_urb(hdev, GFP_KERNEL);

done:
	usb_autopm_put_interface(data->intf);
	AICBT_INFO("%s: End", __func__);
	return 0;

failed:
	clear_bit(BTUSB_INTR_RUNNING, &data->flags);
	clear_bit(HCI_RUNNING, &hdev->flags);
	usb_autopm_put_interface(data->intf);
	AICBT_ERR("%s: Failed", __func__);
	return err;
}

static void btusb_stop_traffic(struct btusb_data *data)
{
	mdelay(URB_CANCELING_DELAY_MS);
	usb_kill_anchored_urbs(&data->intr_anchor);
	usb_kill_anchored_urbs(&data->bulk_anchor);
	usb_kill_anchored_urbs(&data->isoc_anchor);
}

static int btusb_close(struct hci_dev *hdev)
{
	struct btusb_data *data = GET_DRV_DATA(hdev);
	int i, err;

	AICBT_INFO("%s: hci running %lu", __func__, hdev->flags & HCI_RUNNING);

	if (!test_and_clear_bit(HCI_RUNNING, &hdev->flags))
		return 0;

	if (!test_and_clear_bit(BTUSB_INTR_RUNNING, &data->flags))
		return 0;

	for (i = 0; i < NUM_REASSEMBLY; i++) {
		if (hdev->reassembly[i]) {
			AICBT_DBG("%s: free ressembly[%d]", __func__, i);
			kfree_skb(hdev->reassembly[i]);
			hdev->reassembly[i] = NULL;
		}
	}

	cancel_work_sync(&data->work);
	cancel_work_sync(&data->waker);

	clear_bit(BTUSB_ISOC_RUNNING, &data->flags);
	clear_bit(BTUSB_BULK_RUNNING, &data->flags);

	btusb_stop_traffic(data);
	err = usb_autopm_get_interface(data->intf);
	if (err < 0)
		goto failed;

	data->intf->needs_remote_wakeup = 0;
	usb_autopm_put_interface(data->intf);

failed:
	mdelay(URB_CANCELING_DELAY_MS);
	usb_scuttle_anchored_urbs(&data->deferred);
	return 0;
}

static int btusb_flush(struct hci_dev *hdev)
{
	struct btusb_data *data = GET_DRV_DATA(hdev);

	AICBT_DBG("%s", __func__);

	mdelay(URB_CANCELING_DELAY_MS);
	usb_kill_anchored_urbs(&data->tx_anchor);

	return 0;
}


static int btusb_send_frame(struct sk_buff *skb)
{
	struct hci_dev *hdev = (struct hci_dev *) skb->dev;

	struct btusb_data *data = GET_DRV_DATA(hdev);
	struct usb_ctrlrequest *dr;
	struct urb *urb;
	unsigned int pipe;
	int err;
	int retries = 0;
	u16 *opcode = NULL;

	AICBT_DBG("%s: hdev %p, btusb data %p, pkt type %d",
			__func__, hdev, data, bt_cb(skb)->pkt_type);

	if (!test_bit(HCI_RUNNING, &hdev->flags))
		return -EBUSY;


	switch (bt_cb(skb)->pkt_type) {
	case HCI_COMMAND_PKT:
		print_command(skb);
		urb = usb_alloc_urb(0, GFP_ATOMIC);
		if (!urb)
			return -ENOMEM;

		dr = kmalloc(sizeof(*dr), GFP_ATOMIC);
		if (!dr) {
			usb_free_urb(urb);
			return -ENOMEM;
		}

		dr->bRequestType = data->cmdreq_type;
		dr->bRequest     = 0;
		dr->wIndex       = 0;
		dr->wValue       = 0;
		dr->wLength      = __cpu_to_le16(skb->len);

		pipe = usb_sndctrlpipe(data->udev, 0x00);

		usb_fill_control_urb(urb, data->udev, pipe, (void *) dr,
				skb->data, skb->len, btusb_tx_complete, skb);

		hdev->stat.cmd_tx++;
		break;

	case HCI_ACLDATA_PKT:
		if (bt_cb(skb)->pkt_type == HCI_COMMAND_PKT) {
			print_command(skb);
			opcode = (u16 *)(skb->data);
			printk("aic cmd:0x%04x", *opcode);
		} else {
			print_acl(skb, 1);
		}
		if (!data->bulk_tx_ep)
			return -ENODEV;

		urb = usb_alloc_urb(0, GFP_ATOMIC);
		if (!urb)
			return -ENOMEM;

		pipe = usb_sndbulkpipe(data->udev,
					data->bulk_tx_ep->bEndpointAddress);

		usb_fill_bulk_urb(urb, data->udev, pipe,
				skb->data, skb->len, btusb_tx_complete, skb);

		hdev->stat.acl_tx++;
		break;

	case HCI_SCODATA_PKT:
		print_sco(skb, 1);
		if (!data->isoc_tx_ep || SCO_NUM < 1) {
			kfree(skb);
			return -ENODEV;
		}

		urb = usb_alloc_urb(BTUSB_MAX_ISOC_FRAMES, GFP_ATOMIC);
		if (!urb) {
			AICBT_ERR("%s: Failed to allocate mem for sco pkts", __func__);
			kfree(skb);
			return -ENOMEM;
		}

		pipe = usb_sndisocpipe(data->udev, data->isoc_tx_ep->bEndpointAddress);

		usb_fill_int_urb(urb, data->udev, pipe,
				skb->data, skb->len, btusb_isoc_tx_complete,
				skb, data->isoc_tx_ep->bInterval);

		urb->transfer_flags  = URB_ISO_ASAP;

		fill_isoc_descriptor(urb, skb->len,
				le16_to_cpu(data->isoc_tx_ep->wMaxPacketSize));

		hdev->stat.sco_tx++;
		goto skip_waking;

	default:
		return -EILSEQ;
	}

	err = inc_tx(data);
	if (err) {
		usb_anchor_urb(urb, &data->deferred);
		schedule_work(&data->waker);
		err = 0;
		goto done;
	}

skip_waking:
	usb_anchor_urb(urb, &data->tx_anchor);
retry:
	err = usb_submit_urb(urb, GFP_ATOMIC);
	if (err < 0) {
		AICBT_ERR("%s: Failed to submit urb %p, pkt type %d, err %d, retries %d",
				__func__, urb, bt_cb(skb)->pkt_type, err, retries);
		if ((bt_cb(skb)->pkt_type != HCI_SCODATA_PKT) && (retries < 10)) {
			mdelay(1);

			if (bt_cb(skb)->pkt_type == HCI_COMMAND_PKT)
				print_error_command(skb);
			retries++;
			goto retry;
		}
		kfree(urb->setup_packet);
		usb_unanchor_urb(urb);
	} else
		usb_mark_last_busy(data->udev);
	usb_free_urb(urb);

done:
	return err;
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 4, 0)
static void btusb_destruct(struct hci_dev *hdev)
{
	struct btusb_data *data = GET_DRV_DATA(hdev);

	AICBT_DBG("%s: name %s", __func__, hdev->name);

	kfree(data);
}
#endif

static void btusb_notify(struct hci_dev *hdev, unsigned int evt)
{
	struct btusb_data *data = GET_DRV_DATA(hdev);

	AICBT_DBG("%s: name %s, evt %d", __func__, hdev->name, evt);

	if (SCO_NUM != data->sco_num) {
		data->sco_num = SCO_NUM;
		schedule_work(&data->work);
	}
}

static inline int set_isoc_interface(struct hci_dev *hdev, int altsetting)
{
	struct btusb_data *data = GET_DRV_DATA(hdev);
	struct usb_interface *intf = data->isoc;
	struct usb_endpoint_descriptor *ep_desc;
	int i, err;

	if (!data->isoc)
		return -ENODEV;

	err = usb_set_interface(data->udev, 1, altsetting);
	if (err < 0) {
		AICBT_ERR("%s: Failed to set interface, altsetting %d, err %d",
				__func__, altsetting, err);
		return err;
	}

	data->isoc_altsetting = altsetting;

	data->isoc_tx_ep = NULL;
	data->isoc_rx_ep = NULL;

	for (i = 0; i < intf->cur_altsetting->desc.bNumEndpoints; i++) {
		ep_desc = &intf->cur_altsetting->endpoint[i].desc;

		if (!data->isoc_tx_ep && usb_endpoint_is_isoc_out(ep_desc)) {
			data->isoc_tx_ep = ep_desc;
			continue;
		}

		if (!data->isoc_rx_ep && usb_endpoint_is_isoc_in(ep_desc)) {
			data->isoc_rx_ep = ep_desc;
			continue;
		}
	}

	if (!data->isoc_tx_ep || !data->isoc_rx_ep) {
		AICBT_ERR("%s: Invalid SCO descriptors", __func__);
		return -ENODEV;
	}

	return 0;
}

static void btusb_work(struct work_struct *work)
{
	struct btusb_data *data = container_of(work, struct btusb_data, work);
	struct hci_dev *hdev = data->hdev;
	int err;
	int new_alts;
	if (data->sco_num > 0) {
		if (!test_bit(BTUSB_DID_ISO_RESUME, &data->flags)) {
			err = usb_autopm_get_interface(data->isoc ? data->isoc : data->intf);
			if (err < 0) {
				clear_bit(BTUSB_ISOC_RUNNING, &data->flags);
				mdelay(URB_CANCELING_DELAY_MS);
				usb_kill_anchored_urbs(&data->isoc_anchor);
				return;
			}

			set_bit(BTUSB_DID_ISO_RESUME, &data->flags);
		}
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 7, 1)
		if (hdev->voice_setting & 0x0020) {
			static const int alts[3] = { 2, 4, 5 };
			new_alts = alts[data->sco_num - 1];
		} else {
			new_alts = data->sco_num;
		}
		if (data->isoc_altsetting != new_alts) {
#else
		if (data->isoc_altsetting != 2) {
			new_alts = 2;
#endif

			clear_bit(BTUSB_ISOC_RUNNING, &data->flags);
			mdelay(URB_CANCELING_DELAY_MS);
			usb_kill_anchored_urbs(&data->isoc_anchor);

			if (set_isoc_interface(hdev, new_alts) < 0)
				return;
		}

		if (!test_and_set_bit(BTUSB_ISOC_RUNNING, &data->flags)) {
			if (btusb_submit_isoc_urb(hdev, GFP_KERNEL) < 0)
				clear_bit(BTUSB_ISOC_RUNNING, &data->flags);
			else
				btusb_submit_isoc_urb(hdev, GFP_KERNEL);
		}
	} else {
		clear_bit(BTUSB_ISOC_RUNNING, &data->flags);
		mdelay(URB_CANCELING_DELAY_MS);
		usb_kill_anchored_urbs(&data->isoc_anchor);

		set_isoc_interface(hdev, 0);
		if (test_and_clear_bit(BTUSB_DID_ISO_RESUME, &data->flags))
			usb_autopm_put_interface(data->isoc ? data->isoc : data->intf);
	}
}

static void btusb_waker(struct work_struct *work)
{
	struct btusb_data *data = container_of(work, struct btusb_data, waker);
	int err;

	AICBT_DBG("%s", __func__);

	err = usb_autopm_get_interface(data->intf);
	if (err < 0)
		return;

	usb_autopm_put_interface(data->intf);
}

int bt_pm_notify(struct notifier_block *notifier, ulong pm_event, void *unused)
{
	struct btusb_data *data;
	firmware_info *fw_info;
	struct usb_device *udev;

	AICBT_INFO("%s: pm event %ld", __func__, pm_event);

	data = container_of(notifier, struct btusb_data, pm_notifier);
	fw_info = data->fw_info;
	udev = fw_info->udev;

	switch (pm_event) {
	case PM_SUSPEND_PREPARE:
	case PM_HIBERNATION_PREPARE:
#if 0
		patch_entry->fw_len = load_firmware(fw_info, &patch_entry->fw_cache);
		if (patch_entry->fw_len <= 0) {
		/* We may encount failure in loading firmware, just give a warning */
			AICBT_WARN("%s: Failed to load firmware", __func__);
		}
#endif
		if (!device_may_wakeup(&udev->dev)) {
#if (CONFIG_RESET_RESUME || CONFIG_BLUEDROID)
			AICBT_INFO("%s:remote wakeup not supported, reset resume supported", __func__);
#else
			fw_info->intf->needs_binding = 1;
			AICBT_INFO("%s:remote wakeup not supported, binding needed", __func__);
#endif
		}
		break;

	case PM_POST_SUSPEND:
	case PM_POST_HIBERNATION:
	case PM_POST_RESTORE:
#if 0
		/* Reclaim fw buffer when bt usb resumed */
		if (patch_entry->fw_len > 0) {
			kfree(patch_entry->fw_cache);
			patch_entry->fw_cache = NULL;
			patch_entry->fw_len = 0;
		}
#endif

#if BTUSB_RPM
		usb_disable_autosuspend(udev);
		usb_enable_autosuspend(udev);
		pm_runtime_set_autosuspend_delay(&(udev->dev), 2000);
#endif
		break;

	default:
		break;
	}

	return NOTIFY_DONE;
}

int bt_reboot_notify(struct notifier_block *notifier, ulong pm_event, void *unused)
{
	struct btusb_data *data;
	firmware_info *fw_info;
	struct usb_device *udev;

	AICBT_INFO("%s: pm event %ld", __func__, pm_event);

	data = container_of(notifier, struct btusb_data, reboot_notifier);
	fw_info = data->fw_info;
	udev = fw_info->udev;

	switch (pm_event) {
	case SYS_DOWN:
		AICBT_DBG("%s:system down or restart", __func__);
	break;

	case SYS_HALT:
	case SYS_POWER_OFF:
#if SUSPNED_DW_FW
		cancel_work_sync(&data->work);

		btusb_stop_traffic(data);
		mdelay(URB_CANCELING_DELAY_MS);
		usb_kill_anchored_urbs(&data->tx_anchor);

		if (fw_info_4_suspend) {
			download_suspend_patch(fw_info_4_suspend, 1);
		} else {
			AICBT_ERR("%s: Failed to download suspend fw", __func__);
		}
#endif

#ifdef SET_WAKEUP_DEVICE
		set_wakeup_device_from_conf(fw_info_4_suspend);
#endif
		AICBT_DBG("%s:system halt or power off", __func__);
		break;

	default:
		break;
	}

	return NOTIFY_DONE;
}

static int btusb_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device *udev = interface_to_usbdev(intf);
	struct usb_endpoint_descriptor *ep_desc;
	u8 endpoint_num;
	struct btusb_data *data;
	struct hci_dev *hdev;
	firmware_info *fw_info;
	int i, err = 0;
	uint16_t vid = le16_to_cpu(udev->descriptor.idVendor);
	uint16_t pid = le16_to_cpu(udev->descriptor.idProduct);

	AICBT_INFO("%s: usb_interface %p, bInterfaceNumber %d, idVendor 0x%04x, "
			"idProduct 0x%04x", __func__, intf,
			intf->cur_altsetting->desc.bInterfaceNumber,
			id->idVendor, id->idProduct);

	usb_info = (uint32_t)(vid<<16) | pid;

	/* interface numbers are hardcoded in the spec */
	if (intf->cur_altsetting->desc.bInterfaceNumber != 0)
		return -ENODEV;

	AICBT_DBG("%s: can wakeup = %x, may wakeup = %x", __func__,
			device_can_wakeup(&udev->dev), device_may_wakeup(&udev->dev));

	data = aic_alloc(intf);
	if (!data)
		return -ENOMEM;

	for (i = 0; i < intf->cur_altsetting->desc.bNumEndpoints; i++) {
		ep_desc = &intf->cur_altsetting->endpoint[i].desc;

		endpoint_num = usb_endpoint_num(ep_desc);
		printk("endpoint num %d\n", endpoint_num);

		if (!data->intr_ep && usb_endpoint_is_int_in(ep_desc)) {
			data->intr_ep = ep_desc;
			continue;
		}

		if (!data->bulk_tx_ep && usb_endpoint_is_bulk_out(ep_desc)) {
			data->bulk_tx_ep = ep_desc;
			continue;
		}

		if (!data->bulk_rx_ep && usb_endpoint_is_bulk_in(ep_desc)) {
			data->bulk_rx_ep = ep_desc;
			continue;
		}
	}

	if (!data->intr_ep || !data->bulk_tx_ep || !data->bulk_rx_ep) {
		aic_free(data);
		return -ENODEV;
	}

	data->cmdreq_type = USB_TYPE_CLASS;

	data->udev = udev;
	data->intf = intf;

	dlfw_dis_state = 0;
	spin_lock_init(&queue_lock);
	spin_lock_init(&dlfw_lock);
	spin_lock_init(&data->lock);

	INIT_WORK(&data->work, btusb_work);
	INIT_WORK(&data->waker, btusb_waker);
	spin_lock_init(&data->txlock);

	init_usb_anchor(&data->tx_anchor);
	init_usb_anchor(&data->intr_anchor);
	init_usb_anchor(&data->bulk_anchor);
	init_usb_anchor(&data->isoc_anchor);
	init_usb_anchor(&data->deferred);

	fw_info = firmware_info_init(intf);
	if (fw_info)
		data->fw_info = fw_info;
	else {
		AICBT_WARN("%s: Failed to initialize fw info", __func__);
		/* Skip download patch */
		goto end;
	}

	AICBT_INFO("%s: download begining...", __func__);

#if CONFIG_BLUEDROID
	mutex_lock(&btchr_mutex);
#endif



#if CONFIG_BLUEDROID
	mutex_unlock(&btchr_mutex);
#endif

	AICBT_INFO("%s: download ending...", __func__);

	hdev = hci_alloc_dev();
	if (!hdev) {
		aic_free(data);
		data = NULL;
		return -ENOMEM;
	}

	HDEV_BUS = HCI_USB;

	data->hdev = hdev;

	SET_HCIDEV_DEV(hdev, &intf->dev);

	hdev->open     = btusb_open;
	hdev->close    = btusb_close;
	hdev->flush    = btusb_flush;
	hdev->send     = btusb_send_frame;
	hdev->notify   = btusb_notify;

#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 4, 0)
	hci_set_drvdata(hdev, data);
#else
	hdev->driver_data = data;
	hdev->destruct = btusb_destruct;
	hdev->owner = THIS_MODULE;
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 1)
	if (!reset_on_close) {
		/* set_bit(HCI_QUIRK_RESET_ON_CLOSE, &hdev->quirks); */
		AICBT_DBG("%s: Set HCI_QUIRK_RESET_ON_CLOSE", __func__);
	}
#endif

	/* Interface numbers are hardcoded in the specification */
	data->isoc = usb_ifnum_to_if(data->udev, 1);
	if (data->isoc) {
		err = usb_driver_claim_interface(&btusb_driver,
							data->isoc, data);
		if (err < 0) {
			hci_free_dev(hdev);
			hdev = NULL;
			aic_free(data);
			data = NULL;
			return err;
		}
	}

	err = hci_register_dev(hdev);
	if (err < 0) {
		hci_free_dev(hdev);
		hdev = NULL;
		aic_free(data);
		data = NULL;
		return err;
	}

	usb_set_intfdata(intf, data);

//#ifdef CONFIG_HAS_EARLYSUSPEND
#if 0
	data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	data->early_suspend.suspend = btusb_early_suspend;
	data->early_suspend.resume = btusb_late_resume;
	register_early_suspend(&data->early_suspend);
#else
	data->pm_notifier.notifier_call = bt_pm_notify;
	data->reboot_notifier.notifier_call = bt_reboot_notify;
	register_pm_notifier(&data->pm_notifier);
	register_reboot_notifier(&data->reboot_notifier);
#endif

#if CONFIG_BLUEDROID
	AICBT_INFO("%s: Check bt reset flag %d", __func__, bt_reset);
	/* Report hci hardware error after everthing is ready,
	 * especially hci register is completed. Or, btchr_poll
	 * will get null hci dev when hotplug in.
	 */
	if (bt_reset == 1) {
		hci_hardware_error();
		bt_reset = 0;
	} else
		bt_reset = 0; /* Clear and reset it anyway */
#endif

end:
	return 0;
}

static void btusb_disconnect(struct usb_interface *intf)
{
	struct btusb_data *data;
	struct hci_dev *hdev = NULL;

	wait_event_interruptible(bt_dlfw_wait, (check_set_dlfw_state_value(2) == 2));

	AICBT_INFO("%s: usb_interface %p, bInterfaceNumber %d",
			__func__, intf, intf->cur_altsetting->desc.bInterfaceNumber);

	data = usb_get_intfdata(intf);

	if (intf->cur_altsetting->desc.bInterfaceNumber != 0)
		return;

	if (data)
		hdev = data->hdev;
	else {
		AICBT_WARN("%s: Failed to get bt usb data[Null]", __func__);
		return;
	}

//#ifdef CONFIG_HAS_EARLYSUSPEND
#if 0
	unregister_early_suspend(&data->early_suspend);
#else
	unregister_pm_notifier(&data->pm_notifier);
	unregister_reboot_notifier(&data->reboot_notifier);
#endif

	firmware_info_destroy(intf);

#if CONFIG_BLUEDROID
	if (test_bit(HCI_RUNNING, &hdev->flags)) {
		AICBT_INFO("%s: Set BT reset flag", __func__);
		bt_reset = 1;
	}
#endif

	usb_set_intfdata(data->intf, NULL);

	if (data->isoc)
		usb_set_intfdata(data->isoc, NULL);

	hci_unregister_dev(hdev);

	if (intf == data->isoc)
		usb_driver_release_interface(&btusb_driver, data->intf);
	else if (data->isoc)
		usb_driver_release_interface(&btusb_driver, data->isoc);

#if !CONFIG_BLUEDROID
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 4, 0)
	__hci_dev_put(hdev);
#endif
#endif

	hci_free_dev(hdev);
	aic_free(data);
	data = NULL;
	set_dlfw_state_value(0);
}

#ifdef CONFIG_PM
static int btusb_suspend(struct usb_interface *intf, pm_message_t message)
{
	struct btusb_data *data = usb_get_intfdata(intf);
	firmware_info *fw_info = data->fw_info;

	AICBT_INFO("%s: event 0x%x, suspend count %d", __func__,
			message.event, data->suspend_count);

	if (intf->cur_altsetting->desc.bInterfaceNumber != 0)
		return 0;

	if (!test_bit(HCI_RUNNING, &data->hdev->flags))
		set_bt_onoff(fw_info, 1);

	if (data->suspend_count++)
		return 0;

	spin_lock_irq(&data->txlock);
	if (!((message.event & PM_EVENT_AUTO) && data->tx_in_flight)) {
		set_bit(BTUSB_SUSPENDING, &data->flags);
		spin_unlock_irq(&data->txlock);
	} else {
		spin_unlock_irq(&data->txlock);
		data->suspend_count--;
		AICBT_ERR("%s: Failed to enter suspend", __func__);
		return -EBUSY;
	}

	cancel_work_sync(&data->work);

	btusb_stop_traffic(data);
	mdelay(URB_CANCELING_DELAY_MS);
	usb_kill_anchored_urbs(&data->tx_anchor);

	return 0;
}

static void play_deferred(struct btusb_data *data)
{
	struct urb *urb;
	int err;

	while ((urb = usb_get_from_anchor(&data->deferred))) {
		usb_anchor_urb(urb, &data->tx_anchor);
		err = usb_submit_urb(urb, GFP_ATOMIC);
		if (err < 0) {
			AICBT_ERR("%s: Failed to submit urb %p, err %d",
					__func__, urb, err);
			kfree(urb->setup_packet);
			usb_unanchor_urb(urb);
		} else {
			usb_mark_last_busy(data->udev);
		}
		usb_free_urb(urb);

		data->tx_in_flight++;
	}
	mdelay(URB_CANCELING_DELAY_MS);
	usb_scuttle_anchored_urbs(&data->deferred);
}

static int btusb_resume(struct usb_interface *intf)
{
	struct btusb_data *data = usb_get_intfdata(intf);
	struct hci_dev *hdev = data->hdev;
	int err = 0;

	AICBT_INFO("%s: Suspend count %d", __func__, data->suspend_count);

	if (intf->cur_altsetting->desc.bInterfaceNumber != 0)
		return 0;

	if (--data->suspend_count)
		return 0;

	if (test_bit(BTUSB_BULK_RUNNING, &data->flags)) {
		err = btusb_submit_bulk_urb(hdev, GFP_NOIO);
		if (err < 0) {
			clear_bit(BTUSB_BULK_RUNNING, &data->flags);
			goto failed;
		}

		btusb_submit_bulk_urb(hdev, GFP_NOIO);
	}

	if (test_bit(BTUSB_ISOC_RUNNING, &data->flags)) {
		if (btusb_submit_isoc_urb(hdev, GFP_NOIO) < 0)
			clear_bit(BTUSB_ISOC_RUNNING, &data->flags);
		else
			btusb_submit_isoc_urb(hdev, GFP_NOIO);
	}

	spin_lock_irq(&data->txlock);
	play_deferred(data);
	clear_bit(BTUSB_SUSPENDING, &data->flags);
	spin_unlock_irq(&data->txlock);
	schedule_work(&data->work);

	return 0;

failed:
	mdelay(URB_CANCELING_DELAY_MS);
	usb_scuttle_anchored_urbs(&data->deferred);
	spin_lock_irq(&data->txlock);
	clear_bit(BTUSB_SUSPENDING, &data->flags);
	spin_unlock_irq(&data->txlock);

	return err;
}
#endif

static struct usb_driver btusb_driver = {
	.name        = "aic_btusb",
	.probe        = btusb_probe,
	.disconnect    = btusb_disconnect,
#ifdef CONFIG_PM
	.suspend    = btusb_suspend,
	.resume        = btusb_resume,
#endif
#if CONFIG_RESET_RESUME
	.reset_resume    = btusb_resume,
#endif
	.id_table    = btusb_table,
	.supports_autosuspend = 1,
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 7, 1)
	.disable_hub_initiated_lpm = 1,
#endif
};

static int __init btusb_init(void)
{
	int err;

	AICBT_INFO("AICBT_RELEASE_NAME: %s", AICBT_RELEASE_NAME);
	AICBT_INFO("Driver Release Tag: %s", DRV_RELEASE_TAG);
	AICBT_INFO("AicSemi Bluetooth USB driver module init, version %s", VERSION);
#if CONFIG_BLUEDROID
	err = btchr_init();
	if (err < 0) {
		/* usb register will go on, even bt char register failed */
		AICBT_ERR("Failed to register usb char device interfaces");
	} else
		bt_char_dev_registered = 1;
#endif
	err = usb_register(&btusb_driver);
	if (err < 0)
		AICBT_ERR("Failed to register aic bluetooth USB driver");
	return err;
}

static void __exit btusb_exit(void)
{
	AICBT_INFO("AicSemi Bluetooth USB driver module exit");
#if CONFIG_BLUEDROID
	if (bt_char_dev_registered > 0)
		btchr_exit();
#endif
	usb_deregister(&btusb_driver);
}

module_init(btusb_init);
module_exit(btusb_exit);

MODULE_AUTHOR("AicSemi Corporation");
MODULE_DESCRIPTION("AicSemi Bluetooth USB driver version");
MODULE_VERSION(VERSION);
MODULE_LICENSE("GPL");
