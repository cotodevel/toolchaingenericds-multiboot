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
#include "biosTGDS.h"
#include "spifwTGDS.h"
#include "posixHandleTGDS.h"
#include "pff.h"
#include "ipcfifoTGDSUser.h"
#include "loader.h"
#include "dldi.h"
#include "exceptionTGDS.h"
#include "dmaTGDS.h"

////////////////////////////////TGDS-MB v3 VRAM Bootcode start////////////////////////////////
FATFS fileHandle;					// Petit-FatFs work area 
char fname[256];
u8 NDSHeaderStruct[4096];
char debugBuf7[256];

/*
//If NTR/TWL Binary
	int isNTRTWLBinary = isNTROrTWLBinaryTGDSMB7(fh);
	//Trying to boot a TWL binary in NTR mode? 
	if(!(isNTRTWLBinary == isNDSBinaryV1) && !(isNTRTWLBinary == isNDSBinaryV2) && !(isNTRTWLBinary == isNDSBinaryV3) && !(isNTRTWLBinary == isTWLBinary) && !(isNTRTWLBinary == isNDSBinaryV1Slot2)){
	}
*/

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int isNTROrTWLBinaryTGDSMB7(FATFS * currentFH){
	int mode = notTWLOrNTRBinary;
	uint8_t fresult;
	int headerSize = sizeof(NDSHeaderStruct);
	u8 passmeRead[24];
	memset(passmeRead, 0, sizeof(passmeRead));
	memset(&NDSHeaderStruct, 0, sizeof(NDSHeaderStruct));
	pf_lseek(0, currentFH);
	UINT nbytes_read;
	pf_read((u8*)&NDSHeaderStruct, headerSize, &nbytes_read, currentFH);
	if(nbytes_read != headerSize){
		return mode;
	}
	else{
		pf_lseek(0xA0, currentFH);
		pf_read((u8*)&passmeRead[0], sizeof(passmeRead), &nbytes_read, currentFH);
	}
	struct sDSCARTHEADER * NDSHdr = (struct sDSCARTHEADER *)&NDSHeaderStruct;
	u32 arm9EntryAddress = NDSHdr->arm9entryaddress;
	u32 arm7EntryAddress = NDSHdr->arm7entryaddress;
	u32 arm9BootCodeOffsetInFile = NDSHdr->arm9romoffset;
	int arm7BootCodeSize = NDSHdr->arm7size;
	int arm9BootCodeSize = NDSHdr->arm9size;
	int checkCounter = 0;
	int i = 0;
	for(i = 0; i < sizeof(NDSHdr->reserved1); i++){
		checkCounter += NDSHdr->reserved1[i];
	}
	checkCounter += NDSHdr->reserved2;
	if(
		//(gotDLDISection == false) && //pre-DLDI era could be confused with no filesystem binaries, so skipped.
		(
			//Slot 2 passme v1 (pre 2008 NTR homebrew)
			(0x66 == passmeRead[0x0])
			&&	(0x72 == passmeRead[0x1])
			&&	(0x61 == passmeRead[0x2])
			&&	(0x6D == passmeRead[0x3])
			&&	(0x65 == passmeRead[0x4])
			&&	(0x62 == passmeRead[0x5])
			&&	(0x75 == passmeRead[0x6])
			&&	(0x66 == passmeRead[0x7])
			&&	(0x66 == passmeRead[0x8])
			&&	(0x65 == passmeRead[0x9])
			&&	(0x72 == passmeRead[0xA])
			&&	(0x5F == passmeRead[0xB])
			&&	(0x50 == passmeRead[0xC])
			&&	(0x41 == passmeRead[0xD])
			&&	(0x53 == passmeRead[0xE])
			&&	(0x53 == passmeRead[0xF])
			&&	(0x44 == passmeRead[0x10])
			&&	(0x46 == passmeRead[0x11])
			&&	(0x96 == passmeRead[0x12])
			&&	(0x00 == passmeRead[0x13])
		)
	){
		mode = isNDSBinaryV1Slot2;
	}

	else if(
		(checkCounter == 0) &&
		(arm9EntryAddress >= 0x02000000) &&
		//(gotDLDISection == false) && //pre-DLDI era could be confused with no filesystem binaries, so skipped.
		(
			//Slot 1 passme v1 (pre 2008 NTR homebrew)
			(0x00 == passmeRead[0x0])
			&&	(0x00 == passmeRead[0x1])
			&&	(0x00 == passmeRead[0x2])
			&&	(0x00 == passmeRead[0x3])
			&&	(0x00 == passmeRead[0x4])
			&&	(0x00 == passmeRead[0x5])
			&&	(0x00 == passmeRead[0x6])
			&&	(0x00 == passmeRead[0x7])
			&&	(0x00 == passmeRead[0x8])
			&&	(0x00 == passmeRead[0x9])
			&&	(0x00 == passmeRead[0xA])
			&&	(0x00 == passmeRead[0xB])
			&&	(0x50 == passmeRead[0xC])
			&&	(0x41 == passmeRead[0xD])
			&&	(0x53 == passmeRead[0xE])
			&&	(0x53 == passmeRead[0xF])
			&&	(0x00 == passmeRead[0x10])
			&&	(0x00 == passmeRead[0x11])
			&&	(0x00 == passmeRead[0x12])
			&&	(0x00 == passmeRead[0x13])
		)
	){
		mode = isNDSBinaryV1;
	}
	else if(
		(checkCounter == 0) &&
		(arm9EntryAddress >= 0x02000000) &&
		(
			//Slot 1 passme v2 (pre 2008 NTR homebrew)
			(0x00 == passmeRead[0x0])
			&&	(0x00 == passmeRead[0x1])
			&&	(0x00 == passmeRead[0x2])
			&&	(0x00 == passmeRead[0x3])
			&&	(0x00 == passmeRead[0x4])
			&&	(0x00 == passmeRead[0x5])
			&&	(0x00 == passmeRead[0x6])
			&&	(0x00 == passmeRead[0x7])
			&&	(0x00 == passmeRead[0x8])
			&&	(0x00 == passmeRead[0x9])
			&&	(0x00 == passmeRead[0xA])
			&&	(0x00 == passmeRead[0xB])
			&&	(0x50 == passmeRead[0xC])
			&&	(0x41 == passmeRead[0xD])
			&&	(0x53 == passmeRead[0xE])
			&&	(0x53 == passmeRead[0xF])
			&&	(0x30 == passmeRead[0x10])
			&&	(0x31 == passmeRead[0x11])
			&&	(0x96 == passmeRead[0x12])
			&&	(0x00 == passmeRead[0x13])
		)
	){
		mode = isNDSBinaryV2;
	}
	else if(
		(checkCounter == 0) &&
		(arm9EntryAddress >= 0x02000000) &&
		//(gotDLDISection == true) && //some v2+ homebrew may have the DLDI section stripped (such as barebones demos without filesystem at the time the translation unit built the ARM9 payload)
			//Slot 1 passme v3 (2009+ NTR homebrew)
			(0x53 == passmeRead[0x0])
			&&	(0x52 == passmeRead[0x1])
			&&	(0x41 == passmeRead[0x2])
			&&	(0x4D == passmeRead[0x3])
			&&	(0x5F == passmeRead[0x4])
			&&	(0x56 == passmeRead[0x5])
			&&	(0x31 == passmeRead[0x6])
			&&	(0x31 == passmeRead[0x7])
			&&	(0x30 == passmeRead[0x8])
			&&	(0x00 == passmeRead[0x9])
			&&	(0x00 == passmeRead[0xA])
			&&	(0x00 == passmeRead[0xB])
			&&	(0x50 == passmeRead[0xC])
			&&	(0x41 == passmeRead[0xD])
			&&	(0x53 == passmeRead[0xE])
			&&	(0x53 == passmeRead[0xF])
			&&	(0x30 == passmeRead[0x10])
			&&	(0x31 == passmeRead[0x11])
			&&	(0x96 == passmeRead[0x12])
			&&	(0x00 == passmeRead[0x13])
	){
		mode = isNDSBinaryV3;
	}
	
	//TWL Slot 1 / Internal SD mode: (2009+ TWL homebrew)
	else if( 
		(checkCounter > 0) && (arm9EntryAddress >= 0x02000000) && (arm9EntryAddress <= 0x02FFFFFF) &&
		(
			((arm7EntryAddress >= 0x02000000) && (arm7EntryAddress <= 0x02FFFFFF))
			||
			((arm7EntryAddress >= 0x037F8000) && (arm7EntryAddress <= 0x03810000))
		)
		//&&
		//(gotDLDISection == true) //some homebrew may have the DLDI section stripped (such as barebones demos without filesystem at the time the translation unit built the ARM9 payload)
	){
		mode = isTWLBinary;
	}
	
	//Check for Headerless NTR binary (2004 homebrew on custom devkits, or custom devkits overall)
	if( 
		(arm7EntryAddress >= 0x02000000)
		&&
		(arm9EntryAddress >= 0x02000000)
		&&
		(arm7BootCodeSize > 0)
		&&
		(arm9BootCodeSize > 0)
		&&
		(arm9BootCodeOffsetInFile > 0) //even headerless Passme NTR binaries reserve the NTR header section of 0x200 bytes
		&&
		(mode == notTWLOrNTRBinary)
		&&
		(checkCounter == 0)
	){
		mode = isNDSBinaryV1;
	}
	return mode;
}


