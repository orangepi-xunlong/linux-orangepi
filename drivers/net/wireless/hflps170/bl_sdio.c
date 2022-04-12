#include <linux/module.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>

#include <linux/firmware.h>

#include "bl_defs.h"
#include "bl_v7.h"

#include "bl_sdio.h"
#include "bl_bootrom.h"


#define SDIO_VENDOR_ID_BFL		0x424c
#define SD_DEVICE_ID_BFL		0x0606


#define BOOTROM_SDIO_PORT_USED 0x0001
#define BOOTROM_CMD_BUFFER_SIZE (1024 * 2)
#define BOOTROM_DATA_BUFFER_SIZE (512 * 2 * 2)

#define BOOTROM_SDIO_TEST_READ_BLOCK_COUNT (4 * 8)
#define BOOTROM_SDIO_TEST_READ_LOOP_COUNT (1024 * 100)
#define BOOTROM_SDIO_TEST_WRITE_BLOCK_COUNT (4 * 8)
#define BOOTROM_SDIO_TEST_WRITE_LOOP_COUNT (1024 * 100)
static int count = 0;
unsigned long time_start, time_end, time_diff, time_ms;
extern unsigned long volatile jiffies;


static const struct sdio_device_id bl_sdio_ids[] = {
    {SDIO_DEVICE(SDIO_VENDOR_ID_BFL, SD_DEVICE_ID_BFL)},
    {0,}
};
MODULE_DEVICE_TABLE(sdio, bl_sdio_ids);

void bl_get_rd_len(struct bl_hw *bl_hw, u32 reg_l, u32 reg_u, u32 *len)
{
	u8 rd_len_l = 0;
	u8 rd_len_u = 0;
	
	bl_read_reg(bl_hw, reg_l, &rd_len_l);
	bl_read_reg(bl_hw, reg_u, &rd_len_u);

	*len = (rd_len_u << 8) + rd_len_l;
}
/*
 * This function gets the read port.
 *
 * If control port bit is set in MP read bitmap, the control port
 * is returned, otherwise the current read port is returned and
 * the value is increased (provided it does not reach the maximum
 * limit, in which case it is reset to 1)
 */
int bl_get_rd_port(struct bl_hw *bl_hw, u32 *port)
{
	struct bl_device *bl_device;
	u32 rd_bitmap = bl_hw->plat->mp_rd_bitmap;
	bl_device = (struct bl_device *)(bl_hw->plat->priv);

	if (!(rd_bitmap & (CTRL_PORT_MASK | bl_device->reg->data_port_mask))) {
		return -EBUSY;
	}

	if ((bl_device->has_control_mask) && (bl_hw->plat->mp_rd_bitmap & CTRL_PORT_MASK)) {
		bl_hw->plat->mp_rd_bitmap &= (u32) (~CTRL_PORT_MASK);
		*port = CTRL_PORT;
		BL_DBG("ctrl: port=%d rd_bitmap=0x%08x -> 0x%08x\n",
		*port, rd_bitmap, bl_hw->plat->mp_rd_bitmap);
		return 0;
	}

	if (!(bl_hw->plat->mp_rd_bitmap & (1 << bl_hw->plat->curr_rd_port)))
		return -1;

	/* We are now handling the SDIO data ports */
	bl_hw->plat->mp_rd_bitmap &= (u32)(~(1 << bl_hw->plat->curr_rd_port));
	*port = bl_hw->plat->curr_rd_port;

	if (++(bl_hw->plat->curr_rd_port) == MAX_PORT_NUM)
		bl_hw->plat->curr_rd_port = bl_device->reg->start_rd_port;

	BL_DBG("data: port=%d rd_bitmap=0x%08x -> 0x%08x\n",
		*port, rd_bitmap, bl_hw->plat->mp_rd_bitmap);

	return 0;
}

/*
 * This function gets the write port for data.
 *
 * The current write port is returned if available and the value is
 * increased (provided it does not reach the maximum limit, in which
 * case it is reset to 1)
 */
int bl_get_wr_port(struct bl_hw *bl_hw, u32 *port)
{
	struct bl_device *bl_device;
	u32 wr_bitmap = bl_hw->plat->mp_wr_bitmap;
	bl_device = (struct bl_device *)(bl_hw->plat->priv);
	
	if (!(wr_bitmap & bl_device->reg->data_port_mask)) {
		printk("no available port\n");
		return -EBUSY;
	}

	if (wr_bitmap & (1 << bl_hw->plat->curr_wr_port)) {
		bl_hw->plat->mp_wr_bitmap &= (u32) (~(1 << bl_hw->plat->curr_wr_port));
		*port = bl_hw->plat->curr_wr_port;
		if (++(bl_hw->plat->curr_wr_port) == MAX_PORT_NUM)
			bl_hw->plat->curr_wr_port = bl_device->reg->start_wr_port;
	} else {
		printk("no available port\n");
		return -EBUSY;
	}

	BL_DBG("data: port=%d wr_bitmap=0x%08x -> 0x%08x\n",
		*port, wr_bitmap, bl_hw->plat->mp_wr_bitmap);

	return 0;
}

/*
 * This function writes multiple data into SDIO card memory.
 *
 * This does not work in suspended mode.
 */
int bl_write_data_sync_dnldfw(struct bl_plat *bl_hw, u8 *buffer, u32 pkt_len, u32 port)
{
	int ret;
	u8 blk_mode = (pkt_len < BL_SDIO_BLOCK_SIZE) ? BYTE_MODE : BLOCK_MODE;
	u32 blk_size = (blk_mode == BLOCK_MODE) ? BL_SDIO_BLOCK_SIZE : 1;
	u32 blk_cnt = (blk_mode == BLOCK_MODE) ? ((pkt_len + BL_SDIO_BLOCK_SIZE -1)/BL_SDIO_BLOCK_SIZE) : pkt_len;

	u32 ioport = (port & BL_SDIO_IO_PORT_MASK);

	sdio_claim_host(bl_hw->func);
	ret = sdio_writesb(bl_hw->func, ioport, buffer, blk_cnt * blk_size);
	sdio_release_host(bl_hw->func);

	return ret;
}

/*
 * This function reads multiple data from SDIO card memory.
 */
int bl_read_data_sync_dnldfw(struct bl_plat *bl_hw, u8 *buffer, u32 len, u32 port)
{
	int ret;
	u8 blk_mode = (len < BL_SDIO_BLOCK_SIZE) ? BYTE_MODE : BLOCK_MODE;	
	u32 blk_size = (blk_mode == BLOCK_MODE) ? BL_SDIO_BLOCK_SIZE : 1;
	u32 blk_cnt = (blk_mode == BLOCK_MODE) ? ((len + BL_SDIO_BLOCK_SIZE -1)/BL_SDIO_BLOCK_SIZE) : len;
	u32 ioport = (port & BL_SDIO_IO_PORT_MASK);

	sdio_claim_host(bl_hw->func);
	ret = sdio_readsb(bl_hw->func, buffer, ioport, blk_cnt * blk_size);
	sdio_release_host(bl_hw->func);

	return ret;
}


/*
 * This function reads data from SDIO card register.
 */
static int bl_sdio_read_reg(struct bl_plat *card, u32 reg, u8 *data)
{
	int ret = -1;
	u8 val;

	val = sdio_readb(card->func, reg, &ret);

	*data = val;

	return ret;
}

/*
 * This function write data to SDIO card register.
 */
static int bl_sdio_write_reg(struct bl_plat *card, u32 reg, u8 data)
{
	int ret;

	sdio_writeb(card->func, data, reg, &ret);

	return ret;
}

