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
#include "dswnifi_lib.h"
#include "TGDSLogoLZSSCompressed.h"
#include "ipcfifoTGDSUser.h"
#include "fatfslayerTGDS.h"
#include "cartHeader.h"
#include "ff.h"
#include "dldi.h"
#include "loader.h"
#include "dmaTGDS.h"
#include "arm7bootldr.h"
#include "nds_cp15_misc.h"
#include "fileBrowse.h"
#include <stdio.h>
#include "biosTGDS.h"

static const uint32  DLDI_MAGIC_NUMBER = 0xBF8DA5ED;	
static const data_t dldiMagicString[12] = "\xED\xA5\x8D\xBF Chishm";	// Normal DLDI file
static const data_t dldiMagicLoaderString[12] = "\xEE\xA5\x8D\xBF Chishm";	// Different to a normal DLDI file


//Coto: this one copies the DLDI section from EWRAM and relocates it to u32 DldiRelocatedAddress, so all DLDI code can be called and executed from there.
void fixAndRelocateDLDI(u32 dldiSourceInRam){
	//#ifdef ARM7
	u32 DldiRelocatedAddress = (u32)&_dldi_start; //relocation offset
	bool clearBSS = false;
	bool patchStatus = dldiRelocateLoader(clearBSS, DldiRelocatedAddress, dldiSourceInRam);	//Coto: re-use DLDI code to patch the DLDI loaded to us (by the loader itself), so we can relocate the DLDI to whatever the address we want.
	if(patchStatus == true){
		//*((vu32*)0x06202000) = 0x4b4f5450; //PTOK
	}
	else{
		//*((vu32*)0x06202000) = 0x52455450; //PTER	//Coto: means the loader failed to give us a proper DLDI driver. Make sure to launch GBARunner2 through any loader that does DLDI patching.
		while(1==1);
	}
	//#endif
	
	/*
	#ifdef ARM9
	u32 DldiRelocatedAddress = (u32)0x06800000; //relocation offset
	bool clearBSS = false;
	bool patchStatus = dldiRelocateLoader(clearBSS, DldiRelocatedAddress, dldiSourceInRam);	//Coto: re-use DLDI code to patch the DLDI loaded to us (by the loader itself), so we can relocate the DLDI to whatever the address we want.
	if(patchStatus == true){
		*((vu32*)0x06202000) = 0x4b4f5450; //PTOK
	}
	else{
		*((vu32*)0x06202000) = 0x52455450; //PTER	//Coto: means the loader failed to give us a proper DLDI driver. Make sure to launch GBARunner2 through any loader that does DLDI patching.
		while(1==1);
	}
	#endif
	*/
}