#include <stdio.h>

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void strrev(char *arr, int start, int end)
{
    char temp;

    if (start >= end)
        return;

    temp = *(arr + start);
    *(arr + start) = *(arr + end);
    *(arr + end) = temp;

    start++;
    end--;
    strrev(arr, start, end);
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
char *itoa(int number, char *arr, int base)
{
    int i = 0, r, negative = 0;

    if (number == 0)
    {
        arr[i] = '0';
        arr[i + 1] = '\0';
        return arr;
    }

    if (number < 0 && base == 10)
    {
        number *= -1;
        negative = 1;
    }

    while (number != 0)
    {
        r = number % base;
        arr[i] = (r > 9) ? (r - 10) + 'a' : r + '0';
        i++;
        number /= base;
    }

    if (negative)
    {
        arr[i] = '-';
        i++;
    }

    strrev(arr, 0, i - 1);

    arr[i] = '\0';

    return arr;
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("Os")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void bootfile(){
	//Wait for 96K ARM7 mapped @ 0x037f8000 ~ 0x03810000 & Clean IWRAM
	while( !(WRAM_CR & WRAM_0KARM9_32KARM7) ){
		swiDelay(1);
	}
	dmaFillHalfWord(0, 0, ((uint32)0x037f8000), (uint32)(96*1024));
	
	uint8_t fresult;
	FATFS * currentFH = &fileHandle;
	char * filename = (char*)(*ARM9_STRING_PTR);
	strcpy((char*)fname, &filename[2]);
	
	fresult = pf_mount(currentFH);
	if (fresult != FR_OK) {
		//Throw exception
		int stage = 10;
		handleDSInitOutputMessage("TGDS-MB: arm7bootldr/bootfile(): pf_mount() failed");
		handleDSInitError7(stage, (u32)savedDSHardware);
	}
	else{
		fresult = pf_open(fname, currentFH);
		if (fresult != FR_OK) {
			int stage = 10;
			strcpy(debugBuf7, "TGDS-MB: arm7bootldr/bootfile():[");
			strcat(debugBuf7, filename);
			strcat(debugBuf7, "] Open FAIL! Halting.");
			handleDSInitOutputMessage((char*)&debugBuf7[0]);
			handleDSInitError7(stage, (u32)savedDSHardware);
		}
		
		pf_lseek(0, currentFH);
		int isNTRTWLBinary = isNTROrTWLBinaryTGDSMB7(currentFH);
		if(
			(isNTRTWLBinary == isNDSBinaryV1) || (isNTRTWLBinary == isNDSBinaryV2) || (isNTRTWLBinary == isNDSBinaryV3) || (isNTRTWLBinary == isNDSBinaryV1Slot2)
			||
			(isNTRTWLBinary == isTWLBinary)
		){
			///////////////////////////////////////////////////ARM7 Loader start ///////////////////////////////////////////////////////
			
			//Todo: Support isNDSBinaryV1Slot2 binary (Slot2 Passme v1 .ds.gba homebrew)
			if(isNTRTWLBinary == isNDSBinaryV1Slot2){
				setValueSafe((u32*)0x02FFFE24, (u32)0xFFFFFFFF); //ARM9 go handle error
			}
			
			setupDisabledExceptionHandler();
			
			//Clean EWRAM except where tgds_multiboot_payload.bin is running currently (NTR Mirror)
			dmaFillHalfWord(0, 0, ((uint32)0x02000000), (uint32)  (4*1024*1024) - (0x02400000 - ((int)TGDS_MB_V3_PAYLOAD_ADDR)) );
			
			UINT nbytes_read;
			pf_lseek(0, currentFH);
			memset(&NDSHeaderStruct, 0, sizeof(NDSHeaderStruct));
			pf_read((u8*)&NDSHeaderStruct, sizeof(NDSHeaderStruct), &nbytes_read, currentFH);
			struct sDSCARTHEADER * NDSHdr = (struct sDSCARTHEADER *)&NDSHeaderStruct;
			//- ARM9 passes the filename to ARM7
			//- ARM9 waits in ITCM code until ARM7 reloads
			//- ARM7 handles/reloads everything in memory
			//- ARM7 gives permission to ARM9 to reload and ARM7 reloads as well
			//ARM7 reloads here
			int arm7BootCodeSize = NDSHdr->arm7size;
			u32 arm7BootCodeOffsetInFile = NDSHdr->arm7romoffset;
			u32 arm7EntryAddress = NDSHdr->arm7entryaddress;	
			
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
			//Wait until ARM9 arrives at TGDS_MB_payload first
			while( getValueSafe((u32*)0x02FFFE24) != ((u32)0)){
				
			}
			
			int arm9BootCodeSize = NDSHdr->arm9size;
			u32 arm9BootCodeOffsetInFile = NDSHdr->arm9romoffset + dsiARM9headerOffset;
			int arm9BaseOffsetRAM = 0; 
			
			//Check ARM9: If NTRv3, inspect if an ARM branch opcode exist there. If yes, restore arm9BaseOffsetRAM, otherwise there's the secure section, and needs to be copied on base memory
			if((isNTRTWLBinary == isNDSBinaryV3) || (isNTRTWLBinary == isTWLBinary)){
				pf_lseek(arm9BootCodeOffsetInFile, currentFH);
				pf_read((u8*)0x02000000, 4, &nbytes_read, currentFH);
				u32 opcode = ((u32)*(u32*)0x02000000);
				if(
					!(opcode == 0)
					&&
					!(opcode == 0xFFFFFFFF)
				){
					arm9BaseOffsetRAM = (NDSHdr->arm9entryaddress & 0x3FFFFF);
				}
			}
			
			u32 arm9EntryAddress = (((int)(NDSHdr->arm9entryaddress & 0xFF000000)) + arm9BaseOffsetRAM);
			memset((void *)arm9EntryAddress, 0x0, arm9BootCodeSize);
			pf_lseek(arm9BootCodeOffsetInFile, currentFH);
			pf_read((u8*)arm9EntryAddress, arm9BootCodeSize, &nbytes_read, currentFH);
			if( ((int)nbytes_read) != ((int)arm9BootCodeSize) ){
				int stage = 10;
				strcpy(debugBuf7, "TGDS-MB: arm7bootldr/bootfile():[");
				strcat(debugBuf7, filename);
				strcat(debugBuf7, "] ARM9 payload write FAIL...");
				handleDSInitOutputMessage((char*)&debugBuf7[0]);
				handleDSInitError7(stage, (u32)savedDSHardware);
			}
			/*
			else{
				int stage = 10;
				char buffer[sizeof(int) * 10 + 1];
				
				itoa(arm9EntryAddress, buffer, 16);
				
				strcpy(debugBuf7, "TGDS-MB: arm7bootldr/bootfile():");
				strcat(debugBuf7, "ARM9 payload[");
				strcat(debugBuf7, buffer);
				strcat(debugBuf7, "]:size(0x");
				
				itoa(arm9BootCodeSize, buffer, 16);
				
				strcat(debugBuf7, buffer);
				strcat(debugBuf7, ")");
				
				handleDSInitOutputMessage((char*)&debugBuf7[0]);
				handleDSInitError7(stage, (u32)savedDSHardware);
			}
			*/
			
			//so far ok (arm9 twl & arm twl size)
			
			pf_lseek(arm7BootCodeOffsetInFile, currentFH);
			pf_read((u8*)arm7EntryAddress, arm7BootCodeSize, &nbytes_read, currentFH);
			if( ((int)nbytes_read) != ((int)arm7BootCodeSize) ){
				int stage = 10;
				strcpy(debugBuf7, "TGDS-MB: arm7bootldr/bootfile():[");
				strcat(debugBuf7, filename);
				strcat(debugBuf7, "] ARM7 payload write FAIL...");
				handleDSInitOutputMessage((char*)&debugBuf7[0]);
				handleDSInitError7(stage, (u32)savedDSHardware);
			}
			/*
			else{
				int stage = 10;
				char buffer[sizeof(int) * 10 + 1];
				
				itoa(arm7EntryAddress, buffer, 16);
				
				strcpy(debugBuf7, "TGDS-MB: arm7bootldr/bootfile():");
				strcat(debugBuf7, "ARM7 payload[");
				strcat(debugBuf7, buffer);
				strcat(debugBuf7, "]:size(0x");
				
				itoa(arm7BootCodeSize, buffer, 16);
				
				strcat(debugBuf7, buffer);
				strcat(debugBuf7, ")");
				
				handleDSInitOutputMessage((char*)&debugBuf7[0]);
				handleDSInitError7(stage, (u32)savedDSHardware);
			}
			*/
			
			//so far ok (arm7 twl & arm7 twl size)
			
			//Turn off IRQs right now because an interrupt calling to ARM7 exception handler (through bios) crashes ARM7 
			REG_IME = 0;
			REG_IE = 0;
			REG_IF = 0;
			
			//Bios can now be changed @ ARM9 (from ARM7, keep reading)
			//BIOS NTR/TWL switch (backwards compatibility mode): ARM9 has 4004000h - DSi9 - SCFG_A9ROM - ROM Status (R) [0000h] bits as read-only. 
			//Needs to be set on ARM7 side
			if (*((vu32*)(arm9EntryAddress + 0xEC)) == 0xEAFFFFFE){ // b #0; //loop
				*((vu32*)(arm9EntryAddress + 0xEC)) = 0xE3A00000; // mov r0, #0
			}
			
			//Bios can now be changed @ ARM7
			if(__dsimode == true){
				//- DSi7 - SCFG_ROM - ROM Control (R/W, Set-Once)
				u16 * SCFG_ROM = 0x04004000;
				//4004000h - DSi7 - SCFG_ROM - ROM Control (R/W, Set-Once)
				//  0     ARM9 BIOS Upper 32Kbyte of DSi BIOS (0=Enabled, 1=Disabled) (FFFF8xxxh)
				//  1     ARM9 BIOS for NDS Mode              (0=DSi BIOS, 1=NDS BIOS)(FFFF0xxxh)
				//  2-7   Unused (0)
				//  8     ARM7 BIOS Upper 32Kbyte of DSi BIOS (0=Enabled, 1=Disabled)  (0008xxxh)
				//  9     ARM7 BIOS for NDS Mode              (0=DSi BIOS, 1=NDS BIOS) (0000xxxh)
				//  10    Access to Console ID registers      (0=Enabled, 1=Disabled)  (4004Dxxh)
				//  11-15 Unused (0)
				//  16    Unknown, used by bootrom, set to 0  (0=Maybe start ARM9 ?)
				//  17-31 Unused (0)
				//Bits in this register can be set once (cannot be changed back from 1 to 0).
				//Don't change bit1 while executing IRQs or SWI functions on ARM9 side.
				if( 
					(isNTRTWLBinary == isNDSBinaryV1Slot2)
					||
					(isNTRTWLBinary == isNDSBinaryV1)
					||
					(isNTRTWLBinary == isNDSBinaryV2)
					||
					(isNTRTWLBinary == isNDSBinaryV3)
				){
					//Use NTR Bios
					*SCFG_ROM = 0x0703;
				}
				else if(isNTRTWLBinary == isTWLBinary){
					//Use TWL Bios
					*SCFG_ROM = 0x0101;
				}
				else{
					//Throw exception
					int stage = 10;
					strcpy(debugBuf7, "TGDS-MB: arm7bootldr/bootfile():[");
					strcat(debugBuf7, filename);
					strcat(debugBuf7, "] Unknown Mode BIOS! ");
					handleDSInitOutputMessage((char*)&debugBuf7[0]);
					handleDSInitError7(stage, (u32)savedDSHardware);
				}
			}
			
			//so far OK
			/*
			{
				int stage = 10;
				char buffer[sizeof(int) * 10 + 1];
				
				itoa(arm7EntryAddress, buffer, 16);
				
				strcpy(debugBuf7, "TGDS-MB: arm7bootldr/bootfile():");
				strcat(debugBuf7, "ARM7 payload execution [");
				strcat(debugBuf7, buffer);
				strcat(debugBuf7, "]:size(0x");
				
				itoa(arm7BootCodeSize, buffer, 16);
				
				strcat(debugBuf7, buffer);
				strcat(debugBuf7, ")");
				
				handleDSInitOutputMessage((char*)&debugBuf7[0]);
				handleDSInitError7(stage, (u32)savedDSHardware);
			}
			*/
			
			setValueSafe((u32*)ARM9_TWLORNTRPAYLOAD_MODE, (u32)isNTRTWLBinary);
			setValueSafe((u32*)ARM9_BOOT_SIZE, (u32)arm9BootCodeSize);
			setValueSafe((u32*)ARM7_BOOT_SIZE, (u32)arm7BootCodeSize);
			setValueSafe((u32*)ARM7_BOOTCODE_OFST, (u32)arm7BootCodeOffsetInFile);
			setValueSafe((u32*)ARM9_BOOTCODE_OFST, (u32)arm9BootCodeOffsetInFile);
			setValueSafe((u32*)0x02FFFE34, (u32)arm7EntryAddress);
			setValueSafe((u32*)0x02FFFE24, (u32)arm9EntryAddress); //ARM9 go
			
			//Reload ARM7 core
			swiSoftReset();
			///////////////////////////////////////////////////ARM7 Loader end ///////////////////////////////////////////////////////
			
			//Should never read this
			int stage = 10;
			strcpy(debugBuf7, "TGDS-MB: arm7bootldr/bootfile():[");
			strcat(debugBuf7, filename);
			strcat(debugBuf7, "] NTR or TWL Boot failed! Halting. ");
			handleDSInitOutputMessage((char*)&debugBuf7[0]);
			handleDSInitError7(stage, (u32)savedDSHardware);
			
		}
		else{
			int stage = 10;
			strcpy(debugBuf7, "TGDS-MB: arm7bootldr/bootfile():[");
			strcat(debugBuf7, filename);
			strcat(debugBuf7, "] NOT NTR or TWL Binary! Halting.");
			handleDSInitOutputMessage((char*)&debugBuf7[0]);
			handleDSInitError7(stage, (u32)savedDSHardware);
		}
	}
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------
	/*			TGDS 1.6 Standard ARM7 Init code start	*/
	while(!(*(u8*)0x04000240 & 2) ){} //wait for VRAM_D block
	ARM7InitDLDI(TGDS_ARM7_MALLOCSTART, TGDS_ARM7_MALLOCSIZE, TGDSDLDI_ARM7_ADDRESS);
	/*			TGDS 1.6 Standard ARM7 Init code end	*/
	SendFIFOWords(FIFO_ARM7_RELOAD, 0xFF); //ARM7 Reload OK -> acknowledge ARM9
    
	while (1) {
		handleARM7SVC();	/* Do not remove, handles TGDS services */
	}
	return 0;
}

////////////////////////////////TGDS-MB v3 VRAM Bootcode end////////////////////////////////