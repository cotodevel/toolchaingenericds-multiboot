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
#include "dsregs.h"
#include "dsregs_asm.h"
#include "typedefsTGDS.h"
#include "gui_console_connector.h"
#include "dswnifi_lib.h"
#include "fileBrowse.hpp"	//generic template functions from TGDS: maintain 1 source, whose changes are globally accepted by all TGDS Projects.
#include "TGDSLogoLZSSCompressed.h"
#include "ipcfifoTGDSUser.h"
#include "fatfslayerTGDS.h"
#include "cartHeader.h"
#include "ff.h"
#include "dldi.h"
#include "loader.h"
#include "dmaTGDS.h"
#include "arm7bootldr.h"
#include "nds_cp15_misc.h"
#include <stdio.h>

char curChosenBrowseFile[MAX_TGDSFILENAME_LENGTH+1];

void menuShow(){
	clrscr();
	printf("                              ");
	printf("Start: File browser + load NDS");
	printf("Select: this menu");
}



static inline void initNDSLoader(){
	if(getNDSLoaderInitStatus() == NDSLOADER_INIT_WAIT){
		coherent_user_range_by_size((uint32)0x023DC000, (int)0x24000);
		dmaFillHalfWord(3, 0, (uint32)0x023DC000, (uint32)0x24000);	//setNDSLoaderInitStatus(NDSLOADER_INIT_WAIT);  (0);
		
		
		//copy loader code (arm7bootldr.bin) to ARM7's EWRAM portion while preventing Cache issues
		coherent_user_range_by_size((uint32)&arm7bootldr[0], (int)arm7bootldr_size);					
		memcpy ((void *)NDS_LOADER_IPC_HIGHCODEARM7_CACHED, (u32*)&arm7bootldr[0], arm7bootldr_size); 	//memcpy ( void * destination, const void * source, size_t num );	//memset(void *str, int c, size_t n)
		
		setNDSLoaderInitStatus(NDSLOADER_INIT_OK);
	}
}