//TODO TIMEOUT value
static int bl_io_write(struct bl_plat *card, u8* data, int len)
{
	u8 wr_bitmap_l = 0;
	u8 wr_bitmap_u = 0;
	u32 bitmap = 0;
    int ret;

	//SDIO_DBG(SDIO_FN_ENTRY_STR);

#if 0
    data[len - 1] = 0xFF;
    data[len - 2] = 0xFE;
    data[len - 3] = 0xFD;
    data[len - 4] = 0xFC;
    data[len - 5] = 0xFB;
    data[len - 6] = 0xFA;
#endif
    count++;

    while (0 == (bitmap & (1 << card->curr_wr_port))) {
        bl_sdio_read_reg(card, WR_BITMAP_L, &wr_bitmap_l);
        bl_sdio_read_reg(card, WR_BITMAP_U, &wr_bitmap_u);
        bitmap = wr_bitmap_l;
        bitmap |= wr_bitmap_u << 8;
#if 0
        printk("bl_io_write: wr_bitmap=0x%x, count %d, curr_wr_port %d\n",
            bitmap, count, card->curr_wr_port);
#endif
        //msleep(500);
    }
    printk("bl_io_write: wr_bitmap=0x%x, count %d\n", bitmap, count);
	ret = bl_write_data_sync_dnldfw(card, data, len, card->io_port + card->curr_wr_port);
#if 0
    if ((++(card->curr_wr_port)) == 4) {
        card->curr_wr_port = 0;
    }
#endif
    //printk("%s: ret %d\n", __func__, ret);

	//SDIO_DBG(SDIO_FN_LEAVE_STR);

	return 0;
}

//caller must free the memory
int bl_io_read(struct bl_plat *card, u8 *buf, int rd_len)
{
	u8 rd_bitmap_l = 0;
	u8 rd_bitmap_u = 0;
	u32 bitmap = 0;
	
#if 0
	SDIO_DBG(SDIO_FN_ENTRY_STR);
#endif

#if 1
    while (0 == (bitmap & (1 << card->curr_rd_port))) {
		bl_sdio_read_reg(card, RD_BITMAP_L, &rd_bitmap_l);
		bl_sdio_read_reg(card, RD_BITMAP_U, &rd_bitmap_u);
		bitmap = rd_bitmap_l;
		bitmap |= rd_bitmap_u << 8;
		card->mp_rd_bitmap = bitmap;
#if 0
		printk("int: UPLD: rd_bitmap=0x%x\n", card->mp_rd_bitmap);
#endif
        //msleep(500);
    }
#endif

#if 1
	bl_read_data_sync_dnldfw(card, buf, rd_len, card->io_port + card->curr_rd_port);
#endif
#if 1
	printk("[XXX] len:%d--->receive ok\n", rd_len);
    printk("Read 00: %02X %02X %02X %02X %02X %02X %02X %02X\n"
           "Read 10: %02X %02X %02X %02X %02X %02X %02X %02X\n"
           "Read 20: %02X %02X %02X %02X %02X %02X %02X %02X\n",
        buf[0], buf[1],
        buf[2], buf[3],
        buf[4], buf[5],
        buf[6], buf[7],
        buf[8 + 0], buf[8 + 1],
        buf[8 + 2], buf[8 + 3],
        buf[8 + 4], buf[8 + 5],
        buf[8 + 6], buf[8 + 7],
        buf[16 + 0], buf[16 + 1],
        buf[16 + 2], buf[16 + 3],
        buf[16 + 4], buf[16 + 5],
        buf[16 + 6], buf[16 + 7]
    );
#endif
#if 0
    if ((++(card->curr_rd_port)) == 4) {
        card->curr_rd_port = 0;
    }
#endif
#if 0
	SDIO_DBG(SDIO_FN_LEAVE_STR);
#endif

	return 0;
}

static int bl_sdio_run_fw(struct bl_plat *card)
{
	int ret;

	printk("Enter bl_sdio_run_fw...\n");
	ret = bl_sdio_write_reg(card, CARD_FW_STATUS0_REG, 0x1);

	return ret;

}

static int firmware_dump_head(bootheader_t *header)
{
    int i;

    printk("[************Header************]\n");
    printk(" magiccode: 0x%X\n", header->magiccode);
    printk(" rivison: 0x%X\n", header->rivison);
    printk(" flashCfg\n");
    printk("    magiccode: 0x%X\n", header->flashCfg.magiccode);
    printk("    cfg see below, size: %lu\n", sizeof(header->flashCfg.cfg));
    printk("    crc32: 0x%X\n", header->flashCfg.crc32);
    printk(" pllCfg\n");
    printk("    magiccode: 0x%X\n", header->pllCfg.magiccode);
    printk("    root_clk: %u\n", header->pllCfg.root_clk);
    printk("    xtal_type: %u\n", header->pllCfg.xtal_type);
    printk("    pll_clk: %u\n", header->pllCfg.pll_clk);
    printk("    hclk_div: %u\n", header->pllCfg.hclk_div);
    printk("    bclk_div: %u\n", header->pllCfg.bclk_div);
    printk("    flash_clk_div: %u\n", header->pllCfg.flash_clk_div);
    printk("    uart_clk_div: %u\n", header->pllCfg.uart_clk_div);
    printk("    sdu_clk_div: %u\n", header->pllCfg.sdu_clk_div);
    printk("    crc32: 0x%X\n", header->pllCfg.crc32);
    printk(" seccfg\n");
    printk("    bval in union\n");
    printk("        encrypt: %u\n", header->seccfg.bval.encrypt);
    printk("        sign: %u\n", header->seccfg.bval.sign);
    printk("        key_sel: %u\n", header->seccfg.bval.key_sel);
    printk("        rsvd6_31: %u\n", header->seccfg.bval.rsvd6_31);
    printk("    wval in union\n");
    printk("        wval: 0x%X\n", header->seccfg.wval);
    printk(" segment_cnt: %u\n", header->segment_cnt);
    printk(" bootentry: 0x%X\n", header->bootentry);
    printk(" flashoffset: 0x%X\n", header->flashoffset);
    printk(" bootentry: 0x%X\n", header->bootentry);
    printk(" hash:\n");

    for (i = 0; i < sizeof(header->hash); i+=4) {
        //XXX we may access over-flow
        printk("0x%08X ", ((uint32_t)header->hash[i]) << 24 |
            (uint32_t)header->hash[i + 1] << 16 |
            (uint32_t)header->hash[i + 2] << 8 |
            (uint32_t)header->hash[i + 3] << 0
        ); 
    }


    printk(" rsv1: %u\n", header->rsv1);
    printk(" rsv2: %u\n", header->rsv2);
    printk(" crc32: 0x%X\n", header->crc32);
    printk("--------\n");

    return 0;
}

