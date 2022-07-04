#ifndef __zipDecomp_h__
#define __microphone9_h__

#include "main.h"
#include "dsregs.h"
#include "dsregs_asm.h"
#include "typedefsTGDS.h"
#include "gui_console_connector.h"
#include "dswnifi_lib.h"
#include "dldi.h"
#include "ipcfifoTGDS.h"
#include "fileBrowse.h"	//generic template functions from TGDS: maintain 1 source, whose changes are globally accepted by all TGDS Projects.
#include "lzss9.h"
#include "nds_cp15_misc.h"
#include "soundTGDS.h"
#include "biosTGDS.h"
#include "busTGDS.h"
#include "clockTGDS.h"
#include "global_settings.h"
#include "cartHeader.h"
#include "posixHandleTGDS.h"
#include "TGDSMemoryAllocator.h"
#include "ipcfifoTGDSUser.h"
#include "fatfslayerTGDS.h"
#include "utilsTGDS.h"

#endif


#ifdef __cplusplus
extern "C" {
#endif

extern int handleDecompressor(char * zippedFile, char * buf);
extern char logBuf[4096];

#ifdef __cplusplus
}
#endif


