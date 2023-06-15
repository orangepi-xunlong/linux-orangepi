/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */


#include "core_edid.h"
#include "hdmi_core.h"

static unsigned char test_edid[1024];
static bool use_test_edid;

/*
 * @data: virtual addr of edid data
 * @size: edid bytes size
 */
void edid_set_test_data(const unsigned char *data, unsigned int size)
{
	if (size > 1024) {
		pr_err("[hdmi]invalid edid size input\n");
		return;
	}

	memset(test_edid, 0, 1024);
	memcpy(test_edid, data, size);
	use_test_edid = true;
}

void edid_test_mode_enable(bool en)
{
	use_test_edid = en;
}

unsigned char edid_get_test_mode(void)
{
	return (unsigned char)use_test_edid;
}

static void edid_init_sink_cap(sink_edid_t *sink)
{
	memset(sink, 0, sizeof(sink_edid_t));
}

void edid_read_cap(void)
{
	struct hdmi_tx_core *core = get_platform();
	struct edid  *edid;
	u8  *edid_ext;
	sink_edid_t *sink = core->mode.sink_cap;
	int edid_ok = 0;
	int edid_tries = 3;
	int edid_ext_cnt = 0;

	LOG_TRACE();
	if ((!core) || (!sink)) {
		pr_err("Could not get core structure\n");
		return;
	}

	core->mode.edid = kzalloc(sizeof(struct edid), GFP_KERNEL);
	edid = core->mode.edid;
	if (!edid)
		pr_err("kzalloc for edid failed\n");

	memset(sink, 0, sizeof(sink_edid_t));
	core->blacklist_sink = -1;

	mutex_lock(&core->hdmi_tx.i2c_lock);
	edid_init_sink_cap(sink);
	core->dev_func.edid_parser_cea_ext_reset(sink);

	do {
		if (use_test_edid) {
			memcpy(edid, test_edid, 128);
		} else {
			edid_ok = core->dev_func.edid_read(edid);
			if (edid_ok == READ_ERROR) /* error case */
				continue;
		}

		core->mode.edid = edid;

		if (core->dev_func.edid_parser((u8 *) edid, sink, 128) == false) {
			pr_err("Error:parse EDID failed, error edid block0\n");
			core->mode.edid_done = 0;
			mutex_unlock(&core->hdmi_tx.i2c_lock);
			return;
		}

		core->blacklist_sink = hdmi_mode_blacklist_check((u8 *)edid);
		if (core->blacklist_sink >= 0)
			pr_info("Warning: this sink is in blacklist: %d\n", core->blacklist_sink);

		break;
	} while (edid_tries--);

	if (edid_tries <= 0) {
		pr_info("Could not read EDID\n");
		core->mode.edid_done = 0;
		mutex_unlock(&core->hdmi_tx.i2c_lock);
		return;
	}

	if (edid->extensions == 0) {
		core->mode.edid_done = 1;
	} else {
		int edid_ext_total_cnt = 0;

		/*(1)<----read and parse the 1st extension block data*/
		u8 temp_ext[128];

		EDID_INF("read and parse EDID Extension 1\n");

		edid_tries = 3;
		do {
			if (use_test_edid) {
				memcpy(temp_ext, test_edid + 128, 128);
			} else {
				edid_ok = core->dev_func.edid_extension_read(1, temp_ext);
				if (edid_ok)
					continue;
			}

			core->mode.edid_done = 1;
			if (core->dev_func.edid_parser(temp_ext, sink, 128) == false) {
				pr_info("Could not parse EDID EXTENSIONS\n");
				core->mode.edid_done = 0;
			}
			break;
		} while (edid_tries--);
		/*end of read and parse the 1st extension block data--->*/

		/*(2)<---decide total edid extension block count*/
		if (sink->hf_eeodb_block_count) {
			edid_ext_total_cnt = sink->hf_eeodb_block_count;
			EDID_INF("has HF-EEODB, hf_eeodb_block_count:%d\n",
						sink->hf_eeodb_block_count);
		} else
			edid_ext_total_cnt = edid->extensions;
		EDID_INF("Sink edid has %d extension block count\n",
						edid_ext_total_cnt);
		/* end of decide total edid extension block count--->*/


		/*(3)<---allocate memory to store edid extension block data*/
		edid_ext = kzalloc(128 * edid_ext_total_cnt, GFP_KERNEL);
		if (!edid_ext) {
			mutex_unlock(&core->hdmi_tx.i2c_lock);
			pr_err("kzalloc for edid extension failed\n");
			return;
		}
		core->mode.edid_ext = edid_ext;
		/*end of allocate memory--->*/

		/*(4)<---copy the 1st extension block data to core->mode.edid_ext*/
		memcpy(edid_ext, temp_ext, 128);
		/*end of copy--->*/

		/*(5)<---read the other extension blocks(>=2) and parse hf-eeodb*/
		edid_ext_cnt = 2;
		while (edid_ext_cnt <= edid_ext_total_cnt) {
			EDID_INF("read EDID Extension %d\n", edid_ext_cnt);
			edid_tries = 3;
			do {
				/*(5-1)reading extension block data*/
				if (use_test_edid) {
					memcpy(edid_ext + (edid_ext_cnt - 1) * 128,
						test_edid + edid_ext_cnt * 128,
						128);
				} else {
					edid_ok = core->dev_func.edid_extension_read(edid_ext_cnt,
								edid_ext + (edid_ext_cnt - 1) * 128);
					if (edid_ok == READ_ERROR)
						continue;
				}

				/*(5-2)<---parse the HF-EEODB*/
				if (sink->hf_eeodb_block_count >= edid_ext_cnt) {
					u8 *eeodb, tag, length;

					eeodb = &edid_ext[(edid_ext_cnt - 1) * 0x80 + 4];
					tag = eeodb[0] >> 5 & 0x7;
					length = eeodb[0] & 0x1f;
					if (tag == 0x2) {
						int i, j;

						for (i = 1; i <= length; i++) {
							for (j = 0; j < sink->edid_mSvdIndex; j++) {
								if (sink->edid_mSvd[i].mCode == eeodb[i])
									break;
							}

							if (j == sink->edid_mSvdIndex) {
								sink->edid_mSvd[sink->edid_mSvdIndex].mCode
									= eeodb[i];
								sink->edid_mSvdIndex++;
								EDID_INF("HF-EEODB, block:%d has VIC:%d\n",
										edid_ext_cnt, eeodb[i]);
							}
						}
					} else {
						EDID_INF("block:%d has NOT valid video data block\n",
															edid_ext_cnt);
					}
				}

				break;
			} while (edid_tries--);
			edid_ext_cnt++;
		}
		/*end of read the other extension blocks-->*/
	}

	mutex_unlock(&core->hdmi_tx.i2c_lock);
}

