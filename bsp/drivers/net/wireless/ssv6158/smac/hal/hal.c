/*
 * Copyright (c) 2015 iComm-semi Ltd.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef ECLIPSE
#include <ssv_mod_conf.h>
#endif // ECLIPSE
#include <linux/version.h>
#include <linux/platform_device.h>

#include <linux/string.h>
#include <ssv_chip_id.h>
#include <ssv6200.h>
#include <smac/dev.h>
#include <hal.h>
#include <smac/ssv_skb.h>
#include <hci/hctrl.h>

//include firmware binary header
#include <include/ssv6x5x-sw.h>
MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);
static void ssv6xxx_cmd_rc(struct ssv_hw *sh, int argc, char *argv[])
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_set_sifs(struct ssv_hw *sh, int band)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_cmd_hwinfo(struct ssv_hw *sh, int argc, char *argv[])
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}
static int ssv6xxx_get_tkip_mmic_err(struct sk_buff *skb)
{
    return 0;
}

static void ssv6xxx_cmd_txgen(struct ssv_hw *sh, u8 drate)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_cmd_rf(struct ssv_hw *sh, int argc, char *argv[])
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_cmd_rfble(struct ssv_hw *sh, int argc, char *argv[])
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_cmd_efuse(struct ssv_hw *sh, int argc, char *argv[])
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_cmd_hwq_limit(struct ssv_hw *sh, int argc, char *argv[])
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_init_gpio_cfg(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_flash_read_all_map(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static u8 ssv6xxx_read_efuse(struct ssv_hw *sh, u8 *mac)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
        "%s is not supported for this model!!\n",__func__);
    return 1;
}

static void ssv6xxx_update_rf_table(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

void ssv6xxx_set_on3_enable(struct ssv_hw *sh, bool val)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static int ssv6xxx_init_hci_rx_aggr(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
    return -1;
}

static int ssv6xxx_reset_hw_mac(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
    return -1;
}

static void ssv6xxx_set_crystal_clk(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_wait_usb_rom_ready(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_detach_usb_hci(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_pll_chk(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static bool ssv6xxx_put_mic_space_for_hw_ccmp_encrypt(struct ssv_softc *sc, struct sk_buff *skb)
{

    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
    return false;
}

#ifdef CONFIG_PM
static void ssv6xxx_save_clear_trap_reason(struct ssv_softc *sc)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);

}

static void ssv6xxx_restore_trap_reason(struct ssv_softc *sc)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);

}

static void ssv6xxx_ps_save_reset_rx_flow(struct ssv_softc *sc)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);

}

static void ssv6xxx_ps_restore_rx_flow(struct ssv_softc *sc)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_pmu_awake(struct ssv_softc *sc)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_ps_hold_on3(struct ssv_softc *sc, int value)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
}
#endif

static int ssv6xxx_get_sram_mode(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
	    "%s is not supported for this model!!\n",__func__);
    return 0;
}

static void ssv6xxx_enable_sram(struct ssv_hw *sh)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
        "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_rc_mac80211_tx_rate_idx(struct ssv_softc *sc, int hw_rate_idx, struct ieee80211_tx_info *tx_info)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
        "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_update_mac80211_chan_info(struct ssv_softc *sc,
            struct sk_buff *skb, struct ieee80211_rx_status *rxs)
{
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
        "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_read_allid_map(struct ssv_hw *sh, u32 *id0, u32 *id1, u32 *id2, u32 *id3)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
        "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_read_txid_map(struct ssv_hw *sh, u32 *id0, u32 *id1, u32 *id2, u32 *id3)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
        "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_read_rxid_map(struct ssv_hw *sh, u32 *id0, u32 *id1, u32 *id2, u32 *id3)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
        "%s is not supported for this model!!\n",__func__);
}

static void ssv6xxx_read_wifi_halt_status(struct ssv_hw *sh, u32 *status)
{
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL,
        "%s is not supported for this model!!\n",__func__);
}

/////// not register ops
static void *ssv6xxx_open_firmware(char *user_mainfw)
{
    struct file *fp;

    fp = filp_open(user_mainfw, O_RDONLY, 0);
    if (IS_ERR(fp))
        fp = NULL;

    return fp;
}

static int ssv6xxx_read_fw_block(char *buf, int len, void *image)
{
    struct file *fp = (struct file *)image;
    int rdlen;

    if (!image)
        return 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
    rdlen = kernel_read(fp, buf, len, &fp->f_pos);
#else
    rdlen = kernel_read(fp, fp->f_pos, buf, len);
    if (rdlen > 0)
        fp->f_pos += rdlen;
#endif

    return rdlen;
}

static void ssv6xxx_close_firmware(void *image)
{
    if (image)
        filp_close((struct file *)image, NULL);
}

/* ####### firmware info #######
 * ------- block count        -------    4 bytes
 * ------- block0 sram addr   -------    4 bytes
 * ------- block0 sram len    -------    4 bytes
 * ------- block1 sram addr   -------    4 bytes
 * ------- block1 sram len    -------    4 bytes
 * ------- block2 sram addr   -------    4 bytes
 * ------- block2 sram len    -------    4 bytes
 * */
