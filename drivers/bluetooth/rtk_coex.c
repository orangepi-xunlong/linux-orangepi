#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/skbuff.h>
#include <linux/usb.h>
#include <linux/dcache.h>
#include <net/sock.h>
#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>
#include <net/bluetooth/l2cap.h>

#include "hci_uart.h"
#include "rtk_coex.h"

#define RTK_VERSION "1.1"

rtk_uart_coexistence_info uart_coex_info;

#define is_profile_connected(profile)   ((uart_coex_info.profile_bitmap & BIT(profile)) >0)
#define is_profile_busy(profile)        ((uart_coex_info.profile_status & BIT(profile)) >0)

static void rtk_handle_event_from_wifi(uint8_t* msg);
static void count_a2dp_packet_timeout(unsigned long data);
static void count_pan_packet_timeout(unsigned long data);
static void count_hogp_packet_timeout(unsigned long data);

static int8_t psm_to_profile_index(uint16_t psm)
{
	switch (psm) {
	case PSM_AVCTP:
	case PSM_SDP:
		return -1;   //ignore

	case PSM_HID:
	case PSM_HID_INT:
		return profile_hid;

	case PSM_AVDTP:
		return profile_a2dp;

	case PSM_PAN:
	case PSM_OPP:
	case PSM_FTP:
	case PSM_BIP:
	case PSM_RFCOMM:
		return profile_pan;

	default:
		return profile_pan;
	}
}

static rtk_conn_prof* find_connection_by_handle(rtk_uart_coexistence_info * coex, uint16_t handle)
{
	struct list_head *head = &coex->conn_hash;
	struct list_head *iter = NULL, *temp = NULL;
	rtk_conn_prof* desc = NULL;

	list_for_each_safe(iter, temp, head) {
		desc = list_entry(iter, rtk_conn_prof, list);
		if ((handle & 0xEFF) == desc->handle ) {
			return desc;
		}
	}
	return NULL;
}

static rtk_conn_prof* allocate_connection_by_handle(uint16_t handle)
{
	rtk_conn_prof* phci_conn = NULL;
	phci_conn = kmalloc(sizeof(rtk_conn_prof), GFP_ATOMIC);
	if(phci_conn)
		phci_conn->handle = handle;

	return phci_conn;
}

static void init_connection_hash(rtk_uart_coexistence_info* coex)
{
	struct list_head* head = &coex->conn_hash;
	INIT_LIST_HEAD(head);
}

static void add_connection_to_hash(rtk_uart_coexistence_info* coex, rtk_conn_prof* desc)
{
	struct list_head* head = &coex->conn_hash;
	list_add_tail(&desc->list, head);
}

static void delete_connection_from_hash(rtk_conn_prof* desc)
{
	if (desc) {
		list_del(&desc->list);
        kfree(desc);
    }
}

static void flush_connection_hash(rtk_uart_coexistence_info* coex)
{
	struct list_head* head = &coex->conn_hash;
	struct list_head* iter = NULL, *temp = NULL;
	rtk_conn_prof* desc = NULL;

	list_for_each_safe(iter, temp, head) {
		desc = list_entry(iter, rtk_conn_prof, list);
		if (desc) {
			list_del(&desc->list);
			kfree(desc);
		}
	}
	//INIT_LIST_HEAD(head);
}

static void init_profile_hash(rtk_uart_coexistence_info* coex)
{
	struct list_head* head = &coex->profile_list;
	INIT_LIST_HEAD(head);
}

static uint8_t list_allocate_add(uint16_t handle, uint16_t psm, int8_t profile_index, uint16_t dcid, uint16_t scid)
{
    rtk_prof_info *pprof_info = NULL;

	if(profile_index < 0) {
		RTKBT_ERR("PSM(0x%x) do not need parse", psm);
		return FALSE;
	}

	pprof_info = kmalloc(sizeof(rtk_prof_info), GFP_ATOMIC);

	if (NULL == pprof_info) {
		RTKBT_ERR("list_allocate_add: allocate error");
		return FALSE;
	}

	pprof_info->handle = handle;
	pprof_info->psm = psm;
	pprof_info->scid = scid;
	pprof_info->dcid = dcid;
	pprof_info->profile_index = profile_index;
	list_add_tail(&(pprof_info->list), &(uart_coex_info.profile_list));

	return TRUE;
}

static void delete_profile_from_hash(rtk_prof_info* desc)
{
    RTKBT_DBG("delete profile for handle: %x, psm:%x, dcid:%x, scid:%x", desc->handle, desc->psm, desc->dcid, desc->scid);
	if (desc) {
		list_del(&desc->list);
		kfree(desc);
		desc = NULL;
	}
}

static void flush_profile_hash(rtk_uart_coexistence_info* coex)
{
	struct list_head* head = &coex->profile_list;
	struct list_head* iter = NULL, *temp = NULL;
	rtk_prof_info* desc = NULL;

	spin_lock(&uart_coex_info.spin_lock_profile);
	list_for_each_safe(iter, temp, head) {
		desc = list_entry(iter, rtk_prof_info, list);
		delete_profile_from_hash(desc);
	}
	//INIT_LIST_HEAD(head);
    spin_unlock(&uart_coex_info.spin_lock_profile);
}

static rtk_prof_info* find_profile_by_handle_scid(rtk_uart_coexistence_info* coex, uint16_t handle, uint16_t scid)
{
	struct list_head* head = &coex->profile_list;
	struct list_head* iter = NULL, *temp = NULL;
	rtk_prof_info* desc = NULL;

	list_for_each_safe(iter, temp, head) {
		desc = list_entry(iter, rtk_prof_info, list);
		if (((handle & 0xFFF) == desc->handle ) && (scid == desc->scid)) {
			return desc;
		}
	}
	return NULL;
}

static rtk_prof_info* find_profile_by_handle_dcid(rtk_uart_coexistence_info* coex, uint16_t handle, uint16_t dcid)
{
	struct list_head* head = &coex->profile_list;
	struct list_head* iter = NULL, *temp = NULL;
	rtk_prof_info* desc = NULL;

	list_for_each_safe(iter, temp, head) {
		desc = list_entry(iter, rtk_prof_info, list);
		if (((handle & 0xFFF) == desc->handle ) && (dcid == desc->dcid)) {
			return desc;
		}
	}
	return NULL;
}

static rtk_prof_info* find_profile_by_handle_dcid_scid(rtk_uart_coexistence_info* coex, uint16_t handle, uint16_t dcid, uint16_t scid)
{
	struct list_head* head = &coex->profile_list;
	struct list_head* iter = NULL, *temp = NULL;
	rtk_prof_info* desc = NULL;

	list_for_each_safe(iter, temp, head) {
		desc = list_entry(iter, rtk_prof_info, list);
		if (((handle & 0xFFF) == desc->handle ) && (dcid == desc->dcid) && (scid == desc->scid)) {
			return desc;
		}
	}
	return NULL;
}

static void hci_cmd_task(unsigned long arg)
{
    uint8_t *p;
    struct sk_buff *skb;

    RTKBT_DBG("in hci_cmd_task, uart_coex_info.num_hci_cmd_packet is %d", uart_coex_info.num_hci_cmd_packet);

    if ((!uart_coex_info.num_hci_cmd_packet) && (time_after(jiffies, uart_coex_info.cmd_last_tx + HZ))) {
        RTKBT_ERR("command timeout");
        uart_coex_info.num_hci_cmd_packet = 1;
    }

    if((uart_coex_info.num_hci_cmd_packet) && (skb = skb_dequeue(&uart_coex_info.cmd_q))) {
        p = (uint8_t *)(skb->data);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0)
        hci_uart_send_frame(uart_coex_info.hdev, skb);
#else
        hci_uart_send_frame(skb);
#endif
        uart_coex_info.last_send_cmd = *p + (*(p+1) << 8);
        RTKBT_DBG("send cmd to fw, opcode = 0x%x", uart_coex_info.last_send_cmd);
        uart_coex_info.num_hci_cmd_packet = 0;
        uart_coex_info.cmd_last_tx = jiffies;
    }
}

static void rtk_vendor_cmd_to_fw(uint16_t opcode, uint8_t parameter_len, uint8_t* parameter)
{
    int len = HCI_CMD_PREAMBLE_SIZE + parameter_len;
    uint8_t *p;
    struct sk_buff *skb;

    skb = bt_skb_alloc(len, GFP_ATOMIC);
    if(!skb) {
        RTKBT_DBG("there is no room for cmd 0x%x", opcode);
        return;
    }

    p = (uint8_t *)skb_put(skb, HCI_CMD_PREAMBLE_SIZE);
    UINT16_TO_STREAM(p, opcode);
    *p++ = parameter_len;

    if(parameter_len)
        memcpy(skb_put(skb, parameter_len), parameter, parameter_len);

    bt_cb(skb)->pkt_type = HCI_COMMAND_PKT;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 13, 0)
    skb->dev = (void *)uart_coex_info.hdev;
#endif
    RTKBT_DBG("rtk_vendor_cmd_to_fw, opcode is 0x%x", opcode);
    skb_queue_tail(&uart_coex_info.cmd_q, skb);
    tasklet_schedule(&uart_coex_info.cmd_task);

    return;
}

