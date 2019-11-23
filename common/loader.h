/*

			Copyright (C) 2017  Coto
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
USA

*/


#ifndef __loader_common_h__
#define __loader_common_h__

#include "dsregs.h"
#include "dsregs_asm.h"
#include "ipcfifoTGDS.h"
#include "dswnifi.h"
#include "utilsTGDS.h"

#define ARM7COMMAND_RELOADNDS (uint32)(0xFFFFFF01)

struct ndsloader_s
{
	//raw NDS Binary
	u32 sectorTableBootCode[(4 * 1024 * 1024)/512];	//32K bytes in sectors = 4MB file addressed (NDS max binary size)
	int fileSize;
	
	//ARM7 BootCode
	int bootCode7FileSize;
	u32 arm7EntryAddress;
	int sectorOffsetStart7;
	int sectorOffsetEnd7;
	u32 arm7BootCodeOffsetInFile;
	
	//ARM9 BootCode
	int bootCode9FileSize;
	u32 arm9EntryAddress;
	int sectorOffsetStart9;
	int sectorOffsetEnd9;
	u32 arm9BootCodeOffsetInFile;
	
	//SD specific
	int sectorsPerCluster;
	int sectorSize;
	bool enabled;
};

//Shared IPC : 

//NDSLoaderContext = EWRAM - 33K shared IPC (NDS_LOADER_IPC_CTX_SIZE)
#define NDS_LOADER_IPC_CTX_SIZE (33*1024)
#define NDS_LOADER_IPC_CTXADDR		(0x02400000 - (int)NDS_LOADER_IPC_CTX_SIZE)
#define NDS_LOADER_IPC_CTX_CACHED ((struct ndsloader_s*)NDS_LOADER_IPC_CTXADDR)
#define NDS_LOADER_IPC_CTX_UNCACHED ((struct ndsloader_s*)(NDS_LOADER_IPC_CTXADDR | 0x400000))

//ARM7 Highcode: ARM7 loader code
#define NDS_LOADER_IPC_HIGHCODEARM7_CACHED		(NDS_LOADER_IPC_CTXADDR - (32*1024))
#define NDS_LOADER_IPC_HIGHCODEARM7_UNCACHED 	(NDS_LOADER_IPC_HIGHCODEARM7_CACHED | 0x400000)

//ARM7 Pagefile: used as storage buffer (32K <- 512*64 == sectorSize * sectorsPerCluster)
#define NDS_LOADER_IPC_PAGEFILEARM7_CACHED		(NDS_LOADER_IPC_HIGHCODEARM7_CACHED - (32*1024))
#define NDS_LOADER_IPC_PAGEFILEARM7_UNCACHED 	(NDS_LOADER_IPC_PAGEFILEARM7_CACHED | 0x400000)