static int firmware_dump_flashcfg(const boot_flash_cfg_t *cfg)
{
    printk("[************FLASH CFG(%lu)************]\n", sizeof(boot_flash_cfg_t));
	printk(" magiccode: 0x%X\n", cfg->magiccode);
	printk(" flash cfg\n");
    printk("   ioMode: %u\n", cfg->cfg.ioMode);
	printk("   cReadSupport: %u\n", cfg->cfg.cReadSupport);
	printk("   clk_delay;: %u\n", cfg->cfg.clk_delay);
	printk("   rsvd[1]: %X\n", cfg->cfg.rsvd[0]);
    printk("   resetEnCmd: %u\n", cfg->cfg.resetEnCmd);
	printk("   resetCmd: %u\n", cfg->cfg.resetCmd);
	printk("   resetCreadCmd: %u\n", cfg->cfg.resetCreadCmd);
	printk("   rsvd_reset[1]: %X\n", cfg->cfg.rsvd_reset[0]);
	printk("   jedecIdCmd: %u(0x%X)\n", cfg->cfg.jedecIdCmd, cfg->cfg.jedecIdCmd);
	printk("   jedecIdCmdDmyClk: %u(0x%X)\n", cfg->cfg.jedecIdCmdDmyClk, cfg->cfg.jedecIdCmdDmyClk);
	printk("   qpiJedecIdCmd: %u(0x%X)\n", cfg->cfg.qpiJedecIdCmd, cfg->cfg.qpiJedecIdCmd);
	printk("   qpiJedecIdCmdDmyClk: %u(0x%X)\n", cfg->cfg.qpiJedecIdCmdDmyClk, cfg->cfg.qpiJedecIdCmdDmyClk);
	printk("   sectorSize: %u(0x%X)\n", cfg->cfg.sectorSize, cfg->cfg.sectorSize);
	printk("   capBase: %u(0x%X)\n", cfg->cfg.capBase, cfg->cfg.capBase);
	printk("   pageSize: %u(0x%X)\n", cfg->cfg.pageSize, cfg->cfg.pageSize);
	printk("   chipEraseCmd: %u(0x%X)\n", cfg->cfg.chipEraseCmd, cfg->cfg.chipEraseCmd);
	printk("   sectorEraseCmd: %u(0x%X)\n", cfg->cfg.sectorEraseCmd, cfg->cfg.sectorEraseCmd);
	printk("   blk32EraseCmd: %u(0x%X)\n", cfg->cfg.blk32EraseCmd, cfg->cfg.blk32EraseCmd);
	printk("   blk64EraseCmd: %u(0x%X)\n", cfg->cfg.blk64EraseCmd, cfg->cfg.blk64EraseCmd);
    printk("   writeEnableCmd: %u(0x%X)\n", cfg->cfg.writeEnableCmd, cfg->cfg.writeEnableCmd);
	printk("   pageProgramCmd: %u(0x%X)\n", cfg->cfg.pageProgramCmd, cfg->cfg.pageProgramCmd);
	printk("   qpageProgramCmd: %u(0x%X)\n", cfg->cfg.qpageProgramCmd, cfg->cfg.qpageProgramCmd);
	printk("   qppAddrMode: %u(0x%X)\n", cfg->cfg.qppAddrMode, cfg->cfg.qppAddrMode);
	printk("   fastReadCmd: %u(0x%X)\n", cfg->cfg.fastReadCmd, cfg->cfg.fastReadCmd);
	printk("   frDmyClk: %u(0x%X)\n", cfg->cfg.frDmyClk, cfg->cfg.frDmyClk);
	printk("   qpiFastReadCmd: %u(0x%X)\n", cfg->cfg.qpiFastReadCmd, cfg->cfg.qpiFastReadCmd);
	printk("   qpiFrDmyClk: %u(0x%X)\n", cfg->cfg.qpiFrDmyClk, cfg->cfg.qpiFrDmyClk);
    printk("   fastReadDoCmd: %u(0x%X)\n", cfg->cfg.fastReadDoCmd, cfg->cfg.fastReadDoCmd);
	printk("   frDoDmyClk: %u(0x%X)\n", cfg->cfg.frDoDmyClk, cfg->cfg.frDoDmyClk);
	printk("   fastReadDioCmd: %u(0x%X)\n", cfg->cfg.fastReadDioCmd, cfg->cfg.fastReadDioCmd);
	printk("   frDioDmyClk: %u(0x%X)\n", cfg->cfg.frDioDmyClk, cfg->cfg.frDioDmyClk);
    printk("   fastReadQoCmd: %u(0x%X)\n", cfg->cfg.fastReadQoCmd, cfg->cfg.fastReadQoCmd);
	printk("   frQoDmyClk: %u(0x%X)\n", cfg->cfg.frQoDmyClk, cfg->cfg.frQoDmyClk);
    printk("   fastReadQioCmd: %u(0x%X)\n", cfg->cfg.fastReadQioCmd, cfg->cfg.fastReadQioCmd);
	printk("   frQioDmyClk: %u(0x%X)\n", cfg->cfg.frQioDmyClk, cfg->cfg.frQioDmyClk);
    printk("   qpiFastReadQioCmd: %u(0x%X)\n", cfg->cfg.qpiFastReadQioCmd, cfg->cfg.qpiFastReadQioCmd);
    printk("   qpiFrQioDmyClk: %u(0x%X)\n", cfg->cfg.qpiFrQioDmyClk, cfg->cfg.qpiFrQioDmyClk);
	printk("   qpiPageProgramCmd: %u(0x%X)\n", cfg->cfg.qpiPageProgramCmd, cfg->cfg.qpiPageProgramCmd);
	printk("   writeVregEnableCmd: %u(0x%X)\n", cfg->cfg.writeVregEnableCmd, cfg->cfg.writeVregEnableCmd);
	printk("   wrEnableIndex: %u(0x%X)\n", cfg->cfg.wrEnableIndex, cfg->cfg.wrEnableIndex);
	printk("   qeIndex: %u(0x%X)\n", cfg->cfg.qeIndex, cfg->cfg.qeIndex);
	printk("   busyIndex: %u(0x%X)\n", cfg->cfg.busyIndex, cfg->cfg.busyIndex);
	printk("   wrEnableBit: %u(0x%X)\n", cfg->cfg.wrEnableBit, cfg->cfg.wrEnableBit);
	printk("   qeBit: %u(0x%X)\n", cfg->cfg.qeBit, cfg->cfg.qeBit);
    printk("   busyBit: %u(0x%X)\n", cfg->cfg.busyBit, cfg->cfg.busyBit);
	printk("   wrEnableWriteRegLen: %u(0x%X)\n", cfg->cfg.wrEnableWriteRegLen, cfg->cfg.wrEnableWriteRegLen);
	printk("   wrEnableReadRegLen: %u(0x%X)\n", cfg->cfg.wrEnableReadRegLen, cfg->cfg.wrEnableReadRegLen);
	printk("   qeWriteRegLen: %u(0x%X)\n", cfg->cfg.qeWriteRegLen, cfg->cfg.qeWriteRegLen);
	printk("   qeReadRegLen: %u(0x%X)\n", cfg->cfg.qeReadRegLen, cfg->cfg.qeReadRegLen);
	printk("   rsvd1: %u(0x%X)\n", cfg->cfg.rsvd1, cfg->cfg.rsvd1);
	printk("   busyReadRegLen: %u(0x%X)\n", cfg->cfg.busyReadRegLen, cfg->cfg.busyReadRegLen);
	printk("   readRegCmd[4]: %u(0x%X), %u(0x%X), %u(0x%X), %u(0x%X)\n",
            cfg->cfg.readRegCmd[0], cfg->cfg.readRegCmd[0],
            cfg->cfg.readRegCmd[1], cfg->cfg.readRegCmd[1],
            cfg->cfg.readRegCmd[2], cfg->cfg.readRegCmd[2],
            cfg->cfg.readRegCmd[3], cfg->cfg.readRegCmd[3]
    );
	printk("   writeRegCmd[4]: %u(0x%X), %u(0x%X), %u(0x%X), %u(0x%X)\n",
        cfg->cfg.writeRegCmd[0], cfg->cfg.writeRegCmd[0],
        cfg->cfg.writeRegCmd[1], cfg->cfg.writeRegCmd[1],
        cfg->cfg.writeRegCmd[2], cfg->cfg.writeRegCmd[2],
        cfg->cfg.writeRegCmd[3], cfg->cfg.writeRegCmd[3]
    );
    printk("   enterQpi: %u(0x%X)\n", cfg->cfg.enterQpi, cfg->cfg.enterQpi);
	printk("   exitQpi: %u(0x%X)\n", cfg->cfg.exitQpi, cfg->cfg.exitQpi);
	printk("   cReadMode: %u(0x%X)\n", cfg->cfg.cReadMode, cfg->cfg.cReadMode);
    printk("   cRExit: %u(0x%X)\n", cfg->cfg.cRExit, cfg->cfg.cRExit);
    printk("   burstWrapCmd: %u(0x%X)\n", cfg->cfg.burstWrapCmd, cfg->cfg.burstWrapCmd);
    printk("   burstWrapCmdDmyClk: %u(0x%X)\n", cfg->cfg.burstWrapCmdDmyClk, cfg->cfg.burstWrapCmdDmyClk);
    printk("   burstWrapDataMode: %u(0x%X)\n", cfg->cfg.burstWrapDataMode, cfg->cfg.burstWrapDataMode);
    printk("   burstWrapData: %u(0x%X)\n", cfg->cfg.burstWrapData, cfg->cfg.burstWrapData);
    printk("   deBurstWrapCmd: %u(0x%X)\n", cfg->cfg.deBurstWrapCmd, cfg->cfg.deBurstWrapCmd);
    printk("   deBurstWrapCmdDmyClk: %u(0x%X)\n", cfg->cfg.deBurstWrapCmdDmyClk, cfg->cfg.deBurstWrapCmdDmyClk);
    printk("   deBurstWrapDataMode: %u(0x%X)\n", cfg->cfg.deBurstWrapDataMode, cfg->cfg.deBurstWrapDataMode);
    printk("   deBurstWrapData: %u(0x%X)\n", cfg->cfg.deBurstWrapData, cfg->cfg.deBurstWrapData);
    printk("   timeEsector: %u(0x%X)\n", cfg->cfg.timeEsector, cfg->cfg.timeEsector);
	printk("   timeE32k: %u(0x%X)\n", cfg->cfg.timeE32k, cfg->cfg.timeE32k);
	printk("   timeE64k: %u(0x%X)\n", cfg->cfg.timeE64k, cfg->cfg.timeE64k);
	printk("   timePagePgm: %u(0x%X)\n", cfg->cfg.timePagePgm, cfg->cfg.timePagePgm);
	printk("   timeCe: %u(0x%X)\n", cfg->cfg.timeCe, cfg->cfg.timeCe);
    printk(" CRC32(%lu): %02X%02X%02X%02X\n",
        sizeof(cfg->crc32),
        (cfg->crc32 >> 24) & 0xFF,
        (cfg->crc32 >> 16) & 0xFF,
        (cfg->crc32 >> 8) & 0xFF,
        (cfg->crc32 >> 0) & 0xFF
    );
    printk("--------\n");

    return 0;
}

