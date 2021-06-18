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
#include "stage2_9.h"

bool stopSoundStreamUser(){
	return false;
}

void closeSoundUser(){
	//Stubbed. Gets called when closing an audiostream of a custom audio decoder
}

//generates a table of sectors out of a given file. It has the ARM7 binary and ARM9 binary
__attribute__((section(".itcm")))
bool ReloadNDSBinaryFromContext(char * filename) __attribute__ ((optnone)) {
	
	//Will reload stage 2 payload and while passing it the filename into ARGV[0] vector
	volatile u8 * outBuf7 = NULL;
	volatile u8 * outBuf9 = NULL;

	//copy loader code (arm7bootldr.bin) to ARM7's EWRAM portion while preventing Cache issues
	coherent_user_range_by_size((uint32)&arm7bootldr[0], (int)arm7bootldr_size);					
	memcpy ((void *)NDS_LOADER_IPC_BOOTSTUBARM7_CACHED, (u32*)&arm7bootldr[0], arm7bootldr_size); 	//memcpy ( void * destination, const void * source, size_t num );	//memset(void *str, int c, size_t n)
	
	int headerSize = sizeof(struct sDSCARTHEADER);
	u8 * NDSHeader = (u8 *)TGDSARM9Malloc(headerSize*sizeof(u8));
	memcpy(NDSHeader, (u8*)&stage2_9[0], headerSize);
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
	NDS_LOADER_IPC_CTX_UNCACHED_NTR->sectorsPerCluster = sectorsPerCluster;
	NDS_LOADER_IPC_CTX_UNCACHED_NTR->sectorSize = sectorSize;
	
	//ARM7
	int arm7BootCodeSize = NDSHdr->arm7size;
	u32 arm7BootCodeOffsetInFile = NDSHdr->arm7romoffset;
	NDS_LOADER_IPC_CTX_UNCACHED_NTR->arm7EntryAddress = NDSHdr->arm7entryaddress;
	NDS_LOADER_IPC_CTX_UNCACHED_NTR->bootCode7FileSize = arm7BootCodeSize;
	int sectorOffsetStart7 = arm7BootCodeOffsetInFile % sectorSize;
	int sectorOffsetEnd7 = (arm7BootCodeOffsetInFile + arm7BootCodeSize - sectorOffsetStart7) % sectorSize;
	NDS_LOADER_IPC_CTX_UNCACHED_NTR->sectorOffsetStart7 = sectorOffsetStart7;
	NDS_LOADER_IPC_CTX_UNCACHED_NTR->sectorOffsetEnd7 = sectorOffsetEnd7;
	
	//ARM9
	int arm9BootCodeSize = NDSHdr->arm9size;
	u32 arm9BootCodeOffsetInFile = NDSHdr->arm9romoffset;
	NDS_LOADER_IPC_CTX_UNCACHED_NTR->arm9EntryAddress = NDSHdr->arm9entryaddress;
	NDS_LOADER_IPC_CTX_UNCACHED_NTR->bootCode9FileSize = arm9BootCodeSize;
	int sectorOffsetStart9 = arm9BootCodeOffsetInFile % sectorSize;
	int sectorOffsetEnd9 = (arm9BootCodeOffsetInFile + arm9BootCodeSize - sectorOffsetStart9) % sectorSize;
	NDS_LOADER_IPC_CTX_UNCACHED_NTR->sectorOffsetStart9 = sectorOffsetStart9;
	NDS_LOADER_IPC_CTX_UNCACHED_NTR->sectorOffsetEnd9 = sectorOffsetEnd9;
	
	clrscr();
	printf("-");
	printf("-");
	
	//reset FP to make it sector-relative + get file size
	int fileSize = stage2_9_size;
	NDS_LOADER_IPC_CTX_UNCACHED_NTR->fileSize = fileSize;
	
	printf("ReloadNDSBinaryFromContext1():");
	printf("arm7BootCodeSize:%d", arm7BootCodeSize);
	printf("arm7BootCodeOffsetInFile:%x", arm7BootCodeOffsetInFile);
	printf("arm7BootCodeEntryAddress:%x", NDS_LOADER_IPC_CTX_UNCACHED_NTR->arm7EntryAddress);
	printf("arm9BootCodeSize:%d", arm9BootCodeSize);
	printf("arm9BootCodeOffsetInFile:%x", arm9BootCodeOffsetInFile);
	printf("arm9BootCodeEntryAddress:%x", NDS_LOADER_IPC_CTX_UNCACHED_NTR->arm9EntryAddress);
	
	printf("NDSLoader start. ");
	
	u8 * outBuf = (u8 *)TGDSARM9Malloc(sectorSize * sectorsPerCluster);
	
	//Uncached to prevent cache issues right at once
	outBuf7 = (u8 *)(NDS_LOADER_IPC_ARM7BIN_UNCACHED_NTR);	//will not be higher than: arm7BootCodeSize
	outBuf9 = (u8 *)(NDS_LOADER_IPC_CTX_UNCACHED_NTR->arm9EntryAddress | 0x400000); //will not be higher than: arm9BootCodeSize or 0x2D0000 (2,949,120 bytes)
	
	memcpy(outBuf7, (u8*)&stage2_9[arm7BootCodeOffsetInFile], arm7BootCodeSize);
	memcpy(outBuf9, (u8*)&stage2_9[arm9BootCodeOffsetInFile], arm9BootCodeSize);
	
	//Build NDS Header
	memcpy((u8*)0x027FFE00, NDSHeader, (headerSize*sizeof(u8)));
	
	printf("NDSLoader end. ");
	
	TGDSARM9Free(outBuf);
	TGDSARM9Free(NDSHeader);
	
	//Copy and relocate current TGDS DLDI section into target ARM9 binary
	//printf("Boot Stage2");
	bool stat = dldiPatchLoader((data_t *)NDS_LOADER_IPC_CTX_UNCACHED_NTR->arm9EntryAddress, (u32)arm9BootCodeSize, (u32)&_io_dldi_stub);
	if(stat == false){
		printf("DLDI Patch failed. APP does not support DLDI format.");
	}
	
	int ret=FS_deinit();
	
	asm("mcr	p15, 0, r0, c7, c10, 4");
	WRAM_CR = WRAM_0KARM9_32KARM7;	//96K ARM7 : 0x037f8000 ~ 0x03810000
	
	runBootstrapARM7();	//ARM9 Side						/	
	setNDSLoaderInitStatus(NDSLOADER_LOAD_OK);	//		|	Wait until ARM7.bin is copied back to IWRAM's target address
	//waitWhileNotSetStatus(NDSLOADER_START);		//		\
	
	u32 arm9Addr = (uint32)NDS_LOADER_IPC_CTX_UNCACHED_NTR->arm9EntryAddress;
	coherent_user_range_by_size(arm9Addr, (u32)arm9BootCodeSize);
	memset(0x023C0000, 0, 0x4000);
	reloadARMCore(arm9Addr);
	
	return false;
}

