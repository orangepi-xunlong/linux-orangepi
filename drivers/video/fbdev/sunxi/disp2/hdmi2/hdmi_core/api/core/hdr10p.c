/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "packets.h"
#include "audio.h"
#include "hdr10p.h"
#include <video/sunxi_display2.h>
#include <video/sunxi_metadata.h>
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>

u32 mymin(u32 left, u32 right)
{
	if (left < right)
		return left;
	else
		return (right);
}

u32 myround(u32 in, u32 div)
{
	u32 res = 0;
	if (div == 0)
		return in;
	res = in % div;
	if (res >= (div / 2))
		return in / div + 1;
	else
		return in / div;
}

int hdr10p_Configure(hdmi_tx_dev_t *dev, void *config,
	videoParams_t *video, productParams_t *prod,
	struct disp_device_dynamic_config *scfg)
{
	LOG_TRACE();

	if (dev->snps_hdmi_ctrl.hdmi_on == 0) {
		pr_info("DVI mode selected: packets not configured\n");
		return -1;
	}

	if (config == NULL)  {
		u32 oui = 0;
		u8 struct_3d = 0;
		u8 data[4];
		u8 *vendor_payload = prod->mVendorPayload;
		u8 payload_length = prod->mVendorPayloadLength;
		if (video->mHdmiVideoFormat == 2) {
			struct_3d = video->m3dStructure;
			VIDEO_INF("3D packets configure\n");

			/* frame packing || tab || sbs */
			if ((struct_3d == 0) || (struct_3d == 6) || (struct_3d == 8)) {
				data[0] = video->mHdmiVideoFormat << 5; /* PB4 */
				data[1] = struct_3d << 4; /* PB5 */
				data[2] = video->m3dExtData << 4;
				data[3] = 0;
				/* HDMI Licensing, LLC */
				packets_VendorSpecificInfoFrame(dev, 0x000C03, data,
												sizeof(data), 1);
				fc_vsif_enable(dev, 1);
			} else {
				pr_err("Error:3D structure not supported %d\n",
					   struct_3d);
				return FALSE;
			}
		} else if ((video->mHdmiVideoFormat == 0x1) || (video->mHdmiVideoFormat == 0x0)) {
			if (prod != 0) {
				fc_spd_info_t spd_data;

				spd_data.vName = prod->mVendorName;
				spd_data.vLength = prod->mVendorNameLength;
				spd_data.pName = prod->mProductName;
				spd_data.pLength = prod->mProductNameLength;
				spd_data.code = prod->mSourceType;
				spd_data.autoSend = 1;

				oui = prod->mOUI;
				fc_spd_config(dev, &spd_data);
				packets_VendorSpecificInfoFrame(dev, oui, vendor_payload,
												payload_length, 1);
				fc_vsif_enable(dev, 1);
			} else {
				VIDEO_INF("No product info provided: not configured\n");
			}
		} else {
			pr_info("error: unknow video format\n");
			fc_vsif_enable(dev, 0);
		}

		fc_packets_metadata_config(dev);

		/* default phase 1 = true */
		dev_write_mask(dev, FC_GCP, FC_GCP_DEFAULT_PHASE_MASK,
					   ((video->mPixelPackingDefaultPhase == 1) ? 1 : 0));

		fc_gamut_config(dev);
		return 0;
	}

