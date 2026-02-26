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

#include "drm_pdp_drv.h"

#include <linux/moduleparam.h>
#include <linux/version.h>
#include <linux/component.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/hdmi.h>

#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_of.h>
#include <drm/drm_edid.h>

#if defined(PDP_USE_ATOMIC)
#include <drm/drm_atomic_helper.h>
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0))
#include <drm/drm_probe_helper.h>
#endif

#include "hdmi.h"
#include "hdmi_regs.h"
#include "hdmi_phy.h"
#include "hdmi_video.h"
#include "plato_drv.h"

#include "kernel_compatibility.h"

#define DRIVER_NAME "hdmi"
#define DRIVER_DESC "Imagination Technologies HDMI Driver"
#define DRIVER_DATE "20160809"
#define HDMI_VERSION_PLATO 1

static bool force_connected = false;

module_param(force_connected, bool, 0444);

MODULE_PARM_DESC(force_connected,
		 "Force connector status: connected (default: N)");

#if defined(PRINT_HDMI_REGISTERS)
static u32 gs_hdmi_registers[] = {
	HDMI_ID_DESIGN_ID_OFFSET,
	HDMI_ID_REVISION_ID_OFFSET,
	HDMI_ID_PRODUCT_ID0_OFFSET,
	HDMI_ID_PRODUCT_ID1_OFFSET,
	HDMI_ID_CONFIG0_ID_OFFSET,
	HDMI_ID_CONFIG1_ID_OFFSET,
	HDMI_ID_CONFIG2_ID_OFFSET,
	HDMI_ID_CONFIG3_ID_OFFSET,
	HDMI_IH_FC_STAT0_OFFSET,
	HDMI_IH_FC_STAT1_OFFSET,
	HDMI_IH_FC_STAT2_OFFSET,
	HDMI_IH_AS_STAT0_OFFSET,
	HDMI_IH_PHY_STAT0_OFFSET,
	HDMI_IH_I2CM_STAT0_OFFSET,
	HDMI_IH_CEC_STAT0_OFFSET,
	HDMI_IH_VP_STAT0_OFFSET,
	HDMI_IH_I2CMPHY_STAT0_OFFSET,
	HDMI_IH_AHBDMAAUD_STAT0_OFFSET,
	HDMI_IH_DECODE_OFFSET,
	HDMI_IH_MUTE_FC_STAT0_OFFSET,
	HDMI_IH_MUTE_FC_STAT1_OFFSET,
	HDMI_IH_MUTE_FC_STAT2_OFFSET,
	HDMI_IH_MUTE_AS_STAT0_OFFSET,
	HDMI_IH_MUTE_PHY_STAT0_OFFSET,
	HDMI_IH_MUTE_I2CM_STAT0_OFFSET,
	HDMI_IH_MUTE_CEC_STAT0_OFFSET,
	HDMI_IH_MUTE_VP_STAT0_OFFSET,
	HDMI_IH_MUTE_I2CMPHY_STAT0_OFFSET,
	HDMI_IH_MUTE_AHBDMAAUD_STAT0_OFFSET,
	HDMI_IH_MUTE_OFFSET,
	HDMI_TX_INVID0_OFFSET,
	HDMI_TX_INSTUFFING_OFFSET,
	HDMI_TX_GYDATA0_OFFSET,
	HDMI_TX_GYDATA1_OFFSET,
	HDMI_TX_RCRDATA0_OFFSET,
	HDMI_TX_RCRDATA1_OFFSET,
	HDMI_TX_BCBDATA0_OFFSET,
	HDMI_TX_BCBDATA1_OFFSET,
	HDMI_VP_STATUS_OFFSET,
	HDMI_VP_PR_CD_OFFSET,
	HDMI_VP_STUFF_OFFSET,
	HDMI_VP_REMAP_OFFSET,
	HDMI_VP_CONF_OFFSET,
	HDMI_VP_MASK_OFFSET,
	HDMI_FC_INVIDCONF_OFFSET,
	HDMI_FC_INHACTIV0_OFFSET,
	HDMI_FC_INHACTIV1_OFFSET,
	HDMI_FC_INHBLANK0_OFFSET,
	HDMI_FC_INHBLANK1_OFFSET,
	HDMI_FC_INVACTIV0_OFFSET,
	HDMI_FC_INVACTIV1_OFFSET,
	HDMI_FC_INVBLANK_OFFSET,
	HDMI_FC_HSYNCINDELAY0_OFFSET,
	HDMI_FC_HSYNCINDELAY1_OFFSET,
	HDMI_FC_HSYNCINWIDTH0_OFFSET,
	HDMI_FC_HSYNCINWIDTH1_OFFSET,
	HDMI_FC_VSYNCINDELAY_OFFSET,
	HDMI_FC_VSYNCINWIDTH_OFFSET,
	HDMI_FC_INFREQ0_OFFSET,
	HDMI_FC_INFREQ1_OFFSET,
	HDMI_FC_INFREQ2_OFFSET,
	HDMI_FC_CTRLDUR_OFFSET,
	HDMI_FC_EXCTRLDUR_OFFSET,
	HDMI_FC_EXCTRLSPAC_OFFSET,
	HDMI_FC_CH0PREAM_OFFSET,
	HDMI_FC_CH1PREAM_OFFSET,
	HDMI_FC_CH2PREAM_OFFSET,
	HDMI_FC_AVICONF3_OFFSET,
	HDMI_FC_GCP_OFFSET,
	HDMI_FC_AVICONF0_OFFSET,
	HDMI_FC_AVICONF1_OFFSET,
	HDMI_FC_AVICONF2_OFFSET,
	HDMI_FC_AVIVID_OFFSET,
	HDMI_FC_AVIETB0_OFFSET,
	HDMI_FC_AVIETB1_OFFSET,
	HDMI_FC_AVISBB0_OFFSET,
	HDMI_FC_AVISBB1_OFFSET,
	HDMI_FC_AVIELB0_OFFSET,
	HDMI_FC_AVIELB1_OFFSET,
	HDMI_FC_AVISRB0_OFFSET,
	HDMI_FC_AVISRB1_OFFSET,
	HDMI_FC_PRCONF_OFFSET,
	HDMI_FC_SCRAMBLER_CTRL_OFFSET,
	HDMI_FC_MULTISTREAM_CTRL_OFFSET,
	HDMI_FC_PACKET_TX_EN_OFFSET,
	HDMI_FC_DBGFORCE_OFFSET,
	HDMI_FC_DBGTMDS_0_OFFSET,
	HDMI_FC_DBGTMDS_1_OFFSET,
	HDMI_FC_DBGTMDS_2_OFFSET,
	HDMI_PHY_CONF0_OFFSET,
	HDMI_PHY_TST0_OFFSET,
	HDMI_PHY_TST1_OFFSET,
	HDMI_PHY_TST2_OFFSET,
	HDMI_PHY_STAT0_OFFSET,
	HDMI_PHY_INT0_OFFSET,
	HDMI_PHY_MASK0_OFFSET,
	HDMI_PHY_POL0_OFFSET,
	HDMI_PHY_I2CM_SLAVE_OFFSET,
	HDMI_PHY_I2CM_ADDRESS_OFFSET,
	HDMI_PHY_I2CM_DATAO_1_OFFSET,
	HDMI_PHY_I2CM_DATAO_0_OFFSET,
	HDMI_PHY_I2CM_DATAI_1_OFFSET,
	HDMI_PHY_I2CM_DATAI_0_OFFSET,
	HDMI_PHY_I2CM_OPERATION_OFFSET,
	HDMI_AUDIO_SAMPLER_OFFSET,
	HDMI_AUD_N1_OFFSET,
	HDMI_AUD_N2_OFFSET,
	HDMI_AUD_N3_OFFSET,
	HDMI_AUD_CTS1_OFFSET,
	HDMI_AUD_CTS2_OFFSET,
	HDMI_AUD_CTS3_OFFSET,
	HDMI_AUD_INPUTCLKFS_OFFSET,
	HDMI_AUDIO_DMA_OFFSET,
	HDMI_MC_CLKDIS_OFFSET,
	HDMI_MC_SWRSTZREQ_OFFSET,
	HDMI_MC_OPCTRL_OFFSET,
	HDMI_MC_FLOWCTRL_OFFSET,
	HDMI_MC_PHYRSTZ_OFFSET,
	HDMI_MC_LOCKONCLOCK_OFFSET,
	HDMI_MC_HEACPHY_RST_OFFSET,
	HDMI_MC_LOCKONCLOCK_2_OFFSET,
	HDMI_MC_SWRSTZREQ_2_OFFSET,
	HDMI_COLOR_SPACE_CONVERTER_OFFSET,
	HDMI_HDCP_ENCRYPTION_OFFSET,
	HDMI_I2CM_SLAVE_OFFSET,
	HDMI_I2CM_ADDRESS_OFFSET,
	HDMI_I2CM_DATAO_OFFSET,
	HDMI_I2CM_DATAI_OFFSET,
	HDMI_I2CM_OPERATION_OFFSET,
	HDMI_I2CM_INT_OFFSET,
	HDMI_I2CM_CTLINT_OFFSET,
	HDMI_I2CM_DIV_OFFSET,
	HDMI_I2CM_SEGADDR_OFFSET,
	HDMI_I2CM_SOFTRSTZ_OFFSET,
	HDMI_I2CM_SEGPTR_OFFSET,
	HDMI_I2CM_SS_SCL_HCNT_1_OFFSET,
	HDMI_I2CM_SS_SCL_HCNT_0_OFFSET,
	HDMI_I2CM_SS_SCL_LCNT_1_OFFSET,
	HDMI_I2CM_SS_SCL_LCNT_0_OFFSET,
	HDMI_I2CM_FS_SCL_HCNT_1_OFFSET,
	HDMI_I2CM_FS_SCL_HCNT_0_OFFSET,
	HDMI_I2CM_FS_SCL_LCNT_1_OFFSET,
	HDMI_I2CM_FS_SCL_LCNT_0_OFFSET,
	HDMI_I2CM_SDA_HOLD_OFFSET,
	HDMI_I2CM_SCDC_READ_UPDATE_OFFSET,
};

