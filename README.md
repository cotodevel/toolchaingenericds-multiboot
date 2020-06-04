Branch: master -> uses an older loader but it is more compatible with older slot-1 cards.

-

This is the Toolchain Generic multiboot project:

Compile Toolchain: To compile this project you will need to follow the steps at https://bitbucket.org/Coto88/toolchaingenericds : Then simply extract the project somewhere.

Compile this project: Open msys, through msys commands head to the directory your extracted this project. Then write: make clean make

After compiling, run the project in NDS.


Project Specific description: 

TGDS homebrew loader. Loads .NDS TGDS/DevkitARM binaries from within a menu. As long ARM7 entry address sits between 0x037f8000 ~ 0x03810000.
Simply DLDI patch ToolchainGenericDS-multiboot.nds, if the loader used to boot this app does not perform auto-DLDI patching.

/release folder has the latest binary precompiled for your convenience.


Loader Code map layout:

0x02340000 + (used 315K ~ 0x4F000?) = 0x0238F000 NDS App Size.
0x02400000 - 0x30000 = 0x023D0000 onwards, reserved for NDSLoader context.

So basically:
0xC0000 used by loader code from top 4MB EWRAM backwards, which leaves us with:

ARM9: (0x02400000 - 0xC0000) = 0x02340000 - 0x02000000 = 0x340000 ~ 3.407.872 bytes, which is NOT too bad ;-)


Todo:
TGDS-multiboot should execute from 0x02000000 and that will allow to reload itself and other NDS apps relying on the Loader Code address.


Coto