static int firmware_dump_pllcfg(const boot_pll_cfg_t *cfg)
{
    printk("[************PLL CFG(%lu)************]\n", sizeof(boot_pll_cfg_t));
	printk(" magiccode: 0x%08X\n", cfg->magiccode);
	printk(" root_clk: %u(0x%X)\n", cfg->root_clk, cfg->root_clk);
	printk(" xtal_type: %u(0x%X)\n", cfg->xtal_type, cfg->xtal_type);
	printk(" pll_clk: %u(0x%X)\n", cfg->pll_clk, cfg->pll_clk);
	printk(" hclk_div: %u(0x%X)\n", cfg->hclk_div, cfg->hclk_div);
	printk(" bclk_div: %u(0x%X)\n", cfg->bclk_div, cfg->bclk_div);
	printk(" flash_clk_div: %u(0x%X)\n", cfg->flash_clk_div, cfg->flash_clk_div);
	printk(" uart_clk_div: %u(0x%X)\n", cfg->uart_clk_div, cfg->uart_clk_div);
	printk(" sdu_clk_div: %u(0x%X)\n", cfg->sdu_clk_div, cfg->sdu_clk_div);
    printk(" CRC32(%lu): %02X%02X%02X%02X\n",
        sizeof(cfg->crc32),
        (cfg->crc32 >> 24) & 0xFF,
        (cfg->crc32 >> 16) & 0xFF,
        (cfg->crc32 >> 8) & 0xFF,
        (cfg->crc32 >> 0) & 0xFF
    );
    printk("--------\n");

    return 0;
}

static int firmware_dump_encryptiondata(const u8 *encryption)
{
    //FIXME NOT use magic number
    printk("[************ENCRYPTION DATA (20)************]\n");
	printk(" IV(16): %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X\n",
        encryption[0],
        encryption[1],
        encryption[2],
        encryption[3],
        encryption[4],
        encryption[5],
        encryption[6],
        encryption[7],
        encryption[8],
        encryption[9],
        encryption[10],
        encryption[11],
        encryption[12],
        encryption[13],
        encryption[14],
        encryption[15]
    );
	printk(" CRC32(4): %02X%02X%02X%02X\n",
        encryption[19],
        encryption[18],
        encryption[17],
        encryption[16]
    );
    printk("--------\n");

    return 0;
}

static int firmware_dump_publickey(const pkey_cfg_t *key)
{
    printk("[************PUBLIC KEY DATA(%lu)************]\n", sizeof(pkey_cfg_t));
    printk(" public key X(%lu):\n", sizeof(key->eckeyx));
    printk("   %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X\n",
        key->eckeyx[0],
        key->eckeyx[1],
        key->eckeyx[2],
        key->eckeyx[3],
        key->eckeyx[4],
        key->eckeyx[5],
        key->eckeyx[6],
        key->eckeyx[7],
        key->eckeyx[8],
        key->eckeyx[9],
        key->eckeyx[10],
        key->eckeyx[11],
        key->eckeyx[12],
        key->eckeyx[13],
        key->eckeyx[14],
        key->eckeyx[15],
        key->eckeyx[16],
        key->eckeyx[17],
        key->eckeyx[18],
        key->eckeyx[19],
        key->eckeyx[20],
        key->eckeyx[21],
        key->eckeyx[22],
        key->eckeyx[23],
        key->eckeyx[24],
        key->eckeyx[25],
        key->eckeyx[26],
        key->eckeyx[27],
        key->eckeyx[28],
        key->eckeyx[29],
        key->eckeyx[30],
        key->eckeyx[31]
    );
    printk(" public key Y(%lu):\n", sizeof(key->eckeyy));
    printk("   %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X\n",
        key->eckeyy[0],
        key->eckeyy[1],
        key->eckeyy[2],
        key->eckeyy[3],
        key->eckeyy[4],
        key->eckeyy[5],
        key->eckeyy[6],
        key->eckeyy[7],
        key->eckeyy[8],
        key->eckeyy[9],
        key->eckeyy[10],
        key->eckeyy[11],
        key->eckeyy[12],
        key->eckeyy[13],
        key->eckeyy[14],
        key->eckeyy[15],
        key->eckeyy[16],
        key->eckeyy[17],
        key->eckeyy[18],
        key->eckeyy[19],
        key->eckeyy[20],
        key->eckeyy[21],
        key->eckeyy[22],
        key->eckeyy[23],
        key->eckeyy[24],
        key->eckeyy[25],
        key->eckeyy[26],
        key->eckeyy[27],
        key->eckeyy[28],
        key->eckeyy[29],
        key->eckeyy[30],
        key->eckeyy[31]
    );
    printk(" CRC32(%lu): %02X%02X%02X%02X\n",
        sizeof(key->crc32),
        (key->crc32 >> 24) & 0xFF,
        (key->crc32 >> 16) & 0xFF,
        (key->crc32 >> 8) & 0xFF,
        (key->crc32 >> 0) & 0xFF
    );
    printk("--------\n");

    return 0;
}

