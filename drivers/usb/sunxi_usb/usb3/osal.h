/*
 * drivers/usb/sunxi_usb/udc/usb3/osal.h
 * (C) Copyright 2013-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * wangjx, 2014-3-14, create this file
 *
 * usb3.0 contoller osal
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef __DRIVERS_USB_OSAL_H
#define __DRIVERS_USB_OSAL_H

/* Global constants */
#define SUNXI_EVENT_TYPE_DEV	0
#define SUNXI_EVENT_TYPE_CARKIT	3
#define SUNXI_EVENT_TYPE_I2C	4

#define SUNXI_DEVICE_EVENT_DISCONNECT		0
#define SUNXI_DEVICE_EVENT_RESET		1
#define SUNXI_DEVICE_EVENT_CONNECT_DONE		2
#define SUNXI_DEVICE_EVENT_LINK_STATUS_CHANGE	3
#define SUNXI_DEVICE_EVENT_WAKEUP		4
#define SUNXI_DEVICE_EVENT_EOPF			6
#define SUNXI_DEVICE_EVENT_SOF			7
#define SUNXI_DEVICE_EVENT_ERRATIC_ERROR	9
#define SUNXI_DEVICE_EVENT_CMD_CMPL		10
#define SUNXI_DEVICE_EVENT_OVERFLOW		11

#define SUNXI_GCTL_PRTCAP(n)	(((n) & (3 << 12)) >> 12)
#define SUNXI_GCTL_PRTCAP_HOST		1
#define SUNXI_GCTL_PRTCAP_DEVICE	2
#define SUNXI_GCTL_PRTCAP_OTG		3

#define SUNXI_DCFG_SPEED_MASK	(7 << 0)
#define SUNXI_DCFG_SUPERSPEED	(4 << 0)
#define SUNXI_DCFG_HIGHSPEED	(0 << 0)
#define SUNXI_DCFG_FULLSPEED2	(1 << 0)
#define SUNXI_DCFG_LOWSPEED	(2 << 0)
#define SUNXI_DCFG_FULLSPEED1	(3 << 0)

#define SUNXI_DCTL_TRGTULST_U2		(SUNXI_DCTL_TRGTULST(2))
#define SUNXI_DCTL_TRGTULST_U3		(SUNXI_DCTL_TRGTULST(3))
#define SUNXI_DCTL_TRGTULST_SS_DIS 	(SUNXI_DCTL_TRGTULST(4))

#define SUNXI_DSTS_SUPERSPEED		(4 << 0)

/* Device Endpoint Command Register */
#define SUNXI_DEPCMD_PARAM_SHIFT		16
#define SUNXI_DEPCMD_PARAM(x)		((x) << SUNXI_DEPCMD_PARAM_SHIFT)
#define SUNXI_DEPCMD_GET_RSC_IDX(x)     (((x) >> SUNXI_DEPCMD_PARAM_SHIFT) & 0x7f)

#define SUNXI_DEPCMD_STATUS_MASK		(0x0f << 12)
#define SUNXI_DEPCMD_STATUS(x)		(((x) & SUNXI_DEPCMD_STATUS_MASK) >> 12)
#define SUNXI_DEPCMD_HIPRI_FORCERM	(1 << 11)
#define SUNXI_DEPCMD_CMDACT		(1 << 10)
#define SUNXI_DEPCMD_CMDIOC		(1 << 8)

#define SUNXI_DEPCMD_DEPSTARTCFG		(0x09 << 0)
#define SUNXI_DEPCMD_ENDTRANSFER		(0x08 << 0)

#define SUNXI_DEPCMD_UPDATETRANSFER	(0x07 << 0)
#define SUNXI_DEPCMD_STARTTRANSFER	(0x06 << 0)
#define SUNXI_DEPCMD_CLEARSTALL		(0x05 << 0)
#define SUNXI_DEPCMD_SETSTALL		(0x04 << 0)
#define SUNXI_DEPCMD_GETSEQNUMBER	(0x03 << 0)
#define SUNXI_DEPCMD_SETTRANSFRESOURCE	(0x02 << 0)
#define SUNXI_DEPCMD_SETEPCONFIG		(0x01 << 0)


/* TRB Length, PCM and Status */
#define SUNXI_TRB_SIZE_MASK	(0x00ffffff)
#define SUNXI_TRB_SIZE_LENGTH(n)	((n) & SUNXI_TRB_SIZE_MASK)