static void rtk_notify_profileinfo_to_fw(void)
{
	struct list_head* head = NULL;
	struct list_head* iter = NULL;
	struct list_head* temp = NULL;
	rtk_conn_prof* hci_conn = NULL;
	uint8_t  handle_number = 0;
	uint32_t buffer_size = 0;
    uint8_t *p_buf = NULL;
	uint8_t *p = NULL;

	head = &uart_coex_info.conn_hash;
	list_for_each_safe(iter, temp, head) {
		hci_conn = list_entry(iter, rtk_conn_prof, list);
		if (hci_conn && hci_conn->profile_bitmap)
			handle_number++;
	}

	buffer_size = 1 + handle_number*3 + 1; 

	p_buf = kmalloc(buffer_size, GFP_ATOMIC);

    if(NULL == p_buf) {
		RTKBT_ERR("rtk_notify_profileinfo_to_fw: alloc error");
		return;
	}
    p = p_buf;

    RTKBT_DBG("rtk_notify_profileinfo_to_fw, BufferSize is %x", buffer_size);
	*p++ = handle_number;
	RTKBT_DBG("rtk_notify_profileinfo_to_fw, NumberOfHandles is %x", handle_number);
    head = &uart_coex_info.conn_hash;
	list_for_each(iter, head) {
		hci_conn = list_entry(iter, rtk_conn_prof, list);
		if (hci_conn && hci_conn->profile_bitmap) {
			UINT16_TO_STREAM(p, hci_conn->handle);
			RTKBT_DBG("rtk_notify_profileinfo_to_fw, handle is %x",hci_conn->handle);
			*p++ = hci_conn->profile_bitmap;
			RTKBT_DBG("rtk_notify_profileinfo_to_fw, profile_bitmap is %x",hci_conn->profile_bitmap);
			handle_number --;
		}
		if(0 == handle_number)
			break;
	}

	*p++ = uart_coex_info.profile_status;
	RTKBT_DBG("rtk_notify_profileinfo_to_fw, profile_status is %x", uart_coex_info.profile_status);

    rtk_vendor_cmd_to_fw(HCI_VENDOR_SET_PROFILE_REPORT_COMMAND, buffer_size, p_buf);

	kfree(p_buf);
	return ;
}

static void rtk_check_setup_timer(int8_t profile_index)
{
    if(profile_index == profile_a2dp) {
        uart_coex_info.a2dp_packet_count = 0;
        setup_timer(&(uart_coex_info.a2dp_count_timer), count_a2dp_packet_timeout, 0);
        uart_coex_info.a2dp_count_timer.expires = jiffies + msecs_to_jiffies(1000);
        add_timer(&(uart_coex_info.a2dp_count_timer));
    }

    if(profile_index == profile_pan) {
        uart_coex_info.pan_packet_count = 0;
        setup_timer(&(uart_coex_info.pan_count_timer), count_pan_packet_timeout, 0);
        uart_coex_info.pan_count_timer.expires = jiffies + msecs_to_jiffies(1000);
        add_timer(&(uart_coex_info.pan_count_timer));
    }

    /* hogp & voice share one timer now */
    if((profile_index == profile_hogp) || (profile_index == profile_voice)) {
        if((0 == uart_coex_info.profile_refcount[profile_hogp]) 
                && (0 == uart_coex_info.profile_refcount[profile_voice])) {
            uart_coex_info.hogp_packet_count = 0;
            uart_coex_info.voice_packet_count = 0;
            setup_timer(&(uart_coex_info.hogp_count_timer), count_hogp_packet_timeout, 0);
            uart_coex_info.hogp_count_timer.expires = jiffies + msecs_to_jiffies(1000);
            add_timer(&(uart_coex_info.hogp_count_timer));
        }
    }
}

static void rtk_check_del_timer(int8_t profile_index)
{
    if(profile_a2dp == profile_index) {
        uart_coex_info.a2dp_packet_count = 0;
        del_timer(&(uart_coex_info.a2dp_count_timer));
    }
    if(profile_pan == profile_index) {
        uart_coex_info.pan_packet_count = 0;
        del_timer(&(uart_coex_info.pan_count_timer));
    }
    if(profile_hogp == profile_index) {
        uart_coex_info.hogp_packet_count = 0;
        if(uart_coex_info.profile_refcount[profile_voice] == 0) {
            del_timer(&(uart_coex_info.hogp_count_timer));
        }
    }
    if(profile_voice == profile_index) {
        uart_coex_info.voice_packet_count = 0;
        if(uart_coex_info.profile_refcount[profile_hogp] == 0) {
            del_timer(&(uart_coex_info.hogp_count_timer));
        }
    }
}

static void update_profile_state(uint8_t profile_index, uint8_t is_busy)
{
	uint8_t need_update = FALSE;

	if((uart_coex_info.profile_bitmap & BIT(profile_index)) == 0) {
		RTKBT_ERR("update_profile_state: ERROR!!! profile(Index: %x) does not exist", profile_index);
		return;
	}

	if(is_busy) {
		if((uart_coex_info.profile_status & BIT(profile_index)) == 0) {
			need_update = TRUE;
			uart_coex_info.profile_status |= BIT(profile_index);
		}
	} else {
		if((uart_coex_info.profile_status & BIT(profile_index)) > 0) {
			need_update = TRUE;
			uart_coex_info.profile_status &= ~(BIT(profile_index));
		}
	}

	if(need_update) {
		RTKBT_DBG("update_profile_state, uart_coex_info.profie_bitmap = %x", uart_coex_info.profile_bitmap);
		RTKBT_DBG("update_profile_state, uart_coex_info.profile_status = %x", uart_coex_info.profile_status);
		rtk_notify_profileinfo_to_fw();
	}
}

static void update_profile_connection(rtk_conn_prof* phci_conn, int8_t profile_index, uint8_t is_add)
{
	uint8_t need_update = FALSE;
    uint8_t kk;

	RTKBT_DBG("update_profile_connection: is_add=%d, profile_index=%x", is_add, profile_index);
    if (profile_index < 0)
        return;

	if(is_add) {
		if(uart_coex_info.profile_refcount[profile_index] == 0) {
			need_update = TRUE;
			uart_coex_info.profile_bitmap |= BIT(profile_index);

			/* SCO is always busy */
			if(profile_index == profile_sco)
			    uart_coex_info.profile_status |= BIT(profile_index);

            rtk_check_setup_timer(profile_index);
		}
        uart_coex_info.profile_refcount[profile_index]++;

        if(0 == phci_conn->profile_refcount[profile_index]) {
			need_update = TRUE;
			phci_conn->profile_bitmap |= BIT(profile_index);
		}
		phci_conn->profile_refcount[profile_index]++;
	} else {
		uart_coex_info.profile_refcount[profile_index]--;
        RTKBT_DBG("for test: --, uart_coex_info.profile_refcount[%x] = %x", 
                    profile_index, uart_coex_info.profile_refcount[profile_index]);
		if(uart_coex_info.profile_refcount[profile_index] == 0) {
			need_update = TRUE;
			uart_coex_info.profile_bitmap &= ~(BIT(profile_index));

			/* if profile does not exist, status is meaningless */
			uart_coex_info.profile_status &= ~(BIT(profile_index));
            rtk_check_del_timer(profile_index);
		}

        phci_conn->profile_refcount[profile_index]--;
		if(0 == phci_conn->profile_refcount[profile_index]) {
			need_update = TRUE;
			phci_conn->profile_bitmap &= ~(BIT(profile_index));

            /* clear profile_hid_interval if need */
			if((profile_hid == profile_index)
                    &&(phci_conn->profile_bitmap &(BIT(profile_hid_interval)))) {
			    phci_conn->profile_bitmap &= ~(BIT(profile_hid_interval));
			    uart_coex_info.profile_refcount[profile_hid_interval]--;
			}
        }
	}

    RTKBT_DBG("update_profile_connection: uart_coex_info.profile_bitmap = %x", uart_coex_info.profile_bitmap);
    for(kk = 0; kk < 8; kk++)
        RTKBT_DBG("update_profile_connection: uart_coex_info.profile_refcount[%d] = %d", kk, uart_coex_info.profile_refcount[kk]);

	if(need_update)
		rtk_notify_profileinfo_to_fw();
}

static void update_hid_active_state(uint16_t handle, uint16_t interval)
{
	uint8_t need_update = 0;
	rtk_conn_prof *phci_conn = find_connection_by_handle(&uart_coex_info, handle);
    
	if(phci_conn == NULL)
		return;

	RTKBT_DBG("update_hid_active_state: handle = %x, interval = 0x%x", handle, interval);
	if(((phci_conn->profile_bitmap) & (BIT(profile_hid))) == 0) {
		RTKBT_DBG("hid not connected in the handle, nothing to be down");
		return;
	}

	if(interval < 60) {
		if((phci_conn->profile_bitmap &(BIT(profile_hid_interval))) == 0) {
			need_update = 1;
			phci_conn->profile_bitmap |= BIT(profile_hid_interval);

			uart_coex_info.profile_refcount[profile_hid_interval]++;
			if(uart_coex_info.profile_refcount[profile_hid_interval] == 1)
				uart_coex_info.profile_status |= BIT(profile_hid);
		}
	} else {
		if((phci_conn->profile_bitmap &(BIT(profile_hid_interval)))) {
			need_update = 1;
			phci_conn->profile_bitmap &= ~(BIT(profile_hid_interval));

			uart_coex_info.profile_refcount[profile_hid_interval]--;
			if(uart_coex_info.profile_refcount[profile_hid_interval] == 0)
				uart_coex_info.profile_status &= ~(BIT(profile_hid));
		}
	}

	if(need_update)
		rtk_notify_profileinfo_to_fw();
}

