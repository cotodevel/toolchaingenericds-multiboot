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

void runBootstrapARM7(void){
	#ifdef ARM9
	SendFIFOWords(ARM7COMMAND_RELOADNDS, 0);
	#endif
	
	#ifdef ARM7
	reloadARMCore((u32)NDS_LOADER_IPC_BOOTSTUBARM7_CACHED);	//Run Bootstrap7 
	#endif	
}

void reloadARMCore(u32 targetAddress){
	#ifdef ARM7
	*((vu32*)0x027FFE34) = (u32)targetAddress;
	#endif
	#ifdef ARM9
	*((vu32*)0x027FFE24) = (u32)targetAddress;
	#endif
	
	REG_IME = IME_DISABLE;	// Disable interrupts
	REG_IF = REG_IF;	// Acknowledge interrupt
	
	swiSoftReset();	// Jump to boot loader
}

//Waits while a given status is NOT set
void waitWhileNotSetStatus(u32 status){
	while(NDS_LOADER_IPC_CTX_UNCACHED->ndsloaderInitStatus != status){
		swiDelay(111);	
	}
}

//Waits while a given status is set
void waitWhileSetStatus(u32 status){
	while(NDS_LOADER_IPC_CTX_UNCACHED->ndsloaderInitStatus == status){
		swiDelay(111);	
	}
}

void setNDSLoaderInitStatus(int ndsloaderStatus){
	NDS_LOADER_IPC_CTX_UNCACHED->ndsloaderInitStatus = ndsloaderStatus;
}