bool dldiRelocateLoader(bool clearBSS, u32 DldiRelocatedAddress, u32 dldiSourceInRam)
{
	addr_t memOffset;			// Offset of DLDI after the file is loaded into memory
	addr_t patchOffset;			// Position of patch destination in the file
	addr_t relocationOffset;	// Value added to all offsets within the patch to fix it properly
	addr_t ddmemOffset;			// Original offset used in the DLDI file
	addr_t ddmemStart;			// Start of range that offsets can be in the DLDI file
	addr_t ddmemEnd;			// End of range that offsets can be in the DLDI file
	addr_t ddmemSize;			// Size of range that offsets can be in the DLDI file

	addr_t addrIter;

	data_t *pDH;
	data_t *pAH;

	size_t dldiFileSize = 0;
	
	// Target the DLDI we want to use as stub copy and then relocate it to a DldiRelocatedAddress address
	DLDI_INTERFACE* dldiInterface = (DLDI_INTERFACE*)DldiRelocatedAddress;
	pDH = (data_t*)dldiInterface;
	pAH = (data_t *)dldiSourceInRam;
	
	dldiFileSize = 1 << pAH[DO_driverSize];

	// Copy the DLDI patch into the application
	//dmaCopyWords(0, (void*)pAH, (void*)pDH, dldiFileSize);			//dmaCopyWords (uint8 channel, const void *src, void *dest, uint32 size)
	dmaTransferWord(3, (uint32)pAH, (uint32)pDH, (uint32)dldiFileSize);	//void dmaTransferWord(sint32 dmachannel, uint32 source, uint32 dest, uint32 word_count)


	if (*((u32*)(pDH + DO_ioType)) == DEVICE_TYPE_DLDI) {
		// No DLDI patch
		return false;
	}
	
	if (pDH[DO_driverSize] > pAH[DO_allocatedSpace]) {
		// Not enough space for patch
		return false;
	}
	
	memOffset = DldiRelocatedAddress;	//readAddr (pAH, DO_text_start);
	if (memOffset == 0) {
			memOffset = readAddr (pAH, DO_startup) - DO_code;
	}
	ddmemOffset = readAddr (pDH, DO_text_start);
	relocationOffset = memOffset - ddmemOffset;

	ddmemStart = readAddr (pDH, DO_text_start);
	ddmemSize = (1 << pDH[DO_driverSize]);
	ddmemEnd = ddmemStart + ddmemSize;

	// Remember how much space is actually reserved
	pDH[DO_allocatedSpace] = pAH[DO_allocatedSpace];
	
	// Fix the section pointers in the DLDI @ VRAM header
	writeAddr (pDH, DO_text_start, readAddr (pAH, DO_text_start) + relocationOffset);
	writeAddr (pDH, DO_data_end, readAddr (pAH, DO_data_end) + relocationOffset);
	writeAddr (pDH, DO_glue_start, readAddr (pAH, DO_glue_start) + relocationOffset);
	writeAddr (pDH, DO_glue_end, readAddr (pAH, DO_glue_end) + relocationOffset);
	writeAddr (pDH, DO_got_start, readAddr (pAH, DO_got_start) + relocationOffset);
	writeAddr (pDH, DO_got_end, readAddr (pAH, DO_got_end) + relocationOffset);
	writeAddr (pDH, DO_bss_start, readAddr (pAH, DO_bss_start) + relocationOffset);
	writeAddr (pDH, DO_bss_end, readAddr (pAH, DO_bss_end) + relocationOffset);
	
	// Fix the function pointers in the header
	writeAddr (pDH, DO_startup, readAddr (pAH, DO_startup) + relocationOffset);
	writeAddr (pDH, DO_isInserted, readAddr (pAH, DO_isInserted) + relocationOffset);
	writeAddr (pDH, DO_readSectors, readAddr (pAH, DO_readSectors) + relocationOffset);
	writeAddr (pDH, DO_writeSectors, readAddr (pAH, DO_writeSectors) + relocationOffset);
	writeAddr (pDH, DO_clearStatus, readAddr (pAH, DO_clearStatus) + relocationOffset);
	writeAddr (pDH, DO_shutdown, readAddr (pAH, DO_shutdown) + relocationOffset);
	
	if (pDH[DO_fixSections] & FIX_ALL) { 
		// Search through and fix pointers within the data section of the file
		for (addrIter = (readAddr(pDH, DO_text_start) - ddmemStart); addrIter < (readAddr(pDH, DO_data_end) - ddmemStart); addrIter++) {
			if ((ddmemStart <= readAddr(pAH, addrIter)) && (readAddr(pAH, addrIter) < ddmemEnd)) {
				writeAddr (pAH, addrIter, readAddr(pAH, addrIter) + relocationOffset);
			}
		}
	}
	
	
	if (pDH[DO_fixSections] & FIX_GLUE) { 
		// Search through and fix pointers within the glue section of the file
		for (addrIter = (readAddr(pDH, DO_glue_start) - ddmemStart); addrIter < (readAddr(pDH, DO_glue_end) - ddmemStart); addrIter++) {
			if ((ddmemStart <= readAddr(pAH, addrIter)) && (readAddr(pAH, addrIter) < ddmemEnd)) {
				writeAddr (pAH, addrIter, readAddr(pAH, addrIter) + relocationOffset);
			}
		}
	}
	
	if (pDH[DO_fixSections] & FIX_GOT) { 
		// Search through and fix pointers within the Global Offset Table section of the file
		for (addrIter = (readAddr(pDH, DO_got_start) - ddmemStart); addrIter < (readAddr(pDH, DO_got_end) - ddmemStart); addrIter++) {
			if ((ddmemStart <= readAddr(pAH, addrIter)) && (readAddr(pAH, addrIter) < ddmemEnd)) {
				writeAddr (pAH, addrIter, readAddr(pAH, addrIter) + relocationOffset);
			}
		}
	}
	
	/*
	if (clearBSS && (pDH[DO_fixSections] & FIX_BSS)) { 
		// Initialise the BSS to 0, only if the disc is being re-inited
		for(int i = 0; i < (readAddr(pDH, DO_bss_end) - readAddr(pDH, DO_bss_start)) / 4; i++)
		{
			((uint32_t*)&pAH[readAddr(pDH, DO_bss_start) - ddmemStart])[i] = 0;
		}
		
	}
	*/
	return true;
}


