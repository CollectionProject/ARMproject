/******************** (C) COPYRIGHT 2009 www.armjishu.com ****************/
/* Low level disk I/O module skeleton for FatFs              2007        */
/* Author             : www.armjishu.com Team                            */
/* This is a stub disk I/O module that acts as front end of the existing */
/* disk I/O modules and attach it to FatFs module with common interface. */
/*-----------------------------------------------------------------------*/
#include <string.h>
#include "diskio.h"
#include "sdcard.h"

/*-----------------------------------------------------------------------*/
/* Correspondence between physical drive number and physical drive.      */
/* Note that Tiny-FatFs supports only single drive and always            */
/* accesses drive number 0.                                              */

#define SECTOR_SIZE 512U

u32 buff2[512/4];
/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */

DSTATUS disk_initialize (
	BYTE drv				/* Physical drive nmuber (0..) */
)
{
	return 0;
}



/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */

DSTATUS disk_status (
	BYTE drv		/* Physical drive nmuber (0..) */
)
{	
	return 0;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */

DRESULT disk_read (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	BYTE count		/* Number of sectors to read (1..255) */
)
{
  //memset(buff2, 0, sizeof(buff2));
	if(count==1)
        {
          SD_ReadBlock((uint8_t*)buff2, (uint32_t)(sector << 9) ,SECTOR_SIZE);
          memcpy(buff,buff2,SECTOR_SIZE);
	}
	else
        {
          SD_ReadMultiBlocks((uint8_t*)buff2,(uint32_t)(sector << 9),SECTOR_SIZE,count);
          memcpy(buff,buff2,SECTOR_SIZE * count);
	}


	return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */

#if _READONLY == 0
DRESULT disk_write (
	BYTE drv,			/* Physical drive nmuber (0..) */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address (LBA) */
	BYTE count			/* Number of sectors to write (1..255) */
)
{
  //memset(buff2, 0, sizeof(buff2));
	if(count==1)
        {
          memcpy(buff2,buff,SECTOR_SIZE);
          SD_WriteBlock((uint8_t*)buff2, (uint32_t)(sector << 9) ,SECTOR_SIZE);
	}
	else
        {
          memcpy(buff2,buff,SECTOR_SIZE * count);
          SD_WriteMultiBlocks((uint8_t*)buff2,sector << 9 ,SECTOR_SIZE,count);
	}
        
  return RES_OK;
}
#endif /* _READONLY */



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */

DRESULT disk_ioctl (
	BYTE drv,		/* Physical drive nmuber (0..) */
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{

	return RES_OK;
}

DWORD get_fattime(void){
	return 0;
}























/******************* (C) COPYRIGHT 2009 www.armjishu.com *****END OF FILE****/