static int firmware_dump_signdata(const sign_cfg_t *sign)
{
    int i;

    printk("[************SIGNATURE DATA(%lu)************]\n", sizeof(sign_cfg_t) + sign->sig_len);
    printk(" sig_len: %u(0x%X)\n", sign->sig_len, sign->sig_len);
    printk(" signature(%u):\n", sign->sig_len);

    for (i = 0; i < sign->sig_len; i+= 32) {
        printk("   %03u: %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X\n",
            i,
            sign->signature[i + 0],
            sign->signature[i + 1],
            sign->signature[i + 2],
            sign->signature[i + 3],
            sign->signature[i + 4],
            sign->signature[i + 5],
            sign->signature[i + 6],
            sign->signature[i + 7],
            sign->signature[i + 8],
            sign->signature[i + 9],
            sign->signature[i + 10],
            sign->signature[i + 11],
            sign->signature[i + 12],
            sign->signature[i + 13],
            sign->signature[i + 14],
            sign->signature[i + 15],
            sign->signature[i + 16],
            sign->signature[i + 17],
            sign->signature[i + 18],
            sign->signature[i + 19],
            sign->signature[i + 20],
            sign->signature[i + 21],
            sign->signature[i + 22],
            sign->signature[i + 23],
            sign->signature[i + 24],
            sign->signature[i + 25],
            sign->signature[i + 26],
            sign->signature[i + 27],
            sign->signature[i + 28],
            sign->signature[i + 29],
            sign->signature[i + 30],
            sign->signature[i + 31]
        );
    }
    printk(" CRC32(%lu): %02X%02X%02X%02X\n",
        sizeof(sign->crc32),
        sign->signature[sign->sig_len],
        sign->signature[sign->sig_len + 1],
        sign->signature[sign->sig_len + 2],
        sign->signature[sign->sig_len + 3]
    );
    printk("--------\n");
    return 0;
}

static int firmware_dump_segment_piece(const segment_header_t *segment)
{
    printk("[************SEGMENT DUMP(%lu + %u = %lu)************]\n",
        sizeof(segment_header_t),
        segment->len,
        sizeof(segment_header_t) + segment->len
    );
    printk(" destaddr: %u(0x%X)\n", segment->destaddr, segment->destaddr);
    printk(" len: %u(0x%X)\n", segment->len, segment->len);
    printk(" rsv: %u(0x%X)\n", segment->rsv, segment->rsv);
    printk(" crc32: %u(0x%X)\n", segment->crc32, segment->crc32);
    printk("--------\n");
    return 0;
}

static int firmware_download_head(struct bl_plat *card, bootrom_host_cmd_t *cmd, u8* response,
    bootheader_t *header, const u8 **data, int *len)
{
    int ret;

	SDIO_DBG(SDIO_FN_ENTRY_STR);
    printk("FUNC header: data ptr %p, data left %d\n", *data, *len);
    if (*len < sizeof(bootheader_t)) {
        printk("%s:len too small %lu:%d\n", __func__, sizeof(bootheader_t), *len);
        return -1;
    }

    firmware_dump_head(header);

    memset(cmd, 0, BOOTROM_CMD_BUFFER_SIZE);
    bl_bootrom_cmd_bootheader_load(cmd, header);
    printk("CMD bootheader len %d\n", cmd->len);
    ret = bl_io_write(card, (u8*)cmd, BOOTROM_CMD_BUFFER_SIZE);
    if (ret) {
        printk("ERR when write\n");
        return -1;
    }
    ret = bl_io_read(card, response, BOOTROM_CMD_BUFFER_SIZE);
    if (ret) {
        printk("NULL response get");
        return -1;
    }
    bl_bootrom_cmd_bootheader_load_get_res((bootrom_res_bootheader_load_t*)response);

    *data += sizeof(bootheader_t);
    *len -= sizeof(bootheader_t);

	SDIO_DBG(SDIO_FN_LEAVE_STR);

    return 0;
}

static int firmware_download_flashcfg(struct bl_plat *card, bootrom_host_cmd_t *cmd, u8 *response, bootheader_t *header)
{
    firmware_dump_flashcfg(&(header->flashCfg));

    return 0;
}

static int firmware_download_pllcfg(struct bl_plat *card, bootrom_host_cmd_t *cmd, u8* response,
    bootheader_t *header)
{
    firmware_dump_pllcfg(&(header->pllCfg));

    return 0;
}

static int firmware_download_encryptiondata(struct bl_plat *card, bootrom_host_cmd_t *cmd, u8 *response,
    bootheader_t *header, const u8 **data, int *len)
{
    int ret;

	SDIO_DBG(SDIO_FN_ENTRY_STR);

    printk("FUNC encryptiondata: data ptr %p, data left %d\n", *data, *len);
    if (0 == header->seccfg.bval.encrypt) {
        printk("no encrypt field detected\n");
        return 0;
    }

    printk("%s, Offset 0x%X\n", __func__, (uint32_t)(((u8*)*data) - ((u8*)header)));
    //FIXME NOT use magic number
    if (*len < 20) {
        printk("%s:len too small %d:%d\n", __func__, 20, *len);
        return -1;
    }

    firmware_dump_encryptiondata(*data);

    memset(cmd, 0, BOOTROM_CMD_BUFFER_SIZE);
    bl_bootrom_cmd_aesiv_load(cmd, *data);
    printk("CMD encryptiondata len %d\n", cmd->len);
    ret = bl_io_write(card, (u8*)cmd, BOOTROM_CMD_BUFFER_SIZE);
    if (ret) {
        printk("ERR when write\n");
        return -1;
    }
    ret = bl_io_read(card, response, BOOTROM_CMD_BUFFER_SIZE);
    if (ret) {
        printk("NULL response get");
        return -1;
    }
    bl_bootrom_cmd_aesiv_load_get_res((bootrom_res_aesiv_load_t*)response);

    //FIXME NOT use magic number
    *data += 20;
    *len -= 20;

	SDIO_DBG(SDIO_FN_LEAVE_STR);

    return 0;
}

static int firmware_download_publickey1(struct bl_plat *card, bootrom_host_cmd_t *cmd, u8 *response,
    bootheader_t *header, const u8 **data, int *len)
{
    int ret;
    pkey_cfg_t *key;

    printk("FUNC publickey1: data ptr %p, data left %d\n", *data, *len);
    if (0 == header->seccfg.bval.sign) {
        printk("no sign field detected\n");
        return 0;
    }

    printk("%s, Offset 0x%X\n", __func__, (uint32_t)(((u8*)*data) - ((u8*)header)));
    key = (pkey_cfg_t*)(*data);
    if (*len < sizeof(pkey_cfg_t)) {
        printk("%s:len too small %lu:%d\n", __func__, sizeof(pkey_cfg_t), *len);
        return -1;
    }
    firmware_dump_publickey(key);

    memset(cmd, 0, BOOTROM_CMD_BUFFER_SIZE);
    bl_bootrom_cmd_pkey1_load(cmd, key);
    printk("CMD public key1 len %d\n", cmd->len);
    ret = bl_io_write(card, (u8*)cmd, BOOTROM_CMD_BUFFER_SIZE);
    if (ret) {
        printk("ERR when write\n");
        return -1;
    }
    ret = bl_io_read(card, response, BOOTROM_CMD_BUFFER_SIZE);
    if (ret) {
        printk("NULL response get");
        return -1;
    }
    ret = bl_bootrom_cmd_pkey1_load_get_res((bootrom_res_pkey_load_t*)response);
    if (ret) {
        printk("Error response get pkey1\n");
        return -1;
    }

    *data += sizeof(pkey_cfg_t);
    *len -= sizeof(pkey_cfg_t);

    return 0;
}

