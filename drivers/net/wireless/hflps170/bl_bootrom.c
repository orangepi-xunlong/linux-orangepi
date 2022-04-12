#include <linux/printk.h>
#include <linux/string.h>

#include "bl_bootrom.h"

int bl_bootrom_cmd_len(bootrom_host_cmd_t *cmd)
{
    return cmd->len + BL_BOOTROM_HOST_CMD_LEN_HEADER;
}

int bl_bootrom_cmd_bootinfo_get(bootrom_host_cmd_t *cmd)
{
    cmd->id = BL_BOOTROM_HOST_CMD_BOOTINFO_GET;
    cmd->len = 0;
    return 0;
}

int bl_bootrom_cmd_bootinfo_get_res(bootrom_res_bootinfo_t *bootinfo)
{
    if ('O' == bootinfo->status[0] &&
        'K' == bootinfo->status[1]) {
        printk("[RSP] bootinfo versoin: 0x%08X\n",  bootinfo->version);
        printk("[RSP] bootinfo password_mode: 0x%02X\n", bootinfo->password_mode);
        printk("[RSP] bootinfo sboot_enable: 0x%02X\n", bootinfo->sboot_enable);
        printk("[RSP] bootinfo jtag_disable: 0x%02X\n", bootinfo->jtag_disable);
        printk("[RSP] bootinfo uart_disable: 0x%02X\n", bootinfo->uart_disable);
        printk("[RSP] bootinfo sign_type: 0x%02X\n", bootinfo->sign_type);
        printk("[RSP] bootinfo aes_type: 0x%02X\n", bootinfo->aes_type);
        printk("[RSP] bootinfo reserved: 0x%02X\n", bootinfo->reserved);
        printk("[RSP] bootinfo chip_id: 0x%02X%02X%02X%02X%02X\n",
            bootinfo->chip_id[0],
            bootinfo->chip_id[1],
            bootinfo->chip_id[2],
            bootinfo->chip_id[3],
            bootinfo->chip_id[4]
        );
    } else {
        printk("[RSP] unkown status:%c%c\n", 
            bootinfo->status[0],
            bootinfo->status[1]
        );
        return -1;
    }
    return -1;
}

#if 0
int bl_bootrom_cmd_password_load(bootrom_host_cmd_t *cmd, uint8_t *password)
{
    cmd->id = BL_BOOTROM_HOST_CMD_PASSWORD_LOAD;
    cmd->len = sizeof(bootrom_host_cmd_t) - BL_BOOTROM_HOST_CMD_LEN_HEADER;
    memcpy(cmd->data, password, sizeof(cmd->data));

    return 0;
}

int bl_bootrom_cmd_password_load_get_res(bootrom_res_password_load_t *bootresponse, int size)
{
    if (size != (sizeof(bootrom_res_password_load_t)) && size != 1) {
        printk("[RSP] password_load response len is not correct %d\n", size);
        return -1;
    }

    if ('O' == bootresponse->status[0] &&
        'K' == bootresponse->status[1] && 1 == size) {
        printk("[RSP] password_load response OK\n");
    } else if ('F' == bootresponse->status[0] &&
        'L' == bootresponse->status[1] && 3 == size) {
        printk("[RSP] password_load response failed, staus 0x%04X\n", bootresponse->code);
    } else {
        printk("[RSP] unkown status:%c%c\n", 
            bootresponse->status[0],
            bootresponse->status[1]
        );
        return -1;
    }
    return -1;
}

int bl_bootrom_cmd_jtag_open(bootrom_host_cmd_t *cmd, uint8_t *password)
{
    cmd->id = BL_BOOTROM_HOST_CMD_JTAG_OPEN;
    cmd->len = sizeof(bootrom_host_cmd_t) - BL_BOOTROM_HOST_CMD_LEN_HEADER;
    memcpy(cmd->data, password, sizeof(cmd->data));

    return 0;
}

int bl_bootrom_cmd_jtag_open_get_res(bootrom_res_jtag_open_t *bootresponse, int size)
{
    if (size != (sizeof(bootrom_res_jtag_open_t)) && size != 1) {
        printk("[RSP] jtag_open response len is not correct %d\n", size);
        return -1;
    }

    if ('O' == bootresponse->status[0] &&
        'K' == bootresponse->status[1] && 1 == size) {
        printk("[RSP] jtag_open response OK\n");
    } else if ('F' == bootresponse->status[0] &&
        'L' == bootresponse->status[1] && 3 == size) {
        printk("[RSP] jtag_open response failed, staus 0x%04X\n", bootresponse->code);
    } else {
        printk("[RSP] unkown status:%c%c\n", 
            bootresponse->status[0],
            bootresponse->status[1]
        );
        return -1;
    }
    return -1;
}
#endif

