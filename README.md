![ToolchainGenericDS](img/TGDS-Logo.png)

Branch: master

This is the Toolchain Generic multiboot project:

Compile Toolchain: To compile this project you will need to follow the steps at https://bitbucket.org/Coto88/toolchaingenericds : Then simply extract the project somewhere.

Compile this project: Open msys, through msys commands head to the directory your extracted this project. Then write: make clean make

After compiling, run the project in NDS.


Project Specific description: 

TGDS homebrew loader. Loads .NDS TGDS/DevkitARM binaries from within a menu. As long ARM7 entry address sits between 0x037f8000 ~ 0x03810000.
Simply DLDI patch ToolchainGenericDS-multiboot.nds, if the loader used to boot this app does not perform auto-DLDI patching.

/release folder has the latest binary precompiled for your convenience.


Loader Code map layout:

0x02300000 + (used 430K ~ 0x6B800?) = 0x0236B800 NDS App Size.
0x02400000 - 0x38000 = 0x023C8000 onwards, reserved for NDSLoader context.

So basically:
0xC0000 used by loader code from top 4MB EWRAM backwards, which leaves us with:

ARM9: (0x02400000 - 0x02300000) = 3MB ARM9.bin


Todo: 

1)
Some dkARM DS binaries aren't working. As a workaround you can reload moonshell2 through tgds-multiboot, then reload the problematic ds binary.

Coto