#define MAX_FIRMWARE_BLOCKS_CNT     3
#define MAC_FIRMWARE_BLOCKS_LEN     8
static int ssv6xxx_get_firmware_info(struct ssv_hw *sh, void *fw_fp, u8 openfile)
{
    int offset = 0;
    int i = 0, len = 0;
    u8 buf[MAX_FIRMWARE_BLOCKS_CNT * MAC_FIRMWARE_BLOCKS_LEN];
    u32 fw_block_cnt = 0;
    u32 fw_block_info_len = 0;
    u32 start_addr = 0, fw_block_len = 0;

    memset(buf, 0, MAX_FIRMWARE_BLOCKS_CNT * MAC_FIRMWARE_BLOCKS_LEN);
    if (openfile) {
        // reset file pos
        ((struct file *)fw_fp)->f_pos = 0;

        // fw info len, 4 byte
        if (!(len = ssv6xxx_read_fw_block((char *)&fw_block_cnt, sizeof(u32), fw_fp))) {
            offset = -1;
            goto out;
        }

        // fw block info
        fw_block_info_len = fw_block_cnt * MAC_FIRMWARE_BLOCKS_LEN;
        if (!(len = ssv6xxx_read_fw_block((u8 *)buf, fw_block_info_len, fw_fp))) {
            offset = -1;
            goto out;
        }

    } else {
        memcpy(&fw_block_cnt, (u8 *)fw_fp, (sizeof(u32)));
        fw_block_info_len = fw_block_cnt * MAC_FIRMWARE_BLOCKS_LEN;
        memcpy((u8 *)buf, (u8 *)fw_fp+sizeof(u32), fw_block_info_len);
    }

    // parse fw block info
    for (i = 0; i < fw_block_cnt; i++) {
        start_addr =   (buf[i*8+0] << 0) | (buf[i*8+1] << 8) | (buf[i*8+2] << 16) | (buf[i*8+3] << 24);
        fw_block_len = (buf[i*8+4] << 0) | (buf[i*8+5] << 8) | (buf[i*8+6] << 16) | (buf[i*8+7] << 24);
        sh->fw_info[i].block_len = fw_block_len;
        sh->fw_info[i].block_start_addr = start_addr;
    }
    offset = 4 + fw_block_info_len;

out:
    return offset;
}

