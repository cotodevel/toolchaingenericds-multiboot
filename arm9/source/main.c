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
#include "nds_cp15_misc.h"
#include "fileBrowse.h"
#include <stdio.h>
#include "biosTGDS.h"
#include "global_settings.h"
#include "posixHandleTGDS.h"
#include "TGDSMemoryAllocator.h"

bool GDBEnabled = false;
char curChosenBrowseFile[MAX_TGDSFILENAME_LENGTH+1];

//Back to loader, based on Whitelisted DLDI names
static char curLoaderNameFromDldiString[MAX_TGDSFILENAME_LENGTH+1];
static inline char * canGoBackToLoader(){
	char * dldiName = dldi_tryingInterface();
	if(dldiName != NULL){
		if(strcmp(dldiName, "R4iDSN") == 0){	//R4iGold loader
			strcpy(curLoaderNameFromDldiString, "0:/_DS_MENU.dat_ori");	//this allows to return to original payload code, and autoboot to TGDS-multiboot 
			return (char*)&curLoaderNameFromDldiString[0];
		}
		if(strcmp(dldiName, "Ninjapass X9 (SD Card)") == 0){	//Ninjapass X9SD
			strcpy(curLoaderNameFromDldiString, "0:/loader.nds");
			return (char*)&curLoaderNameFromDldiString[0];
		}
	}
	return NULL;
}

void menuShow(){
	clrscr();
	printf("                              ");
	printf("ToolchainGenericDS-multiboot:");
	printf("                              ");
	printf("Button (Start): File browser ");
	printf("    Button (A) Load TGDS/devkitARM NDS Binary. ");
	printf("                              ");
	printf("(Select): back to Loader. >%d", TGDSPrintfColor_Green);
	printf("Available heap memory: %d", getMaxRam());
	printf("Select: this menu");
	printarm7DebugBuffer();
}

//NTR Bootcode:
__attribute__((section(".itcm")))
void TGDSMultibootRunNDSPayload(char * filename) __attribute__ ((optnone)) __attribute__ ((optnone)) {
	strcpy((char*)(0x02280000 - (MAX_TGDSFILENAME_LENGTH+1)), filename);	//Arg0:	
	
	FILE * tgdsPayloadFh = fopen("0:/tgds_multiboot_payload.bin", "r");
	if(tgdsPayloadFh != NULL){
		fseek(tgdsPayloadFh, 0, SEEK_SET);
		int	tgds_multiboot_payload_size = FS_getFileSizeFromOpenHandle(tgdsPayloadFh);
		fread((u32*)0x02280000, 1, tgds_multiboot_payload_size, tgdsPayloadFh);
		coherent_user_range_by_size(0x02280000, (int)tgds_multiboot_payload_size);
		fclose(tgdsPayloadFh);
		int ret=FS_deinit();
		//Copy and relocate current TGDS DLDI section into target ARM9 binary
		bool stat = dldiPatchLoader((data_t *)0x02280000, (u32)tgds_multiboot_payload_size, (u32)&_io_dldi_stub);
		if(stat == false){
			//printf("DLDI Patch failed. APP does not support DLDI format.");
		}
		REG_IME = 0;
		typedef void (*t_bootAddr)();
		t_bootAddr bootARM9Payload = (t_bootAddr)0x02280000;
		bootARM9Payload();
	}
}

bool stopSoundStreamUser(){
	return false;
}

void closeSoundUser(){
	//Stubbed. Gets called when closing an audiostream of a custom audio decoder
}

