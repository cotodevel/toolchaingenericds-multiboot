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
#include "fileBrowse.h"
#include <stdio.h>
#include "biosTGDS.h"
#include "global_settings.h"
#include "posixHandleTGDS.h"
#include "TGDSMemoryAllocator.h"

bool GDBEnabled = false;
char curChosenBrowseFile[MAX_TGDSFILENAME_LENGTH+1];

//Back to loader, based on Whitelisted DLDI names
static char curLoaderNameFromDldiString[MAX_TGDSFILENAME_LENGTH+1];
static inline char * canGoBackToLoader(){
	char * dldiName = dldi_tryingInterface();
	if(dldiName != NULL){
		if(strcmp(dldiName, "R4iDSN") == 0){	//R4iGold loader
			strcpy(curLoaderNameFromDldiString, "0:/_DS_MENU.dat");
			return (char*)&curLoaderNameFromDldiString[0];
		}
	}
	return NULL;
}

void menuShow(){
	clrscr();
	printf("                              ");
	printf("ToolchainGenericDS-multiboot:");
	printf("                              ");
	printf("Button (Start): File browser ");
	printf("    Button (A) Load TGDS/devkitARM NDS Binary. ");
	printf("                              ");
	printf("(Select): back to Loader. >%d", TGDSPrintfColor_Green);
	printf("Available heap memory: %d", getMaxRam());
	printf("Select: this menu");
	printarm7DebugBuffer();
}

static inline void initNDSLoader(){
	coherent_user_range_by_size((uint32)NDS_LOADER_DLDISECTION_CACHED, (48*1024) + (96*1024) + (64*1024) + (16*1024));
	dmaFillHalfWord(3, 0, (uint32)NDS_LOADER_DLDISECTION_CACHED, (48*1024) + (96*1024) + (64*1024) + (16*1024));
	
	//copy loader code (arm7bootldr.bin) to ARM7's EWRAM portion while preventing Cache issues
	coherent_user_range_by_size((uint32)&arm7bootldr[0], (int)arm7bootldr_size);					
	memcpy ((void *)NDS_LOADER_IPC_BOOTSTUBARM7_CACHED, (u32*)&arm7bootldr[0], arm7bootldr_size); 	//memcpy ( void * destination, const void * source, size_t num );	//memset(void *str, int c, size_t n)

	setNDSLoaderInitStatus(NDSLOADER_INIT_OK);
}

