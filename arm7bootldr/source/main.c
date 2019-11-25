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
#include "biosTGDS.h"
#include "loader.h"

//---------------------------------------------------------------------------------
int main(int _argc, sint8 **_argv) {
//---------------------------------------------------------------------------------
	/*			TGDS 1.5 Standard ARM7 Init code start	*/
	installWifiFIFO();		
	/*			TGDS 1.5 Standard ARM7 Init code end	*/
	
	ARM7ExecuteNDSLoader();	//prevent to execute this at this time. fillNDSLoaderContext() will set this call to true
	
	SendFIFOWords(0xff11ff22, 0);	//if we get a signal out of this, means bootcode works!
	
	
    while (1) {
		
		scanKeys();
		
		if (keysPressed() & KEY_DOWN){
			SendFIFOWords(0xff11ff44, 0);	//printf("ARM7 ALIVE!");
		}
		
		handleARM7SVC();	/* Do not remove, handles TGDS services */
		IRQVBlankWait();
	}
   
	return 0;
}