static u32 ssv6xxx_write_firmware_to_sram(struct ssv_hw *sh, void *fw_buffer,
        void *fw_fp, int fw_len, u32 sram_start_addr, u8 openfile, bool need_checksum, bool *write_sram_err)
{
    int ret = 0, i = 0;
    u32 len = 0, total_len = 0;
    u32 sram_addr = sram_start_addr;
    u32 *fw_data32 = NULL;
    u32 fw_res_size = fw_len;
    u32 rsize = 0;
    u32 checksum = 0;
    u8 *pt=NULL;
    struct ssv_softc *sc = sh->sc;
    dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL, "Writing firmware to SSV6XXX...\n");
    while (fw_res_size) { // load fw block size
        memset(fw_buffer, 0xA5, CHECKSUM_BLOCK_SIZE);
		rsize = (fw_res_size>CHECKSUM_BLOCK_SIZE)?CHECKSUM_BLOCK_SIZE:fw_res_size;
        if (openfile) {
            //((struct file *)fw_fp)->f_pos = fw_offset;
            if (!(len = ssv6xxx_read_fw_block((char*)fw_buffer, rsize, fw_fp))) {
                break;
            }
        } else {
            memcpy((void *)fw_buffer, (const void *)fw_fp, rsize);
            pt=(u8 *)(fw_fp);
            fw_fp = (void *)(pt+CHECKSUM_BLOCK_SIZE);
        }

        fw_res_size -= rsize;
        total_len += rsize;
        #if 1
        if ((ret = SMAC_LOAD_FW(sh, sram_addr, (u8 *)fw_buffer, CHECKSUM_BLOCK_SIZE)) != 0) {
            *write_sram_err = true;
            goto out;
        }
    #endif
        // checksum
        if (need_checksum) {
            fw_data32 = (u32 *)fw_buffer;
            for (i = 0; i < (CHECKSUM_BLOCK_SIZE / sizeof(u32)); i++)
                checksum += fw_data32[i];
        }

        sram_addr += CHECKSUM_BLOCK_SIZE;
    }

out:
    return checksum;
}