static u8 * outBuf7 = NULL;
static u8 * outBuf9 = NULL;

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
		
		//Get TGDS file handle
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
		
		//#define DLDI_CREATE_ARM7_ARM9_BIN
		
		printf("NDSLoader start. ");
		
		#ifdef DLDI_CREATE_ARM7_ARM9_BIN
		FILE * fout7 = fopen("0:/arm7.bin", "w+");
		FILE * fout9 = fopen("0:/arm9.bin", "w+");
		#endif
		
		u8 * outBuf = (u8 *)malloc(sectorSize * sectorsPerCluster);
		
		//Uncached to prevent cache issues right at once
		outBuf7 = (u8 *)(NDS_LOADER_IPC_ARM7BIN_UNCACHED);	//will not be higher than: arm7BootCodeSize
		outBuf9 = (u8 *)(NDS_LOADER_IPC_CTX_UNCACHED->arm9EntryAddress | 0x400000); //will not be higher than: arm9BootCodeSize or 0x2D0000 (2,949,120 bytes)
		
		u8 * outBuf7Seek = outBuf7;
		u8 * outBuf9Seek = outBuf9;
		
		int globalPtr = 0; //this one maps the entire file in 512 bytes (sectorSize)
		u32 cur_clustersector = NDS_LOADER_IPC_CTX_UNCACHED->sectorTableBootCode[0];
		uint32_t data_max = (uint32_t)(fileSize);
		uint32_t data_read = 0;
		while((cur_clustersector != 0xFFFFFFFF) && ((data_read * (sectorSize * sectorsPerCluster)) < data_max) )
		{
			//full sector copy
			memset(outBuf, 0, sectorSize * sectorsPerCluster);
			
			dldi_handler_read_sectors(cur_clustersector, sectorsPerCluster, (void*)(outBuf));
			
			//for each sector per cluster...
			int i = 0;
			for(i = 0; i < sectorsPerCluster; i++){
				
				//copy it into output ARM7 binary
				if ( (globalPtr >= arm7BootCodeOffsetInFile) && (globalPtr < (arm7BootCodeOffsetInFile+arm7BootCodeSize)) ){
					//last part?
					if( ((arm7BootCodeOffsetInFile+arm7BootCodeSize) - globalPtr) > sectorOffsetEnd7){
						memcpy (outBuf7Seek, outBuf + (i*512), 512);	//fwrite(outBuf + (i*512) , 1, 512, fout7);
						outBuf7Seek+=512;
					}
					else{
						memcpy (outBuf7Seek, outBuf + (i*512), sectorOffsetEnd7);	//fwrite(outBuf + (i*512) , 1, sectorOffsetEnd7, fout7);
						outBuf7Seek+=sectorOffsetEnd7;
					}
				}
				
				//copy it into output ARM9 binary
				if ( (globalPtr >= arm9BootCodeOffsetInFile) && (globalPtr < (arm9BootCodeOffsetInFile+arm9BootCodeSize)) ){
					//last part?
					if( ((arm9BootCodeOffsetInFile+arm9BootCodeSize) - globalPtr) > sectorOffsetEnd9){
						memcpy (outBuf9Seek, outBuf + (i*512), 512);	//fwrite(outBuf + (i*512) , 1, 512, fout9);
						outBuf9Seek+=512;
					}
					else{
						memcpy (outBuf9Seek, outBuf + (i*512), sectorOffsetEnd9);	//fwrite(outBuf + (i*512) , 1, sectorOffsetEnd9, fout9);
						outBuf9Seek+=sectorOffsetEnd9;
					}
				}	
				globalPtr +=512;
			}
			
			//ARM7 Range check
			data_read++;
			cur_clustersector = (u32)NDS_LOADER_IPC_CTX_UNCACHED->sectorTableBootCode[data_read];
		}
		
		#ifdef DLDI_CREATE_ARM7_ARM9_BIN
		//Write all at once
		int written7 = fwrite(outBuf7 , 1, arm7BootCodeSize, fout7);
		printf("written: ARM7 %d bytes. [Addr: %x]", written7, outBuf7);
		
		int written9 = fwrite(outBuf9 , 1, arm9BootCodeSize, fout9);
		printf("written: ARM9 %d bytes. [Addr: %x]", written9, outBuf9);
		#endif
		
		#ifndef DLDI_CREATE_ARM7_ARM9_BIN
		printf("ARM7 %d bytes. [Addr: %x]", arm7BootCodeSize, (outBuf7 - 0x400000));
		printf("ARM9 %d bytes. [Addr: %x]", arm9BootCodeSize, (outBuf9 - 0x400000));
		#endif
		
		#ifdef DLDI_CREATE_ARM7_ARM9_BIN
		fclose(fout9);
		fclose(fout7);
		#endif
		
		//Build NDS Header
		memcpy((u8*)0x027FFE00, NDSHeader, (headerSize*sizeof(u8)) );
		
		printf("NDSLoader end. ");
		
		free(outBuf);
		free(NDSHeader);
		fclose(fh);
		int ret=FS_deinit();
		
		//Copy and relocate current TGDS DLDI section into target ARM9 binary
		bool stat = dldiPatchLoader((data_t *)NDS_LOADER_IPC_CTX_UNCACHED->arm9EntryAddress, (u32)arm9BootCodeSize, (u32)&_dldi_start);
		if(stat == false){
			printf("DLDI Patch failed. APP does not support DLDI format.");
		}
		
		asm("mcr	p15, 0, r0, c7, c10, 4");
		flush_icache_all();
		flush_dcache_all();
		
		WRAM_CR = WRAM_0KARM9_32KARM7;	//96K ARM7 : 0x037f8000 ~ 0x03810000
		
		runBootstrapARM7();	//ARM9 Side						/	
		setNDSLoaderInitStatus(NDSLOADER_LOAD_OK);	//		|	Wait until ARM7.bin is copied back to IWRAM's target address
		waitWhileNotSetStatus(NDSLOADER_START);		//		\
		
		//reload ARM9.bin
		reloadARMCore(NDS_LOADER_IPC_CTX_UNCACHED->arm9EntryAddress);
	}
	return false;
}