static uint8_t handle_l2cap_con_req(uint16_t handle, uint16_t psm, uint16_t scid, uint8_t direction)
{
	uint8_t status = FALSE;
	rtk_prof_info* prof_info = NULL;
	int8_t profile_index = psm_to_profile_index(psm);

	if(profile_index < 0) {
		RTKBT_DBG("PSM(0x%x) do not need parse", psm);
		return status;
	}

	spin_lock(&uart_coex_info.spin_lock_profile);
	if(direction)//1: out
		prof_info = find_profile_by_handle_scid(&uart_coex_info, handle, scid);
	else // 0:in
		prof_info = find_profile_by_handle_dcid(&uart_coex_info, handle, scid);

	if(prof_info) {
		RTKBT_DBG("handle_l2cap_con_req: this profile is already exist!!!");
		spin_unlock(&uart_coex_info.spin_lock_profile);
		return status;
	}

	if(direction)//1: out
		status = list_allocate_add(handle, psm, profile_index, 0, scid);
	else // 0:in
		status = list_allocate_add(handle, psm, profile_index, scid, 0);

	spin_unlock(&uart_coex_info.spin_lock_profile);

	if (!status)
		RTKBT_ERR("handle_l2cap_con_req: list_allocate_add failed!");

	return status;
}

static uint8_t handle_l2cap_con_rsp(uint16_t handle, uint16_t dcid, uint16_t scid, uint8_t direction, uint8_t result)
{
	rtk_prof_info* prof_info = NULL;
	rtk_conn_prof* phci_conn = NULL;

	spin_lock(&uart_coex_info.spin_lock_profile);
	if(!direction)//0, in
		prof_info = find_profile_by_handle_scid(&uart_coex_info, handle, scid);
	else //1, out
		prof_info = find_profile_by_handle_dcid(&uart_coex_info, handle, scid);

	if (!prof_info) {
		//RTKBT_DBG("handle_l2cap_con_rsp: prof_info Not Find!!");
		spin_unlock(&uart_coex_info.spin_lock_profile);
		return FALSE;
	}

    if(!result){ //success
	    RTKBT_DBG("l2cap connection success, update connection");
	    if(!direction)//0, in
		    prof_info->dcid = dcid;
	    else//1, out
		    prof_info->scid = dcid;

	    phci_conn = find_connection_by_handle(&uart_coex_info, handle);
        if(phci_conn)
	        update_profile_connection(phci_conn, prof_info->profile_index, TRUE);
    }

	spin_unlock(&uart_coex_info.spin_lock_profile);
	return TRUE;
}

static uint8_t handle_l2cap_discon_req(uint16_t handle, uint16_t dcid, uint16_t scid, uint8_t direction)
{
	rtk_prof_info* prof_info = NULL;
	rtk_conn_prof* phci_conn = NULL;
    RTKBT_DBG("l2cap_discon_req, handle = %x, dcid = %x, scid = %x, direction = %x", handle, dcid, scid, direction);

    spin_lock(&uart_coex_info.spin_lock_profile);
	if(!direction)//0: in
		prof_info = find_profile_by_handle_dcid_scid(&uart_coex_info, handle, scid, dcid);
	else //1: out
		prof_info = find_profile_by_handle_dcid_scid(&uart_coex_info, handle, dcid, scid);

	if (!prof_info) {
		//LogMsg("handle_l2cap_discon_req: prof_info Not Find!");
		spin_unlock(&uart_coex_info.spin_lock_profile);
		return 0;
	}

	phci_conn = find_connection_by_handle(&uart_coex_info, handle);
    if(!phci_conn) {
		spin_unlock(&uart_coex_info.spin_lock_profile);
        return 0;
    }

	update_profile_connection(phci_conn, prof_info->profile_index, FALSE);
	delete_profile_from_hash(prof_info);
    spin_unlock(&uart_coex_info.spin_lock_profile);

	return 1;
}

static void packets_count(uint16_t handle, uint16_t scid, uint16_t length, uint8_t direction)
{
	rtk_prof_info* prof_info = NULL;

    rtk_conn_prof* hci_conn = find_connection_by_handle(&uart_coex_info, handle);
    if(NULL == hci_conn)
        return;

    if(0 == hci_conn->type) {
        if(!direction)	//0: in
		    prof_info = find_profile_by_handle_scid(&uart_coex_info, handle, scid);
	    else //1: out
		    prof_info = find_profile_by_handle_dcid(&uart_coex_info, handle, scid);

	    if(!prof_info) {
		    //RTKBT_DBG("packets_count: prof_info Not Find!");
		    return ;
	    }

	    if((prof_info->profile_index == profile_a2dp) && (length > 100)) { //avdtp media data
		    if(!is_profile_busy(profile_a2dp))
			    update_profile_state(profile_a2dp, TRUE);
		    uart_coex_info.a2dp_packet_count++;
	    }

	    if(prof_info->profile_index == profile_pan)
		    uart_coex_info.pan_packet_count++;    
    }
}

static void count_a2dp_packet_timeout(unsigned long data)
{
	RTKBT_DBG("count a2dp packet timeout, a2dp_packet_count = %d",uart_coex_info.a2dp_packet_count);
	if(uart_coex_info.a2dp_packet_count == 0) {
		if(is_profile_busy(profile_a2dp)) {
			RTKBT_DBG("timeout_handler: a2dp busy->idle!");
			update_profile_state(profile_a2dp, FALSE);
		}
	}
	uart_coex_info.a2dp_packet_count = 0;
    mod_timer(&(uart_coex_info.a2dp_count_timer), jiffies + msecs_to_jiffies(1000));
}

static void count_pan_packet_timeout(unsigned long data)
{
	RTKBT_DBG("count pan packet timeout, pan_packet_count = %d",uart_coex_info.pan_packet_count);
	if(uart_coex_info.pan_packet_count < PAN_PACKET_COUNT) {
		if(is_profile_busy(profile_pan)) {
			RTKBT_DBG("timeout_handler: pan busy->idle!");
			update_profile_state(profile_pan, FALSE);
		}
	} else {
		if(!is_profile_busy(profile_pan)) {
			RTKBT_DBG("timeout_handler: pan idle->busy!");
			update_profile_state(profile_pan, TRUE);
		}
	}
	uart_coex_info.pan_packet_count = 0;
    mod_timer(&(uart_coex_info.pan_count_timer), jiffies + msecs_to_jiffies(1000));
}

static void count_hogp_packet_timeout(unsigned long data)
{
	RTKBT_DBG("count hogp packet timeout, hogp_packet_count = %d",uart_coex_info.hogp_packet_count);
	if(uart_coex_info.hogp_packet_count == 0) {
		if(is_profile_busy(profile_hogp)) {
			RTKBT_DBG("timeout_handler: hogp busy->idle!");
			update_profile_state(profile_hogp, FALSE);
		}
	}
	uart_coex_info.hogp_packet_count = 0;
	
	RTKBT_DBG("count hogp packet timeout, voice_packet_count = %d",uart_coex_info.voice_packet_count);
	if(uart_coex_info.voice_packet_count == 0) {
		if(is_profile_busy(profile_voice)) {
			RTKBT_DBG("timeout_handler: voice busy->idle!");
			update_profile_state(profile_voice, FALSE);
		}
	}
	uart_coex_info.voice_packet_count = 0;
    mod_timer(&(uart_coex_info.hogp_count_timer), jiffies + msecs_to_jiffies(1000));
}

static int udpsocket_send(char *tx_msg, int msg_size)
{
    u8 error = 0;
    struct msghdr   udpmsg;
    mm_segment_t    oldfs;
    struct iovec    iov;

    RTKBT_DBG("send msg %s with len:%d", tx_msg, msg_size);

    if(uart_coex_info.sock_open) {
        iov.iov_base     = (void *)tx_msg;
        iov.iov_len  = msg_size;
        udpmsg.msg_name  = &uart_coex_info.wifi_addr;
        udpmsg.msg_namelen  = sizeof(struct sockaddr_in);
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)
        udpmsg.msg_iov   = &iov;
        udpmsg.msg_iovlen   = 1;
#else
        iov_iter_init(&udpmsg.msg_iter, WRITE, &iov, 1, msg_size);
#endif
        udpmsg.msg_control  = NULL;
        udpmsg.msg_controllen = 0;
        udpmsg.msg_flags    = MSG_DONTWAIT | MSG_NOSIGNAL;
        oldfs = get_fs();
        set_fs(KERNEL_DS);
        error = sock_sendmsg(uart_coex_info.udpsock, &udpmsg, msg_size);
        set_fs(oldfs);

        if(error < 0)
            RTKBT_DBG("Error when sendimg msg, error:%d",error);
    }

    return error;
}

