![ToolchainGenericDS](img/TGDS-Logo.png)

NTR/TWL SDK: TGDS1.65

master: Development branch. Use TGDS1.65: branch for stable features.

This is the Toolchain Generic multiboot project:

Compile Toolchain: To compile this project you will need to follow the steps at https://bitbucket.org/Coto88/toolchaingenericds : Then simply extract the project somewhere.

Compile this project: Open msys, through msys commands head to the directory your extracted this project. Then write: make clean make

After compiling, run the project in NDS.


Project Specific description: 

TGDS homebrew loader. Loads NTR/TWL binaries from within a menu. 
Simply DLDI patch ToolchainGenericDS-multiboot.nds, if the loader used to boot this app does not perform auto-DLDI patching.
TGDS-MB TWL does not require any DLDI patch because it uses the internal SD instead.
----Don't forget to copy toolchaingenericds-multiboot-config.txt to SD root path or TGDS-multiboot won't boot!!----

############################################### HOW TO USE TGDS-MULTIBOOT IN TGDS PROJECTS ############################################### 
1) 1a) There's a standalone NDS Binary of TGDS-multiboot (ToolchainGenericDS-multiboot.nds/ToolchainGenericDS-multiboot.srl) for launching homebrew.
   1b) TGDS-multiboot is also embedded into TGDS projects: in target's TGDS project, simply call 
		void TGDSMultibootRunNDSPayload(char * filename);
	-For both cases it's required to have tgds_multiboot_payload_ntr.bin/tgds_multiboot_payload_twl.bin copied in SD root folder.


____Remoteboot____
Also, it's recommended to use the remoteboot feature. It allows to send the current TGDS Project over wifi removing the necessity
to take out the SD card repeteadly and thus, causing it to wear out and to break the SD slot of your unit.

Usage:
- Make sure the wifi settings in the NintendoDS are properly set up, so you're already able to connect to internet from it.

- Get a copy of ToolchainGenericDS-multiboot: https://bitbucket.org/Coto88/ToolchainGenericDS-multiboot/get/TGDS1.65.zip
Follow the instructions there and get either the TWL or NTR version. Make sure you update the computer IP address used to build TGDS Projects, 
in the file: toolchaingenericds-multiboot-config.txt of said repository before moving it into SD card.

For example if you're running NTR mode (say, a DS Lite), you'll need ToolchainGenericDS-multiboot.nds, tgds_multiboot_payload_ntr.bin
and toolchaingenericds-multiboot-config.txt (update here, the computer's IP you use to build TGDS Projects) then move all of them to root SD card directory.

- Build the TGDS Project as you'd normally would, and run these commands from the shell.
<make clean>
<make>

- Then if you're on NTR mode:
<remoteboot ntr_mode computer_ip_address>

- Or if you're on TWL mode:
<remoteboot twl_mode computer_ip_address>

- And finally boot ToolchainGenericDS-multiboot, and press (X), wait a few seconds and TGDS Project should boot remotely.
  After that, everytime you want to remoteboot a TGDS Project, repeat the last 2 steps. ;-)




/release folder has the latest binary precompiled for your convenience.
Latest stable release: https://bitbucket.org/Coto88/ToolchainGenericDS-multiboot/get/TGDS1.65.zip


Coto
