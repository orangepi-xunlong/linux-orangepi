/**
 * aicwf_usb.h
 *
 * USB function declarations
 *
 * Copyright (C) AICSemi 2018-2020
 */

#ifndef _AICWF_USB_H_
#define _AICWF_USB_H_

#include <linux/usb.h>
#include "rwnx_cmds.h"

#ifdef AICWF_USB_SUPPORT
extern struct device_match_entry *aic_matched_ic;
extern struct aicbsp_feature_t aicwf_feature;

/* USB Device ID */
#define USB_VENDOR_ID_AIC               0xA69C
#define USB_VENDOR_ID_AIC_V2            0x368B

#define USB_DEVICE_ID_AIC_8800          0x8800
#define USB_DEVICE_ID_AIC_8801          0x8801

#define USB_DEVICE_ID_AIC_8800D80       0x8D80
#define USB_DEVICE_ID_AIC_8800D81       0x8D81
#define USB_DEVICE_ID_AIC_8800D40       0x8D40
#define USB_DEVICE_ID_AIC_8800D41       0x8D41

#define USB_DEVICE_ID_AIC_8800D80X2     0x8D90
#define USB_DEVICE_ID_AIC_8800D81X2     0x8D91

#define AICWF_USB_RX_URBS               (20)//(200)
#ifdef CONFIG_USB_MSG_IN_EP
#define AICWF_USB_MSG_RX_URBS           (100)
#endif
#define AICWF_USB_TX_URBS               (200)//(100)
#define AICWF_USB_TX_LOW_WATER          (AICWF_USB_TX_URBS/4)
#define AICWF_USB_TX_HIGH_WATER         (AICWF_USB_TX_LOW_WATER*3)
#define AICWF_USB_AGGR_MAX_PKT_SIZE     (2048*10)
#define AICWF_USB_MSG_MAX_PKT_SIZE      (2048)
#define AICWF_USB_MAX_PKT_SIZE          (2048)

typedef enum {
	USB_TYPE_DATA         = 0X00,
	USB_TYPE_CFG          = 0X10,
	USB_TYPE_CFG_CMD_RSP  = 0X11,
	USB_TYPE_CFG_DATA_CFM = 0X12,
	USB_TYPE_CFG_PRINT    = 0X13
} usb_type;

enum aicwf_usb_state {
	USB_DOWN_ST,
	USB_UP_ST,
	USB_SLEEP_ST
};

struct aicwf_usb_buf {
	struct list_head list;
	struct aic_usb_dev *usbdev;
	struct urb *urb;
	struct sk_buff *skb;
	bool cfm;
	u8* usb_align_data;
};

struct aic_usb_dev {
	struct rwnx_hw *rwnx_hw;
	struct aicwf_bus *bus_if;
	struct usb_device *udev;
	struct device *dev;
	struct aicwf_rx_priv *rx_priv;
	enum aicwf_usb_state state;
	struct rwnx_cmd_mgr cmd_mgr;

	struct usb_anchor rx_submitted;
	struct work_struct rx_urb_work;
#ifdef CONFIG_USB_MSG_IN_EP
	struct usb_anchor msg_rx_submitted;
	struct work_struct msg_rx_urb_work;
#endif

	spinlock_t rx_free_lock;
	spinlock_t tx_free_lock;
	spinlock_t tx_post_lock;
	spinlock_t tx_flow_lock;
#ifdef CONFIG_USB_MSG_IN_EP
	spinlock_t msg_rx_free_lock;
#endif

	struct list_head rx_free_list;
	struct list_head tx_free_list;
	struct list_head tx_post_list;
#ifdef CONFIG_USB_MSG_IN_EP
	struct list_head msg_rx_free_list;
#endif

	uint bulk_in_pipe;
	uint bulk_out_pipe;
#ifdef CONFIG_USB_MSG_OUT_EP
    uint msg_out_pipe;
#endif
#ifdef CONFIG_USB_MSG_IN_EP
	uint msg_in_pipe;
#endif


	int tx_free_count;
	int tx_post_count;
	bool rx_prepare_ready;

#if 0
	struct aicwf_usb_buf usb_tx_buf[AICWF_USB_TX_URBS];
	struct aicwf_usb_buf usb_rx_buf[AICWF_USB_RX_URBS];
#else
	struct aicwf_usb_buf *usb_tx_buf;
	struct aicwf_usb_buf *usb_rx_buf;
#endif

#ifdef CONFIG_USB_MSG_IN_EP
	struct aicwf_usb_buf usb_msg_rx_buf[AICWF_USB_MSG_RX_URBS];
#endif

	int msg_finished;
	wait_queue_head_t msg_wait;
	ulong msg_busy;
	struct urb *msg_out_urb;

	bool tbusy;
};

extern void aicwf_usb_exit(void);
extern void aicwf_usb_register(void);
extern void aicwf_usb_tx_flowctrl(struct rwnx_hw *rwnx_hw, bool state);
#ifdef CONFIG_USB_MSG_IN_EP
int usb_msg_busrx_thread(void *data);
#endif
int usb_bustx_thread(void *data);
int usb_busrx_thread(void *data);
extern void aicwf_hostif_ready(void);

void aicwf_usb_rx_submit_all_urb_(struct aic_usb_dev *usb_dev);
#ifdef CONFIG_USB_MSG_IN_EP
void aicwf_usb_msg_rx_submit_all_urb_(struct aic_usb_dev *usb_dev);
#endif


#endif /* AICWF_USB_SUPPORT */
#endif /* _AICWF_USB_H_       */