static void udpsocket_recv_data(void)
{
    u8 recv_data[512];
    u32 len = 0;
    u16 recv_length;
    struct sk_buff * skb;

    RTKBT_DBG("udpsocket_recv_data");

    spin_lock(&uart_coex_info.spin_lock_sock);
    len = skb_queue_len(&uart_coex_info.sk->sk_receive_queue);

    while(len > 0) {
        skb = skb_dequeue(&uart_coex_info.sk->sk_receive_queue);

        /*important: cut the udp header from skb->data! header length is 8 byte*/
        recv_length = skb->len - 8;
        memset(recv_data, 0, sizeof(recv_data));
        memcpy(recv_data, skb->data + 8, recv_length);
        //RTKBT_DBG("received data: %s :with len %u", recv_data, recv_length);

        rtk_handle_event_from_wifi(recv_data);

        len--;
        kfree_skb(skb);
    }

    spin_unlock(&uart_coex_info.spin_lock_sock);
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 14, 0)
static void udpsocket_recv(struct sock *sk, int bytes)
#else
static void udpsocket_recv(struct sock *sk)
#endif
{
    spin_lock(&uart_coex_info.spin_lock_sock);
    uart_coex_info.sk = sk;
    spin_unlock(&uart_coex_info.spin_lock_sock);
    queue_delayed_work(uart_coex_info.sock_wq, &uart_coex_info.sock_work, 0);
}

static void create_udpsocket(void)
{
    int err;
    RTKBT_DBG("create udpsocket, connect_port: %d", CONNECT_PORT);
    uart_coex_info.sock_open = 0;

    err = sock_create(AF_INET, SOCK_DGRAM, IPPROTO_UDP, &uart_coex_info.udpsock);
    if (err < 0) {
        RTKBT_ERR("create udp socket error, err = %d", err);
        return;
    }

    memset(&uart_coex_info.addr, 0, sizeof(struct sockaddr_in));
    uart_coex_info.addr.sin_family       = AF_INET;
    uart_coex_info.addr.sin_addr.s_addr  = htonl(INADDR_LOOPBACK);
    uart_coex_info.addr.sin_port         = htons(CONNECT_PORT);

    memset(&uart_coex_info.wifi_addr, 0, sizeof(struct sockaddr_in));
    uart_coex_info.wifi_addr.sin_family       = AF_INET;
    uart_coex_info.wifi_addr.sin_addr.s_addr  = htonl(INADDR_LOOPBACK);
    uart_coex_info.wifi_addr.sin_port         = htons(CONNECT_PORT_WIFI);

    err = uart_coex_info.udpsock->ops->bind(uart_coex_info.udpsock, (struct sockaddr *)&uart_coex_info.addr, sizeof(struct sockaddr));
    if (err < 0) {
        sock_release(uart_coex_info.udpsock);
        RTKBT_ERR("bind udp socket error, err = %d", err);
        return;
    }

    uart_coex_info.sock_open = 1;
    uart_coex_info.udpsock->sk->sk_data_ready = udpsocket_recv;
}

static void rtk_notify_extension_version_to_wifi(void)
{
    uint8_t para_length = 2;
    char p_buf[para_length + HCI_CMD_PREAMBLE_SIZE];
    char *p = p_buf;

    if(!uart_coex_info.wifi_on)
        return;

    UINT16_TO_STREAM(p, HCI_OP_HCI_EXTENSION_VERSION_NOTIFY);
    *p++ = para_length;
    UINT16_TO_STREAM(p, HCI_EXTENSION_VERSION);
    RTKBT_DBG("extension version is 0x%x", HCI_EXTENSION_VERSION);
    if(udpsocket_send(p_buf, para_length + HCI_CMD_PREAMBLE_SIZE) < 0)
       RTKBT_ERR("rtk_notify_extension_version_to_wifi: udpsocket send error");
}

static void rtk_notify_btpatch_version_to_wifi(void)
{
    uint8_t para_length = 4;
    char p_buf[para_length + HCI_CMD_PREAMBLE_SIZE];
    char *p = p_buf;

    if(!uart_coex_info.wifi_on)
        return;

    UINT16_TO_STREAM(p, HCI_OP_HCI_BT_PATCH_VER_NOTIFY);
    *p++ = para_length;
    UINT16_TO_STREAM(p, uart_coex_info.hci_reversion);
    UINT16_TO_STREAM(p, uart_coex_info.lmp_subversion);
    RTKBT_DBG("btpatch_version, length is 0x%x, hci_reversion is 0x%x, lmp_subversion is %x",
                    para_length, uart_coex_info.hci_reversion, uart_coex_info.lmp_subversion);

    if(udpsocket_send(p_buf, para_length + HCI_CMD_PREAMBLE_SIZE) < 0)
        RTKBT_ERR("rtk_notify_btpatch_version_to_wifi: udpsocket send error");
}

static void rtk_notify_afhmap_to_wifi(void)
{
    uint8_t para_length = 13;
    char p_buf[para_length + HCI_CMD_PREAMBLE_SIZE];
    char *p = p_buf;
    uint8_t kk = 0;

    if(!uart_coex_info.wifi_on)
        return;

    UINT16_TO_STREAM(p, HCI_OP_HCI_BT_AFH_MAP_NOTIFY);
    *p++ = para_length;
    *p++ = uart_coex_info.piconet_id;
    *p++ = uart_coex_info.mode;
    *p++ = 10;
    memcpy(p, uart_coex_info.afh_map, 10);

    RTKBT_DBG("afhmap, piconet_id is 0x%x, map type is 0x%x", uart_coex_info.piconet_id, uart_coex_info.mode);
    for(kk = 0; kk < 10; kk++)
        RTKBT_DBG("afhmap data[%d] is 0x%x", kk, uart_coex_info.afh_map[kk]);

    if(udpsocket_send(p_buf, para_length + HCI_CMD_PREAMBLE_SIZE) < 0)
        RTKBT_ERR("rtk_notify_afhmap_to_wifi: udpsocket send error");
}

static void rtk_notify_btcoex_to_wifi(uint8_t opcode, uint8_t status)
{
    uint8_t para_length = 2;
    char p_buf[para_length + HCI_CMD_PREAMBLE_SIZE];
    char *p = p_buf;

    if(!uart_coex_info.wifi_on)
        return;

    UINT16_TO_STREAM(p, HCI_OP_HCI_BT_COEX_NOTIFY);
    *p++ = para_length;
    *p++ = opcode;
    if(!status)
        *p++ = 0;
    else
        *p++ = 1;

    RTKBT_DBG("btcoex, opcode is 0x%x, status is 0x%x", opcode, status);

    if(udpsocket_send(p_buf, para_length + HCI_CMD_PREAMBLE_SIZE) < 0)
        RTKBT_ERR("rtk_notify_btcoex_to_wifi: udpsocket send error");
}

static void rtk_notify_btoperation_to_wifi(uint8_t operation, uint8_t append_data_length, uint8_t *append_data)
{
    uint8_t para_length = 3 + append_data_length;
    char p_buf[para_length + HCI_CMD_PREAMBLE_SIZE];
    char *p = p_buf;
    uint8_t kk = 0;

    if(!uart_coex_info.wifi_on)
        return;

    UINT16_TO_STREAM(p, HCI_OP_BT_OPERATION_NOTIFY);
    *p++ = para_length;
    *p++ = operation;
    *p++ = append_data_length;
    if(append_data_length)
        memcpy(p, append_data, append_data_length);

    RTKBT_DBG("btoperation, opration is 0x%x, append_data_length is 0x%x", operation, append_data_length);
    if(append_data_length) {
        for(kk = 0; kk < append_data_length; kk++)
            RTKBT_DBG("append data is 0x%x", *(append_data+kk));
    }

    if(udpsocket_send(p_buf, para_length + HCI_CMD_PREAMBLE_SIZE) < 0)
        RTKBT_ERR("rtk_notify_btoperation_to_wifi: udpsocket send error");
}

static void rtk_notify_info_to_wifi(uint8_t reason, uint8_t length, uint8_t* report_info)
{
    uint8_t para_length = 4 + length;
    char p_buf[para_length + HCI_CMD_PREAMBLE_SIZE];
    char *p = p_buf;
    hci_linkstatus_report *report = (hci_linkstatus_report *)report_info;

    if(!uart_coex_info.wifi_on)
        return;

    UINT16_TO_STREAM(p, HCI_OP_HCI_BT_INFO_NOTIFY);
    *p++ = para_length;
    *p++ = uart_coex_info.polling_enable;
    *p++ = uart_coex_info.polling_interval;
    *p++ = reason;
    *p++ = length;

    if(length)
        memcpy(p, report_info, length);

    RTKBT_DBG("bt info, length is 0x%x, polling_enable is 0x%x, poiiling_interval is %x",
                        para_length, uart_coex_info.polling_enable, uart_coex_info.polling_interval);
    RTKBT_DBG("bt info, reason is 0x%x, info length is 0x%x", reason, length);
    if(length) {
        RTKBT_DBG("bt info, cmd_index is %x", report->cmd_index);
        RTKBT_DBG("bt info, cmd_length is %x", report->cmd_length);
        RTKBT_DBG("bt info, link_status is %x", report->link_status);
        RTKBT_DBG("bt info, retry_cnt is %x", report->retry_cnt);
        RTKBT_DBG("bt info, rssi is %x", report->rssi);
        RTKBT_DBG("bt info, mailbox_info is %x", report->mailbox_info);
        RTKBT_DBG("bt info, acl_throughtput is %x", report->acl_throughput);
    }

    if(udpsocket_send(p_buf, para_length + HCI_CMD_PREAMBLE_SIZE) < 0)
        RTKBT_ERR("rtk_notify_info_to_wifi: udpsocket send error");
}

