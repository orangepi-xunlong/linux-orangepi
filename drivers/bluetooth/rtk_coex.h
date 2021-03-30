#include <net/bluetooth/hci_core.h>
#include <linux/list.h>
#include <linux/version.h>

#if 1
#define RTKBT_DBG(fmt, arg...) printk(KERN_INFO "rtk_btcoex: " fmt "\n" , ## arg)
#else
#define RTKBT_DBG(fmt, arg...)
#endif

#if 1
#define RTKBT_ERR(fmt, arg...) printk(KERN_ERR "rtk_btcoex: " fmt "\n" , ## arg)
#else
#define RTKBT_ERR(fmt, arg...)
#endif

/***********************************
** Realtek - For coexistence **
***********************************/
#define TRUE                1
#define FALSE               0

#define CONNECT_PORT        30001
#define CONNECT_PORT_WIFI   30000

#define invite_req          "INVITE_REQ"
#define invite_rsp          "INVITE_RSP"
#define attend_req          "ATTEND_REQ"
#define attend_ack          "ATTEND_ACK"
#define wifi_leave          "WIFI_LEAVE"
#define leave_ack           "LEAVE_ACK"
#define bt_leave            "BT_LEAVE"

#define HCI_OP_PERIODIC_INQ								0x0403
#define HCI_EV_LE_META			                        0x3e
#define HCI_EV_LE_CONN_COMPLETE		                    0x01
#define HCI_EV_LE_CONN_UPDATE_COMPLETE	                0x03

//vendor cmd to fw
#define HCI_VENDOR_ENABLE_PROFILE_REPORT_COMMAND        0xfc18
#define HCI_VENDOR_SET_PROFILE_REPORT_COMMAND           0xfc19
#define HCI_VENDOR_MAILBOX_CMD                          0xfc8f

//subcmd to fw
#define HCI_VENDOR_SUB_CMD_WIFI_CHANNEL_AND_BANDWIDTH_CMD   0x11
#define HCI_VENDOR_SUB_CMD_WIFI_FORCE_TX_POWER_CMD          0x17
#define HCI_VENDOR_SUB_CMD_BT_ENABLE_IGNORE_WLAN_ACT_CMD    0x1B
#define HCI_VENDOR_SUB_CMD_BT_REPORT_CONN_SCO_INQ_INFO      0x23
#define HCI_VENDOR_SUB_CMD_BT_AUTO_REPORT_STATUS_INFO       0x27
#define HCI_VENDOR_SUB_CMD_BT_AUTO_REPORT_ENABLE            0x28
#define HCI_VENDOR_SUB_CMD_BT_SET_TXRETRY_REPORT_PARAM      0x29
#define HCI_VENDOR_SUB_CMD_BT_SET_PTATABLE                  0x2A
#define HCI_VENDOR_SUB_CMD_SET_BT_PSD_MODE                  0x31
#define HCI_VENDOR_SUB_CMD_SET_BT_LNA_CONSTRAINT            0x32
#define HCI_VENDOR_SUB_CMD_GET_AFH_MAP_L                    0x40
#define HCI_VENDOR_SUB_CMD_GET_AFH_MAP_M                    0x41
#define HCI_VENDOR_SUB_CMD_GET_AFH_MAP_H                    0x42
#define HCI_VENDOR_SUB_CMD_RD_REG_REQ                       0x43
#define HCI_VENDOR_SUB_CMD_WR_REG_REQ                       0x44

#define HCI_EV_VENDOR_SPECIFIC      0xff

//sub event from fw start
#define HCI_VENDOR_PTA_REPORT_EVENT         0x24
#define HCI_VENDOR_PTA_AUTO_REPORT_EVENT    0x25

//vendor cmd to wifi driver
#define HCI_GRP_VENDOR_SPECIFIC             (0x3f << 10)
#define HCI_OP_HCI_EXTENSION_VERSION_NOTIFY (0x0100 | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_OP_BT_OPERATION_NOTIFY          (0x0102 | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_OP_HCI_BT_INFO_NOTIFY           (0x0106 | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_OP_HCI_BT_COEX_NOTIFY           (0x0107 | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_OP_HCI_BT_PATCH_VER_NOTIFY      (0x0108 | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_OP_HCI_BT_AFH_MAP_NOTIFY        (0x0109 | HCI_GRP_VENDOR_SPECIFIC)
#define HCI_OP_HCI_BT_REGISTER_VALUE_NOTIFY (0x010a | HCI_GRP_VENDOR_SPECIFIC)

//bt info reason to wifi
#define HOST_RESPONSE                   0 //Host response when receive the BT Info Control Event
#define POLLING_RESPONSE                1 //The BT Info response for polling by BT firmware.
#define AUTO_REPORT                     2 //BT auto report by BT firmware.
#define STACK_REPORT_WHILE_DEVICE_D2    3 //Stack report when BT firmware is under power save state(ex:D2)

// vendor event from wifi
#define RTK_HS_EXTENSION_EVENT_WIFI_SCAN            0x01
#define RTK_HS_EXTENSION_EVENT_RADIO_STATUS_NOTIFY  0x02
#define RTK_HS_EXTENSION_EVENT_HCI_BT_INFO_CONTROL  0x03
#define RTK_HS_EXTENSION_EVENT_HCI_BT_COEX_CONTROL  0x04

//op code from wifi
#define BT_PATCH_VERSION_QUERY      0x00
#define IGNORE_WLAN_ACTIVE_CONTROL  0x01
#define LNA_CONSTRAIN_CONTROL       0x02
#define BT_POWER_DECREASE_CONTROL   0x03
#define BT_PSD_MODE_CONTROL         0x04
#define WIFI_BW_CHNL_NOTIFY         0x05
#define QUERY_BT_AFH_MAP            0x06
#define BT_REGISTER_ACCESS          0x07

