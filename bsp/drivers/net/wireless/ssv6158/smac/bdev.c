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

#include <linux/delay.h>
#include <linux/version.h>
#include <linux/time.h>
#include <linux/kthread.h>
#include "ssv6xxx_common.h"
#ifdef CONFIG_BLE
#if (CONFIG_BLE_HCI_BUS==SSV_BLE_HCI_OVER_SDIO)
#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>
#endif //#if (CONFIG_BLE_HCI_BUS==SSV_BLE_HCI_OVER_SDIO)
#include <ssv6200.h>
#include <hci/hctrl.h>

#include "lib.h"
#include "wow.h"
#include "dev.h"
#include "ap.h"
#include "init.h"
#include "ssv_skb.h"
#include "hw_scan.h"
#include <hal.h>
#include "bdev.h"

extern struct hci_dev *ssv_hdev;;
int ssv_ble_hci_rx_packet(u8 *rx_packet, u32 rx_len)
{
#if (CONFIG_BLE_HCI_BUS == SSV_BLE_HCI_OVER_SDIO)
	struct sk_buff *bskb=NULL;
#endif
    if(rx_packet[0]==0x04) //Packet Type
    {
        switch(rx_packet[1])
        {
            case 0x3E: //LE Event Code
                //sub event code
                switch(rx_packet[3])
                {
                    case 0x01:printk("BLE RX: LE Connection Complete. len=%d\n",rx_len); break;
                    case 0x02:printk("BLE RX: LE Advertising Report.  len=%d\n",rx_len); break;
                    case 0x03:printk("BLE RX: LE Connection Update Complete. len=%d\n",rx_len); break;
                    case 0x04:printk("BLE RX: LE Read Remote Used Features Complete. len=%d\n",rx_len); break;
                    case 0x05:printk("BLE RX: LE Long Term Key Requested. len=%d\n",rx_len); break;
                    case 0x06:printk("BLE RX: LE Remote Connection Parameter Request. len=%d\n",rx_len); break;
                    case 0x07:printk("BLE RX: LE Data Length Change. len=%d\n",rx_len); break;
                    case 0x08:printk("BLE RX: LE Read Local P256 Public Key Complete. len=%d\n",rx_len); break;
                    case 0x09:printk("BLE RX: LE Generate DHKey Complete. len=%d\n",rx_len); break;
                    case 0x0A:printk("BLE RX: LE Enhanced Connection Complete. len=%d\n",rx_len); break;
                    case 0x0B:printk("BLE RX: LE Direct Advertising Report. len=%d\n",rx_len); break;
                    default:printk("BLE RX: Unknow sub event (%d)\n",rx_packet[3]); break;
                }
                break;
            //BT Event code
            case 0x05:printk("BLE RX: Disconnection Complete. len=%d\n",rx_len); break;
            case 0x08:printk("BLE RX: Encryption Change. len=%d\n",rx_len); break;
            case 0x0C:printk("BLE RX: Read Remote Version Information Complete. len=%d\n",rx_len); break;
            case 0x0E:printk("BLE RX: Command Complete. len=%d\n",rx_len); break;
            case 0x0F:printk("BLE RX: Command Status. len=%d\n",rx_len); break;
            case 0x10:printk("BLE RX: Hardware Error. len=%d\n",rx_len); break;
            case 0x13:printk("BLE RX: Number Of Completed Packets. len=%d\n",rx_len); break;
            case 0x1A:printk("BLE RX: Data Buffer Overflow. len=%d\n",rx_len); break;
            case 0x30:printk("BLE RX: Encryption Key Refresh Complete. len=%d\n",rx_len); break;
            case 0x57:printk("BLE RX: Authenticated Payload Timeout Expired. len=%d\n",rx_len); break;
            default:printk("BLE RX: Unknow event (%d)\n",rx_packet[3]); break;
        }
    }
#if (CONFIG_BLE_HCI_BUS == SSV_BLE_HCI_OVER_SDIO)
    /* A SDIO packet is exactly containing a Bluetooth packet */
	bskb = bt_skb_alloc(rx_len, GFP_KERNEL);
	if (!bskb)
    {
		return -ENOMEM;
    }
	skb_put(bskb, rx_len);
    memcpy(bskb->data, rx_packet, rx_len);
    bt_cb(bskb)->pkt_type = rx_packet[0];
	skb_pull(bskb,1);
    //_ssv6xxx_hexdump("ble recv pack 1",rx_packet,rx_len);
    _ssv6xxx_hexdump("ble recv pack",bskb->data,bskb->len);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,13,0)
    hci_recv_frame(ssv_hdev,bskb);
