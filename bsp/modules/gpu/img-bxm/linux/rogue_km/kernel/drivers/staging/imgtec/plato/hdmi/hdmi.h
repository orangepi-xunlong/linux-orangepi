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

#ifndef _DRM_HDMI_H_
#define _DRM_HDMI_H_

#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0))
#include <drm/drmP.h>
#endif

#include <drm/drm_modes.h>

#include <linux/device.h>
#include <linux/module.h>

#include <drm/drm_encoder.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0))
#include <linux/i2c.h>
#endif

#define HDMI_EDID_LEN                   512

#define ENCODING_RGB                    0
#define ENCODING_YCC444                 1
#define ENCODING_YCC422_16BITS          2
#define ENCODING_YCC422_8BITS           3
#define ENCODING_XVYCC444               4

struct hdmi_device_vmode {
	bool dvi;
	bool hsync_polarity;
	bool vsync_polarity;
	bool interlaced;
	bool data_enable_polarity;

	u32 pixel_clock;
	u32 pixel_repetition_input;
	u32 pixel_repetition_output;
};

struct hdmi_device_data_info {
	u32 enc_in_format;
	u32 enc_out_format;
	u32 enc_color_depth;
	u32 colorimetry;
	u32 pix_repet_factor;
	u32 pp_default_phase;
	u32 hdcp_enable;
	u32 active_aspect_ratio;
	int vic;
	struct hdmi_device_vmode video_mode;
};

struct hdmi_i2c {
	struct i2c_adapter adap;

	u32 ddc_addr;
	u32 segment_ptr;

	struct mutex lock;
	struct completion comp;
};

struct hdmi_device {
	struct drm_device *drm_dev;
	struct drm_connector connector;
	struct drm_encoder encoder;
	struct hdmi_i2c *i2c;
	struct device *dev;

	struct workqueue_struct *workq;
	struct work_struct hpd_work;

	/* Video parameters */
	struct hdmi_device_data_info hdmi_data;
	struct edid *edid;
	struct drm_display_mode *native_mode;
	unsigned int vrefresh;

	bool hpd_detect;
	bool phy_enabled;
	void __iomem *core_regs;
	void __iomem *top_regs;
};

#define connector_to_hdmi(c) \
	struct hdmi_device *hdmi = container_of((c), struct hdmi_device, connector)
#define encoder_to_hdmi(e) \
	struct hdmi_device *hdmi = container_of((e), struct hdmi_device, encoder)


enum HDMI_STATUS {
	HDMI_INIT_SUCCESS = 0,
	HDMI_INIT_FAILED_VIDEO,
	HDMI_INIT_FAILED_PHY,
	HDMI_INIT_FAILED_EDID,
	HDMI_INIT_FAILED_HDCP,
};

/* Core APIs */
inline void hdmi_write_reg32(struct hdmi_device *hdmi, int offset, u32 val);
inline u32 hdmi_read_reg32(struct hdmi_device *hdmi, int offset);
inline void hdmi_mod_reg32(struct hdmi_device *hdmi, u32 offset, u32 data, u32 mask);

/* I2C APIs */
int hdmi_i2c_init(struct hdmi_device *hdmi);
void hdmi_i2c_deinit(struct hdmi_device *hdmi);

#if defined(PRINT_HDMI_REGISTERS)
void PrintVideoRegisters(struct hdmi_device *hdmi);
#endif

#define IS_BIT_SET(value, bit) ((value) & (1 << (bit)))

#if defined(HDMI_DEBUG)
	#define hdmi_info(hdmi, fmt, ...)   dev_info(hdmi->dev, fmt, ##__VA_ARGS__)
	#define hdmi_error(hdmi, fmt, ...)  dev_err(hdmi->dev, fmt, ##__VA_ARGS__)
	#define HDMI_CHECKPOINT pr_debug("%s: line %d\n", __func__, __LINE__)
#else
	#define hdmi_info(hdmi, fmt, ...)
	#define hdmi_error(hdmi, fmt, ...)
	#define HDMI_CHECKPOINT
#endif

#endif
