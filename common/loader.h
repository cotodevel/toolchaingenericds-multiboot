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

#ifndef __loader_common_h__
#define __loader_common_h__

//enable GDB debug
//#define NDSGDB_DEBUG_ENABLE

#include "dsregs.h"
#include "dsregs_asm.h"
#include "ipcfifoTGDS.h"
#include "dswnifi.h"
#include "utilsTGDS.h"

#define ARM7COMMAND_RELOADNDS (u32)(0xFFFFFF01)
#define NDSLOADER_INIT_OK (u32)(0xFF222218)	//Minimal setup: OK (NDSLoader context not generated yet)
#define NDSLOADER_LOAD_OK (u32)(0xFF222219)	//File read OK -> (NDSLoader context generated properly) ARM7 must reload ARM7.bin into arm7 target address
#define NDSLOADER_START (u32)(0xFF22221A)	//Reload ARM7.bin -> ARM7 target addresses && ARM9.bin -> ARM9 target addresses and boot NDS homebrew

#define NDSLOADER_ENTERGDB_FROM_ARM7 (u32)(0xFF222220)	//file loaded OK
#define NDSLOADER_INITDLDIARM7_BUSY (u32)(0xFF222221)	//DLDI SETUP ARM9 -> ARM7 Taking place...
#define NDSLOADER_INITDLDIARM7_DONE (u32)(0xFF222224)	//DLDI SETUP ARM9 -> ARM7 Done!

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
	u32 arm7BootCodeOffsetInFile;
	
	//ARM9 BootCode
	int bootCode9FileSize;
	u32 arm9EntryAddress;
	int sectorOffsetStart9;
	int sectorOffsetEnd9;
	u32 arm9BootCodeOffsetInFile;
	
	//SD specific
	int sectorsPerCluster;
	int sectorSize;
	int ndsloaderInitStatus;
};

//Shared IPC : 

//NDSLoaderContext = EWRAM - 33K shared IPC (NDS_LOADER_IPC_CTX_SIZE)
#define NDS_LOADER_IPC_CTX_SIZE (48*1024)	//about 34K used anyway, but for addressing purposes we align it

#define NDS_LOADER_IPC_CTXADDR		(0x02300000 - (int)NDS_LOADER_IPC_CTX_SIZE)	//0x022F4000
#define NDS_LOADER_IPC_CTX_CACHED ((struct ndsloader_s*)NDS_LOADER_IPC_CTXADDR)
#define NDS_LOADER_IPC_CTX_UNCACHED ((struct ndsloader_s*)(NDS_LOADER_IPC_CTXADDR | 0x400000))

//ARM7.bin
#define NDS_LOADER_IPC_ARM7BIN_CACHED		(NDS_LOADER_IPC_CTXADDR - (64*1024))	//0x022E4000
#define NDS_LOADER_IPC_ARM7BIN_UNCACHED		(NDS_LOADER_IPC_ARM7BIN_CACHED | 0x400000)

//ARM7 NDSLoader code: Bootstrap that reloads ARM7.bin into arm7 EntryAddress (arm7bootldr/arm7bootldr.bin)
#define NDS_LOADER_IPC_BOOTSTUBARM7_CACHED		(NDS_LOADER_IPC_ARM7BIN_CACHED - (64*1024))	//0x022D4000	-> same as IWRAMBOOTCODE	(rwx)	: ORIGIN = 0x022D4000, LENGTH = 64K (arm7bootldr.bin) entry address
#define NDS_LOADER_IPC_BOOTSTUBARM7_UNCACHED 	(NDS_LOADER_IPC_BOOTSTUBARM7_CACHED | 0x400000)

//ARM7 DLDI section code
#define NDS_LOADER_DLDISECTION_CACHED		(NDS_LOADER_IPC_BOOTSTUBARM7_CACHED - (16*1024))	//0x022D0000
#define NDS_LOADER_DLDISECTION_UNCACHED 	(NDS_LOADER_DLDISECTION_CACHED | 0x400000)

#endif


#ifdef __cplusplus
extern "C"{
#endif

extern void runBootstrapARM7(void);
extern void setNDSLoaderInitStatus(int ndsloaderStatus);
extern void waitWhileNotSetStatus(u32 status);
extern void waitWhileSetStatus(u32 status);
extern void reloadARMCore(u32 targetAddress);

#ifdef __cplusplus
}
#endif