#else
    bskb->dev = (void *)ssv_hdev;
    hci_recv_frame(bskb);
#endif
#endif
    return 0;

}
#if (CONFIG_BLE_HCI_BUS == SSV_BLE_HCI_OVER_SDIO)
/* ======================== BLE HCI interface ======================== */
extern void _ssv6xxx_hexdump(const char *title, const u8 *buf,
        size_t len);
static void ssv_send_ble_packet(struct ssv_softc *sc, u8 *hci_data, u32 len)
{
#if(CONFIG_BLE_HCI_BUS==1) //HCI OVER SDIO
    struct sk_buff          *skb;
    struct cfg_host_cmd     *host_cmd;

    skb = ssv_skb_alloc(sc,HOST_CMD_HDR_LEN + len);
    if(skb == NULL)
    {
        printk("%s:_skb_alloc fail!!!\n", __func__);
        return;
    }

    skb->data_len = HOST_CMD_HDR_LEN + len;
    skb->len      = skb->data_len;

    host_cmd = (struct cfg_host_cmd *)skb->data;
    memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
    host_cmd->c_type = HOST_CMD;
    host_cmd->h_cmd  = (u8)SSV6XXX_HOST_CMD_BLE_PACKET;
    host_cmd->len    = skb->data_len;

    memcpy(host_cmd->un.dat8, hci_data, len);
    //printk(" ssv6xxx_noa_hdl_bss_change  vif_idx[%d]\n",  vif_idx);
    HCI_SEND(sc->sh, skb, SSV_SW_TXQ_ID_BLE_PDU, false);
#elif (CONFIG_BLE_HCI_BUS==0)
    printk(KERN_ERR "\33[31m BLE HCI OVER UART is not implemented\33[0m\r\n");
#else
    error
#endif
}

int ssv_ble_hci_flush(struct hci_dev *hdev)
{
	struct ssv_softc *sc = hci_get_drvdata(hdev);
    printk(KERN_ERR "\33[32m%s():%d\33[0m\r\n",__FUNCTION__ ,__LINE__);
    skb_queue_purge(&sc->ble_tx_queue);
	return 0;
}


int ssv_ble_hci_open(struct hci_dev *hdev)
{
    struct ssv_softc *sc = hci_get_drvdata(hdev);
    printk(KERN_ERR "\33[32m%s():%d \33[0m\r\n",__FUNCTION__ ,__LINE__);
    HCI_BLE_START(sc->sh);
    ssv_ble_init(sc, 0);
    return 0;
}


int ssv_ble_hci_close(struct hci_dev *hdev)
{
	struct ssv_softc *sc = hci_get_drvdata(hdev);

    printk(KERN_ERR "\33[32m%s():%d \33[0m\r\n",__FUNCTION__ ,__LINE__);
    HCI_BLE_STOP(sc->sh);
    HCI_BLE_TXQ_FLUSH(sc->sh);
    skb_queue_purge(&sc->ble_tx_queue);
    return 0;
}

