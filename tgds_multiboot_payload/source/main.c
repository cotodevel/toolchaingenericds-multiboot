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
#include "exceptionTGDS.h"

//ARM7 VRAM core
#include "arm7bootldr.h"
#include "arm7bootldr_twl.h"

char bootfileName[MAX_TGDSFILENAME_LENGTH];
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

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("Ofast")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int main(int argc, char **argv) {	
	//TWL/NTR Mode bios will change here, so prevent jumps to BIOS exception vector through any interrupts
	REG_IME = 0;
	REG_IE = 0;
	REG_IF = 0;
	setupDisabledExceptionHandler();
	register isNTRTWLBinary = (int)getValueSafe((u32*)ARM9_TWLORNTRPAYLOAD_MODE); //register means save this register and restore it everywhere it's used below. Save it now as it'll get erased
	
	//Execute Stage 1: IWRAM ARM7 payload: NTR/TWL (0x03800000)
	executeARM7Payload((u32)0x02380000, 96*1024, (u32*)TGDS_MB_V3_ARM7_STAGE1_ADDR);
	
	//Execute Stage 2: VRAM ARM7 payload: NTR/TWL (0x06000000)
	u32 * payload = NULL;
	if(__dsimode == false){
		payload = (u32*)&arm7bootldr[0];	
	}
	else{
		payload = (u32*)&arm7bootldr_twl[0];
	}
	executeARM7Payload((u32)0x02380000, 96*1024, payload);
	
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
	memset(bootfileName, 0, sizeof(bootfileName));
	strcpy(bootfileName, tempARGV);
	
	nocashMessage(" ---- ");
	nocashMessage(" ---- ");
	
	if(__dsimode == true){
		nocashMessage(" tgds_multiboot_payload.bin [TWL mode]");
	}
	else{
		nocashMessage(" tgds_multiboot_payload.bin [NTR mode]");
	}
	
	//NTR / TWL RAM Setup
	if(
		(__dsimode == true)
		&&
		(
		(isNTRTWLBinary == isNDSBinaryV1Slot2)
		||
		(isNTRTWLBinary == isNDSBinaryV1)
		||
		(isNTRTWLBinary == isNDSBinaryV2)
		||
		(isNTRTWLBinary == isNDSBinaryV3)
		)
	){
		//Enable 4M EWRAM (TWL)
		u32 SFGEXT9 = *(u32*)0x04004008;
		//14-15 Main Memory RAM Limit (0..1=4MB/DS, 2=16MB/DSi, 3=32MB/DSiDebugger)
		SFGEXT9 = (SFGEXT9 & ~(0x3 << 14)) | (0x0 << 14);
		*(u32*)0x04004008 = SFGEXT9;
	}
	else if(
		(__dsimode == true)
		&&
		(isNTRTWLBinary == isTWLBinary)
	){
		//Enable 16M EWRAM (TWL)
		u32 SFGEXT9 = *(u32*)0x04004008;
		//14-15 Main Memory RAM Limit (0..1=4MB/DS, 2=16MB/DSi, 3=32MB/DSiDebugger)
		SFGEXT9 = (SFGEXT9 & ~(0x3 << 14)) | (0x2 << 14);
		*(u32*)0x04004008 = SFGEXT9;
	}
	
	//ARM9 SVCs & loader context initialized:
	//Copy the file into non-case sensitive "tgdsboot.bin" into ARM7,
	//since PetitFS only understands 8.3 DOS format filenames
	WRAM_CR = WRAM_0KARM9_32KARM7;	//96K ARM7 : 0x037f8000 ~ 0x03810000
	asm("mcr	p15, 0, r0, c7, c10, 4");
	
	nocashMessage("Booting ...");
	
	//ARM9 waits & reloads into its new binary.
	setValueSafe(0x02FFFE24, (u32)0);
	
	struct sIPCSharedTGDS * TGDSIPC = TGDSIPCStartAddress;
	uint32 * fifomsg = (uint32 *)&TGDSIPC->fifoMesaggingQueueSharedRegion[0];
	setValueSafe(&fifomsg[7], (u32)BOOT_FILE_TGDSMB);
	SendFIFOWords(FIFO_SEND_TGDS_CMD, 0xFF);
	
	while( getValueSafe(0x02FFFE24) == ((u32)0) ){
		
	}
	
	u32 * arm7EntryAddress = (u32*)getValueSafe((u32*)0x02FFFE34);
	u32 * arm9EntryAddress = (u32*)getValueSafe((u32*)0x02FFFE24);
	int arm7BootCodeSize = (int)getValueSafe((u32*)ARM7_BOOT_SIZE);
	int arm9BootCodeSize = (int)getValueSafe((u32*)ARM9_BOOT_SIZE);
	u32 arm7OffsetInFile = (u32)getValueSafe((u32*)ARM7_BOOTCODE_OFST);
	u32 arm9OffsetInFile = (u32)getValueSafe((u32*)ARM9_BOOTCODE_OFST);
	u32 arm7iRamAddress = (u32)getValueSafe((u32*)ARM7i_RAM_ADDRESS);
	int arm7iBootCodeSize = (int)getValueSafe((u32*)ARM7i_BOOT_SIZE);
	u32 arm9iRamAddress = (u32)getValueSafe((u32*)ARM9i_RAM_ADDRESS);
	int arm9iBootCodeSize = (int)getValueSafe((u32*)ARM9i_BOOT_SIZE);
	
	//Todo: Support isNDSBinaryV1Slot2 binary (Slot2 Passme v1 .ds.gba homebrew)
	if(isNTRTWLBinary == isNDSBinaryV1Slot2){
		u8 fwNo = *(u8*)ARM7_ARM9_SAVED_DSFIRMWARE;
		int stage = 0;
		handleDSInitError(stage, (u32)fwNo);	
	}
	
	coherent_user_range_by_size((uint32)arm7EntryAddress, arm7BootCodeSize); //Make ARM9 
	coherent_user_range_by_size((uint32)arm9EntryAddress, arm9BootCodeSize); //	 memory	coherent (NTR/TWL)
	
	if(isNTRTWLBinary == isTWLBinary){
		if((arm7iRamAddress >= ARM_MININUM_LOAD_ADDR) && (arm7iBootCodeSize > 0)){
			coherent_user_range_by_size((uint32)arm7iRamAddress, arm7iBootCodeSize); //		also for TWL binaries 
		}
		if((arm9iRamAddress >= ARM_MININUM_LOAD_ADDR) && (arm9iBootCodeSize > 0)){
			coherent_user_range_by_size((uint32)arm9iRamAddress, arm9iBootCodeSize); //		   if we're launching one
		}
	}
	else if(isNTRTWLBinary != isNDSBinaryV1Slot2){ //DLDI is for Slot 1 v1/v2/v3 NTR binaries only
		u32 dldiSrc = (u32)&_io_dldi_stub;
		bool stat = dldiPatchLoader((data_t *)arm9EntryAddress, (u32)arm9BootCodeSize, dldiSrc);
		if(stat == true){
			nocashMessage("DLDI patch success!");
		}
	}
	
	if(__dsimode == true){
		//NTR (Backwards Compatibility mode) / TWL Bios Setup
		u32 * SCFG_ROM = 0x04004000;
		if(isNTRTWLBinary == isTWLBinary){
			while ((u16)getValueSafe(SCFG_ROM) != ((u16)1)){}
			//BIOS mode: [TWL].
		}
		else if( 
			(isNTRTWLBinary == isNDSBinaryV1Slot2)
			||
			(isNTRTWLBinary == isNDSBinaryV1)
			||
			(isNTRTWLBinary == isNDSBinaryV2)
			||
			(isNTRTWLBinary == isNDSBinaryV3)
		){
			while ((u16)getValueSafe(SCFG_ROM) != ((u16)3)){}
			//BIOS mode: [NTR]. 
		}
		else{
			handleDSInitOutputMessage("tgds_multiboot_payload.bin[TWL Mode]: BIOS cfg fail");
			u8 fwNo = *(u8*)(0x027FF000 + 0x5D);
			int stage = 10;
			handleDSInitError(stage, (u32)fwNo);	
		}
		setValueSafe((u32*)0x02FFFE24, (u32)arm9EntryAddress); //TWL mode memory overwrites the entrypoint, restore it
	}

	//Copy ARGV-CMD line
	memcpy((void *)__system_argv, (const void *)&argvIntraTGDSMB[0], 256);
	
	//give VRAM_A & VRAM_B & VRAM_C & VRAM_D back to ARM9	//can't do this because ARM7 still executes code from VRAM
	//*(u8*)0x04000240 = (VRAM_A_LCDC_MODE | VRAM_ENABLE);	//4000240h  1  VRAMCNT_A - VRAM-A (128K) Bank Control (W)
	//*(u8*)0x04000241 = (VRAM_B_LCDC_MODE | VRAM_ENABLE);	//4000241h  1  VRAMCNT_B - VRAM-B (128K) Bank Control (W)
	//*(u8*)0x04000242 = (VRAM_C_LCDC_MODE | VRAM_ENABLE);	//4000242h  1  VRAMCNT_C - VRAM-C (128K) Bank Control (W)
	//*(u8*)0x04000243 = (VRAM_D_LCDC_MODE | VRAM_ENABLE);	//4000243h  1  VRAMCNT_D - VRAM-D (128K) Bank Control (W)
	
	//Reload ARM9 core. //Note: swiSoftReset(); can't be used here because ARM Core needs to switch to Thumb1 or ARM v5te now
	bootarm9payload();
}
