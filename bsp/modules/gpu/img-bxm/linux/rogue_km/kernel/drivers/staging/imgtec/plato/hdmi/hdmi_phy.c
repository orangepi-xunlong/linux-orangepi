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

#include <linux/delay.h>
#include "hdmi_phy.h"
#include "hdmi_regs.h"

/* PHY Gen2 slave addr */
#if defined(EMULATOR)
#define HDMI_PHY_SLAVE_ADDRESS                  0xA0
#else
#define HDMI_PHY_SLAVE_ADDRESS                  0x00
#endif

#if defined(VIRTUAL_PLATFORM)
#define HDMI_PHY_TIMEOUT_MS                     1
#define HDMI_PHY_POLL_INTERVAL_MS               1
/* PHY dependent time for reset */
#define HDMI_PHY_RESET_TIME_MS                  1
#else
/* Timeout for waiting on PHY operations */
#define HDMI_PHY_TIMEOUT_MS                     200
#define HDMI_PHY_POLL_INTERVAL_MS               100
/* PHY dependent time for reset */
#define HDMI_PHY_RESET_TIME_MS                  10
#endif

/* PHY I2C register definitions */
#define PHY_PWRCTRL_OFFSET                      (0x00)
#define PHY_SERDIVCTRL_OFFSET                   (0x01)
#define PHY_SERCKCTRL_OFFSET                    (0x02)
#define PHY_SERCKKILLCTRL_OFFSET                (0x03)
#define PHY_TXRESCTRL_OFFSET                    (0x04)
#define PHY_CKCALCTRL_OFFSET                    (0x05)
#define PHY_OPMODE_PLLCFG_OFFSET                (0x06)
#define PHY_CLKMEASCTRL_OFFSET                  (0x07)
#define PHY_TXMEASCTRL_OFFSET                   (0x08)
#define PHY_CKSYMTXCTRL_OFFSET                  (0x09)
#define PHY_CMPSEQCTRL_OFFSET                   (0x0A)
#define PHY_CMPPWRCTRL_OFFSET                   (0x0B)
#define PHY_CMPMODECTRL_OFFSET                  (0x0C)
#define PHY_MEASCTRL_OFFSET                     (0x0D)
#define PHY_VLEVCTRL_OFFSET                     (0x0E)
#define PHY_D2ACTRL_OFFSET                      (0x0F)
#define PHY_PLLCURRCTRL_OFFSET                  (0x10)
#define PHY_PLLDRVANACTRL_OFFSET                (0x11)
#define PHY_PLLCTRL_OFFSET                      (0x14)
#define PHY_PLLGMPCTRL_OFFSET                   (0x15)
#define PHY_PLLMEASCTRL_OFFSET                  (0x16)
#define PHY_PLLCLKBISTPHASE_OFFSET              (0x17)
#define PHY_COMPRCAL_OFFSET                     (0x18)
#define PHY_TXTERM_OFFSET                       (0x19)
#define PHY_PWRSEQ_PATGENSKIP_OFFSET            (0x1A)
/* Leaving out BIST regs */

static bool phy_poll(struct hdmi_device *hdmi, int offset, u32 mask)
{
	u16 timeout = 0;

	while (!(hdmi_read_reg32(hdmi, offset) & mask) &&
						timeout < HDMI_PHY_TIMEOUT_MS) {
		msleep(HDMI_PHY_POLL_INTERVAL_MS);
		timeout += HDMI_PHY_POLL_INTERVAL_MS;
	}
	if (timeout == HDMI_PHY_TIMEOUT_MS) {
		dev_err(hdmi->dev, "%s: PHY timed out polling on 0x%x in register offset 0x%x\n", __func__, mask, offset);
		return false;
	}
	return true;
}

