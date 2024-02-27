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
#include "utilsTGDS.h"
#include "nds_cp15_misc.h"
#include "fileBrowse.h"
#include <stdio.h>
#include "biosTGDS.h"
#include "global_settings.h"
#include "posixHandleTGDS.h"
#include "TGDSMemoryAllocator.h"
#include "debugNocash.h"
#include "tgds_ramdisk_dldi.h"
#include "arm7bootldr.h"
#include "arm7bootldr_twl.h"

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

char thisArgv[10][MAX_TGDSFILENAME_LENGTH];
char * getPayloadName(){
	if(__dsimode == false){
		return (char*)"0:/tgds_multiboot_payload_ntr.bin";	//TGDS NTR SDK (ARM9 binaries) emits TGDSMultibootRunNDSPayload() which reloads into NTR TGDS-MB Reload payload
	}		
	return (char*)"0:/tgds_multiboot_payload_twl.bin";	//TGDS TWL SDK (ARM9i binaries) emits TGDSMultibootRunNDSPayload() which reloads into TWL TGDS-MB Reload payload
}			
			
#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int ReloadNDSBinaryFromContext(char * filename) {
	if(getTGDSDebuggingState() == true){
		printf("tgds_multiboot_payload:ReloadNDSBinaryFromContext()");
		printf("fname:[%s]", filename);
	}
	int isNTRTWLBinary = isNTROrTWLBinary(filename);
	FILE * fh = NULL;
	fh = fopen(filename, "r");
	int headerSize = sizeof(struct sDSCARTHEADER);
	u8 * NDSHeader = (u8 *)TGDSARM9Malloc(headerSize*sizeof(u8));
	if (fread(NDSHeader, 1, headerSize, fh) != headerSize){
		if(getTGDSDebuggingState() == true){
			printf("header read error");
		}
		TGDSARM9Free(NDSHeader);
		fclose(fh);
	}
	else{
		if(getTGDSDebuggingState() == true){
			printf("header parsed correctly.");
		}
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
	
	WRAM_CR = WRAM_0KARM9_32KARM7;	//96K ARM7 : 0x037f8000 ~ 0x03810000
	asm("mcr	p15, 0, r0, c7, c10, 4");
	memset((void *)ARM7_PAYLOAD, 0x0, arm7BootCodeSize);
	coherent_user_range_by_size((uint32)ARM7_PAYLOAD, arm7BootCodeSize);
	fseek(fh, (int)arm7BootCodeOffsetInFile, SEEK_SET);
	int readSize7 = fread((void *)ARM7_PAYLOAD, 1, arm7BootCodeSize, fh);
	coherent_user_range_by_size((uint32)ARM7_PAYLOAD, arm7BootCodeSize);
	if(getTGDSDebuggingState() == true){
		printf("ARM7 (IWRAM payload) written! %d bytes", readSize7);
	}
	
	//TWL extended Header: secure section (TWL9 only)
	int dsiARM9headerOffset = 0;
	if(
		(__dsimode == true) 
		&&
		(isNTRTWLBinary == isTWLBinary)
	){
		dsiARM9headerOffset = 0x800;
	}
	
	//ARM9
	int arm9BootCodeSize = NDSHdr->arm9size;
	u32 arm9BootCodeOffsetInFile = NDSHdr->arm9romoffset + dsiARM9headerOffset;
	u32 arm9EntryAddress = NDSHdr->arm9entryaddress;	
	memset((void *)arm9EntryAddress, 0x0, arm9BootCodeSize);
	coherent_user_range_by_size((uint32)arm9EntryAddress, arm9BootCodeSize);
	fseek(fh, (int)arm9BootCodeOffsetInFile, SEEK_SET);
	int readSize9 = fread((void *)arm9EntryAddress, 1, arm9BootCodeSize, fh);
	coherent_user_range_by_size((uint32)arm9EntryAddress, arm9BootCodeSize);
	if(getTGDSDebuggingState() == true){
		printf("ARM9 written! %d bytes", readSize9);
	}
	fclose(fh);
	
	REG_IME = 0;
	REG_IE = 0;
	REG_IF = 0;
	

	if (*((vu32*)(arm9EntryAddress + 0xEC)) == 0xEAFFFFFE) // b #0; //loop
        *((vu32*)(arm9EntryAddress + 0xEC)) = 0xE3A00000; // mov r0, #0

	char tempDebug[256];
	sprintf(tempDebug, "[arm7EntryAddress:%x]\n[arm7BootCodeOffsetInFile:%x]\n[arm7BootCodeSize:%x]\n[readSize7:%x]\n", arm7EntryAddress, arm7BootCodeOffsetInFile, arm7BootCodeSize, readSize7);
	nocashMessage(tempDebug);

	sprintf(tempDebug, "[arm9EntryAddress:%x]\n[arm9BootCodeOffsetInFile:%x]\n[arm9BootCodeSize:%x]\n[readSize9:%x]\n", arm9EntryAddress, arm9BootCodeOffsetInFile, arm9BootCodeSize, readSize9);
	nocashMessage(tempDebug);

	struct sIPCSharedTGDS * TGDSIPC = TGDSIPCStartAddress;
	uint32 * fifomsg = (uint32 *)&TGDSIPC->fifoMesaggingQueueSharedRegion[0];
	setValueSafe(&fifomsg[0], (u32)arm7EntryAddress);
	setValueSafe(&fifomsg[1], (u32)arm7BootCodeSize);
	SendFIFOWords(FIFO_TGDSMBRELOAD_SETUP, 0xFF);
	while (getValueSafe(&fifomsg[0]) == (u32)arm7EntryAddress){
		swiDelay(1);
	}
	if(getTGDSDebuggingState() == true){
		printf("ARM7: %x - ARM9: %x", arm7EntryAddress, arm9EntryAddress);
	}
	//DLDI patch it. If TGDS DLDI RAMDISK: Use standalone version, otherwise direct DLDI patch
	if(strncmp((char*)&dldiGet()->friendlyName[0], "TGDS RAMDISK", 12) == 0){
		if(getTGDSDebuggingState() == true){
			printf("GOT TGDS DLDI: Skipping patch");
		}
	}
	else{
		u32 dldiSrc = (u32)&_io_dldi_stub;
		bool stat = dldiPatchLoader((data_t *)arm9EntryAddress, (u32)arm9BootCodeSize, dldiSrc);
		if(stat == true){
			if(getTGDSDebuggingState() == true){
				printf("DLDI patch success!");
			}
		}
	}
	
	//Copy CMD line
	memcpy((void *)__system_argv, (const void *)&argvIntraTGDSMB[0], 256);
	
	typedef void (*t_bootAddr)();
	t_bootAddr bootARM9Payload = (t_bootAddr)arm9EntryAddress;
	bootARM9Payload();
	return -1;
}