static void rtk_notify_regester_to_wifi(uint8_t* reg_value)
{
    uint8_t para_length = 9;
    char p_buf[para_length + HCI_CMD_PREAMBLE_SIZE];
    char *p = p_buf;
    hci_mailbox_register *reg = (hci_mailbox_register *)reg_value;

    if(!uart_coex_info.wifi_on)
        return;

    UINT16_TO_STREAM(p, HCI_OP_HCI_BT_REGISTER_VALUE_NOTIFY);
    *p++ = para_length;
    memcpy(p, reg_value, para_length);

    RTKBT_DBG("bt register, register type is %x", reg->type);
    RTKBT_DBG("bt register, register offset is %x", reg->offset);
    RTKBT_DBG("bt register, register value is %x", reg->value);

    if(udpsocket_send(p_buf, para_length + HCI_CMD_PREAMBLE_SIZE) < 0)
        RTKBT_ERR("rtk_notify_regester_to_wifi: udpsocket send error");
}

void rtk_uart_parse_cmd(struct sk_buff *skb)
{
    u16 opcode = (skb->data[0]) + ((skb->data[1])<<8);

    if((opcode == HCI_OP_INQUIRY) ||(opcode == HCI_OP_PERIODIC_INQ)) {
        if(!uart_coex_info.isinquirying) {
            uart_coex_info.isinquirying = 1;
            RTKBT_DBG("notify wifi inquiry start");
            rtk_notify_btoperation_to_wifi(BT_OPCODE_INQUIRY_START, 0, NULL);
        }
    }

    if((opcode == HCI_OP_INQUIRY_CANCEL) || (opcode == HCI_OP_EXIT_PERIODIC_INQ)) {
        if(uart_coex_info.isinquirying) {
            uart_coex_info.isinquirying = 0;
            RTKBT_DBG("notify wifi inquiry stop");
            rtk_notify_btoperation_to_wifi(BT_OPCODE_INQUIRY_END, 0, NULL);
        }
    }

    if(opcode == HCI_OP_ACCEPT_CONN_REQ) {
        if(!uart_coex_info.ispaging) {
            uart_coex_info.ispaging = 1;
            RTKBT_DBG("notify wifi page start");
            rtk_notify_btoperation_to_wifi(BT_OPCODE_PAGE_START, 0, NULL);
        }
    }
}

static void rtk_handle_inquiry_complete(void)
{
    if(uart_coex_info.isinquirying) {
        uart_coex_info.isinquirying = 0;
        RTKBT_DBG("notify wifi inquiry end");
        rtk_notify_btoperation_to_wifi(BT_OPCODE_INQUIRY_END, 0, NULL);
    }
}

static void rtk_handle_pin_code_req(void)
{
    if(!uart_coex_info.ispairing) {
        uart_coex_info.ispairing = 1;
        RTKBT_DBG("notify wifi pair start");
        rtk_notify_btoperation_to_wifi(BT_OPCODE_PAIR_START, 0, NULL);
    }
}

static void rtk_handle_io_capa_req(void)
{
    if(!uart_coex_info.ispairing) {
        uart_coex_info.ispairing = 1;
        RTKBT_DBG("notify wifi pair start");
        rtk_notify_btoperation_to_wifi(BT_OPCODE_PAIR_START, 0, NULL);
    }
}

static void rtk_handle_auth_request(void)
{
    if(uart_coex_info.ispairing) {
        uart_coex_info.ispairing = 0;
        RTKBT_DBG("notify wifi pair end");
        rtk_notify_btoperation_to_wifi(BT_OPCODE_PAIR_END, 0, NULL);
    }
}

static void rtk_handle_link_key_notify(void)
{
    if(uart_coex_info.ispairing) {
        uart_coex_info.ispairing = 0;
        RTKBT_DBG("notify wifi pair end");
        rtk_notify_btoperation_to_wifi(BT_OPCODE_PAIR_END, 0, NULL);
    }
}

static void rtk_handle_mode_change_evt(u8* p)
{
    u16 mode_change_handle, mode_interval;

    p++;
    STREAM_TO_UINT16(mode_change_handle, p);
    p++;
    STREAM_TO_UINT16(mode_interval, p);
    update_hid_active_state(mode_change_handle, mode_interval);
}

static void rtk_parse_vendor_mailbox_cmd_evt(u8*p, u8 total_len)
{
	u8 status, subcmd;
    u8 temp_cmd[10];

    status = *p++;
    if(total_len <= 4) {
        RTKBT_DBG("receive mailbox cmd from fw, total length <= 4");
        return;
    }
    subcmd = *p++;
    RTKBT_DBG("receive mailbox cmd from fw, subcmd is 0x%x, status is 0x%x", subcmd, status);

    switch(subcmd) {
    case HCI_VENDOR_SUB_CMD_BT_REPORT_CONN_SCO_INQ_INFO:
        if(status == 0) //success
            rtk_notify_info_to_wifi(POLLING_RESPONSE, sizeof(hci_linkstatus_report), (uint8_t*)p);
        break;

    case HCI_VENDOR_SUB_CMD_WIFI_CHANNEL_AND_BANDWIDTH_CMD:
        rtk_notify_btcoex_to_wifi(WIFI_BW_CHNL_NOTIFY, status);
        break;

    case HCI_VENDOR_SUB_CMD_WIFI_FORCE_TX_POWER_CMD:
        rtk_notify_btcoex_to_wifi(BT_POWER_DECREASE_CONTROL, status);
        break;

    case HCI_VENDOR_SUB_CMD_BT_ENABLE_IGNORE_WLAN_ACT_CMD:
        rtk_notify_btcoex_to_wifi(IGNORE_WLAN_ACTIVE_CONTROL, status);
        break;

    case HCI_VENDOR_SUB_CMD_SET_BT_PSD_MODE:
        rtk_notify_btcoex_to_wifi(BT_PSD_MODE_CONTROL, status);
        break;

    case HCI_VENDOR_SUB_CMD_SET_BT_LNA_CONSTRAINT:
        rtk_notify_btcoex_to_wifi(LNA_CONSTRAIN_CONTROL, status);
        break;

    case HCI_VENDOR_SUB_CMD_BT_AUTO_REPORT_ENABLE:
        break;

    case HCI_VENDOR_SUB_CMD_BT_SET_TXRETRY_REPORT_PARAM:
        break;

    case HCI_VENDOR_SUB_CMD_BT_SET_PTATABLE:
        break;

    case HCI_VENDOR_SUB_CMD_GET_AFH_MAP_L:
        if(status == 0) {
            memcpy(uart_coex_info.afh_map, p+4, 4); /* cmd_idx, length, piconet_id, mode */
            temp_cmd[0] = HCI_VENDOR_SUB_CMD_GET_AFH_MAP_M;
            temp_cmd[1] = 2;
            temp_cmd[2] = uart_coex_info.piconet_id;
            temp_cmd[3] = uart_coex_info.mode;
            rtk_vendor_cmd_to_fw(HCI_VENDOR_MAILBOX_CMD, 4, temp_cmd);
        } else {
            memset(uart_coex_info.afh_map, 0, 10);
            rtk_notify_afhmap_to_wifi();
        }
        break;

    case HCI_VENDOR_SUB_CMD_GET_AFH_MAP_M:
        if(status == 0) {
            memcpy(uart_coex_info.afh_map+4, p+4, 4);
            temp_cmd[0] = HCI_VENDOR_SUB_CMD_GET_AFH_MAP_H;
            temp_cmd[1] = 2;
            temp_cmd[2] = uart_coex_info.piconet_id;
            temp_cmd[3] = uart_coex_info.mode;
            rtk_vendor_cmd_to_fw(HCI_VENDOR_MAILBOX_CMD, 4, temp_cmd);
        } else {
            memset(uart_coex_info.afh_map, 0, 10);
            rtk_notify_afhmap_to_wifi();
        }
        break;

    case HCI_VENDOR_SUB_CMD_GET_AFH_MAP_H:
        if(status == 0)
            memcpy(uart_coex_info.afh_map+8, p+4, 2);
        else
            memset(uart_coex_info.afh_map, 0, 10);

        rtk_notify_afhmap_to_wifi();
        break;

    case HCI_VENDOR_SUB_CMD_RD_REG_REQ:
        if(status == 0)
            rtk_notify_regester_to_wifi(p+3);/* cmd_idx,length,regist type */
        break;

    case HCI_VENDOR_SUB_CMD_WR_REG_REQ:
        rtk_notify_btcoex_to_wifi(BT_REGISTER_ACCESS, status);
        break;

    default:
        break;
    }
}

static void rtk_handle_cmd_complete_evt(u8 total_len, u8* p)
{
    u16 opcode;

    p++;
    STREAM_TO_UINT16(opcode, p);
    //RTKBT_DBG("cmd_complete, opcode is 0x%x", opcode);
    if(opcode == uart_coex_info.last_send_cmd) {
        //RTKBT_DBG("opcode is send by us");
        uart_coex_info.num_hci_cmd_packet = 1;
        if(!skb_queue_empty(&uart_coex_info.cmd_q))
            tasklet_schedule(&uart_coex_info.cmd_task);
    }

    if (opcode == HCI_OP_PERIODIC_INQ) {
        if(*p++ && uart_coex_info.isinquirying) {
            uart_coex_info.isinquirying = 0;
            RTKBT_DBG("HCI_PERIODIC_INQUIRY starte error, notify wifi inquiry stop");
            rtk_notify_btoperation_to_wifi(BT_OPCODE_INQUIRY_END, 0, NULL);
        }
    }

    if (opcode == HCI_OP_READ_LOCAL_VERSION) {
        if(!(*p++)) {
            p++;
            STREAM_TO_UINT16(uart_coex_info.hci_reversion, p);
            p += 3;
            STREAM_TO_UINT16(uart_coex_info.lmp_subversion, p);
            RTKBT_DBG("uart_coex_info.hci_reversion = %x", uart_coex_info.hci_reversion);
            RTKBT_DBG("uart_coex_info.lmp_subversion = %x", uart_coex_info.lmp_subversion);
        }
    }

    if (opcode == HCI_VENDOR_MAILBOX_CMD) {
        rtk_parse_vendor_mailbox_cmd_evt(p, total_len);
    }
}