static int _ssv6xxx_load_firmware(struct ssv_hw *sh, u8 *firmware_name, u8 openfile)
{
    int ret = 0;
    u8   *fw_buffer = NULL;
    u32   block_count = 0, res_size = 0;
    u32   ilm_block_count = 0, bus_block_count = 0;
    void *fw_fp = NULL;
    //u8    interface = SSV_HWIF_INTERFACE_SDIO;
    u32   checksum = FW_CHECKSUM_INIT;
    u32   fw_checksum, fw_clkcnt;
    u32   retry_count = 1;
    int fw_start_offset = 0, fw_pos = 0;
    int fw_block_idx = 0;
    bool write_sram_err = false;
    u8 *pt=NULL;
    struct ssv_softc *sc = sh->sc;

    //if (hci_ctrl->redownload == 1) {
    /* force mcu jump to rom for always*/
    // Redownload firmware
    //HCI_DBG_PRINT(hci_ctrl, "Re-download FW\n");
    //HCI_JUMP_TO_ROM(sh);
    HAL_JUMP_TO_ROM(sh->sc);
    //}

    // Enable all SRAM
    HAL_ENABLE_SRAM(sh);

    // Load firmware
    if(openfile)
    {
        fw_fp = ssv6xxx_open_firmware(firmware_name);
        if (!fw_fp) {
            dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL, "failed to find firmware (%s)\n", firmware_name);
            ret = -1;
            goto out;
        }
    }
    else
    {
        fw_fp = (void *)ssv6x5x_sw_bin;
    }

    fw_start_offset = ssv6xxx_get_firmware_info(sh, fw_fp, openfile);
    if (fw_start_offset < 0) {
        dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL, "Failed to get firmware information\n");
        ret = -1;
        goto out;
    }

    // Allocate buffer firmware aligned with FW_BLOCK_SIZE and padding with 0xA5 in empty space.
    fw_buffer = (u8 *)kzalloc(FW_BLOCK_SIZE, GFP_KERNEL);
    if (fw_buffer == NULL) {
        dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL, "Failed to allocate buffer for firmware.\n");
        ret = -1;
        goto out;
    }

    do {
        // initial value
        checksum = FW_CHECKSUM_INIT;
        res_size = 0;

        // Disable MCU (USB ROM code must be alive for downloading FW, so USB doesn't do it)
        //if (!(interface == SSV_HWIF_INTERFACE_USB)) {
            ret = HAL_LOAD_FW_DISABLE_MCU(sh);
            if (ret == -1)
                goto out;
        //}

        for (fw_block_idx = 0; fw_block_idx < MAX_FIRMWARE_BLOCKS_CNT; fw_block_idx++) {
            // calculate the fw block position
            if (fw_block_idx == 0)
                fw_pos += fw_start_offset;
            else
                fw_pos += sh->fw_info[fw_block_idx-1].block_len;

            if (openfile)
                ((struct file *)fw_fp)->f_pos = fw_pos;
            else {
                fw_fp = (void *)ssv6x5x_sw_bin;
                pt=(u8 *)(fw_fp);
                fw_fp = (void *)(pt+fw_pos);
            }

            // write firmware to sram and calculate checksum(ILM, BUS)
            // fw_block_idx == 1, DLM
            checksum += ssv6xxx_write_firmware_to_sram(sh, (void *)fw_buffer, (void *)fw_fp,
                    sh->fw_info[fw_block_idx].block_len, sh->fw_info[fw_block_idx].block_start_addr,
                    openfile, ((fw_block_idx == 1) ? false : true), &write_sram_err);

            if (write_sram_err) {
                printk("Firmware \"%s\" fail to write sram.\n", firmware_name);
                ret = -1;
                goto out;
            }
        }

        // Calculate the final checksum.
        checksum = ((checksum >> 24) + (checksum >> 16) + (checksum >> 8) + checksum) & 0x0FF;
        checksum <<= 16;
#if 0
        // Reset CPU for USB switching ROM to firmware
        if (interface == SSV_HWIF_INTERFACE_USB) {
            if (-1 == (ret = SSV_RESET_CPU(hci_ctrl)))
                goto out;
        }
#endif
        // calculate ILM block count
        ilm_block_count = sh->fw_info[0].block_len / CHECKSUM_BLOCK_SIZE;
        res_size = sh->fw_info[0].block_len % CHECKSUM_BLOCK_SIZE;
        if (res_size)
            ilm_block_count++;

        // calculate BUS block count
        bus_block_count = sh->fw_info[2].block_len / CHECKSUM_BLOCK_SIZE;
        res_size = sh->fw_info[2].block_len % CHECKSUM_BLOCK_SIZE;
        if (res_size)
            bus_block_count++;

        block_count = ((ilm_block_count<<16) | (bus_block_count<<0));
        // Inform FW that how many blocks is downloaded such that FW can calculate the checksum.
        HAL_LOAD_FW_SET_STATUS(sh, block_count);
        HAL_LOAD_FW_GET_STATUS(sh, &fw_clkcnt);
        dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL, "block_count = 0x%08x, reg = 0x%08x\n", block_count, fw_clkcnt);
        printk("block_count = 0x%08x, reg = 0x%08x\n", block_count, fw_clkcnt);
        // Release reset to let CPU run.
        HAL_LOAD_FW_ENABLE_MCU(sh);
        dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL, "Firmware \"%s\" loaded\n", firmware_name);
        // Wait FW to calculate checksum.
        msleep(50);
        // Check checksum result and set to complement value if checksum is OK.
        HAL_LOAD_FW_GET_STATUS(sh, &fw_checksum);
        fw_checksum = fw_checksum & FW_STATUS_MASK;
        if (fw_checksum == checksum) {
            HAL_LOAD_FW_SET_STATUS(sh, (~checksum & FW_STATUS_MASK));
            dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL, "Firmware check OK.%04x = %04x\n", fw_checksum, checksum);
            ret = 0;
        } else {
            dbgprint(&sc->cmd_data, sc->log_ctrl, LOG_HAL, "FW checksum error: %04x != %04x\n", fw_checksum, checksum);
            printk("FW checksum error: %04x != %04x\n", fw_checksum, checksum);
            ret = -1;
        }

        if (0 == ret) // firmware check ok
	        break;

    } while (--retry_count);

out:
    if (fw_buffer != NULL) {
        kfree(fw_buffer);
    }

    if (openfile) {
        if (fw_fp) {
            ssv6xxx_close_firmware(fw_fp);
        }
    }

    return ret;
}