void hdmi_edid_release(void)
{
	struct hdmi_tx_core *core = get_platform();

	if (!core)
		return;

	if (core->mode.edid) {
		kfree(core->mode.edid);
		core->mode.edid = NULL;
	}

	if (core->mode.edid_ext) {
		kfree(core->mode.edid_ext);
		core->mode.edid_ext = NULL;
	}
}

int edid_sink_supports_vic_code(u32 vic_code)
{
	int i = 0;
	dtd_t dtd;
	struct hdmi_tx_core *p_core = get_platform();
	sink_edid_t *edid_exten = p_core->mode.sink_cap;
	struct detailed_timing *dt_ext = edid_exten->detailed_timings;
	struct edid *edid_block0 = p_core->mode.edid;
	struct detailed_timing *dt;

	if (p_core == NULL)
		return false;

	if (edid_exten == NULL)
		return false;


	for (i = 0; (i < 128) && (edid_exten->edid_mSvd[i].mCode != 0); i++) {
		if (edid_exten->edid_mSvd[i].mCode == vic_code)
			return true;
	}

	for (i = 0; (i < 4) && (edid_exten->edid_mHdmivsdb.mHdmiVic[i] != 0); i++) {
		if (edid_exten->edid_mHdmivsdb.mHdmiVic[i] == videoParams_GetHdmiVicCode(vic_code))
				return true;

	}

	if ((vic_code == 0x80 + 19) || (vic_code == 0x80 + 4)
		|| (vic_code == 0x80 + 32)) {
		if (edid_exten->edid_mHdmivsdb.m3dPresent == 1) {
			VIDEO_INF("Support basic 3d video format\n");
			return true;
		} else {
			pr_warn("Do not support any basic 3d video format\n");
			return false;
		}
	}

	if (!p_core->dev_func.dtd_fill(&dtd, vic_code, 0)) {
		pr_err("Error: Get detailed timing failed for vic_code:%d\n", vic_code);
		return false;
	}

	if (!edid_block0)
		return false;
	dt = edid_block0->detailed_timings;
	/*Check EDID Block0 detailed timing block*/
	for (i = 0; i < 2; i++) {
		if ((dtd.mPixelClock * (dtd.mPixelRepetitionInput + 1) == dt[i].pixel_clock * 10)
			&& (dtd.mHActive == ((((dt[i].data.pixel_data.hactive_hblank_hi >> 4) & 0x0f) << 8)
			| dt[i].data.pixel_data.hactive_lo))
			&& ((dtd.mVActive / (dtd.mInterlaced + 1)) == ((((dt[i].data.pixel_data.vactive_vblank_hi >> 4) & 0x0f) << 8)
			| dt[i].data.pixel_data.vactive_lo))
			&& (dtd.mPixelClock == ((unsigned int)dt[i].pixel_clock * 10))) {
			pr_info("detailed timing block:%d of edid block0 support\n", i);
			return true;
		}
	}

	/*Check EDID Extension Block detailed timing block*/
	for (i = 0; i < 2; i++) {
		if ((dtd.mPixelClock * (dtd.mPixelRepetitionInput + 1) == dt_ext[i].pixel_clock * 10)
			&& (dtd.mHActive == ((((dt_ext[i].data.pixel_data.hactive_hblank_hi >> 4) & 0x0f) << 8)
			| dt_ext[i].data.pixel_data.hactive_lo))
			&& ((dtd.mVActive / (dtd.mInterlaced + 1)) == ((((dt_ext[i].data.pixel_data.vactive_vblank_hi >> 4) & 0x0f) << 8)
			| dt_ext[i].data.pixel_data.vactive_lo))
			&& (dtd.mPixelClock == ((unsigned int)dt[i].pixel_clock * 10))) {
			pr_info("detailed timing block:%d of edid block0 support\n", 2 + i);
			return true;
		}
	}

	return false;
}