/* TRB Control */
#define SUNXI_TRB_CTRL_HWO		(1 << 0)
#define SUNXI_TRB_CTRL_LST		(1 << 1)
#define SUNXI_TRB_CTRL_CHN		(1 << 2)
#define SUNXI_TRB_CTRL_CSP		(1 << 3)
#define SUNXI_TRB_CTRL_TRBCTL(n)	(((n) & 0x3f) << 4)
#define SUNXI_TRB_CTRL_ISP_IMI		(1 << 10)
#define SUNXI_TRB_CTRL_IOC		(1 << 11)
#define SUNXI_TRB_CTRL_SID_SOFN(n)	(((n) & 0xffff) << 14)

#define SUNXI_TRBCTL_NORMAL		SUNXI_TRB_CTRL_TRBCTL(1)
#define SUNXI_TRBCTL_CONTROL_SETUP	SUNXI_TRB_CTRL_TRBCTL(2)
#define SUNXI_TRBCTL_CONTROL_STATUS2	SUNXI_TRB_CTRL_TRBCTL(3)
#define SUNXI_TRBCTL_CONTROL_STATUS3	SUNXI_TRB_CTRL_TRBCTL(4)
#define SUNXI_TRBCTL_CONTROL_DATA	SUNXI_TRB_CTRL_TRBCTL(5)
#define SUNXI_TRBCTL_ISOCHRONOUS_FIRST	SUNXI_TRB_CTRL_TRBCTL(6)
#define SUNXI_TRBCTL_ISOCHRONOUS		SUNXI_TRB_CTRL_TRBCTL(7)
#define SUNXI_TRBCTL_LINK_TRB		SUNXI_TRB_CTRL_TRBCTL(8)

#define SUNXI_DEPEVT_XFERCOMPLETE	0x01
#define SUNXI_DEPEVT_XFERINPROGRESS	0x02
#define SUNXI_DEPEVT_XFERNOTREADY	0x03
#define SUNXI_DEPEVT_RXTXFIFOEVT	0x04
#define SUNXI_DEPEVT_STREAMEVT		0x06
#define SUNXI_DEPEVT_EPCMDCMPLT		0x07

/* HWPARAMS0 */
#define SUNXI_MODE(n)		((n) & 0x7)

#define SUNXI_MODE_DEVICE	0
#define SUNXI_MODE_HOST		1
#define SUNXI_MODE_DRD		2
#define SUNXI_MODE_HUB		3

#define SUNXI_MDWIDTH(n)	(((n) & 0xff00) >> 8)

/* HWPARAMS1 */
#define SUNXI_NUM_INT(n)	(((n) & (0x3f << 15)) >> 15)

/* HWPARAMS7 */
#define SUNXI_RAM1_DEPTH(n)	((n) & 0xffff)

#define SUNXI_EP_ENABLED	(1 << 0)
#define SUNXI_EP_STALL		(1 << 1)
#define SUNXI_EP_WEDGE		(1 << 2)
#define SUNXI_EP_BUSY		(1 << 4)
#define SUNXI_EP_PENDING_REQUEST	(1 << 5)

/* This last one is specific to EP0 */
#define SUNXI_EP0_DIR_IN		(1 << 31)

/*osal*/
void osal_mdelay(unsigned long n);
void osal_udelay(unsigned long n);
void osal_info( const char* format, ... );
void osal_err( const char* format, ... );

