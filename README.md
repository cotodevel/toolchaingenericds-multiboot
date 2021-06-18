![ToolchainGenericDS](img/TGDS-Logo.png)

Branch: master

This is the Toolchain Generic multiboot project:

Compile Toolchain: To compile this project you will need to follow the steps at https://bitbucket.org/Coto88/toolchaingenericds : Then simply extract the project somewhere.

Compile this project: Open msys, through msys commands head to the directory your extracted this project. Then write: make clean make

After compiling, run the project in NDS.


Project Specific description: 

TGDS homebrew loader. Loads .NDS TGDS/DevkitARM binaries from within a menu. As long ARM7 entry address sits between 0x037f8000 ~ 0x03810000.
Simply DLDI patch ToolchainGenericDS-multiboot.nds, if the loader used to boot this app does not perform auto-DLDI patching.


############################################### HOW TO EMBED PAYLOAD TGDS-MULTIBOOT INTO TARGET NDS BINARY ############################################### 
1) Build the project, then move the tgds_multiboot.h and tgds_multiboot.c into ARM9 /source folder, embedding TGDS-multiboot into target NDS Binary. 
   Optionally, there's a standalone NDS Binary of TGDS-multiboot.
   Optionally, if none of these work for you (such as saving binary space), a payload of TGDS-multiboot is read from filesystem by default.
   
   
2) TGDS-multiboot implementation: in target's TGDS project, C source add: 

#include "dmaTGDS.h"
#include "nds_cp15_misc.h"
#include "fatfslayerTGDS.h"

And implement this method:
//NTR Bootcode:
__attribute__((section(".itcm")))
void TGDSMultibootRunNDSPayload(char * filename) __attribute__ ((optnone)) {
	strcpy((char*)(0x02280000 - (MAX_TGDSFILENAME_LENGTH+1)), filename);	//Arg0:	
	
	FILE * tgdsPayloadFh = fopen("0:/tgds_multiboot_payload.bin", "r");
	if(tgdsPayloadFh != NULL){
		fseek(tgdsPayloadFh, 0, SEEK_SET);
		int	tgds_multiboot_payload_size = FS_getFileSizeFromOpenHandle(tgdsPayloadFh);
		fread((u32*)0x02280000, 1, tgds_multiboot_payload_size, tgdsPayloadFh);
		coherent_user_range_by_size(0x02280000, (int)tgds_multiboot_payload_size);
		fclose(tgdsPayloadFh);
		int ret=FS_deinit();
		//Copy and relocate current TGDS DLDI section into target ARM9 binary
		bool stat = dldiPatchLoader((data_t *)0x02280000, (u32)tgds_multiboot_payload_size, (u32)&_io_dldi_stub);
		if(stat == false){
			//printf("DLDI Patch failed. APP does not support DLDI format.");
		}
		REG_IME = 0;
		typedef void (*t_bootAddr)();
		t_bootAddr bootARM9Payload = (t_bootAddr)0x02280000;
		bootARM9Payload();
	}
}

3) Boot NDS file by calling this method along the NDS Binary to-be run.






Issues: 
Some dkARM DS binaries aren't working. As a workaround you can reload moonshell2 through tgds-multiboot, then reload the problematic ds binary.
Coto