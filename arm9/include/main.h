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
#include "limitsTGDS.h"
#include "dldi.h"

#endif


#ifdef __cplusplus
extern "C" {
#endif

extern int main(int _argc, sint8 **_argv);
extern char curChosenBrowseFile[MAX_TGDSFILENAME_LENGTH+1];
extern bool fillNDSLoaderContext(char * filename);
extern bool GDBEnabled;
extern struct FileClassList * thisFileList;

extern addr_t readAddr (data_t *mem, addr_t offset);
extern void writeAddr (data_t *mem, addr_t offset, addr_t value);
extern addr_t quickFind (const data_t* data, const data_t* search, size_t dataLen, size_t searchLen);

extern bool dldiRelocateLoader(bool clearBSS, u32 DldiRelocatedAddress, u32 dldiSourceInRam);
extern bool dldiPatchLoader (data_t *binData, u32 binSize); //original DLDI code: seeks a DLDI section in binData, and uses current NTR TGDS homebrew's DLDI to relocate it in there


#ifdef __cplusplus
}
#endif