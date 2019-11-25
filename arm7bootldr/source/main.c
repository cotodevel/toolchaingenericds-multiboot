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

#include "main.h"
#include "biosTGDS.h"
#include "loader.h"

static inline void enterGDBFromARM7(void){
	SendFIFOWords(NDSLOADER_ENTERGDB_FROM_ARM7, 0);
}

//this code is targeted to ARMv4, but it is compiled by ARM9 side, and also linked into ARM9 binary. Later by the NDSLoader, it will be moved over ARM7
static inline void ARM7ExecuteNDSLoader(void){

	int highcodeEnabled = getNDSLoaderInitStatus();
	if(highcodeEnabled == NDSLOADER_LOAD_OK){
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
						memcpy (entryAddr7 + globalPtr, outBuf, 512); //fwrite(outBuf + (i*512) , 1, 512, fout7);	//memcpy ( void * destination, const void * source, size_t num );
					}
					else{
						memcpy (entryAddr7 + globalPtr, outBuf, sectorOffsetEnd7); //fwrite(outBuf + (i*512) , 1, sectorOffsetEnd7, fout7);
					}
				}
				
				//copy it into output ARM9 binary
				if ( (globalPtr >= arm9BootCodeOffsetInFile) && (globalPtr < (arm9BootCodeOffsetInFile+arm9BootCodeSize)) ){
					//last part?
					if( ((arm9BootCodeOffsetInFile+arm9BootCodeSize) - globalPtr) > sectorOffsetEnd9){
						//Todo arm7 first. //fwrite(outBuf + (i*512) , 1, 512, fout9);
					}
					else{
						//Todo arm7 first. //memcpy (entryAddr9 + (data_read * (sectorSize * i)), outBuf, sectorOffsetEnd9);	//fwrite(outBuf + (i*512) , 1, sectorOffsetEnd9, fout9);
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

int main(int _argc, sint8 **_argv) {
	/*			TGDS 1.5 Standard ARM7 Init code start	*/
	installWifiFIFO();		
	/*			TGDS 1.5 Standard ARM7 Init code end	*/
	
	//init dldi, must work properly
	//redirect DLDI calls here
	//then...
	
	//this bootstub will proceed only when file has been loaded properly
	while(getNDSLoaderInitStatus() != NDSLOADER_LOAD_OK){
	
	}
	
	SendFIFOWords(0xff11ff22, 0);	//if we get a signal out of this, means bootcode works!
	
	ARM7ExecuteNDSLoader();
	
	//enterGDBFromARM7();	//debug
	
    while (1) {
		
		scanKeys();
		
		if (keysPressed() & KEY_DOWN){
			SendFIFOWords(0xff11ff44, 0);	//printf("ARM7 ALIVE!");
		}
		
		handleARM7SVC();	/* Do not remove, handles TGDS services */
		IRQVBlankWait();
	}
   
	return 0;
}
