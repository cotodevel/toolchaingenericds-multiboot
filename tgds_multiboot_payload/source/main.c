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
int ReloadNDSBinaryFromContext(char * filename, int isNTRTWLBinary) {
	//Todo: Support isNDSBinaryV1Slot2 binary (Slot2 Passme v1 .ds.gba homebrew)
	if(isNTRTWLBinary == isNDSBinaryV1Slot2){
		u8 fwNo = *(u8*)(0x02300000);
		int stage = 0;
		handleDSInitError(stage, (u32)fwNo);	
	}

	if(getTGDSDebuggingState() == true){
		GUI_printf("tgds_multiboot_payload:ReloadNDSBinaryFromContext()");
		GUI_printf("fname:[%s]", filename);
	}
	FILE * fh = NULL;
	fh = fopen(filename, "r");
	int headerSize = sizeof(struct sDSCARTHEADER);
	u8 * NDSHeader = (u8 *)TGDSARM9Malloc(headerSize*sizeof(u8));
	if (fread(NDSHeader, 1, headerSize, fh) != headerSize){
		if(getTGDSDebuggingState() == true){
			GUI_printf("header read error");
		}
		TGDSARM9Free(NDSHeader);
		fclose(fh);
	}
	else{
		if(getTGDSDebuggingState() == true){
			GUI_printf("header parsed correctly.");
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
		GUI_printf("ARM7 (IWRAM payload) written! %d bytes", readSize7);
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
		GUI_printf("ARM9 written! %d bytes", readSize9);
	}
	fclose(fh);
	
	REG_IME = 0;
	REG_IE = 0;
	REG_IF = 0;
	
	if (*((vu32*)(arm9EntryAddress + 0xEC)) == 0xEAFFFFFE){ // b #0; //loop
        *((vu32*)(arm9EntryAddress + 0xEC)) = 0xE3A00000; // mov r0, #0
	}
	
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
		GUI_printf("ARM7: %x - ARM9: %x", arm7EntryAddress, arm9EntryAddress);
	}
	//DLDI patch it. If TGDS DLDI RAMDISK: Use standalone version, otherwise direct DLDI patch
	if(strncmp((char*)&dldiGet()->friendlyName[0], "TGDS RAMDISK", 12) == 0){
		if(getTGDSDebuggingState() == true){
			GUI_printf("GOT TGDS DLDI: Skipping patch");
		}
	}
	else{
		u32 dldiSrc = (u32)&_io_dldi_stub;
		bool stat = dldiPatchLoader((data_t *)arm9EntryAddress, (u32)arm9BootCodeSize, dldiSrc);
		if(stat == true){
			if(getTGDSDebuggingState() == true){
				GUI_printf("DLDI patch success!");
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
	reloadStatus = (u32)0xFFFFFFFF;
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
	//give VRAM_D to ARM7 @0x06000000
	*(u8*)0x04000243 = (VRAM_D_0x06000000_ARM7 | VRAM_ENABLE);
	setValueSafe(&fifomsg[1], (u32)arm7BootCodeSize);
	setValueSafe(&fifomsg[2], (u32)arm7entryaddress);
	SendFIFOWords(FIFO_ARM7_RELOAD, 0xFF);
	while(reloadStatus == (u32)0xFFFFFFFF){
		swiDelay(1);	
	}
}
#endif

//This payload has all the ARM9 core TGDS Services working.
#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int main(int argc, char **argv) {
	//Reload ARM7 payload
	reloadARM7Payload((u32)0x06000000, 64*1024);
	
	//Libnds compatibility: If libnds homebrew implemented TGDS-MB support for some reason, and uses a TGDS-MB payload, then swap "fat:/" to "0:/"
	char tempARGV[MAX_TGDSFILENAME_LENGTH];
	memset(tempARGV, 0, sizeof(tempARGV));
	strcpy(tempARGV, argv[1]);
	
	if(
		(tempARGV[0] == 'f')
		&&
		(tempARGV[1] == 'a')
		&&
		(tempARGV[2] == 't')
		&&
		(tempARGV[3] == ':')
		&&
		(tempARGV[4] == '/')
		){
		char tempARGV2[MAX_TGDSFILENAME_LENGTH];
		memset(tempARGV2, 0, sizeof(tempARGV2));
		strcpy(tempARGV2, "0:/");
		strcat(tempARGV2, &tempARGV[5]);
		
		//copy back
		memset(tempARGV, 0, sizeof(tempARGV));
		strcpy(tempARGV, tempARGV2);
	}
	
	/*			TGDS 1.6 Standard ARM9 Init code start	*/
	bool isTGDSCustomConsole = false;	//set default console or custom console: default console
	GUI_init(isTGDSCustomConsole);
	GUI_clear();
	
	bool isCustomTGDSMalloc = true;
	setTGDSMemoryAllocator(getProjectSpecificMemoryAllocatorSetup(isCustomTGDSMalloc));
	sint32 fwlanguage = (sint32)getLanguage();
	
	GUI_printf(" ---- ");
	GUI_printf(" ---- ");
	GUI_printf(" ---- ");
	
	if(__dsimode == true){
		GUI_printf(" tgds_multiboot_payload.bin [TWL mode]");
	}
	else{
		GUI_printf(" tgds_multiboot_payload.bin [NTR mode]");
	}
	int ret=FS_init();
	if (ret != 0){
		GUI_printf("%s: FS Init error: %d >%d", TGDSPROJECTNAME, ret, TGDSPrintfColor_Red);
		while(1==1){
			swiDelay(1);
		}
	}
	/*			TGDS 1.6 Standard ARM9 Init code end	*/
	
	//If NTR/TWL Binary
	int isNTRTWLBinary = isNTROrTWLBinary(tempARGV);
	//Trying to boot a TWL binary in NTR mode? 
	if(!(isNTRTWLBinary == isNDSBinaryV1) && !(isNTRTWLBinary == isNDSBinaryV2) && !(isNTRTWLBinary == isNDSBinaryV3) && !(isNTRTWLBinary == isTWLBinary) && !(isNTRTWLBinary == isNDSBinaryV1Slot2)){
		char * TGDSMBPAYLOAD = getPayloadName();
		clrscr();
		GUI_printf("----");
		GUI_printf("----");
		GUI_printf("----");
		GUI_printf("%s: tried to boot >%d", TGDSMBPAYLOAD, TGDSPrintfColor_Yellow);
		GUI_printf("an invalid binary.>%d", TGDSPrintfColor_Yellow);
		GUI_printf("[%s]:  >%d", tempARGV, TGDSPrintfColor_Green);
		GUI_printf("Please supply proper binaries. >%d", TGDSPrintfColor_Red);
		GUI_printf("Turn off the hardware now.");
		while(1==1){
			IRQWait(0, IRQ_VBLANK);
		}		
	}
	else{
		GUI_printf(tempARGV);
		GUI_printf(" Loading... ");
	}
	return ReloadNDSBinaryFromContext((char*)tempARGV, isNTRTWLBinary);	//Boot NDS file.
}
