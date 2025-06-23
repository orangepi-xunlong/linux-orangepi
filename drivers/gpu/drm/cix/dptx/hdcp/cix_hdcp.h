// SPDX-License-Identifier: GPL-2.0
/*
 * display port hdcp driver for the cix ec
 *
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#ifndef __CIX_HDCP_H_
#define __CIX_HDCP_H_

#include <linux/device.h>
#include <linux/miscdevice.h>
#include <drm/drm_device.h>
#include <drm/drm_connector.h>
#include <drm/display/drm_dp_helper.h>

#define CIX_HDCP_DEBUG 0
//------------------------------------------------------------------------------
//  States
//------------------------------------------------------------------------------
//  State   Description
//------------------------------------------------------------------------------
//  H0      No RX Attached
//  H1      Transmit Low Value Content
//  A0      Determine Rx HDCP Capable
//  A1      Exchange km
//  A2      Locality Check
//  A3      Exchange ks
//  A4      Test for Repeater
//  A5      Authenticated
//  A6      Wait for Receiver ID list
//  A7      Verify Receiver ID List
//  A8      Send Receiver ID List acknowledgment
//  A9      Content Steam Management
//------------------------------------------------------------------------------
typedef enum {
ST2_H0,		//  H0: No RX Attached
ST2_H1,		//  H1: Transmit Low Value Content
ST2_A0,		//  A0: Determine Rx HDCP Capable
ST2_A1,		//  A1: Exchange km
ST2_A2,		//  A2: Locality Check
ST2_A3,		//  A3: Exchange ks
ST2_A4,		//  A4: Test for Repeater
ST2_A5,		//  A5: Authenticated
ST2_A6,		//  A6: Wait for Receiver ID list
ST2_A7,		//  A7: Verify Receiver ID List
ST2_A8,		//  A8: Send Receiver ID List acknowledgment
ST2_A9,		//  A9: Content Steam Management
ST2_TX_CT	//  state count (must be last)
} TX2_STATE_T;

//------------------------------------------------------------------------------
//  Events
//------------------------------------------------------------------------------
//  Start       End     Event                       Notes
//------------------------------------------------------------------------------
//  Any State   H0      EV2_TX_RESET                state machine reset
//  Any State   H0      EV2_TX_RX_DISCONNECT        receiver disconnect
//  H0          H1      EV2_TX_RX_CONNECT           receiver connect indication
//  Any State   H1      EV2_TX_NOT_HDCP_CAPABLE     Not HDCP capable
//  Any State   H1      EV2_TX_FAIL_AUTH            Authentication fail
//  H1          A0      EV2_TX_CP_DESIRED           CP desired
//  A0          A1      EV2_TX_HDCP_CAPABLE         HDCP Capable
//  A0          H1      EV2_TX_NOT_HDCP_CAPABLE     Not HDCP Capable
//  A1          A2      EV2_TX_DONE                 Completed A1 processing
//  A1          H1      EV2_TX_FAIL                 Failed A1 processing
//  A2          A3      EV2_TX_DONE                 Completed A2 processing
//  A2          H1      EV2_TX_FAIL                 Failed A2 processing
//  A3          A4      EV2_TX_DONE                 Completed A3 processing
//  A4          A5      EV2_TX_NOT_REPEATER         Not an HDCP Repeater
//  A4          A6      EV2_TX_REPEATER             HDCP Repeater
//  A5          H1      EV2_TX_REAUTH_REQ           REAUTH REQ
//  A5          H1      EV2_TX_INTEGRITY_FAILURE    LINK_INTEGRITY_FAILURE received
//  A5          A7      EV2_TX_READY                READY asserted
//  A6          A7      EV2_TX_READY                READY
//  A6          H1      EV2_TX_TIMEOUT              Wait for receiver ID list
//  A7          H1      EV2_TX_FAIL                 Verify Receiver ID List
//  A7          A8      EV2_TX_DONE                 Receiver ID List: verified
//  A8          A9      EV2_TX_DONE                 Receiver ID List ACK: sent
//  A9          A5      EV2_TX_DONE                 Stream management done (succeed or fail)
//  A9          H1      EV2_TX_ROLL_OVER            seq_num_M roll-over
//------------------------------------------------------------------------------
typedef enum {
    EV2_TX_TIMER,			//  Timer
    EV2_TX_RESET,			//  Reset
    EV2_TX_RX_DISCONNECT,		//  Receiver disconnect indication
    EV2_TX_RX_CONNECT,			//  Receiver connect indication
    EV2_TX_FAIL_AUTH,			//  Fail Authentication
    EV2_TX_REAUTH_REQ,			//  Reauthorization request
    EV2_TX_CP_DESIRED,			//  Content Protection desired
    EV2_TX_HDCP_CAPABLE,		//  HDCP capable
    EV2_TX_NOT_HDCP_CAPABLE,		//  Not HDCP capable
    EV2_TX_HPRIME_AVAILABLE,		//  H' available
    EV2_TX_PAIRING_AVAILABLE,		//  Pairing data available
    EV2_TX_DONE,			//  Done
    EV2_TX_REPEATER,			//  HDCP Repeater
    EV2_TX_NOT_REPEATER,		//  Not HDCP Repeater
    EV2_TX_READY,			//  Ready
    EV2_TX_FAIL,			//  Fail
    EV2_TX_TIMEOUT,			//  Timeout
    EV2_TX_INTEGRITY_FAILURE,		//  Integrity Failure
    EV2_TX_ABORT_AUTHENTICATION,	//  Abort authentication
    EV2_TX_DPCD_WRITE_COMPLETE,		//  DPCD data write succeeded
    EV2_TX_DPCD_WRITE_FAIL,		//  DPCD data write failed
    EV2_TX_DPCD_READ_COMPLETE,		//  DPCD data read succeeded
    EV2_TX_DPCD_READ_FAIL,		//  DPCD data write failed
    EV2_TX_CP_IRQ,			//  Received a CP_IRQ
    EV2_TX_CT				//  event count (must be last)
} TX2_EVENT_T;

struct hdcp_event {
	struct list_head list;
	unsigned int event;
};

struct cix_hdcp {
	char name[20];
	struct list_head list;
	struct miscdevice misc;
	struct drm_dp_aux *aux;
	void __iomem *base_addr;
	void __iomem *tmr_addr;
	bool opened;
	bool timer_in_use;
	int hdcp_capable;
	int hdcp_version;
	int hdcp_repeater;
	TX2_STATE_T state;
	spinlock_t event_lock;
	struct list_head event_list;
	wait_queue_head_t event_wait;
	struct mutex mutex;
};

int cix_hdcp_init(struct cix_hdcp *hdcp);
int cix_hdcp_uninit(struct cix_hdcp *hdcp);
int cix_hdcp_timer_process(struct cix_hdcp *hdcp);
int cix_hdcp_cp_irq_process(struct cix_hdcp *hdcp, u8 rx_status);
int cix_hdcp_hpd_event_process(struct cix_hdcp *hdcp, bool plugged);

#endif // __CIX_HDCP_H_