static int firmware_download_publickey2(struct bl_plat *card, bootrom_host_cmd_t *cmd, u8 *response,
    bootheader_t *header, const u8 **data, int *len)
{
    int ret;
    pkey_cfg_t *key;

    printk("FUNC publickey2: data ptr %p, data left %d\n", *data, *len);
    if (0 == header->seccfg.bval.sign) {
        printk("no sign field detected\n");
        return 0;
    }

    printk("%s, Offset 0x%X\n", __func__, (uint32_t)(((u8*)*data) - ((u8*)header)));
    key = (pkey_cfg_t*)(*data);
    if (*len < sizeof(pkey_cfg_t)) {
        printk("%s:len too small %lu:%d\n", __func__, sizeof(pkey_cfg_t), *len);
        return -1;
    }
    firmware_dump_publickey(key);

    memset(cmd, 0, BOOTROM_CMD_BUFFER_SIZE);
    bl_bootrom_cmd_pkey2_load(cmd, key);
    printk("CMD public key2 len %d\n", cmd->len);
    ret = bl_io_write(card, (u8*)cmd, BOOTROM_CMD_BUFFER_SIZE);
    if (ret) {
        printk("ERR when write\n");
        return -1;
    }
    ret = bl_io_read(card, response, BOOTROM_CMD_BUFFER_SIZE);
    if (ret) {
        printk("NULL response get");
        return -1;
    }
    ret = bl_bootrom_cmd_pkey2_load_get_res((bootrom_res_pkey_load_t*)response);
    if (ret) {
        printk("Error response get pkey2\n");
        return -1;
    }

    *data += sizeof(pkey_cfg_t);
    *len -= sizeof(pkey_cfg_t);

    return 0;
}

static int firmware_download_signdata1(struct bl_plat *card, bootrom_host_cmd_t *cmd, u8 *response,
    bootheader_t *header, const u8 **data, int *len)
{
    int ret;
    sign_cfg_t *sign;

    printk("FUNC signdata1: data ptr %p, data left %d\n", *data, *len);
    if (0 == header->seccfg.bval.sign) {
        printk("no sign field detected\n");
        return 0;
    }

    printk("%s, Offset 0x%X\n", __func__, (uint32_t)(((u8*)*data) - ((u8*)header)));
    sign = (sign_cfg_t*)(*data);
    if (*len < sizeof(sign_cfg_t) + sign->sig_len) {
        printk("%s:len too small %lu:%d\n", __func__, sizeof(sign_cfg_t), *len);
        return -1;
    }

    firmware_dump_signdata(sign);

    memset(cmd, 0, BOOTROM_CMD_BUFFER_SIZE);
    bl_bootrom_cmd_signature1_load(cmd, (uint8_t*)sign, sizeof(sign_cfg_t) + sign->sig_len);
    printk("CMD signa data1 len %d\n", cmd->len);
    ret = bl_io_write(card, (u8*)cmd, BOOTROM_CMD_BUFFER_SIZE);
    if (ret) {
        printk("ERR when write\n");
        return -1;
    }
    ret = bl_io_read(card, response, BOOTROM_CMD_BUFFER_SIZE);
    if (ret) {
        printk("NULL response get");
        return -1;
    }
    ret = bl_bootrom_cmd_signature1_get_res((bootrom_res_signature_load_t*)response);
    if (ret) {
        printk("Error response get signature1\n");
        return -1;
    }

    *data += (sizeof(sign_cfg_t) + sign->sig_len);
    *len -= (sizeof(sign_cfg_t) + sign->sig_len);

    return 0;
}

static int firmware_download_signdata2(struct bl_plat *card, bootrom_host_cmd_t *cmd, u8 *response,
    bootheader_t *header, const u8 **data, int *len)
{
    int ret;
    sign_cfg_t *sign;

    printk("FUNC signdata2: data ptr %p, data left %d\n", *data, *len);
    if (0 == header->seccfg.bval.sign) {
        printk("no sign field detected\n");
        return 0;
    }

    printk("%s, Offset 0x%X\n", __func__, (uint32_t)(((u8*)*data) - ((u8*)header)));
    sign = (sign_cfg_t*)(*data);
    if (*len < sizeof(sign_cfg_t) + sign->sig_len) {
        printk("%s:len too small %lu:%d\n", __func__, sizeof(sign_cfg_t), *len);
        return -1;
    }

    firmware_dump_signdata(sign);

    memset(cmd, 0, BOOTROM_CMD_BUFFER_SIZE);
    bl_bootrom_cmd_signature2_load(cmd, (uint8_t*)sign, sizeof(sign_cfg_t) + sign->sig_len);
    printk("CMD signa data2 len %d\n", cmd->len);
    ret = bl_io_write(card, (u8*)cmd, BOOTROM_CMD_BUFFER_SIZE);
    if (ret) {
        printk("ERR when write\n");
        return -1;
    }
    ret = bl_io_read(card, response, BOOTROM_CMD_BUFFER_SIZE);
    if (ret) {
        printk("NULL response get");
        return -1;
    }
    ret = bl_bootrom_cmd_signature2_get_res((bootrom_res_signature_load_t*)response);
    if (ret) {
        printk("Error response get signature2\n");
        return -1;
    }

    *data += (sizeof(sign_cfg_t) + sign->sig_len);
    *len -= (sizeof(sign_cfg_t) + sign->sig_len);

    return 0;
}

static int firmware_download_segment_piece(struct bl_plat *card, bootrom_host_cmd_t *cmd, u8 *response,
    bootheader_t *header, const u8 **data, int *len)
{
    const segment_header_t *segment;
    segment_header_t segment_plain;
    int ret = 0, i, pos, wr_len;

    SDIO_DBG(SDIO_FN_ENTRY_STR);

    printk("FUNC segment_piece: data ptr %p, data left %d\n", *data, *len);
    segment = (const segment_header_t*)(*data);
    memset(cmd, 0, BOOTROM_CMD_BUFFER_SIZE);
    bl_bootrom_cmd_sectionheader_load(cmd, segment);
    printk("CMD segment piece len %d\n", cmd->len);
    ret = bl_io_write(card, (u8*)cmd, BOOTROM_CMD_BUFFER_SIZE);
    if (ret) {
        printk("ERR when write\n");
        return -1;
    }
    ret = bl_io_read(card, response, BOOTROM_CMD_BUFFER_SIZE);
    if (ret) {
        printk("NULL response get\n");
        return -1;
    }
    ret = bl_bootrom_cmd_sectionheader_get_res((bootrom_res_sectionheader_load_t*)response);
    if (ret) {
        printk("response Error\n");
        return -1;
    }
    memcpy(&segment_plain,
        &(((bootrom_res_sectionheader_load_t*)response)->header), sizeof(segment_plain));
    segment = &segment_plain;
    printk("Get SEGMENT From BL606: destaddr: 0x%08x, len %d, rsv %08X, CRC32 %08X\n",
        segment->destaddr,
        segment->len,
        segment->rsv,
        segment->crc32
    );
    if (*len < (sizeof(segment_header_t) + segment->len)) {
        printk("SEGMENT: buffer too small %lu(%lX required):%d(%X actual) to 0x%x\n",
            sizeof(segment_header_t) + segment->len,
            sizeof(segment_header_t) + segment->len,
            *len,
            *len,
            segment->destaddr
        );
        return -1;
    }
    firmware_dump_segment_piece(segment);

    pos = 0;
    i = 0;
    while (pos < segment->len) {
#define WR_LEN ((BOOTROM_DATA_BUFFER_SIZE - sizeof(bootrom_host_cmd_t)) & 0xFFFFFFF0)
        wr_len = segment->len - pos;
        wr_len = (wr_len < WR_LEN) ?  wr_len : WR_LEN;
        printk("------piece %03d[%03d] %d:%u------\n", i++, wr_len, pos, segment->len);
#if 1
        /*data len is less than one packet*/
        printk("data SRC[0-7]: %02X %02X %02X %02X %02X %02X %02X %02X\n",
            (*data + sizeof(segment_header_t) + pos)[0],
            (*data + sizeof(segment_header_t) + pos)[1],
            (*data + sizeof(segment_header_t) + pos)[2],
            (*data + sizeof(segment_header_t) + pos)[3],
            (*data + sizeof(segment_header_t) + pos)[4],
            (*data + sizeof(segment_header_t) + pos)[5],
            (*data + sizeof(segment_header_t) + pos)[6],
            (*data + sizeof(segment_header_t) + pos)[7]
        );
#endif
        bl_bootrom_cmd_sectiondata_load(cmd, *data + sizeof(segment_header_t) + pos, wr_len);
#if 1
        printk("data[%u] CMD[0-7]: %02X %02X %02X %02X %02X %02X %02X %02X\n"
               "     TAL[0-7]: %02X %02X %02X %02X %02X %02X %02X %02X\n",
            cmd->len,
            cmd->data[0],
            cmd->data[1],
            cmd->data[2],
            cmd->data[3],
            cmd->data[4],
            cmd->data[5],
            cmd->data[6],
            cmd->data[7],
            cmd->data[cmd->len - 1 - 7],
            cmd->data[cmd->len - 1 - 6],
            cmd->data[cmd->len - 1 - 5],
            cmd->data[cmd->len - 1 - 4],
            cmd->data[cmd->len - 1 - 3],
            cmd->data[cmd->len - 1 - 2],
            cmd->data[cmd->len - 1 - 1],
            cmd->data[cmd->len - 1 - 0]
        );
#endif
        printk("CMD segment piece loop len %d\n", cmd->len);
        ret = bl_io_write(card, (u8*)cmd, BOOTROM_CMD_BUFFER_SIZE);
        if (ret) {
            printk("ERR when write\n");
            return -1;
        }
#if 0
        ret = bl_io_read(card, response, BOOTROM_CMD_BUFFER_SIZE);
        if (ret) {
            printk("NULL response get");
            return -1;
        }
        bl_bootrom_cmd_sectiondata_get_res((bootrom_res_sectiondata_load_t*)response);
#endif
        pos += wr_len;
    }

    *data += (sizeof(segment_header_t) + segment->len);
    *len -= (sizeof(segment_header_t) + segment->len);

    SDIO_DBG(SDIO_FN_LEAVE_STR);

    return 0;
}