bool dldiPatchLoader (data_t *binData, u32 binSize)
{
	addr_t memOffset;			// Offset of DLDI after the file is loaded into memory
	addr_t patchOffset;			// Position of patch destination in the file
	addr_t relocationOffset;	// Value added to all offsets within the patch to fix it properly
	addr_t ddmemOffset;			// Original offset used in the DLDI file
	addr_t ddmemStart;			// Start of range that offsets can be in the DLDI file
	addr_t ddmemEnd;			// End of range that offsets can be in the DLDI file
	addr_t ddmemSize;			// Size of range that offsets can be in the DLDI file

	addr_t addrIter;

	data_t *pDH;
	data_t *pAH;

	size_t dldiFileSize = 0;
	
	DLDI_INTERFACE* dldiInterface = dldiGet();
	pDH = (data_t*)dldiInterface;
	
	if (*((u32*)(pDH + DO_ioType)) == DEVICE_TYPE_DLDI) 
	{
		/*
		// no dldi present in DSO, lets load it from the file we found earlier if it's available	
		char *dldiPath = getDLDI();
		
		if(strlen(dldiPath) == 0) // No DLDI patch
			return false;
		
		// load and then present
		
		DRAGON_chdir(d_res);
		
		DRAGON_FILE *df = DRAGON_fopen(dldiPath, "r");
		
		u32 fLen = DRAGON_flength(df);
		pDH = safeMalloc(PATCH_SIZE);
		memset(pDH, 0, PATCH_SIZE);
		
		DRAGON_fread(pDH, 1, fLen, df);
		DRAGON_fclose(df);
		*/
		printf("DLDI section not found in NTR binary. ");
	}
	else{
		printf("DLDI section found in NTR binary. Patching... ");
	}
	
	// Find the DLDI reserved space in the file
	patchOffset = quickFind(binData, dldiMagicString, binSize, sizeof(dldiMagicString));

	if (patchOffset < 0) {
		// does not have a DLDI section
		//printf("ERR: NO DLDI SECTION");
		return false;
	}
	
	pAH = &(binData[patchOffset]);

	if (pDH[DO_driverSize] > pAH[DO_allocatedSpace]) 
	{
		// Not enough space for patch
		//printf("ERR: NOT ENOUGH SPACE");
		return false;
	}
	
	dldiFileSize = 1 << pDH[DO_driverSize];

	memOffset = readAddr (pAH, DO_text_start);
	if (memOffset == 0) {
			memOffset = readAddr (pAH, DO_startup) - DO_code;
	}
	ddmemOffset = readAddr (pDH, DO_text_start);
	relocationOffset = memOffset - ddmemOffset;

	ddmemStart = readAddr (pDH, DO_text_start);
	ddmemSize = (1 << pDH[DO_driverSize]);
	ddmemEnd = ddmemStart + ddmemSize;

	// Remember how much space is actually reserved
	pDH[DO_allocatedSpace] = pAH[DO_allocatedSpace];
	// Copy the DLDI patch into the application
	memcpy (pAH, pDH, dldiFileSize);

	// Fix the section pointers in the header
	writeAddr (pAH, DO_text_start, readAddr (pAH, DO_text_start) + relocationOffset);
	writeAddr (pAH, DO_data_end, readAddr (pAH, DO_data_end) + relocationOffset);
	writeAddr (pAH, DO_glue_start, readAddr (pAH, DO_glue_start) + relocationOffset);
	writeAddr (pAH, DO_glue_end, readAddr (pAH, DO_glue_end) + relocationOffset);
	writeAddr (pAH, DO_got_start, readAddr (pAH, DO_got_start) + relocationOffset);
	writeAddr (pAH, DO_got_end, readAddr (pAH, DO_got_end) + relocationOffset);
	writeAddr (pAH, DO_bss_start, readAddr (pAH, DO_bss_start) + relocationOffset);
	writeAddr (pAH, DO_bss_end, readAddr (pAH, DO_bss_end) + relocationOffset);
	// Fix the function pointers in the header
	writeAddr (pAH, DO_startup, readAddr (pAH, DO_startup) + relocationOffset);
	writeAddr (pAH, DO_isInserted, readAddr (pAH, DO_isInserted) + relocationOffset);
	writeAddr (pAH, DO_readSectors, readAddr (pAH, DO_readSectors) + relocationOffset);
	writeAddr (pAH, DO_writeSectors, readAddr (pAH, DO_writeSectors) + relocationOffset);
	writeAddr (pAH, DO_clearStatus, readAddr (pAH, DO_clearStatus) + relocationOffset);
	writeAddr (pAH, DO_shutdown, readAddr (pAH, DO_shutdown) + relocationOffset);

	if (pDH[DO_fixSections] & FIX_ALL) { 
		// Search through and fix pointers within the data section of the file
		for (addrIter = (readAddr(pDH, DO_text_start) - ddmemStart); addrIter < (readAddr(pDH, DO_data_end) - ddmemStart); addrIter++) {
			if ((ddmemStart <= readAddr(pAH, addrIter)) && (readAddr(pAH, addrIter) < ddmemEnd)) {
				writeAddr (pAH, addrIter, readAddr(pAH, addrIter) + relocationOffset);
			}
		}
	}

	if (pDH[DO_fixSections] & FIX_GLUE) { 
		// Search through and fix pointers within the glue section of the file
		for (addrIter = (readAddr(pDH, DO_glue_start) - ddmemStart); addrIter < (readAddr(pDH, DO_glue_end) - ddmemStart); addrIter++) {
			if ((ddmemStart <= readAddr(pAH, addrIter)) && (readAddr(pAH, addrIter) < ddmemEnd)) {
				writeAddr (pAH, addrIter, readAddr(pAH, addrIter) + relocationOffset);
			}
		}
	}

	if (pDH[DO_fixSections] & FIX_GOT) { 
		// Search through and fix pointers within the Global Offset Table section of the file
		for (addrIter = (readAddr(pDH, DO_got_start) - ddmemStart); addrIter < (readAddr(pDH, DO_got_end) - ddmemStart); addrIter++) {
			if ((ddmemStart <= readAddr(pAH, addrIter)) && (readAddr(pAH, addrIter) < ddmemEnd)) {
				writeAddr (pAH, addrIter, readAddr(pAH, addrIter) + relocationOffset);
			}
		}
	}

	/*
	if (pDH[DO_fixSections] & FIX_BSS) { 
		// Initialise the BSS to 0
		memset (&pAH[readAddr(pDH, DO_bss_start) - ddmemStart] , 0, readAddr(pDH, DO_bss_end) - readAddr(pDH, DO_bss_start));
	}
	*/

	return true;
}