bool get_cea_vic_support(u32 cea_vic)
{
	struct hdmi_tx_core *core = get_platform();
	struct hdmi_mode *mode = &core->mode;
	bool support = false;
	u32 i;

	if (cea_vic == 0) {
		pr_info("error: invalid cea_vic code:%d\n", cea_vic);
		goto exit;
	}

	if ((mode == NULL) || (mode->sink_cap == NULL)) {
		pr_info("error: Has NULL struct\n");
		goto exit;
	}
	for (i = 0; (i < 128) && (mode->sink_cap->edid_mSvd[i].mCode != 0); i++) {
		if (mode->sink_cap->edid_mSvd[i].mCode == cea_vic)
			support = true;
	}

exit:
	return support;
}

bool get_hdmi_vic_support(u32 hdmi_vic)
{
	struct hdmi_tx_core *core = get_platform();
	struct hdmi_mode *mode = &core->mode;
	bool support = false;
	u32 i;

	if (hdmi_vic == 0) {
		pr_info("error: invalid cea_vic code:%d\n", hdmi_vic);
		goto exit;
	}

	if ((mode == NULL) || (mode->sink_cap == NULL)) {
		pr_info("error: Has NULL struct\n");
		goto exit;
	}
	for (i = 0; (i < 4) && (mode->sink_cap->edid_mHdmivsdb.mHdmiVic[i] != 0); i++) {
		if (mode->sink_cap->edid_mHdmivsdb.mHdmiVic[i] == hdmi_vic)
			support = true;
	}

exit:
	return support;
}