static u8 ble_rx_packet[512]={0};
int _ssv_ble_process_unknow_hci_opcode(u16 opcode, struct sk_buff *skb)
{
   struct ssv_softc *sc = hci_get_drvdata(ssv_hdev);
   struct sk_buff   *rskb=NULL;
   rskb = ssv_skb_alloc(sc,sizeof(ble_rx_packet));
   if(NULL==rskb)
   {
       printk("allocate skb for unknow blc hci cmd fail\n");
       return -1;
   }
   memset(ble_rx_packet,0,sizeof(ble_rx_packet));
   switch(opcode)
   {
       case 0xc14:
            ble_rx_packet[0]=0x04;
            ble_rx_packet[1]=0x0e;
            ble_rx_packet[2]=0xfc;
            ble_rx_packet[3]=0x01;
            ble_rx_packet[4]=0x14;
            ble_rx_packet[5]=0x0c;
            ble_rx_packet[6]=0x00;
            ble_rx_packet[7]=0x75;
            ble_rx_packet[8]=0x62;
            ble_rx_packet[9]=0x75;
            ble_rx_packet[10]=0x6e;
            ble_rx_packet[11]=0x74;
            ble_rx_packet[12]=0x75;
            ble_rx_packet[13]=0x2d;
            ble_rx_packet[14]=0x30;
       break;
       case 0xc23:
            ble_rx_packet[0]=0x04;
            ble_rx_packet[1]=0x0e;
            ble_rx_packet[2]=0x07;
            ble_rx_packet[3]=0x02;
            ble_rx_packet[4]=0x23;
            ble_rx_packet[5]=0x0c;
            ble_rx_packet[6]=0x00;
            ble_rx_packet[7]=0x00;
            ble_rx_packet[8]=0x01;
            ble_rx_packet[9]=0x6c;
           break;
       default:
           printk("host driver doesn't implement this unknow hci cmd\r\n");
           return -1;
    }
    rskb->len=skb->data_len = ble_rx_packet[2]+2;
    memcpy(rskb->data,&ble_rx_packet[1],ble_rx_packet[2]+2);
    bt_cb(rskb)->pkt_type = ble_rx_packet[0];
    _ssv6xxx_hexdump("fake ble hci complete!!",rskb->data,rskb->len);
    hci_recv_frame(ssv_hdev,rskb);
    return 0;
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,13,0)
int ssv_ble_hci_send_frame(struct hci_dev *hdev, struct sk_buff *skb)
#else
int ssv_ble_hci_send_frame(struct sk_buff *skb)
#endif
{
    struct ssv_softc *sc = hci_get_drvdata(ssv_hdev);
    u16 opcode=0;
    u16 unknow_opcode=0;
    if (ssv_hdev == NULL) {
        printk(KERN_ERR "\33[32mssv_ble_hci_send_frame ssv_hdev is NULL return \33[0m\r\n");
        return 0;
    }

    //printk(KERN_ERR "\33[32m%s():%d \33[0m\r\n",__FUNCTION__ ,__LINE__);
    switch (bt_cb(skb)->pkt_type) {
	case HCI_COMMAND_PKT:
		ssv_hdev->stat.cmd_tx++;
        //printk(KERN_ERR "\33[32mHCI_COMMAND_PKT \33[0m\r\n");
		break;
	case HCI_ACLDATA_PKT:
		//hdev->stat.acl_tx++;
        printk(KERN_ERR "\33[32mHCI_COMMAND_PKT \33[0m\r\n");
		break;
	case HCI_SCODATA_PKT:
		ssv_hdev->stat.sco_tx++;
        //printk(KERN_ERR "\33[32mHCI_SCODATA_PKT \33[0m\r\n");
		break;
	}
    opcode=(skb->data[0]|(skb->data[1]<<8));
    //printk("opcode=0x%x\n",opcode);
    if((bt_cb(skb)->pkt_type)==HCI_COMMAND_PKT)
    {
        switch(opcode)
        {
            case 0x0406:printk("BLE TX: Disconnect\n");break;
            case 0x041D:printk("BLE TX: Read Remote Version Information\n");break;
            case 0x0C01:printk("BLE TX: Set Event Mask\n");break;
            case 0x0C03:printk("BLE TX: Reset\n");break;
            case 0x0C2D:printk("BLE TX: Read Transmit Power Level\n");break;
            case 0x0C31:printk("BLE TX: Set Controller To Host Flow Control (optional)\n");break;
            case 0x0C33:printk("BLE TX: Host Buffer Size (optional)\n");break;
            case 0x0C35:printk("BLE TX: Host Number Of Completed Packets (optional)\n");break;
            case 0x0C63:printk("BLE TX: Set Event Mask Page\n");break;
            case 0x0C7B:printk("BLE TX: Read Authenticated Payload Timeout\n");break;
            case 0x0C7C:printk("BLE TX: Write Authenticated Payload Timeout\n");break;
            case 0x1001:printk("BLE TX: Read Local Version Information\n");break;
            case 0x1002:printk("BLE TX: Read Local Supported Commands -optional-\n");break;
            case 0x1003:printk("BLE TX: Read Local Supported Features\n");break;
            case 0x1009:printk("BLE TX: Read BD_ADDR\n");break;
            case 0x1405:printk("BLE TX: Read RSSI\n");break;
            case 0x2001:printk("BLE TX: LE Set Event Mask\n");break;
            case 0x2002:printk("BLE TX: LE Read Buffer Size\n");break;
            case 0x2003:printk("BLE TX: LE Read Local Supported Features\n");break;
            case 0x2005:printk("BLE TX: LE Set Random Address\n");break;
            case 0x2006:printk("BLE TX: LE Set Advertising Parameters\n");break;
            case 0x2007:printk("BLE TX: LE Read Advertising Channel TX Power\n");break;
            case 0x2008:printk("BLE TX: LE Set Advertising Data\n");break;
            case 0x2009:printk("BLE TX: LE Set Scan Response Data\n");break;
            case 0x200A:printk("BLE TX: LE Set Advertise Enable\n");break;
            case 0x200B:printk("BLE TX: LE Set Scan Parameters\n");break;
            case 0x200C:printk("BLE TX: LE Set Scan Enable\n");break;
            case 0x200D:printk("BLE TX: LE Create Connection\n");break;
            case 0x200E:printk("BLE TX: LE Create Connection Cancel\n");break;
            case 0x200F:printk("BLE TX: LE Read White List Size\n");break;
            case 0x2010:printk("BLE TX: LE Clear White Lis\n");break;
            case 0x2011:printk("BLE TX: LE Add Device To White List\n");break;
            case 0x2012:printk("BLE TX: LE Remove Device From White List\n");break;
            case 0x2013:printk("BLE TX: LE Connection Update\n");break;
            case 0x2014:printk("BLE TX: LE Set Host Channel Classification\n");break;
            case 0x2015:printk("BLE TX: LE Read Channel Map\n");break;
            case 0x2016:printk("BLE TX: LE Read Remote Used Features\n");break;
            case 0x2017:printk("BLE TX: LE Encrypt\n");break;
            case 0x2018:printk("BLE TX: LE Rand\n");break;
            case 0x2019:printk("BLE TX: LE Start Encryption\n");break;
            case 0x201A:printk("BLE TX: LE Long Term Key Requested Reply\n");break;
            case 0x201B:printk("BLE TX: LE Long Term Key Requested Negative Reply\n");break;
            case 0x201C:printk("BLE TX: LE Read Supported States\n");break;
            case 0x201D:printk("BLE TX: LE Receiver Test\n");break;
            case 0x201E:printk("BLE TX: LE Transmitter Test\n");break;
            case 0x201F:printk("BLE TX: LE Test End Command\n");break;
            case 0x2020:printk("BLE TX: LE Remote Connection Parameter Request Reply\n");break;
            case 0x2021:printk("BLE TX: LE Remote Connection Parameter Request Negative Reply\n");break;
            case 0x2022:printk("BLE TX: LE Set Data Length\n");break;
            case 0x2023:printk("BLE TX: LE Read Suggested Default Data Length\n");break;
            case 0x2024:printk("BLE TX: LE Write Suggested Default Data Length\n");break;
            case 0x2026:printk("BLE TX: LE Read Local P256 Public Key 37 0x2025 LE Generate DHKey\n");break;
            case 0x2027:printk("BLE TX: LE Add Device to Resolving List\n");break;
            case 0x2028:printk("BLE TX: LE Remove Device from Resolving List\n");break;
            case 0x2029:printk("BLE TX: LE Clear Resolving List\n");break;
            case 0x202A:printk("BLE TX: LE Read Resolving List Size\n");break;
            case 0x202B:printk("BLE TX: LE Read Peer Resolvable Address\n");break;
            case 0x202C:printk("BLE TX: LE Read Local Resolvable Address\n");break;
            case 0x202D:printk("BLE TX: LE Set Address Resolution Enable\n");break;
            case 0x202E:printk("BLE TX: LE Set Resolvable Private Address Timeout\n");break;
            case 0x202F:printk("BLE TX: LE Read Maximum Data Length\n");break;
            default:
                //printk("BLE TX: unknow hci cmd 0x%x\n",opcode);
                unknow_opcode++;
                break;
        }
    }
    if(0!=unknow_opcode)
    {
        _ssv_ble_process_unknow_hci_opcode(opcode,skb);
        goto END;
    }

    /* Prepend skb with frame type */
    memcpy(skb_push(skb, 1), &bt_cb(skb)->pkt_type, 1);

    _ssv6xxx_hexdump("ble send frame",skb->data,skb->len);
    ssv_send_ble_packet(sc,skb->data,skb->len);
END:
    skb_queue_tail(&sc->ble_tx_queue, skb);
    return 0;
}