void PrintVideoRegisters(struct hdmi_device *hdmi)
{
	u32 n;

	dev_info(hdmi->dev, "HDMI register dump:\n");
	for (n = 0; n < ARRAY_SIZE(gs_hdmi_registers); ++n) {
		u32 value = hdmi_read_reg32(hdmi, gs_hdmi_registers[n]);

		dev_info(hdmi->dev, "Reg 0x%04x = 0x%08x\n", gs_hdmi_registers[n], value);
	}
}
#endif // PRINT_HDMI_REGISTERS

/* 16 - 1920x1080@60Hz */
static struct drm_display_mode forced_mode = {
	DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 148500, 1920, 2008,
		2052, 2200, 0, 1080, 1084, 1089, 1125, 0,
		DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	.picture_aspect_ratio = HDMI_PICTURE_ASPECT_16_9,
};


inline void hdmi_write_reg32(struct hdmi_device *hdmi, int offset, u32 val)
{
	plato_write_reg32(hdmi->core_regs, offset * 4, val);
}

inline u32 hdmi_read_reg32(struct hdmi_device *hdmi, int offset)
{
	return plato_read_reg32(hdmi->core_regs, offset * 4);
}

inline void hdmi_mod_reg32(struct hdmi_device *hdmi, u32 offset, u32 data, u32 mask)
{
	u32 val = hdmi_read_reg32(hdmi, offset) & ~mask;

	val |= data & mask;
	hdmi_write_reg32(hdmi, offset, val);
}

