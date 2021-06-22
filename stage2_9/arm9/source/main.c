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
#include "debugNocash.h"
#include "utilsTGDS.h"

//3D Cube start 
#include "videoGL.h"
#include "videoTGDS.h"
#include "math.h"
#include "imagepcx.h"

float camDist = -0.3*4;
float rotateX = 0.0;
float rotateY = 0.0;
//3D Cube end

char curChosenBrowseFile[MAX_TGDSFILENAME_LENGTH+1];

//Back to loader, based on Whitelisted DLDI names
static char curLoaderNameFromDldiString[MAX_TGDSFILENAME_LENGTH+1];
static inline char * canGoBackToLoader(){
	char * dldiName = dldi_tryingInterface();
	if(dldiName != NULL){
		if(strcmp(dldiName, "R4iDSN") == 0){	//R4iGold loader
			strcpy(curLoaderNameFromDldiString, "0:/_DS_MENU.dat_ori");	//this allows to return to original payload code, and autoboot to TGDS-multiboot 
			return (char*)&curLoaderNameFromDldiString[0];
		}
		if(strcmp(dldiName, "Ninjapass X9 (SD Card)") == 0){	//Ninjapass X9SD
			strcpy(curLoaderNameFromDldiString, "0:/loader.nds");
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

//new: this bootcode will run from VRAM, once the NDSBinary context has been created and handled by the reload bootcode.
//generates a table of sectors out of a given file. It has the ARM7 binary and ARM9 binary
bool ReloadNDSBinaryFromContext(char * filename) __attribute__ ((optnone)) {
	
	nocashMessage("stage2_9->arm9->ReloadNDSBinaryFromContext() Start");
	
	char dbgMsg[96];
	memset(dbgMsg, 0, sizeof(dbgMsg));
	strcpy(dbgMsg, "Trying to open: ");
	strcat(dbgMsg, filename);
	nocashMessage(dbgMsg);
	
	volatile u8 * outBuf7 = NULL;
	volatile u8 * outBuf9 = NULL;

	//copy loader code (arm7bootldr.bin) to ARM7's EWRAM portion while preventing Cache issues
	coherent_user_range_by_size((uint32)&arm7bootldr[0], (int)arm7bootldr_size);					
	memcpy ((void *)NDS_LOADER_IPC_BOOTSTUBARM7_CACHED, (u32*)&arm7bootldr[0], arm7bootldr_size); 	//memcpy ( void * destination, const void * source, size_t num );	//memset(void *str, int c, size_t n)
	
	FILE * fh = NULL;
	if(__dsimode == false){
		fh = fopen(filename, "r+");	//NTR DLDI R/W enabled
	}
	else{
		fh = fopen(filename, "r");	//TWL mode hasn't TWL SD Write implemented yet
	}
	if(fh != NULL){
		int headerSize = sizeof(struct sDSCARTHEADER);
		u8 * NDSHeader = (u8 *)TGDSARM9Malloc(headerSize*sizeof(u8));
		if (fread(NDSHeader, 1, headerSize, fh) != headerSize){
			nocashMessage("header read error");
			printf("header read error");
			TGDSARM9Free(NDSHeader);
			fclose(fh);
			return false;
		}
		nocashMessage("header parsed correctly.");
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
		
		//Get TGDS file handle
		int fdindex = fileno(fh);
		struct fd * fdinst = getStructFD(fdindex);
		
		//reset FP to make it sector-relative + get file size
		fseek(fh,0,SEEK_END);
		int fileSize = ftell(fh);
		fseek(fh,0,SEEK_SET);
		NDS_LOADER_IPC_CTX_UNCACHED_NTR->fileSize = fileSize;
		
		printf("ReloadNDSBinaryFromContext3():");
		printf("arm7BootCodeSize:%d", arm7BootCodeSize);
		printf("arm7BootCodeOffsetInFile:%x", arm7BootCodeOffsetInFile);
		printf("arm7BootCodeEntryAddress:%x", NDS_LOADER_IPC_CTX_UNCACHED_NTR->arm7EntryAddress);
		printf("arm9BootCodeSize:%d", arm9BootCodeSize);
		printf("arm9BootCodeOffsetInFile:%x", arm9BootCodeOffsetInFile);
		printf("arm9BootCodeEntryAddress:%x", NDS_LOADER_IPC_CTX_UNCACHED_NTR->arm9EntryAddress);
		
		nocashMessage("ReloadNDSBinaryFromContext3():");
		sprintf(dbgMsg, "arm7BootCodeSize:%d", arm7BootCodeSize);
		nocashMessage(dbgMsg);
		sprintf(dbgMsg, "arm7BootCodeOffsetInFile:%x", arm7BootCodeOffsetInFile);
		nocashMessage(dbgMsg);
		sprintf(dbgMsg, "arm7BootCodeEntryAddress:%x", NDS_LOADER_IPC_CTX_UNCACHED_NTR->arm7EntryAddress);
		nocashMessage(dbgMsg);
		sprintf(dbgMsg, "arm9BootCodeSize:%d", arm9BootCodeSize);
		nocashMessage(dbgMsg);
		sprintf(dbgMsg, "arm9BootCodeOffsetInFile:%x", arm9BootCodeOffsetInFile);
		nocashMessage(dbgMsg);
		sprintf(dbgMsg, "arm9BootCodeEntryAddress:%x", NDS_LOADER_IPC_CTX_UNCACHED_NTR->arm9EntryAddress);
		nocashMessage(dbgMsg);
		
		//Generate cluster filemap
		uint32_t* cluster_table = (uint32_t*)&NDS_LOADER_IPC_CTX_UNCACHED_NTR->sectorTableBootCode[0];
		uint32_t  cur_cluster = getStructFDFirstCluster(fdinst);
		while (cur_cluster >= 2 && cur_cluster != 0xFFFFFFFF)
		{
			*cluster_table = clst2sect(&dldiFs, cur_cluster);
			cluster_table++;
			cur_cluster = f_getFat(fdinst->filPtr, cur_cluster);
		}
		*(cluster_table) = 0xFFFFFFFF;
		
		printf("NDSLoader start. ");
		
		u8 * outBuf = (u8 *)TGDSARM9Malloc(sectorSize * sectorsPerCluster);
		
		//Uncached to prevent cache issues right at once
		outBuf7 = (u8 *)(NDS_LOADER_IPC_ARM7BIN_UNCACHED_NTR);	//will not be higher than: arm7BootCodeSize
		outBuf9 = (u8 *)(NDS_LOADER_IPC_CTX_UNCACHED_NTR->arm9EntryAddress); //will not be higher than: arm9BootCodeSize or 0x2D0000 (2,949,120 bytes)
		
		u8 * outBuf7Seek = outBuf7;
		u8 * outBuf9Seek = outBuf9;
		
		int globalPtr = 0; //this one maps the entire file in 512 bytes (sectorSize)
		u32 cur_clustersector = NDS_LOADER_IPC_CTX_UNCACHED_NTR->sectorTableBootCode[0];
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
						memcpy (outBuf7Seek, outBuf + (i*512), 512);
						outBuf7Seek+=512;
					}
					else{
						memcpy (outBuf7Seek, outBuf + (i*512), sectorOffsetEnd7);
						outBuf7Seek+=sectorOffsetEnd7;
					}
				}
				
				//copy it into output ARM9 binary
				if ( (globalPtr >= arm9BootCodeOffsetInFile) && (globalPtr < (arm9BootCodeOffsetInFile+arm9BootCodeSize)) ){
					//last part?
					if( ((arm9BootCodeOffsetInFile+arm9BootCodeSize) - globalPtr) > sectorOffsetEnd9){
						memcpy (outBuf9Seek, outBuf + (i*512), 512);
						outBuf9Seek+=512;
					}
					else{
						memcpy (outBuf9Seek, outBuf + (i*512), sectorOffsetEnd9);
						outBuf9Seek+=sectorOffsetEnd9;
					}
				}	
				globalPtr +=512;
			}
			
			//ARM7 Range check
			data_read++;
			cur_clustersector = (u32)NDS_LOADER_IPC_CTX_UNCACHED_NTR->sectorTableBootCode[data_read];
		}
		
		printf("ARM7 %d bytes. [Addr: %x]", arm7BootCodeSize, (unsigned int)(outBuf7));
		printf("ARM9 %d bytes. [Addr: %x]", arm9BootCodeSize, (unsigned int)(outBuf9));
		
		//Build NDS Header
		memcpy((u8*)0x027FFE00, NDSHeader, (headerSize*sizeof(u8)));
		
		printf("NDSLoader end. ");
		
		TGDSARM9Free(outBuf);
		TGDSARM9Free(NDSHeader);
		
		//NTR Mode: DLDI R/W
		//TWL mode hasn't TWL SD Write implemented yet
		if(__dsimode == false){
			bool stat = dldiPatchLoader((data_t *)outBuf9, (u32)arm9BootCodeSize, (u32)&_io_dldi_stub);
			if(stat == false){
				printf("DLDI Patch failed. APP does not support DLDI format.");
			}
			
			//Patch DLDI to file before launching it
			fseek(fh, (int)arm9BootCodeOffsetInFile, SEEK_SET);
			int wrote = fwrite((u8*)outBuf9, 1, arm9BootCodeSize, fh);
			if(wrote >= 0){
				sint32 FDToSync = fileno(fh);
				fsync(FDToSync);	//save cached changes into disk
			}
		}
		fclose(fh);
		int ret=FS_deinit();
		
		asm("mcr	p15, 0, r0, c7, c10, 4");
		WRAM_CR = WRAM_0KARM9_32KARM7;	//96K ARM7 : 0x037f8000 ~ 0x03810000
		flush_icache_all();
		flush_dcache_all();
		//Copy and relocate current TGDS DLDI section into target ARM9 binary
		printf("Boot Stage3");
		
		runBootstrapARM7();	//ARM9 Side						/	
		setNDSLoaderInitStatus(NDSLOADER_LOAD_OK);	//		|	Wait until ARM7.bin is copied back to IWRAM's target address
		waitWhileNotSetStatus(NDSLOADER_START);		//		\
		
		//reload ARM9.bin
		coherent_user_range_by_size((uint32)outBuf9, (u32)arm9BootCodeSize);
		reloadARMCore(outBuf9);
	}
	return false;
}

