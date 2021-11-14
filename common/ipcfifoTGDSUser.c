
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

//TGDS required version: IPC Version: 1.3

//IPC FIFO Description: 
//		struct sIPCSharedTGDS * TGDSIPC = TGDSIPCStartAddress; 														// Access to TGDS internal IPC FIFO structure. 		(ipcfifoTGDS.h)
//		struct sIPCSharedTGDSSpecific * TGDSUSERIPC = (struct sIPCSharedTGDSSpecific *)TGDSIPCUserStartAddress;		// Access to TGDS Project (User) IPC FIFO structure	(ipcfifoTGDSUser.h)

#include "ipcfifoTGDS.h"
#include "ipcfifoTGDSUser.h"
#include "dsregs.h"
#include "dsregs_asm.h"
#include "InterruptsARMCores_h.h"
#include "biosTGDS.h"
#include "loader.h"
#include "dmaTGDS.h"
#include "libutilsShared.h"

#ifdef ARM7
#include <string.h>
#include "main.h"
#include "spifwTGDS.h"
#endif

#ifdef ARM9

#include <stdbool.h>
#include "main.h"
#include "nds_cp15_misc.h"
#include "dldi.h"

#ifdef NTRMODE
#include "arm7bootldr.h"
#endif

#ifdef TWLMODE
#include "arm7ibootldr.h"
#endif

#endif

#ifdef ARM9
__attribute__((section(".itcm")))
#endif
struct sIPCSharedTGDSSpecific* getsIPCSharedTGDSSpecific(){
	struct sIPCSharedTGDSSpecific* sIPCSharedTGDSSpecificInst = (struct sIPCSharedTGDSSpecific*)(TGDSIPCUserStartAddress);
	return sIPCSharedTGDSSpecificInst;
}

#ifdef ARM9
__attribute__((section(".itcm")))
#endif
#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void HandleFifoNotEmptyWeakRef(uint32 cmd1, uint32 cmd2){
	switch (cmd1) {
		//NDS7: 
		#ifdef ARM7
		
		#endif
		
		//NDS9: 
		#ifdef ARM9
		
		#endif
	}
	
}

#ifdef ARM9
__attribute__((section(".itcm")))
#endif
void HandleFifoEmptyWeakRef(uint32 cmd1,uint32 cmd2){
}

//project specific stuff

#ifdef ARM9

void updateStreamCustomDecoder(u32 srcFrmt){

}

void freeSoundCustomDecoder(u32 srcFrmt){

}

#endif


#ifdef ARM9
#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void reloadARM7PlayerPayload(u32 arm7entryaddress, int arm7BootCodeSize){
	#ifdef NTRMODE
	coherent_user_range_by_size((u32)&arm7bootldr[0], arm7BootCodeSize);
	struct sIPCSharedTGDS * TGDSIPC = TGDSIPCStartAddress;
	uint32 * fifomsg = (uint32 *)&TGDSIPC->fifoMesaggingQueueSharedRegion[0];
	setValueSafe(&fifomsg[0], (u32)&arm7bootldr[0]);
	setValueSafe(&fifomsg[1], (u32)arm7BootCodeSize);
	setValueSafe(&fifomsg[2], (u32)arm7entryaddress);
	#endif
	
	#ifdef TWLMODE
	coherent_user_range_by_size((u32)&arm7ibootldr[0], arm7BootCodeSize);
	struct sIPCSharedTGDS * TGDSIPC = TGDSIPCStartAddress;
	uint32 * fifomsg = (uint32 *)&TGDSIPC->fifoMesaggingQueueSharedRegion[0];
	setValueSafe(&fifomsg[0], (u32)&arm7ibootldr[0]);
	setValueSafe(&fifomsg[1], (u32)arm7BootCodeSize);
	setValueSafe(&fifomsg[2], (u32)arm7entryaddress);
	#endif
	SendFIFOWords(FIFO_ARM7_RELOAD, 0xFF);
}
#endif

//Libutils setup: TGDS project doesn't use any libutils extensions.
void setupLibUtils(){
	//libutils:
	
	//Stage 0
	#ifdef ARM9
	initializeLibUtils9(
		NULL, //ARM7 & ARM9
		NULL, //ARM9 
		NULL, //ARM7 & ARM9: void EnableSoundSampleContext(int SndSamplemode)
		NULL, //ARM7 & ARM9: void DisableSoundSampleContext()
		NULL, //ARM9: bool stopSoundStream(struct fd * tgdsStructFD1, struct fd * tgdsStructFD2, int * internalCodecType)
		NULL  //ARM9: void updateStream() 
	);
	#endif
	
	//Stage 1
	#ifdef ARM7
	initializeLibUtils7(
		NULL, //ARM7 & ARM9
		NULL, //ARM7
		NULL, //ARM7
		NULL, //ARM7: void TIMER1Handler()
		NULL, //ARM7: void stopSound()
		NULL, //ARM7: void setupSound()
		NULL, //ARM7: initSoundSampleContext()
		NULL, //ARM7 & ARM9: void EnableSoundSampleContext(int SndSamplemode)
		NULL,  //ARM7 & ARM9: void DisableSoundSampleContext()
		NULL //ARM7: void initARM7Malloc(u32 ARM7MallocStartaddress, u32 ARM7MallocSize);
	);
	#endif
}