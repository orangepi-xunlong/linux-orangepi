/**
 * sunxi-eh-test.c - SUNXI Embedded Host Test Support Driver
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2, as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/usb.h>

#include <linux/usb/ch11.h>
#include <linux/usb/ch9.h>

#define EHCI0_SYSFS_PATH "/devices/soc.0/1c1a000.ehci0-controller"
#define EHCI1_SYSFS_PATH "/devices/soc.0/1c1b000.ehci1-controller"
#define EHCI0_SUN8IW11_SYSFS_PATH "/devices/soc.0/1c14000.ehci0-controller"
#define EHCI1_SUN8IW11_SYSFS_PATH "/devices/soc.0/1c19000.ehci1-controller"
#define EHCI2_SUN8IW11_SYSFS_PATH "/devices/soc.0/1c1c000.ehci2-controller"

#define EHCI0_ED_TEST_PATH "/sys/bus/platform/devices/1c1a000.ehci0" \
			   "-controller/ed_test"
#define EHCI1_ED_TEST_PATH "/sys/bus/platform/devices/1c1b000.ehci1" \
			   "-controller/ed_test"
#define EHCI0_SUN8IW11_ED_TEST_PATH "/sys/bus/platform/devices/1c14000.ehci0" \
				    "-controller/ed_test"
#define EHCI1_SUN8IW11_ED_TEST_PATH "/sys/bus/platform/devices/1c19000.ehci1" \
				    "-controller/ed_test"
#define EHCI2_SUN8IW11_ED_TEST_PATH "/sys/bus/platform/devices/1c1c000.ehci2" \
				    "-controller/ed_test"

#define USB_IF_TEST_VID				0x1a0a

#define USB_IF_TEST_SE0_NAK			0x0101
#define USB_IF_TEST_J				0x0102
#define USB_IF_TEST_K				0x0103
#define USB_IF_TEST_PACKET			0x0104
#define USB_IF_HS_HOST_PORT_SUSPEND_RESUME	0x0106
#define USB_IF_SINGLE_STEP_GET_DEV_DESC		0x0107
#define USB_IF_SINGLE_STEP_GET_DEV_DESC_DATA	0x0108

#define USB_IF_PROTOCOL_OTG_ELECTRICAL_TEST	0x0200

#define BUFLEN 32

static char *sunxi_eh_get_ed_test_path(struct usb_interface *intf)
{
	char *udev_sysfs_path = NULL;
	char *ed_test_path = NULL;
	struct usb_device *udev;

	udev = interface_to_usbdev(intf);

	udev_sysfs_path = kobject_get_path(&udev->dev.kobj, GFP_KERNEL);

#if defined(CONFIG_ARCH_SUN50IW1P1)
	if (!strncmp(udev_sysfs_path, EHCI0_SYSFS_PATH,
		strlen(EHCI0_SYSFS_PATH))) /* usb0 */
		ed_test_path = EHCI0_ED_TEST_PATH;
	else if (!strncmp(udev_sysfs_path, EHCI1_SYSFS_PATH,
		strlen(EHCI1_SYSFS_PATH))) /* usb1 */
		ed_test_path = EHCI1_ED_TEST_PATH;

#elif defined(CONFIG_ARCH_SUN8IW10)
	if (!strncmp(udev_sysfs_path, EHCI0_SYSFS_PATH,
		strlen(EHCI0_SYSFS_PATH))) /* usb0 */
		ed_test_path = EHCI0_ED_TEST_PATH;

#elif defined(CONFIG_ARCH_SUN8IW11)
	if (!strncmp(udev_sysfs_path, EHCI0_SUN8IW11_SYSFS_PATH,
		strlen(EHCI0_SUN8IW11_SYSFS_PATH))) /* usb0 */
		ed_test_path = EHCI0_SUN8IW11_ED_TEST_PATH;
	else if (!strncmp(udev_sysfs_path, EHCI1_SUN8IW11_SYSFS_PATH,
		strlen(EHCI1_SUN8IW11_SYSFS_PATH))) /* usb1 */
		ed_test_path = EHCI1_SUN8IW11_ED_TEST_PATH;
	else if (!strncmp(udev_sysfs_path, EHCI2_SUN8IW11_SYSFS_PATH,
		strlen(EHCI2_SUN8IW11_SYSFS_PATH))) /* usb2 */
		ed_test_path = EHCI2_SUN8IW11_ED_TEST_PATH;

#endif

	return ed_test_path;
}

static void sunxi_eh_set_ed_test(struct usb_interface *intf, char *test_mode)
{
	struct file *filep;
	loff_t pos;
	char *ed_test_path = NULL;

	dev_info(&intf->dev, "sunxi_eh_set_ed_test, test_mode:%s\n", test_mode);

	ed_test_path = sunxi_eh_get_ed_test_path(intf);
	if (ed_test_path == NULL) {
		dev_err(&intf->dev, "ed test path is NULL\n");
		return;
	}

	filep=filp_open(ed_test_path, O_RDWR, 0);
	if (IS_ERR(filep)) {
		dev_err(&intf->dev, "open status fail\n");
		return;
	}

	pos = 0;
	vfs_write(filep, (char __user *)test_mode, BUFLEN, &pos);

	filp_close(filep, NULL);

	return;
}

