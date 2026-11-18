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
#include "timerTGDS.h"
#include "TGDS_threads.h"
#include "powerTGDS.h"
#include "videoGL.h"

//TCP
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <in.h>
#include <string.h>
#include "imagepcx.h"
#include "Texture_Cube.h"
#include "GXPayload.h"

//ARM7 VRAM core
#include "arm7bootldr.h"
#include "arm7bootldr_twl.h"

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
u32 * getTGDSMBV3ARM7Bootloader(){
	if(__dsimode == false){
		swiDecompressLZSSWram((u8*)&arm7bootldr[0], (u8*)TGDS_MB_V3_ARM7_SCRATCHPAD_LZSS_DECOMP_BUF);
	}
	else{
		swiDecompressLZSSWram((u8*)&arm7bootldr_twl[0], (u8*)TGDS_MB_V3_ARM7_SCRATCHPAD_LZSS_DECOMP_BUF);
	}
	return (u32*)TGDS_MB_V3_ARM7_SCRATCHPAD_LZSS_DECOMP_BUF;
}

float camDist = -0.3*4;
float rotateX = 0.0;
float rotateY = 0.0;
bool recoveryFileExists = false;
bool TGDSWirelessAvailable = false;
char curChosenBrowseFile[MAX_TGDSFILENAME_LENGTH];
char lastHomebrewBooted[MAX_TGDSFILENAME_LENGTH];

//Back to loader, based on Whitelisted DLDI names
static char curLoaderNameFromDldiString[MAX_TGDSFILENAME_LENGTH+1];
#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
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

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void menuShow(){
	clrscr();
	printf("                              ");
	printf("ToolchainGenericDS-multiboot:");
	printf("                              ");
	printf("Button (Start): File browser ");
	printf("    Button (A) Load TGDS/devkitARM NDS Binary. ");
	printf("                              ");
	if(TGDSWirelessAvailable == true){
		printf("(X): Remoteboot >%d", TGDSPrintfColor_Yellow);
		printf("    (Server IP detected: %s [Port:%d]) >%d", remoteBooterIPAddr, remoteBooterPort, TGDSPrintfColor_Yellow);
	}
	else{
		printf("Remoteboot disabled. >%d", TGDSPrintfColor_Yellow);
		printf("  - ");
	}
	printf("                              ");
	if(recoveryFileExists == true){
		printf("(B): Restore from %s >%d", RECOVERY_FILENAME, TGDSPrintfColor_Orange);
		printf("                              ");
	}
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
	disableScreenPowerTimeout(); //timeout backlight is disabled when remoteboot is active
	
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
			u32 * payload = getTGDSMBV3ARM7Bootloader();
			if(TGDSMultibootRunNDSPayload(fileBuf, (u8*)payload, 3, (char*)&thisArgv) == false){ //should never reach here, nor even return true. Should fail it returns false
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
		enableScreenPowerTimeout(); //timeout backlight is enabled when remoteboot failed
		return -1;
	}
	enableScreenPowerTimeout(); //timeout backlight is enabled when remoteboot failed
	return -1;
}

/// Sets up the OpenGL state machine environment

GLfloat boxcol[5][3]=
{
	{1.0f,0.0f,0.0f},{1.0f,0.5f,0.0f},{1.0f,1.0f,0.0f},{0.0f,1.0f,0.0f},{0.0f,1.0f,1.0f}
};

GLfloat topcol[5][3]=
{
	{.5f,0.0f,0.0f},{0.5f,0.25f,0.0f},{0.5f,0.5f,0.0f},{0.0f,0.5f,0.0f},{0.0f,0.5f,0.5f}
};

GLuint	box=-1;				// Storage For The Box Display List
GLuint	top=-1;				// Storage For The Top Display List

GLfloat	xrot=0.0;				// Rotates Cube On The X Axis
GLfloat	yrot=0.0;				// Rotates Cube On The Y Axis

#ifdef WIN32
GLuint	texture[1];			// Storage For 1 Texture
#endif