static int firmware_download_segment(struct bl_plat *card, bootrom_host_cmd_t *cmd, u8 *response,
    bootheader_t *header, const u8 **data, int *len)
{
    int i, ret;

    printk("SEGMENT AERA: @Offset 0x%lX\n", (u8*)(*data) - (u8*)(header));
    printk("FUNC segment: data ptr %p, data left %d\n", *data, *len);
    for (i = 0; i < header->segment_cnt; i++) {
        printk("------SEGMENT[%02d]@0x%lX------\n", i, (u8*)(*data) - (u8*)(header));
        ret = firmware_download_segment_piece(card, cmd, response, header, data, len);
        if (ret) {
            break;
        }
    }

    return ret;
}

static int firmware_download_image_check(struct bl_plat *card, bootrom_host_cmd_t *cmd, u8 *response, bootheader_t *header)
{
    int ret;

    memset(cmd, 0, BOOTROM_CMD_BUFFER_SIZE);
    bl_bootrom_cmd_checkimage_get(cmd);
    printk("CMD checkimage len %d\n", cmd->len);
    ret = bl_io_write(card, (u8*)cmd, BOOTROM_CMD_BUFFER_SIZE);
    if (ret) {
        printk("ERR when write\n");
        return -1;
    }
    ret = bl_io_read(card, response, BOOTROM_CMD_BUFFER_SIZE);
    if (ret) {
        printk("NULL response get");
        return -1;
    }
    ret = bl_bootrom_cmd_checkimage_get_res((bootrom_res_checkimage_t*)response);
    if (ret) {
        printk("Error response get checkimage\n");
        return -1;
    }

    return 0;
}

static int firmware_download_image_run(struct bl_plat *card, bootrom_host_cmd_t *cmd, u8 *response, bootheader_t *header)
{
    int ret;

    memset(cmd, 0, BOOTROM_CMD_BUFFER_SIZE);
    bl_bootrom_cmd_runimage_get(cmd);
    printk("CMD runimage len %d\n", cmd->len);
    ret = bl_io_write(card, (u8*)cmd, BOOTROM_CMD_BUFFER_SIZE);
    if (ret) {
        printk("ERR when write\n");
        return -1;
    }
    ret = bl_io_read(card, response, BOOTROM_CMD_BUFFER_SIZE);
    if (ret) {
        printk("NULL response get");
        return -1;
    }
    bl_bootrom_cmd_runimage_get_res((bootrom_res_runimage_t*)response);

    return 0;
}

static int firmware_downloader(struct bl_plat *card, const u8 *data, int len)
{
    int ret = 0;
    bootheader_t *header;
    bootrom_host_cmd_t *cmd;
    u8 *response;

    if (len < sizeof(bootheader_t)) {
        printk("Illegal len when check bootheader_t\n");
        return -1;
    }
    header = (bootheader_t*)data;
    cmd = kzalloc(BOOTROM_DATA_BUFFER_SIZE, GFP_KERNEL);
    if (NULL == cmd) {
        printk("NO MEM for alloc host CMD\n");
        return -1;
    }

    response = kzalloc(BOOTROM_CMD_BUFFER_SIZE, GFP_KERNEL);
    if (NULL == response) {
        printk("NO MEM for alloc CMD response\n");
        kfree(cmd);
        return -1;
    }

    /*do NOT change the order bellow*/
    if (firmware_download_head(card, cmd, response, header, &data, &len)
        || firmware_download_flashcfg(card, cmd, response, header)
        || firmware_download_pllcfg(card, cmd, response, header)
        || firmware_download_publickey1(card, cmd, response, header, &data, &len)
        //|| firmware_download_publickey2(card, cmd, response, header, &data, &len)
        || firmware_download_signdata1(card, cmd, response, header, &data, &len)
        //|| firmware_download_signdata2(card, cmd, response, header, &data, &len)
        || firmware_download_encryptiondata(card, cmd, response, header, &data, &len)
        || firmware_download_segment(card, cmd, response, header, &data, &len)
        || firmware_download_image_check(card, cmd, response, header)) {
        printk("firmware load faield\n");
        return -1;
    }
    /*check if we need to download the other image*/
    if (len > 0) {
        printk("Potential Second image found\n");
        header = (bootheader_t*)data;//update header to the next image
        if (firmware_download_head(card, cmd, response, header, &data, &len)
            || firmware_download_flashcfg(card, cmd, response, header)
            || firmware_download_pllcfg(card, cmd, response, header)
            || firmware_download_publickey1(card, cmd, response, header, &data, &len)
            //|| firmware_download_publickey2(card, cmd, response, header, &data, &len)
            || firmware_download_signdata1(card, cmd, response, header, &data, &len)
            //|| firmware_download_signdata2(card, cmd, response, header, &data, &len)
            || firmware_download_encryptiondata(card, cmd, response, header, &data, &len)
            || firmware_download_segment(card, cmd, response, header, &data, &len)
            || firmware_download_image_check(card, cmd, response, header)) {
            printk("Sencond firmware load faield\n");
            return -1;
        }
    }
    if (firmware_download_image_run(card, cmd, response, header)) {
        printk("firmware check/Run faield\n");
        ret = -1;
    }

    kfree(cmd);
    kfree(response);
    return ret;
}

