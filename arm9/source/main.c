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
#include "nds_cp15_misc.h"
#include "fileBrowse.h"
#include <stdio.h>
#include "biosTGDS.h"
#include "global_settings.h"
#include "posixHandleTGDS.h"
#include "TGDSMemoryAllocator.h"
#include "dswnifi_lib.h"
#include "wifi_arm9.h"
#include "utilsTGDS.h"
#include "conf.h"
#include "zipDecomp.h"

//TCP
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <in.h>
#include <string.h>

//ARM7 VRAM core
#include "arm7vram.h"
#include "arm7vram_twl.h"

char curChosenBrowseFile[MAX_TGDSFILENAME_LENGTH];
char lastHomebrewBooted[MAX_TGDSFILENAME_LENGTH];

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
	printf("(X): Remoteboot >%d", TGDSPrintfColor_Yellow);
	printf("    (Server IP detected: %s [Port:%d]) >%d", remoteBooterIPAddr, remoteBooterPort, TGDSPrintfColor_Yellow);
	printf("                              ");
	printf("(Y): Boot last homebrew:  >%d", TGDSPrintfColor_Red);
	printf("    [%s]) >%d", lastHomebrewBooted, TGDSPrintfColor_Red);
	printf("                              ");
	printf("(Select): back to Loader. >%d", TGDSPrintfColor_Green);
	printf("Available heap memory: %d", getMaxRam());
}

int internalCodecType = SRC_NONE;//Internal because WAV raw decompressed buffers are used if Uncompressed WAV or ADPCM
bool stopSoundStreamUser(){
	return false;
}

void closeSoundUser(){
	//Stubbed. Gets called when closing an audiostream of a custom audio decoder
}