#ifdef ARM9
#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__((optnone))
#endif
#endif
GLvoid BuildLists()
{
	box=glGenLists(2);									// Generate 2 Different Lists
	glNewList(box,GL_COMPILE);							// Start With The Box List
		glBegin(GL_QUADS);
			// Bottom Face
			glNormal3f( 0.0f,-1.0f, 0.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
			glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f, -1.0f, -1.0f);
			glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
			// Front Face
			glNormal3f( 0.0f, 0.0f, 1.0f);
			glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
			glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);
			// Back Face
			glNormal3f( 0.0f, 0.0f,-1.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);
			glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);
			glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -1.0f);
			// Right face
			glNormal3f( 1.0f, 0.0f, 0.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -1.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);
			glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
			glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
			// Left Face
			glNormal3f(-1.0f, 0.0f, 0.0f);
			glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);
			glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);
		glEnd();
	glEndList();
	top=box+1;											// Storage For "Top" Is "Box" Plus One
	glNewList(top,GL_COMPILE);							// Now The "Top" Display List
		glBegin(GL_QUADS);
			// Top Face
			glNormal3f( 0.0f, 1.0f, 0.0f);
			glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);
			glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f,  1.0f,  1.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);
		glEnd();
	glEndList();

	glEnable(GL_TEXTURE_2D); // Enable Texture Mapping 
	glEnable(GL_BLEND);

}

#ifdef ARM9
#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__((optnone))
#endif
#endif
void setupTGDSProjectOpenGLDisplayLists(){
	
	//#1: Load a texture and map each one to a texture slot
	u32 arrayOfTextures[1];
	arrayOfTextures[0] = (u32)&Texture_Cube;  
	int texturesInSlot = LoadLotsOfGLTextures((u32*)&arrayOfTextures, (sizeof(arrayOfTextures)/sizeof(u32)) ); //Implements both glBindTexture and glTexImage2D 
	int i = 0;
	for(i = 0; i < texturesInSlot; i++){
		//printf("Tex. index: %d: Tex. name[%d]", i, getTextureNameFromIndex(i));
	}
	//printf("Free Mem: %d KB", ((int)TGDSARM9MallocFreeMemory()/1024));
	glCallListGX((u32*)&GXPayload); //Run this payload once to force cache flushes on DMA GXFIFO

	BuildLists();
}

/// All hints, culling, fog, light models, depth testing, polygon model
/// and blending are set up here

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__((optnone))
#endif
GLvoid ReSizeGLScene(GLsizei width, GLsizei height)		// resizes the window (GLUT & TGDS GL)
{
	if (height==0)										// Prevent A Divide By Zero By
	{
		height=1;										// Making Height Equal One
	}

	glViewport(0,0,width,height);						// Reset The Current Viewport

	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix

	// Calculate The Aspect Ratio Of The Window
	gluPerspective(45.0f,(GLfloat)width/(GLfloat)height,0.1f,100.0f);

	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glLoadIdentity();									// Reset The Modelview Matrix
}

#ifdef ARM9
#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__((optnone))
#endif
#endif
int InitGL(int argc, char *argv[]){
	glShadeModel(GL_SMOOTH);

	// depth testing used on with less than testing
	int TGDSOpenGLDisplayListWorkBufferSize = (256*1024);
	glInit(TGDSOpenGLDisplayListWorkBufferSize); //NDSDLUtils: Initializes a new videoGL context
	
	glClearColor(255,255,255);		// White Background
	glClearDepth(0x7FFF);		// Depth Buffer Setup
	glEnable(GL_ANTIALIAS|GL_TEXTURE_2D|GL_BLEND); // Enable Texture Mapping + light #0 enabled per scene

	glDisable(GL_LIGHT0|GL_LIGHT1|GL_LIGHT2|GL_LIGHT3);
	REG_IE |= IRQ_VBLANK;
	glClearColor(0,35,195);		// blue green background colour

	/* TGDS 1.65 OpenGL 1.1 Initialization */
	ReSizeGLScene(255, 191);
	glMaterialShinnyness();

	glDisable(GL_CULL_FACE); 
	glCullFace (GL_NONE);
	glColorMaterial(GL_FRONT, GL_DIFFUSE); 
	
	glEnable(GL_COLOR_MATERIAL);	//allow to mix both glColor3f + light sources (glVertex + glNormal3f)

	setupTGDSProjectOpenGLDisplayLists();
	return 0;
}