//ToolchainGenericDS-LinkedModule User implementation: Called if TGDS-LinkedModule fails to reload ARM9.bin from DLDI.
char args[8][MAX_TGDSFILENAME_LENGTH];
char *argvs[8];
int TGDSProjectReturnFromLinkedModule() __attribute__ ((optnone)) {
	return -1;
}

//This payload has all the ARM9 core hardware, TGDS Services, so SWI/SVC can work here.
int main(int argc, char **argv)  __attribute__ ((optnone)) {
	
	/*			TGDS 1.6 Standard ARM9 Init code start	*/
	bool isTGDSCustomConsole = false;	//set default console or custom console: default console
	GUI_init(isTGDSCustomConsole);
	GUI_clear();
	
	bool isCustomTGDSMalloc = true;
	setTGDSMemoryAllocator(getProjectSpecificMemoryAllocatorSetup(TGDS_ARM7_MALLOCSTART, TGDS_ARM7_MALLOCSIZE, isCustomTGDSMalloc));
	sint32 fwlanguage = (sint32)getLanguage();
	
	printf(" -- ");
	printf(" -- ");
	printf(" -- ");
	printf(" -- ");
	
	int ret=FS_init();
	if (ret == 0)
	{
		printf("FS Init ok.");
	}
	else if(ret == -1)
	{
		printf("FS Init error.");
	}
	asm("mcr	p15, 0, r0, c7, c10, 4");
	flush_icache_all();
	flush_dcache_all();
	/*			TGDS 1.6 Standard ARM9 Init code end	*/
	
	
	ReloadNDSBinaryFromContext(argv[0]);	//Boot NDS file
	return 0;
}