//generates a table of sectors out of a given file. It has the ARM7 binary and ARM9 binary
bool fillNDSLoaderContext(char * filename){
	
	FILE * fh = fopen(filename, "r");
	if(fh != NULL){
		
		int headerSize = sizeof(struct sDSCARTHEADER);
		u8 * NDSHeader = (u8 *)malloc(headerSize*sizeof(u8));
		if (fread(NDSHeader, 1, headerSize, fh) != headerSize){
			printf("header read error");
			free(NDSHeader);
			fclose(fh);
			return false;
		}
		printf("header parsed correctly.");
		struct sDSCARTHEADER * NDSHdr = (struct sDSCARTHEADER *)NDSHeader;
		
		//fill data into shared buffers
		int sectorSize = 0;
		if(FF_MAX_SS == FF_MIN_SS){
			sectorSize = (int)FF_MIN_SS;
		}
		else{
			#if (FF_MAX_SS != FF_MIN_SS)
			sectorSize = dldiFs.ssize;	//when dldiFs sectorsize is variable, it's 512 bytes per sector
			#endif
		}
		
		//Common
		int clusterSizeBytes = getDiskClusterSizeBytes();
		int sectorsPerCluster = dldiFs.csize;
		NDS_LOADER_IPC_CTX_UNCACHED->sectorsPerCluster = sectorsPerCluster;
		NDS_LOADER_IPC_CTX_UNCACHED->sectorSize = sectorSize;
		//ARM7
		int arm7BootCodeSize = NDSHdr->arm7size;
		u32 arm7BootCodeOffsetInFile = NDSHdr->arm7romoffset;
		NDS_LOADER_IPC_CTX_UNCACHED->arm7EntryAddress = NDSHdr->arm7entryaddress;
		NDS_LOADER_IPC_CTX_UNCACHED->bootCode7FileSize = arm7BootCodeSize;
		int sectorOffsetStart7 = arm7BootCodeOffsetInFile % sectorSize;
		int sectorOffsetEnd7 = (arm7BootCodeOffsetInFile + arm7BootCodeSize - sectorOffsetStart7) % sectorSize;
		NDS_LOADER_IPC_CTX_UNCACHED->sectorOffsetStart7 = sectorOffsetStart7;
		NDS_LOADER_IPC_CTX_UNCACHED->sectorOffsetEnd7 = sectorOffsetEnd7;
		
		//ARM9
		int arm9BootCodeSize = NDSHdr->arm9size;
		u32 arm9BootCodeOffsetInFile = NDSHdr->arm9romoffset;
		NDS_LOADER_IPC_CTX_UNCACHED->arm9EntryAddress = NDSHdr->arm9entryaddress;
		NDS_LOADER_IPC_CTX_UNCACHED->bootCode9FileSize = arm9BootCodeSize;
		int sectorOffsetStart9 = arm9BootCodeOffsetInFile % sectorSize;
		int sectorOffsetEnd9 = (arm9BootCodeOffsetInFile + arm9BootCodeSize - sectorOffsetStart9) % sectorSize;
		NDS_LOADER_IPC_CTX_UNCACHED->sectorOffsetStart9 = sectorOffsetStart9;
		NDS_LOADER_IPC_CTX_UNCACHED->sectorOffsetEnd9 = sectorOffsetEnd9;
		
		clrscr();
		printf("-");
		printf("-");
		
		//get TGDS file handle
		int fdindex = fileno(fh);
		struct fd * fdinst = getStructFD(fdindex);
		
		//reset FP to make it sector-relative + get file size
		fseek(fh,0,SEEK_END);
		int fileSize = ftell(fh);
		fseek(fh,0,SEEK_SET);
		
		NDS_LOADER_IPC_CTX_UNCACHED->fileSize = fileSize;
		
		printf("fillNDSLoaderContext():");
		printf("arm7BootCodeSize:%d", arm7BootCodeSize);
		printf("arm7BootCodeOffsetInFile:%x", arm7BootCodeOffsetInFile);
		printf("arm7BootCodeEntryAddress:%x", NDS_LOADER_IPC_CTX_UNCACHED->arm7EntryAddress);
		printf("arm9BootCodeSize:%d", arm9BootCodeSize);
		printf("arm9BootCodeOffsetInFile:%x", arm9BootCodeOffsetInFile);
		printf("arm9BootCodeEntryAddress:%x", NDS_LOADER_IPC_CTX_UNCACHED->arm9EntryAddress);
		
		//Generate cluster filemap
		uint32_t* cluster_table = (uint32_t*)&NDS_LOADER_IPC_CTX_UNCACHED->sectorTableBootCode[0];
		uint32_t  cur_cluster = getStructFDFirstCluster(fdinst);
		while (cur_cluster >= 2 && cur_cluster != 0xFFFFFFFF)
		{
			*cluster_table = clst2sect(&dldiFs, cur_cluster);
			cluster_table++;
			cur_cluster = f_getFat(fdinst->filPtr, cur_cluster);
		}
		*(cluster_table) = 0xFFFFFFFF;
		
		
		//test code already implemented in loader.h
		/*
		//Test: read ARM7/ARM9 BootCode into .bin
		printf("test init.");
		
		//works fine, but starts from file start
		FILE * fout7 = fopen("0:/arm7.bin", "w+");
		FILE * fout9 = fopen("0:/arm9.bin", "w+");
		
		u8 * outBuf = (u8 *)malloc(sectorSize * sectorsPerCluster);
		
		int globalPtr = 0; //this one maps the entire file in 512 bytes (sectorSize)
		u32 cur_clustersector = NDS_LOADER_IPC_CTX_UNCACHED->sectorTableBootCode[0];
		uint32_t data_max = (uint32_t)(fileSize);
		uint32_t data_read = 0;
		while((cur_clustersector != 0xFFFFFFFF) && ((data_read * (sectorSize * sectorsPerCluster)) < data_max) )
		{
			//full sector copy
			memset(outBuf, 0, sectorSize * sectorsPerCluster);
			io_dldi_data->ioInterface.readSectors(cur_clustersector, sectorsPerCluster, (void*)(outBuf));
			
			//for each sector per cluster...
			for(int i = 0; i < sectorsPerCluster; i++){
				
				//copy it into output ARM7 binary
				if ( (globalPtr >= arm7BootCodeOffsetInFile) && (globalPtr < (arm7BootCodeOffsetInFile+arm7BootCodeSize)) ){
					//last part?
					if( ((arm7BootCodeOffsetInFile+arm7BootCodeSize) - globalPtr) > sectorOffsetEnd7){
						fwrite(outBuf + (i*512) , 1, 512, fout7);
					}
					else{
						fwrite(outBuf + (i*512) , 1, sectorOffsetEnd7, fout7);
					}
				}
				
				//copy it into output ARM9 binary
				if ( (globalPtr >= arm9BootCodeOffsetInFile) && (globalPtr < (arm9BootCodeOffsetInFile+arm9BootCodeSize)) ){
					//last part?
					if( ((arm9BootCodeOffsetInFile+arm9BootCodeSize) - globalPtr) > sectorOffsetEnd9){
						fwrite(outBuf + (i*512) , 1, 512, fout9);
					}
					else{
						fwrite(outBuf + (i*512) , 1, sectorOffsetEnd9, fout9);
					}
				}
				
				
				globalPtr +=512;
			}
			
			
			
			//ARM7 Range check
			data_read++;
			cur_clustersector = (u32)NDS_LOADER_IPC_CTX_UNCACHED->sectorTableBootCode[data_read];
		}
		
		fclose(fout9);
		fclose(fout7);
		printf("test end.");
		free(outBuf);
		//Test end
		*/
		
		//todo:
		//1) Enable DLDI at ARM7
		//2) then relocate Test code into ARM7's highcode section: 0x03800000 + 64K = 0x03810000 + 32K (highcode), and instead, write from the DLDI driver to ARM7'sarm7BootCodeEntryAddress and ARM9's arm9BootCodeEntryAddress
		//3) When 2) done, copy high code to EWRAM's end - 32K(NDSBinary ctx) - 32K(ARM7 highcode section) - 32K(ARM7 pagefile) from ARM9 to ARM7, ARM9 triggers ARM7 highcode reloading code (fifo irq), then ARM9 does 4)
		//4) ITCM Call: Let ARM9 jump, disable Interrupts, and wait loop for value in IPC region. When loop value is met, ARM9 triggers swi soft reset
		//5) Once ARM7 highcode is executed, write the wait loop value in IPC, disable Interrupts and ARM7 triggers swi soft reset
		
		free(NDSHeader);
		fclose(fh);
		
	}
	return false;
}