static void hdmi_mode_setup(struct hdmi_device *hdmi, struct drm_display_mode *mode)
{
	int err;

	HDMI_CHECKPOINT;

	plato_enable_pixel_clock(hdmi->drm_dev->dev->parent, mode->clock);

	/* Step D: Configure video mode */
	err = video_configure_mode(hdmi, mode);
	if (err != 0) {
		hdmi_error(hdmi, "%s: Failed to configure video mode\n", __func__);
		return;
	}

	// No need to configure audio on Plato,
	// skip to step F: Configure InfoFrames
	err = video_configure_infoframes(hdmi, mode);
	if (err != 0) {
		hdmi_error(hdmi, "%s: Failed to initialise PHY\n", __func__);
		return;
	}

	hdmi_info(hdmi, "%s: Final HDMI timing configuration:\n\nVIC: %d\nPixel clock: %d\nMode: %dx%d@%d\n\n",
		__func__, hdmi->hdmi_data.vic, mode->clock,
		mode->hdisplay, mode->vdisplay, hdmi->vrefresh);

#if defined(PRINT_HDMI_REGISTERS)
	PrintVideoRegisters(hdmi);
#endif
}

static void hdmi_poweron(struct hdmi_device *hdmi)
{
	HDMI_CHECKPOINT;
	mdelay(100);
	hdmi_mode_setup(hdmi, hdmi->native_mode);
}

static void hdmi_poweroff(struct hdmi_device *hdmi)
{
	phy_power_down(hdmi);
}

static void
hdmi_helper_connector_destroy(struct drm_connector *connector)
{
	connector_to_hdmi(connector);

	HDMI_CHECKPOINT;

	/* Disable all interrupts */
	hdmi_write_reg32(hdmi, HDMI_IH_MUTE_PHY_STAT0_OFFSET, ~0);
	kfree(hdmi->edid);
	drm_connector_cleanup(connector);
}

static enum drm_connector_status
hdmi_helper_connector_detect(struct drm_connector *connector, bool force)
{
	connector_to_hdmi(connector);
	enum drm_connector_status status;
	u32 phy_status;

	HDMI_CHECKPOINT;

	phy_status = hdmi_read_reg32(hdmi, HDMI_PHY_STAT0_OFFSET);

	hdmi_info(hdmi, "%s: HDMI HPD status %d\n", __func__, phy_status);

	if (phy_status & HDMI_PHY_STAT0_HPD_MASK)
		status = connector_status_connected;
	else
		status = force_connected ?
			connector_status_connected :
			connector_status_disconnected;

	return status;
}

static struct drm_display_mode *
hdmi_connector_native_mode(struct drm_connector *connector)
{
	connector_to_hdmi(connector);
	const struct drm_connector_helper_funcs *helper = connector->helper_private;
	struct drm_device *dev = connector->dev;
	struct drm_display_mode *mode, *largest = NULL;
	int high_w = 0, high_h = 0, high_v = 0;
	const int vrefresh_default = 60;

	list_for_each_entry(mode, &hdmi->connector.probed_modes, head) {
		hdmi->vrefresh = drm_mode_vrefresh(mode);
		if (!hdmi->vrefresh)
		{
			hdmi->vrefresh = vrefresh_default;
			DRM_INFO_ONCE(
				"vertical refresh rate is zero, defaulting to %d\n",
				hdmi->vrefresh);
		}

		if (helper->mode_valid(connector, mode) != MODE_OK ||
			(mode->flags & DRM_MODE_FLAG_INTERLACE))
			continue;

		/* Use preferred mode if there is one.. */
		if (mode->type & DRM_MODE_TYPE_PREFERRED) {
			hdmi_info(hdmi, "Retrieving native mode from preferred\n");
			return drm_mode_duplicate(dev, mode);
		}

		/*
		 * Otherwise, take the resolution with the largest width, then
		 * height, then vertical refresh
		 */
		if (mode->hdisplay < high_w)
			continue;

		if (mode->hdisplay == high_w && mode->vdisplay < high_h)
			continue;

		if (mode->hdisplay == high_w && mode->vdisplay == high_h &&
			hdmi->vrefresh < high_v)
			continue;

		high_w = mode->hdisplay;
		high_h = mode->vdisplay;
		high_v = hdmi->vrefresh;
		largest = mode;
	}

	hdmi_info(hdmi, "native mode from largest: %dx%d@%d\n",
				high_w, high_h, high_v);

	return largest ? drm_mode_duplicate(dev, largest) : NULL;
}