bool stopSoundStreamUser(){
	return false;
}

void closeSoundUser(){
	//Stubbed. Gets called when closing an audiostream of a custom audio decoder
}

//ToolchainGenericDS-LinkedModule User implementation: Called if TGDS-LinkedModule fails to reload ARM9.bin from DLDI.
char args[8][MAX_TGDSFILENAME_LENGTH];
char *argvs[8];
int TGDSProjectReturnFromLinkedModule() __attribute__ ((optnone)) {
	return -1;
}

int main(int argc, char **argv)  __attribute__ ((optnone)) {
	
	/*			TGDS 1.6 Standard ARM9 Init code start	*/
	bool isTGDSCustomConsole = true;	//set default console or custom console: default console
	GUI_init(isTGDSCustomConsole);
	GUI_clear();
	bool isCustomTGDSMalloc = true;
	setTGDSMemoryAllocator(getProjectSpecificMemoryAllocatorSetup(TGDS_ARM7_MALLOCSTART, TGDS_ARM7_MALLOCSIZE, isCustomTGDSMalloc));
	sint32 fwlanguage = (sint32)getLanguage();
	
	nocashMessage("stage2_9->arm9->main() Before DLDI Init.");
	reportTGDSPayloadMode();
	
	printf("     ");
	printf("     ");
	
	int ret=FS_init();
	if (ret == 0)
	{
		printf("FS Init ok.");
		nocashMessage("stage2_9->arm9->main() DLDI Init OK.");
	}
	else if(ret == -1)
	{
		printf("FS Init error.");
		nocashMessage("stage2_9->arm9->main() DLDI Init fail.");
	}
	switch_dswnifi_mode(dswifi_idlemode);
	asm("mcr	p15, 0, r0, c7, c10, 4");
	flush_icache_all();
	flush_dcache_all();
	/*			TGDS 1.6 Standard ARM9 Init code end	*/
	
	menuShow();
	
	//gl start
	int i;
	{
		setOrientation(ORIENTATION_0, true);
		
		//set mode 0, enable BG0 and set it to 3D
		SETDISPCNT_MAIN(MODE_0_3D);
		
		//this should work the same as the normal gl call
		glViewPort(0,0,255,191);
		
		glClearColor(0,0,0);
		glClearDepth(0x7FFF);
		
		glReset();
		
		LoadGLTextures();
		
		glEnable(GL_ANTIALIAS);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		
		//any floating point gl call is being converted to fixed prior to being implemented
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(35, 256.0 / 192.0, 0.1, 40);
		
	}
	//gl end
	
	char * arg0 = (0x02280000 - (MAX_TGDSFILENAME_LENGTH+1));
	ReloadNDSBinaryFromContext(arg0);
	
	while (1){
		handleARM9SVC();	/* Do not remove, handles TGDS services */
		IRQWait(0, IRQ_VBLANK);
	}
}