static void rtk_handle_cmd_status_evt(u8* p)
{
	u16 opcode;
	u8 status;

    status = *p++;
    p++;
    STREAM_TO_UINT16(opcode, p);
    //RTKBT_DBG("cmd_status, opcode is 0x%x", opcode);
    if(opcode == uart_coex_info.last_send_cmd) {
        //RTKBT_DBG("opcode is send by us");
        uart_coex_info.num_hci_cmd_packet = 1;
        if(!skb_queue_empty(&uart_coex_info.cmd_q))
            tasklet_schedule(&uart_coex_info.cmd_task);
    }

    if((opcode == HCI_OP_INQUIRY) && (status)) {
        if(uart_coex_info.isinquirying) {
            uart_coex_info.isinquirying = 0;
            RTKBT_DBG("inquiry start error, notify wifi inquiry stop");
            rtk_notify_btoperation_to_wifi(BT_OPCODE_INQUIRY_END, 0, NULL);
        }
    }

    if(opcode == HCI_OP_CREATE_CONN) {
        if(!status && !uart_coex_info.ispaging) {
            uart_coex_info.ispaging = 1;
            RTKBT_DBG("notify wifi start page");
            rtk_notify_btoperation_to_wifi(BT_OPCODE_PAGE_START, 0, NULL);
        }
    }
}

static void rtk_handle_connection_complete_evt(u8* p)
{
	u16 handle;
	u8 status, link_type;
    rtk_conn_prof* hci_conn = NULL;

    status = *p++;
    STREAM_TO_UINT16 (handle, p);
    p +=6;
    link_type = *p++;

    if(status == 0) {
        if(uart_coex_info.ispaging){
            uart_coex_info.ispaging = 0;
            RTKBT_DBG("notify wifi page success end");
            rtk_notify_btoperation_to_wifi(BT_OPCODE_PAGE_SUCCESS_END, 0, NULL);
        }

        hci_conn = find_connection_by_handle(&uart_coex_info, handle);
        if(hci_conn == NULL) {
            hci_conn = allocate_connection_by_handle(handle);
            if(hci_conn) {
                add_connection_to_hash(&uart_coex_info, hci_conn);
                hci_conn->profile_bitmap = 0;
                memset(hci_conn->profile_refcount, 0, 8);
                if((0 == link_type) || (2 == link_type)) {//sco or esco
                    hci_conn->type = 1;
                    update_profile_connection(hci_conn, profile_sco, TRUE);
                } else
                    hci_conn->type = 0;
            } else {
                RTKBT_ERR("hci connection allocate fail");
            }
        } else {
            RTKBT_DBG("hci connection handle(0x%x) has already exist!", handle);
            hci_conn->profile_bitmap = 0;
            memset(hci_conn->profile_refcount, 0, 8);
            if((0 == link_type)||(2 == link_type)) {//sco or esco
                hci_conn->type = 1;
                update_profile_connection(hci_conn, profile_sco, TRUE);
            } else
                hci_conn->type = 0;
        }
    } else if(uart_coex_info.ispaging) {
        uart_coex_info.ispaging = 0;
        RTKBT_DBG("notify wifi page unsuccess end");
        rtk_notify_btoperation_to_wifi(BT_OPCODE_PAGE_UNSUCCESS_END, 0, NULL);
    }
}

static void rtk_handle_le_connection_complete_evt(u8* p)
{
    u16 handle, interval;
    u8 status;
    rtk_conn_prof* hci_conn = NULL;

    status = *p++;
    STREAM_TO_UINT16 (handle, p);
    p += 8; //role, address type, address
    STREAM_TO_UINT16 (interval, p);

    if(status == 0) {
        if(uart_coex_info.ispaging){
            uart_coex_info.ispaging = 0;
            RTKBT_DBG("notify wifi page success end");
            rtk_notify_btoperation_to_wifi(BT_OPCODE_PAGE_SUCCESS_END, 0, NULL);
        }

        hci_conn = find_connection_by_handle(&uart_coex_info, handle);
        if(hci_conn == NULL) {
            hci_conn = allocate_connection_by_handle(handle);
            if(hci_conn) {
                add_connection_to_hash(&uart_coex_info, hci_conn);
                hci_conn->profile_bitmap = 0;
                memset(hci_conn->profile_refcount, 0, 8);
                hci_conn->type = 2;
                update_profile_connection(hci_conn, profile_hid, TRUE); //for coex, le is the same as hid
                update_hid_active_state(handle, interval);
            } else {
                RTKBT_ERR("hci connection allocate fail");
            }
        } else {
            RTKBT_DBG("hci connection handle(0x%x) has already exist!", handle);
            hci_conn->profile_bitmap = 0;
            memset(hci_conn->profile_refcount, 0, 8);
            hci_conn->type = 2;
            update_profile_connection(hci_conn, profile_hid, TRUE);
            update_hid_active_state(handle, interval);
        }
    } else if(uart_coex_info.ispaging) {
        uart_coex_info.ispaging = 0;
        RTKBT_DBG("notify wifi page unsuccess end");
        rtk_notify_btoperation_to_wifi(BT_OPCODE_PAGE_UNSUCCESS_END, 0, NULL);
    }
}

static void rtk_handle_le_connection_update_complete_evt(u8* p)
{
    u16 handle, interval;
    u8 status;

    status = *p++;
    STREAM_TO_UINT16 (handle, p);
    STREAM_TO_UINT16 (interval, p);
    update_hid_active_state(handle, interval);
}

static void rtk_handle_le_meta_evt(u8* p)
{
    u8 sub_event = *p++;
    switch (sub_event) {
    case HCI_EV_LE_CONN_COMPLETE:
        rtk_handle_le_connection_complete_evt(p);
        break;

    case HCI_EV_LE_CONN_UPDATE_COMPLETE:
        rtk_handle_le_connection_update_complete_evt(p);
        break;

    default :
        break;
    }
}

static void rtk_handle_disconnect_complete_evt(u8* p)
{
	u16 handle;
	u8 status, reason;
	rtk_conn_prof* hci_conn = NULL;
    struct list_head* iter = NULL, *temp = NULL;
	rtk_prof_info* prof_info = NULL;

    if(uart_coex_info.ispairing) { //for slave: connection will be disconnected if authentication fail
        uart_coex_info.ispairing = 0;
        RTKBT_DBG("notify wifi pair end");
        rtk_notify_btoperation_to_wifi(BT_OPCODE_PAIR_END, 0, NULL);
    }

    status = *p++;
    STREAM_TO_UINT16(handle, p);
    reason = *p;

    if(status == 0) {
        hci_conn = find_connection_by_handle(&uart_coex_info, handle);
        if(hci_conn) {
            switch(hci_conn->type) {
            case 0:
                spin_lock(&uart_coex_info.spin_lock_profile);
                list_for_each_safe(iter, temp, &uart_coex_info.profile_list) {
                    prof_info = list_entry(iter, rtk_prof_info, list);
                    if ((handle == prof_info->handle) && prof_info->scid && prof_info->dcid) {
                        RTKBT_DBG("find info when hci disconnect, handle:%x, psm:%x, dcid:%x, scid:%x",
                                     prof_info->handle, prof_info->psm, prof_info->dcid, prof_info->scid);
                        //If both scid and dcid > 0, L2cap connection is exist.
                        update_profile_connection(hci_conn, prof_info->profile_index, FALSE);
                        delete_profile_from_hash(prof_info);
                    }
                }
                spin_unlock(&uart_coex_info.spin_lock_profile);
                break;

            case 1:
                update_profile_connection(hci_conn, profile_sco, FALSE);
                break;

            case 2:
                update_profile_connection(hci_conn, profile_hid, FALSE);
                break;

            default:
                break;
            }
            delete_connection_from_hash(hci_conn);
        } else {
            RTKBT_ERR("hci connection handle(0x%x) not found", handle);
        }
    }
}

static void rtk_handle_specific_evt(u8* p)
{
    u16 subcode;

    STREAM_TO_UINT16(subcode, p);
    if(subcode == HCI_VENDOR_PTA_AUTO_REPORT_EVENT) {
        RTKBT_DBG("notify wifi driver with autoreport data");
        rtk_notify_info_to_wifi(AUTO_REPORT, sizeof(hci_linkstatus_report), (uint8_t *)p);
    }
}

