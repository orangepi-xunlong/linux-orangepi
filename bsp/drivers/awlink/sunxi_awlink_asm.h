/*
 *
 * This file is provided under a dual BSD/GPL license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef __SUNXI_AWLINK_ASM_H__
#define __SUNXI_AWLINK_ASM_H__

#include <linux/can/dev.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/arm-smccc.h>

#define SUNXI_REG_MSEL_ADDR	0x0000
#define SUNXI_REG_STA_ADDR	0x0008
#define SUNXI_REG_INT_ADDR	0x000c
#define SUNXI_REG_INTEN_ADDR	0x0010

#define SUNXI_REG_RBUF_RBACK_START_ADDR	0x0180
#define SUNXI_MSEL_RESET_MODE		BIT(0)

#define SUNXI_CMD_SELF_RCV_REQ	BIT(4)
#define SUNXI_CMD_CLEAR_OR_FLAG	BIT(3)
#define SUNXI_CMD_RELEASE_RBUF	BIT(2)
#define SUNXI_CMD_TRANS_REQ	BIT(0)

#define SUNXI_STA_BIT_ERR	(0x00 << 22)
#define SUNXI_STA_FORM_ERR	(0x01 << 22)
#define SUNXI_STA_STUFF_ERR	(0x02 << 22)
#define SUNXI_STA_MASK_ERR	(0x03 << 22)
#define SUNXI_STA_ERR_DIR	BIT(21)
#define SUNXI_STA_ERR_SEG_CODE	(0x1f << 16)

#define SUNXI_STA_BUS_OFF	BIT(7)
#define SUNXI_STA_ERR_STA	BIT(6)
#define SUNXI_STA_TRANS_BUSY	BIT(5)
#define SUNXI_STA_TBUF_RDY	BIT(2)
#define SUNXI_STA_RBUF_RDY	BIT(0)

#define SUNXI_INT_BUS_ERR	BIT(7)
#define SUNXI_INT_ARB_LOST	BIT(6)
#define SUNXI_INT_ERR_PASSIVE	BIT(5)
#define SUNXI_INT_WAKEUP	BIT(4)
#define SUNXI_INT_DATA_OR	BIT(3)
#define SUNXI_INT_ERR_WRN	BIT(2)
#define SUNXI_INT_TBUF_VLD	BIT(1)
#define SUNXI_INT_RBUF_VLD	BIT(0)
#define SUNXI_MSG_RTR_FLAG	BIT(6)

#define SUNXI_AWLINK_MAX_IRQ	20
#define SUNXI_MODE_MAX_RETRIES	100

void awlink_asm_write_cmdreg(u32 mod_reg_val, volatile void __iomem *mod_reg_addr, volatile void __iomem *reg_addr);
void awlink_asm_start(volatile void __iomem *mod_reg_addr, unsigned long ctrlmode, unsigned long num);
void awlink_asm_set_bittiming(volatile void __iomem *mod_reg_addr, u32 mod_reg_val, u32 *cfg, volatile void __iomem *reg_addr);
void awlink_asm_clean_transfer_err(volatile void __iomem *addr, u16 *t_err, u16 *r_err);
void awlink_asm_rx(volatile void __iomem *addr, u8 *r_fi, u32 *dreg, u32 *id);

void awlink_asm_start_xmit(volatile void __iomem *mod_reg_addr, volatile void __iomem *addr, u32 id, u32 *flag, u32 *mod_reg_val, u8 *pdata);
int awlink_asm_probe(struct device_node *node, unsigned long num);
void awlink_asm_fun0(unsigned long num, unsigned long pin_id);
void awlink_asm_fun1(unsigned long num);
void awlink_asm_fun2(unsigned long num);
void awlink_asm_fun3(unsigned long num);
void awlink_asm_fun4(unsigned long num);
void __iomem *awlink_asm_fun5(unsigned long num);

#endif