//this code is targeted to ARMv4, but it is compiled by ARM9 side, and also linked into ARM9 binary. Later by the NDSLoader, it will be moved over ARM7
#ifdef ARM7
__attribute__((section(".highcode")))
__attribute__((noinline)) static void ARM7ExecuteNDSLoader(void){
	
	bool highcodeEnabled = NDS_LOADER_IPC_CTX_UNCACHED->enabled;

	if(highcodeEnabled == true){
		//Common
		int sectorsPerCluster = NDS_LOADER_IPC_CTX_UNCACHED->sectorsPerCluster;
		int sectorSize = NDS_LOADER_IPC_CTX_UNCACHED->sectorSize;
		int fileSize = NDS_LOADER_IPC_CTX_UNCACHED->fileSize;
		
		//ARM7
		u32 arm7BootCodeOffsetInFile = NDS_LOADER_IPC_CTX_UNCACHED->arm7BootCodeOffsetInFile;
		int arm7BootCodeSize = NDS_LOADER_IPC_CTX_UNCACHED->bootCode7FileSize;
		int sectorOffsetStart7 = arm7BootCodeOffsetInFile % sectorSize;
		int sectorOffsetEnd7 = (arm7BootCodeOffsetInFile + arm7BootCodeSize - sectorOffsetStart7) % sectorSize;
		u32 arm7entryaddress = NDS_LOADER_IPC_CTX_UNCACHED->arm7EntryAddress;
		
		//ARM9
		u32 arm9BootCodeOffsetInFile = NDS_LOADER_IPC_CTX_UNCACHED->arm9BootCodeOffsetInFile;
		int arm9BootCodeSize = NDS_LOADER_IPC_CTX_UNCACHED->bootCode9FileSize;
		int sectorOffsetStart9 = arm9BootCodeOffsetInFile % sectorSize;
		int sectorOffsetEnd9 = (arm9BootCodeOffsetInFile + arm9BootCodeSize - sectorOffsetStart9) % sectorSize;
		u32 arm9entryaddress = NDS_LOADER_IPC_CTX_UNCACHED->arm9EntryAddress;
		
		//ARM7 Highcode loader code: ARM7/ARM9 BootCode setup
		u8 * outBuf = (u8 *)NDS_LOADER_IPC_PAGEFILEARM7_UNCACHED;
		u8 * entryAddr7 = (u8 *)arm7entryaddress;
		u8 * entryAddr9 = (u8 *)arm9entryaddress;
		
		int globalPtr = 0; //this one maps the entire file in 512 bytes (sectorSize)
		u32 cur_clustersector = NDS_LOADER_IPC_CTX_UNCACHED->sectorTableBootCode[0];
		uint32_t data_max = (uint32_t)(fileSize);
		uint32_t data_read = 0;
		while((cur_clustersector != 0xFFFFFFFF) && ((data_read * (sectorSize * sectorsPerCluster)) < data_max) )
		{
			//full sector copy
			memset(outBuf, 0, sectorSize * sectorsPerCluster);
			//io_dldi_data->ioInterface.readSectors(cur_clustersector, sectorsPerCluster, (void*)(outBuf));	//todo
			
			//for each sector per cluster...
			int i = 0;
			for(i = 0; i < sectorsPerCluster; i++){
				
				//copy it into output ARM7 binary
				if ( (globalPtr >= arm7BootCodeOffsetInFile) && (globalPtr < (arm7BootCodeOffsetInFile+arm7BootCodeSize)) ){
					//last part?
					if( ((arm7BootCodeOffsetInFile+arm7BootCodeSize) - globalPtr) > sectorOffsetEnd7){
						memcpy (entryAddr7 + (data_read * (sectorSize * i)), outBuf, 512); //fwrite(outBuf + (i*512) , 1, 512, fout7);	//memcpy ( void * destination, const void * source, size_t num );
						
					}
					else{
						memcpy (entryAddr7 + (data_read * (sectorSize * i)), outBuf, sectorOffsetEnd7); //fwrite(outBuf + (i*512) , 1, sectorOffsetEnd7, fout7);
					}
				}
				
				//copy it into output ARM9 binary
				if ( (globalPtr >= arm9BootCodeOffsetInFile) && (globalPtr < (arm9BootCodeOffsetInFile+arm9BootCodeSize)) ){
					//last part?
					if( ((arm9BootCodeOffsetInFile+arm9BootCodeSize) - globalPtr) > sectorOffsetEnd9){
						memcpy (entryAddr9 + (data_read * (sectorSize * i)), outBuf, 512);	//fwrite(outBuf + (i*512) , 1, 512, fout9);
					}
					else{
						memcpy (entryAddr9 + (data_read * (sectorSize * i)), outBuf, sectorOffsetEnd9);	//fwrite(outBuf + (i*512) , 1, sectorOffsetEnd9, fout9);
					}
				}
				
				
				globalPtr +=512;
			}
			
			//ARM7 Range check
			data_read++;
			cur_clustersector = (u32)NDS_LOADER_IPC_CTX_UNCACHED->sectorTableBootCode[data_read];
		}
	}
}

#endif


#endif


#ifdef __cplusplus
extern "C"{
#endif

extern void boot_nds(void);

#ifdef __cplusplus
}
#endif