#if defined(NEED_PHY_READ)
static u16 phy_i2c_read(struct hdmi_device *hdmi, u8 addr)
{
	u16 data;

	hdmi_write_reg32(hdmi, HDMI_PHY_I2CM_SLAVE_OFFSET, HDMI_PHY_SLAVE_ADDRESS);
	hdmi_write_reg32(hdmi, HDMI_PHY_I2CM_ADDRESS_OFFSET, addr);
	hdmi_write_reg32(hdmi, HDMI_PHY_I2CM_SLAVE_OFFSET, HDMI_PHY_SLAVE_ADDRESS);
	hdmi_write_reg32(hdmi, HDMI_PHY_I2CM_ADDRESS_OFFSET, addr);
	hdmi_write_reg32(hdmi, HDMI_PHY_I2CM_OPERATION_OFFSET,
				SET_FIELD(HDMI_PHY_I2CM_OPERATION_RD_START,
				HDMI_PHY_I2CM_OPERATION_RD_MASK, 1));

	/* Wait for done indication */
	//phy_poll(base, HDMI_IH_I2CMPHY_STAT0_OFFSET, HDMI_IH_I2CMPHY_STAT0_I2CMPHYDONE_MASK, ERROR);
	mdelay(10);
	/* read the data registers */
	data  = (hdmi_read_reg32(hdmi, HDMI_PHY_I2CM_DATAI_1_OFFSET) & 0xFF) << 8;
	data |= (hdmi_read_reg32(hdmi, HDMI_PHY_I2CM_DATAI_0_OFFSET) & 0xFF);

	return data;
}
#endif	// NEED_PHY_READ

static int phy_i2c_write(struct hdmi_device *hdmi, u8 addr, u16 data)
{
	HDMI_CHECKPOINT;

	hdmi_write_reg32(hdmi, HDMI_PHY_I2CM_SLAVE_OFFSET, HDMI_PHY_SLAVE_ADDRESS);
	hdmi_write_reg32(hdmi, HDMI_PHY_I2CM_ADDRESS_OFFSET, addr);
	hdmi_write_reg32(hdmi, HDMI_PHY_I2CM_DATAO_1_OFFSET, (data >> 8));
	hdmi_write_reg32(hdmi, HDMI_PHY_I2CM_DATAO_0_OFFSET, data & 0xFF);
	hdmi_write_reg32(hdmi, HDMI_PHY_I2CM_OPERATION_OFFSET,
	SET_FIELD(HDMI_PHY_I2CM_OPERATION_WR_START,
					HDMI_PHY_I2CM_OPERATION_WR_MASK, 1));

	/* Wait for done interrupt */
	if (!phy_poll(hdmi, HDMI_IH_I2CMPHY_STAT0_OFFSET,
					HDMI_IH_I2CMPHY_STAT0_I2CMPHYDONE_MASK))
		return -ETIMEDOUT;

	hdmi_write_reg32(hdmi, HDMI_IH_I2CMPHY_STAT0_OFFSET, 0x3);
	return 0;
}

/*
 * Configure PHY based on pixel clock, color resolution, pixel repetition, and PHY model
 * NOTE: This assumes PHY model TSMC 28-nm HPM/ 1.8V
 */