addr_t readAddr (data_t *mem, addr_t offset) {
	return ((addr_t*)mem)[offset/sizeof(addr_t)];
}

void writeAddr (data_t *mem, addr_t offset, addr_t value) {
	((addr_t*)mem)[offset/sizeof(addr_t)] = value;
}

addr_t quickFind (const data_t* data, const data_t* search, size_t dataLen, size_t searchLen) {
	const int* dataChunk = (const int*) data;
	int searchChunk = ((const int*)search)[0];
	addr_t i;
	addr_t dataChunkEnd = (addr_t)(dataLen / sizeof(int));

	for ( i = 0; i < dataChunkEnd; i++) {
		if (dataChunk[i] == searchChunk) {
			if ((i*sizeof(int) + searchLen) > dataLen) {
				return -1;
			}
			if (memcmp (&data[i*sizeof(int)], search, searchLen) == 0) {
				return i*sizeof(int);
			}
		}
	}

	return -1;
}

bool GDBEnabled = false;
char curChosenBrowseFile[MAX_TGDSFILENAME_LENGTH+1];

void menuShow(){
	clrscr();
	printf("                              ");
	printf("Start: File browser + load NDS");
	printf("Select: this menu");
}

static inline void initNDSLoader(){
	coherent_user_range_by_size((uint32)NDS_LOADER_DLDISECTION_CACHED, (48*1024) + (64*1024) + (64*1024) + (16*1024));
	dmaFillHalfWord(3, 0, (uint32)NDS_LOADER_DLDISECTION_CACHED, (48*1024) + (64*1024) + (64*1024) + (16*1024));
	
	//copy loader code (arm7bootldr.bin) to ARM7's EWRAM portion while preventing Cache issues
	coherent_user_range_by_size((uint32)&arm7bootldr[0], (int)arm7bootldr_size);					
	memcpy ((void *)NDS_LOADER_IPC_BOOTSTUBARM7_CACHED, (u32*)&arm7bootldr[0], arm7bootldr_size); 	//memcpy ( void * destination, const void * source, size_t num );	//memset(void *str, int c, size_t n)

	setNDSLoaderInitStatus(NDSLOADER_INIT_OK);
}