int bl_bootrom_cmd_bootheader_load(bootrom_host_cmd_t *cmd, bootheader_t *header)
{
    //XXX should caller known the size of cmd->data ?
    cmd->id = BL_BOOTROM_HOST_CMD_BOOTHEADER_LOAD;
    cmd->len = sizeof(bootrom_host_cmd_t) - BL_BOOTROM_HOST_CMD_LEN_HEADER + sizeof(bootheader_t);
    memcpy(cmd->data, header, sizeof(bootheader_t));

    return 0;
}

int bl_bootrom_cmd_bootheader_load_get_res(bootrom_res_bootheader_load_t *bootresponse)
{
    if ('O' == bootresponse->status[0] &&
        'K' == bootresponse->status[1]) {
        printk("[RSP] bootheader_load response OK\n");
    } else if ('F' == bootresponse->status[0] &&
        'L' == bootresponse->status[1]) {
        printk("[RSP] bootheader_load response failed, staus 0x%04X\n", bootresponse->code);
    } else {
        printk("[RSP] unkown status:%c%c\n", 
            bootresponse->status[0],
            bootresponse->status[1]
        );
        return -1;
    }
    return -1;
}

int bl_bootrom_cmd_aesiv_load(bootrom_host_cmd_t *cmd, const uint8_t *aesiv)
{
    //XXX should caller known the size of cmd->data ?
    cmd->id = BL_BOOTROM_HOST_CMD_AESIV_LOAD;
    cmd->len = sizeof(bootrom_host_cmd_t) - BL_BOOTROM_HOST_CMD_LEN_HEADER + 20;//FIXME use struct instead
    memcpy(cmd->data, aesiv, 20);//FIXME use struct instead, but NOT magic number(16 IV + 4 CRC32)

    return 0;
}

int bl_bootrom_cmd_aesiv_load_get_res(bootrom_res_aesiv_load_t *bootresponse)
{
    if ('O' == bootresponse->status[0] &&
        'K' == bootresponse->status[1]) {
        printk("[RSP] aesiv_load response OK\n");
    } else if ('F' == bootresponse->status[0] &&
        'L' == bootresponse->status[1]) {
        printk("[RSP] aesiv_load response failed, staus 0x%04X\n", bootresponse->code);
    } else {
        printk("[RSP] unkown status:%c%c\n", 
            bootresponse->status[0],
            bootresponse->status[1]
        );
        return -1;
    }
    return -1;
}

int bl_bootrom_cmd_pkey1_load(bootrom_host_cmd_t *cmd, pkey_cfg_t *pk)
{
    cmd->id = BL_BOOTROM_HOST_CMD_PK1_LOAD;
    cmd->len = sizeof(bootrom_host_cmd_t) - BL_BOOTROM_HOST_CMD_LEN_HEADER + sizeof(pkey_cfg_t);
    memcpy(cmd->data, pk, sizeof(pkey_cfg_t));

    return 0;
}

int bl_bootrom_cmd_pkey1_load_get_res(bootrom_res_pkey_load_t *bootresponse)
{
    if ('O' == bootresponse->status[0] &&
        'K' == bootresponse->status[1]) {
        printk("[RSP] pkey1_load response OK\n");
        return 0;
    } else if ('F' == bootresponse->status[0] &&
        'L' == bootresponse->status[1]) {
        printk("[RSP] pkey1_load response failed, staus 0x%04X\n", bootresponse->code);
    } else {
        printk("[RSP] unkown status:%c%c\n", 
            bootresponse->status[0],
            bootresponse->status[1]
        );
    }
    return -1;
}

int bl_bootrom_cmd_pkey2_load(bootrom_host_cmd_t *cmd, pkey_cfg_t *pk)
{
    cmd->id = BL_BOOTROM_HOST_CMD_PK2_LOAD;
    cmd->len = sizeof(bootrom_host_cmd_t) - BL_BOOTROM_HOST_CMD_LEN_HEADER + sizeof(pkey_cfg_t);
    memcpy(cmd->data, pk, sizeof(pkey_cfg_t));

    return 0;
}

