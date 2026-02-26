/*************************************************************************/ /*!
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

@Copyright      Portions Copyright (c) Synopsys Ltd. All Rights Reserved
@License        Synopsys Permissive License

The Synopsys Software Driver and documentation (hereinafter "Software")
is an unsupported proprietary work of Synopsys, Inc. unless otherwise
expressly agreed to in writing between  Synopsys and you.

The Software IS NOT an item of Licensed Software or Licensed Product under
any End User Software License Agreement or Agreement for Licensed Product
with Synopsys or any supplement thereto.  Permission is hereby granted,
free of charge, to any person obtaining a copy of this software annotated
with this license and the Software, to deal in the Software without
restriction, including without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so, subject
to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS" BASIS
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE HEREBY DISCLAIMED. IN NO EVENT SHALL SYNOPSYS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT     LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.
*/ /**************************************************************************/

#include "hdmi_regs.h"
#include "hdmi_video.h"
#include "hdmi_phy.h"
#include "plato_drv.h"
#include <linux/delay.h>

enum hdmi_datamap {
	RGB444_8B = 0x01,
	RGB444_10B = 0x03,
	RGB444_12B = 0x05,
	RGB444_16B = 0x07,
	YCbCr444_8B = 0x09,
	YCbCr444_10B = 0x0B,
	YCbCr444_12B = 0x0D,
	YCbCr444_16B = 0x0F,
	YCbCr422_8B = 0x16,
	YCbCr422_10B = 0x14,
	YCbCr422_12B = 0x12,
};

/*
 * Sets up Video Packetizer registers
 */