static int hdmi_connector_get_modes(struct drm_connector *connector)
{
	connector_to_hdmi(connector);
	struct drm_device *dev = connector->dev;
	int count = 0;

	HDMI_CHECKPOINT;

	if (!hdmi->edid) {
		hdmi->edid = drm_get_edid(connector, &hdmi->i2c->adap);
		if (!hdmi->edid) {
			dev_err(hdmi->dev, "failed to get edid\n");
			return 0;
		}
	}

	drm_connector_update_edid_property(connector, hdmi->edid);

	/*
	 * Destroy the native mode, the attached monitor could have changed.
	 */
	if (hdmi->native_mode) {
		drm_mode_destroy(dev, hdmi->native_mode);
		hdmi->native_mode = NULL;
	}

	count = drm_add_edid_modes(connector, hdmi->edid);
	hdmi->native_mode = hdmi_connector_native_mode(connector);
	if (count == 0 && hdmi->native_mode) {
		/*
		 * Find the native mode if this is a digital panel, if we didn't
		 * find any modes through DDC previously add the native mode to
		 * the list of modes.
		 */
		struct drm_display_mode *mode;

		mode = drm_mode_duplicate(dev, hdmi->native_mode);
		drm_mode_probed_add(connector, mode);
		count = 1;
	}

	hdmi_info(hdmi, "Native mode: %dx%d@%d\n", hdmi->native_mode->hdisplay,
		hdmi->native_mode->vdisplay, hdmi->native_mode->vrefresh);

	hdmi_info(hdmi, "Edid: width[%d] x height[%d]\n",
		hdmi->edid->width_cm, hdmi->edid->height_cm);
	hdmi_info(hdmi, "\nVersion: %d\nRevision: %d\nExtensions: %d\n# of modes: %d\n",
		hdmi->edid->version, hdmi->edid->revision,
		hdmi->edid->extensions, count);

	return count;
}

#if !defined(PDP_USE_ATOMIC)
static struct drm_encoder *hdmi_connector_best_encoder(
				struct drm_connector *connector)
{
	connector_to_hdmi(connector);

	HDMI_CHECKPOINT;

	return &hdmi->encoder;
}
#endif

static void hdmi_encoder_mode_set(struct drm_encoder *encoder,
			struct drm_display_mode *mode,
			struct drm_display_mode *adjusted_mode)
{
	encoder_to_hdmi(encoder);

	HDMI_CHECKPOINT;

	hdmi_mode_setup(hdmi, mode);
}

static bool hdmi_encoder_mode_fixup(struct drm_encoder *encoder,
			const struct drm_display_mode *mode,
			struct drm_display_mode *adjusted_mode)
{
	HDMI_CHECKPOINT;

	return true;
}

static void hdmi_encoder_dpms(struct drm_encoder *encoder, int mode)
{
	encoder_to_hdmi(encoder);

	HDMI_CHECKPOINT;

	if (mode)
		hdmi_poweroff(hdmi);
	else
		hdmi_poweron(hdmi);
}

static void hdmi_encoder_prepare(struct drm_encoder *encoder)
{
	encoder_to_hdmi(encoder);

	HDMI_CHECKPOINT;

	hdmi_poweroff(hdmi);
}

static void hdmi_encoder_commit(struct drm_encoder *encoder)
{
	encoder_to_hdmi(encoder);

	HDMI_CHECKPOINT;

	/* If forced, the driver has always got a modeline. Either
	 * via EDID, or via VESA fallback. The encoder has already
	 * been updated (stolen) in ->mode_set(), never attempt to
	 * re-initialise the hdmi status.
	 */
	if (force_connected)
		return;

	hdmi_poweron(hdmi);
}

static int hdmi_helper_connector_set_property(struct drm_connector *connector,
	struct drm_property *property,
	uint64_t value)
{
#if defined(HDMI_DEBUG)
	connector_to_hdmi(connector);
	HDMI_CHECKPOINT;
	hdmi_info(hdmi, "property name: %s\n", property->name);
#endif
	return 0;
}

static void hdmi_helper_connector_force(struct drm_connector *connector)
{
	if (force_connected && connector) {
#if defined(HDMI_DEBUG)
		connector_to_hdmi(connector);
		HDMI_CHECKPOINT;
		hdmi_info(hdmi, "%s: skip EDID, force status connected\n",
			  __func__);
#endif
		connector->status = connector_status_connected;
	}
}

static enum drm_mode_status
hdmi_mode_valid(struct drm_connector *connector, struct drm_display_mode *mode)
{
	return MODE_OK;
}

static void
hdmi_encoder_destroy(struct drm_encoder *encoder)
{
	drm_encoder_cleanup(encoder);
}

static const struct drm_encoder_funcs hdmi_encoder_funcs = {
	.destroy = hdmi_encoder_destroy,
};