int main(int _argc, sint8 **_argv) {
	
	/*			TGDS 1.5 Standard ARM9 Init code start	*/
	bool project_specific_console = true;	//set default console or custom console: custom console
	GUI_init(project_specific_console);
	GUI_clear();
	
	sint32 fwlanguage = (sint32)getLanguage();
	
	printf("     ");
	printf("     ");
	
	int ret=FS_init();
	if (ret == 0)
	{
		printf("FS Init ok.");
	}
	else if(ret == -1)
	{
		printf("FS Init error.");
	}
	switch_dswnifi_mode(dswifi_idlemode);
	/*			TGDS 1.5 Standard ARM9 Init code end	*/
	
	initNDSLoader();	//set up NDSLoader
	
	//load TGDS Logo (NDS BMP Image)
	//VRAM A Used by console
	//VRAM C Keyboard and/or TGDS Logo
	
	//render TGDSLogo from a LZSS compressed file
	RenderTGDSLogoSubEngine((uint8*)&TGDSLogoLZSSCompressed[0], TGDSLogoLZSSCompressed_size);

	menuShow();
	
	while (1){
		scanKeys();
		
		if (keysPressed() & KEY_START){
			char startPath[MAX_TGDSFILENAME_LENGTH+1];
			strcpy(startPath,"/");
			while( ShowBrowser((char *)startPath, (char *)&curChosenBrowseFile[0]) == true ){	//as long you keep using directories ShowBrowser will be true
				
			}
			while(keysPressed() & KEY_START){
				scanKeys();
			}
			fillNDSLoaderContext(curChosenBrowseFile);
			boot_nds();
		}
		
		if (keysPressed() & KEY_SELECT){
			menuShow();
		}
		
		
		//GDB Debugging start
		#ifdef NDSGDB_DEBUG_ENABLE
		
		//GDB Stub Process must run here
		int retGDBVal = remoteStubMain();
		if(retGDBVal == remoteStubMainWIFINotConnected){
			if (switch_dswnifi_mode(dswifi_gdbstubmode) == true){
				clrscr();
				//Show IP and port here
				printf("    ");
				printf("    ");
				printf("[Connect to GDB]: %s", ((getValidGDBMapFile() == true) ? " GDBFile Mode!" : "NDSMemory Mode!"));
				char IP[16];
				printf("Port:%d GDB IP:%s",remotePort, print_ip((uint32)Wifi_GetIP(), IP));
				remoteInit();
			}
			else{
				//GDB Client Reconnect:ERROR
			}
		}
		else if(retGDBVal == remoteStubMainWIFIConnectedGDBDisconnected){
			setWIFISetup(false);
			clrscr();
			printf("    ");
			printf("    ");
			printf("Remote GDB Client disconnected. ");
			printf("Press A to retry this GDB Session. ");
			printf("Press B to reboot NDS GDB Server ");
			
			int keys = 0;
			while(1){
				scanKeys();
				keys = keysPressed();
				if (keys&KEY_A){
					break;
				}
				if (keys&KEY_B){
					break;
				}
				IRQVBlankWait();
			}
			
			if (keys&KEY_B){
				setValidGDBMapFile(false);
				main(0, (sint8**)"");
			}
			
			if (switch_dswnifi_mode(dswifi_gdbstubmode) == true){ // gdbNdsStart() called
				reconnectCount++;
				clrscr();
				//Show IP and port here
				printf("    ");
				printf("    ");
				printf("[Re-Connect to GDB]: %s",((getValidGDBMapFile() == true) ? " GDBFile Mode!" : "NDSMemory Mode!"));
				char IP[16];
				printf("Retries: %d",reconnectCount);
				printf("Port:%d GDB IP:%s", remotePort, print_ip((uint32)Wifi_GetIP(), IP));
				remoteInit();
			}
		}
		
		//GDB Debugging end
		#endif
		
		IRQVBlankWait();
	}
	
}

