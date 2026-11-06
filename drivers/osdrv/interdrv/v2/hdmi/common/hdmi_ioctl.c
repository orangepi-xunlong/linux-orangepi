#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <base_ctx.h>
#include <linux/version.h>
#include <linux/hdmi_uapi.h>

#include "hdmi_debug.h"
#include "core/hdmi_core.h"
#include "hdmi_ioctl.h"

long hdmitx_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	char stack_kdata[128];
	char *kdata = stack_kdata;
	int ret = 0;
	unsigned int in_size, out_size, drv_size, ksize;

	/* Figure out the delta between user cmd size and kernel cmd size */
	drv_size = _IOC_SIZE(cmd);
	out_size = _IOC_SIZE(cmd);
	in_size = out_size;
	if ((cmd & IOC_IN) == 0)
		in_size = 0;
	if ((cmd & IOC_OUT) == 0)
		out_size = 0;
	ksize = max(max(in_size, out_size), drv_size);

	/* If necessary, allocate buffer for ioctl argument */
	if (ksize > sizeof(stack_kdata)) {
		kdata = kmalloc(ksize, GFP_KERNEL);
		if (!kdata)
			return -ENOMEM;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	if (!access_ok((void __user *)arg, in_size)) {
		CVI_TRACE_HDMI(CVI_DBG_ERR, "access_ok failed\n");
	}
#else
	if (!access_ok(VERIFY_READ, (void __user *)arg, in_size)) {
		CVI_TRACE_HDMI(CVI_DBG_ERR, "access_ok failed\n");
	}
#endif

	ret = copy_from_user(kdata, (void __user *)arg, in_size);
	if (ret != 0) {
		CVI_TRACE_HDMI(CVI_DBG_ERR, "copy_from_user failed: ret=%d\n", ret);
		goto err;
	}

	/* zero out any difference between the kernel/user structure size */
	if (ksize > in_size)
		memset(kdata + in_size, 0, ksize - in_size);

	switch (cmd) {
		case CVI_HDMI_INIT:
		{
			ret = hdmitx_init();
			break;
		}
		case CVI_HDMI_DEINIT:
		{
			ret = hdmitx_deinit();
			break;
		}
		case CVI_HDMI_STOP:
		{
			ret = hdmitx_stop();
			break;
		}
		case CVI_HDMI_OPEN:
		{
			break;
		}
		case CVI_HDMI_CLOSE:
		{
			break;
		}
		case CVI_HDMI_GET_SINK_CAPABILITY:
		{
			hdmi_sink_capability* sink_cap = (hdmi_sink_capability*)kdata;
			CHECK_IOCTL_CMD(cmd, hdmi_sink_capability);
			ret = sink_capability(sink_cap);
			break;
		}
		case CVI_HDMI_SET_ATTR:
		{
			hdmi_attr* attr = (hdmi_attr*)kdata;
			CHECK_IOCTL_CMD(cmd, hdmi_attr);
			ret = hdmitx_set_attr(attr);
			break;
		}
		case CVI_HDMI_GET_ATTR:
		{
			hdmi_attr* attr = (hdmi_attr*)kdata;
			CHECK_IOCTL_CMD(cmd, hdmi_attr);
			ret = hdmitx_get_attr(attr);
			break;
		}
		case CVI_HDMI_START:
		{
			ret = hdmitx_start();
			break;
		}
		case CVI_HDMI_FORCE_GET_EDID:
		{
			hdmi_edid* edid_raw = (hdmi_edid*)kdata;
			CHECK_IOCTL_CMD(cmd, hdmi_edid);
			ret = hdmitx_force_get_edid(edid_raw, NULL);
			break;
		}
		case CVI_HDMI_GET_EVENT_ID:
		{
			u32* event_id = (u32*)kdata;
			ret = get_current_event_id(event_id);
			break;
		}
		case CVI_HDMI_UNREGISTER_CALLBACK:
		{
			break;
		}
		case CVI_HDMI_SET_INFOFRAME:
		{
			hdmi_infoframe* info_frame = (hdmi_infoframe*)kdata;
			CHECK_IOCTL_CMD(cmd, hdmi_infoframe);
			ret = hdmitx_set_infoframe(info_frame);
			break;
		}
		case CVI_HDMI_GET_INFOFRAME:
		{
			hdmi_infoframe* info_frame = (hdmi_infoframe*)kdata;
			CHECK_IOCTL_CMD(cmd, hdmi_infoframe);
			ret = hdmitx_get_infoframe(info_frame);
			break;
		}
		case CVI_HDMI_SET_HW_SPEC:
		{
			break;
		}
		case CVI_HDMI_GET_HW_SPEC:
		{
			break;
		}
		case CVI_HDMI_SET_AVMUTE:
		{
			unsigned char* enable = (unsigned char*)kdata;
			ret = hdmitx_set_avmute(*enable);
			break;
		}
		case CVI_HDMI_SET_AUDIO_MUTE:
		{
			unsigned char* enable = (unsigned char*)kdata;
			ret = hdmitx_set_audio_mute(*enable);
			break;
		}

		default:
			CVI_TRACE_HDMI(CVI_DBG_ERR, "unknown cmd(0x%x)\n", cmd);
			break;
	}

	if (copy_to_user((void __user *)arg, kdata, out_size) != 0)
		ret = -EFAULT;

err:
	if (kdata != stack_kdata)
		kfree(kdata);

	return ret;
}