int widthScene;	/// the width of the window
int heightScene;	/// the height of the window

// light 0 colours

//https://www.glprogramming.com/red/chapter05.html
//The GL_DIFFUSE parameter probably most closely correlates with what you naturally think of as "the color of a light." 
//It defines the RGBA color of the diffuse light that a particular light source adds to a scene. By default, GL_DIFFUSE is (1.0, 1.0, 1.0, 1.0) for GL_LIGHT0, 
//which produces a bright, white light as shown in the left side of "Plate 13" in Appendix I. 
//The default value for any other light (GL_LIGHT1, ... , GL_LIGHT7) is (0.0, 0.0, 0.0, 0.0).
GLfloat light_diffuse0Scene[4]	= {0.9f, 0.9f, 0.4f, 1.01f}; //WIN32

GLfloat light_ambient0Scene[4]	= {0.1f, 0.1f, 0.1f, 1.0f}; //WIN32
GLfloat light_specular0Scene[4]	= {0.6f, 0.6f, 0.6f, 1.0f}; //WIN32
GLfloat light_position0Scene[4]	= {-1.0f, -1.0f, 1.0f, 0.0f}; //WIN32

// light 1 colours
GLfloat light_ambient1Scene[4]	= {0.1f, 0.1f, 0.1f, 1.0f};
GLfloat light_diffuse1Scene[4]	= {0.45f, 0.45f, 0.45f, 1.0f};
GLfloat light_specular1Scene[4]	= {0.5f, 0.5f, 0.5f, 1.0f};
GLfloat light_position1Scene[4]	= {-2.0f, -5.0f, -5.0f, -1.0f};

//material
GLfloat mat_ambient[]    = { 8.0f, 8.0f, 8.0f, 0.0f }; 
GLfloat mat_diffuse[]    = { 16.0f, 16.0f, 16.0f, 0.0f }; 
GLfloat mat_specular[]   = { 8.0f, 8.0f, 8.0f, 0.0f }; 
GLfloat mat_emission[]   = { 5.0f, 5.0f, 5.0f, 0.0f }; 
GLfloat high_shininess[] = { 128.0f };


