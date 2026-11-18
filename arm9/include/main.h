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

#ifndef __main9_h__
#define __main9_h__

#include "typedefsTGDS.h"
#include "dsregs.h"
#include "utilsTGDS.h"
#include "limitsTGDS.h"
#include "dldi.h"
#include "powerTGDS.h"
#include "videoGL.h"

#define TGDSMULTIBOOT_CFG_FILE ((char*)"0:/toolchaingenericds-multiboot-config.txt")
#define RemoteBootTGDSPackage ((char*)"0:/remotepackage.zip")
#define TGDS_PROJECT_LIT_SCREENS (POWMAN_BACKLIGHT_BOTTOM_BIT | POWMAN_BACKLIGHT_TOP_BIT)
#define RECOVERY_FILENAME ((char*)"0:/tgds-zlib-backup.zip")
#define Texture_MetalCubeID ((int)0)

#endif


#ifdef __cplusplus
extern "C" {
#endif

extern bool recoveryFileExists;
extern u32 * getTGDSMBV3ARM7Bootloader();
extern int main(int argc, char **argv);
extern char curChosenBrowseFile[MAX_TGDSFILENAME_LENGTH];
extern struct FileClassList * thisFileList;
extern char args[argvItems][MAX_TGDSFILENAME_LENGTH]; //used by remoteboot
extern char *argvs[argvItems];
extern int handleRemoteBoot(char * ipToConnect, int portToConnect);
extern char remoteBooterIPAddr[256];
extern int remoteBooterPort;
extern bool DownloadFileFromServer(char * downloadAddr, int ServerPort, char * outputPath);
extern char lastHomebrewBooted[MAX_TGDSFILENAME_LENGTH];
extern bool TGDSWirelessAvailable;

//Handle backlight timeout 
extern void enableScreenPowerTimeout();
extern void disableScreenPowerTimeout();
extern void handleTurnOnTurnOffScreenTimeout();
extern bool bottomScreenIsLit;

//////////////////////////////////////////////////////// Threading User code: TGDS Project specific ////////////////////////////////////////////////////////
extern void onThreadOverflowUserCode(u32 * args);
extern void taskA(u32 * args);

extern void glMaterialfCustom(int mode, rgb color);
extern void glViewPort(uint8 x1, uint8 y1, uint8 x2, uint8 y2);

extern int InitGL(int argc, char *argv[]);
extern GLvoid ReSizeGLScene(GLsizei width, GLsizei height);
extern GLint DLSOLIDCUBE0_06F;
extern void glut2SolidCube0_06f();
extern void drawScene();

extern int widthScene;	/// the width of the window
extern int heightScene;	/// the height of the window

// light 0 colours

//https://www.glprogramming.com/red/chapter05.html
//The GL_DIFFUSE parameter probably most closely correlates with what you naturally think of as "the color of a light." 
//It defines the RGBA color of the diffuse light that a particular light source adds to a scene. By default, GL_DIFFUSE is (1.0, 1.0, 1.0, 1.0) for GL_LIGHT0, 
//which produces a bright, white light as shown in the left side of "Plate 13" in Appendix I. 
//The default value for any other light (GL_LIGHT1, ... , GL_LIGHT7) is (0.0, 0.0, 0.0, 0.0).
extern GLfloat light_diffuse0Scene[4]	; //WIN32

extern GLfloat light_ambient0Scene[4]	; //WIN32
extern GLfloat light_specular0Scene[4]	; //WIN32
extern GLfloat light_position0Scene[4]	; //WIN32

// light 1 colours
extern GLfloat light_ambient1Scene[4]	;
extern GLfloat light_diffuse1Scene[4]	;
extern GLfloat light_specular1Scene[4]	;
extern GLfloat light_position1Scene[4]	;

//material
extern GLfloat mat_ambient[]    ; 
extern GLfloat mat_diffuse[]    ; 
extern GLfloat mat_specular[]   ; 
extern GLfloat mat_emission[]   ; 
extern GLfloat high_shininess[] ;
extern bool get_pen_delta( int *dx, int *dy );
extern float xrot;

#ifdef __cplusplus
}
#endif