int bl_bootrom_cmd_pkey2_load_get_res(bootrom_res_pkey_load_t *bootresponse)
{
    if ('O' == bootresponse->status[0] &&
        'K' == bootresponse->status[1]) {
        printk("[RSP] pkey2_load response OK\n");
        return 0;
    } else if ('F' == bootresponse->status[0] &&
        'L' == bootresponse->status[1]) {
        printk("[RSP] pkey2_load response failed, staus 0x%04X\n", bootresponse->code);
    } else {
        printk("[RSP] unkown status:%c%c\n", 
            bootresponse->status[0],
            bootresponse->status[1]
        );
    }
    return -1;
}

int bl_bootrom_cmd_signature1_load(bootrom_host_cmd_t *cmd, uint8_t *signature, int len)
{
    cmd->id = BL_BOOTROM_HOST_CMD_SIGNATURE1_LOAD;
    cmd->len = sizeof(bootrom_host_cmd_t) - BL_BOOTROM_HOST_CMD_LEN_HEADER + len;//use magic number
    memcpy(cmd->data, signature, len);//XXX overflow check

    return 0;
}

int bl_bootrom_cmd_signature1_get_res(bootrom_res_signature_load_t *bootresponse)
{
    if ('O' == bootresponse->status[0] &&
        'K' == bootresponse->status[1]) {
        printk("[RSP] signature1_load response OK\n");
        return 0;
    } else if ('F' == bootresponse->status[0] &&
        'L' == bootresponse->status[1]) {
        printk("[RSP] signature1_load response failed, staus 0x%04X\n", bootresponse->code);
    } else {
        printk("[RSP] unkown status:%c%c\n", 
            bootresponse->status[0],
            bootresponse->status[1]
        );
    }
    return -1;
}

int bl_bootrom_cmd_signature2_load(bootrom_host_cmd_t *cmd, uint8_t *signature, int len)
{
    cmd->id = BL_BOOTROM_HOST_CMD_SIGNATURE2_LOAD;
    cmd->len = sizeof(bootrom_host_cmd_t) - BL_BOOTROM_HOST_CMD_LEN_HEADER + len;//use magic number
    memcpy(cmd->data, signature, len);//XXX overflow check

    return 0;
}

int bl_bootrom_cmd_signature2_get_res(bootrom_res_signature_load_t *bootresponse)
{
    if ('O' == bootresponse->status[0] &&
        'K' == bootresponse->status[1]) {
        printk("[RSP] signature2_load response OK\n");
        return 0;
    } else if ('F' == bootresponse->status[0] &&
        'L' == bootresponse->status[1]) {
        printk("[RSP] signature2_load response failed, staus 0x%04X\n", bootresponse->code);
    } else {
        printk("[RSP] unkown status:%c%c\n", 
            bootresponse->status[0],
            bootresponse->status[1]
        );
    }
    return -1;
}

#if 0
int bl_bootrom_cmd_tzc_load(bootrom_host_cmd_t *cmd, uint8_t *tzc, int len)
{
    cmd->id = BL_BOOTROM_HOST_CMD_TZC_LOAD;
    cmd->len = sizeof(bootrom_host_cmd_t) - BL_BOOTROM_HOST_CMD_LEN_HEADER + len;//use magic number
    memcpy(cmd->data, tzc, len);//XXX overflow check

    return 0;
}

int bl_bootrom_cmd_tzc_get_res(bootrom_res_tzc_load_t *bootresponse, int size)
{
    if (size != (sizeof(bootrom_res_tzc_load_t)) && size != 1) {
        printk("[RSP] tzc_load response len is not correct %d\n", size);
        return -1;
    }

    if ('O' == bootresponse->status[0] &&
        'K' == bootresponse->status[1] && 1 == size) {
        printk("[RSP] tzc_load response OK\n");
    } else if ('F' == bootresponse->status[0] &&
        'L' == bootresponse->status[1] && 3 == size) {
        printk("[RSP] tzc_load response failed, staus 0x%04X\n", bootresponse->code);
    } else {
        printk("[RSP] unkown status:%c%c\n", 
            bootresponse->status[0],
            bootresponse->status[1]
        );
        return -1;
    }
    return -1;
}
#endif

int bl_bootrom_cmd_sectionheader_load(bootrom_host_cmd_t *cmd, const segment_header_t *header)
{
    cmd->id = BL_BOOTROM_HOST_CMD_SECTIONHEADER_LOAD;
    cmd->len = sizeof(bootrom_host_cmd_t) - BL_BOOTROM_HOST_CMD_LEN_HEADER + sizeof(segment_header_t);
    memcpy(cmd->data, header, sizeof(segment_header_t));//XXX overflow check

    return 0;
}

