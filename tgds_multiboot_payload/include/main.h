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
#include "posixHandleTGDS.h"

#endif


#ifdef __cplusplus
extern "C" {
#endif

extern int main(int argc, char **argv);
//ToolchainGenericDS-LinkedModule User implementation: Vanilla TGDS Project
extern char args[8][MAX_TGDSFILENAME_LENGTH];
extern char *argvs[8];

extern u8 argvIntraTGDSMB[256];

#ifdef __cplusplus
}
#endif