static bool video_configure_vp(struct hdmi_device *hdmi)
{
	u8 reg = 0;
	u8 output_select = 0;
	u8 remap_size = 0;
	u8 color_depth = 0;

	HDMI_CHECKPOINT;

	if (hdmi->hdmi_data.enc_out_format == ENCODING_RGB ||
		hdmi->hdmi_data.enc_out_format == ENCODING_YCC444) {
		if (hdmi->hdmi_data.enc_color_depth == 0) {
			output_select = 3;
		} else if (hdmi->hdmi_data.enc_color_depth == 8) {
			color_depth = 4;
			output_select = 3;
		} else if (hdmi->hdmi_data.enc_color_depth == 10) {
			color_depth = 5;
		} else if (hdmi->hdmi_data.enc_color_depth == 12) {
			color_depth = 6;
		} else if (hdmi->hdmi_data.enc_color_depth == 16) {
			color_depth = 7;
		} else {
			hdmi_error(hdmi, "- %s: Found unsupported color depth (%d)", __func__, hdmi->hdmi_data.enc_color_depth);
			return false;
		}
	} else if (hdmi->hdmi_data.enc_out_format == ENCODING_YCC422_8BITS) {
		if ((hdmi->hdmi_data.enc_color_depth == 8)
		    || (hdmi->hdmi_data.enc_color_depth == 0)) {
			remap_size = 0;
		} else if (hdmi->hdmi_data.enc_color_depth == 10) {
			remap_size = 1;
		} else if (hdmi->hdmi_data.enc_color_depth == 12) {
			remap_size = 2;
		} else {
			hdmi_error(hdmi, "- %s: Invalid color remap size (%d)", __func__, hdmi->hdmi_data.enc_color_depth);
			return false;
		}
		output_select = 1;
	} else {
		hdmi_error(hdmi, "- %s: Invalid output encoding type: %d",
					__func__, hdmi->hdmi_data.enc_out_format);
		return false;
	}

	hdmi_info(hdmi, " - %s: output_select: %d, color_depth: %d\n",
					__func__, output_select, color_depth);

	reg = SET_FIELD(HDMI_VP_PR_CD_COLOR_DEPTH_START,
		HDMI_VP_PR_CD_COLOR_DEPTH_MASK,
		color_depth) |
	SET_FIELD(HDMI_VP_PR_CD_DESIRED_PR_FACTOR_START,
		HDMI_VP_PR_CD_DESIRED_PR_FACTOR_MASK,
		hdmi->hdmi_data.pix_repet_factor);
	hdmi_write_reg32(hdmi, HDMI_VP_PR_CD_OFFSET, reg);

	reg = SET_FIELD(HDMI_VP_STUFF_IDEFAULT_PHASE_START,
		HDMI_VP_STUFF_IDEFAULT_PHASE_MASK,
		hdmi->hdmi_data.pp_default_phase);
	hdmi_write_reg32(hdmi, HDMI_VP_STUFF_OFFSET, reg);

	reg = SET_FIELD(HDMI_VP_REMAP_YCC422_SIZE_START,
		HDMI_VP_REMAP_YCC422_SIZE_MASK,
		remap_size);
	hdmi_write_reg32(hdmi, HDMI_VP_REMAP_OFFSET, reg);

	/* Grabbed from reference driver */
	reg = 0;
	if (output_select == 0) {
		/* pixel packing */
		reg = SET_FIELD(HDMI_VP_CONF_PP_EN_START,
			HDMI_VP_CONF_PP_EN_MASK,
			1);
	} else if (output_select == 1) {
		/* YCC422 */
		reg = SET_FIELD(HDMI_VP_CONF_YCC422_EN_START,
			HDMI_VP_CONF_YCC422_EN_MASK,
			1);
	} else if (output_select == 2 || output_select == 3) {
		/* Enable bypass */
		reg = SET_FIELD(HDMI_VP_CONF_BYPASS_EN_START,
			HDMI_VP_CONF_BYPASS_EN_MASK,
			1) |
		SET_FIELD(HDMI_VP_CONF_BYPASS_SELECT_START,
			HDMI_VP_CONF_BYPASS_SELECT_MASK,
			1);
	}

	reg |= SET_FIELD(HDMI_VP_CONF_OUTPUT_SELECTOR_START,
		HDMI_VP_CONF_OUTPUT_SELECTOR_MASK,
		output_select);

#if defined(EMULATOR)
	reg |= SET_FIELD(HDMI_VP_CONF_BYPASS_EN_START,
		HDMI_VP_CONF_BYPASS_EN_MASK,
		1) |
	SET_FIELD(HDMI_VP_CONF_BYPASS_SELECT_START,
		HDMI_VP_CONF_BYPASS_SELECT_MASK,
		1);
#endif

	/* pixel packing */
	reg |= SET_FIELD(HDMI_VP_CONF_PP_EN_START,
		HDMI_VP_CONF_PP_EN_MASK,
		1);
	hdmi_write_reg32(hdmi, HDMI_VP_CONF_OFFSET, reg);

	/* YCC422 and pixel packing stuffing */
	reg = SET_FIELD(HDMI_VP_STUFF_IDEFAULT_PHASE_START,
		HDMI_VP_STUFF_IDEFAULT_PHASE_MASK,
		hdmi->hdmi_data.pp_default_phase) |
	SET_FIELD(HDMI_VP_STUFF_YCC422_STUFFING_START,
		HDMI_VP_STUFF_YCC422_STUFFING_MASK,
		1) |
	SET_FIELD(HDMI_VP_STUFF_PP_STUFFING_START,
		HDMI_VP_STUFF_PP_STUFFING_MASK,
		1);
	hdmi_write_reg32(hdmi, HDMI_VP_STUFF_OFFSET, reg);

	return true;
}

/*
 *Sets up Video Sampler registers
 */
