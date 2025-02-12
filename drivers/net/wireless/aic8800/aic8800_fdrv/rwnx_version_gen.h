#define RWNX_VERS_REV    "241c091M (master)"
#define DRV_RELEASE_DATE "20240919"
#define DRV_PATCH_LEVEL  "004"
#define RWNX_VERS_MOD    DRV_RELEASE_DATE "-" DRV_PATCH_LEVEL "-6.4.3.0"
#define RWNX_VERS_BANNER "rwnx " RWNX_VERS_MOD " - - " RWNX_VERS_REV

#if defined(AICWF_SDIO_SUPPORT)
#define DRV_TYPE_NAME   "compatible(sdio)"
#elif defined(AICWF_USB_SUPPORT)
#define DRV_TYPE_NAME   "compatible(usb)"
#else
#define DRV_TYPE_NAME   "compatible(unknow)"
#endif

#define DRV_RELEASE_TAG "aic-rwnx-" DRV_TYPE_NAME "-" DRV_RELEASE_DATE "-" DRV_PATCH_LEVEL
