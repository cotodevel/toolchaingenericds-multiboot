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
#include "TGDSLogoLZSSCompressed.h"
#include "ipcfifoTGDSUser.h"
#include "fatfslayerTGDS.h"
#include "cartHeader.h"
#include "ff.h"
#include "dldi.h"
#include "loader.h"
#include "dmaTGDS.h"
#include "nds_cp15_misc.h"
#include "fileBrowse.h"
#include <stdio.h>
#include "biosTGDS.h"
#include "global_settings.h"
#include "posixHandleTGDS.h"
#include "TGDSMemoryAllocator.h"
#include "debugNocash.h"
#include "tgds_ramdisk_dldi.h"

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int internalCodecType = SRC_NONE;//Internal because WAV raw decompressed buffers are used if Uncompressed WAV or ADPCM

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
bool stopSoundStreamUser(){
	return false;
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void closeSoundUser(){
	//Stubbed. Gets called when closing an audiostream of a custom audio decoder
}

//generates a table of sectors out of a given file. It has the ARM7 binary and ARM9 binary
__attribute__((section(".itcm")))
#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("Os")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
bool ReloadNDSBinaryFromContext(char * filename) {
	printf("tgds_multiboot_payload:ReloadNDSBinaryFromContext()");
	printf("fname:[%s]", filename);
	FILE * fh = NULL;
	fh = fopen(filename, "r+");
	int headerSize = sizeof(struct sDSCARTHEADER);
	u8 * NDSHeader = (u8 *)TGDSARM9Malloc(headerSize*sizeof(u8));
	if (fread(NDSHeader, 1, headerSize, fh) != headerSize){
		printf("header read error");
		TGDSARM9Free(NDSHeader);
		fclose(fh);
	}
	else{
		printf("header parsed correctly.");
	}
	struct sDSCARTHEADER * NDSHdr = (struct sDSCARTHEADER *)NDSHeader;
	//- ARM9 passes the filename to ARM7
	//- ARM9 waits in ITCM code until ARM7 reloads
	//- ARM7 handles/reloads everything in memory
	//- ARM7 gives permission to ARM9 to reload and ARM7 reloads as well
	//ARM7 reloads here (only if within 0x02000000 range)
	int arm7BootCodeSize = NDSHdr->arm7size;
	u32 arm7BootCodeOffsetInFile = NDSHdr->arm7romoffset;
	u32 arm7EntryAddress = NDSHdr->arm7entryaddress;	
	//is ARM7 Payload within 0x02xxxxxx range?
	if((arm7EntryAddress >= 0x02000000) && (arm7EntryAddress != 0x037f8000) && (arm7EntryAddress != 0x03800000) ){
		memset((void *)arm7EntryAddress, 0x0, arm7BootCodeSize);
		fseek(fh, (int)arm7BootCodeOffsetInFile, SEEK_SET);
		int readSize = fread((void *)(arm7EntryAddress | 0x400000), 1, arm7BootCodeSize, fh);
		printf("ARM7 (EWRAM payload) written! %d bytes", readSize);
	}
	//ARM7 Payload within 0x03xxxxxx range
	else{
		WRAM_CR = WRAM_0KARM9_32KARM7;	//96K ARM7 : 0x037f8000 ~ 0x03810000
		asm("mcr	p15, 0, r0, c7, c10, 4");
		memset((void *)ARM7_PAYLOAD, 0x0, arm7BootCodeSize);
		fseek(fh, (int)arm7BootCodeOffsetInFile, SEEK_SET);
		int readSize = fread((void *)ARM7_PAYLOAD, 1, arm7BootCodeSize, fh);
		coherent_user_range_by_size((uint32)ARM7_PAYLOAD, arm7BootCodeSize);
		printf("ARM7 (IWRAM payload) written! %d bytes", readSize);
	}
	
	//ARM9
	int arm9BootCodeSize = NDSHdr->arm9size;
	u32 arm9BootCodeOffsetInFile = NDSHdr->arm9romoffset;
	u32 arm9EntryAddress = NDSHdr->arm9entryaddress;	
	memset((void *)arm9EntryAddress, 0x0, arm9BootCodeSize);
	fseek(fh, (int)arm9BootCodeOffsetInFile, SEEK_SET);
	int readSize9 = fread((void *)(arm9EntryAddress | 0x400000), 1, arm9BootCodeSize, fh);
	printf("ARM9 written! %d bytes", readSize9);
	fclose(fh);
	
	REG_IME = 0;
	REG_IE = 0;
	REG_IF = 0;
	
	struct sIPCSharedTGDS * TGDSIPC = TGDSIPCStartAddress;
	uint32 * fifomsg = (uint32 *)&TGDSIPC->fifoMesaggingQueueSharedRegion[0];
	setValueSafe(&fifomsg[0], (u32)arm7EntryAddress);
	setValueSafe(&fifomsg[1], (u32)arm7BootCodeSize);
	SendFIFOWords(FIFO_TGDSMBRELOAD_SETUP, 0xFF);
	while (getValueSafe(&fifomsg[0]) == (u32)arm7EntryAddress){
		swiDelay(1);
	}
	
	printf("ARM7: %x - ARM9: %x", arm7EntryAddress, arm9EntryAddress);
	//DLDI patch it. If TGDS DLDI RAMDISK: Use standalone version, otherwise direct DLDI patch
	coherent_user_range_by_size((uint32)arm9EntryAddress, arm9BootCodeSize);
	u32 dldiSrc = (u32)&_io_dldi_stub;
	if(strncmp((char*)&dldiGet()->friendlyName[0], "TGDS RAMDISK", 12) == 0){
		dldiSrc = (u32)&tgds_ramdisk_dldi[0];
		//printf("GOT TGDS DLDI: %s", (char*)&dldiGet()->friendlyName[0]);
	}
	else{
		//printf("GOT direct DLDI: %s", (char*)&dldiGet()->friendlyName[0]);
	}
	bool stat = dldiPatchLoader((data_t *)arm9EntryAddress, (u32)arm9BootCodeSize, dldiSrc);
	if(stat == true){
		printf("DLDI patch success!");
	}
	
	typedef void (*t_bootAddr)();
	t_bootAddr bootARM9Payload = (t_bootAddr)arm9EntryAddress;
	bootARM9Payload();
	return false;
}

//ToolchainGenericDS-LinkedModule User implementation: Called if TGDS-LinkedModule fails to reload ARM9.bin from DLDI.
#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
char args[8][MAX_TGDSFILENAME_LENGTH];

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
char *argvs[8];

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("Os")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int TGDSProjectReturnFromLinkedModule() {
	return -1;
}

char thisARGV[MAX_TGDSFILENAME_LENGTH];

//This payload has all the ARM9 core hardware, TGDS Services, so SWI/SVC can work here.
#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int main(int argc, char **argv) {
	
	/*			TGDS 1.6 Standard ARM9 Init code start	*/
	bool isTGDSCustomConsole = true;	//set default console or custom console: default console
	GUI_init(isTGDSCustomConsole);
	GUI_clear();
	
	//Reload ARM7 player payload
	reloadStatus = (u32)0xFFFFFFFF;
	reloadARM7PlayerPayload((u32)0x06020000, 96*1024); //last 32K as sound buffer
	while(reloadStatus == (u32)0xFFFFFFFF){
		swiDelay(1);	
	}
	
	bool isCustomTGDSMalloc = true;
	setTGDSMemoryAllocator(getProjectSpecificMemoryAllocatorSetup(TGDS_ARM7_MALLOCSTART, TGDS_ARM7_MALLOCSIZE, isCustomTGDSMalloc, TGDSDLDI_ARM7_ADDRESS));
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
	}//			TGDS 1.6 Standard ARM9 Init code end	
	
	char * thisARGV = &argvIntraTGDSMB[0+20];
	if(
		(thisARGV[0] == 'f')
		&&
		(thisARGV[1] == 'a')
		&&
		(thisARGV[2] == 't')
		&&
		(thisARGV[3] == ':')
		&&
		(thisARGV[4] == '/')
		){
		char thisARGV2[MAX_TGDSFILENAME_LENGTH];
		memset(thisARGV2, 0, sizeof(thisARGV2));
		strcpy(thisARGV2, "0:/");
		strcat(thisARGV2, &thisARGV[5]);
		
		//copy back
		memset(thisARGV, 0, sizeof(thisARGV));
		strcpy(thisARGV, thisARGV2);
	}
	
	ReloadNDSBinaryFromContext((char*)thisARGV);	//Boot NDS file
	return 0;
}