static bool video_configure_vs(struct hdmi_device *hdmi)
{
	u8 map_code = 0;
	u8 reg = 0;

	HDMI_CHECKPOINT;

	if (hdmi->hdmi_data.enc_in_format == ENCODING_RGB ||
		hdmi->hdmi_data.enc_in_format == ENCODING_YCC444) {
		if (hdmi->hdmi_data.enc_color_depth == 8 || hdmi->hdmi_data.enc_color_depth == 0) {
			map_code = 1;
		} else if (hdmi->hdmi_data.enc_color_depth == 10) {
			map_code = 3;
		} else if (hdmi->hdmi_data.enc_color_depth == 12) {
			map_code = 5;
		} else if (hdmi->hdmi_data.enc_color_depth == 16) {
			map_code = 7;
		} else {
			hdmi_error(hdmi, "- %s: Invalid color depth\n", __func__);
			return false;
		}
		map_code += (hdmi->hdmi_data.enc_in_format  == ENCODING_YCC444) ? 8 : 0;
	} else if (hdmi->hdmi_data.enc_in_format == ENCODING_YCC422_8BITS) {
		/* YCC422 mapping is discontinued - only map 1 is supported */
		if (hdmi->hdmi_data.enc_color_depth == 12) {
			map_code = 18;
		} else if (hdmi->hdmi_data.enc_color_depth == 10) {
			map_code = 20;
		} else if ((hdmi->hdmi_data.enc_color_depth == 8)
			   || (hdmi->hdmi_data.enc_color_depth == 0)) {
			map_code = 22;
		} else {
			hdmi_error(hdmi, "- %s: Invalid color remap size: %d",
					__func__, hdmi->hdmi_data.enc_color_depth);
			return false;
		}
	} else {
		hdmi_error(hdmi, "- %s: Invalid input encoding type: %d",
					__func__, hdmi->hdmi_data.enc_in_format);
		return false;
	}

	reg = SET_FIELD(HDMI_TX_INVID0_INTERNAL_DE_GEN_START, HDMI_TX_INVID0_INTERNAL_DE_GEN_MASK, 0);
	reg |= SET_FIELD(HDMI_TX_INVID0_VIDEO_MAPPING_START, HDMI_TX_INVID0_VIDEO_MAPPING_MASK, map_code);
	hdmi_write_reg32(hdmi, HDMI_TX_INVID0_OFFSET, reg);

#if !defined(EMULATOR)
	hdmi_write_reg32(hdmi, HDMI_TX_GYDATA0_OFFSET, 0);
	hdmi_write_reg32(hdmi, HDMI_TX_GYDATA1_OFFSET, 0);
	hdmi_write_reg32(hdmi, HDMI_TX_RCRDATA0_OFFSET, 0);
	hdmi_write_reg32(hdmi, HDMI_TX_RCRDATA1_OFFSET, 0);
	hdmi_write_reg32(hdmi, HDMI_TX_BCBDATA0_OFFSET, 0);
	hdmi_write_reg32(hdmi, HDMI_TX_BCBDATA1_OFFSET, 0);
	/* Sets stuffing enable for BDBDATA, RCRDATA, and GYDATA */
	hdmi_write_reg32(hdmi, HDMI_TX_INSTUFFING_OFFSET, 0x7);
#endif

	return true;
}

/*
 * Frame composer
 */