static const struct drm_encoder_helper_funcs hdmi_encoder_helper_funcs = {
	.dpms = hdmi_encoder_dpms,
	.prepare = hdmi_encoder_prepare,
	.commit = hdmi_encoder_commit,
	.mode_set = hdmi_encoder_mode_set,
	.mode_fixup = hdmi_encoder_mode_fixup,
};

static const struct drm_connector_funcs hdmi_connector_funcs = {
	.fill_modes = drm_helper_probe_single_connector_modes,
	.detect = hdmi_helper_connector_detect,
	.destroy = hdmi_helper_connector_destroy,
	.set_property = hdmi_helper_connector_set_property,
	.force = hdmi_helper_connector_force,
#if defined(PDP_USE_ATOMIC)
	.reset = drm_atomic_helper_connector_reset,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
#else
	.dpms = drm_helper_connector_dpms,
#endif
};

static const struct drm_connector_helper_funcs hdmi_connector_helper_funcs = {
	.get_modes = hdmi_connector_get_modes,
	.mode_valid = hdmi_mode_valid,
	/*
	 * For atomic, don't set atomic_best_encoder or best_encoder. This will
	 * cause the DRM core to fallback to drm_atomic_helper_best_encoder().
	 * This is fine as we only have a single connector and encoder.
	 */
#if !defined(PDP_USE_ATOMIC)
	.best_encoder = hdmi_connector_best_encoder,
#endif
};

static void hdmi_init_interrupts(struct hdmi_device *hdmi)
{
	hdmi_write_reg32(hdmi, HDMI_IH_MUTE_PHY_STAT0_OFFSET, 0x3e);

	/* Mute all other interrupts */
	hdmi_write_reg32(hdmi, HDMI_IH_MUTE_FC_STAT0_OFFSET, 0xff);
	hdmi_write_reg32(hdmi, HDMI_IH_MUTE_FC_STAT1_OFFSET, 0xff);
	hdmi_write_reg32(hdmi, HDMI_IH_MUTE_FC_STAT2_OFFSET, 0xff);
	hdmi_write_reg32(hdmi, HDMI_IH_MUTE_AS_STAT0_OFFSET, 0xff);
	hdmi_write_reg32(hdmi, HDMI_IH_MUTE_I2CM_STAT0_OFFSET, 0xff);
	hdmi_write_reg32(hdmi, HDMI_IH_MUTE_CEC_STAT0_OFFSET, 0xff);
	hdmi_write_reg32(hdmi, HDMI_IH_MUTE_VP_STAT0_OFFSET, 0xff);
	hdmi_write_reg32(hdmi, HDMI_IH_MUTE_I2CMPHY_STAT0_OFFSET, 0xff);
	hdmi_write_reg32(hdmi, HDMI_IH_MUTE_AHBDMAAUD_STAT0_OFFSET, 0xff);
}

static void hdmi_disable_hpd(struct hdmi_device *hdmi)
{
	/* Disable hot plug interrupts */
	HDMI_CHECKPOINT;

	/* Clear HPD interrupt by writing 1*/
	if (IS_BIT_SET(hdmi_read_reg32(hdmi, HDMI_IH_PHY_STAT0_OFFSET),
						HDMI_IH_PHY_STAT0_HPD_START)) {
		hdmi_write_reg32(hdmi, HDMI_IH_PHY_STAT0_OFFSET,
			SET_FIELD(HDMI_IH_PHY_STAT0_HPD_START,
					HDMI_IH_PHY_STAT0_HPD_MASK, 1));
	}

	hdmi_init_interrupts(hdmi); // actually disables all (ready to enable just what we want).

	/* Power off */
	hdmi_write_reg32(hdmi, HDMI_PHY_CONF0_OFFSET,
		SET_FIELD(HDMI_PHY_CONF0_TXPWRON_START, HDMI_PHY_CONF0_TXPWRON_MASK, 0) |
		SET_FIELD(HDMI_PHY_CONF0_PDDQ_START, HDMI_PHY_CONF0_PDDQ_MASK, 1) |
		SET_FIELD(HDMI_PHY_CONF0_SVSRET_START, HDMI_PHY_CONF0_SVSRET_MASK, 1));
	/* Now flip the master switch to mute */
	hdmi_write_reg32(hdmi, HDMI_IH_MUTE_OFFSET, HDMI_IH_MUTE_ALL_MASK);

}

#if defined(HDMI_USE_HPD_INTERRUPTS)

static void hdmi_enable_hpd(struct hdmi_device *hdmi)
{
	/* Enable hot plug interrupts */
	HDMI_CHECKPOINT;

	/* Clear HPD interrupt by writing 1*/
	if (IS_BIT_SET(hdmi_read_reg32(hdmi, HDMI_IH_PHY_STAT0_OFFSET),
				HDMI_IH_PHY_STAT0_HPD_START)) {
		hdmi_write_reg32(hdmi, HDMI_IH_PHY_STAT0_OFFSET,
			SET_FIELD(HDMI_IH_PHY_STAT0_HPD_START,
				HDMI_IH_PHY_STAT0_HPD_MASK, 1));
	}

	hdmi_init_interrupts(hdmi);

	/* Power off */
	hdmi_write_reg32(hdmi, HDMI_PHY_CONF0_OFFSET,
		SET_FIELD(HDMI_PHY_CONF0_TXPWRON_START, HDMI_PHY_CONF0_TXPWRON_MASK, 0) |
		SET_FIELD(HDMI_PHY_CONF0_PDDQ_START, HDMI_PHY_CONF0_PDDQ_MASK, 1) |
		SET_FIELD(HDMI_PHY_CONF0_SVSRET_START, HDMI_PHY_CONF0_SVSRET_MASK, 1));

	/* Enable hot plug detection */
	hdmi_mod_reg32(hdmi, HDMI_PHY_CONF0_OFFSET,
			HDMI_PHY_CONF0_ENHPDRXSENSE_MASK,
			HDMI_PHY_CONF0_ENHPDRXSENSE_MASK);

	/* Now flip the master switch to unmute */
	hdmi_write_reg32(hdmi, HDMI_IH_MUTE_OFFSET,
			SET_FIELD(HDMI_IH_MUTE_ALL_START,
			HDMI_IH_MUTE_ALL_MASK, 0));

}

