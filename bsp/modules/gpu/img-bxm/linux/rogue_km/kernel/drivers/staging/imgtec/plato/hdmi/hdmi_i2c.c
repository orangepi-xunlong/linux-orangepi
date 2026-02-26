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
#include <linux/i2c.h>
#include <linux/delay.h>

#include "hdmi.h"
#include "hdmi_regs.h"

/*************************************************************************/ /*!
 EDID Information
*/ /**************************************************************************/
#define EDID_SIGNATURE                      0x00ffffffffffff00ULL
#define EDID_BLOCK_SIZE                     128
#define EDID_CHECKSUM                       0 // per VESA spec, 8 bit checksum of base block
#define EDID_SLAVE_ADDRESS                  0x50
#define EDID_SEGMENT_ADDRESS                0x00
#define EDID_REFRESH_RATE                   60000
#define EDID_I2C_TIMEOUT_MS                 1000

static int hdmi_i2c_read(struct hdmi_device *hdmi, struct i2c_msg *msg)
{
	u32 intr_reg = 0;
	u32 poll = 0;
	u8 *buf = msg->buf;
	u32 len = msg->len;
	u32 i = 0;

	if (len != EDID_BLOCK_SIZE)
		return 0;

	/* NOT THE RIGHT WAY TO DO THIS, NEED TO USE ADDR FIELD IN I2C_MSG */
	if (hdmi->i2c->ddc_addr >= 0xFF) {
		hdmi->i2c->ddc_addr = 0;
		hdmi->i2c->segment_ptr++;
	}
	hdmi_write_reg32(hdmi, HDMI_I2CM_SEGPTR_OFFSET, hdmi->i2c->segment_ptr);

	memset(buf, 0, len);

	for (i = 0; i < msg->len; i++) {
		/* Write 1 to clear interrupt status */
		hdmi_write_reg32(hdmi, HDMI_IH_I2CM_STAT0_OFFSET,
			SET_FIELD(HDMI_IH_I2CM_STAT0_I2CMASTERDONE_START,
				HDMI_IH_I2CM_STAT0_I2CMASTERDONE_MASK, 1));

		/* Setup EDID base address */
		hdmi_write_reg32(hdmi, HDMI_I2CM_SLAVE_OFFSET, EDID_SLAVE_ADDRESS);
		hdmi_write_reg32(hdmi, HDMI_I2CM_SEGADDR_OFFSET, EDID_SEGMENT_ADDRESS);

		/* Address offset */
		hdmi_write_reg32(hdmi, HDMI_I2CM_ADDRESS_OFFSET, hdmi->i2c->ddc_addr);

		/* Set operation to normal read */
		hdmi_write_reg32(hdmi, HDMI_I2CM_OPERATION_OFFSET,
			SET_FIELD(HDMI_I2CM_OPERATION_RD_START,
					HDMI_I2CM_OPERATION_RD_MASK, 1));

		intr_reg = hdmi_read_reg32(hdmi, HDMI_IH_I2CM_STAT0_OFFSET);
		while (!IS_BIT_SET(intr_reg, HDMI_IH_I2CM_STAT0_I2CMASTERDONE_START) &&
			   !IS_BIT_SET(intr_reg, HDMI_IH_I2CM_STAT0_I2CMASTERERROR_START)) {
			intr_reg = hdmi_read_reg32(hdmi, HDMI_IH_I2CM_STAT0_OFFSET);
			mdelay(1);
			poll += 1;
			if (poll > EDID_I2C_TIMEOUT_MS) {
				dev_err(hdmi->dev, "%s: Timeout polling on I2CMasterDoneBit (STAT0: %d)\n", __func__, intr_reg);
				return -ETIMEDOUT;
			}
		}

		if (IS_BIT_SET(intr_reg, HDMI_IH_I2CM_STAT0_I2CMASTERERROR_START)) {
			dev_err(hdmi->dev, "%s: I2C EDID read failed, Master error bit is set\n", __func__);
			return -ETIMEDOUT;
		}

		*buf++ = hdmi_read_reg32(hdmi, HDMI_I2CM_DATAI_OFFSET) & 0xFF;
		hdmi->i2c->ddc_addr++;
	}

	return 0;
}

static int hdmi_i2c_write(struct hdmi_device *hdmi, struct i2c_msg *msg)
{
	if (msg->addr == EDID_SLAVE_ADDRESS && msg->len == 0x01)
		hdmi->i2c->ddc_addr = msg->buf[0];

	return 0;

}
static int hdmi_i2c_master_xfer(struct i2c_adapter *adap,
					struct i2c_msg *msgs, int num)
{
	struct hdmi_device *hdmi = i2c_get_adapdata(adap);
	int i;