#define VENDOR_SPECIFIC_OP 0x3f
#define HCI_SET_BD_ADDR_OP 0x7
int ssv_ble_hci_set_bdaddr(struct hci_dev *hdev, const bdaddr_t *bdaddr)
{
    u16 opcode = 0;
    u8 cmd[10] = {0};
    u8 len = 0;

    struct ssv_softc *sc = hci_get_drvdata(ssv_hdev);

    if (hdev == NULL || bdaddr == NULL || ssv_hdev == NULL)
        return -1;

    printk(KERN_ERR "\33[32m%s():%d \33[0m\r\n",__FUNCTION__ ,__LINE__);
    opcode = 0;
    len = 6;
    opcode = ((VENDOR_SPECIFIC_OP << 10) | HCI_SET_BD_ADDR_OP);

    cmd[0] = 1;
    memcpy(&cmd[1], &opcode, sizeof(opcode));
    memcpy(&cmd[1] + sizeof(opcode), &len, sizeof(len));
    memcpy(&cmd[1] + sizeof(opcode) + sizeof(len), bdaddr, 6);
    printk("set ble addr - %pM\n", bdaddr);
    ssv_send_ble_packet(sc, cmd, sizeof(cmd));
    return 0;
}
#endif//#if (CONFIG_BLE_HCI_BUS == SSV_BLE_HCI_OVER_SDIO)

