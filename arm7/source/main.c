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
#include "spifwTGDS.h"
#include "posixHandleTGDS.h"
#include "wifi_arm7.h"

//---------------------------------------------------------------------------------
int main(int argc, char **argv)  {
//---------------------------------------------------------------------------------
	/*			TGDS 1.6 Standard ARM7 Init code start	*/
	installWifiFIFO();
	/*			TGDS 1.6 Standard ARM7 Init code end	*/
	
    while (1) {
		//up to this point, is free to reload the EWRAM code		
		handleARM7SVC();	/* Do not remove, handles TGDS services */
	}
	return 0;
}