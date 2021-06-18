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

#include "InterruptsARMCores_h.h"
#include "ipcfifoTGDSUser.h"
#include "dsregs_asm.h"
#include "main.h"
#include "keypadTGDS.h"
#include "interrupts.h"
#include "utilsTGDS.h"
#include "spifwTGDS.h"
#include "videoGL.h"
#include "videoTGDS.h"
#include "math.h"

#include "imagepcx.h"
#include "Cube.h"	//Mesh Generated from Blender 2.49b
//#include "Texture_Cube.h"	//Textures of it

//User Handler Definitions

#ifdef ARM9
__attribute__((section(".itcm")))
#endif
void IpcSynchandlerUser(uint8 ipcByte){
	switch(ipcByte){
		default:{
			//ipcByte should be the byte you sent from external ARM Core through sendByteIPC(ipcByte);
		}
		break;
	}
}

#ifdef ARM9
__attribute__((section(".itcm")))
#endif
void Timer0handlerUser(){

}

#ifdef ARM9
__attribute__((section(".itcm")))
#endif
void Timer1handlerUser(){

}

#ifdef ARM9
__attribute__((section(".itcm")))
#endif
void Timer2handlerUser(){

}

#ifdef ARM9
__attribute__((section(".itcm")))
#endif
void Timer3handlerUser(){

}

#ifdef ARM9
__attribute__((section(".itcm")))
#endif
void HblankUser(){

}

#ifdef ARM9
__attribute__((section(".itcm")))
#endif
void VblankUser(){
	struct sIPCSharedTGDSSpecific* TGDSUSERIPC = getsIPCSharedTGDSSpecific();
	if(TGDSUSERIPC->frameCounter9 < 60){
		TGDSUSERIPC->frameCounter9++;
	}
	else{
		TGDSUSERIPC->frameCounter9 = 0;
	}
}

#ifdef ARM9
__attribute__((section(".itcm")))
#endif
void VcounterUser(){
	glPushMatrix();
		
	glMatrixMode(GL_POSITION);
	glLoadIdentity();
	
	gluLookAt(	0.0, 0.0, camDist,		//camera possition 
			0.0, -1.0, 4.0,		//look at
			0.0, 1.0, 0.0);		//up
	
	glTranslatef32(0, 0, 0.0);
	glRotateXi(rotateX);
	glRotateYi(rotateY);
	glMaterialf(GL_EMISSION, RGB15(31,31,31));
	glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK );
	
	glRotateX(-90.0);	//Because OBJ Axis is 90º inverted...
	glRotateY(45.0);	
	glCallList((u32*)&Cube);
	glFlush();
	glPopMatrix(1);
	
	rotateX += 0.3;
	rotateY += 0.3;
}

//Note: this event is hardware triggered from ARM7, on ARM9 a signal is raised through the FIFO hardware
#ifdef ARM9
__attribute__((section(".itcm")))
#endif
void screenLidHasOpenedhandlerUser(){

}

//Note: this event is hardware triggered from ARM7, on ARM9 a signal is raised through the FIFO hardware
#ifdef ARM9
__attribute__((section(".itcm")))
#endif
void screenLidHasClosedhandlerUser(){
	
}