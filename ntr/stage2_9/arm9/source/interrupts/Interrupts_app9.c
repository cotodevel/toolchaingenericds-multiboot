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
	//any floating point gl call is being converted to fixed prior to being implemented
	gluPerspective(30, 256.0 / 192.0, 0.1, 70);

	gluLookAt(	0.0, 0.0, camDist,		//camera possition 
				0.0, 0.0, 0.0,		//look at
				0.0, 1.0, 0.0);		//up
	
	glLight(0, RGB15(31,31,31) , 0,				  floattov10(-1.0),		 0);
	glLight(1, RGB15(31,0,31),   0,				  floattov10(1) - 1,			 0);
	glLight(2, RGB15(0,31,0) ,   floattov10(-1.0), 0,					 0);
	glLight(3, RGB15(0,0,31) ,   floattov10(1.0) - 1,  0,					 0);

	glPushMatrix();

	//move it away from the camera
	glTranslate3f32(0, 0, floattof32(-1));
			
	glRotateX(rotateX);
	glRotateY(rotateY);
	
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	
	glMatrixMode(GL_MODELVIEW);

	glMaterialf(GL_AMBIENT, RGB15(8,8,8));
	glMaterialf(GL_DIFFUSE, RGB15(16,16,16));
	glMaterialf(GL_SPECULAR, BIT(15) | RGB15(8,8,8));
	glMaterialf(GL_EMISSION, RGB15(5,5,5));

	//ds uses a table for shinyness..this generates a half-ass one
	glMaterialShinnyness();

	//not a real gl function and will likely change
	glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK | POLY_FORMAT_LIGHT0 | POLY_FORMAT_LIGHT1 | 
												POLY_FORMAT_LIGHT2 | POLY_FORMAT_LIGHT3 ) ;
	
	u32 keypad = keysHeld();
	
	/*
	if(keypad & KEY_UP) rotateX += 3;
	if(keypad & KEY_DOWN) rotateX -= 3;
	if(keypad & KEY_LEFT) rotateY += 3;
	if(keypad & KEY_RIGHT) rotateY -= 3;
	
	if(keypad & KEY_A) camDist += 0.3;
	if(keypad & KEY_B) camDist -= 0.3;
	*/
	
	glBindTexture(0, textureID);

	//draw the obj
	glBegin(GL_QUAD);
		for(int i = 0; i < 6; i++){
			drawQuad(i);
		}
	glEnd();
	
	glPopMatrix(1);
		
	glFlush();
 	
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