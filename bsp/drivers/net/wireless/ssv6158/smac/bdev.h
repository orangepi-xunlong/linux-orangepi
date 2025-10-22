/*
 * Copyright (c) 2015 iComm-semi Ltd.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _BDEV_H_
#define _BDEV_H_

#ifdef CONFIG_BLE
#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>
#if (CONFIG_BLE_HCI_BUS == SSV_BLE_HCI_OVER_SDIO)



extern int ssv_ble_hci_flush(struct hci_dev *hdev);
extern int ssv_ble_hci_open(struct hci_dev *hdev);
extern int ssv_ble_hci_close(struct hci_dev *hdev);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,13,0)
extern int ssv_ble_hci_send_frame(struct hci_dev *hdev, struct sk_buff *skb);
#else
extern int ssv_ble_hci_send_frame(struct sk_buff *skb);
#endif

#endif //#if (CONFIG_BLE_HCI_BUS == SSV_BLE_HCI_OVER_SDIO)
extern int ssv_ble_hci_set_bdaddr(struct hci_dev *hdev, const bdaddr_t *bdaddr);
extern void ssv_ble_init(struct ssv_softc *sc, u8 reset_only);
extern int ssv_ble_hci_rx_packet(u8 *rx_packet, u32 rx_len);
#endif //#ifdef CONFIG_BLE
#endif /* _BDEV_H_ */
