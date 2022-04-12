#ifndef __BL_BOOTROOM_H__
#define __BL_BOOTROOM_H__
#include <linux/types.h>
#pragma pack(push, 1)

#define BFLB_BOOTROM_HASH_SIZE 			256/8
#define BFLB_BOOTROM_SIGN_MAXSIZE 		2048/8
#define BFLB_BOOTROM_ECC_KEYXSIZE 		256/8
#define BFLB_BOOTROM_ECC_KEYYSIZE 		256/8

typedef struct segment_header
{
	uint32_t destaddr;
	uint32_t len;
	uint32_t rsv;
	uint32_t crc32;
} segment_header_t;

typedef struct spi_Flash_Cfg_Tag
{
    uint8_t ioMode;
    uint8_t cReadSupport;
    uint8_t clk_delay;
    uint8_t rsvd[1];

    uint8_t resetEnCmd;
    uint8_t resetCmd;
    uint8_t resetCreadCmd;
    uint8_t rsvd_reset[1];

	uint8_t jedecIdCmd;         	        /* jedec id cmd */
	uint8_t jedecIdCmdDmyClk;
	uint8_t qpiJedecIdCmd;            
	uint8_t qpiJedecIdCmdDmyClk;         	            

	uint8_t sectorSize;			            /* *1024bytes */
	uint8_t capBase;                        /* 0x17 for 64Mbits,0x18 for 128Mbits, 0x19 for 256Mbits */
	uint16_t pageSize;    			        /* page size */

	uint8_t chipEraseCmd;			        /* chip erase cmd */ 
	uint8_t sectorEraseCmd;                 /* sector erase command */ 
	uint8_t blk32EraseCmd;                  /* block 32K erase command,some Micron not support */
	uint8_t blk64EraseCmd;                  /* block 64K erase command */

    uint8_t writeEnableCmd;		            /* need before every erase or program */
	uint8_t pageProgramCmd;                 /* page program cmd */
	uint8_t qpageProgramCmd;                /* qio page program cmd */
	uint8_t qppAddrMode;                    /* gd and winbond and Micron use one line for addr while microchip use four lines */

	uint8_t fastReadCmd; 			        /* fast read command */
	uint8_t frDmyClk;
	uint8_t qpiFastReadCmd; 			    /* qpi fast read command */
	uint8_t qpiFrDmyClk;

    uint8_t fastReadDoCmd; 			        /* fast read dual output command */
	uint8_t frDoDmyClk;
	uint8_t fastReadDioCmd;		            /* fast read dual io comamnd */
	uint8_t frDioDmyClk;

    uint8_t fastReadQoCmd;		            /* fast read quad output comamnd */
	uint8_t frQoDmyClk;
    uint8_t fastReadQioCmd;		            /* fast read quad io comamnd */
	uint8_t frQioDmyClk;

    uint8_t qpiFastReadQioCmd;              /* qpi fast read quad io comamnd */
    uint8_t qpiFrQioDmyClk;
	uint8_t qpiPageProgramCmd;		        /* qpi program command */
	uint8_t writeVregEnableCmd;             /* enable write reg */

	/*reg*/
	uint8_t wrEnableIndex;                  /* write enable bit index */
	uint8_t qeIndex;                        /* quad mode enable bit index */
	uint8_t busyIndex;                      /* busy status bit index */
	uint8_t wrEnableBit;

	uint8_t qeBit;
    uint8_t busyBit;                       /* gd and winbond need set this bit for SPI_QIO, it seems that Micron need not */
	uint8_t wrEnableWriteRegLen;
	uint8_t wrEnableReadRegLen;

	uint8_t qeWriteRegLen;
	uint8_t qeReadRegLen;
	uint8_t rsvd1;
	uint8_t busyReadRegLen;

	uint8_t readRegCmd[4];

	uint8_t writeRegCmd[4];

    uint8_t enterQpi;		                /* enter qpi command */
	uint8_t exitQpi;                        /* exit qpi command */
	uint8_t cReadMode;                      /* continuous read modo value */
    uint8_t cRExit;                    

    uint8_t burstWrapCmd;                   /* wrap around operation */
    uint8_t burstWrapCmdDmyClk;
    uint8_t burstWrapDataMode;
    uint8_t burstWrapData;

    uint8_t deBurstWrapCmd;                  /* disable wrap around operation */
    uint8_t deBurstWrapCmdDmyClk;
    uint8_t deBurstWrapDataMode;
    uint8_t deBurstWrapData;

    uint16_t timeEsector;                   /* 4K erase time */
	uint16_t timeE32k;                      /* 32K erase time */

	uint16_t timeE64k;                      /* 64K erase time */
	uint16_t timePagePgm;                   /* page program time */

	uint32_t timeCe;                        /* chip erase time */
}spi_Flash_Cfg;