static void hdmi_irq_handler(void *data)
{
	struct hdmi_device *hdmi = (struct hdmi_device *)data;
	u32 phy_stat0 = 0;
	u32 ih_phy_stat0 = 0;
	u32 hpd_polarity = 0;
	u32 decode = 0;

	HDMI_CHECKPOINT;

	/* Mute all interrupts */
	hdmi_write_reg32(hdmi, HDMI_IH_MUTE_OFFSET, 0x03);

	decode = hdmi_read_reg32(hdmi, HDMI_IH_DECODE_OFFSET);

	/* Only support interrupts on PHY (eg HPD) */
	if (!IS_BIT_SET(decode, HDMI_IH_DECODE_PHY_START)) {
		hdmi_info(hdmi, "%s: Unknown interrupt generated, decode: %x\n",
			__func__, hdmi_read_reg32(hdmi, HDMI_IH_DECODE_OFFSET));
		return;
	}

	phy_stat0 = hdmi_read_reg32(hdmi, HDMI_PHY_STAT0_OFFSET);
	hpd_polarity = GET_FIELD(hdmi_read_reg32(hdmi, HDMI_PHY_POL0_OFFSET),
					HDMI_PHY_POL0_HPD_START, HDMI_PHY_POL0_HPD_MASK);
	ih_phy_stat0 = hdmi_read_reg32(hdmi, HDMI_IH_PHY_STAT0_OFFSET);

	hdmi_info(hdmi, "%s: Hdmi interrupt detected PHYStat0: 0x%x, HPD Polarity: 0x%x, IH Decode: 0x%x, IH PhyStat0: 0x%x",
		__func__, phy_stat0, hpd_polarity, decode, ih_phy_stat0);

	/* Check if hot-plugging occurred */
	if (GET_FIELD(phy_stat0, HDMI_PHY_STAT0_HPD_START, HDMI_PHY_STAT0_HPD_MASK) == 1 &&
		GET_FIELD(ih_phy_stat0, HDMI_IH_PHY_STAT0_HPD_START, HDMI_IH_PHY_STAT0_HPD_MASK) == 1) {
		hdmi_info(hdmi, "%s: Hot plug detected", __func__);

		/* Flip polarity */
		hdmi_write_reg32(hdmi, HDMI_PHY_POL0_OFFSET,
			SET_FIELD(HDMI_PHY_POL0_HPD_START, HDMI_PHY_POL0_HPD_MASK, !hpd_polarity));

		/* Write 1 to clear interrupt */
		hdmi_write_reg32(hdmi, HDMI_IH_PHY_STAT0_OFFSET, ih_phy_stat0);

		/* Finish mode setup */
		if (hdmi->hpd_detect == 0) {
			hdmi->hpd_detect = 1;
			//hdmi_poweron(hdmi);   // do indelayed work
		}

		/* Mute non-HPD interrupts */
		hdmi_init_interrupts(hdmi);

		/* Now flip the master switch to unmute */
		hdmi_write_reg32(hdmi, HDMI_IH_MUTE_OFFSET, 0);

		queue_work(hdmi->workq, &hdmi->hpd_work);
	} else {
		hdmi_info(hdmi, "%s: Cable unplugged\n", __func__);

		/* Flip polarity */
		hdmi_write_reg32(hdmi, HDMI_PHY_POL0_OFFSET,
			SET_FIELD(HDMI_PHY_POL0_HPD_START, HDMI_PHY_POL0_HPD_MASK, !hpd_polarity));

		/* Write 1 to clear interrupts */
		hdmi_write_reg32(hdmi, HDMI_IH_PHY_STAT0_OFFSET, ih_phy_stat0);

		/* Unmute and enable HPD interrupt */
		hdmi_enable_hpd(hdmi);

		/* Cable was unplugged */
		hdmi->hpd_detect = 0;
	}
}
#endif  // HDMI_USE_HPD_INTERRUPTS

static void hdmi_delayed_hpd(struct work_struct *work)
{
	struct hdmi_device *hdmi = container_of(work, struct hdmi_device, hpd_work);
	struct drm_connector *connector = &hdmi->connector;

	HDMI_CHECKPOINT;

	drm_kms_helper_hotplug_event(connector->dev);
}