static int bl_sdio_checkversion(struct bl_plat *card)
{
    bootrom_host_cmd_t *cmd;
    int ret = 0;
    u8 *response;

	SDIO_DBG(SDIO_FN_ENTRY_STR);

    cmd = kzalloc(BOOTROM_CMD_BUFFER_SIZE, GFP_KERNEL);
    if (NULL == cmd) {
        printk("NO MEM for alloc host CMD\n");
        return -1;
    }
    response = kzalloc(BOOTROM_CMD_BUFFER_SIZE, GFP_KERNEL);
    if (NULL == response) {
        printk("NO MEM for alloc CMD response\n");
        return -1;
    }

    memset(cmd, 0, BOOTROM_CMD_BUFFER_SIZE);
    memset(response, 0, BOOTROM_CMD_BUFFER_SIZE);
    bl_bootrom_cmd_bootinfo_get(cmd);
    printk("CMD check version len %d\n", cmd->len);
    ret = bl_io_write(card, (u8*)cmd, BOOTROM_CMD_BUFFER_SIZE);
    if (ret) {
        printk("ERR when write\n");
        return -1;
    }
    ret = bl_io_read(card, response, BOOTROM_CMD_BUFFER_SIZE);
    if (ret) {
        printk("NULL response get");
        return -1;
    }
    bl_bootrom_cmd_bootinfo_get_res((bootrom_res_bootinfo_t*)response);

    kfree(response);
    kfree(cmd);
	SDIO_DBG(SDIO_FN_LEAVE_STR);

    return ret;
}

static int bl_sdio_download_firmware(struct bl_plat *card)
{
    const struct firmware *fw_helper = NULL;
    int ret, imagelen;
    const u8 *image = NULL;
	ktime_t start;
	s64 consume_time;

    printk("Enter bl_sdio_download_firmware...\n");
    ret = request_firmware(&fw_helper, "img_if_bl602_a0_rel_f157d2013f.bin", &card->func->dev);
    if ((ret < 0) || !fw_helper) {
        printk("request_firmware failed, error code = %d", ret);
        ret = -ENOENT;
	return ret;
    }

    image = fw_helper->data;
    imagelen = fw_helper->size;

    BL_DBG("Downloading image (%d bytes)\n", imagelen);
    //time_start = jiffies;
	start = ktime_get();	
    firmware_downloader(card, image, imagelen);
	consume_time = ktime_us_delta(ktime_get(), start);
	BL_DBG("Download fw time: %lld\n", consume_time);
	
    //time_end = jiffies;

    release_firmware(fw_helper);

    return ret;

}

int bl_sdio_init(struct bl_plat *card)
{
	u32 ret = 0;
	
	SDIO_DBG(SDIO_FN_ENTRY_STR);

	ret = bl_sdio_run_fw(card);

	ret = bl_sdio_checkversion(card);

	ret = bl_sdio_download_firmware(card);

	SDIO_DBG(SDIO_FN_LEAVE_STR);

	return ret;
}

int bl_read_data_sync_claim0(struct bl_hw *bl_hw, u8 *buffer, u32 len, u32 port)
{
	int ret;
	u8 blk_mode = (len < BL_SDIO_BLOCK_SIZE) ? BYTE_MODE : BLOCK_MODE;	
	u32 blk_size = (blk_mode == BLOCK_MODE) ? BL_SDIO_BLOCK_SIZE : 1;
	u32 blk_cnt = (blk_mode == BLOCK_MODE) ? ((len + BL_SDIO_BLOCK_SIZE -1)/BL_SDIO_BLOCK_SIZE) : len;
	u32 ioport = (port & BL_SDIO_IO_PORT_MASK);

	ret = sdio_readsb(bl_hw->plat->func, buffer, ioport, blk_cnt * blk_size);

	return ret;
}

/*
 * This function writes multiple data into SDIO card memory.
 *
 * This does not work in suspended mode.
 */
int bl_write_data_sync(struct bl_hw *bl_hw, u8 *buffer, u32 pkt_len, u32 port)
{
	int ret;
	u8 blk_mode = (pkt_len < BL_SDIO_BLOCK_SIZE) ? BYTE_MODE : BLOCK_MODE;
	u32 blk_size = (blk_mode == BLOCK_MODE) ? BL_SDIO_BLOCK_SIZE : 1;
	u32 blk_cnt = (blk_mode == BLOCK_MODE) ? ((pkt_len + BL_SDIO_BLOCK_SIZE -1)/BL_SDIO_BLOCK_SIZE) : pkt_len;

	u32 ioport = (port & BL_SDIO_IO_PORT_MASK);

	sdio_claim_host(bl_hw->plat->func);
	ret = sdio_writesb(bl_hw->plat->func, ioport, buffer, blk_cnt * blk_size);
	sdio_release_host(bl_hw->plat->func);

	return ret;
}

/*
 * This function reads multiple data from SDIO card memory.
 */
int bl_read_data_sync(struct bl_hw *bl_hw, u8 *buffer, u32 len, u32 port)
{
	int ret;
	u8 blk_mode = (len < BL_SDIO_BLOCK_SIZE) ? BYTE_MODE : BLOCK_MODE;	
	u32 blk_size = (blk_mode == BLOCK_MODE) ? BL_SDIO_BLOCK_SIZE : 1;
	u32 blk_cnt = (blk_mode == BLOCK_MODE) ? ((len + BL_SDIO_BLOCK_SIZE -1)/BL_SDIO_BLOCK_SIZE) : len;
	u32 ioport = (port & BL_SDIO_IO_PORT_MASK);

	sdio_claim_host(bl_hw->plat->func);
	ret = sdio_readsb(bl_hw->plat->func, buffer, ioport, blk_cnt * blk_size);
	sdio_release_host(bl_hw->plat->func);

	return ret;
}

/*
 * This function reads data from SDIO card register.
 */
int bl_read_reg(struct bl_hw *bl_hw, u32 reg, u8 *data)
{
	int ret = -1;
	u8 val;

	sdio_claim_host(bl_hw->plat->func);
	val = sdio_readb(bl_hw->plat->func, reg, &ret);
	sdio_release_host(bl_hw->plat->func);

	*data = val;

	return ret;
}

/*
 * This function write data to SDIO card register.
 */
int bl_write_reg(struct bl_hw *bl_hw, u32 reg, u8 data)
{
	int ret;

	sdio_claim_host(bl_hw->plat->func);
	sdio_writeb(bl_hw->plat->func, data, reg, &ret);
	sdio_release_host(bl_hw->plat->func);

	return ret;
}

static int bl_sdio_probe(struct sdio_func *func, const struct sdio_device_id *id)
{
    struct bl_plat *bl_plat = NULL;
    void *drvdata;
    int ret = -ENODEV;

    BL_DBG(BL_FN_ENTRY_STR);

	BL_DBG("info: vendor=0x%4.04X device=0x%4.04X class=%d function=%d\n",
		 func->vendor, func->device, func->class, func->num);
		
    ret = bl_device_init(func, &bl_plat);
    if (ret)
        return ret;

    bl_plat->func = func;

    ret = bl_platform_init(bl_plat, &drvdata);
	sdio_set_drvdata(func, drvdata);

    if (ret)
		bl_device_deinit(bl_plat);

    return ret;
}

static void bl_sdio_remove(struct sdio_func *func)
{    
	struct bl_hw *bl_hw;
    struct bl_plat *bl_plat;

    BL_DBG(BL_FN_ENTRY_STR);

    bl_hw = sdio_get_drvdata(func);
    bl_plat = bl_hw->plat;

    bl_platform_deinit(bl_hw);
    bl_device_deinit(bl_plat);

    sdio_set_drvdata(func, NULL);

}

static struct sdio_driver bl_sdio_drv = {
    .name     = KBUILD_MODNAME,
    .id_table = bl_sdio_ids,
    .probe    = bl_sdio_probe,
    .remove   = bl_sdio_remove
};

int bl_sdio_register_drv(void)
{
    return sdio_register_driver(&bl_sdio_drv);
}

void bl_sdio_unregister_drv(void)
{
    sdio_unregister_driver(&bl_sdio_drv);
}