static int ssv6xxx_load_firmware_openfile(struct ssv_hw *sh, u8 *firmware_name)
{
    return _ssv6xxx_load_firmware(sh, firmware_name, 1);
}

static int ssv6xxx_load_firmware_fromheader(struct ssv_hw *sh, u8 *firmware_name)
{
    return _ssv6xxx_load_firmware(sh, firmware_name, 0);
}
/////// not register ops
//HAL OPS
static int ssv6xxx_load_fw(struct ssv_hw *sh, u8 *firmware_name, u8 openfile)
{
    int ret = 0;

    HAL_LOAD_FW_PRE_CONFIG_DEVICE(sh);

    if (openfile)
        ret = ssv6xxx_load_firmware_openfile(sh, firmware_name);
    else
        ret = ssv6xxx_load_firmware_fromheader(sh, firmware_name);
#if 0 //Keep it, but not used.
    ret = ssv6xxx_hci_load_firmware_request(hci_ctrl, firmware_name);
#endif

    // Sleep to let SSV6XXX get ready.
    msleep(50);

    if (ret == 0)
        HAL_LOAD_FW_POST_CONFIG_DEVICE(sh);

    return ret;
}
static void ssv6xxx_attach_common_hal (struct ssv_hal_ops  *hal_ops)
{
    hal_ops->cmd_rc = ssv6xxx_cmd_rc;
    hal_ops->set_sifs = ssv6xxx_set_sifs;
	hal_ops->cmd_hwinfo = ssv6xxx_cmd_hwinfo;
	hal_ops->get_tkip_mmic_err = ssv6xxx_get_tkip_mmic_err;
	hal_ops->cmd_txgen = ssv6xxx_cmd_txgen;
	hal_ops->cmd_rf = ssv6xxx_cmd_rf;
	hal_ops->cmd_rfble = ssv6xxx_cmd_rfble;
	hal_ops->cmd_efuse = ssv6xxx_cmd_efuse;
	hal_ops->cmd_hwq_limit = ssv6xxx_cmd_hwq_limit;
    hal_ops->init_gpio_cfg = ssv6xxx_init_gpio_cfg;
    hal_ops->flash_read_all_map = ssv6xxx_flash_read_all_map;
    hal_ops->read_efuse = ssv6xxx_read_efuse;
    hal_ops->update_rf_table = ssv6xxx_update_rf_table;
    hal_ops->set_on3_enable = ssv6xxx_set_on3_enable;
    hal_ops->init_hci_rx_aggr = ssv6xxx_init_hci_rx_aggr;
    hal_ops->reset_hw_mac = ssv6xxx_reset_hw_mac;
    hal_ops->set_crystal_clk = ssv6xxx_set_crystal_clk;
    hal_ops->wait_usb_rom_ready = ssv6xxx_wait_usb_rom_ready;
    hal_ops->detach_usb_hci = ssv6xxx_detach_usb_hci;
    hal_ops->pll_chk = ssv6xxx_pll_chk;
    hal_ops->put_mic_space_for_hw_ccmp_encrypt = ssv6xxx_put_mic_space_for_hw_ccmp_encrypt;
#ifdef CONFIG_PM
	hal_ops->save_clear_trap_reason = ssv6xxx_save_clear_trap_reason;
	hal_ops->restore_trap_reason = ssv6xxx_restore_trap_reason;
	hal_ops->ps_save_reset_rx_flow = ssv6xxx_ps_save_reset_rx_flow;
	hal_ops->ps_restore_rx_flow = ssv6xxx_ps_restore_rx_flow;
	hal_ops->pmu_awake = ssv6xxx_pmu_awake;
	hal_ops->ps_hold_on3 = ssv6xxx_ps_hold_on3;
#endif
    hal_ops->get_sram_mode = ssv6xxx_get_sram_mode;
    hal_ops->enable_sram = ssv6xxx_enable_sram;
    hal_ops->rc_mac80211_tx_rate_idx = ssv6xxx_rc_mac80211_tx_rate_idx;
    hal_ops->update_mac80211_chan_info = ssv6xxx_update_mac80211_chan_info;
    hal_ops->read_allid_map = ssv6xxx_read_allid_map;
    hal_ops->read_txid_map = ssv6xxx_read_txid_map;
    hal_ops->read_rxid_map = ssv6xxx_read_rxid_map;
    hal_ops->read_wifi_halt_status = ssv6xxx_read_wifi_halt_status;
    hal_ops->load_fw = ssv6xxx_load_fw;
}