static void rtk_parse_event_data(void)
{
	u8* p = uart_coex_info.event_data;
	u8 event_code = *p++;
	u8 total_len = *p++;

	switch (event_code) {
	case HCI_EV_INQUIRY_COMPLETE:
        rtk_handle_inquiry_complete();
		break;

	case HCI_EV_PIN_CODE_REQ:
        rtk_handle_pin_code_req();
		break;

	case HCI_EV_IO_CAPA_REQUEST:
        rtk_handle_io_capa_req();
		break;

	case HCI_EV_AUTH_COMPLETE:
        rtk_handle_auth_request();
		break;

	case HCI_EV_LINK_KEY_NOTIFY:
        rtk_handle_link_key_notify();
		break;

	 case HCI_EV_MODE_CHANGE:
        rtk_handle_mode_change_evt(p);
		break;

	case HCI_EV_CMD_COMPLETE:
		rtk_handle_cmd_complete_evt(total_len, p);
		break;

    case HCI_EV_CMD_STATUS:
        rtk_handle_cmd_status_evt(p);
        break;

    case HCI_EV_CONN_COMPLETE:
    case HCI_EV_SYNC_CONN_COMPLETE:
        rtk_handle_connection_complete_evt(p);
        break;

    case HCI_EV_DISCONN_COMPLETE:
        rtk_handle_disconnect_complete_evt(p);
        break;

    case HCI_EV_LE_META:
        rtk_handle_le_meta_evt(p);
        break;

    case HCI_EV_VENDOR_SPECIFIC:
        rtk_handle_specific_evt(p);
        break;

	default:
		break;
	}
}

void rtk_uart_parse_event(struct sk_buff *skb)
{
    uart_coex_info.event_data= (u8*)(skb->data);;
    queue_delayed_work(uart_coex_info.fw_wq, &uart_coex_info.fw_work, 0);
}

void rtk_uart_parse_l2cap_data_tx(struct sk_buff *skb)
{
	u16 handle, total_len, pdu_len, channel_ID, command_len, psm, scid, dcid, result, status;
	u8 flag, code, identifier;
	u8 *pp = (u8*)(skb->data);
	STREAM_TO_UINT16 (handle, pp);
	flag = handle >> 12;
	handle = handle & 0x0FFF;
	STREAM_TO_UINT16 (total_len, pp);
	STREAM_TO_UINT16 (pdu_len, pp);
	STREAM_TO_UINT16 (channel_ID, pp);

	if(channel_ID == 0x0001) {
		code = *pp++;
		switch (code) {
		case L2CAP_CONN_REQ:
			identifier = *pp++;
			STREAM_TO_UINT16 (command_len, pp);
			STREAM_TO_UINT16 (psm, pp);
			STREAM_TO_UINT16 (scid, pp);
			RTKBT_DBG("L2CAP_CONNECTION_REQ, handle=%x, PSM=%x, scid=%x", handle, psm, scid);
			handle_l2cap_con_req(handle, psm, scid, 1);
			break;

		case L2CAP_CONN_RSP:
			identifier = *pp++;
			STREAM_TO_UINT16 (command_len, pp);
			STREAM_TO_UINT16 (dcid, pp);
			STREAM_TO_UINT16 (scid, pp);
			STREAM_TO_UINT16 (result, pp);
			STREAM_TO_UINT16 (status, pp);
			RTKBT_DBG("L2CAP_CONNECTION_RESP, handle=%x, dcid=%x, scid=%x, result=%x", handle, dcid, scid, result);
			handle_l2cap_con_rsp(handle, dcid, scid, 1, result);
			break;

		case L2CAP_DISCONN_REQ:
			identifier = *pp++;
			STREAM_TO_UINT16 (command_len, pp);
			STREAM_TO_UINT16 (dcid, pp);
			STREAM_TO_UINT16 (scid, pp);
			RTKBT_DBG("L2CAP_DISCONNECTION_REQ, handle=%x, dcid=%x, scid=%x",handle, dcid, scid);
			handle_l2cap_discon_req(handle, dcid, scid, 1);
			break;

		case L2CAP_DISCONN_RSP:
			break;

		default:
			break;
		}
	} else {
		if((flag != 0x01)&&(is_profile_connected(profile_a2dp) || is_profile_connected(profile_pan)))//Do not count the continuous packets
			packets_count(handle, channel_ID, pdu_len, 1);
	}
}

static void rtk_parse_l2cap_data_rx_data(u8 *p)
{
	u16 handle, total_len, pdu_len, channel_ID, command_len, psm, scid, dcid, result, status;
	u8 flag, code, identifier;
	u8 *pp = p;
	STREAM_TO_UINT16 (handle, pp);
	flag = handle >> 12;
	handle = handle & 0x0FFF;
	STREAM_TO_UINT16 (total_len, pp);
	STREAM_TO_UINT16 (pdu_len, pp);
	STREAM_TO_UINT16 (channel_ID, pp);

	if(channel_ID == 0x0001) {
		code = *pp++;
		switch (code) {
		case L2CAP_CONN_REQ:
			identifier = *pp++;
			STREAM_TO_UINT16 (command_len, pp);
			STREAM_TO_UINT16 (psm, pp);
			STREAM_TO_UINT16 (scid, pp);
			RTKBT_DBG("L2CAP_CONNECTION_REQ, handle=%x, PSM=%x, scid=%x", handle, psm, scid);
			handle_l2cap_con_req(handle, psm, scid, 0);
			break;

		case L2CAP_CONN_RSP:
			identifier = *pp++;
			STREAM_TO_UINT16 (command_len, pp);
			STREAM_TO_UINT16 (dcid, pp);
			STREAM_TO_UINT16 (scid, pp);
			STREAM_TO_UINT16 (result, pp);
			STREAM_TO_UINT16 (status, pp);
			RTKBT_DBG("L2CAP_CONNECTION_RESP, handle=%x, dcid=%x, scid=%x, result=%x", handle, dcid, scid, result);
			handle_l2cap_con_rsp(handle, dcid, scid, 0, result);
			break;

		case L2CAP_DISCONN_REQ:
			identifier = *pp++;
			STREAM_TO_UINT16 (command_len, pp);
			STREAM_TO_UINT16 (dcid, pp);
			STREAM_TO_UINT16 (scid, pp);
			RTKBT_DBG("L2CAP_DISCONNECTION_REQ, handle=%x, dcid=%x, scid=%x",handle, dcid, scid);
			handle_l2cap_discon_req(handle, dcid, scid, 0);
			break;

		case L2CAP_DISCONN_RSP:
			break;

		default:
			break;
		}
	} else {
		if((flag != 0x01)&&(is_profile_connected(profile_a2dp) || is_profile_connected(profile_pan)))//Do not count the continuous packets
			packets_count(handle, channel_ID, pdu_len, 0);
	}
}

void rtk_uart_parse_l2cap_data_rx(struct sk_buff *skb)
{
	u8 *p = (u8*)(skb->data);
	rtk_parse_l2cap_data_rx_data(p);
}

void rtk_uart_parse_recv_data(u8* p)
{
	u8 *data = p;
	if (*data == HCI_EVENT_PKT) {
		uart_coex_info.event_data= ++data;
		queue_delayed_work(uart_coex_info.fw_wq, &uart_coex_info.fw_work, 0);
	}

	if (*data == HCI_ACLDATA_PKT) {
		rtk_parse_l2cap_data_rx_data(++data);
	}
}

static void polling_bt_info(unsigned long data)
{
    uint8_t temp_cmd[1];
    RTKBT_DBG("polling timer");
    if(uart_coex_info.polling_enable) {
        temp_cmd[0] = HCI_VENDOR_SUB_CMD_BT_REPORT_CONN_SCO_INQ_INFO;
        rtk_vendor_cmd_to_fw(HCI_VENDOR_MAILBOX_CMD, 1, temp_cmd);
    }
    mod_timer(&(uart_coex_info.polling_timer), jiffies + msecs_to_jiffies(1000 * uart_coex_info.polling_interval));
}

static void rtk_handle_bt_info_control(uint8_t* p)
{
    uint8_t temp_cmd[20];
    hci_bt_info_control*  info = (hci_bt_info_control*)p;
    RTKBT_DBG("uart_coex_info.polling_enable is %x", uart_coex_info.polling_enable);
    RTKBT_DBG("receive bt info control event from wifi, polling enable is 0x%x, polling time is 0x%x, auto report is 0x%x", info->polling_enable, info->polling_time, info->autoreport_enable);

    if(info->polling_enable && !uart_coex_info.polling_enable) {
        setup_timer(&(uart_coex_info.polling_timer), polling_bt_info, 0);
        uart_coex_info.polling_timer.expires = jiffies + msecs_to_jiffies(info->polling_time * 1000);
        add_timer(&(uart_coex_info.polling_timer));
    }

    if(!info->polling_enable && uart_coex_info.polling_enable)
        del_timer(&(uart_coex_info.polling_timer));

    if(uart_coex_info.autoreport != info->autoreport_enable) {
        temp_cmd[0] = HCI_VENDOR_SUB_CMD_BT_AUTO_REPORT_ENABLE;
        temp_cmd[1] = 1;
        temp_cmd[2] = info->autoreport_enable;
        rtk_vendor_cmd_to_fw(HCI_VENDOR_MAILBOX_CMD, 3, temp_cmd);
    }

    uart_coex_info.polling_enable = info->polling_enable;
    uart_coex_info.polling_interval = info->polling_time;
    uart_coex_info.autoreport = info->autoreport_enable;

    rtk_notify_info_to_wifi(HOST_RESPONSE, 0, NULL);
}