int main(int argc, char **argv)  __attribute__ ((optnone)) {
	
	/*			TGDS 1.6 Standard ARM9 Init code start	*/
	bool isTGDSCustomConsole = false;	//set default console or custom console: default console
	GUI_init(isTGDSCustomConsole);
	GUI_clear();
	bool isCustomTGDSMalloc = false;
	setTGDSMemoryAllocator(getProjectSpecificMemoryAllocatorSetup(TGDS_ARM7_MALLOCSTART, TGDS_ARM7_MALLOCSIZE, isCustomTGDSMalloc));
	sint32 fwlanguage = (sint32)getLanguage();
	
	printf("     ");
	printf("     ");
	
	#ifdef ARM7_DLDI
	setDLDIARM7Address((u32 *)TGDSDLDI_ARM7_ADDRESS);	//Required by ARM7DLDI!
	#endif
	
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
	asm("mcr	p15, 0, r0, c7, c10, 4");
	flush_icache_all();
	flush_dcache_all();
	/*			TGDS 1.6 Standard ARM9 Init code end	*/
	
	//load TGDS Logo (NDS BMP Image)
	//VRAM A Used by console
	//VRAM C Keyboard and/or TGDS Logo
	
	//Show logo
	RenderTGDSLogoMainEngine((uint8*)&TGDSLogoLZSSCompressed[0], TGDSLogoLZSSCompressed_size);
	
	//Remove logo and restore Main Engine registers
	//restoreFBModeMainEngine();
	
	menuShow();
	
	while (1){
		scanKeys();
		
		if (keysDown() & KEY_START){
			char startPath[MAX_TGDSFILENAME_LENGTH+1];
			strcpy(startPath,"/");
			while( ShowBrowser((char *)startPath, (char *)&curChosenBrowseFile[0]) == true ){	//as long you keep using directories ShowBrowser will be true
				
			}
			
			scanKeys();
			while((keysDown() & KEY_A) || (keysDown() & KEY_START)){
				scanKeys();
			}
			
			//Send args
			printf("[Booting %s]", curChosenBrowseFile);
			printf("Want to send argument?");
			printf("(A) Yes: (Start) Choose arg.");
			printf("(B) No. ");
			
			char argv0[MAX_TGDSFILENAME_LENGTH+1];
			memset(argv0, 0, sizeof(argv0));
			
			while(1==1){
				scanKeys();
				if(keysDown()&KEY_A){
					scanKeys();
					while(keysDown() & KEY_A){
						scanKeys();
					}
					
					while( ShowBrowser((char *)startPath, (char *)&argv0[0]) == true ){	//as long you keep using directories ShowBrowser will be true
						
					}
					
					break;
				}
				else if(keysDown()&KEY_B){
					
					break;
				}
			}
			
			char thisArgv[3][MAX_TGDSFILENAME_LENGTH];
			memset(thisArgv, 0, sizeof(thisArgv));
			strcpy(&thisArgv[0][0], curChosenBrowseFile);	//Arg0:	NDS Binary loaded
			strcpy(&thisArgv[1][0], argv0);					//Arg1: ARGV0
			addARGV(2, (char*)&thisArgv);
			
			TGDSMultibootRunNDSPayload(curChosenBrowseFile);
		}
		
		if (keysDown() & KEY_SELECT){
			char * loaderName = canGoBackToLoader();
			if(loaderName != NULL){
				TGDSMultibootRunNDSPayload(loaderName);
			}
			else{
				clrscr();
				printf("--");
				printf("Dldi name: %s ", dldi_tryingInterface());
				printf("isn't registered.");
				printf("Press B to exit.");
				scanKeys();
				while(1==1){
					scanKeys();
					if(keysDown()&KEY_B){
						break;
					}
				}
				menuShow();
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
					printf("[Connect to GDB]: NDSMemory Mode!");
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
					keys = keysDown();
					if (keys&KEY_A){
						break;
					}
					if (keys&KEY_B){
						break;
					}
					IRQVBlankWait();
				}
				
				if (keys&KEY_B){
					main(argc, argv);
				}
				
				if (switch_dswnifi_mode(dswifi_gdbstubmode) == true){ // gdbNdsStart() called
					reconnectCount++;
					clrscr();
					//Show IP and port here
					printf("    ");
					printf("    ");
					printf("[Re-Connect to GDB]: NDSMemory Mode!");
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
		IRQWait(IRQ_VBLANK);
	}
}

