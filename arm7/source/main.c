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

static inline sint32 get_highcode_size(){
	return (sint32)((uint8*)(uint32*)&__highcode_vma_end__ - (sint32)(&__highcode_vma_start));
}

//---------------------------------------------------------------------------------
int main(int _argc, sint8 **_argv) {
//---------------------------------------------------------------------------------
	/*			TGDS 1.5 Standard ARM7 Init code start	*/
	installWifiFIFO();		
	/*			TGDS 1.5 Standard ARM7 Init code end	*/
	
	//set WORKRAM 32K to ARM7
	WRAM_CR = WRAM_0KARM9_32KARM7;
	
	//before call set up highcode
	memcpy ((u32*)0x03810000, (u32*)(&__highcode_vma_start), get_highcode_size());	//memcpy ( void * destination, const void * source, size_t num )
	
		
	
	void (*foo)(void) = 0x03810000;
	foo();

	ARM7ExecuteNDSLoader();	//prevent to execute this at this time. fillNDSLoaderContext() will set this call to true
	
    while (1) {
		handleARM7SVC();	/* Do not remove, handles TGDS services */
		IRQVBlankWait();
	}
   
	return 0;
}