static int hdmi_register(struct hdmi_device *hdmi)
{
	struct pdp_drm_private *pdp_priv = hdmi->drm_dev->dev_private;

	// we should make our own container connector object and put queue into that.
	INIT_WORK(&hdmi->hpd_work, hdmi_delayed_hpd);

	hdmi->connector.polled = DRM_CONNECTOR_POLL_HPD;
	hdmi->connector.interlace_allowed = 0;
	hdmi->connector.doublescan_allowed = 0;
	drm_connector_init(hdmi->drm_dev, &hdmi->connector, &hdmi_connector_funcs,
			DRM_MODE_CONNECTOR_HDMIA);
	drm_connector_helper_add(&hdmi->connector,
		&hdmi_connector_helper_funcs);

	hdmi->encoder.possible_crtcs = BIT(0);
	drm_encoder_init(hdmi->drm_dev, &hdmi->encoder, &hdmi_encoder_funcs,
			 DRM_MODE_ENCODER_TMDS, NULL);
	drm_encoder_helper_add(&hdmi->encoder, &hdmi_encoder_helper_funcs);

	drm_connector_attach_encoder(&hdmi->connector, &hdmi->encoder);

	// save the connector in the pdp priv which gets freed by tc layer.
	pdp_priv->connector = &hdmi->connector;

	drm_kms_helper_poll_init(hdmi->drm_dev);

	return 0;
}

static void hdmi_destroy(struct hdmi_device *hdmi)
{
	drm_kms_helper_poll_fini(hdmi->drm_dev);

	// disable hot plug detection, its interrupt
	hdmi_disable_hpd(hdmi);
	plato_disable_interrupt(hdmi->dev->parent, PLATO_INTERRUPT_HDMI);
	plato_set_interrupt_handler(hdmi->dev->parent, PLATO_INTERRUPT_HDMI, NULL, NULL);

	if (hdmi->workq) {
		flush_workqueue(hdmi->workq);
		destroy_workqueue(hdmi->workq);
		hdmi->workq = NULL;
	}

	hdmi_i2c_deinit(hdmi);

	// all allocs and mappings use devm so will be freed/unmapped on device shutdown.
	dev_set_drvdata(hdmi->dev, NULL);
}


