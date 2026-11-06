#include <linux/types.h>
#include "aicwf_usb.h"

#define USB_DEVICE_ID_AIC_8800D80X2       0x8D90
#define USB_DEVICE_ID_AIC_8800D81X2       0x8D91
#define USB_DEVICE_ID_AIC_8800D89X2       0x8D99


#define FW_BASE_NAME_8800D80X2                "fmacfw_8800d80x2.bin"
#define FW_RF_BASE_NAME_8800D80X2             "lmacfw_rf_8800d80x2.bin"

#define FW_PATCH_BASE_NAME_8800D80X2_U03      "fw_patch_8800d80x2_u03.bin"
#define FW_PATCH_BASE_NAME_8800D80X2_U03_EXT  "fw_patch_8800d80x2_u03_ext"
#define FW_ADID_BASE_NAME_8800D80X2_U03       "fw_adid_8800d80x2_u03.bin"
#define FW_PATCH_TABLE_NAME_8800D80X2_U03     "fw_patch_table_8800d80x2_u03.bin"

#define FW_PATCH_BASE_NAME_8800D80X2_U05      "fw_patch_8800d80x2_u05.bin"
#define FW_PATCH_BASE_NAME_8800D80X2_U05_EXT  "fw_patch_8800d80x2_u05_ext"
#define FW_ADID_BASE_NAME_8800D80X2_U05       "fw_adid_8800d80x2_u05.bin"
#define FW_PATCH_TABLE_NAME_8800D80X2_U05     "fw_patch_table_8800d80x2_u05.bin"

#define FLASH_BIN_8800M80X2                   "host_wb_8800m80x2.bin"

#define FW_USERCONFIG_NAME_8800D80X2          "aic_userconfig_8800d80x2.txt"

#define RAM_FMAC_FW_ADDR_8800D80X2           0x120000
#define RAM_FMAC_RF_FW_ADDR_8800D80X2        0x120000

#define FW_RAM_ADID_BASE_ADDR_8800D80X2_U03  0x003018f8
#define FW_RAM_PATCH_BASE_ADDR_8800D80X2_U03 0x0030b494

#define FW_RAM_ADID_BASE_ADDR_8800D80X2_U05  0x003018f8
#define FW_RAM_PATCH_BASE_ADDR_8800D80X2_U05 0x0030b48c

#define FLASH_BIN_ADDR_8800M80X2             0x8000000


int aicwf_patch_config_8800d80x2(struct aic_usb_dev *usb_dev);
int rwnx_plat_userconfig_load_8800d80x2(struct aic_usb_dev *usbdev);
int system_config_8800d80x2(struct aic_usb_dev *usb_dev);
int aicfw_download_fw_8800d80x2(struct aic_usb_dev *usb_dev);