void phy_configure_mode(struct hdmi_device *hdmi, struct drm_display_mode *mode)
{
	int pixel_clock;
	u8 color_depth;

	HDMI_CHECKPOINT;

	pixel_clock = mode->clock / 10;
	color_depth = hdmi->hdmi_data.enc_color_depth;

	hdmi_write_reg32(hdmi, HDMI_PHY_CONF0_OFFSET,
		SET_FIELD(HDMI_PHY_CONF0_SELDATAENPOL_START,
					HDMI_PHY_CONF0_SELDATAENPOL_MASK, 1) |
		SET_FIELD(HDMI_PHY_CONF0_ENHPDRXSENSE_START,
					HDMI_PHY_CONF0_ENHPDRXSENSE_MASK, 1) |
		SET_FIELD(HDMI_PHY_CONF0_PDDQ_START,
					HDMI_PHY_CONF0_PDDQ_MASK, 1));

	/* RESISTANCE TERM 133Ohm Cfg */
	phy_i2c_write(hdmi, PHY_TXTERM_OFFSET, 0x0004); /* TXTERM */
	/* REMOVE CLK TERM */
	//phy_i2c_write(hdmi, PHY_CKCALCTRL_OFFSET, 0x8000); /* CKCALCTRL */

	hdmi_info(hdmi, " %s: pixel clock: %d, color_depth: %d\n", __func__,
				pixel_clock, color_depth);

	switch (pixel_clock) {
	case 2520:
		switch (color_depth) {
		case 8:
			/* PLL/MPLL Cfg */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET, 0x00b3);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,   0x0018); /* CURRCTRL */
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,    0x0000); /* GMPCTRL */
			break;
		case 10: /* TMDS = 31.50MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET, 0x2153);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,   0x0018);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,    0x0000);
			break;
		case 12: /* TMDS = 37.80MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET, 0x40F3);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,   0x0018);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,    0x0000);
			break;
		case 16: /* TMDS = 50.40MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET, 0x60B2);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,   0x0028);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,    0x0001);
			break;
		default:
			dev_err(hdmi->dev, "%s: Color depth not supported (%d)",
						__func__, color_depth);
			break;
		}
		/* PREEMP Cgf 0.00 */
		//phy_i2c_write(hdmi, PHY_CKSYMTXCTRL_OFFSET, 0x8009); /* CKSYMTXCTRL */
		/* TX/CK LVL 10 */
		//phy_i2c_write(hdmi, PHY_VLEVCTRL_OFFSET, 0x0251); /* VLEVCTRL */
		break;
	case 2700:
		switch (color_depth) {
		case 8:
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET,  0x00B3);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,    0x0018);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,     0x0000);
			break;
		case 10: /* TMDS = 33.75MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET,  0x2153);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,    0x0018);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,     0x0000);
			break;
		case 12: /* TMDS = 40.50MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET,  0x40F3);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,    0x0018);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,     0x0000);
			break;
		case 16: /* TMDS = 54MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET,  0x60B2);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,    0x0028);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,     0x0001);
			break;
		default:
			dev_err(hdmi->dev, "%s: Color depth not supported (%d)",
						__func__, color_depth);
			break;
		}
		//phy_i2c_write(hdmi, 0x8009, 0x09);
		//phy_i2c_write(hdmi, 0x0251, 0x0E);
		break;
	case 5040:
		switch (color_depth) {
		case 8:
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET,  0x0072);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,    0x0028);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,     0x0001);
			break;
		case 10: /* TMDS = 63MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET,  0x2142);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,    0x0028);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,     0x0001);
			break;
		case 12: /* TMDS = 75.60MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET,  0x40A2);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,    0x0028);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,     0x0001);
			break;
		case 16: /* TMDS = 100.80MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET,  0x6071);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,    0x0038);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,     0x0002);
			break;
		default:
			dev_err(hdmi->dev, "%s: Color depth not supported (%d)",
						__func__, color_depth);
			break;
		}
		//phy_i2c_write(hdmi, 0x8009, 0x09);
		//phy_i2c_write(hdmi, 0x0251, 0x0E);
		break;
	case 5400:
		switch (color_depth) {
		case 8:
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET,  0x0072);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,    0x0028);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,     0x0001);
			break;
		case 10: /* TMDS = 67.50MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET,  0x2142);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,    0x0028);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,     0x0001);
			break;
		case 12: /* TMDS = 81MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET,  0x40A2);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,    0x0028);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,     0x0001);
			break;
		case 16: /* TMDS = 108MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET,  0x6071);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,    0x0038);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,     0x0002);
			break;
		default:
			dev_err(hdmi->dev, "%s: Color depth not supported (%d)",
						__func__, color_depth);
			break;
		}
		//phy_i2c_write(hdmi, 0x8009, 0x09);
		//phy_i2c_write(hdmi, 0x0251, 0x0E);
		break;
	case 5940:
		switch (color_depth) {
		case 8:
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET,  0x0072);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,    0x0028);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,     0x0001);
			break;
		case 10: /* TMDS = 74.25MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET,  0x2142);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,    0x0028);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,     0x0001);
			break;
		case 12: /* TMDS = 89.10MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET,  0x40A2);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,    0x0028);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,     0x0001);
			break;
		case 16: /* TMDS = 118.80MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET,  0x6071);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,    0x0038);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,     0x0002);
			break;
		default:
			dev_err(hdmi->dev, "%s: Color depth not supported (%d)",
						__func__, color_depth);
			break;
		}
		//phy_i2c_write(hdmi, 0x8009, 0x09);
		//phy_i2c_write(hdmi, 0x0251, 0x0E);
		break;
	case 6500:
		if (color_depth == 8) {
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET,  0x0072);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,    0x0028);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,     0x0001);
		} else {
			dev_err(hdmi->dev, "%s: Color depth not supported (%d)",
				__func__, color_depth);
		}
		break;
	case 7200:
		switch (color_depth) {
		case 8:
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET,  0x0072);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,    0x0028);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,     0x0001);
			break;
		case 10: /* TMDS = 90MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET,  0x2142);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,    0x0028);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,     0x0001);
			break;
		case 12: /* TMDS = 108MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET,  0x4061);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,    0x0038);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,     0x0002);
			break;
		case 16: /* TMDS = 144MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET,  0x6071);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,    0x0038);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,     0x0002);
			break;
		default:
			dev_err(hdmi->dev, "%s: Color depth not supported (%d)",
						__func__, color_depth);
			break;
		}
		//phy_i2c_write(hdmi, 0x8009, 0x09);
		//phy_i2c_write(hdmi, 0x0251, 0x0E);
		break;
	/* 74.25 MHz pixel clock (720p)*/
	case 7425:
		switch (color_depth) {
		case 8:
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET, 0x0072);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET, 0x0028);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET, 0x0001);
			break;
		case 10: /* TMDS = 92.812MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET, 0x2145);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET, 0x0038);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET, 0x0002);
			break;
		case 12: /* TMDS = 111.375MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET, 0x4061);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET, 0x0038);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET, 0x0002);
			break;
		case 16: /* TMDS = 148.5MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET, 0x6071);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET, 0x0038);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET, 0x0002);
			break;
		default:
			dev_err(hdmi->dev, "%s: Color depth not supported (%d)",
						__func__, color_depth);
			break;
		}
		//phy_i2c_write(hdmi, PHY_CKSYMTXCTRL_OFFSET, 0x8009);
		//phy_i2c_write(hdmi, PHY_VLEVCTRL_OFFSET, 0x0251);
		break;
	case 10080:
		switch (color_depth) {
		case 8:
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET, 0x0051);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,   0x0038);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,    0x0002);
			phy_i2c_write(hdmi, PHY_CKSYMTXCTRL_OFFSET,   0x8009);
			phy_i2c_write(hdmi, PHY_VLEVCTRL_OFFSET,      0x0251);
			break;
		case 10: /* TMDS = 126MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET, 0x2145);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,   0x0038);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,    0x0002);
			phy_i2c_write(hdmi, PHY_CKSYMTXCTRL_OFFSET,   0x8009);
			phy_i2c_write(hdmi, PHY_VLEVCTRL_OFFSET,      0x0251);
			break;
		case 12: /* TMDS = 151.20MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET, 0x4061);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,   0x0038);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,    0x0002);
			phy_i2c_write(hdmi, PHY_CKSYMTXCTRL_OFFSET,   0x8009);
			phy_i2c_write(hdmi, PHY_VLEVCTRL_OFFSET,      0x0251);
			break;
		case 16: /* TMDS = 201.60MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET, 0x6050);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,   0x0038);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,    0x0003);
			phy_i2c_write(hdmi, PHY_CKSYMTXCTRL_OFFSET,   0x8009);
			phy_i2c_write(hdmi, PHY_VLEVCTRL_OFFSET,      0x0211);
			break;
		default:
			dev_err(hdmi->dev, "%s: Color depth not supported (%d)",
						__func__, color_depth);
			break;
		}
		break;
	case 10100:
	case 10225:
	case 10650:
	case 10800:
		switch (color_depth) {
		case 8:
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET, 0x0051);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,   0x0038);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,    0x0002);
			//phy_i2c_write(hdmi, PHY_CKSYMTXCTRL_OFFSET,   0x8009);
			phy_i2c_write(hdmi, PHY_VLEVCTRL_OFFSET,      0x0251);
			break;
		case 10: /* TMDS = 135MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET, 0x2145);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,   0x0038);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,    0x0002);
			phy_i2c_write(hdmi, PHY_CKSYMTXCTRL_OFFSET,   0x8009);
			phy_i2c_write(hdmi, PHY_VLEVCTRL_OFFSET,      0x0251);
			break;
		case 12: /* TMDS = 162MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET, 0x4061);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,   0x0038);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,    0x0002);
			phy_i2c_write(hdmi, PHY_CKSYMTXCTRL_OFFSET,   0x8009);
			phy_i2c_write(hdmi, PHY_VLEVCTRL_OFFSET,      0x0211);
			break;
		case 16: /* TMDS = 216MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET, 0x6050);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,   0x0038);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,    0x0003);
			phy_i2c_write(hdmi, PHY_CKSYMTXCTRL_OFFSET,   0x8009);
			phy_i2c_write(hdmi, PHY_VLEVCTRL_OFFSET,      0x0211);
			break;
		default:
			dev_err(hdmi->dev, "%s: Color depth not supported (%d)",
						__func__, color_depth);
			break;
		}
		break;
	case 11880:
		switch (color_depth) {
		case 8:
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET, 0x0051);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,   0x0038);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,    0x0002);
			phy_i2c_write(hdmi, PHY_CKSYMTXCTRL_OFFSET,   0x8009);
			phy_i2c_write(hdmi, PHY_VLEVCTRL_OFFSET,      0x0251);
			break;
		case 10: /* TMDS = 148.50MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET, 0x2145);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,   0x0038);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,    0x0002);
			phy_i2c_write(hdmi, PHY_CKSYMTXCTRL_OFFSET,   0x8009);
			phy_i2c_write(hdmi, PHY_VLEVCTRL_OFFSET,      0x0251);
			break;
		case 12: /* TMDS = 178.20MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET, 0x4061);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,   0x0038);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,    0x0002);
			phy_i2c_write(hdmi, PHY_CKSYMTXCTRL_OFFSET,   0x8009);
			phy_i2c_write(hdmi, PHY_VLEVCTRL_OFFSET,      0x0211);
			break;
		case 16: /* TMDS = 237.60MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET, 0x6050);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,   0x0038);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,    0x0003);
			phy_i2c_write(hdmi, PHY_CKSYMTXCTRL_OFFSET,   0x8009);
			phy_i2c_write(hdmi, PHY_VLEVCTRL_OFFSET,      0x0211);
			break;
		default:
			dev_err(hdmi->dev, "%s: Color depth not supported (%d)",
						__func__, color_depth);
			break;
		}
		break;
	case 14400:
		switch (color_depth) {
		case 8:
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET, 0x0051);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,   0x0038);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,    0x0002);
			phy_i2c_write(hdmi, PHY_CKSYMTXCTRL_OFFSET,   0x8009);
			phy_i2c_write(hdmi, PHY_VLEVCTRL_OFFSET,      0x0251);
			break;
		case 10: /* TMDS = 180MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET, 0x2145);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,   0x0038);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,    0x0002);
			phy_i2c_write(hdmi, PHY_CKSYMTXCTRL_OFFSET,   0x8009);
			phy_i2c_write(hdmi, PHY_VLEVCTRL_OFFSET,      0x0211);
			break;
		case 12: /* TMDS = 216MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET, 0x4064);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,   0x0038);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,    0x0002);
			phy_i2c_write(hdmi, PHY_CKSYMTXCTRL_OFFSET,   0x8009);
			phy_i2c_write(hdmi, PHY_VLEVCTRL_OFFSET,      0x0211);
			break;
		case 16: /* TMDS = 288MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET, 0x6050);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,   0x0038);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,    0x0003);
			phy_i2c_write(hdmi, PHY_CKSYMTXCTRL_OFFSET,   0x8009);
			phy_i2c_write(hdmi, PHY_VLEVCTRL_OFFSET,      0x0211);
			break;
		default:
			dev_err(hdmi->dev, "%s: Color depth not supported (%d)",
						__func__, color_depth);
			break;
		}
		break;
	case 14625:
		switch (color_depth) {
		case 8:
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET, 0x0051);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,   0x0038);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,    0x0002);
			phy_i2c_write(hdmi, PHY_CKSYMTXCTRL_OFFSET,   0x8009);
			phy_i2c_write(hdmi, PHY_VLEVCTRL_OFFSET,      0x0251);
			break;
		default:
			dev_err(hdmi->dev, "%s: Color depth not supported (%d)",
						__func__, color_depth);
			break;
		}
		break;
	case 14850:
		switch (color_depth) {
		case 8:
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET, 0x0051);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,   0x0038);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,    0x0002);
			phy_i2c_write(hdmi, PHY_VLEVCTRL_OFFSET,      0x0251);
			break;
		case 10: /* TMDS = 185.62MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET, 0x214C);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET, 0x0038);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET, 0x0003);
			phy_i2c_write(hdmi, PHY_VLEVCTRL_OFFSET, 0x0211);
			break;
		case 12: /* TMDS = 222.75MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET, 0x4064);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET, 0x0038);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET, 0x0003);
			phy_i2c_write(hdmi, PHY_VLEVCTRL_OFFSET, 0x0211);
			break;
		case 16: /* TMDS = 297MHz */
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET, 0x6050);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET, 0x0038);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET, 0x0003);
			phy_i2c_write(hdmi, PHY_VLEVCTRL_OFFSET, 0x0273);
			break;
		default:
			dev_err(hdmi->dev, "%s: Color depth not supported (%d)",
						__func__, color_depth);
			break;
		}
		phy_i2c_write(hdmi, PHY_CKSYMTXCTRL_OFFSET, 0x8009);
		break;
	case 15400:
		switch (color_depth) {
		case 8:
			phy_i2c_write(hdmi, PHY_OPMODE_PLLCFG_OFFSET, 0x0051);
			phy_i2c_write(hdmi, PHY_PLLCURRCTRL_OFFSET,   0x0038);
			phy_i2c_write(hdmi, PHY_PLLGMPCTRL_OFFSET,    0x0002);
			phy_i2c_write(hdmi, PHY_VLEVCTRL_OFFSET,      0x0251);
			break;
		default:
			dev_err(hdmi->dev, "%s: Color depth not supported (%d)",
						__func__, color_depth);
			break;
		}
		break;
	default:
		dev_err(hdmi->dev, "%s: Pixel clock not supported (%d)",
			__func__, pixel_clock);
		break;
	}

	{
		u32 value = hdmi_read_reg32(hdmi, HDMI_PHY_CONF0_OFFSET);

		value = SET_FIELD(HDMI_PHY_CONF0_SELDATAENPOL_START,
					HDMI_PHY_CONF0_SELDATAENPOL_MASK, 1) |
			SET_FIELD(HDMI_PHY_CONF0_ENHPDRXSENSE_START,
					HDMI_PHY_CONF0_ENHPDRXSENSE_MASK, 1) |
			SET_FIELD(HDMI_PHY_CONF0_TXPWRON_START,
					HDMI_PHY_CONF0_TXPWRON_MASK, 1) |
			SET_FIELD(HDMI_PHY_CONF0_SVSRET_START,
					HDMI_PHY_CONF0_SVSRET_MASK, 1);
		hdmi_write_reg32(hdmi, HDMI_PHY_CONF0_OFFSET, value);
	}
}

static void phy_reset(struct hdmi_device *hdmi, u8 value)
{
	/* Handle different types of PHY here... */
	hdmi_write_reg32(hdmi, HDMI_MC_PHYRSTZ_OFFSET, value & 0x1);
	msleep(HDMI_PHY_RESET_TIME_MS);
}

int phy_power_down(struct hdmi_device *hdmi)
{
	/* For HDMI TX 1.4 PHY, power down by placing PHY in reset */
	phy_reset(hdmi, 0);
	phy_reset(hdmi, 1);
	phy_reset(hdmi, 0);
	phy_reset(hdmi, 1);
	return 0;
}

int phy_wait_lock(struct hdmi_device *hdmi)
{
	if (!phy_poll(hdmi, HDMI_PHY_STAT0_OFFSET, HDMI_PHY_STAT0_TX_PHY_LOCK_MASK))
		return -ETIMEDOUT;

	return 0;
}

int phy_init(struct hdmi_device *hdmi)
{
	/* Init slave address */
	hdmi_write_reg32(hdmi, HDMI_PHY_I2CM_SLAVE_OFFSET, HDMI_PHY_SLAVE_ADDRESS);
	return 0;
}