/// Renders a single frame of the scene
#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void drawScene(){
	glMatrixMode(GL_TEXTURE); //GX 3D hardware needs this to enable texturing on a frame basis
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	
	// draw element(s) in the scene + light source(s)
	glMaterialShinnyness();		
	updateGXLights(); //Update GX 3D light scene!
	glColor3f(1.0, 1.0, 1.0); //clear last scene color/light vectors

	if(1 == 0){
	}
	else{
		int yloop = 0;
		int xloop= 0;

		float rotateX = 0.0;
		float rotateY = 0.0;

#ifdef ARM9
		scanKeys();
		int pen_delta[2];
		bool isTSCActive = get_pen_delta( &pen_delta[0], &pen_delta[1] );
		if( isTSCActive == false ){
			rotateX = 0.0;
			rotateY = 0.0;
		}
		else{
			rotateX = pen_delta[0];
			rotateY = pen_delta[1];
			if(yrot > 0){
				yrot-=rotateY;
			}
			else{
				yrot+=rotateY;
			}
		
			if(xrot > 0){
				xrot-=rotateX;
			}
			else{
				xrot+=rotateX;
			}
		}
#endif
		//DS doesn't support filtering.
		#ifdef WIN32
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		#endif
		
#ifdef ARM9
	//Clear The Screen And The Depth Buffer
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	todo: implement https://stackoverflow.com/questions/28137027/why-do-i-need-glcleargl-depth-buffer-bit through cuboid tests and rendering into OpenGL
	
	//Set initial view to look at
	gluPerspective(18, 256.0 / 192.0, 0.1, 40);

	static float Xread = 2.0f;

	//Camera perspective from there
	gluLookAt(	0.0, 0.0, 4.0,		//camera possition 
				0.0, 0.0, 0.0,		//look at
				0.0, 20.0, 0.0		//up
	);

	
	scanKeys();
	u32 keysRd= keysDown();
	if(keysRd & KEY_LEFT){
		xrot+=1.0f;
	}
	if(keysRd & KEY_RIGHT){
		xrot-=1.0f;
	}
#endif

		for (yloop=1;yloop<2;yloop++)
		{
			for (xloop=0;xloop<yloop;xloop++)
			{
				// enable texturing
				glEnable(GL_TEXTURE_2D);
	
				glBindTexture(
			#ifdef WIN32		
					GL_TEXTURE_2D, texture[0]
			#endif
			#ifdef ARM9
				0, textureSizePixelCoords[Texture_MetalCubeID].textureIndex
			#endif	
				);

				glLoadIdentity();							// Reset The View
				glTranslatef(1.4f+(((float)xloop)*2.8f)-(((float)yloop)*1.4f),((6.0f-((float)yloop))*2.4f)-7.0f, -20.0f);
				glRotatef(45.0f-(2.0f*yloop)+xrot,1.0f,0.0f,0.0f);
				glRotatef(45.0f+yrot,0.0f,1.0f,0.0f);

				glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient0Scene);
				glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse0Scene);
				glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular0Scene);
				glLightfv(GL_LIGHT0, GL_POSITION, light_position0Scene);
	
				glLightfv(GL_LIGHT1, GL_AMBIENT, light_ambient1Scene);
				glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse1Scene);
				glLightfv(GL_LIGHT1, GL_SPECULAR, light_specular1Scene);
				glLightfv(GL_LIGHT1, GL_POSITION, light_position1Scene);

				#ifdef ARM9
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   mat_ambient); 
				glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   mat_diffuse);
				glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  mat_specular);
				glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,  mat_emission);
				glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, high_shininess);
				#endif
				glScalef(6.0,6.0,6.0);
				glCallList(box);
				glCallList(top);
			}
		}
	}
	
	#ifdef WIN32
	glutSwapBuffers();
	#endif

	#ifdef ARM9
    glFlush();
	#endif
}