static bool video_configure_fc(struct hdmi_device *hdmi, struct drm_display_mode *mode)
{
	u32 reg = 0;
	u32 hblank, vblank, hsync_delay, vsync_delay, hsync_length, vsync_length;

	/*
	 *Input video configuration
	 */
	reg = SET_FIELD(HDMI_FC_INVIDCONF_VSYNC_IN_POLARITY_START,
		HDMI_FC_INVIDCONF_VSYNC_IN_POLARITY_MASK,
		hdmi->hdmi_data.video_mode.vsync_polarity) |
	SET_FIELD(HDMI_FC_INVIDCONF_HSYNC_IN_POLARITY_START,
		HDMI_FC_INVIDCONF_HSYNC_IN_POLARITY_MASK,
		hdmi->hdmi_data.video_mode.hsync_polarity) |
	SET_FIELD(HDMI_FC_INVIDCONF_DE_IN_POLARITY_START,
		HDMI_FC_INVIDCONF_DE_IN_POLARITY_MASK,
		hdmi->hdmi_data.video_mode.data_enable_polarity) |
	SET_FIELD(HDMI_FC_INVIDCONF_R_V_BLANK_IN_OSC_START,
		HDMI_FC_INVIDCONF_R_V_BLANK_IN_OSC_MASK,
		hdmi->hdmi_data.video_mode.interlaced) |
	SET_FIELD(HDMI_FC_INVIDCONF_IN_I_P_START,
		HDMI_FC_INVIDCONF_IN_I_P_MASK,
		hdmi->hdmi_data.video_mode.interlaced) |
	SET_FIELD(HDMI_FC_INVIDCONF_DVI_MODEZ_START,
		HDMI_FC_INVIDCONF_DVI_MODEZ_MASK,
		hdmi->hdmi_data.video_mode.dvi);    // Nick's original was !dvi
	hdmi_write_reg32(hdmi, HDMI_FC_INVIDCONF_OFFSET, reg);

	/*
	 * Input HActive Pixels
	 */
	reg = SET_FIELD(HDMI_FC_INHACTIV0_H_IN_ACTIV_START,
		HDMI_FC_INHACTIV0_H_IN_ACTIV_MASK,
		mode->hdisplay & 0xFF);
	hdmi_write_reg32(hdmi, HDMI_FC_INHACTIV0_OFFSET, reg);

	reg = SET_FIELD(HDMI_FC_INHACTIV1_H_IN_ACTIV_UPPER_START,
		HDMI_FC_INHACTIV1_H_IN_ACTIV_UPPER_MASK,
		mode->hdisplay >> 8);
	hdmi_write_reg32(hdmi, HDMI_FC_INHACTIV1_OFFSET, reg);

	/*
	 * Input VActive Pixels
	 */
	reg = SET_FIELD(HDMI_FC_INVACTIV0_V_IN_ACTIV_START,
		HDMI_FC_INVACTIV0_V_IN_ACTIV_MASK,
		mode->vdisplay & 0xFF);
	hdmi_write_reg32(hdmi, HDMI_FC_INVACTIV0_OFFSET, reg);

	reg = SET_FIELD(HDMI_FC_INVACTIV1_V_IN_ACTIV_UPPER_START,
		HDMI_FC_INVACTIV1_V_IN_ACTIV_UPPER_MASK,
		mode->vdisplay >> 8);
	hdmi_write_reg32(hdmi, HDMI_FC_INVACTIV1_OFFSET, reg);

	/*
	 * Input HBlank Pixels
	 */
	hblank = mode->htotal - mode->hdisplay;
	reg = SET_FIELD(HDMI_FC_INHBLANK0_H_IN_BLANK_START,
		HDMI_FC_INHBLANK0_H_IN_BLANK_MASK,
		hblank & 0xFF);
	hdmi_write_reg32(hdmi, HDMI_FC_INHBLANK0_OFFSET, reg);

	reg = SET_FIELD(HDMI_FC_INHBLANK1_H_IN_BLANK_UPPER_START,
		HDMI_FC_INHBLANK1_H_IN_BLANK_UPPER_MASK,
		hblank >> 8);
	hdmi_write_reg32(hdmi, HDMI_FC_INHBLANK1_OFFSET, reg);

	/*
	 * Input VBlank Pixels
	 */
	vblank = mode->vtotal - mode->vdisplay;
	reg = SET_FIELD(HDMI_FC_INVBLANK_V_IN_BLANK_START,
		HDMI_FC_INVBLANK_V_IN_BLANK_MASK,
		vblank & 0xFF);
	hdmi_write_reg32(hdmi, HDMI_FC_INVBLANK_OFFSET, reg);

	/*
	 * Input HSync Front Porch (pixel clock cycles from "de" non active edge of the last "de" valid period)
	 */
	hsync_delay = mode->hsync_start - mode->hdisplay;
	reg = SET_FIELD(HDMI_FC_HSYNCINDELAY0_H_IN_DELAY_START,
		HDMI_FC_HSYNCINDELAY0_H_IN_DELAY_MASK,
		hsync_delay & 0xFF);
	hdmi_write_reg32(hdmi, HDMI_FC_HSYNCINDELAY0_OFFSET, reg);

	reg = SET_FIELD(HDMI_FC_HSYNCINDELAY1_H_IN_DELAY_UPPER_START,
		HDMI_FC_HSYNCINDELAY1_H_IN_DELAY_UPPER_MASK,
		hsync_delay >> 8);
	hdmi_write_reg32(hdmi, HDMI_FC_HSYNCINDELAY1_OFFSET, reg);

	/*
	 * Input VSync Front porch
	 */
	vsync_delay = mode->vsync_start - mode->vdisplay;
	reg = SET_FIELD(HDMI_FC_VSYNCINDELAY_V_IN_DELAY_START,
		HDMI_FC_VSYNCINDELAY_V_IN_DELAY_MASK,
		vsync_delay & 0xFF);
	hdmi_write_reg32(hdmi, HDMI_FC_VSYNCINDELAY_OFFSET, reg);

	/*
	 * Input HSync pulse width
	 */
	hsync_length = mode->hsync_end - mode->hsync_start;
	reg = SET_FIELD(HDMI_FC_HSYNCINWIDTH0_H_IN_WIDTH_START,
		HDMI_FC_HSYNCINWIDTH0_H_IN_WIDTH_MASK,
		hsync_length & 0xFF);
	hdmi_write_reg32(hdmi, HDMI_FC_HSYNCINWIDTH0_OFFSET, reg);

	reg = SET_FIELD(HDMI_FC_HSYNCINWIDTH1_H_IN_WIDTH_UPPER_START,
		HDMI_FC_HSYNCINWIDTH1_H_IN_WIDTH_UPPER_MASK,
		hsync_length >> 8);
	hdmi_write_reg32(hdmi, HDMI_FC_HSYNCINWIDTH1_OFFSET, reg);

	/*
	 * Input Vsync pulse width
	 */
	vsync_length = mode->vsync_end - mode->vsync_start;
	reg = SET_FIELD(HDMI_FC_VSYNCINWIDTH_V_IN_WIDTH_START,
		HDMI_FC_VSYNCINWIDTH_V_IN_WIDTH_MASK,
		vsync_length & 0xFF);
	hdmi_write_reg32(hdmi, HDMI_FC_VSYNCINWIDTH_OFFSET, reg);

	return true;

}