#ifdef ARM9
#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void reloadARM7Payload(u32 arm7entryaddress, int arm7BootCodeSize){
	struct sIPCSharedTGDS * TGDSIPC = TGDSIPCStartAddress;
	uint32 * fifomsg = (uint32 *)&TGDSIPC->fifoMesaggingQueueSharedRegion[0];
	
	//NTR ARM7 payload
	if(__dsimode == false){
		coherent_user_range_by_size((u32)&arm7bootldr[0], arm7BootCodeSize);
		setValueSafe(&fifomsg[0], (u32)&arm7bootldr[0]);
	}
	//TWL ARM7 payload
	else{
		coherent_user_range_by_size((u32)&arm7bootldr_twl[0], arm7BootCodeSize);
		setValueSafe(&fifomsg[0], (u32)&arm7bootldr_twl[0]);
	}
	setValueSafe(&fifomsg[1], (u32)arm7BootCodeSize);
	setValueSafe(&fifomsg[2], (u32)arm7entryaddress);
	SendFIFOWords(FIFO_ARM7_RELOAD, 0xFF);
}
#endif

//This payload has all the ARM9 core hardware, TGDS Services, so SWI/SVC can work here.
#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int main(int argc, char **argv) {
	//Reload ARM7 payload
	reloadStatus = (u32)0xFFFFFFFF;
	reloadARM7Payload((u32)0x023D0000, 64*1024);
	while(reloadStatus == (u32)0xFFFFFFFF){
		swiDelay(1);	
	}
	
	//Libnds compatibility: If (recv) mainARGV fat:/ change to 0:/
	char thisARGV[MAX_TGDSFILENAME_LENGTH];
	memset(thisARGV, 0, sizeof(thisARGV));
	strcpy(thisARGV, argv[1]);
	
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
	
	/*			TGDS 1.6 Standard ARM9 Init code start	*/
	bool isTGDSCustomConsole = true;	//set default console or custom console: default console
	GUI_init(isTGDSCustomConsole);
	GUI_clear();
	
	bool dsimodeARM7 = getNTRorTWLModeFromExternalProcessor();
	if(dsimodeARM7 != __dsimode){
		char * TGDSMBPAYLOAD = getPayloadName();
		clrscr();
		printf("----");
		printf("----");
		printf("----");
		printf("%s: tried to boot >%d", TGDSMBPAYLOAD, TGDSPrintfColor_Yellow);
		printf("with an incompatible ARM7 core .>%d", TGDSPrintfColor_Yellow);
		
		char arm7Mode[256];
		char arm9Mode[256];
		if(dsimodeARM7 == true){
			strcpy(arm7Mode, "ARM7 Mode: [TWL]");
		}
		else {
			strcpy(arm7Mode, "ARM7 Mode: [NTR]");
		}
		if(__dsimode == true){
			strcpy(arm9Mode, "ARM9 Mode: [TWL]");
		}
		else {
			strcpy(arm9Mode, "ARM9 Mode: [NTR]");
		}
		printf("(%s) (%s) >%d", arm7Mode, arm9Mode);
		printf("Did you try to boot a TWL payload in NTR mode? .>%d", TGDSPrintfColor_Yellow);
		printf("Turn off the hardware now.");
		while(1==1){
			IRQWait(0, IRQ_VBLANK);
		}
	}


	bool isCustomTGDSMalloc = true;
	setTGDSMemoryAllocator(getProjectSpecificMemoryAllocatorSetup(TGDS_ARM7_MALLOCSTART, TGDS_ARM7_MALLOCSIZE, isCustomTGDSMalloc, TGDSDLDI_ARM7_ADDRESS));
	sint32 fwlanguage = (sint32)getLanguage();
	
	printf(" ---- ");
	printf(" ---- ");
	printf(" ---- ");
	
	int ret=FS_init();
	if (ret != 0){
		printf("%s: FS Init error: %d >%d", TGDSPROJECTNAME, ret, TGDSPrintfColor_Red);
		while(1==1){
			swiDelay(1);
		}
	}
	/*			TGDS 1.6 Standard ARM9 Init code end	*/
	
	//If NTR/TWL Binary
	int isNTRTWLBinary = isNTROrTWLBinary(thisARGV);
	//Trying to boot a TWL binary in NTR mode? 
	if(!(isNTRTWLBinary == isNDSBinaryV1) && !(isNTRTWLBinary == isNDSBinaryV2) && !(isNTRTWLBinary == isNDSBinaryV3) && !(isNTRTWLBinary == isTWLBinary)){
		char * TGDSMBPAYLOAD = getPayloadName();
		clrscr();
		printf("----");
		printf("----");
		printf("----");
		printf("%s: tried to boot >%d", TGDSMBPAYLOAD, TGDSPrintfColor_Yellow);
		printf("an invalid binary.>%d", TGDSPrintfColor_Yellow);
		printf("[%s]:  >%d", thisARGV, TGDSPrintfColor_Green);
		printf("Please supply proper binaries. >%d", TGDSPrintfColor_Red);
		printf("Turn off the hardware now.");
		while(1==1){
			IRQWait(0, IRQ_VBLANK);
		}		
	}
	return ReloadNDSBinaryFromContext((char*)thisARGV);	//Boot NDS file	
}