//true: pen touch
//false: no tsc activity
#ifdef ARM9
#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__((optnone))
#endif
#endif
bool get_pen_delta( int *dx, int *dy ){
	static int prev_pen[2] = { 0x7FFFFFFF, 0x7FFFFFFF };
	
	// TSC Test.
	struct touchPosition touch;
	XYReadScrPosUser(&touch);
	
	if( (touch.px == 0) && (touch.py == 0) ){
		prev_pen[0] = prev_pen[1] = 0x7FFFFFFF;
		*dx = *dy = 0;
		return false;
	}
	else{
		if( prev_pen[0] != 0x7FFFFFFF ){
			*dx = (prev_pen[0] - touch.px);
			*dy = (prev_pen[1] - touch.py);
		}
		prev_pen[0] = touch.px;
		prev_pen[1] = touch.py;
	}
	return true;
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
	u32 * payload = getTGDSMBV3ARM7Bootloader();
	executeARM7Payload((u32)0x02380000, 96*1024, payload);
	
	bool isTGDSCustomConsole = true;	//set default console or custom console: default console
	GUI_init(isTGDSCustomConsole);
	GUI_clear();

	bool isCustomTGDSMalloc = true;		//default newlib-nds's malloc
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
	setupDisabledExceptionHandler();
	
	//load TGDS Logo (NDS BMP Image)
	//VRAM A Used by console
	//VRAM C Keyboard and/or TGDS Logo
	
	if(FileExists(RECOVERY_FILENAME) == FT_NONE){
		recoveryFileExists = false;
	}
	else{
		recoveryFileExists = true;
	}
	
	//TGDS-MB chainload boot? Boot it then
	if(argc > 2){
		//Arg0:	Chainload caller: TGDS-MB
		//Arg1:	NDS Binary reloaded through ChainLoad
		//Arg2: NDS Binary reloaded through ChainLoad's ARG0
		
		disableScreenPowerTimeout(); //timeout backlight is disabled when loading homebrew
		
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
		
		u32 * payload = getTGDSMBV3ARM7Bootloader();
		if(TGDSMultibootRunNDSPayload(curChosenBrowseFile, (u8*)payload, tempArgc, (char*)&tempArgv) == false){ //should never reach here, nor even return true. Should fail it returns false
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
	
	if(FileExists(TGDSMULTIBOOT_CFG_FILE) == FT_NONE){
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
	bool isTGDSTWLHomebrew = false;
	if( (strlen(lastHomebrewBootedReadFromCFG) > 7) && (isNTROrTWLBinary(lastHomebrewBootedReadFromCFG, &isTGDSTWLHomebrew) != notTWLOrNTRBinary) ){  //0:/x.ext
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
	
	
	//Register threads.
	struct task_Context * TGDSThreads = getTGDSThreadSystem();
	int taskATimeMS = 1; //Task execution requires at least 1ms
	if(registerThread(TGDSThreads, (TaskFn)&taskA, (u32*)NULL, taskATimeMS, (TaskFn)&onThreadOverflowUserCode, tUnitsMilliseconds) != THREAD_OVERFLOW){
        
    }
	
	TGDSWirelessAvailable = isTGDSWirelessServiceAvailable();
	
	//Show logo
	//RenderTGDSLogoMainEngine((uint8*)&TGDSLogoLZSSCompressed[0], TGDSLogoLZSSCompressed_size);
	menuShow();
	bool remoteBootEnabled = false;
	
	//gl start
	InitGL(0, NULL);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glEnable(GL_LIGHT2);
	glEnable(GL_LIGHT3);
	glMaterialShinnyness();
	//gl end

	//powerOFF3DEngine(); //Power off ARM9 3D Engine to save power
	enableScreenPowerTimeout();
	bottomScreenIsLit = true;
	
	while (1){
		scanKeys();
		
		/*
		//GDB debug start
		if (keysDown() & KEY_R){
			disableScreenPowerTimeout(); //timeout backlight is disabled when remoteboot is active
			int TGDSDebuggerStage = 10;
			u8 fwNo = *(u8*)(0x027FF000 + 0x5D);
			sprintf((char*)ConsolePrintfBuf, "ARM9: TGDS-multiboot(): debugging enabled. Halt.");
			handleDSInitOutputMessage((char*)ConsolePrintfBuf);
			handleDSInitError(TGDSDebuggerStage, (u32)fwNo);
		}
		//GDB debug end
		*/
		
		if (keysDown() & KEY_START){
			disableScreenPowerTimeout(); //Default TGDS filebrowser's backlight timeout disabled when loading homebrew 
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
					argcCount=3;
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
			bool isTGDSTWLHomebrew = false;
			int isNTRTWLBinary = isNTROrTWLBinary(curChosenBrowseFile, &isTGDSTWLHomebrew);
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
			
			u32 * payload = getTGDSMBV3ARM7Bootloader();
			if(TGDSMultibootRunNDSPayload(curChosenBrowseFile, (u8*)payload, argcCount, (char*)&thisArgv) == false){ //should never reach here, nor even return true. Should fail it returns false
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
				enableScreenPowerTimeout(); //Default TGDS filebrowser's backlight timeout enabled when loading homebrew failed
				menuShow();
			}
		}
		
		if (keysDown() & KEY_SELECT){
			bottomScreenIsLit = true; //input event triggered
			if(__dsimode == false){
				char * loaderName = canGoBackToLoader();
				if(loaderName != NULL){
					char thisArgv[3][MAX_TGDSFILENAME_LENGTH];
					memset(thisArgv, 0, sizeof(thisArgv));
					strcpy(&thisArgv[0][0], loaderName);	//Arg0:	NDS Binary loaded
					strcpy(&thisArgv[1][0], "");				//Arg1: ARGV0
					u32 * payload = getTGDSMBV3ARM7Bootloader();
					TGDSMultibootRunNDSPayload(loaderName, (u8*)payload, 2, (char*)&thisArgv);
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
			else{
				resetNDSHardware();
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
			disableScreenPowerTimeout(); //timeout backlight is disabled when loading homebrew 
			printf("[Booting... Please wait] >%d", TGDSPrintfColor_Red);
			
			char thisArgv[3][MAX_TGDSFILENAME_LENGTH];
			memset(thisArgv, 0, sizeof(thisArgv));
			strcpy(&thisArgv[0][0], TGDSPROJECTNAME);	//Arg0:	This Binary loaded
			strcpy(&thisArgv[1][0], lastHomebrewBooted);	//Arg1:	NDS Binary reloaded
			strcpy(&thisArgv[2][0], "");					//Arg2: NDS Binary ARG0
			u32 * payload = getTGDSMBV3ARM7Bootloader();
			if(TGDSMultibootRunNDSPayload(lastHomebrewBooted, (u8*)payload, 0, (char*)&thisArgv) == false){ //should never reach here, nor even return true. Should fail it returns false
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
				enableScreenPowerTimeout(); //Default TGDS filebrowser's backlight timeout enabled when loading homebrew failed
			}
			scanKeys();
			while(keysHeld() & KEY_Y){
				scanKeys();
			}
		}
		
		if(recoveryFileExists == true){
			

			if (keysDown() & KEY_B){
				scanKeys();
				while(keysHeld() & KEY_B){
					scanKeys();
				}

				disableScreenPowerTimeout(); //timeout backlight is disabled when loading homebrew 
				GUI_clear();
				switch_dswnifi_mode(dswifi_idlemode);
				
				printf("                     ");
				printf("                     ");
				
				printf("[Restore File: %s started] >%d", RECOVERY_FILENAME, TGDSPrintfColor_Orange);
				printf("                     ");
				char logBuf[256];
				if(handleDecompressor(RECOVERY_FILENAME, (char*)&logBuf[0]) == 0){
					printf("[Restore File: %s OK] >%d", RECOVERY_FILENAME, TGDSPrintfColor_Green);
				}
				else{
					printf("[Restore File: %s ERROR] >%d", RECOVERY_FILENAME, TGDSPrintfColor_Red);
				}
				printf("                     ");
				printf("Press Start to turn off the DS. >%d",  TGDSPrintfColor_Orange);
				
				while(1==1){
					scanKeys();
					if(keysDown() & KEY_START){
						shutdownNDSHardware();
					}
				}
				
			}

		}

		//////////////////////////////////////////////////////////////////////////////////////////////
		
		//Handle normal input to turn back on bottom screen 
		u32 pressed = keysDown();
		if(
			(pressed&KEY_TOUCH)
			||
			(pressed&KEY_A)
			||
			(pressed&KEY_B)
			||
			(pressed&KEY_UP)
			||
			(pressed&KEY_DOWN)
			||
			(pressed&KEY_LEFT)
			||
			(pressed&KEY_RIGHT)
			||
			(pressed&KEY_L)
			||
			(pressed&KEY_R)
			){
			bottomScreenIsLit = true; //input event triggered
		}
		
		//////////////////////////////////////////////////////////////////////////////////////////////
		
		if(remoteBootEnabled == true){
			char URLPathRequested[256];
			sprintf(URLPathRequested, "%s%s", remoteBooterIPAddr, (char*)&RemoteBootTGDSPackage[2]);
			handleRemoteBoot((char*)&URLPathRequested[0], remoteBooterPort);
			remoteBootEnabled = false;
		}

		drawScene();
		bool waitForVblank = true;
		int threadsRan = runThreads(TGDSThreads, waitForVblank);
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

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void enableScreenPowerTimeout(){
	setBacklight(TGDS_PROJECT_LIT_SCREENS);
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void disableScreenPowerTimeout(){
	setBacklight(TGDS_PROJECT_LIT_SCREENS);
}

bool bottomScreenIsLit = false;
static int millisecondsElapsed = 0;	

//called 100 times per second
#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void handleTurnOnTurnOffScreenTimeout(){
	millisecondsElapsed ++;
	if (  millisecondsElapsed >= 500 ){
		setBacklight(0);
		millisecondsElapsed = 0;
	}
	//turn on bottom screen if input event
	if(bottomScreenIsLit == true){
		setBacklight(TGDS_PROJECT_LIT_SCREENS);
		bottomScreenIsLit = false;
		millisecondsElapsed = 0;
	}
}

//////////////////////////////////////////////////////// Threading User code start : TGDS Project specific ////////////////////////////////////////////////////////
//User callback when Task Overflows. Intended for debugging purposes only, as normal user code tasks won't overflow if a task is implemented properly.
//	u32 * args = This Task context
#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void onThreadOverflowUserCode(u32 * args){
	struct task_def * thisTask = (struct task_def *)args;
	struct task_Context * parentTaskCtx = thisTask->parentTaskCtx;	//get parent Task Context node 

	char threadStatus[64];
	switch(thisTask->taskStatus){
		case(INVAL_THREAD):{
			strcpy(threadStatus, "INVAL_THREAD");
		}break;
		
		case(THREAD_OVERFLOW):{
			strcpy(threadStatus, "THREAD_OVERFLOW");
		}break;
		
		case(THREAD_EXECUTE_OK_WAIT_FOR_SLEEP):{
			strcpy(threadStatus, "THREAD_EXECUTE_OK_WAIT_FOR_SLEEP");
		}break;
		
		case(THREAD_EXECUTE_OK_WAKEUP_FROM_SLEEP_GO_IDLE):{
			strcpy(threadStatus, "THREAD_EXECUTE_OK_WAKEUP_FROM_SLEEP_GO_IDLE");
		}break;
	}
	
	char debOut2[256];
	char timerUnitsMeasurement[32];
	if( thisTask->taskStatus == THREAD_OVERFLOW){
		if(thisTask->timerFormat == tUnitsMilliseconds){
			strcpy(timerUnitsMeasurement, "ms");
		}
		else if(thisTask->timerFormat == tUnitsMicroseconds){
			strcpy(timerUnitsMeasurement, "us");
		} 
		else{
			strcpy(timerUnitsMeasurement, "-");
		}
		sprintf(debOut2, "[%s]. Thread requires at least (%d) %s. ", threadStatus, thisTask->remainingThreadTime, timerUnitsMeasurement);
	}
	else{
		sprintf(debOut2, "[%s]. ", threadStatus);
	}
	
	int TGDSDebuggerStage = 10;
	u8 fwNo = *(u8*)(0x027FF000 + 0x5D);
	handleDSInitOutputMessage((char*)debOut2);
	handleDSInitError(TGDSDebuggerStage, (u32)fwNo);
	
	while(1==1){
		HaltUntilIRQ();
	}
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
void taskA(u32 * args){
	handleTurnOnTurnOffScreenTimeout();
}

//////////////////////////////////////////////////////////////////////// Threading User code end /////////////////////////////////////////////////////////////////////////////

void glMaterialfCustom(int mode, rgb color)
{
  static uint32 diffuse_ambient = 0;
  static uint32 specular_emission = 0;

  switch(mode) {
    case GL_AMBIENT:
      diffuse_ambient = (color << 16) | (diffuse_ambient & 0xFFFF);
      break;
    case GL_DIFFUSE:
      diffuse_ambient = color | (diffuse_ambient & 0xFFFF0000);
      break;
    case GL_AMBIENT_AND_DIFFUSE:
      diffuse_ambient= color + (color << 16);
      break;
    case GL_SPECULAR:
      specular_emission = color | (specular_emission & 0xFFFF0000);
      break;
    case GL_SHININESS:
      break;
    case GL_EMISSION:
      specular_emission = (color << 16) | (specular_emission & 0xFFFF);
      break;
  }
  
  GFX_DIFFUSE_AMBIENT = diffuse_ambient;
  GFX_SPECULAR_EMISSION = specular_emission;
}

void glViewPort(uint8 x1, uint8 y1, uint8 x2, uint8 y2)
{
  GFX_VIEWPORT = (x1) + (y1 << 8) + (x2 << 16) + (y2 << 24);
}
