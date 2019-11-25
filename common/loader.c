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

#include "loader.h"

#include "dsregs.h"
#include "dsregs_asm.h"
#include "ipcfifoTGDS.h"
#include "ipcfifoTGDSUser.h"
#include "InterruptsARMCores_h.h"
#include "biosTGDS.h"

#ifdef ARM7

//this code is targeted to ARMv4, but it is compiled by ARM9 side, and also linked into ARM9 binary. Later by the NDSLoader, it will be moved over ARM7
void ARM7ExecuteNDSLoader(void){
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
						memcpy (entryAddr7 + (data_read * (sectorSize * i)), outBuf, 512); //fwrite(outBuf + (i*512) , 1, 512, fout7);	//memcpy ( void * destination, const void * source, size_t num );
						
					}
					else{
						memcpy (entryAddr7 + (data_read * (sectorSize * i)), outBuf, sectorOffsetEnd7); //fwrite(outBuf + (i*512) , 1, sectorOffsetEnd7, fout7);
					}
				}
				
				//copy it into output ARM9 binary
				if ( (globalPtr >= arm9BootCodeOffsetInFile) && (globalPtr < (arm9BootCodeOffsetInFile+arm9BootCodeSize)) ){
					//last part?
					if( ((arm9BootCodeOffsetInFile+arm9BootCodeSize) - globalPtr) > sectorOffsetEnd9){
						memcpy (entryAddr9 + (data_read * (sectorSize * i)), outBuf, 512);	//fwrite(outBuf + (i*512) , 1, 512, fout9);
					}
					else{
						memcpy (entryAddr9 + (data_read * (sectorSize * i)), outBuf, sectorOffsetEnd9);	//fwrite(outBuf + (i*512) , 1, sectorOffsetEnd9, fout9);
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


/*-------------------------------------------------------------------------
resetMemory_ARM7
Clears all of the NDS's RAM that is visible to the ARM7
Written by Darkain.
Modified by Chishm:
 * Added STMIA clear mem loop
--------------------------------------------------------------------------*/
__attribute__((noinline)) static void resetMemory_ARM7 (void)
{
	int i;
	u8 settings1, settings2;
	
	REG_IME = 0;

	/*
	for (i=0; i<16; i++) {
		SCHANNEL_CR(i) = 0;
		SCHANNEL_TIMER(i) = 0;
		SCHANNEL_SOURCE(i) = 0;
		SCHANNEL_LENGTH(i) = 0;
	}
	*/
	for (i=0x04000400; i<0x04000500; i+=4) {
	  *((u32*)i)=0;
	}
	SOUND_CR = 0;

	//clear out ARM7 DMA channels and timers
	/*
	for (i=0; i<4; i++) {
		DMA_CR(i) = 0;
		DMA_SRC(i) = 0;
		DMA_DEST(i) = 0;
		TIMER_CR(i) = 0;
		TIMER_DATA(i) = 0;
	}
	*/

	for(i=0x040000B0;i<(0x040000B0+0x30);i+=4){
		*((vu32*)i)=0;
	}
	for(i=0x04000100;i<0x04000110;i+=2){
		*((u16*)i)=0;
	}

	//can't do:
	//switch to user mode
	/*
	__asm volatile(
	"mov r6, #0x1F                \n"
	"msr cpsr, r6                 \n"
	:
	:
	: "r6"
	);
	*/
	
	REG_IE = 0;
	REG_IF = REG_IF;
	(*(vu32*)(0x04000000-4)) = 0;  //IRQ_HANDLER ARM7 version
	(*(vu32*)(0x04000000-8)) = ~0; //VBLANK_INTR_WAIT_FLAGS, ARM7 version
	POWER_CR = 1;  //turn off power to stuffs

	/*
	__asm volatile (
	// clear most of EWRAM - except after 0x023FF800, which has DS settings
	"mov r8, #0x02000000		\n"	// Start address part 1 
	"orr r8, r8, #0x8000		\n" // Start address part 2
	"mov r9, #0x02300000		\n" // End address part 1
	"orr r9, r9, #0xff000		\n" // End address part 2
	"orr r9, r9, #0x00800		\n" // End address part 3
	"clear_EXRAM_loop:			\n"
	"stmia r8!, {r0, r1, r2, r3, r4, r5, r6, r7} \n"
	"cmp r8, r9					\n"
	"blt clear_EXRAM_loop		\n"
	:
	:
	: "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9"
	);
	*/
	
	//done by TGDS on launch
	/*
	// Reload DS Firmware settings
	readFirmware2((u32)0x03FE70, 0x1, &settings1);
	readFirmware2((u32)0x03FF70, 0x1, &settings2);
	
	if (settings1 > settings2) {
		readFirmware2((u32)0x03FE00, 0x70, (u8*)0x027FFC80);
	} else {
		readFirmware2((u32)0x03FF00, 0x70, (u8*)0x027FFC80);
	}
	*/
}
#endif


#ifdef ARM9
__attribute__((section(".itcm")))
#endif
void boot_nds(void)
{
	#ifdef ARM7
	resetMemory_ARM7();
	#endif
	
	#ifdef ARM9
	SendFIFOWords(ARM7COMMAND_RELOADNDS, 0);
	#endif
	
	REG_IME = IME_DISABLE;	// Disable interrupts
	REG_IF = REG_IF;	// Acknowledge interrupt
	
	//todo: write ARM7 and ARM9 binaries to target Bootloader addresses
	
	#ifdef ARM7
	*((vu32*)0x027FFE34) = (u32)0x03800000;	// Bootloader start address
	#endif
	#ifdef ARM9
	*((vu32*)0x027FFE24) = (u32)0x02000000;	// Bootloader start address
	#endif
	
	swiSoftReset();	// Jump to boot loader
}


int getNDSLoaderInitStatus(){
	return (NDS_LOADER_IPC_CTX_UNCACHED->ndsloaderInitStatus);
}

void setNDSLoaderInitStatus(int ndsloaderStatus){
	NDS_LOADER_IPC_CTX_UNCACHED->ndsloaderInitStatus = ndsloaderStatus;
}