int bl_bootrom_cmd_sectionheader_get_res(bootrom_res_sectionheader_load_t *bootresponse)
{
    if ('O' == bootresponse->status[0] &&
        'K' == bootresponse->status[1]) {
        printk("[RSP] section_header response OK\n");
        return 0;
    } else if ('F' == bootresponse->status[0] &&
        'L' == bootresponse->status[1]) {
        printk("[RSP] section_header response failed, staus 0x%04X\n", bootresponse->len);
    } else {
        printk("[RSP] unkown status:%c%c\n", 
            bootresponse->status[0],
            bootresponse->status[1]
        );
    }
    return -1;
}

int bl_bootrom_cmd_sectiondata_load(bootrom_host_cmd_t *cmd, const uint8_t *data, int len)
{
    cmd->id = BL_BOOTROM_HOST_CMD_SECTIONDATA_LOAD;
    cmd->len = sizeof(bootrom_host_cmd_t) - BL_BOOTROM_HOST_CMD_LEN_HEADER + len;//use magic number
    memcpy(cmd->data, data, len);//XXX overflow check

    return 0;
}

int bl_bootrom_cmd_sectiondata_get_res(bootrom_res_sectiondata_load_t *bootresponse)
{
    if ('O' == bootresponse->status[0] &&
        'K' == bootresponse->status[1]) {
        printk("[RSP] section_data response OK\n");
    } else if ('F' == bootresponse->status[0] &&
        'L' == bootresponse->status[1]) {
        printk("[RSP] section_data response failed, staus 0x%04X\n", bootresponse->code);
    } else {
        printk("[RSP] unkown status:%c%c\n", 
            bootresponse->status[0],
            bootresponse->status[1]
        );
        return -1;
    }
    return -1;
}

int bl_bootrom_cmd_checkimage_get(bootrom_host_cmd_t *cmd)
{
    cmd->id = BL_BOOTROM_HOST_CMD_CHECK_IMAGE;
    cmd->len = 0;
    return 0;
}

int bl_bootrom_cmd_checkimage_get_res(bootrom_res_checkimage_t *checkimage)
{
    if ('O' == checkimage->status[0] &&
        'K' == checkimage->status[1]) {
        printk("[RSP] checkimage response OK\n");
        return 0;
    } else if ('F' == checkimage->status[0] &&
        'L' == checkimage->status[1]) {
        printk("[RSP] checkimage response failed, staus 0x%04X\n", checkimage->code);
    } else {
        printk("[RSP] unkown status:%c%c\n", 
            checkimage->status[0],
            checkimage->status[1]
        );
    }
    return -1;
}

int bl_bootrom_cmd_runimage_get(bootrom_host_cmd_t *cmd)
{
    cmd->id = BL_BOOTROM_HOST_CMD_RUN;
    cmd->len = 0;
    return 0;
}

int bl_bootrom_cmd_runimage_get_res(bootrom_res_runimage_t *runimage)
{
    if ('O' == runimage->status[0] &&
        'K' == runimage->status[1]) {
        printk("[RSP] runimage response OK\n");
    } else if ('F' == runimage->status[0] &&
        'L' == runimage->status[1]) {
        printk("[RSP] runimage response failed, staus 0x%04X\n", runimage->code);
    } else {
        printk("[RSP] unkown status:%c%c\n", 
            runimage->status[0],
            runimage->status[1]
        );
        return -1;
    }
    return 0;
}

int bl_bootrom_cmd_run(bootrom_host_cmd_t *cmd)
{
    cmd->id = BL_BOOTROM_HOST_CMD_RUN;
    cmd->len = 0;
    return 0;
}

int bl_bootrom_cmd_run_get_res(bootrom_res_run_t *bootresponse, int size)
{
    if (size != (sizeof(bootrom_res_sectiondata_load_t)) && size != 1) {
        printk("[RSP] section_data response len is not correct %d\n", size);
        return -1;
    }

    if ('O' == bootresponse->status[0] &&
        'K' == bootresponse->status[1] && 1 == size) {
        printk("[RSP] section_data response OK\n");
    } else if ('F' == bootresponse->status[0] &&
        'L' == bootresponse->status[1] && 3 == size) {
        printk("[RSP] section_data response failed, staus 0x%04X\n", bootresponse->code);
    } else {
        printk("[RSP] unkown status:%c%c\n", 
            bootresponse->status[0],
            bootresponse->status[1]
        );
        return -1;
    }
    return -1;
}