int main(int argc, char argv[argvItems][MAX_TGDSFILENAME_LENGTH]) {
	
	/*			TGDS 1.5 Standard ARM9 Init code start	*/
	bool isTGDSCustomConsole = false;	//set default console or custom console: default console
	GUI_init(isTGDSCustomConsole);
	GUI_clear();
	setTGDSMemoryAllocator(getProjectSpecificMemoryAllocatorSetup());
	sint32 fwlanguage = (sint32)getLanguage();
	
	printf("     ");
	printf("     ");
	
	#ifdef ARM7_DLDI
	setDLDIARM7Address((u32 *)TGDSDLDI_ARM7_ADDRESS);	//Required by ARM7DLDI!
	#endif
	
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
	asm("mcr	p15, 0, r0, c7, c10, 4");
	flush_icache_all();
	flush_dcache_all();
	/*			TGDS 1.5 Standard ARM9 Init code end	*/
	
	initNDSLoader();	//set up NDSLoader
	
	//load TGDS Logo (NDS BMP Image)
	//VRAM A Used by console
	//VRAM C Keyboard and/or TGDS Logo
	
	//Show logo
	RenderTGDSLogoMainEngine((uint8*)&TGDSLogoLZSSCompressed[0], TGDSLogoLZSSCompressed_size);
	
	//Remove logo and restore Main Engine registers
	//restoreFBModeMainEngine();
	
	menuShow();
	
	while (1){
		scanKeys();
		
		if (keysPressed() & KEY_START){
			char startPath[MAX_TGDSFILENAME_LENGTH+1];
			strcpy(startPath,"/");
			while( ShowBrowser((char *)startPath, (char *)&curChosenBrowseFile[0]) == true ){	//as long you keep using directories ShowBrowser will be true
				
			}
			
			scanKeys();
			while((keysPressed() & KEY_A) || (keysPressed() & KEY_START)){
				scanKeys();
			}
			
			//Send args
			printf("[Booting %s]", curChosenBrowseFile);
			printf("Want to send argument?");
			printf("(A) Yes: (Start) Choose arg.");
			printf("(B) No. ");
			
			while(1==1){
				scanKeys();
				if(keysPressed()&KEY_A){
					scanKeys();
					while(keysPressed() & KEY_A){
						scanKeys();
					}
					
					char argv0[MAX_TGDSFILENAME_LENGTH+1];
					memset(argv0, 0, sizeof(argv0));
					
					while( ShowBrowser((char *)startPath, (char *)&argv0[0]) == true ){	//as long you keep using directories ShowBrowser will be true
						
					}
					
					char thisArgv[3][MAX_TGDSFILENAME_LENGTH];
					memset(thisArgv, 0, sizeof(thisArgv));
					strcpy(&thisArgv[0][0], curChosenBrowseFile);	//Arg0:	NDS Binary loaded
					strcpy(&thisArgv[1][0], argv0);					//Arg1: ARGV0
					addARGV(2, (char*)&thisArgv);
					break;
				}
				else if(keysPressed()&KEY_B){
					
					char thisArgv[3][MAX_TGDSFILENAME_LENGTH];
					memset(thisArgv, 0, sizeof(thisArgv));
					strcpy(&thisArgv[0][0], "");	//Arg0:	
					strcpy(&thisArgv[1][0], "");	//Arg1: 
					strcpy(&thisArgv[2][0], "");	//Arg2:
					addARGV(3, (char*)&thisArgv);
					break;
				}
			}
			
			fillNDSLoaderContext(curChosenBrowseFile);
		}
		
		if (keysPressed() & KEY_SELECT){
			char * loaderName = canGoBackToLoader();
			if(loaderName != NULL){
				fillNDSLoaderContext(loaderName);
			}
			else{
				clrscr();
				printf("--");
				printf("Dldi name: %s isn't registered.", dldi_tryingInterface());
				printf("Press B to exit.");
				scanKeys();
				while(1==1){
					scanKeys();
					if(keysPressed()&KEY_B){
						break;
					}
				}
			}
		}
		
		
		
		//GDB Debugging start
		//#ifdef NDSGDB_DEBUG_ENABLE
		if(GDBEnabled == true){
			//GDB Stub Process must run here
			int retGDBVal = remoteStubMain();
			if(retGDBVal == remoteStubMainWIFINotConnected){
				if (switch_dswnifi_mode(dswifi_gdbstubmode) == true){
					//clrscr();
					//Show IP and port here
					//printf("    ");
					//printf("    ");
					printf("[Connect to GDB]: NDSMemory Mode!");
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
				//clrscr();
				//printf("    ");
				//printf("    ");
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
					main(0, (sint8**)"");
				}
				
				if (switch_dswnifi_mode(dswifi_gdbstubmode) == true){ // gdbNdsStart() called
					reconnectCount++;
					clrscr();
					//Show IP and port here
					printf("    ");
					printf("    ");
					printf("[Re-Connect to GDB]: NDSMemory Mode!");
					char IP[16];
					printf("Retries: %d",reconnectCount);
					printf("Port:%d GDB IP:%s", remotePort, print_ip((uint32)Wifi_GetIP(), IP));
					remoteInit();
				}
			}
			
			//GDB Debugging end
			//#endif
		}	
		
		handleARM9SVC();	/* Do not remove, handles TGDS services */
		IRQVBlankWait();
	}
}

