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
#include "TGDS_threads.h"

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
__attribute__((optimize("Os")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void IRQInitCustom(u8 DSHardware)  {

	//Disable mpu
	CP15ControlRegisterDisable(CR_M);
	
	//Hardware ARM9 Init
	//resetMemory_ARMCores(DSHardware); //no. Breaks TGDS-MB ARM7 bootldr core
	

	/////////////////////IRQ INIT
	#ifdef ARM9
	DrainWriteBuffer();
	setTGDSARM9PrintfCallback((printfARM9LibUtils_fn)&TGDSDefaultPrintf2DConsole); //Setup default TGDS Project: console 2D printf 
	#endif
	
	//FIFO IRQ Init
	REG_IE = 0;
	REG_IF = ~0;
	REG_IME = 0;
	
	//FIFO IRQ Init
	REG_IPC_SYNC = (1 << 14);	//14    R/W  Enable IRQ from remote CPU  (0=Disable, 1=Enable)
	REG_IPC_FIFO_CR = IPC_FIFO_SEND_CLEAR | RECV_FIFO_IPC_IRQ  | FIFO_IPC_ENABLE;
	
	//Set up PPU IRQ: HBLANK/VBLANK/VCOUNT
	#ifdef ARM7
	REG_DISPSTAT = (DISP_VBLANK_IRQ | DISP_YTRIGGER_IRQ);
	#endif
	#ifdef ARM9
	REG_DISPSTAT = (DISP_VBLANK_IRQ | DISP_YTRIGGER_IRQ);
	#endif
	
	//Set up PPU IRQ Vertical Line
	setVCountIRQLine(TGDS_VCOUNT_LINE_INTERRUPT);
	
	volatile uint32 interrupts_to_wait_armX = 0;	
	#ifdef ARM7
	interrupts_to_wait_armX = IRQ_TIMER1 | IRQ_VBLANK | IRQ_VCOUNT | IRQ_IPCSYNC | IRQ_RECVFIFO_NOT_EMPTY | IRQ_SCREENLID;
	#endif
	
	#ifdef ARM9
	interrupts_to_wait_armX = IRQ_VBLANK | IRQ_VCOUNT | IRQ_IPCSYNC | IRQ_RECVFIFO_NOT_EMPTY;
	#endif
	
	REG_IE = interrupts_to_wait_armX;
	INTERRUPT_VECTOR = (uint32)&NDS_IRQHandler;
	
	setupDisabledExceptionHandler();

	int isNTRTWLBinary = isThisPayloadNTROrTWLMode();
	if(isNTRTWLBinary == isTWLBinary){
		__dsimode = true;
		#ifdef ARM9
		nocashMessage("TGDS:IRQInit():TWL Mode!");
		#endif
	}
	else{
		__dsimode = false;
		#ifdef ARM9
		nocashMessage("TGDS:IRQInit():NTR Mode!"); //ok so far
		#endif
	}
	#ifdef TWLMODE
		#ifdef ARM7
		//TWL ARM7 IRQ Init
		REG_AUXIE = 0;
		REG_AUXIF = ~0;
		irqEnableAUX(IRQ_I2C);
		
		//TGDS-Projects -> TWL TSC
		TWLSetTouchscreenNTRMode();
		#endif
		
		#ifdef ARM9
		//TWL ARM9 IRQ Init
		#endif
	#endif

	set0xFFFF0000FastMPUSettings(); //MPU needs to be setup here

	REG_IME = 1;

	////////////////////////////////IRQ INIT end
	
	//Enable mpu
	CP15ControlRegisterEnable(CR_M);
	
	//Library init code
	
	//Newlib init
	//Stream Devices Init: see devoptab_devices.c
	//setbuf(stdin, NULL);
	setbuf(stdout, NULL);	//iprintf directs to DS Framebuffer (printf already does that)
	//setbuf(stderr, NULL);
	
	TryToDefragmentMemory();
	
	//Enable TSC
	setTouchScreenEnabled(true);
	
	handleARM9InitSVC();
	
	savedDSHardware = (u32)DSHardware; //Global DS Firmware ARM7/ARM9
	
	//Shared ARM Cores
	disableTGDSDebugging(); //Disable debugging by default
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int main(int argc, char **argv) {
	
	//Save ARGV-CMD line
	memcpy((char*)TGDS_ARGV_BUFFER, (void *)argvIntraTGDSMB, 256);
	coherent_user_range_by_size((uint32)TGDS_ARGV_BUFFER, (int)256);
	
	memset(0x02000000, 0, (int)512*1024); //clear RAM: Fixes DKARM / Custom Devkit NTR/TWL homebrew relying on RAM state not initialized properly
	coherent_user_range_by_size((u32)0x02000000, (int)512*1024);

	struct DLDI_INTERFACE * dldiCopy = (struct DLDI_INTERFACE *)&_io_dldi_stub;
	char * dldiName = (char*)&dldiCopy->friendlyName[0];
	bool isEMUEnv = ( (dldiName[0] == 'T') && (dldiName[1] == 'G') && (dldiName[2] == 'D') && (dldiName[3] == 'S') && (dldiName[4] == ' ')  );  

	//ARM7DLDI = Slot1 or Slot2. VERY IMPORTANT, OTHERWISE THE ARM7 Core won't see the DLDI card mapped, even if the driver is there!!
	if (dldiCopy->ioInterface.features & FEATURE_SLOT_GBA) {
		SetBusSLOT1ARM9SLOT2ARM7();
	}
	if (dldiCopy->ioInterface.features & FEATURE_SLOT_NDS) {
		SetBusSLOT1ARM7SLOT2ARM9();
	}
	
	//Execute Stage 2: VRAM ARM7 payload: NTR/TWL (0x06000000). Required to have the DLDI context initializated in this payload.
	executeARM7Payload((u32)0x02380000, 96*1024, TGDS_MB_V3_ARM7_STAGE1_ADDR);
	
	//TWL/NTR Mode bios will change here, so prevent jumps to BIOS exception vector through any interrupts
	REG_IME = 0;
	REG_IE = 0;
	REG_IF = 0;
	CP15ControlRegisterDisable(CR_M); //prevent MPU irqs
	
	register int isNTRTWLBinary = (int)getValueSafe((u32*)ARM9_TWLORNTRPAYLOAD_MODE); //register means save this register and restore it everywhere it's used below. Save it now as it'll get erased
	bool isTGDSTWLHomebrew = (bool)getValueSafe((u32*)TGDS_IS_TGDS_TWL_HOMEBREW); //is TGDS TWL homebrew? Uses special map
	int tgds_multiboot_payload_size = (int)getValueSafe((u32*)TGDS_MB_V3_PAYLOAD_SIZE);
	
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
		||
		(isTGDSTWLHomebrew == true)
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
		&&
		(isTGDSTWLHomebrew == false)
	){
		//Enable 16M EWRAM (TWL)
		u32 SFGEXT9 = *(u32*)0x04004008;
		//14-15 Main Memory RAM Limit (0..1=4MB/DS, 2=16MB/DSi, 3=32MB/DSiDebugger)
		SFGEXT9 = (SFGEXT9 & ~(0x3 << 14)) | (0x2 << 14);
		*(u32*)0x04004008 = SFGEXT9;
		
		REG_EXMEMCNT = 0xE880;
		initMBKARM9();
	}
	else if(	//TGDS TWL ?
		(__dsimode == true)
		&&
		(isNTRTWLBinary == isTWLBinary)
		&&
		(isTGDSTWLHomebrew == true)
	){
		//Enable 4M EWRAM (TWL)
		u32 SFGEXT9 = *(u32*)0x04004008;
		//14-15 Main Memory RAM Limit (0..1=4MB/DS, 2=16MB/DSi, 3=32MB/DSiDebugger)
		SFGEXT9 = (SFGEXT9 & ~(0x3 << 14)) | (0x0 << 14);
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
	
	struct sIPCSharedTGDS * TGDSIPC = (struct sIPCSharedTGDS *)TGDSIPCStartAddress;
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
	
	//Slot2 Passme v1 .ds.gba homebrew is unsupported. See: https://github.com/cotodevel/toolchaingenericds-multiboot/issues/8
	if(isNTRTWLBinary == isNDSBinaryV1Slot2){
		u8 fwNo = *(u8*)ARM7_ARM9_SAVED_DSFIRMWARE;
		int stage = 0;
		handleDSInitError(stage, (u32)fwNo);	
	}
	
	//TGDS-MB v3 ARM7 bootldr error handling.
	if((u32)arm9EntryAddress == (u32)0xFFFFFFFE){
					handleDSInitOutputMessage("TGDS-MB: arm7bootldr/bootfile(): Boot fail. Notify developer.");
			u8 fwNo = *(u8*)(0x027FF000 + 0x5D);
			int stage = 10;
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
	if(isNTRTWLBinary != isNDSBinaryV1Slot2){ //DLDI is for Slot 1 v1/v2/v3 NTR / TGDS/DKARM hybrid TWL binaries only
		u32 dldiSrc = (u32)&_io_dldi_stub;
		bool stat = dldiPatchLoader((data_t *)arm9EntryAddress, (int)arm9BootCodeSize, dldiSrc);
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
	memcpy((void *)__system_argv, (const void *)TGDS_ARGV_BUFFER, 256);
	coherent_user_range_by_size((uint32)__system_argv, (int)256);
	
	//give VRAM_A & VRAM_B & VRAM_C & VRAM_D back to ARM9	//can't do this because ARM7 still executes code from VRAM
	//*(u8*)0x04000240 = (VRAM_A_LCDC_MODE | VRAM_ENABLE);	//4000240h  1  VRAMCNT_A - VRAM-A (128K) Bank Control (W)
	//*(u8*)0x04000241 = (VRAM_B_LCDC_MODE | VRAM_ENABLE);	//4000241h  1  VRAMCNT_B - VRAM-B (128K) Bank Control (W)
	//*(u8*)0x04000242 = (VRAM_C_LCDC_MODE | VRAM_ENABLE);	//4000242h  1  VRAMCNT_C - VRAM-C (128K) Bank Control (W)
	//*(u8*)0x04000243 = (VRAM_D_LCDC_MODE | VRAM_ENABLE);	//4000243h  1  VRAMCNT_D - VRAM-D (128K) Bank Control (W)
	
	//Reload ARM9 core. //Note: swiSoftReset(); can't be used here because ARM Core needs to switch to Thumb1 or ARM v5te now
	bootarm9payload();
}

//////////////////////////////////////////////////////// Threading User code start : TGDS Project specific ////////////////////////////////////////////////////////
//User callback when Task Overflows. Intended for debugging purposes only, as normal user code tasks won't overflow if a task is implemented properly.
//	u32 * args = This Task context
#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void onThreadOverflowUserCode(u32 * args){
	struct task_def * thisTask = (struct task_def *)args;
	struct task_Context * parentTaskCtx = thisTask->parentTaskCtx;	//get parent Task Context node 

	char threadStatus[64];
	switch(thisTask->taskStatus){
		case(INVAL_THREAD):{
			strcpy(threadStatus, "INVAL_THREAD");
		}break;
		
		case(THREAD_OVERFLOW):{
			strcpy(threadStatus, "THREAD_OVERFLOW");
		}break;
		
		case(THREAD_EXECUTE_OK_WAIT_FOR_SLEEP):{
			strcpy(threadStatus, "THREAD_EXECUTE_OK_WAIT_FOR_SLEEP");
		}break;
		
		case(THREAD_EXECUTE_OK_WAKEUP_FROM_SLEEP_GO_IDLE):{
			strcpy(threadStatus, "THREAD_EXECUTE_OK_WAKEUP_FROM_SLEEP_GO_IDLE");
		}break;
	}
	
	char debOut2[256];
	char timerUnitsMeasurement[32];
	if( thisTask->taskStatus == THREAD_OVERFLOW){
		if(thisTask->timerFormat == tUnitsMilliseconds){
			strcpy(timerUnitsMeasurement, "ms");
		}
		else if(thisTask->timerFormat == tUnitsMicroseconds){
			strcpy(timerUnitsMeasurement, "us");
		} 
		else{
			strcpy(timerUnitsMeasurement, "-");
		}
		sprintf(debOut2, "[%s]. Thread requires at least (%d) %s. ", threadStatus, thisTask->remainingThreadTime, timerUnitsMeasurement);
	}
	else{
		sprintf(debOut2, "[%s]. ", threadStatus);
	}
	
	int TGDSDebuggerStage = 10;
	u8 fwNo = *(u8*)(0x027FF000 + 0x5D);
	handleDSInitOutputMessage((char*)debOut2);
	handleDSInitError(TGDSDebuggerStage, (u32)fwNo);
	
	while(1==1){
		HaltUntilIRQ();
	}
}

//////////////////////////////////////////////////////////////////////// Threading User code end /////////////////////////////////////////////////////////////////////////////