/*
 * Steps for Configuring video mode:
 * 1. Configure pixel clock, VP, VS, and FC
 * 2. Power down Tx PHY
 * 3. Set desired video mode (based on EDID sink information)
 * 4. Power-on, PLL configuration, and reset Tx PHY
 * 5. Wait for PHY lock to assert
 */
int video_configure_mode(struct hdmi_device *hdmi, struct drm_display_mode *mode)
{
	int err = 0;

	HDMI_CHECKPOINT;

	hdmi_info(hdmi, " - %s: Configuring video mode for VIC %d\n", __func__, hdmi->hdmi_data.vic);

	hdmi->hdmi_data.video_mode.vsync_polarity = mode->flags & DRM_MODE_FLAG_PVSYNC;
	hdmi->hdmi_data.video_mode.hsync_polarity = mode->flags & DRM_MODE_FLAG_PHSYNC;
	hdmi->hdmi_data.video_mode.interlaced = mode->flags & DRM_MODE_FLAG_INTERLACE;

	/*
	 * Step D.2: Set desired video mode based on current VideoParams structure
	 */
	if (!video_configure_vp(hdmi)) {
		err = HDMI_INIT_FAILED_VIDEO;
		goto EXIT;
	}

	if (!video_configure_vs(hdmi)) {
		err = HDMI_INIT_FAILED_VIDEO;
		goto EXIT;
	}

	if (!video_configure_fc(hdmi, mode)) {
		err = HDMI_INIT_FAILED_VIDEO;
		goto EXIT;
	}

	/* Place phy in reset */
	phy_power_down(hdmi);

	/* Audio N and CTS value */
	hdmi_write_reg32(hdmi, HDMI_AUD_N1_OFFSET, 0xE5);
	hdmi_write_reg32(hdmi, HDMI_AUD_N2_OFFSET, 0x0F);
	hdmi_write_reg32(hdmi, HDMI_AUD_N3_OFFSET, 0x00);
	hdmi_write_reg32(hdmi, HDMI_AUD_CTS1_OFFSET, 0x19);
	hdmi_write_reg32(hdmi, HDMI_AUD_CTS2_OFFSET, 0xD5);
	hdmi_write_reg32(hdmi, HDMI_AUD_CTS3_OFFSET, 0x02);

	/* Setup PHY for intended video mode */
	phy_configure_mode(hdmi, mode);

	/* Wait for PHY lock */
	err = phy_wait_lock(hdmi);
	if (err != HDMI_INIT_SUCCESS)
		goto EXIT;

	/* From pdump */
	hdmi_write_reg32(hdmi, HDMI_FC_CTRLDUR_OFFSET, 0x0C);
	hdmi_write_reg32(hdmi, HDMI_FC_EXCTRLDUR_OFFSET, 0x20);
	hdmi_write_reg32(hdmi, HDMI_FC_EXCTRLSPAC_OFFSET, 0x01);
	hdmi_write_reg32(hdmi, HDMI_FC_CH0PREAM_OFFSET, 0x0B);
	hdmi_write_reg32(hdmi, HDMI_FC_CH1PREAM_OFFSET, 0x16);
	hdmi_write_reg32(hdmi, HDMI_FC_CH2PREAM_OFFSET, 0x21);
	hdmi_write_reg32(hdmi, HDMI_MC_CLKDIS_OFFSET,
		SET_FIELD(HDMI_MC_CLKDIS_HDCPCLK_DIS_START, HDMI_MC_CLKDIS_HDCPCLK_DIS_MASK, 1));

EXIT:
	return err;
}

