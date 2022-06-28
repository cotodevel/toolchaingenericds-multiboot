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

//TCP
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <in.h>
#include <string.h>

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
	printf("ToolchainGenericDS-multiboot (arm9.bin):");
	printf("                              ");
	printf("Button (Start): File browser ");
	printf("    Button (A) Load TGDS/devkitARM NDS Binary. ");
	printf("                              ");
	printf("(X): Remoteboot >%d");
	printf("(Select): back to Loader. >%d", TGDSPrintfColor_Green);
	printf("Available heap memory: %d", getMaxRam());
	printf("Select: this menu");
}

int internalCodecType = SRC_NONE;//Internal because WAV raw decompressed buffers are used if Uncompressed WAV or ADPCM
bool stopSoundStreamUser(){
	return false;
}

void closeSoundUser(){
	//Stubbed. Gets called when closing an audiostream of a custom audio decoder
}

char args[8][MAX_TGDSFILENAME_LENGTH];
char *argvs[8];

int handleRemoteBoot(int portToListen){
	clrscr();
	printf("----");
	printf("----");
	printf("----");
	printf("----");
	printf("handleRemoteBoot start.");

	if(connectDSWIFIAP(DSWNIFI_ENTER_WIFIMODE) == true){
		printf("Connect OK.");
	}
	else{
		printf("Connect failed. Check AP settings.");
	}

	int sockfd, newsockfd, portno, clilen;
	struct sockaddr_in serv_addr, cli_addr;
	
	/* First call to socket() function */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	if (sockfd < 0) {
		printf("ERROR opening socket");
		return -1;
	}
   
	/* Initialize socket structure */
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	portno = portToListen;
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	
	/* Now bind the host address using bind() call.*/
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		printf("ERROR on binding");
		return -1;
	}
		
	/* Now start listening for the clients, here process will
		* go in sleep mode and will wait for the incoming connection
	*/
	char IP[16];
	memset(IP, 0, sizeof(IP));
	printf("RemoteBoot Active. IP:[%s] @ Port: %d", print_ip((uint32)Wifi_GetIP(), IP), portToListen);
	listen(sockfd,5);
	clilen = sizeof(cli_addr);
	
	/* Accept actual connection from the client */
	newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
		
	if (newsockfd < 0) {
		printf("ERROR on accept");
		return -1;
	}
	
	printf("Got connection!");
	
	const char * message =  "GET / HTTP/1.1\r\nHost: 1.1.1.1 \r\n\r\n Connection: keep-alive\r\n\r\n Keep-Alive: 300\r\n";
    if( send(newsockfd, message , strlen(message) , 0) < 0)
    {
        return -1;
    }
	
	char * RemoteBootTGDSPackage = "0:/remotepackage.tar.gz";
	remove(RemoteBootTGDSPackage);
    FILE * tgdspkgFH = fopen(RemoteBootTGDSPackage, "w+");
    if(tgdspkgFH == NULL){
        printf("error. Couldn't open: %s", RemoteBootTGDSPackage);
		return -1;
	}

	/* If connection is established then start communicating */
	char * server_reply = (char *)TGDSARM9Malloc(64*1024);
	int received_len = 0;
	int total_len = 0;
	while( ( received_len = recv(newsockfd, server_reply, 64*1024, 0 ) ) != 0 ) { // if recv returns 0, the socket has been closed.
		if(received_len>0) { // data was received!
			total_len += received_len;
			fwrite(server_reply, 1, received_len, tgdspkgFH);
			
			clrscr();
			printf("----");
			printf("----");
			printf("----");
			printf("Received byte size = %d Total length = %d ", received_len, total_len);
		}
	}
	TGDSARM9Free(server_reply);
	shutdown(newsockfd,0); // good practice to shutdown the socket.
	closesocket(newsockfd); // remove the socket.
    fclose(tgdspkgFH);

	clrscr();
	printf("----");
	printf("----");
	printf("----");
	printf("Received Package OK. TotalSize: %d ", total_len);
	
	switch_dswnifi_mode(dswifi_idlemode);
	TGDSMultibootRunTGDSPackage(RemoteBootTGDSPackage);
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
	bool isTGDSCustomConsole = false;	//set default console or custom console: default console
	GUI_init(isTGDSCustomConsole);
	GUI_clear();
	
	//xmalloc init removes args, so save them
	int i = 0;
	for(i = 0; i < argc; i++){
		argvs[i] = argv[i];
	}

	bool isCustomTGDSMalloc = true;
	setTGDSMemoryAllocator(getProjectSpecificMemoryAllocatorSetup(TGDS_ARM7_MALLOCSTART, TGDS_ARM7_MALLOCSIZE, isCustomTGDSMalloc, TGDSDLDI_ARM7_ADDRESS));
	sint32 fwlanguage = (sint32)getLanguage();
	
	//argv destroyed here because of xmalloc init, thus restore them
	for(i = 0; i < argc; i++){
		argv[i] = argvs[i];
	}

	asm("mcr	p15, 0, r0, c7, c10, 4");
	flush_icache_all();
	flush_dcache_all();	
	switch_dswnifi_mode(dswifi_idlemode);
	
	printf("     ");
	printf("     ");
	
	int ret=FS_init();
	if (ret == 0){
		
	}
	else{
		
	}
	
	/*			TGDS 1.6 Standard ARM9 Init code end	*/
	
	//load TGDS Logo (NDS BMP Image)
	//VRAM A Used by console
	//VRAM C Keyboard and/or TGDS Logo
	
	//TGDS-MB chainload boot? Boot it then
	if(argc > 3){		
		//arg 0: caller binary 
		//arg 1: this binary (toolchaingenericds-multiboot.nds / toolchaingenericds-multiboot.srl)
		//arg 2: the NDS/TWL binary we are chainloading into
		//arg 3: the arg we want to send to the chainloaded binary 
		
		//Libnds compatibility: If (recv) mainARGV fat:/ change to 0:/
		char thisBinary[MAX_TGDSFILENAME_LENGTH];
		memset(thisBinary, 0, sizeof(thisBinary));
		strcpy(thisBinary, argv[2]);
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
		strcpy(curChosenBrowseFile, (char*)&thisBinary[0]); //Arg1:	NDS Binary reloaded (TGDS format because we load directly now)
		char tempArgv[3][MAX_TGDSFILENAME_LENGTH];
		memset(tempArgv, 0, sizeof(tempArgv));
		strcpy(&tempArgv[0][0], (char*)TGDSPROJECTNAME);	
		strcpy(&tempArgv[1][0], (char*)curChosenBrowseFile);	
		strcpy(&tempArgv[2][0], argv[3]);
		addARGV(3, (char*)&tempArgv);	
		
		//TGDS-Multiboot chainload:
		printf("Target: %s", curChosenBrowseFile);
		printf("Argv1: %s", argv[3]); //TGDS target binary ARGV0
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
	
	//Force ARM7 reload once if TWL mode
	else if( (argc < 3) && (strncmp(argv[1],"0:/ToolchainGenericDS-multiboot.srl", 35) != 0) && (__dsimode == true)){
		char startPath[MAX_TGDSFILENAME_LENGTH+1];
		strcpy(startPath,"/");
		strcpy(curChosenBrowseFile, "0:/ToolchainGenericDS-multiboot.srl");
		//Send args
		printf("[Booting %s]", curChosenBrowseFile);
		printf("Want to send argument?");
		printf("(A) Yes: (Start) Choose arg.");
		printf("(B) No. ");
		
		char argv0[MAX_TGDSFILENAME_LENGTH+1];
		memset(argv0, 0, sizeof(argv0));
		int argcCount = 0;
		argcCount++;
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
	
	//ARGV Implementation test
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
			
			
			if(strncmp(ext,".gz", 3) == 0){
				if(TGDSMultibootRunTGDSPackage(curChosenBrowseFile) == false){ //should never reach here, nor even return true. Should fail it returns false
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

		if(remoteBootEnabled == true){
			handleRemoteBoot(80);
			remoteBootEnabled = false;
		}

		handleARM9SVC();	/* Do not remove, handles TGDS services */
		IRQVBlankWait();
	}
	return 0;
}

