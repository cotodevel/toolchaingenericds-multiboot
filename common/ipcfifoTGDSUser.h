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


//inherits what is defined in: ipcfifoTGDS.h
#ifndef __specific_shared_h__
#define __specific_shared_h__

#include "dsregs.h"
#include "dsregs_asm.h"
#include "ipcfifoTGDS.h"
#include "dswnifi.h"
#include "utilsTGDS.h"

#define ARM7COMMAND_RELOADNDS (uint32)(0xFFFFFF01)

struct ndsloader_s
{
	//raw NDS Binary
	u32 sectorTableBootCode[(4 * 1024 * 1024)/512];	//32K bytes in sectors = 4MB file addressed (NDS max binary size)
	int fileSize;
	
	//ARM7 BootCode
	int bootCode7FileSize;
	u32 arm7EntryAddress;
	int sectorOffsetStart7;
	int sectorOffsetEnd7;
	
	//ARM9 BootCode
	int bootCode9FileSize;
	u32 arm9EntryAddress;
	int sectorOffsetStart9;
	int sectorOffsetEnd9;
	
	//SD specific
	int sectorsPerCluster;
};

//reserve 40K from bottom EWRAM as shared IPC buffer: ndsloader_t = 4*3 + 512 + 32768 = exactly 33292 bytes -> EWRAM - 40K shared IPC
#define NDS_LOADER_IPC_CTX_SIZE (40*1024)
#define NDS_LOADER_IPC_CTXADDR		(0x02400000 - (int)NDS_LOADER_IPC_CTX_SIZE)
#define NDS_LOADER_IPC_CTX_CACHED ((struct ndsloader_s*)NDS_LOADER_IPC_CTXADDR)
#define NDS_LOADER_IPC_CTX_UNCACHED ((struct ndsloader_s*)(NDS_LOADER_IPC_CTXADDR | 0x400000))

//---------------------------------------------------------------------------------
struct sIPCSharedTGDSSpecific {
//---------------------------------------------------------------------------------
	uint32 frameCounter7;	//VBLANK counter7
	uint32 frameCounter9;	//VBLANK counter9
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

//NOT weak symbols : the implementation of these is project-defined (here)
extern void HandleFifoNotEmptyWeakRef(uint32 cmd1,uint32 cmd2);
extern void HandleFifoEmptyWeakRef(uint32 cmd1,uint32 cmd2);

extern void boot_nds(void);

#ifdef __cplusplus
}
#endif