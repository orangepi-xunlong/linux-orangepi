// SPDX-License-Identifier: GPL-2.0+
/*
 *	webcam.c -- USB webcam gadget driver
 *
 *	Copyright (C) 2009-2010
 *	    Laurent Pinchart (laurent.pinchart@ideasonboard.com)
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <uapi/linux/usb/sunxi_uapi_video.h>

#include "../function/u_uvc.h"

USB_GADGET_COMPOSITE_OPTIONS();

/*-------------------------------------------------------------------------*/

/* module parameters specific to the Video streaming endpoint */
static unsigned int streaming_interval = 1;
module_param(streaming_interval, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(streaming_interval, "1 - 16");

static unsigned int streaming_maxpacket = 1024;
module_param(streaming_maxpacket, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(streaming_maxpacket, "1 - 1023 (FS), 1 - 3072 (hs/ss)");

static unsigned int streaming_maxburst;
module_param(streaming_maxburst, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(streaming_maxburst, "0 - 15 (ss only)");

/* --------------------------------------------------------------------------
 * Device descriptor
 */

#define WEBCAM_VENDOR_ID		0x1d6b	/* Linux Foundation */
#define WEBCAM_PRODUCT_ID		0x0102	/* Webcam A/V gadget */
#define WEBCAM_DEVICE_BCD		0x0010	/* 0.10 */

static char webcam_vendor_label[] = "Linux Foundation";
static char webcam_product_label[] = "Webcam gadget";
static char webcam_config_label[] = "Video";

/* string IDs are assigned dynamically */

#define STRING_DESCRIPTION_IDX		USB_GADGET_FIRST_AVAIL_IDX

static struct usb_string webcam_strings[] = {
	[USB_GADGET_MANUFACTURER_IDX].s = webcam_vendor_label,
	[USB_GADGET_PRODUCT_IDX].s = webcam_product_label,
	[USB_GADGET_SERIAL_IDX].s = "",
	[STRING_DESCRIPTION_IDX].s = webcam_config_label,
	{  }
};

static struct usb_gadget_strings webcam_stringtab = {
	.language = 0x0409,	/* en-us */
	.strings = webcam_strings,
};

static struct usb_gadget_strings *webcam_device_strings[] = {
	&webcam_stringtab,
	NULL,
};

struct webcam_context {
	int id;
	int enable;
	struct usb_function_instance *fi_uvc;
	struct usb_function *f_uvc;
	char fi_uvc_control_string[64];
	char fi_uvc_streaming_string[64];
};
static struct webcam_context contexts[] = {
	{
		.id     = 0,
		.enable = 1,
		.fi_uvc = NULL,
		.f_uvc  = NULL,
		.fi_uvc_control_string = {'U', 'V', 'C', ' ', 'C',
				'a', 'm', 'e', 'r', 'a' },
		.fi_uvc_streaming_string = {'V', 'i', 'd', 'e', 'o', ' ', 'S',
				't', 'r', 'e', 'a', 'm', 'i', 'n', 'g' },
	},
	{
		.id     = 1,
		.enable = 0,
		.fi_uvc = NULL,
		.f_uvc  = NULL,
		.fi_uvc_control_string = {'U', 'V', 'C', ' ', 'C',
				'a', 'm', 'e', 'r', 'a', ' ', '1' },
		.fi_uvc_streaming_string = {'V', 'i', 'd', 'e', 'o', ' ', 'S',
				't', 'r', 'e', 'a', 'm', 'i', 'n', 'g', ' ', '1' },
	},
};

static struct usb_device_descriptor webcam_device_descriptor = {
	.bLength		= USB_DT_DEVICE_SIZE,
	.bDescriptorType	= USB_DT_DEVICE,
	/* .bcdUSB = DYNAMIC */
	.bDeviceClass		= USB_CLASS_MISC,
	.bDeviceSubClass	= 0x02,
	.bDeviceProtocol	= 0x01,
	.bMaxPacketSize0	= 0, /* dynamic */
	.idVendor		= cpu_to_le16(WEBCAM_VENDOR_ID),
	.idProduct		= cpu_to_le16(WEBCAM_PRODUCT_ID),
	.bcdDevice		= cpu_to_le16(WEBCAM_DEVICE_BCD),
	.iManufacturer		= 0, /* dynamic */
	.iProduct		= 0, /* dynamic */
	.iSerialNumber		= 0, /* dynamic */
	.bNumConfigurations	= 0, /* dynamic */
};

DECLARE_UVC_HEADER_DESCRIPTOR(1);

static const struct UVC_HEADER_DESCRIPTOR(1) uvc_control_header = {
	.bLength		= UVC_DT_HEADER_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_HEADER,
	.bcdUVC			= cpu_to_le16(0x0110),
	.wTotalLength		= 0, /* dynamic */
	.dwClockFrequency	= cpu_to_le32(48000000),
	.bInCollection		= 0, /* dynamic */
	.baInterfaceNr[0]	= 0, /* dynamic */
};

static const struct uvc_camera_terminal_descriptor uvc_camera_terminal = {
	.bLength		= UVC_DT_CAMERA_TERMINAL_SIZE(3),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_INPUT_TERMINAL,
	.bTerminalID		= 1,
	.wTerminalType		= cpu_to_le16(0x0201),
	.bAssocTerminal		= 0,
	.iTerminal		= 0,
	.wObjectiveFocalLengthMin	= cpu_to_le16(0),
	.wObjectiveFocalLengthMax	= cpu_to_le16(0),
	.wOcularFocalLength		= cpu_to_le16(0),
	.bControlSize		= 3,
	.bmControls[0]		= 0,
	.bmControls[1]		= 0,
	.bmControls[2]		= 0,
};

static const struct uvc_processing_unit_descriptor uvc_processing = {
	.bLength		= UVC_DT_PROCESSING_UNIT_SIZE(2),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_PROCESSING_UNIT,
	.bUnitID		= 2,
	.bSourceID		= 1,
	.wMaxMultiplier		= cpu_to_le16(16*1024),
	.bControlSize		= 2,
	.bmControls[0]		= 0,
	.bmControls[1]		= 0,
	.iProcessing		= 0,
	.bmVideoStandards	= 0,
};

static const struct uvc_output_terminal_descriptor uvc_output_terminal = {
	.bLength		= UVC_DT_OUTPUT_TERMINAL_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VC_OUTPUT_TERMINAL,
	.bTerminalID		= 3,
	.wTerminalType		= cpu_to_le16(0x0101),
	.bAssocTerminal		= 0,
	.bSourceID		= 2,
	.iTerminal		= 0,
};

DECLARE_UVC_INPUT_HEADER_DESCRIPTOR(1, 4);

static const struct UVC_INPUT_HEADER_DESCRIPTOR(1, 4) uvc_input_header = {
	.bLength		= UVC_DT_INPUT_HEADER_SIZE(1, 4),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_INPUT_HEADER,
	.bNumFormats		= 4,
	.wTotalLength		= 0, /* dynamic */
	.bEndpointAddress	= 0, /* dynamic */
	.bmInfo			= 0,
	.bTerminalLink		= 3,
	.bStillCaptureMethod	= 0,
	.bTriggerSupport	= 0,
	.bTriggerUsage		= 0,
	.bControlSize		= 1,
	.bmaControls[0][0]	= 0,
	.bmaControls[1][0]	= 0,
	.bmaControls[2][0]	= 0,
	.bmaControls[3][0]	= 0,
};

static const struct uvc_format_mjpeg uvc_format_mjpg = {
	.bLength		= UVC_DT_FORMAT_MJPEG_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_MJPEG,
	.bFormatIndex		= 1,
	.bNumFrameDescriptors	= 1,
	.bmFlags		= 0,
	.bDefaultFrameIndex	= 1,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};

DECLARE_UVC_FRAME_MJPEG(1);

static const struct UVC_FRAME_MJPEG(1) uvc_frame_mjpg_480p = {
	.bLength		= UVC_DT_FRAME_MJPEG_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_MJPEG,
	.bFrameIndex		= 1,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(640),
	.wHeight		= cpu_to_le16(480),
	.dwMinBitRate		= cpu_to_le32(10485760),
	.dwMaxBitRate		= cpu_to_le32(10485760),
	.dwMaxVideoFrameBufferSize	= cpu_to_le32(100000),
	.dwDefaultFrameInterval	= cpu_to_le32(500000),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= cpu_to_le32(500000),
};

static const struct uvc_format_uncompressed uvc_format_yuv = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= 2,
	.bNumFrameDescriptors	= 1,
	.guidFormat		= {
		'Y',  'U',  'Y',  '2', 0x00, 0x00, 0x10, 0x00,
		0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 16,
	.bDefaultFrameIndex	= 1,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};

DECLARE_UVC_FRAME_UNCOMPRESSED(1);

static const struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_yuv_240p = {
	.bLength		= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 1,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(320),
	.wHeight		= cpu_to_le16(240),
	.dwMinBitRate		= cpu_to_le32(3072000),
	.dwMaxBitRate		= cpu_to_le32(3072000),
	.dwMaxVideoFrameBufferSize	= cpu_to_le32(153600),
	.dwDefaultFrameInterval	= cpu_to_le32(500000),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= cpu_to_le32(500000),
};

static const struct uvc_format_frame_based uvc_format_h264 = {
	.bLength		= UVC_DT_FORMAT_FRAME_BASED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_FRAME_BASED,
	.bFormatIndex		= 3,
	.bNumFrameDescriptors	= 1,
	.guidFormat		= {'H', '2', '6', '4', 0x00, 0x00, 0x10, 0x00,
					0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 0,
	.bDefaultFrameIndex	= 1,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
	.bVariableSize		= 1,
};

DECLARE_UVC_FRAME_FRAME_BASED(1);

static const struct UVC_FRAME_FRAME_BASED(1) uvc_frame_h264_720p = {
	.bLength		= UVC_DT_FRAME_FRAME_BASED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_FRAME_BASED,
	.bFrameIndex		= 1,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(1280),
	.wHeight		= cpu_to_le16(720),
	.dwMinBitRate		= cpu_to_le32(10485760),
	.dwMaxBitRate		= cpu_to_le32(10485760),
	.dwDefaultFrameInterval	= cpu_to_le32(500000),
	.bFrameIntervalType	= 1,
	.dwBytesPerline         = 0,
	.dwFrameInterval[0]	= cpu_to_le32(500000),
};

static const struct uvc_format_uncompressed uvc_format_nv12 = {
	.bLength		= UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= 4,
	.bNumFrameDescriptors	= 1,
	.guidFormat		= {
		'N',  'V',  '1',  '2', 0x00, 0x00, 0x10, 0x00,
		0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 12,
	.bDefaultFrameIndex	= 1,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};

static const struct UVC_FRAME_UNCOMPRESSED(1) uvc_frame_nv12_240p = {
	.bLength		= UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 1,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(320),
	.wHeight		= cpu_to_le16(240),
	.dwMinBitRate		= cpu_to_le32(2304000),
	.dwMaxBitRate		= cpu_to_le32(2304000),
	.dwMaxVideoFrameBufferSize	= cpu_to_le32(115200),
	.dwDefaultFrameInterval	= cpu_to_le32(500000),
	.bFrameIntervalType	= 1,
	.dwFrameInterval[0]	= cpu_to_le32(500000),
};

static const struct uvc_color_matching_descriptor uvc_color_matching = {
	.bLength		= UVC_DT_COLOR_MATCHING_SIZE,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= UVC_VS_COLORFORMAT,
	.bColorPrimaries	= 1,
	.bTransferCharacteristics	= 1,
	.bMatrixCoefficients	= 4,
};

static const struct uvc_descriptor_header * const uvc_fs_control_cls[] = {
	(const struct uvc_descriptor_header *) &uvc_control_header,
	(const struct uvc_descriptor_header *) &uvc_camera_terminal,
	(const struct uvc_descriptor_header *) &uvc_processing,
	(const struct uvc_descriptor_header *) &uvc_output_terminal,
	NULL,
};

static const struct uvc_descriptor_header * const uvc_ss_control_cls[] = {
	(const struct uvc_descriptor_header *) &uvc_control_header,
	(const struct uvc_descriptor_header *) &uvc_camera_terminal,
	(const struct uvc_descriptor_header *) &uvc_processing,
	(const struct uvc_descriptor_header *) &uvc_output_terminal,
	NULL,
};

static const struct uvc_descriptor_header * const uvc_fs_streaming_cls[] = {
	(const struct uvc_descriptor_header *) &uvc_input_header,
	(const struct uvc_descriptor_header *) &uvc_format_mjpg,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_480p,
	(const struct uvc_descriptor_header *) &uvc_format_yuv,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_240p,
	(const struct uvc_descriptor_header *) &uvc_format_h264,
	(const struct uvc_descriptor_header *) &uvc_frame_h264_720p,
	(const struct uvc_descriptor_header *) &uvc_format_nv12,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_240p,
	(const struct uvc_descriptor_header *) &uvc_color_matching,
	NULL,
};

static const struct uvc_descriptor_header * const uvc_hs_streaming_cls[] = {
	(const struct uvc_descriptor_header *) &uvc_input_header,
	(const struct uvc_descriptor_header *) &uvc_format_mjpg,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_480p,
	(const struct uvc_descriptor_header *) &uvc_format_yuv,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_240p,
	(const struct uvc_descriptor_header *) &uvc_format_h264,
	(const struct uvc_descriptor_header *) &uvc_frame_h264_720p,
	(const struct uvc_descriptor_header *) &uvc_format_nv12,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_240p,
	(const struct uvc_descriptor_header *) &uvc_color_matching,
	NULL,
};

static const struct uvc_descriptor_header * const uvc_ss_streaming_cls[] = {
	(const struct uvc_descriptor_header *) &uvc_input_header,
	(const struct uvc_descriptor_header *) &uvc_format_mjpg,
	(const struct uvc_descriptor_header *) &uvc_frame_mjpg_480p,
	(const struct uvc_descriptor_header *) &uvc_format_yuv,
	(const struct uvc_descriptor_header *) &uvc_frame_yuv_240p,
	(const struct uvc_descriptor_header *) &uvc_format_h264,
	(const struct uvc_descriptor_header *) &uvc_frame_h264_720p,
	(const struct uvc_descriptor_header *) &uvc_format_nv12,
	(const struct uvc_descriptor_header *) &uvc_frame_nv12_240p,
	(const struct uvc_descriptor_header *) &uvc_color_matching,
	NULL,
};

/* --------------------------------------------------------------------------
 * USB configuration
 */

static int
webcam_config_bind(struct usb_configuration *c)
{
	int status = 0;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(contexts); ++i) {
		if (!contexts[i].enable)
			continue;

		contexts[i].f_uvc = usb_get_function(contexts[i].fi_uvc);
		if (IS_ERR(contexts[i].f_uvc))
			return PTR_ERR(contexts[i].f_uvc);

		status = usb_add_function(c, contexts[i].f_uvc);
		if (status < 0)
			usb_put_function(contexts[i].f_uvc);
	}

	return status;
}

static struct usb_configuration webcam_config_driver = {
	.label			= webcam_config_label,
	.bConfigurationValue	= 1,
	.iConfiguration		= 0, /* dynamic */
	.bmAttributes		= USB_CONFIG_ATT_SELFPOWER,
	.MaxPower		= CONFIG_USB_GADGET_VBUS_DRAW,
};

static int
webcam_unbind(struct usb_composite_dev *cdev)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(contexts); ++i) {
		if (!contexts[i].enable)
			continue;
		if (!IS_ERR_OR_NULL(contexts[i].f_uvc))
			usb_put_function(contexts[i].f_uvc);
		if (!IS_ERR_OR_NULL(contexts[i].fi_uvc))
			usb_put_function_instance(contexts[i].fi_uvc);
	}

	return 0;
}

static int
webcam_bind(struct usb_composite_dev *cdev)
{
	struct f_uvc_opts *uvc_opts;
	int ret;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(contexts); ++i) {
		if (!contexts[i].enable)
			continue;
		contexts[i].fi_uvc = usb_get_function_instance("uvc");
		if (IS_ERR(contexts[i].fi_uvc))
			return PTR_ERR(contexts[i].fi_uvc);
		uvc_opts = container_of(contexts[i].fi_uvc, struct f_uvc_opts, func_inst);
		uvc_opts->streaming_interval = streaming_interval;
		uvc_opts->streaming_maxpacket = streaming_maxpacket;
		uvc_opts->streaming_maxburst = streaming_maxburst;
		uvc_opts->fs_control = uvc_fs_control_cls;
		uvc_opts->ss_control = uvc_ss_control_cls;
		uvc_opts->fs_streaming = uvc_fs_streaming_cls;
		uvc_opts->hs_streaming = uvc_hs_streaming_cls;
		uvc_opts->ss_streaming = uvc_ss_streaming_cls;
		strncpy(uvc_opts->control_interface_strings, contexts[i].fi_uvc_control_string,
			sizeof(uvc_opts->control_interface_strings));
		strncpy(uvc_opts->streaming_interface_strings, contexts[i].fi_uvc_streaming_string,
			sizeof(uvc_opts->streaming_interface_strings));
	}

	/* Allocate string descriptor numbers ... note that string contents
	 * can be overridden by the composite_dev glue.
	 */
	ret = usb_string_ids_tab(cdev, webcam_strings);
	if (ret < 0)
		goto error;
	webcam_device_descriptor.iManufacturer =
		webcam_strings[USB_GADGET_MANUFACTURER_IDX].id;
	webcam_device_descriptor.iProduct =
		webcam_strings[USB_GADGET_PRODUCT_IDX].id;
	webcam_config_driver.iConfiguration =
		webcam_strings[STRING_DESCRIPTION_IDX].id;

	/* Register our configuration. */
	ret = usb_add_config(cdev, &webcam_config_driver,
					webcam_config_bind);
	if (ret < 0)
		goto error;

	usb_composite_overwrite_options(cdev, &coverwrite);
	INFO(cdev, "Webcam Video Gadget\n");
	return 0;

error:
	for (i = 0; i < ARRAY_SIZE(contexts); ++i) {
		if (!contexts[i].enable)
			continue;
		usb_put_function_instance(contexts[i].fi_uvc);
	}
	return ret;
}

/* --------------------------------------------------------------------------
 * Driver
 */

static struct usb_composite_driver webcam_driver = {
	.name		= "g_webcam",
	.dev		= &webcam_device_descriptor,
	.strings	= webcam_device_strings,
	.max_speed	= USB_SPEED_SUPER,
	.bind		= webcam_bind,
	.unbind		= webcam_unbind,
};

module_usb_composite_driver(webcam_driver);

MODULE_AUTHOR("Laurent Pinchart");
MODULE_DESCRIPTION("Webcam Video Gadget");
MODULE_LICENSE("GPL");

