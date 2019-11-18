This is the Toolchain Generic multiboot project:

Compile Toolchain: To compile this project you will need to follow the steps at https://bitbucket.org/Coto88/toolchaingenericds : Then simply extract the project somewhere.

Compile this project: Open msys, through msys commands head to the directory your extracted this project. Then write: make clean make

After compiling, run the project in NDS.


Project Specific description: 

ToolchainGenericDS-multiboot: a.k.a: the TGDS homebrew loader! It loads .NDS TGDS binaries from within a menu. I don't know if it relaunches other binaries (from other devkits) but these should work.
Simply DLDI patch ToolchainGenericDS-multiboot.nds (if the loader does not perform auto-DLDI patching), and you can relaunch any other NDS binary, or other TGDS project!

/release folder has the latest binary precompiled for your convenience.

Coto
