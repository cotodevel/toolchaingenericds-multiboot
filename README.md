This is the Toolchain Generic multiboot project:

Compile Toolchain: To compile this project you will need to follow the steps at https://bitbucket.org/Coto88/toolchaingenericds : Then simply extract the project somewhere.

Compile this project: Open msys, through msys commands head to the directory your extracted this project. Then write: make clean make

After compiling, run the project in NDS.


Project Specific description: 

ToolchainGenericDS-multiboot: a.k.a: the TGDS homebrew loader! It loads .NDS TGDS binaries from within a menu. I don't know if it relaunches other binaries (from other devkits) but these should work.
Simply DLDI patch ToolchainGenericDS-multiboot.nds (if the loader does not perform auto-DLDI patching), and you can relaunch any other NDS binary, or other TGDS project!

/release folder has the latest binary precompiled for your convenience.

Notes:

Map layout:

0x02340000 + (used 360K -- 0x5A000?) = 0x0239A000 NDS App Size.

0x02400000 - 0x30000 = 0x023D0000 onwards, reserved for NDSLoader context?.

589.824 "free". Thus this map starts from 0x02340000, uses as little as possible memory for now. Thus,
for ARM9.bin, we have free up to: 0x02340000 - 0x02000000 = 3.407.872 bytes, which is NOT too bad ;-)


Coto