	for (i = 0; i < num; i++) {
		struct i2c_msg *msg = &msgs[i];

		hdmi_info(hdmi, "[msg: %d out of %d][len: %d][type: %s][addr: 0x%x][buf: %d]\n",
			i+1, num, msg->len,
			(msg->flags & I2C_M_RD) ? "read" : "write", msg->addr, msg->buf[0]);

		if (msg->flags & I2C_M_RD)
			hdmi_i2c_read(hdmi, msg);
		else
			hdmi_i2c_write(hdmi, msg);
	}

	return i;
}

static u32 hdmi_i2c_func(struct i2c_adapter *adapter)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm hdmi_algo = {
	.master_xfer	= hdmi_i2c_master_xfer,
	.functionality	= hdmi_i2c_func,
};

int hdmi_i2c_init(struct hdmi_device *hdmi)
{
	int ret;

	hdmi->i2c = devm_kzalloc(hdmi->dev, sizeof(*hdmi->i2c), GFP_KERNEL);
	if (!hdmi->i2c)
		return -ENOMEM;

	mutex_init(&hdmi->i2c->lock);
	init_completion(&hdmi->i2c->comp);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 8, 0))
	hdmi->i2c->adap.class = I2C_CLASS_DDC;
#endif
	hdmi->i2c->adap.owner = THIS_MODULE;
	hdmi->i2c->adap.dev.parent = hdmi->dev;
	hdmi->i2c->adap.dev.of_node = hdmi->dev->of_node;
	hdmi->i2c->adap.algo = &hdmi_algo;
	snprintf(hdmi->i2c->adap.name, sizeof(hdmi->i2c->adap.name), "Plato HDMI");
	i2c_set_adapdata(&hdmi->i2c->adap, hdmi);

	ret = i2c_add_adapter(&hdmi->i2c->adap);
	if (ret) {
		dev_err(hdmi->dev, "Failed to add %s i2c adapter\n",
							hdmi->i2c->adap.name);
		devm_kfree(hdmi->dev, hdmi->i2c);
		return ret;
	}

	hdmi->i2c->segment_ptr = 0;
	hdmi->i2c->ddc_addr = 0;

	/* Mask interrupts */
	hdmi_write_reg32(hdmi, HDMI_I2CM_INT_OFFSET,
		SET_FIELD(HDMI_I2CM_INT_DONE_MASK_START,
				HDMI_I2CM_INT_DONE_MASK_MASK, 1) |
		SET_FIELD(HDMI_I2CM_INT_READ_REQ_MASK_START,
				HDMI_I2CM_INT_READ_REQ_MASK_MASK, 1));
	hdmi_write_reg32(hdmi, HDMI_I2CM_CTLINT_OFFSET,
		SET_FIELD(HDMI_I2CM_CTLINT_ARB_MASK_START,
				HDMI_I2CM_CTLINT_ARB_MASK_START, 1) |
		SET_FIELD(HDMI_I2CM_CTLINT_NACK_MASK_START,
				HDMI_I2CM_CTLINT_NACK_MASK_START, 1));

	hdmi_write_reg32(hdmi, HDMI_I2CM_DIV_OFFSET,
		SET_FIELD(HDMI_I2CM_DIV_FAST_STD_MODE_START,
				HDMI_I2CM_DIV_FAST_STD_MODE_START, 0));

	/* Re-enable interrupts */
	hdmi_write_reg32(hdmi, HDMI_I2CM_INT_OFFSET,
		SET_FIELD(HDMI_I2CM_INT_DONE_MASK_START,
				HDMI_I2CM_INT_DONE_MASK_MASK, 0));
	hdmi_write_reg32(hdmi, HDMI_I2CM_CTLINT_OFFSET,
		SET_FIELD(HDMI_I2CM_CTLINT_ARB_MASK_START,
				HDMI_I2CM_CTLINT_ARB_MASK_START, 0) |
		SET_FIELD(HDMI_I2CM_CTLINT_NACK_MASK_START,
				HDMI_I2CM_CTLINT_NACK_MASK_START, 0));

	hdmi_info(hdmi, "registered %s I2C bus driver\n", hdmi->i2c->adap.name);

	return ret;
}

void hdmi_i2c_deinit(struct hdmi_device *hdmi)
{
	i2c_del_adapter(&hdmi->i2c->adap);
}
