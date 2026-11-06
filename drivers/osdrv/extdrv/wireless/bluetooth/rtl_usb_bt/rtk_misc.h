/*
 *
 *  Realtek Bluetooth USB download firmware driver
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/pm_runtime.h>
#include <linux/usb.h>
#include <linux/suspend.h>

/* Download LPS patch when host suspends or power off
 *   LPS patch name:  lps_rtl8xxx_fw
 *   LPS config name: lps_rtl8xxx_config
 * Download normal patch when host resume or power on */
/* #define RTKBT_SWITCH_PATCH */

/* RTKBT Power-on for sideband wake-up by LE Advertising from Remote. */
/* Note that it's necessary to apply TV FW Patch. */
/* #define RTKBT_SUSPEND_WAKEUP */
/* #define RTKBT_SHUTDOWN_WAKEUP */
#define RTKBT_POWERKEY_WAKEUP

/* RTKBT Power-on Whitelist for sideband wake-up by LE Advertising from Remote.
 * Note that it's necessary to apply TV FW Patch. */
/* #define RTKBT_TV_POWERON_WHITELIST */

#if 1
#define RTKBT_DBG(fmt, arg...) printk(KERN_DEBUG "rtk_btusb: " fmt "\n" , ## arg)
#define RTKBT_INFO(fmt, arg...) printk(KERN_INFO "rtk_btusb: " fmt "\n" , ## arg)
#define RTKBT_WARN(fmt, arg...) printk(KERN_WARNING "rtk_btusb: " fmt "\n", ## arg)
#else
#define RTKBT_DBG(fmt, arg...)
#endif

#if 1
#define RTKBT_ERR(fmt, arg...) printk(KERN_ERR "rtk_btusb: " fmt "\n" , ## arg)
#else
#define RTKBT_ERR(fmt, arg...)
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 33)
#define USB_RPM
#endif

#define CONFIG_NEEDS_BINDING

/* If module is still powered when kernel suspended, there is no re-binding. */
#ifdef RTKBT_SWITCH_PATCH
#undef CONFIG_NEEDS_BINDING
#endif

/* USB SS */
#if (defined CONFIG_BTUSB_AUTOSUSPEND) && (defined USB_RPM)
#define BTUSB_RPM
#endif

#define PRINT_CMD_EVENT			0
#define PRINT_ACL_DATA			0

extern int patch_add(struct usb_interface *intf);
extern void patch_remove(struct usb_interface *intf);
extern int download_patch(struct usb_interface *intf);
extern void print_event(struct sk_buff *skb);
extern void print_command(struct sk_buff *skb);
extern void print_acl(struct sk_buff *skb, int dataOut);

#if defined RTKBT_SWITCH_PATCH || defined RTKBT_TV_POWERON_WHITELIST
int __rtk_send_hci_cmd(struct usb_device *udev, u8 *buf, u16 size);
#endif

#ifdef RTKBT_SWITCH_PATCH
#define RTLBT_CLOSE	(1 << 0)
struct api_context {
	u32			flags;
	struct completion	done;
	int			status;
};

int download_special_patch(struct usb_interface *intf, const char *special_name);
#endif

int setup_btrealtek_flag(struct usb_interface *intf, struct hci_dev *hdev);

enum {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
	REALTEK_ALT6_CONTINUOUS_TX_CHIP,
#endif

	__REALTEK_NUM_FLAGS,
};

struct btrealtek_data {
	DECLARE_BITMAP(flags, __REALTEK_NUM_FLAGS);
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
static inline void *hci_get_priv(struct hci_dev *hdev)
{
	return (char *)hdev + sizeof(*hdev);
}
#endif

#define btrealtek_set_flag(hdev, nr)					\
	do {								\
		struct btrealtek_data *realtek = hci_get_priv((hdev));	\
		set_bit((nr), realtek->flags);				\
	} while (0)

#define btrealtek_get_flag(hdev)						\
	(((struct btrealtek_data *)hci_get_priv(hdev))->flags)

#define btrealtek_test_flag(hdev, nr)	test_bit((nr), btrealtek_get_flag(hdev))

#if defined RTKBT_SUSPEND_WAKEUP || defined RTKBT_SHUTDOWN_WAKEUP || defined RTKBT_SWITCH_PATCH
int set_scan(struct usb_interface *intf);
#endif