static int hdmi_init(struct device *dev, struct device *master, void *data)
{
	int err;
	struct hdmi_device *hdmi;
	struct resource *regs_resource;
	struct platform_device *pdev = to_platform_device(dev);

	hdmi = devm_kzalloc(dev, sizeof(*hdmi), GFP_KERNEL);
	if (!hdmi)
		return -ENOMEM;

	hdmi->dev = dev;
	hdmi->drm_dev = data;

	hdmi_info(hdmi, "DRM device: %p\n", hdmi->drm_dev);

	hdmi_info(hdmi, "max_width is %d\n",
		hdmi->drm_dev->mode_config.max_width);
	hdmi_info(hdmi, "max_height is %d\n",
		hdmi->drm_dev->mode_config.max_height);

	regs_resource = platform_get_resource_byname(pdev, IORESOURCE_MEM, PLATO_HDMI_RESOURCE_REGS);
	if (regs_resource == NULL) {
		dev_err(dev, "%s: failed to get register resource %s", __func__, PLATO_HDMI_RESOURCE_REGS);
		return -ENXIO;
	}

	/* Use managed mmio, OS handles free on destroy */
	hdmi->core_regs = devm_ioremap_resource(dev, regs_resource);
	if (IS_ERR(hdmi->core_regs)) {
		dev_err(dev, "%s: Failed to map HDMI registers", __func__);
		return PTR_ERR(hdmi->core_regs);
	}

	/* Product and revision IDs */
	hdmi_info(hdmi, "\n\nDesign ID: %d\nRev ID: %d\nProduct ID: %d\nProduct ID HDCP: %d\n",
		hdmi_read_reg32(hdmi, HDMI_ID_DESIGN_ID_OFFSET),
		hdmi_read_reg32(hdmi, HDMI_ID_REVISION_ID_OFFSET),
		hdmi_read_reg32(hdmi, HDMI_ID_PRODUCT_ID0_OFFSET),
		GET_FIELD(hdmi_read_reg32(hdmi, HDMI_ID_PRODUCT_ID1_OFFSET),
			HDMI_ID_PRODUCT_ID1_HDCP_START, HDMI_ID_PRODUCT_ID1_HDCP_MASK));

	hdmi_info(hdmi, "\nHDCP Present: %d\nHDMI 1.4: %d\nHDMI 2.0: %d\nPHY Type: %d\n\n",
		GET_FIELD(hdmi_read_reg32(hdmi, HDMI_ID_CONFIG0_ID_OFFSET),
			HDMI_ID_CONFIG0_ID_HDCP_START, HDMI_ID_CONFIG0_ID_HDCP_MASK),
		GET_FIELD(hdmi_read_reg32(hdmi, HDMI_ID_CONFIG0_ID_OFFSET),
			HDMI_ID_CONFIG0_ID_HDMI14_START, HDMI_ID_CONFIG0_ID_HDMI14_MASK),
		GET_FIELD(hdmi_read_reg32(hdmi, HDMI_ID_CONFIG1_ID_OFFSET),
			HDMI_ID_CONFIG1_ID_HDMI20_START, HDMI_ID_CONFIG1_ID_HDMI20_MASK),
		hdmi_read_reg32(hdmi, HDMI_ID_CONFIG2_ID_OFFSET));

	/* Step A: Initialise PHY */
	err = phy_init(hdmi);
	if (err != 0) {
		hdmi_error(hdmi, "%s: Failed to initialise PHY (err %d)\n", __func__, err);
		return err;
	}

	/* Step B: Initialise video/frame composer and DTD for VGA/DVI mode */
	/* Don't need this with DRM? */
	err = video_init(hdmi);
	if (err != 0) {
		hdmi_error(hdmi, "%s: Failed to initialise Video (err %d)\n", __func__, err);
		return err;
	}

	/* Step B: Initialise EDID/I2C */
	err = hdmi_i2c_init(hdmi);
	if (err != 0) {
		hdmi_error(hdmi, "Failed to initialise I2C interface (err %d)", err);
		return err;
	}

	/* 16 - 1920x1080@60Hz */
	hdmi->hdmi_data.vic = 16;
	hdmi->native_mode = drm_mode_duplicate(hdmi->drm_dev, &forced_mode);

	hdmi->hdmi_data.video_mode.pixel_repetition_input = 0;
	hdmi->hdmi_data.enc_in_format = ENCODING_RGB;
	hdmi->hdmi_data.enc_out_format = ENCODING_RGB;
	hdmi->hdmi_data.enc_color_depth = 8;    // Nick's original was 10. Why?
	hdmi->hdmi_data.pix_repet_factor = 0;
	hdmi->hdmi_data.pp_default_phase = 0;
	hdmi->hdmi_data.hdcp_enable = 0;
	hdmi->hdmi_data.colorimetry = 0;
	hdmi->hdmi_data.active_aspect_ratio = 8;

	/* Non-drm mode information */
	hdmi->hdmi_data.video_mode.data_enable_polarity = true;
	hdmi->hdmi_data.video_mode.dvi = 0;
	hdmi->hdmi_data.video_mode.hsync_polarity  = 1;
	hdmi->hdmi_data.video_mode.vsync_polarity = 1;
	hdmi->hdmi_data.video_mode.interlaced = 0;
	hdmi->hdmi_data.video_mode.pixel_repetition_input = 0;
	hdmi->hdmi_data.video_mode.pixel_repetition_output = 0;

	// drm_mode_probed_add(hdmi->connector, hdmi->native_mode);

	hdmi_mode_setup(hdmi, hdmi->native_mode);

	hdmi->workq = alloc_ordered_workqueue("plato_hdmi_hpdq", 0);
	if (!hdmi->workq) {
		dev_info(dev, "failed to alloc ordered workqueue\n");
		return 1;
	}

#if defined(HDMI_USE_HPD_INTERRUPTS)
	err = plato_set_interrupt_handler(hdmi->dev->parent,
					PLATO_INTERRUPT_HDMI,
					hdmi_irq_handler,
					hdmi);
	if (err) {
		dev_info(dev, "failed to set interrupt handler (err=%d)\n", err);
		return err;
	}

	err = plato_enable_interrupt(hdmi->dev->parent, PLATO_INTERRUPT_HDMI);
	if (err) {
		dev_info(dev, "failed to enable HDMI interrupts (err=%d)\n", err);
		return err;
	}

	hdmi_enable_hpd(hdmi);
#endif  // HDMI_USE_HPD_INTERRUPTS

	mdelay(100);
	err = hdmi_register(hdmi);
	if (err) {
		dev_err(dev, "Failed to register HDMI device (err %d)", err);
		return err;
	}

	hdmi_info(hdmi, "%s: Number of FBs: %d\n", __func__,
				hdmi->drm_dev->mode_config.num_fb);

	dev_set_drvdata(hdmi->dev, hdmi);

	return 0;
}

static int hdmi_component_bind(struct device *dev, struct device *master, void *data)
{
	HDMI_CHECKPOINT;
	dev_info(dev, "loading platform device");

	return hdmi_init(dev, master, data);
}

static void hdmi_component_unbind(struct device *dev, struct device *master, void *data)
{
	struct hdmi_device *hdmi = dev_get_drvdata(dev);

	HDMI_CHECKPOINT;
	dev_info(dev, "unloading platform device");

	hdmi_destroy(hdmi);
}

static const struct component_ops hdmi_component_ops = {
	.bind   = hdmi_component_bind,
	.unbind = hdmi_component_unbind,
};

static int hdmi_probe(struct platform_device *pdev)
{
	HDMI_CHECKPOINT;
	return component_add(&pdev->dev, &hdmi_component_ops);
}

static int hdmi_remove(struct platform_device *pdev)
{
	HDMI_CHECKPOINT;
	component_del(&pdev->dev, &hdmi_component_ops);
	return 0;
}

static struct platform_device_id hdmi_platform_device_id_table[] = {
	{ .name = PLATO_DEVICE_NAME_HDMI, .driver_data = HDMI_VERSION_PLATO },
	{ },
};

static struct platform_driver hdmi_platform_driver = {
	.probe    = hdmi_probe,
	.remove   = hdmi_remove,
	.driver   = {
		.owner  = THIS_MODULE,
		.name   = DRIVER_NAME,
	},
	.id_table = hdmi_platform_device_id_table,
};

module_platform_driver(hdmi_platform_driver);

MODULE_AUTHOR("Imagination Technologies Ltd. <gpl-support@imgtec.com>");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("Dual MIT/GPL");