/*
* mainly for correcting hardware config after booting
* reason: as we do not read edid during booting,
* some configs that depended on edid
* can not be sure, so we have to correct them after
* booting and reading edid
*/
void edid_correct_hardware_config(void)
{
	struct hdmi_tx_core *core = get_platform();
	u8 vsif_data[2];
	int cea_code = 0;
	int hdmi_code = 0;

	if (!core->mode.edid_done)
		return;

	/*correct vic setting*/
	core->dev_func.get_vsif(vsif_data);
	if ((vsif_data[0]) >> 5 == 0x1) {/*To check if there is sending hdmi_vic*/
		if (get_hdmi_vic_support(vsif_data[1]))
			return;
		cea_code = videoParams_GetCeaVicCode(vsif_data[1]);
		if (get_cea_vic_support(cea_code)) {
			vsif_data[0] = 0x0;
			vsif_data[1] = 0x0;
			core->dev_func.set_vsif(vsif_data);
			core->dev_func.set_video_code(cea_code);
		}
	} else if ((vsif_data[0] >> 5) == 0x0) {/*To check if there is sending cea_vic*/
		cea_code = core->dev_func.get_video_code();
		hdmi_code = videoParams_GetHdmiVicCode(cea_code);
		if (hdmi_code > 0) {
			vsif_data[0] = 0x1 << 5;
			vsif_data[1] = hdmi_code;
			if (get_hdmi_vic_support(cea_code)) {
				core->dev_func.set_video_code(0);
				core->dev_func.set_vsif(vsif_data);
			}
		}
	}
}

void videoParams_SetYcc420Support(dtd_t *paramsDtd, shortVideoDesc_t *paramsSvd)
{
	paramsDtd->mLimitedToYcc420 = paramsSvd->mLimitedToYcc420;
	paramsDtd->mYcc420 = paramsSvd->mYcc420;
}