void ssv_ble_init(struct ssv_softc *sc, u8 reset_only)
{
    struct sk_buff          *skb=NULL;
    struct cfg_host_cmd     *host_cmd;
    struct ssv6xxx_ble_config ble_config;
    int len=0;

    len=sizeof(struct ssv6xxx_ble_config);
    memset(&ble_config,0,sizeof(struct ssv6xxx_ble_config));
#if(CONFIG_BLE_HCI_BUS == SSV_BLE_HCI_OVER_SDIO)
    printk(KERN_ERR"Config BLE HCI over SDIO\n");
    ble_config.bus=SSV_BLE_HCI_OVER_SDIO;
#elif(CONFIG_BLE_HCI_BUS == SSV_BLE_HCI_OVER_UART)
    printk(KERN_ERR"Config BLE HCI over UART\n");
    ble_config.bus=SSV_BLE_HCI_OVER_UART;
#else
    error!
#endif

    if (reset_only) {
        printk(KERN_ERR"send hci reset\n");
        ble_config.hci_reset = 1;
    }

    ble_config.dtm=sc->sh->cfg.ble_dtm;

    memcpy(ble_config.bdaddr, &sc->sh->cfg.maddr[1][0], 6);
    ble_config.replace_scan_interval = sc->sh->cfg.ble_replace_scan_interval;
    ble_config.replace_scan_win =  sc->sh->cfg.ble_replace_scan_win;

    skb = ssv_skb_alloc(sc,HOST_CMD_HDR_LEN + len);
    if(skb == NULL)
    {
        printk("%s:_skb_alloc fail!!!\n", __func__);
        return;
    }

    skb->data_len = HOST_CMD_HDR_LEN + len;
    skb->len      = skb->data_len;

    host_cmd = (struct cfg_host_cmd *)skb->data;
    memset(host_cmd, 0x0, sizeof(struct cfg_host_cmd));
    host_cmd->c_type = HOST_CMD;
    host_cmd->h_cmd  = (u8)SSV6XXX_HOST_CMD_BLE_INIT;
    host_cmd->len    = skb->data_len;

    memcpy(host_cmd->un.dat8, &ble_config, len);
    //printk(" ssv6xxx_noa_hdl_bss_change  vif_idx[%d]\n",  vif_idx);
    HCI_SEND(sc->sh, skb, SSV_SW_TXQ_ID_BLE_PDU, false);
}

#endif//#ifdef CONFIG_BLE