//bt operation to notify
#define BT_OPCODE_NONE                  0
#define BT_OPCODE_INQUIRY_START         1
#define BT_OPCODE_INQUIRY_END           2
#define BT_OPCODE_PAGE_START            3
#define BT_OPCODE_PAGE_SUCCESS_END      4
#define BT_OPCODE_PAGE_UNSUCCESS_END    5
#define BT_OPCODE_PAIR_START            6
#define BT_OPCODE_PAIR_END              7
#define BT_OPCODE_ENABLE_BT             8
#define BT_OPCODE_DISABLE_BT            9

#define HCI_EXTENSION_VERSION           0x0004
#define HCI_CMD_PREAMBLE_SIZE           3
#define PAN_PACKET_COUNT                5

#define STREAM_TO_UINT16(u16, p) {u16 = ((uint16_t)(*(p)) + (((uint16_t)(*((p) + 1))) << 8)); (p) += 2;}
#define UINT16_TO_STREAM(p, u16) {*(p)++ = (uint8_t)(u16); *(p)++ = (uint8_t)((u16) >> 8);}

#define PSM_SDP     0x0001
#define PSM_RFCOMM  0x0003
#define PSM_PAN     0x000F
#define PSM_HID     0x0011
#define PSM_HID_INT 0x0013
#define PSM_AVCTP   0x0017
#define PSM_AVDTP   0x0019
#define PSM_FTP     0x1001
#define PSM_BIP     0x1003
#define PSM_OPP     0x1015
//--add more if needed--//

enum {
    profile_sco = 0,
    profile_hid = 1,
    profile_a2dp = 2,
    profile_pan = 3,
    profile_hid_interval = 4,
    profile_hogp = 5,
    profile_voice = 6,
    profile_max = 7
};

//profile info data
typedef struct {
    struct list_head  	list;
    uint16_t       		handle;
    uint16_t       		psm;
    uint16_t       		dcid;
    uint16_t       		scid;
    uint8_t             profile_index;
}rtk_prof_info, *prtk_prof_info;

//profile info for each connection
typedef struct {
    struct list_head list;
    uint16_t handle;
    uint8_t type; // 0:l2cap, 1:sco/esco, 2:le
    uint8_t profile_bitmap;
    int8_t  profile_refcount[8];
}rtk_conn_prof, *prtk_conn_prof;

typedef struct {
    uint8_t     cmd_index;
    uint8_t     cmd_length;
    uint8_t     link_status;
    uint8_t     retry_cnt;
    uint8_t     rssi;
    uint8_t     mailbox_info;
    uint16_t    acl_throughput;
}hci_linkstatus_report;

typedef struct {
    uint8_t  type;
    uint32_t offset;
    uint32_t value;
}hci_mailbox_register;

typedef struct {
    uint8_t     polling_enable;
    uint8_t     polling_time;
    uint8_t     autoreport_enable;
}hci_bt_info_control;

typedef struct {
    struct list_head        conn_hash; //hash for connections
    struct list_head        profile_list; //hash for profile info
    struct hci_dev          *hdev;
    struct socket           *udpsock;
    struct sockaddr_in      addr;
    struct sockaddr_in      wifi_addr;
    struct timer_list       polling_timer;
    struct timer_list       a2dp_count_timer;
    struct timer_list       pan_count_timer;
    struct timer_list       hogp_count_timer;
    struct sk_buff_head     cmd_q;
    struct tasklet_struct   cmd_task;
    struct workqueue_struct *sock_wq;
    struct workqueue_struct *fw_wq;
    struct delayed_work     sock_work;
    struct delayed_work     fw_work;
    struct sock             *sk;
    spinlock_t              spin_lock_sock;
    spinlock_t              spin_lock_profile;

    uint8_t                 *event_data;
    uint32_t                a2dp_packet_count;
    uint32_t                pan_packet_count;
    uint32_t                hogp_packet_count;
    uint32_t                voice_packet_count;
    uint8_t                 profile_bitmap;
    uint8_t                 profile_status;
    int8_t                  profile_refcount[8];
    uint8_t                 ispairing;
    uint8_t                 isinquirying;
    uint8_t                 ispaging;
    uint8_t                 wifi_state;
    uint8_t                 autoreport;
    uint8_t                 polling_enable;
    uint8_t                 polling_interval;
    uint8_t                 piconet_id;
    uint8_t                 mode;
    uint8_t                 afh_map[10];
    uint16_t                hci_reversion;
    uint16_t                lmp_subversion;
    uint8_t                 wifi_on;
    uint8_t                 sock_open;
    uint8_t                 num_hci_cmd_packet;
    uint16_t                last_send_cmd;
    unsigned long           cmd_last_tx;
}rtk_uart_coexistence_info;

void rtk_uart_parse_event(struct sk_buff *skb);
void rtk_uart_parse_cmd(struct sk_buff *skb);
void rtk_uart_parse_l2cap_data_tx(struct sk_buff *skb);
void rtk_uart_parse_l2cap_data_rx(struct sk_buff *skb);
void rtk_uart_parse_recv_data(u8 *p);

void rtk_uart_coex_open(struct hci_dev *hdev);
void rtk_uart_coex_close(void);
void rtk_uart_coex_probe(struct hci_dev *hdev);
void rtk_uart_coex_init(void);
void rtk_uart_coex_exit(void);