static void edid_set_video_prefered(sink_edid_t *sink_cap, videoParams_t *pVideo)
{
	/* Configure using Native SVD or HDMI_VIC */
	int i = 0;
	int hdmi_vic = 0;
	bool cea_vic_support = false;
	bool hdmi_vic_support = false;
	unsigned int vic_index = 0;
	struct hdmi_tx_core *core = get_platform();

	LOG_TRACE();
	if ((sink_cap == NULL) || (pVideo == NULL)) {
		pr_err("Error:Have NULL params\n");
		return;
	}

	if (!core->mode.edid_done) {
		pVideo->mCea_code = pVideo->mDtd.mCode;
		return;
	}

	if ((pVideo->mDtd.mCode == 0x80 + 19) ||
		(pVideo->mDtd.mCode == 0x80 + 4) ||
		(pVideo->mDtd.mCode == 0x80 + 32)) {
		pVideo->mCea_code = pVideo->mDtd.mCode - 0x80;
		pVideo->mHdmi_code = 0;
		return;
	}

	if ((pVideo->mDtd.mCode == 0x201)
		|| (pVideo->mDtd.mCode == 0x202)
		|| (pVideo->mDtd.mCode == 0x203)) {
		pVideo->mCea_code = 0;
		pVideo->mHdmi_code = 0;
		return;
	}

	pVideo->mHdmi20 = 0;
	pVideo->mHdmi20 = sink_cap->edid_m20Sink;

	/*parse if this vic in sink support yuv420*/
	for (i = 0; (i < 128) && (sink_cap->edid_mSvd[i].mCode != 0); i++) {
		if (sink_cap->edid_mSvd[i].mCode == pVideo->mDtd.mCode) {
			vic_index = i;
			break;
		}
	}

	videoParams_SetYcc420Support(&pVideo->mDtd,
					&sink_cap->edid_mSvd[vic_index]);

	if (videoParams_GetHdmiVicCode(pVideo->mDtd.mCode) > 0)
		hdmi_vic_support = get_hdmi_vic_support(videoParams_GetHdmiVicCode(pVideo->mDtd.mCode));
	if (hdmi_vic_support)
		VIDEO_INF("Sink Support hdmi vic mode:%d\n",
					videoParams_GetHdmiVicCode(pVideo->mDtd.mCode));

	cea_vic_support = get_cea_vic_support(pVideo->mDtd.mCode);
	if (cea_vic_support)
		VIDEO_INF("Sink Support cea vic mode:%d\n",
						pVideo->mDtd.mCode);
	else
		VIDEO_INF("Sink do NOT Support cea vic mode:%d\n",
						pVideo->mDtd.mCode);

	if (hdmi_vic_support) {
		pVideo->mCea_code = 0;
		hdmi_vic = videoParams_GetHdmiVicCode(pVideo->mDtd.mCode);
		pVideo->mHdmi_code = hdmi_vic;
		core->mode.pProduct.mOUI = 0x000c03;
		core->mode.pProduct.mVendorPayload[0] = 0x20;
		core->mode.pProduct.mVendorPayload[1] = hdmi_vic;
		core->mode.pProduct.mVendorPayload[2] = 0;
		core->mode.pProduct.mVendorPayload[3] = 0;
		core->mode.pProduct.mVendorPayloadLength = 4;
	} else if (cea_vic_support) {
		pVideo->mCea_code = pVideo->mDtd.mCode;
		pVideo->mHdmi_code = 0;

		if (((pVideo->mCea_code == 96)
			|| (pVideo->mCea_code == 97)
			|| (pVideo->mCea_code == 101)
			|| (pVideo->mCea_code == 102)) && (pVideo->mHdmi20 || pVideo->mEncodingOut != YCC420)) {
			core->mode.pProduct.mOUI = 0xc45dd8;
			core->mode.pProduct.mVendorPayload[0] = 0x1;
			core->mode.pProduct.mVendorPayload[1] = 0;
			core->mode.pProduct.mVendorPayload[2] = 0;
			core->mode.pProduct.mVendorPayload[3] = 0;
			core->mode.pProduct.mVendorPayloadLength = 4;
		} else {
			if ((pVideo->mCea_code == 96)
				|| (pVideo->mCea_code == 97)
				|| (pVideo->mCea_code == 101)
				|| (pVideo->mCea_code == 102)) {
				pr_info("[HDMI20 WARN] Source will set 4k50/60 but sink edid do NOT support hdmi20\n");
				pr_info("[HDMI20 WARN] cea_code:%d hdmi20:%d\n", pVideo->mCea_code, pVideo->mHdmi20);
			}

			core->mode.pProduct.mOUI = 0x000c03;
			core->mode.pProduct.mVendorPayload[0] = 0x0;
			core->mode.pProduct.mVendorPayload[1] = 0;
			core->mode.pProduct.mVendorPayload[2] = 0;
			core->mode.pProduct.mVendorPayload[3] = 0;
			core->mode.pProduct.mVendorPayloadLength = 4;

		}
	} else {
		pVideo->mCea_code = pVideo->mDtd.mCode;
		pr_err("[HDMI2 error]: sink do not support this mode:%d\n",
						pVideo->mDtd.mCode);
	}

	pVideo->scdc_ability = sink_cap->edid_mHdmiForumvsdb.mSCDC_Present;
}



void edid_set_video_prefered_core(void)
{
	struct hdmi_tx_core *core = get_platform();

	if (core->mode.sink_cap == NULL)
		pr_info("%s: sink cap is NULL!\n", __func__);
	else
		edid_set_video_prefered(core->mode.sink_cap,
				&core->mode.pVideo);
}


