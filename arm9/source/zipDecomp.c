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
#include "zipDecomp.h"

char logBuf[4096];

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int handleDecompressor(char * zippedFile, char * buf){
	//Handle Package
	int argCount = 2;
	char fileBuf[256+1];
	memset(fileBuf, 0, sizeof(fileBuf));
	strcpy(fileBuf, zippedFile);
	strcpy(&args[0][0], "-x");	//Arg0
	strcpy(&args[1][0], fileBuf);	//Arg1
	strcpy(&args[2][0], "d /");	//Arg2

	int i = 0;
	for(i = 0; i < argCount; i++){	
		argvs[i] = (char*)&args[i][0];
	}
	extern int miniunz_main(int argc,char **argv, char* buf);
	return miniunz_main(argCount, argvs, buf);
}