static void rtk_handle_bt_coex_control(uint8_t* p)
{
    uint8_t temp_cmd[20];
    uint8_t opcode, opcode_len, value, power_decrease, psd_mode, access_type;

    opcode = *p++;
    RTKBT_DBG("receive bt coex control event from wifi, opration is 0x%x", opcode);

    switch (opcode) {
    case BT_PATCH_VERSION_QUERY:
        rtk_notify_btpatch_version_to_wifi();
        break;

    case IGNORE_WLAN_ACTIVE_CONTROL:
        opcode_len = *p++;
        value = *p++;
        temp_cmd[0] = HCI_VENDOR_SUB_CMD_BT_ENABLE_IGNORE_WLAN_ACT_CMD;
        temp_cmd[1] = 1;
        temp_cmd[2] = value;
        rtk_vendor_cmd_to_fw(HCI_VENDOR_MAILBOX_CMD, 3, temp_cmd);
        break;

    case LNA_CONSTRAIN_CONTROL:
        opcode_len = *p++;
        value = *p++;
        temp_cmd[0] = HCI_VENDOR_SUB_CMD_SET_BT_LNA_CONSTRAINT;
        temp_cmd[1] = 1;
        temp_cmd[2] = value;
        rtk_vendor_cmd_to_fw(HCI_VENDOR_MAILBOX_CMD, 3, temp_cmd);
        break;

    case BT_POWER_DECREASE_CONTROL:
        opcode_len = *p++;
        power_decrease = *p++;
        temp_cmd[0] = HCI_VENDOR_SUB_CMD_WIFI_FORCE_TX_POWER_CMD;
        temp_cmd[1] = 1;
        temp_cmd[2] = power_decrease;
        rtk_vendor_cmd_to_fw(HCI_VENDOR_MAILBOX_CMD, 3, temp_cmd);
        break;

    case BT_PSD_MODE_CONTROL:
        opcode_len = *p++;
        psd_mode = *p++;
        temp_cmd[0] = HCI_VENDOR_SUB_CMD_SET_BT_PSD_MODE;
        temp_cmd[1] = 1;
        temp_cmd[2] = psd_mode;
        rtk_vendor_cmd_to_fw(HCI_VENDOR_MAILBOX_CMD, 3, temp_cmd);
        break;

    case WIFI_BW_CHNL_NOTIFY:
        opcode_len = *p++;
        temp_cmd[0] = HCI_VENDOR_SUB_CMD_WIFI_CHANNEL_AND_BANDWIDTH_CMD;
        temp_cmd[1] = 3;
        memcpy(temp_cmd+2, p, 3);//wifi_state, wifi_centralchannel, chnnels_btnotuse
        rtk_vendor_cmd_to_fw(HCI_VENDOR_MAILBOX_CMD, 5, temp_cmd);
        break;

    case QUERY_BT_AFH_MAP:
        opcode_len = *p++;
        uart_coex_info.piconet_id = *p++;
        uart_coex_info.mode = *p++;
        temp_cmd[0] = HCI_VENDOR_SUB_CMD_GET_AFH_MAP_L;
        temp_cmd[1] = 2;
        temp_cmd[2] = uart_coex_info.piconet_id;
        temp_cmd[3] = uart_coex_info.mode;
        rtk_vendor_cmd_to_fw(HCI_VENDOR_MAILBOX_CMD, 4, temp_cmd);
        break;

    case BT_REGISTER_ACCESS:
        opcode_len = *p++;
        access_type = *p++;
        if(access_type == 0) {  //read
            temp_cmd[0] = HCI_VENDOR_SUB_CMD_RD_REG_REQ;
            temp_cmd[1] = 5;
            temp_cmd[2] = *p++;
            memcpy(temp_cmd+3, p, 4);
            rtk_vendor_cmd_to_fw(HCI_VENDOR_MAILBOX_CMD, 7, temp_cmd);
        } else {    //write
            temp_cmd[0] = HCI_VENDOR_SUB_CMD_RD_REG_REQ;
            temp_cmd[1] = 5;
            temp_cmd[2] = *p++;
            memcpy(temp_cmd+3, p, 8);
            rtk_vendor_cmd_to_fw(HCI_VENDOR_MAILBOX_CMD, 11, temp_cmd);
        }
        break;

    default:
        break;
    }
}

static void rtk_handle_event_from_wifi(uint8_t* msg)
{
    uint8_t *p = msg;
    uint8_t event_code = *p++;
    uint8_t total_length;
    uint8_t extension_event;
    uint8_t operation;
    uint16_t wifi_opcode;
    uint8_t op_status;

    if(memcmp(msg, invite_rsp, sizeof(invite_rsp)) == 0) {
        RTKBT_DBG("receive invite rsp from wifi, wifi is already on");
        uart_coex_info.wifi_on = 1;
        rtk_notify_extension_version_to_wifi();
    }

    if(memcmp(msg, attend_req, sizeof(attend_req)) == 0) {
        RTKBT_DBG("receive attend req from wifi, wifi turn on");
        uart_coex_info.wifi_on = 1;
        udpsocket_send(attend_ack, sizeof(attend_ack));
        rtk_notify_extension_version_to_wifi();
    }

    if(memcmp(msg, wifi_leave, sizeof(wifi_leave)) == 0) {
        RTKBT_DBG("receive wifi leave from wifi, wifi turn off");
        uart_coex_info.wifi_on = 0;
        udpsocket_send(leave_ack, sizeof(leave_ack));
        if(uart_coex_info.polling_enable) {
            uart_coex_info.polling_enable = 0;
            del_timer(&(uart_coex_info.polling_timer));
        }
    }

    if(memcmp(msg, leave_ack, sizeof(leave_ack)) == 0) {
        RTKBT_DBG("receive leave ack from wifi");
    }

    if(event_code == 0xFE) {
        total_length = *p++;
        extension_event = *p++;
        switch(extension_event) {
        case  RTK_HS_EXTENSION_EVENT_WIFI_SCAN:
            operation = *p;
            RTKBT_DBG("receive wifi scan notify evnet from wifi, operation is 0x%x", operation);
            break;

        case  RTK_HS_EXTENSION_EVENT_HCI_BT_INFO_CONTROL:
            rtk_handle_bt_info_control(p);
            break;

        case RTK_HS_EXTENSION_EVENT_HCI_BT_COEX_CONTROL:
            rtk_handle_bt_coex_control(p);
            break;

        default:
            break;
        }
    }

    if(event_code == 0x0E) {
        p += 2;//length, number of complete packets
        STREAM_TO_UINT16(wifi_opcode, p);
        op_status = *p;
        RTKBT_DBG("receive command complete event from wifi, op code is 0x%x, status is 0x%x", wifi_opcode, op_status);
    }
}

void rtk_uart_coex_open(struct hci_dev *hdev)
{
    uart_coex_info.num_hci_cmd_packet = 1;
    INIT_DELAYED_WORK(&uart_coex_info.fw_work, (void *)rtk_parse_event_data);
    INIT_DELAYED_WORK(&uart_coex_info.sock_work, (void *)udpsocket_recv_data);

    uart_coex_info.hdev = hdev;
    uart_coex_info.wifi_on = 0;
    skb_queue_head_init(&uart_coex_info.cmd_q);
    tasklet_init(&uart_coex_info.cmd_task, hci_cmd_task, (unsigned long)hdev);

    init_profile_hash(&uart_coex_info);
    init_connection_hash(&uart_coex_info);
    create_udpsocket();
    udpsocket_send(invite_req, sizeof(invite_req));
}

void rtk_uart_coex_close(void)
{
    int kk = 0;
    uart_coex_info.num_hci_cmd_packet = 1;
    skb_queue_purge(&uart_coex_info.cmd_q);
    tasklet_kill(&uart_coex_info.cmd_task);
    if(uart_coex_info.wifi_on)
        udpsocket_send(bt_leave, sizeof(bt_leave));

    if(uart_coex_info.polling_enable) {
        uart_coex_info.polling_enable = 0;
        del_timer(&(uart_coex_info.polling_timer));
    }

    del_timer(&(uart_coex_info.a2dp_count_timer));
    del_timer(&(uart_coex_info.pan_count_timer));

    cancel_delayed_work(&uart_coex_info.sock_work);
    cancel_delayed_work(&uart_coex_info.fw_work);

    if(uart_coex_info.sock_open) {
        uart_coex_info.sock_open = 0;
        RTKBT_DBG("release udp socket");
        sock_release(uart_coex_info.udpsock);
    }

    flush_connection_hash(&uart_coex_info);
    flush_profile_hash(&uart_coex_info);
    uart_coex_info.profile_bitmap = 0;
    uart_coex_info.profile_status = 0;
    for(kk= 0; kk < 8; kk++)
        uart_coex_info.profile_refcount[kk] = 0;
}

void rtk_uart_coex_probe(struct hci_dev *hdev)
{
    uart_coex_info.hdev = hdev;
    skb_queue_head_init(&uart_coex_info.cmd_q);
    tasklet_init(&uart_coex_info.cmd_task, hci_cmd_task, (unsigned long)hdev);
    spin_lock_init(&uart_coex_info.spin_lock_sock);
	spin_lock_init(&uart_coex_info.spin_lock_profile);
}

void rtk_uart_coex_init(void)
{
    RTKBT_DBG("rtk_uart_coex_init, version: %s", RTK_VERSION);
    RTKBT_DBG("create workqueue");
    uart_coex_info.sock_wq = create_workqueue("btudpwork");
    uart_coex_info.fw_wq = create_workqueue("btfwwork");
}

void rtk_uart_coex_exit(void)
{
    RTKBT_DBG("destroy workqueue");
    flush_workqueue(uart_coex_info.sock_wq);
    destroy_workqueue(uart_coex_info.sock_wq);
    flush_workqueue(uart_coex_info.fw_wq);
    destroy_workqueue(uart_coex_info.fw_wq);
}