/*core*/
u32 get_sunxi_gctl_regs(void __iomem *regs);
u32 get_sunxi_dctl_tstctrl_mark(void __iomem *regs);
u32 get_sunxi_dsts_usblnkst_state(void __iomem *regs);
int sunxi_otgc_open_phy(void __iomem *regs);
int sunxi_otgc_close_phy(void __iomem *regs);
void sunxi_set_mode_ex(void __iomem *regs, u32 mode);
void sunxi_core_soft_reset(void __iomem *regs);
void sunxi_set_gevntadrlo(void __iomem *regs, int n, u32 value);
void sunxi_set_gevntadrhi(void __iomem *regs, int n, u32 value);
void sunxi_set_gevntsiz(void __iomem *regs, int n, u32 value);
void sunxi_set_gevntcount(void __iomem *regs, int n, u32 value);
void sunxi_event_buffers_cleanup_ex(void __iomem *regs , u32 n);
u32 sunxi_get_hwparams0(void __iomem *regs);
u32 sunxi_get_hwparams1(void __iomem *regs);
u32 sunxi_get_hwparams2(void __iomem *regs);
u32 sunxi_get_hwparams3(void __iomem *regs);
u32 sunxi_get_hwparams4(void __iomem *regs);
u32 sunxi_get_hwparams5(void __iomem *regs);
u32 sunxi_get_hwparams6(void __iomem *regs);
u32 sunxi_get_hwparams7(void __iomem *regs);
u32 sunxi_get_hwparams8(void __iomem *regs);
u32 sunxi_get_gsnpsid(void __iomem *regs);
u32 sunxi_get_dctl(void __iomem *regs);
void sunxi_set_dctl(void __iomem *regs, u32 value);
void sunxi_set_dctl_csftrst(void __iomem *regs);
u32 sunxi_is_dctl_csftrst(void __iomem *regs);
u32 sunxi_get_gctl(void __iomem *regs);
void sunxi_gctl_int(void __iomem *regs, u32 value, u32 revision);

/*gadget*/
int sunxi_gadget_enable_all_irq(void __iomem *regs);
void disable_sunxi_all_irq(void __iomem *regs);
u32 sunxi_get_ram1_depth(u32 value);
u32 sunxi_get_mdwidth(u32 value);
void sunxi_set_gtxfifosiz(void __iomem *regs, u32 fifo_number,  u32 fifo_size);
const char *sunxi_gadget_ep_cmd_string(u8 cmd);
void sunxi_set_param(void __iomem *regs, unsigned ep,  u32 param0, u32 param1, u32 param2);
int sunxi_send_gadget_ep_cmd_ex(void __iomem *regs, unsigned ep,
		unsigned cmd);
u32 sunxi_get_param0(int type, int maxp, int maxburst);
u32 sunxi_set_param1_en(void);
u32 sunxi_set_param1_bulk(void);
u32 sunxi_set_param1_iso(void);
u32  sunxi_depcfg_ep_number(u8 number);
u32  sunxi_depcfg_fifo_number(u8 number);
u32  sunxi_depcfg_binterval(u8 bInterval);
u32  sunxi_depcfg_num_xfer_res(u32 value);
void sunxi_enable_dalepena_ep(void __iomem *regs, u8 number);
void sunxi_disanble_dalepena_ep(void __iomem *regs, u8 number);
void sunxi_set_speed(void __iomem *regs, u32 speed);
u32 sunxi_gadget_get_frame_ex(void __iomem *regs);
u32 sunxi_get_dsts(void __iomem *regs);
u32 sunxi_get_speed(void __iomem *regs);
u32 sunxi_is_superspeed(void __iomem *regs);
u32 sunxi_get_usblnkst(void __iomem *regs);
void sunxi_clear_ulstchngreq(void __iomem *regs, u32 reg);
u32 sunxi_dsts_usblnkst(u32 reg);
void sunxi_gadget_run_stop_ex(void __iomem *regs, int is_on);
void sunxi_gadget_usb3_phy_power_ex(void __iomem *regs, int on);
void sunxi_gadget_usb2_phy_power_ex(void __iomem *regs, int on);
void sunxi_clear_dcfg_tstctrl_mask(void __iomem *regs);
void sunxi_clear_dcfg_devaddr_mask(void __iomem *regs);
void sunxi_update_ram_clk_sel_ex(void __iomem *regs, u32 speed);
u32  sunxi_link_state_u1u2(void __iomem *regs, u32 u1u2);
u32 sunxi_get_gevntcount(void __iomem *regs, u32 buf);
void sunxi_set_dcfg_devaddr(void __iomem *regs, u32 value);

u32 sunxi_gadget_ep_get_transfer_index(void __iomem *regs, u8 number);
const char *sunxi_gadget_event_string(u8 event);
const char *sunxi_ep_event_string(u8 event);
u32 sunxi_is_revision_183A(u32 revision);
u32 sunxi_is_revision_188A(u32 revision);
u32 sunxi_is_revision_190A(u32 revision);

int sunxi_gadget_set_test_mode(void __iomem *regs, int mode);
int sunxi_gadget_set_link_state(void __iomem *regs, u32 state);
int sunxi_otgc_base(void);

#endif /* __DRIVERS_USB_OSAL_H */