static u8 * outBuf7 = NULL;
static u8 * outBuf9 = NULL;

//generates a table of sectors out of a given file. It has the ARM7 binary and ARM9 binary
bool fillNDSLoaderContext(char * filename){
	
	FILE * fh = fopen(filename, "r");
	if(fh != NULL){
		
		int headerSize = sizeof(struct sDSCARTHEADER);
		u8 * NDSHeader = (u8 *)malloc(headerSize*sizeof(u8));
		if (fread(NDSHeader, 1, headerSize, fh) != headerSize){
			printf("header read error");
			free(NDSHeader);
			fclose(fh);
			return false;
		}
		printf("header parsed correctly.");
		struct sDSCARTHEADER * NDSHdr = (struct sDSCARTHEADER *)NDSHeader;
		
		//fill data into shared buffers
		int sectorSize = 0;
		if(FF_MAX_SS == FF_MIN_SS){
			sectorSize = (int)FF_MIN_SS;
		}
		else{
			#if (FF_MAX_SS != FF_MIN_SS)
			sectorSize = dldiFs.ssize;	//when dldiFs sectorsize is variable, it's 512 bytes per sector
			#endif
		}
		
		//Common
		int clusterSizeBytes = getDiskClusterSizeBytes();
		int sectorsPerCluster = dldiFs.csize;
		NDS_LOADER_IPC_CTX_UNCACHED->sectorsPerCluster = sectorsPerCluster;
		NDS_LOADER_IPC_CTX_UNCACHED->sectorSize = sectorSize;
		
		//ARM7
		int arm7BootCodeSize = NDSHdr->arm7size;
		u32 arm7BootCodeOffsetInFile = NDSHdr->arm7romoffset;
		NDS_LOADER_IPC_CTX_UNCACHED->arm7EntryAddress = NDSHdr->arm7entryaddress;
		NDS_LOADER_IPC_CTX_UNCACHED->bootCode7FileSize = arm7BootCodeSize;
		int sectorOffsetStart7 = arm7BootCodeOffsetInFile % sectorSize;
		int sectorOffsetEnd7 = (arm7BootCodeOffsetInFile + arm7BootCodeSize - sectorOffsetStart7) % sectorSize;
		NDS_LOADER_IPC_CTX_UNCACHED->sectorOffsetStart7 = sectorOffsetStart7;
		NDS_LOADER_IPC_CTX_UNCACHED->sectorOffsetEnd7 = sectorOffsetEnd7;
		
		//ARM9
		int arm9BootCodeSize = NDSHdr->arm9size;
		u32 arm9BootCodeOffsetInFile = NDSHdr->arm9romoffset;
		NDS_LOADER_IPC_CTX_UNCACHED->arm9EntryAddress = NDSHdr->arm9entryaddress;
		NDS_LOADER_IPC_CTX_UNCACHED->bootCode9FileSize = arm9BootCodeSize;
		int sectorOffsetStart9 = arm9BootCodeOffsetInFile % sectorSize;
		int sectorOffsetEnd9 = (arm9BootCodeOffsetInFile + arm9BootCodeSize - sectorOffsetStart9) % sectorSize;
		NDS_LOADER_IPC_CTX_UNCACHED->sectorOffsetStart9 = sectorOffsetStart9;
		NDS_LOADER_IPC_CTX_UNCACHED->sectorOffsetEnd9 = sectorOffsetEnd9;
		
		clrscr();
		printf("-");
		printf("-");
		
		//Get TGDS file handle
		int fdindex = fileno(fh);
		struct fd * fdinst = getStructFD(fdindex);
		
		//reset FP to make it sector-relative + get file size
		fseek(fh,0,SEEK_END);
		int fileSize = ftell(fh);
		fseek(fh,0,SEEK_SET);
		NDS_LOADER_IPC_CTX_UNCACHED->fileSize = fileSize;
		
		printf("fillNDSLoaderContext():");
		printf("arm7BootCodeSize:%d", arm7BootCodeSize);
		printf("arm7BootCodeOffsetInFile:%x", arm7BootCodeOffsetInFile);
		printf("arm7BootCodeEntryAddress:%x", NDS_LOADER_IPC_CTX_UNCACHED->arm7EntryAddress);
		printf("arm9BootCodeSize:%d", arm9BootCodeSize);
		printf("arm9BootCodeOffsetInFile:%x", arm9BootCodeOffsetInFile);
		printf("arm9BootCodeEntryAddress:%x", NDS_LOADER_IPC_CTX_UNCACHED->arm9EntryAddress);
		
		//Generate cluster filemap
		uint32_t* cluster_table = (uint32_t*)&NDS_LOADER_IPC_CTX_UNCACHED->sectorTableBootCode[0];
		uint32_t  cur_cluster = getStructFDFirstCluster(fdinst);
		while (cur_cluster >= 2 && cur_cluster != 0xFFFFFFFF)
		{
			*cluster_table = clst2sect(&dldiFs, cur_cluster);
			cluster_table++;
			cur_cluster = f_getFat(fdinst->filPtr, cur_cluster);
		}
		*(cluster_table) = 0xFFFFFFFF;
		
		//#define DLDI_CREATE_ARM7_ARM9_BIN
		
		printf("NDSLoader start. ");
		
		#ifdef DLDI_CREATE_ARM7_ARM9_BIN
		FILE * fout7 = fopen("0:/arm7.bin", "w+");
		FILE * fout9 = fopen("0:/arm9.bin", "w+");
		#endif
		
		u8 * outBuf = (u8 *)malloc(sectorSize * sectorsPerCluster);
		
		//Uncached to prevent cache issues right at once
		outBuf7 = (u8 *)(NDS_LOADER_IPC_ARM7BIN_UNCACHED);	//will not be higher than: arm7BootCodeSize
		outBuf9 = (u8 *)(NDS_LOADER_IPC_CTX_UNCACHED->arm9EntryAddress | 0x400000); //will not be higher than: arm9BootCodeSize or 0x2D0000 (2,949,120 bytes)
		
		u8 * outBuf7Seek = outBuf7;
		u8 * outBuf9Seek = outBuf9;
		
		int globalPtr = 0; //this one maps the entire file in 512 bytes (sectorSize)
		u32 cur_clustersector = NDS_LOADER_IPC_CTX_UNCACHED->sectorTableBootCode[0];
		uint32_t data_max = (uint32_t)(fileSize);
		uint32_t data_read = 0;
		while((cur_clustersector != 0xFFFFFFFF) && ((data_read * (sectorSize * sectorsPerCluster)) < data_max) )
		{
			//full sector copy
			memset(outBuf, 0, sectorSize * sectorsPerCluster);
			_dldi_start.ioInterface.readSectors(cur_clustersector, sectorsPerCluster, (void*)(outBuf));
			
			//for each sector per cluster...
			int i = 0;
			for(i = 0; i < sectorsPerCluster; i++){
				
				//copy it into output ARM7 binary
				if ( (globalPtr >= arm7BootCodeOffsetInFile) && (globalPtr < (arm7BootCodeOffsetInFile+arm7BootCodeSize)) ){
					//last part?
					if( ((arm7BootCodeOffsetInFile+arm7BootCodeSize) - globalPtr) > sectorOffsetEnd7){
						memcpy (outBuf7Seek, outBuf + (i*512), 512);	//fwrite(outBuf + (i*512) , 1, 512, fout7);
						outBuf7Seek+=512;
					}
					else{
						memcpy (outBuf7Seek, outBuf + (i*512), sectorOffsetEnd7);	//fwrite(outBuf + (i*512) , 1, sectorOffsetEnd7, fout7);
						outBuf7Seek+=sectorOffsetEnd7;
					}
				}
				
				//copy it into output ARM9 binary
				if ( (globalPtr >= arm9BootCodeOffsetInFile) && (globalPtr < (arm9BootCodeOffsetInFile+arm9BootCodeSize)) ){
					//last part?
					if( ((arm9BootCodeOffsetInFile+arm9BootCodeSize) - globalPtr) > sectorOffsetEnd9){
						memcpy (outBuf9Seek, outBuf + (i*512), 512);	//fwrite(outBuf + (i*512) , 1, 512, fout9);
						outBuf9Seek+=512;
					}
					else{
						memcpy (outBuf9Seek, outBuf + (i*512), sectorOffsetEnd9);	//fwrite(outBuf + (i*512) , 1, sectorOffsetEnd9, fout9);
						outBuf9Seek+=sectorOffsetEnd9;
					}
				}	
				globalPtr +=512;
			}
			
			//ARM7 Range check
			data_read++;
			cur_clustersector = (u32)NDS_LOADER_IPC_CTX_UNCACHED->sectorTableBootCode[data_read];
		}
		
		#ifdef DLDI_CREATE_ARM7_ARM9_BIN
		//Write all at once
		int written7 = fwrite(outBuf7 , 1, arm7BootCodeSize, fout7);
		printf("written: ARM7 %d bytes. [Addr: %x]", written7, outBuf7);
		
		int written9 = fwrite(outBuf9 , 1, arm9BootCodeSize, fout9);
		printf("written: ARM9 %d bytes. [Addr: %x]", written9, outBuf9);
		#endif
		
		#ifndef DLDI_CREATE_ARM7_ARM9_BIN
		printf("ARM7 %d bytes. [Addr: %x]", arm7BootCodeSize, (outBuf7 - 0x400000));
		printf("ARM9 %d bytes. [Addr: %x]", arm9BootCodeSize, (outBuf9 - 0x400000));
		#endif
		
		#ifdef DLDI_CREATE_ARM7_ARM9_BIN
		fclose(fout9);
		fclose(fout7);
		#endif
		
		printf("NDSLoader end. ");
		
		free(outBuf);
		free(NDSHeader);
		fclose(fh);
		int ret=FS_deinit();
		
		//Copy and relocate current TGDS DLDI section into target ARM9 binary
		bool stat = dldiPatchLoader((data_t *)NDS_LOADER_IPC_CTX_UNCACHED->arm9EntryAddress, (u32)arm9BootCodeSize);
		if(stat == false){
			printf("DLDI PATCH ERROR. TURN OFF NDS.");
			while(1==1);
		}
		
		//Assign back VRAM to ARM9
		u8 * VRAM_D_ARM9 = (u8*)0x06860000;
		VRAMBLOCK_SETBANK_D(VRAM_D_LCDC_MODE); //LCDC -- VRAM D       128K  0    -     6860000h-687FFFFh
		
		// Clear VRAM
		dmaFillHalfWord(3, 0, (uint32)VRAM_D_ARM9, (uint32)(128*1024));
		
		// Load the loader/patcher into the correct address
		coherent_user_range_by_size((uint32)(outBuf7 - 0x400000), (int)arm7BootCodeSize);					
		dmaTransferWord(3, (uint32)(outBuf7 - 0x400000), (uint32)VRAM_D_ARM9, arm7BootCodeSize);
		
		// Give the VRAM to the ARM7
		VRAMBLOCK_SETBANK_D(VRAM_D_0x06000000_ARM7);
		
		runBootstrapARM7();	//ARM9 Side						/	
		setNDSLoaderInitStatus(NDSLOADER_LOAD_OK);	//		|	Wait until ARM7.bin is copied back to IWRAM's target address
		waitWhileNotSetStatus(NDSLOADER_START);		//		\
		
		//reload ARM9.bin
		reloadARMCore(NDS_LOADER_IPC_CTX_UNCACHED->arm9EntryAddress);
	}
	return false;
}