/*
 * See HDMI ref driver packets.c
 */
int video_configure_infoframes(struct hdmi_device *hdmi, struct drm_display_mode *mode)
{
	u32 reg;

	HDMI_CHECKPOINT;

#if defined(EMULATOR)
	return 0; // This breaks on emu
#endif

	/* Only relevant for HDMI */
	if (!hdmi->hdmi_data.video_mode.dvi) {   // Nick's original was without !
		hdmi_info(hdmi, "- %s: Sink is DVI, not configuring infoframes\n", __func__);
		return 0;
	}

	/* AVI CONF0 setup */
	reg = SET_FIELD(HDMI_FC_AVICONF0_RGBYCC_START, HDMI_FC_AVICONF0_RGBYCC_MASK, 0);
	reg |= SET_FIELD(HDMI_FC_AVICONF0_SCAN_INFO_START,
		HDMI_FC_AVICONF0_SCAN_INFO_MASK,
		0);
	hdmi_write_reg32(hdmi, HDMI_FC_AVICONF0_OFFSET, reg);

	/* AVI CONF1 setup */
	reg = SET_FIELD(HDMI_FC_AVICONF1_PIC_ASPECT_RATIO_START,
		HDMI_FC_AVICONF1_PIC_ASPECT_RATIO_START,
		(mode->picture_aspect_ratio == HDMI_PICTURE_ASPECT_16_9) ? 2 : 1);
	reg |= SET_FIELD(HDMI_FC_AVICONF1_COLORIMETRY_START,
		HDMI_FC_AVICONF1_COLORIMETRY_MASK,
		hdmi->hdmi_data.colorimetry);
	reg |= SET_FIELD(HDMI_FC_AVICONF1_ACTIVE_ASPECT_RATIO_START,
		HDMI_FC_AVICONF1_ACTIVE_ASPECT_RATIO_MASK,
		hdmi->hdmi_data.active_aspect_ratio);
	hdmi_write_reg32(hdmi, HDMI_FC_AVICONF1_OFFSET, reg);

	/* Go back and set active format valid bit */
	reg = SET_FIELD(HDMI_FC_AVICONF0_ACTIVE_FORMAT_PRESENT_START,
		HDMI_FC_AVICONF0_ACTIVE_FORMAT_PRESENT_MASK,
		1);
	hdmi_mod_reg32(hdmi, HDMI_FC_AVICONF0_OFFSET,
		reg, HDMI_FC_AVICONF0_ACTIVE_FORMAT_PRESENT_MASK);

	/* AVI CONF2 setup */
	reg = SET_FIELD(HDMI_FC_AVICONF2_IT_CONTENT_START,
		HDMI_FC_AVICONF2_IT_CONTENT_MASK,
		0);
	reg |= SET_FIELD(HDMI_FC_AVICONF2_QUANTIZATION_RANGE_START,
		HDMI_FC_AVICONF2_QUANTIZATION_RANGE_MASK,
		0);
	reg |= SET_FIELD(HDMI_FC_AVICONF2_NON_UNIFORM_PIC_SCALING_START,
		HDMI_FC_AVICONF2_NON_UNIFORM_PIC_SCALING_MASK,
		0);
	hdmi_write_reg32(hdmi, HDMI_FC_AVICONF2_OFFSET, reg);

	/* AVI VID setup */
	reg = SET_FIELD(HDMI_FC_AVIVID_START, HDMI_FC_AVIVID_MASK, hdmi->hdmi_data.vic);
	hdmi_write_reg32(hdmi, HDMI_FC_AVIVID_OFFSET, reg);

	/* Set horizontal bars to 0 */
	hdmi_write_reg32(hdmi, HDMI_FC_AVIETB0_OFFSET, 0);
	hdmi_write_reg32(hdmi, HDMI_FC_AVIETB1_OFFSET, 0);
	hdmi_write_reg32(hdmi, HDMI_FC_AVISBB0_OFFSET, 0);
	hdmi_write_reg32(hdmi, HDMI_FC_AVISBB1_OFFSET, 0);
	hdmi_mod_reg32(hdmi, HDMI_FC_AVICONF0_OFFSET,
		SET_FIELD(HDMI_FC_AVICONF0_HBAR_VALID_START, HDMI_FC_AVICONF0_HBAR_VALID_MASK, 1),
		HDMI_FC_AVICONF0_HBAR_VALID_MASK);

	/* Set vertical bars to 0 */
	hdmi_write_reg32(hdmi, HDMI_FC_AVIELB0_OFFSET, 0);
	hdmi_write_reg32(hdmi, HDMI_FC_AVIELB1_OFFSET, 0);
	hdmi_write_reg32(hdmi, HDMI_FC_AVISRB0_OFFSET, 0);
	hdmi_write_reg32(hdmi, HDMI_FC_AVISRB1_OFFSET, 0);
	hdmi_mod_reg32(hdmi, HDMI_FC_AVICONF0_OFFSET,
		SET_FIELD(HDMI_FC_AVICONF0_VBAR_VALID_START, HDMI_FC_AVICONF0_VBAR_VALID_MASK, 1),
		HDMI_FC_AVICONF0_VBAR_VALID_MASK);

	/* Set out pixel repetition factor */
	hdmi_mod_reg32(hdmi, HDMI_FC_PRCONF_OFFSET,
		SET_FIELD(HDMI_FC_PRCONF_OUTPUT_PR_FACTOR_START,
			HDMI_FC_PRCONF_OUTPUT_PR_FACTOR_MASK,
			((hdmi->hdmi_data.video_mode.pixel_repetition_input + 1) *
			(hdmi->hdmi_data.pix_repet_factor + 1) - 1)),
		HDMI_FC_PRCONF_OUTPUT_PR_FACTOR_MASK);

	return 0;
}

/*
 * Initialize routine will setup video mode for VGA DVI mode to
 * start EDID communication
 */
int video_init(struct hdmi_device *hdmi)
{
	int err = 0;

	HDMI_CHECKPOINT;

	/* Force video output on init */
	hdmi_write_reg32(hdmi, HDMI_FC_DBGTMDS_2_OFFSET, 0x00); /* R */
	hdmi_write_reg32(hdmi, HDMI_FC_DBGTMDS_1_OFFSET, 0xFF); /* G */
	hdmi_write_reg32(hdmi, HDMI_FC_DBGTMDS_0_OFFSET, 0x00); /* B */
	hdmi_write_reg32(hdmi, HDMI_FC_DBGFORCE_OFFSET,
		SET_FIELD(HDMI_FC_DBGFORCE_FORCE_VIDEO_START,
					HDMI_FC_DBGFORCE_FORCE_VIDEO_MASK, 0));

	return err;
}