typedef struct boot_flash_cfg
{
	uint32_t magiccode;       /*'FCFG'*/
	spi_Flash_Cfg cfg;
	uint32_t crc32;
} boot_flash_cfg_t;

typedef struct boot_pll_cfg_t
{
	uint32_t magiccode;       /*'PCFG'*/

	uint8_t root_clk;
	uint8_t xtal_type;
	uint8_t pll_clk;
	uint8_t hclk_div;

	uint8_t bclk_div;
	uint8_t flash_clk_div;
	uint8_t uart_clk_div;
	uint8_t sdu_clk_div;

	uint32_t crc32;
} boot_pll_cfg_t;

typedef struct pkey_cfg
{
	uint8_t eckeyx[BFLB_BOOTROM_ECC_KEYXSIZE];        //ec key in boot header
	uint8_t eckeyy[BFLB_BOOTROM_ECC_KEYYSIZE];        //ec key in boot header
	uint32_t crc32;
} pkey_cfg_t;

typedef struct sign_cfg
{
	uint32_t sig_len;
	uint8_t signature[0];
	uint32_t crc32; //crc32 append tail, NOT use this field
} sign_cfg_t;

typedef struct bootheader {
	uint32_t magiccode;       			/*'BFXP'*/
	uint32_t rivison;
	boot_flash_cfg_t flashCfg;
	boot_pll_cfg_t pllCfg;
	union {
		struct {
		  uint32_t sign               :  2;   /* [3: 2]      for sign */
		  uint32_t encrypt            :  2;   /* [1: 0]      for encrypt*/
		  uint32_t key_sel            :  2;   /* [5: 4]      for key sel in boot interface*/
		  uint32_t rsvd6_31           :  26;  /* [31:6]     rsvd */
		} bval;
		uint32_t wval;
	} seccfg ;
	uint32_t segment_cnt;
	uint32_t bootentry;      /* entry point of the image*/
	uint32_t flashoffset;

	uint8_t hash[BFLB_BOOTROM_HASH_SIZE];    /*hash of the image*/

	uint32_t rsv1;
	uint32_t rsv2;
	uint32_t crc32;
} bootheader_t;

typedef struct bootrom_host_cmd {
    uint8_t id;
    uint8_t reserved;
    uint16_t len;
    uint8_t data[0];
} bootrom_host_cmd_t;

typedef struct bootrom_host_cmd_password_load {
    uint8_t id;
    uint8_t reserved;
    uint16_t len;
    uint8_t data[8];
} bootrom_host_cmd_password_load_t;

typedef struct bootrom_host_cmd_jtag_open {
    uint8_t id;
    uint8_t reserved;
    uint16_t len;
    uint8_t data[8];
} bootrom_host_cmd_jtag_open_t;

typedef struct bootrom_res_bootinfo {
    uint8_t status[2];
    uint16_t len;
    uint32_t version;

    uint8_t password_mode;
    uint8_t sboot_enable;
    uint8_t jtag_disable;
    uint8_t uart_disable;

    uint8_t sign_type;
    uint8_t aes_type;
    uint8_t reserved;
    uint8_t chip_id[5];

} bootrom_res_bootinfo_t;

typedef struct bootrom_res_password_load {
    uint8_t status[2];
    uint16_t code;
} bootrom_res_password_load_t;

typedef struct bootrom_res_jtag_open {
    uint8_t status[2];
    uint16_t code;
} bootrom_res_jtag_open_t;

typedef struct bootrom_res_bootheader_load {
    uint8_t status[2];
    uint16_t code;
} bootrom_res_bootheader_load_t;

typedef struct bootrom_res_aesiv_load {
    uint8_t status[2];
    uint16_t code;
} bootrom_res_aesiv_load_t;

typedef struct bootrom_res_pkey_load {
    uint8_t status[2];
    uint16_t code;
} bootrom_res_pkey_load_t;

typedef struct bootrom_res_signature_load {
    uint8_t status[2];
    uint16_t code;
} bootrom_res_signature_load_t;

typedef struct bootrom_res_tzc_load {
    uint8_t status[2];
    uint16_t code;
} bootrom_res_tzc_load_t;

typedef struct bootrom_res_sectionheader_load {
    uint8_t status[2];
    uint16_t len;//reuse also as code. TODO: use union
    segment_header_t header;
} bootrom_res_sectionheader_load_t;

typedef struct bootrom_res_sectiondata_load {
    uint8_t status[2];
    uint16_t code;
} bootrom_res_sectiondata_load_t;

typedef struct bootrom_res_run {
    uint8_t status[2];
    uint16_t code;
} bootrom_res_run_t;