char args[argvItems][MAX_TGDSFILENAME_LENGTH];	//used by remoteboot
char *argvs[argvItems];
char remoteBooterIPAddr[256];
int remoteBooterPort = 0;

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int handleRemoteBoot(char * URLPathRequestedByGetVerb, int portToConnect){
	clrscr();
	printf("----");
	printf("----");
	printf("----");
	printf("----");
	printf("handleRemoteBoot start [%s]", URLPathRequestedByGetVerb);
	if(connectDSWIFIAP(DSWNIFI_ENTER_WIFIMODE) == true){
		printf("Connect OK.");
	}
	else{
		printf("Connect failed. Check AP settings.");
	}
	
	//Downloadfile
	remove(RemoteBootTGDSPackage);
	char * fileDownloadDir = "0:/";
	if(DownloadFileFromServer(URLPathRequestedByGetVerb, portToConnect, fileDownloadDir) == true){
		printf("Package download OK:");
		printf("%s", RemoteBootTGDSPackage);
	}
	else{
		printf("Package download ERROR:");
		printf("%s", RemoteBootTGDSPackage);
		
		printf("Press A to try again.");
		while(1==1){
			scanKeys();
			if(keysDown() & KEY_A){
				break;
			}
		}
		handleRemoteBoot(URLPathRequestedByGetVerb, portToConnect);
	}
	
	switch_dswnifi_mode(dswifi_idlemode);
	char logBuf[256];
	clrscr();
	printf("----");
	printf("----");
	printf("----");
	printf("NOTE: If the app gets stuck here");
	printf("you WILL need to reformat your SD card");
	printf("due to SD fragmentation");
	remove("0:/descriptor.txt");
	if(handleDecompressor(RemoteBootTGDSPackage, (char*)&logBuf[0]) == 0){
		//Descriptor is always at root SD path: 0:/descriptor.txt
		set_config_file("0:/descriptor.txt");
		char * baseTargetPath = get_config_string("Global", "baseTargetPath", "");
		char * mainApp = get_config_string("Global", "mainApp", "");
		int mainAppCRC32 = get_config_hex("Global", "mainAppCRC32", 0);
		int TGDSSdkCrc32 = get_config_hex("Global", "TGDSSdkCrc32", 0);
		printf("TGDSPKG Unpack OK:%s", RemoteBootTGDSPackage);
		
		//Boot .NDS file! (homebrew only)
		char tmpName[256];
		char ext[256];
		strcpy(tmpName, mainApp);
		separateExtension(tmpName, ext);
		strlwr(ext);
		if(
			(strncmp(ext,".nds", 4) == 0)
			||
			(strncmp(ext,".srl", 4) == 0)
			){
			//Handle special cases
			
			//TWL TGDS-Package trying to run in NTR mode? Error
			if((strncmp(ext,".srl", 4) == 0) && (__dsimode == false)){
				clrscr();
				printf("----");
				printf("----");
				printf("----");
				printf("ToolchainGenericDS-multiboot tried to boot >%d", TGDSPrintfColor_Yellow);
				printf("a TWL mode package.>%d", TGDSPrintfColor_Yellow);
				printf("[MainApp]: %s >%d", mainApp, TGDSPrintfColor_Green);
				printf("NTR mode-only packages supported. >%d", TGDSPrintfColor_Red);
				printf("Turn off the hardware now.");
				while(1==1){
					IRQWait(0, IRQ_VBLANK);
				}				
			}
			
			char fileBuf[256];
			memset(fileBuf, 0, sizeof(fileBuf));
			strcpy(fileBuf, "0:/");
			strcat(fileBuf, baseTargetPath);
			strcat(fileBuf, mainApp);
			printf("Boot:[%s][CRC32:%x]", fileBuf, mainAppCRC32);
			
			char thisArgv[3][MAX_TGDSFILENAME_LENGTH];
			memset(thisArgv, 0, sizeof(thisArgv));
			strcpy(&thisArgv[0][0], TGDSPROJECTNAME);	//Arg0:	This Binary loaded
			strcpy(&thisArgv[1][0], fileBuf);	//Arg1:	NDS Binary reloaded
			strcpy(&thisArgv[2][0], "");					//Arg2: NDS Binary ARG0
			addARGV(3, (char*)&thisArgv);				
			
			if(TGDSMultibootRunNDSPayload(fileBuf) == false){ //should never reach here, nor even return true. Should fail it returns false
				printf("Invalid NDS/TWL Binary >%d", TGDSPrintfColor_Yellow);
				printf("or you are in NTR mode trying to load a TWL binary. >%d", TGDSPrintfColor_Yellow);
				printf("or you are missing the TGDS-multiboot payload in root path. >%d", TGDSPrintfColor_Yellow);
				printf("Press (A) to continue. >%d", TGDSPrintfColor_Yellow);
				while(1==1){
					scanKeys();
					if(keysDown()&KEY_A){
						scanKeys();
						while(keysDown() & KEY_A){
							scanKeys();
						}
						break;
					}
				}
				menuShow();
			}
		}
		else{
			printf("TGDS App not found:[%s][CRC32:%x]", mainApp, mainAppCRC32);
		}	
	}
	else{
		printf("Couldn't unpack TGDSPackage.:%s", RemoteBootTGDSPackage);
		
		printf("Press A to try again.");
		while(1==1){
			scanKeys();
			if(keysDown() & KEY_A){
				break;
			}
		}
		handleRemoteBoot(URLPathRequestedByGetVerb, portToConnect);
		return false;
	}
	return 0;
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int main(int argc, char **argv) {
	
	/*			TGDS 1.6 Standard ARM9 Init code start	*/
	//Save Stage 1: IWRAM ARM7 payload: NTR/TWL (0x03800000)
	memcpy((void *)TGDS_MB_V3_ARM7_STAGE1_ADDR, (const void *)0x02380000, (int)(96*1024));	//
	coherent_user_range_by_size((uint32)TGDS_MB_V3_ARM7_STAGE1_ADDR, (int)(96*1024)); //		also for TWL binaries 
	
	//Execute Stage 2: VRAM ARM7 payload: NTR/TWL (0x06000000)
	u32 * payload = NULL;
	if(__dsimode == false){
		payload = (u32*)&arm7vram[0];	
	}
	else{
		payload = (u32*)&arm7vram_twl[0];
	}
	executeARM7Payload((u32)0x02380000, 96*1024, payload);
	
	bool isTGDSCustomConsole = false;	//set default console or custom console: default console
	GUI_init(isTGDSCustomConsole);
	GUI_clear();

	bool isCustomTGDSMalloc = true;
	setTGDSMemoryAllocator(getProjectSpecificMemoryAllocatorSetup(isCustomTGDSMalloc));
	sint32 fwlanguage = (sint32)getLanguage();
	
	asm("mcr	p15, 0, r0, c7, c10, 4");
	flush_icache_all();
	flush_dcache_all();
	//switch_dswnifi_mode(dswifi_idlemode); //breaks remoteboot, can't be enabled here
	
	printf("   ");
	printf("   ");
	
	int ret=FS_init();
	if (ret != 0){
		printf("%s: FS Init error: %d >%d", TGDSPROJECTNAME, ret, TGDSPrintfColor_Red);
		while(1==1){
			swiDelay(1);
		}
	}
	
	/*			TGDS 1.6 Standard ARM9 Init code end	*/
	
	REG_IME = 0;
	//MPUSet(); //seems to crash reloaded DKARM NTR homebrew if enabled
	//TGDS-Projects -> legacy NTR TSC compatibility
	if(__dsimode == true){
		TWLSetTouchscreenTWLMode();
	}
	REG_IME = 1;
	
	//load TGDS Logo (NDS BMP Image)
	//VRAM A Used by console
	//VRAM C Keyboard and/or TGDS Logo
	
	//TGDS-MB chainload boot? Boot it then
	if(argc > 2){		
		//Arg0:	Chainload caller: TGDS-MB
		//Arg1:	NDS Binary reloaded through ChainLoad
		//Arg2: NDS Binary reloaded through ChainLoad's ARG0
		
		//Libnds compatibility: If (recv) mainARGV fat:/ change to 0:/
		char thisBinary[MAX_TGDSFILENAME_LENGTH];
		memset(thisBinary, 0, sizeof(thisBinary));
		strcpy(thisBinary, argv[1]);
		if(
			(thisBinary[0] == 'f')
			&&
			(thisBinary[1] == 'a')
			&&
			(thisBinary[2] == 't')
			&&
			(thisBinary[3] == ':')
			&&
			(thisBinary[4] == '/')
			){
			char thisBinary2[MAX_TGDSFILENAME_LENGTH];
			memset(thisBinary2, 0, sizeof(thisBinary2));
			strcpy(thisBinary2, "0:/");
			strcat(thisBinary2, &thisBinary[5]);
			
			//copy back
			memset(thisBinary, 0, sizeof(thisBinary));
			strcpy(thisBinary, thisBinary2);
		}
		strcpy(curChosenBrowseFile, thisBinary); //Arg1:	NDS Binary reloaded (TGDS format because we load directly now)
		char tempArgv[3][MAX_TGDSFILENAME_LENGTH];
		int tempArgc = 2;
		memset(tempArgv, 0, sizeof(tempArgv));
		strcpy(&tempArgv[0][0], (char*)TGDSPROJECTNAME);	
		strcpy(&tempArgv[1][0], (char*)curChosenBrowseFile);	
		if( (argc > 2) && (argv[2] != NULL)){
			strcpy(&tempArgv[2][0], argv[2]);
			tempArgc++;
		}
		addARGV(tempArgc, (char*)&tempArgv);	
		
		//TGDS-Multiboot chainload:
		printf("TGDS-Multiboot chainload:");
		printf("Target: %s", curChosenBrowseFile);
		if(argv[2] != NULL){
			printf("Args passed to chainloaded binary: %s ", argv[2]); //TGDS target binary ARGV0
		}
		else{
			printf("No args passed to chainloaded binary."); //TGDS target binary ARGV0
		}
		if(FAT_FileExists(curChosenBrowseFile) == FT_NONE){
			printf("ERROR: Target homebrew >%d", TGDSPrintfColor_Red);
			printf("%s", curChosenBrowseFile);
			printf("does NOT exist. Halting >%d", TGDSPrintfColor_Red);
			while(1==1){}
		}
		
		if(TGDSMultibootRunNDSPayload(curChosenBrowseFile) == false){ //should never reach here, nor even return true. Should fail it returns false
			printf("Invalid NDS/TWL Binary >%d", TGDSPrintfColor_Yellow);
			printf("or you are in NTR mode trying to load a TWL binary. >%d", TGDSPrintfColor_Yellow);
			printf("or you are missing the TGDS-multiboot payload in root path. >%d", TGDSPrintfColor_Yellow);
			printf("Press (A) to continue. >%d", TGDSPrintfColor_Yellow);
			while(1==1){
				scanKeys();
				if(keysDown()&KEY_A){
					scanKeys();
					while(keysDown() & KEY_A){
						scanKeys();
					}
					break;
				}
			}
			menuShow();
		}
		
	}
	
	else if(FileExists(TGDSMULTIBOOT_CFG_FILE) == FT_NONE){
		clrscr();
		printf("----");
		printf("----");
		printf("----");
		printf("ToolchainGenericDS-multiboot requires:");
		printf("%s >%d", (char*)&TGDSMULTIBOOT_CFG_FILE[3], TGDSPrintfColor_Yellow);
		printf("In SD root folder. Turn off the hardware now.");
		while(1==1){
			IRQWait(0, IRQ_VBLANK);
		}
	}
	
	//Descriptor is always at root SD path (TGDSMULTIBOOT_CFG_FILE)
	set_config_file(TGDSMULTIBOOT_CFG_FILE);

	//A proper Remotebooter IP must be written in said file.
	char * baseTargetPath = get_config_string("Global", "tgdsutilsremotebooteripaddr", "");
	strcpy(remoteBooterIPAddr, baseTargetPath);
	remoteBooterPort = get_config_int("Global", "tgdsutilsremotebooterport", 0);
	if((isValidIpAddress((char*)&remoteBooterIPAddr[0]) != true) || (remoteBooterPort == 0)){
		clrscr();
		printf("----");
		printf("----");
		printf("----");
		printf("Remotebooter IP: ");
		printf("%s >%d", (char*)&remoteBooterIPAddr[0], TGDSPrintfColor_Yellow);
		printf("Or port: ");
		printf("%d >%d", remoteBooterPort, TGDSPrintfColor_Yellow);
		printf("written in %s ", (char*)&TGDSMULTIBOOT_CFG_FILE[3]);
		printf("is missing or invalid. Turn off the hardware now.");
		while(1==1){
			IRQWait(0, IRQ_VBLANK);
		}
	}
	
	//Read last homebrew 
	char * lastHomebrewBootedReadFromCFG = get_config_string("Global", "tgdsmultitbootlasthomebrew", "");
	if( (strlen(lastHomebrewBootedReadFromCFG) > 7) && (isNTROrTWLBinary(lastHomebrewBootedReadFromCFG) != notTWLOrNTRBinary) ){  //0:/x.ext
		strcpy(lastHomebrewBooted, lastHomebrewBootedReadFromCFG);
	}
	else{
		strcpy(lastHomebrewBooted, "0:/");
		strcat(lastHomebrewBooted, TGDSPROJECTNAME);
		if(__dsimode == false){
			strcat(lastHomebrewBooted, ".nds");
		}
		else{
			strcat(lastHomebrewBooted, ".srl");
		}	
	}
	//ARGV Implementation test
	if(getTGDSDebuggingState() == true){
		if (0 != argc ) {
			int i;
			for (i=0; i<argc; i++) {
				if (argv[i]) {
					printf("[%d] %s ", i, argv[i]);
				}
			}
		} 
		else {
			printf("No arguments passed!");
		}
	}
	//Show logo
	RenderTGDSLogoMainEngine((uint8*)&TGDSLogoLZSSCompressed[0], TGDSLogoLZSSCompressed_size);
	menuShow();
	bool remoteBootEnabled = false;
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
			
			scanKeys();
			while(!(keysUp() & KEY_A)){
				scanKeys();
			}
			
			//Send args
			printf("[Booting %s]", curChosenBrowseFile);
			printf("Want to send argument?");
			printf("(A) Yes: (Start) Choose arg.");
			printf("(B) No. ");
			
			char argv0[MAX_TGDSFILENAME_LENGTH+1];
			memset(argv0, 0, sizeof(argv0));
			int argcCount = 0;
			
			while(1==1){
				scanKeys();
				if(keysDown()&KEY_A){
					scanKeys();
					while(keysDown() & KEY_A){
						scanKeys();
					}
					
					while( ShowBrowser((char *)startPath, (char *)&argv0[0]) == true ){	//as long you keep using directories ShowBrowser will be true
						
					}
					argcCount++;
					break;
				}
				else if(keysDown()&KEY_B){
					
					break;
				}
			}
			
			printf("[Booting... Please wait] >%d", TGDSPrintfColor_Red);
			
			//Boot .NDS file! (homebrew only)
			char tmpName[256];
			char ext[256];
			strcpy(tmpName, curChosenBrowseFile);
			separateExtension(tmpName, ext);
			strlwr(ext);
			
			char thisArgv[3][MAX_TGDSFILENAME_LENGTH];
			memset(thisArgv, 0, sizeof(thisArgv));
			strcpy(&thisArgv[0][0], TGDSPROJECTNAME);	//Arg0:	This Binary loaded
			strcpy(&thisArgv[1][0], curChosenBrowseFile);	//Arg1:	NDS Binary reloaded
			strcpy(&thisArgv[2][0], argv0);					//Arg2: NDS Binary ARG0
			addARGV(3, (char*)&thisArgv);
			int isNTRTWLBinary = isNTROrTWLBinary(curChosenBrowseFile);
			/*
			//debug start
			if(isNTRTWLBinary == isTWLBinary){
				printf("i'm TWL binary");
			}
			else if(isNTRTWLBinary == isNDSBinaryV1){
				printf("i'm isNDSBinaryV1");
			}
			else if(isNTRTWLBinary == isNDSBinaryV2){
				printf("i'm isNDSBinaryV2");
			}
			else if(isNTRTWLBinary == isNDSBinaryV3){
				printf("i'm isNDSBinaryV3");
			}
			else if(isNTRTWLBinary == isNDSBinaryV1Slot2){
				printf("i'm isNDSBinaryV1Slot2");
			}
			else{
				printf("Not TWL/NTR Binary.");
			}
			while (1==1){}
			//debug start
			*/
			if( 
				(isNTRTWLBinary == isNDSBinaryV1Slot2)
				||
				(isNTRTWLBinary == isNDSBinaryV1)
				||
				(isNTRTWLBinary == isNDSBinaryV2)
				||
				(isNTRTWLBinary == isNDSBinaryV3)
				||
				(isNTRTWLBinary == isTWLBinary)
			){
				//Save last homebrew 
				strcpy(lastHomebrewBooted, curChosenBrowseFile);
				set_config_string("Global", "tgdsmultitbootlasthomebrew", lastHomebrewBooted);
				save_config_file();
			}
			if(TGDSMultibootRunNDSPayload(curChosenBrowseFile) == false){ //should never reach here, nor even return true. Should fail it returns false
				printf("Invalid NDS/TWL Binary >%d", TGDSPrintfColor_Yellow);
				printf("or you are in NTR mode trying to load a TWL binary. >%d", TGDSPrintfColor_Yellow);
				printf("or you are missing the TGDS-multiboot payload in root path. >%d", TGDSPrintfColor_Yellow);
				printf("Press (A) to continue. >%d", TGDSPrintfColor_Yellow);
				while(1==1){
					scanKeys();
					if(keysDown()&KEY_A){
						scanKeys();
						while(keysDown() & KEY_A){
							scanKeys();
						}
						break;
					}
				}
				menuShow();
			}
		}
		
		if (keysDown() & KEY_SELECT){
			char * loaderName = canGoBackToLoader();
			if(loaderName != NULL){
				char thisArgv[3][MAX_TGDSFILENAME_LENGTH];
				memset(thisArgv, 0, sizeof(thisArgv));
				strcpy(&thisArgv[0][0], loaderName);	//Arg0:	NDS Binary loaded
				strcpy(&thisArgv[1][0], "");				//Arg1: ARGV0
				addARGV(2, (char*)&thisArgv);
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
		

		if ( (keysDown() & KEY_X) && (remoteBootEnabled == false)){
			remoteBootEnabled = true;
			scanKeys();
			while(keysHeld() & KEY_X){
				scanKeys();
			}
		}
		
		if (keysDown() & KEY_Y){
			char thisArgv[3][MAX_TGDSFILENAME_LENGTH];
			memset(thisArgv, 0, sizeof(thisArgv));
			strcpy(&thisArgv[0][0], TGDSPROJECTNAME);	//Arg0:	This Binary loaded
			strcpy(&thisArgv[1][0], lastHomebrewBooted);	//Arg1:	NDS Binary reloaded
			strcpy(&thisArgv[2][0], "");					//Arg2: NDS Binary ARG0
			addARGV(3, (char*)&thisArgv);				
			if(TGDSMultibootRunNDSPayload(lastHomebrewBooted) == false){ //should never reach here, nor even return true. Should fail it returns false
				printf("Invalid NDS/TWL Binary >%d", TGDSPrintfColor_Yellow);
				printf("or you are in NTR mode trying to load a TWL binary. >%d", TGDSPrintfColor_Yellow);
				printf("or you are missing the TGDS-multiboot payload in root path. >%d", TGDSPrintfColor_Yellow);
				printf("Press (A) to continue. >%d", TGDSPrintfColor_Yellow);
				while(1==1){
					scanKeys();
					if(keysDown()&KEY_A){
						scanKeys();
						while(keysDown() & KEY_A){
							scanKeys();
						}
						break;
					}
				}
				menuShow();
			}
			scanKeys();
			while(keysHeld() & KEY_Y){
				scanKeys();
			}
		}
		
		if(remoteBootEnabled == true){
			char URLPathRequested[256];
			sprintf(URLPathRequested, "%s%s", remoteBooterIPAddr, (char*)&RemoteBootTGDSPackage[2]);
			handleRemoteBoot((char*)&URLPathRequested[0], remoteBooterPort);
			remoteBootEnabled = false;
		}

		handleARM9SVC();	/* Do not remove, handles TGDS services */
		IRQVBlankWait();
	}
	return 0;
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
bool DownloadFileFromServer(char * downloadAddr, int ServerPort, char * outputPath) {
	
	// C split to save mem
	char cpyBuf[256] = {0};
	strcpy(cpyBuf, downloadAddr);
	char * outBuf = (char *)TGDSARM9Malloc(256*10);
	
	char * ServerDNSTemp = (char*)((char*)outBuf + (0*256));
	char * strPathTemp = (char*)((char*)outBuf + (1*256));
	char strPath[256];
	int matchCount = str_split((char*)cpyBuf, (char*)"/", outBuf, 10, 256);
	strcpy(&strPath[1], strPathTemp);
	strPath[0] = '/';
	
	char ServerDNS[256];
	strcpy(ServerDNS, ServerDNSTemp);
	TGDSARM9Free(outBuf);
	
	//1 dir or more + filename = fullpath
	if(matchCount > 1){
	    int urlLen = strlen(downloadAddr);
	    int startPos = 0;
	    while(downloadAddr[startPos] != '/'){
	        startPos++;
	    }
	    memset(strPath, 0, sizeof(strPath));
	    strcpy(strPath, (char*)&downloadAddr[startPos]);
	}
	
	//get filename from result
	char strFilename[256];
	int fnamePos = 0;
	int topPathLen = strlen((char*)&strPath[1]);
	while(strPath[topPathLen] != '/'){
	    topPathLen--;
	}
	strcpy(strFilename, (char*)&strPath[topPathLen + 1]);
	
	// C end
	
    // Create a TCP socket
    int my_socket = socket( AF_INET, SOCK_STREAM, 0 );
    printf("Created Socket!");

    // Tell the socket to connect to the IP address we found, on specific port (HTTP)
    struct sockaddr_in sain;
	memset(&sain, 0, sizeof(struct sockaddr_in));
    
	sain.sin_family = AF_INET;
    sain.sin_port = htons(ServerPort);
    
	if(
		(ServerDNS[0] == 'w')
		&&
		(ServerDNS[1] == 'w')
		&&
		(ServerDNS[2] == 'w')
		){
		// Find the IP address of the server, with gethostbyname
		struct hostent * myhost = gethostbyname(ServerDNS);
		printf("Resolved DNS & Found IP Address! [Port:%d]", ServerPort);
		sain.sin_addr.s_addr= *( (unsigned long *)(myhost->h_addr_list[0]) );
	}
	else{
		sain.sin_addr.s_addr = inet_addr(ServerDNS);
		printf("Direct IP Address: %s[Port:%d]", ServerDNS, ServerPort);
	}
	
	connect( my_socket,(struct sockaddr *)&sain, sizeof(struct sockaddr_in) );
    printf("Connected to server!");
	
    //Send request
	FILE *file = NULL;
	char logConsole[256];
	int reply_len = 32*1024;
	char * server_reply = (char *)TGDSARM9Malloc(reply_len);
    int total_len = 0;
	char message[256]; 
	sprintf(message, "GET %s HTTP/1.1\r\nHost: %s \r\n\r\n Connection: keep-alive\r\n\r\n Keep-Alive: 300\r\n", strPath,  ServerDNS);
    if( send(my_socket, message , strlen(message) , 0) < 0){
        return false;
    }
    
	char * tempFile = "0:/temp.pkg";
	char fullFilePath[256];
	sprintf(fullFilePath, "%s%s", outputPath, strFilename);
	remove(fullFilePath);//remove actual file
    remove(tempFile);//remove actual file
    
	file = fopen(tempFile, "w+");
    if(file == NULL){
        return false;
	}
	printf("Download start.");
	int received_len = 0;
	while( ( received_len = recv(my_socket, server_reply, reply_len, 0 ) ) != 0 ) { // if recv returns 0, the socket has been closed.
		if(received_len>0) { // data was received!
			total_len += received_len;
			fwrite(server_reply, 1, received_len, file);
			fsync(fileno(file));
			
			clrscr();
			printf("----");
			printf("----");
			printf("----");
			printf("Received byte size = %d Total length = %d ", received_len, total_len);
		}
		HaltUntilIRQ(); //Save power until next irq
	}
	
	TGDSARM9Free(server_reply);
	shutdown(my_socket,0); // good practice to shutdown the socket.
	closesocket(my_socket); // remove the socket.
    fclose(file);
	
	FILE *fp1;
    FILE *fp2;
    if ((fp1 = fopen(tempFile, "r")) == NULL){
		printf("fail: %s", tempFile);
		while(1==1){}
	}
	fseek(fp1, 0x35, SEEK_SET);
    if ((fp2 = fopen(fullFilePath, "w+")) == NULL){
		printf("fail: %s", fullFilePath);
		while(1==1){}
	}
	fcopy(fp1, fp2);
	fsync(fileno(fp2));
	fclose(fp1);
	fclose(fp2);
	
	printf("Download OK @ SD path: %s", fullFilePath);
	return true;
}