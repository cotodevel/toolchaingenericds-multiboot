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
void TGDSMultibootRunNDSPayload(char * filename);

3) Boot NDS file by calling this method along the NDS Binary to-be run.



Issues: 
Some dkARM DS binaries aren't working. As a workaround you can reload moonshell2 through tgds-multiboot, then reload the problematic ds binary.
Coto