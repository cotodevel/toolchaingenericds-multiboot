/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for Petit FatFs (C)ChaN, 2014      */
/*-----------------------------------------------------------------------*/

#include "pff.h"
#include "diskio_petit.h"
#include "dldi.h"
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>

/*--------------------------------------------------------------------------

   Public Functions

---------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/
__attribute__ ((optnone))
DSTATUS disk_initialize (void){
	
	if(1 == 1){
		return 0;
	}
	return FR_DISK_ERR;
}



/*-----------------------------------------------------------------------*/
/* Read Partial Sector                                                   */
/*-----------------------------------------------------------------------*/
unsigned char scratchPadSector[512];
__attribute__ ((optnone))
DRESULT disk_readp (
	BYTE* buff,		/* Pointer to the destination object */
	DWORD sector,	/* Sector number (LBA) */
	UINT offset,	/* Offset in the sector */
	UINT count		/* Byte count (bit15:destination) */
)
{
	if(dldi_handler_read_sectors(sector, 1, (char*)&scratchPadSector[0]) == true){
		memcpy(buff, (char*)&scratchPadSector[offset], count);
		return RES_OK;
	}
	return RES_ERROR;
}



/*-----------------------------------------------------------------------*/
/* Write Partial Sector                                                  */
/*-----------------------------------------------------------------------*/
/*
DRESULT disk_writep (
	const BYTE* buff,	// Pointer to the data to be written, NULL:Initiate/Finalize write operation 
	DWORD sc		// Sector number (LBA) or Number of bytes to send 
)
{
	DRESULT res;


	if (!buff) {
		if (sc) {

			// Initiate write process

		} else {

			// Finalize write process

		}
	} else {

		// Send data to the disk

	}

	return res;
}
*/
