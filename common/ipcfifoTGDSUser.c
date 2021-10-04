
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

#ifdef ARM7
#include <string.h>

#include "main.h"
#include "wifi_arm7.h"
#include "spifwTGDS.h"

#endif

#ifdef ARM9

#include <stdbool.h>
#include "main.h"
#include "wifi_arm9.h"
#include "nds_cp15_misc.h"
#include "dldi.h"

#ifdef NTRMODE
#include "arm7bootldr.h"
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
void HandleFifoNotEmptyWeakRef(volatile u32 cmd1){	
	switch (cmd1) {
		//NDS7: 
		#ifdef ARM7
		case(FIFO_ARM7_RELOAD):{
			
			uint32 * fifomsg = (uint32 *)&getsIPCSharedTGDSSpecific()->fifoMesaggingQueue[0];
			u32 arm7entryaddress = getValueSafe(&fifomsg[34]);
			int arm7BootCodeSize = getValueSafe(&fifomsg[33]);
			u32 arm7EntryAddressPhys = getValueSafe(&fifomsg[32]);
			//dmaTransferWord(0, (uint32)arm7EntryAddressPhys, (uint32)arm7entryaddress, (uint32)arm7BootCodeSize);
			memcpy((void *)arm7entryaddress,(const void *)arm7EntryAddressPhys, arm7BootCodeSize);
			reloadARMCore((u32)arm7entryaddress);	//Run Bootstrap7 
		}
		break;
		
		case(FIFO_TGDSMBRELOAD_SETUP):{
			reloadNDSBootstub();
		}
		break;
		
		#endif
		
		//NDS9: 
		#ifdef ARM9
		case(FIFO_ARM7_RELOAD_OK):{
			reloadStatus = 0;
		}
		break;
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

#ifdef NTRMODE
__attribute__((optimize("O0")))
void reloadARM7PlayerPayload(u32 arm7entryaddress, int arm7BootCodeSize){
	coherent_user_range_by_size((u32)&arm7bootldr[0], arm7BootCodeSize);
	uint32 * fifomsg = (uint32 *)&getsIPCSharedTGDSSpecific()->fifoMesaggingQueue[0];
	setValueSafe(&fifomsg[32], (u32)&arm7bootldr[0]);
	setValueSafe(&fifomsg[33], (u32)arm7BootCodeSize);
	setValueSafe(&fifomsg[34], (u32)arm7entryaddress);
	SendFIFOWords(FIFO_ARM7_RELOAD);
}
#endif

#endif