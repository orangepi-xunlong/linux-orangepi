/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2014        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "diskio.h"		/* FatFs lower layer API */
#include <linux/module.h>
#include <linux/slab.h>

/* export func from kernel/driver/mmc/card/mmc_fatfs.c */
extern int mmc_fatfs_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count);
extern int mmc_fatfs_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count);
extern int mmc_fatfs_sectorcount(void);
extern int mmc_fatfs_cardstatus(void);

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	return 0;
}

static int g_fsize = 0;

void set_fsize(int size)
{
	g_fsize = size;
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{

	return 0;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address in LBA */
	UINT count		/* Number of sectors to read */
)
{
	return mmc_fatfs_read(pdrv,  buff, sector, count);
}


static int g_count = 0;
static DWORD g_sector = 0;
static char g_write_buff[BUFFER_SIZE];
static DWORD old_secotr;



int disk_write_flush(BYTE pdrv)
{
	int ret = 0;

	ret = mmc_fatfs_write(pdrv,  g_write_buff, g_sector, g_count);

	g_sector = 0;
	g_count = 0;
	return ret;
}
/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address in LBA */
	UINT count			/* Number of sectors to write */
)
{	
	DRESULT ret = 0 ;	
	if(count == 1)                
	{
		if(g_count !=0)
		{
			disk_write_flush(pdrv); //flush it 
		}
		ret = mmc_fatfs_write(pdrv,  buff, sector, count);
	}
	else
	{
		if(old_secotr + g_fsize == sector)
		{
			memcpy(g_write_buff + g_count*512, buff, count*512);
			g_count += count;

			if(g_sector == 0)
			{
				g_sector = sector;
			}
		}
		else
		{
			if(g_count !=0)
			{
				disk_write_flush(pdrv); //flush it 
			}

			memcpy(g_write_buff + g_count*512, buff, count*512);
			g_count += count;	
			if(g_sector == 0)
			{
				g_sector = sector;
			}			
		}
		
		if(g_count == 128)
		{
			disk_write_flush(pdrv); //flush it 
		}	
    }

	old_secotr = sector;
	
	return ret;
}
#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

#if _USE_IOCTL
DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res = RES_PARERR;

	switch (cmd) {
	case CTRL_SYNC:			/* Nothing to do */
		res = RES_OK;
		break;

	case GET_SECTOR_COUNT:	/* Get number of sectors on the drive */
		*(DWORD*)buff = mmc_fatfs_sectorcount();
		res = RES_OK;
		break;

	case GET_SECTOR_SIZE:	/* Get size of sector to read/write */
		*(WORD*)buff = 512;
		res = RES_OK;
		break;

	case GET_BLOCK_SIZE:	/* Get internal block size in unit of sector */
		*(DWORD*)buff = 128;
		res = RES_OK;
		break;

    case MMC_GET_SDSTAT:    /* Get the card's status(exist or not) */
        *(LONG*)buff = mmc_fatfs_cardstatus();
        res = RES_OK;
        break;

	default:
		break;

	}

	return res;
}
#endif