int main(int _argc, sint8 **_argv) {
	
	/*			TGDS 1.5 Standard ARM9 Init code start	*/
	bool project_specific_console = true;	//set default console or custom console: custom console
	GUI_init(project_specific_console);
	GUI_clear();
	
	sint32 fwlanguage = (sint32)getLanguage();
	
	printf("     ");
	printf("     ");
	
	int ret=FS_init();
	if (ret == 0)
	{
		printf("FS Init ok.");
	}
	else if(ret == -1)
	{
		printf("FS Init error.");
	}
	switch_dswnifi_mode(dswifi_idlemode);
	/*			TGDS 1.5 Standard ARM9 Init code end	*/
	
	initNDSLoader();	//set up NDSLoader
	
	//load TGDS Logo (NDS BMP Image)
	//VRAM A Used by console
	//VRAM C Keyboard and/or TGDS Logo
	
	//render TGDSLogo from a LZSS compressed file
	RenderTGDSLogoSubEngine((uint8*)&TGDSLogoLZSSCompressed[0], TGDSLogoLZSSCompressed_size);

	menuShow();
	
	while (1){
		scanKeys();
		
		if (keysPressed() & KEY_START){
			char startPath[MAX_TGDSFILENAME_LENGTH+1];
			strcpy(startPath,"/");
			while( ShowBrowser((char *)startPath, (char *)&curChosenBrowseFile[0]) == true ){	//as long you keep using directories ShowBrowser will be true
				
			}
			while(keysPressed() & KEY_START){
				scanKeys();
			}
			fillNDSLoaderContext(curChosenBrowseFile);
		}
		
		if (keysPressed() & KEY_SELECT){
			menuShow();
			while(keysPressed() & KEY_SELECT){
				scanKeys();
			}
		}
		
		if (keysPressed() & KEY_B){
			while(keysPressed() & KEY_B){
				scanKeys();
			}
		}
		
		//GDB Debugging start
		//#ifdef NDSGDB_DEBUG_ENABLE
		if(GDBEnabled == true){
			//GDB Stub Process must run here
			int retGDBVal = remoteStubMain();
			if(retGDBVal == remoteStubMainWIFINotConnected){
				if (switch_dswnifi_mode(dswifi_gdbstubmode) == true){
					//clrscr();
					//Show IP and port here
					//printf("    ");
					//printf("    ");
					printf("[Connect to GDB]: %s", ((getValidGDBMapFile() == true) ? " GDBFile Mode!" : "NDSMemory Mode!"));
					char IP[16];
					printf("Port:%d GDB IP:%s",remotePort, print_ip((uint32)Wifi_GetIP(), IP));
					remoteInit();
				}
				else{
					//GDB Client Reconnect:ERROR
				}
			}
			else if(retGDBVal == remoteStubMainWIFIConnectedGDBDisconnected){
				setWIFISetup(false);
				//clrscr();
				//printf("    ");
				//printf("    ");
				printf("Remote GDB Client disconnected. ");
				printf("Press A to retry this GDB Session. ");
				printf("Press B to reboot NDS GDB Server ");
				
				int keys = 0;
				while(1){
					scanKeys();
					keys = keysPressed();
					if (keys&KEY_A){
						break;
					}
					if (keys&KEY_B){
						break;
					}
					IRQVBlankWait();
				}
				
				if (keys&KEY_B){
					setValidGDBMapFile(false);
					main(0, (sint8**)"");
				}
				
				if (switch_dswnifi_mode(dswifi_gdbstubmode) == true){ // gdbNdsStart() called
					reconnectCount++;
					clrscr();
					//Show IP and port here
					printf("    ");
					printf("    ");
					printf("[Re-Connect to GDB]: %s",((getValidGDBMapFile() == true) ? " GDBFile Mode!" : "NDSMemory Mode!"));
					char IP[16];
					printf("Retries: %d",reconnectCount);
					printf("Port:%d GDB IP:%s", remotePort, print_ip((uint32)Wifi_GetIP(), IP));
					remoteInit();
				}
			}
			
			//GDB Debugging end
			//#endif
		}	
		
		handleARM9SVC();	/* Do not remove, handles TGDS services */
		IRQVBlankWait();
	}
}