int ssv6xxx_init_hal(struct ssv_softc *sc)
{
    struct ssv_hw *sh;
    int ret = 0;
    struct ssv_hal_ops *hal_ops = NULL;
    extern void ssv_attach_ssv6006(struct ssv_softc *sc, struct ssv_hal_ops *hal_ops);
    extern void ssv_attach_ssv6020(struct ssv_softc *sc, struct ssv_hal_ops *hal_ops);
    bool chip_supportted = false;
    struct ssv6xxx_platform_data *priv = sc->dev->platform_data;

	// alloc hal_ops memory
	hal_ops = kzalloc(sizeof(struct ssv_hal_ops), GFP_KERNEL);
	if (hal_ops == NULL) {
		printk("%s(): Fail to alloc hal_ops\n", __FUNCTION__);
		return -ENOMEM;
	}

    // load common HAL layer function;
    ssv6xxx_attach_common_hal(hal_ops);

#ifdef SSV_SUPPORT_SSV6006
    if (   strstr(priv->chip_id, SSV6006)
		 || strstr(priv->chip_id, SSV6006C)
		 || strstr(priv->chip_id, SSV6006D)) {

        printk(KERN_INFO"%s\n", sc->platform_dev->id_entry->name);
        printk(KERN_INFO"Attach SSV6006 family HAL function  \n");

        ssv_attach_ssv6006(sc, hal_ops);
        chip_supportted = true;
    }
#endif
#ifdef SSV_SUPPORT_SSV6020
    if (strstr(priv->chip_id, SSV6020B)
        || strstr(priv->chip_id, SSV6020C)) {
        printk(KERN_INFO"%s\n", sc->platform_dev->id_entry->name);
        printk(KERN_INFO"Attach SSV6020 family HAL function  \n");
        ssv_attach_ssv6020(sc, hal_ops);
        chip_supportted = true;
    }
#endif
    if (!chip_supportted) {

        printk(KERN_ERR "Chip \"%s\" is not supported by this driver\n", sc->platform_dev->id_entry->name);
        ret = -EINVAL;
        goto out;
    }

    sh = hal_ops->alloc_hw();
    if (sh == NULL) {
        ret = -ENOMEM;
        goto out;
    }

    memcpy(&sh->hal_ops, hal_ops, sizeof(struct ssv_hal_ops));
    sc->sh = sh;
    sh->sc = sc;
    sh->priv = sc->dev->platform_data;
    sh->hci.dev = sc->dev;
    sh->hci.if_ops = sh->priv->ops;
    sh->hci.skb_alloc = ssv_skb_alloc;
    sh->hci.skb_free = ssv_skb_free;
    sh->hci.hci_rx_cb = ssv6200_rx;
    sh->hci.hci_is_rx_q_full = ssv6200_is_rx_q_full;

    sh->priv->skb_alloc = ssv_skb_alloc_ex;
    sh->priv->skb_free = ssv_skb_free;
    sh->priv->skb_param = sc;

    // Set jump to rom functions for HWIF
    sh->priv->enable_usb_acc = ssv6xxx_enable_usb_acc;
    sh->priv->disable_usb_acc = ssv6xxx_disable_usb_acc;
    sh->priv->jump_to_rom = ssv6xxx_jump_to_rom;
    sh->priv->usb_param = sc;

    sh->priv->rx_mode = ssv6xxx_rx_mode;
    sh->priv->rx_mode_param = sc;

    sh->hci.sc = sc;
    sh->hci.sh = sh;

out:
	kfree(hal_ops);
    return ret;
}
