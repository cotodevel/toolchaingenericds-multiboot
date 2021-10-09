![ToolchainGenericDS](img/TGDS-Logo.png)

NTR/TWL SDK: TGDS1.64

master: Development branch. Use TGDS1.64Stable: branch for stable features.

This is the Toolchain Generic multiboot project:

Compile Toolchain: To compile this project you will need to follow the steps at https://bitbucket.org/Coto88/toolchaingenericds : Then simply extract the project somewhere.

Compile this project: Open msys, through msys commands head to the directory your extracted this project. Then write: make clean make

After compiling, run the project in NDS.


Project Specific description: 

TGDS homebrew loader. Loads .NDS TGDS/DevkitARM binaries from within a menu. 
Simply DLDI patch ToolchainGenericDS-multiboot.nds, if the loader used to boot this app does not perform auto-DLDI patching.


############################################### HOW TO USE TGDS-MULTIBOOT IN TGDS PROJECTS ############################################### 
1) 1a) There's a standalone NDS Binary of TGDS-multiboot (ToolchainGenericDS-multiboot.nds/ToolchainGenericDS-multiboot.srl) for launching homebrew.
   1b) TGDS-multiboot is also embedded into TGDS projects: in target's TGDS project, simply call 
		void TGDSMultibootRunNDSPayload(char * filename);

	Note: 	-TWL binaries are WIP. 
			-For both cases it's required to have tgds_multiboot_payload_ntr.bin/tgds_multiboot_payload_twl.bin copied in SD root folder.


/release folder has the latest binary precompiled for your convenience.

Latest stable release: https://bitbucket.org/Coto88/ToolchainGenericDS-multiboot/get/TGDS1.64Stable.zip

Notes/Issues: 
- NTR Binary only, for now
- Some dkARM DS binaries aren't working. As a workaround you can reload moonshell2 through tgds-multiboot, then reload the problematic ds binary.

Coto