static int sunxi_eh_probe(struct usb_interface *intf,
		const struct usb_device_id *id)
{
	struct usb_device *udev;

	udev = interface_to_usbdev(intf);

	switch (id->idProduct) {
	case USB_IF_TEST_SE0_NAK:
		/*
		 * Upon enumerating VID 0x1A0A/PID 0x0101, the host�s
		 * downstream port shall enter a high-speed receive mode as
		 * described in Section 7.1.20 [USB2.0] and drives an SE0 until
		 * the controller is reset.
		 */
		sunxi_eh_set_ed_test(intf, "test_se0_nak");
		break;
	case USB_IF_TEST_J:
		/*
		 * Upon enumerating VID 0x1A0A/PID 0x0102, the host�s
		 * downstream port shall enter a high-speed J state as
		 * described in Section 7.1.20 of [USB2.0] until the host
		 * controller is reset.
		 */
		sunxi_eh_set_ed_test(intf, "test_j_state");
		break;
	case USB_IF_TEST_K:
		/*
		 * Upon enumerating VID 0x1A0A/PID 0x0103, the host�s
		 * downstream port shall enter a high-speed K state as
		 * described in Section 7.1.20 of [USB2.0] until the host
		 * controller is reset.
		 */
		sunxi_eh_set_ed_test(intf, "test_k_state");
		break;
	case USB_IF_TEST_PACKET:
		/*
		 * Upon enumerating VID 0x1A0A/PID 0x0104, the host shall begin
		 * sending test packets as described in Section 7.1.20 of
		 * [USB2.0] until the host controller is reset.
		 */
		sunxi_eh_set_ed_test(intf, "test_pack");
		break;
	case USB_IF_HS_HOST_PORT_SUSPEND_RESUME:
		/*
		 * Upon enumerating VID:0x1A0A/PID 0x0106, the host shall
		 * continue sending SOFs for 15 seconds, then suspend the
		 * downstream port under test per Section 7.1.7.6.1 of
		 * [USB2.0]. After 15 seconds has elapsed, the host shall issue
		 * a ResumeK state on the bus, then continue sending SOFs.
		 */
		dev_err(&intf->dev, "Unsupport hs host port suspend resume\n");
		break;
	case USB_IF_SINGLE_STEP_GET_DEV_DESC:
		/*
		 * When the host discovers a device with VID:0x1A0A/PID 0x0107,
		 * the following steps are executed by the host and the device.
		 *
		 * 1. The host enumerates the test device, reads VID:0x1A0A/PID
		 * 0x0107, then completes its enumeration procedure.
		 *
		 * 2. The host issues SOFs for 15 seconds allowing the test
		 * engineer to raise the scope trigger just above the SOF
		 * voltage level.
		 *
		 * 3. The host sends a complete GetDescriptor(Device) transfer
		 *
		 * 4. The device ACKs the request, triggering the scope. (Note:
		 * SOFs continue.)
		 */
		dev_err(&intf->dev, "Unsupport single step get dev desc\n");
		break;
	case USB_IF_SINGLE_STEP_GET_DEV_DESC_DATA:
		/*
		 * When the host discovers a device with VID:0x1A0A/PID 0x0108,
		 * the following steps are executed by the host and the device.
		 *
		 * 1. The host enumerates the test device and reads
		 * VID:0x1A0A/PID 0x0108, then completes its enumeration
		 * procedure
		 *
		 * 2. After enumerating the device, the host sends
		 * GetDescriptor(Device)
		 *
		 * 3. The device ACKs the request
		 *
		 * 4. The host issues SOFs for 15 seconds allowing the test
		 * engineer to raise the scope trigger just above the SOF
		 * voltage level
		 *
		 * 5. The host sends an IN packet
		 *
		 * 6. The device sends data in response to the IN packet,
		 * triggering the scope
		 *
		 * 7. The host sends an ACK in response to the data. (Note:
		 * SOFs may follow the IN transaction).
		 */
		dev_err(&intf->dev, "Unsupport single step get dev desc data\n");
		break;
	case USB_IF_PROTOCOL_OTG_ELECTRICAL_TEST:
		/* OTG-A Device */
		dev_err(&intf->dev, "Unsupport protocol otg electrical test\n");
		break;
	default:
		dev_err(&intf->dev, "Unsupported device\n");
	}

	return 0;
}

static void sunxi_eh_disconnect(struct usb_interface *intf)
{
	return;
}

static const struct usb_device_id sunxi_eh_id_table[] = {
	{ USB_DEVICE(USB_IF_TEST_VID, USB_IF_TEST_SE0_NAK), },
	{ USB_DEVICE(USB_IF_TEST_VID, USB_IF_TEST_J), },
	{ USB_DEVICE(USB_IF_TEST_VID, USB_IF_TEST_K), },
	{ USB_DEVICE(USB_IF_TEST_VID, USB_IF_TEST_PACKET), },
	{ USB_DEVICE(USB_IF_TEST_VID, USB_IF_HS_HOST_PORT_SUSPEND_RESUME), },
	{ USB_DEVICE(USB_IF_TEST_VID, USB_IF_SINGLE_STEP_GET_DEV_DESC), },
	{ USB_DEVICE(USB_IF_TEST_VID, USB_IF_SINGLE_STEP_GET_DEV_DESC_DATA), },
	{ USB_DEVICE(USB_IF_TEST_VID, USB_IF_PROTOCOL_OTG_ELECTRICAL_TEST), },

	{ } /* Terminating Entry */
};
MODULE_DEVICE_TABLE(usb, sunxi_eh_id_table);

static struct usb_driver sunxi_eh_driver = {
	.name		= "sunxi-eh-test",
	.probe		= sunxi_eh_probe,
	.disconnect	= sunxi_eh_disconnect,
	.id_table	= sunxi_eh_id_table,
	.supports_autosuspend = true,
};

module_usb_driver(sunxi_eh_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("USB SUNXI EH Test Driver");