	if (prod != 0 && config != NULL) {
		u32 oui = 0x90848B;
		u8 vendor_payload[24];
		u8 payload_length = 24;
		int i = 0;
		int ret = 0;
		fc_spd_info_t spd_data;
		void *hdr_buff_addr;
		struct dma_buf *dmabuf;
		struct hdr_static_metadata *hdr_smetadata;
		struct sunxi_metadata *pMeta = (struct sunxi_metadata *) config;
		u8 *temp = kmalloc(scfg->metadata_size, GFP_KERNEL);
		fc_drm_pb_t *dynamic_pb = kmalloc(sizeof(fc_drm_pb_t), GFP_KERNEL);

		spd_data.vName = prod->mVendorName;
		spd_data.vLength = prod->mVendorNameLength;
		spd_data.pName = prod->mProductName;
		spd_data.pLength = prod->mProductNameLength;
		spd_data.code = prod->mSourceType;
		spd_data.autoSend = 1;

		memcpy(vendor_payload, (u8 *)&pMeta->hdr10_plus_smetada.divLut[
			   NUM_DIV - 1][MAX_LUT_SIZE - 1 - 24], 24);

		VIDEO_INF("hdr10p get maximu lumin=%d\n",
				  pMeta->hdr10_plus_smetada.
				  targeted_system_display_maximum_luminance);
		VIDEO_INF("hdr10p get maxrgb=%d\n",
				  pMeta->hdr10_plus_smetada.average_maxrgb);
		for (i = 0; i < 10; i++) {
			VIDEO_INF("%d dist val=%d\n", i,
					  pMeta->hdr10_plus_smetada.distribution_values[i]);
		}
		VIDEO_INF("hdr10p get knee_point_x=%d\n",
				  pMeta->hdr10_plus_smetada.knee_point_x);
		VIDEO_INF("hdr10p get knee_point_y=%d\n",
				  pMeta->hdr10_plus_smetada.knee_point_y);
		VIDEO_INF("hdr10p get num curve=%d\n",
				  pMeta->hdr10_plus_smetada.num_bezier_curve_anchors);
		for (i = 0; i < 9; i++) {
			VIDEO_INF("%d,bezier=%d\n", i,
					  pMeta->hdr10_plus_smetada.bezier_curve_anchors[i]);
		}

		for (i = 0; i < 24; i++) {
			VIDEO_INF("output pb%d=%x\n", 4 + i, vendor_payload[i]);
		}

		/*get the virtual addr of metadata*/
		dmabuf = dma_buf_get(scfg->metadata_fd);
		if (IS_ERR(dmabuf)) {
			pr_info("dma_buf_get failed\n");
			kfree(temp);
			kfree(dynamic_pb);
			return -1;
		}

		ret = dma_buf_begin_cpu_access(dmabuf, DMA_FROM_DEVICE);
		if (ret) {
			dma_buf_put(dmabuf);
			pr_info("dmabuf cpu aceess failed\n");
			kfree(temp);
			kfree(dynamic_pb);
			return ret;
		}
		hdr_buff_addr = dma_buf_kmap(dmabuf, 0);
		if (!hdr_buff_addr) {
			pr_info("dma_buf_kmap failed\n");
			dma_buf_end_cpu_access(dmabuf, DMA_FROM_DEVICE);
			dma_buf_put(dmabuf);
			kfree(temp);
			kfree(dynamic_pb);
			return -1;
		}

		/*obtain metadata*/
		memcpy((void *)temp, hdr_buff_addr, scfg->metadata_size);
		dma_buf_kunmap(dmabuf, 0, dmabuf);
		dma_buf_end_cpu_access(dmabuf, DMA_FROM_DEVICE);
		dma_buf_put(dmabuf);

		dynamic_pb->eotf = video->pb->eotf;
		dynamic_pb->metadata = video->pb->metadata;

		hdr_smetadata = (struct hdr_static_metadata *)temp;
		dynamic_pb->r_x =
			hdr_smetadata->disp_master.display_primaries_x[0];
		dynamic_pb->r_y =
			hdr_smetadata->disp_master.display_primaries_y[0];
		dynamic_pb->g_x =
			hdr_smetadata->disp_master.display_primaries_x[1];
		dynamic_pb->g_y =
			hdr_smetadata->disp_master.display_primaries_y[1];
		dynamic_pb->b_x =
			hdr_smetadata->disp_master.display_primaries_x[2];
		dynamic_pb->b_y =
			hdr_smetadata->disp_master.display_primaries_y[2];
		dynamic_pb->w_x =
			hdr_smetadata->disp_master.white_point_x;
		dynamic_pb->w_y =
			hdr_smetadata->disp_master.white_point_y;
		dynamic_pb->luma_max =
			hdr_smetadata->disp_master.max_display_mastering_luminance
			/ 10000;
		dynamic_pb->luma_min =
			hdr_smetadata->disp_master.min_display_mastering_luminance;
		dynamic_pb->mcll =
			hdr_smetadata->maximum_content_light_level;
		dynamic_pb->mfll =
			hdr_smetadata->maximum_frame_average_light_level;

		/*send dynamic metadata*/
		fc_spd_config(dev, &spd_data);
		packets_VendorSpecificInfoFrame(dev, oui, vendor_payload,
										(payload_length), 1);
		fc_vsif_config(dev, 1);

		/*send static metadata*/
		fc_drm_up(dev, dynamic_pb);

		kfree(temp);
		kfree(dynamic_pb);
	} else {
		VIDEO_INF("No product info provided: not configured\n");
		return -1;
	}

	/*fc_packets_metadata_config(dev);*/
	/* default phase 1 = true */
	/*dev_write_mask(dev, FC_GCP, FC_GCP_DEFAULT_PHASE_MASK,
			((video->mPixelPackingDefaultPhase == 1) ? 1 : 0));
	fc_gamut_config(dev);*/

	return 0;
}