typedef struct bootrom_res_checkimage {
    uint8_t status[2];
    uint16_t code;
} bootrom_res_checkimage_t;

typedef struct bootrom_res_runimage {
    uint8_t status[2];
    uint16_t code;
} bootrom_res_runimage_t;
#define BL_BOOTROM_HOST_CMD_LEN_HEADER 4

#define BL_BOOTROM_HOST_CMD_BOOTINFO_GET 0x10
#define BL_BOOTROM_HOST_CMD_BOOTHEADER_LOAD 0x11
#define BL_BOOTROM_HOST_CMD_PK1_LOAD 0x12
#define BL_BOOTROM_HOST_CMD_PK2_LOAD 0x13
#define BL_BOOTROM_HOST_CMD_SIGNATURE1_LOAD 0x14
#define BL_BOOTROM_HOST_CMD_SIGNATURE2_LOAD 0x15
#define BL_BOOTROM_HOST_CMD_AESIV_LOAD 0x16
#define BL_BOOTROM_HOST_CMD_SECTIONHEADER_LOAD 0x17
#define BL_BOOTROM_HOST_CMD_SECTIONDATA_LOAD 0x18
#define BL_BOOTROM_HOST_CMD_CHECK_IMAGE 0x19
#define BL_BOOTROM_HOST_CMD_RUN 0x1A
#define BL_BOOTROM_HOST_CMD_CHANGE_RATE 0x20
#define BL_BOOTROM_HOST_CMD_RESET 0x21
#define BL_BOOTROM_HOST_CMD_FLASH_ERASE 0x30
#define BL_BOOTROM_HOST_CMD_FLASH_WRITE 0x31
#define BL_BOOTROM_HOST_CMD_FLASH_READ 0x32
#define BL_BOOTROM_HOST_CMD_FLASH_BOOT 0x33
#define BL_BOOTROM_HOST_CMD_EFUSE_WRITE 0x40
#define BL_BOOTROM_HOST_CMD_EFUSE_READ 0x41
int bl_bootrom_cmd_len(bootrom_host_cmd_t *cmd);
int bl_bootrom_cmd_bootinfo_get(bootrom_host_cmd_t *cmd);
int bl_bootrom_cmd_bootinfo_get_res(bootrom_res_bootinfo_t *bootinfo);
int bl_bootrom_cmd_bootheader_load(bootrom_host_cmd_t *cmd, bootheader_t *header);
int bl_bootrom_cmd_bootheader_load_get_res(bootrom_res_bootheader_load_t *bootresponse);
int bl_bootrom_cmd_aesiv_load(bootrom_host_cmd_t *cmd, const uint8_t *aesiv);
int bl_bootrom_cmd_aesiv_load_get_res(bootrom_res_aesiv_load_t *bootresponse);
int bl_bootrom_cmd_pkey1_load(bootrom_host_cmd_t *cmd, pkey_cfg_t *pk);
int bl_bootrom_cmd_pkey1_load_get_res(bootrom_res_pkey_load_t *bootresponse);
int bl_bootrom_cmd_pkey2_load(bootrom_host_cmd_t *cmd, pkey_cfg_t *pk);
int bl_bootrom_cmd_pkey2_load_get_res(bootrom_res_pkey_load_t *bootresponse);
int bl_bootrom_cmd_signature1_load(bootrom_host_cmd_t *cmd, uint8_t *signature, int len);
int bl_bootrom_cmd_signature1_get_res(bootrom_res_signature_load_t *bootresponse);
int bl_bootrom_cmd_signature2_load(bootrom_host_cmd_t *cmd, uint8_t *signature, int len);
int bl_bootrom_cmd_signature2_get_res(bootrom_res_signature_load_t *bootresponse);
int bl_bootrom_cmd_sectionheader_load(bootrom_host_cmd_t *cmd, const segment_header_t *header);
int bl_bootrom_cmd_sectionheader_get_res(bootrom_res_sectionheader_load_t *bootresponse);
int bl_bootrom_cmd_sectiondata_load(bootrom_host_cmd_t *cmd, const uint8_t *data, int len);
int bl_bootrom_cmd_sectiondata_get_res(bootrom_res_sectiondata_load_t *bootresponse);
int bl_bootrom_cmd_checkimage_get(bootrom_host_cmd_t *cmd);
int bl_bootrom_cmd_checkimage_get_res(bootrom_res_checkimage_t *checkimage);
int bl_bootrom_cmd_runimage_get(bootrom_host_cmd_t *cmd);
int bl_bootrom_cmd_runimage_get_res(bootrom_res_runimage_t *runimage);

#pragma pack(pop)
#endif
