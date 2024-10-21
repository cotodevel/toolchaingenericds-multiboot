#
#			Copyright (C) 2017  Coto
#This program is free software; you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation; either version 2 of the License, or
#(at your option) any later version.

#This program is distributed in the hope that it will be useful, but
#WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
#General Public License for more details.

#You should have received a copy of the GNU General Public License
#along with this program; if not, write to the Free Software
#Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
#USA
#

export DIRS_ARM9_SRC = data/	\
	source/gui	\
	source/TGDSMemoryAllocator	\
	source/	\
	source/interrupts/	\
	../common/	\
	../common/templateCode/data/arm9/	\
	../common/templateCode/source/
	

export DIRS_ARM9_HEADER = source/	\
	source/gui	\
	source/TGDSMemoryAllocator	\
	build/	\
	include/	\
	../common/

# relative path (if you are outside arm9 folder then $(DIR_ARM9), arm7 is $(DIR_ARM9))
RELATIVE_PATH =	

#Prepare ARM9 dirs
DIRLIBS_HDR = $(foreach dirres,$(DIRS_ARM9_HEADER),-I "$(RELATIVE_PATH)$(dirres)" )	$(DIRLIBS_ALLOWED)	$(TARGET_LIBRARY_HEADERS_9_INC)
DIRLIBS_SRC = $(foreach dirres,$(DIRS_ARM9_SRC),-I "$(CURDIR)/$(RELATIVE_PATH)$(dirres)" )

file_arm9_src_c=  $(foreach dir,$(DIRS_ARM9_SRC), $(subst $(CURDIR)/$(RELATIVE_PATH)/$(dir)/,,$(wildcard $(CURDIR)/$(RELATIVE_PATH)/$(dir)/*.c))  )
file_arm9_src_o=  $(foreach dir,$(BUILD), $(subst $(CURDIR)/$(RELATIVE_PATH)/$(dir)/,,$(wildcard $(CURDIR)/$(RELATIVE_PATH)/$(dir)/*.o))  )
file_arm9_src_o_out = $(filter-out $(TARGET_LIBRARY_CRT0_FILE_9).o, $(file_arm9_src_o))
file_arm9_src_s=  $(foreach dir,$(DIRS_ARM9_SRC), $(subst $(CURDIR)/$(RELATIVE_PATH)/$(dir)/,,$(wildcard $(CURDIR)/$(RELATIVE_PATH)/$(dir)/*.s))  )
file_arm9_src_SUPP=  $(foreach dir,$(DIRS_ARM9_SRC), $(subst $(CURDIR)/$(RELATIVE_PATH)/$(dir)/,,$(wildcard $(CURDIR)/$(RELATIVE_PATH)/$(dir)/*.S))  )
file_arm9_src_cpp=  $(foreach dir,$(DIRS_ARM9_SRC), $(subst $(CURDIR)/$(RELATIVE_PATH)/$(dir)/,,$(wildcard $(CURDIR)/$(RELATIVE_PATH)/$(dir)/*.cpp))  )
file_arm9_src_bin=  $(foreach dir,$(DIRS_ARM9_SRC), $(subst $(CURDIR)/$(RELATIVE_PATH)/$(dir)/,,$(wildcard $(CURDIR)/$(RELATIVE_PATH)/$(dir)/*.bin))  )

# Object Target
objs_arm9:=		${file_arm9_src_bin:.bin=.o}	${file_arm9_src_c:.c=.o}	${file_arm9_src_s:.s=.o}	${file_arm9_src_SUPP:.S=.o}		${file_arm9_src_cpp:.cpp=.o}
d_objs_arm9:=	${file_arm9_src_c:.c=.d}	${file_arm9_src_s:.s=.d}	${file_arm9_src_SUPP:.S=.d}	${file_arm9_src_cpp:.cpp=.d}

#ARM9 specific definitions. -fno-common so we force initialized data to rodata
MAPFILE = $(BINSTRIP_RULE_COMPRESSED_9).map
AS_ASFLAGS		=	$(GLOBAL_SHARED_FLAGS_9)	$(SPECIFIC_AS_COMPILER_FLAGS_9)		$(GLOBAL_AS_COMPILER_FLAGS)
CC_ASFLAGS		=	$(GLOBAL_SHARED_FLAGS_9)	-marm	-Os	-fno-short-enums	-mthumb-interwork	-DARM9	-fno-unwind-tables -fno-asynchronous-unwind-tables -fno-tree-loop-distribute-patterns -Dappver='$(EXECUTABLE_VERSION)'	-DTGDSPROJECTNAME='"$(TGDSPROJECTNAME)"'	-DIGNORELIBS
CPP_FLAGS		=	$(CC_ASFLAGS)	-nostdinc++	-fno-rtti
BUILD			= 	$(BUILD_ARM9)
DIR_SRC			=	$(DIRS_ARM9_SRC)

#--gc-sections: allows relocatable sections linked (already) to be included, otherwise unused sections are discarded.
#check arm9.map "Discarded input sections" to see --gc-sections in action.
LDFLAGS	=	-specs=nds_arm9_ld_ntr.specs	-Wl,--no-gc-sections,-Map,$(BINSTRIP_RULE_COMPRESSED_9).map,-z,defs
LDFLAGS_DSI	=	-specs=nds_arm9_ld.specs	-Wl,--no-gc-sections,-Map,$(BINSTRIP_RULE_COMPRESSED_9)_twl.map
LIBRARIES_NTR =	-ltgdsfilesystem9	-lutils9	-l$(TARGET_LIBRARY_TGDS_NTR_9)	-lhardware9		-lstdc++	-lcnano9	-lgcc
LIBRARIES_TWL =	-ltgdsfilesystem9i	-lutils9i	-l$(TARGET_LIBRARY_TGDS_TWL_9)	-lhardware9i	-lstdc++	-lcnano9	-lgcc

################################################################################################

#Dont modify anything else if not necessary ..
OBJECTS:=	$(objs_arm9)	$(file_arm9_src_o_out)
OBJDIR 	=	./$(BUILD)
VPATH 	=	$(DIR_SRC)
OBJPROG =	$(addprefix $(OBJDIR)/, $(NONSTRIPELF_ARM9))

all: makebuild	$(TARGET_LIBRARY_CRT0_FILE_COMPRESSED_9).o	$(BINSTRIP_RULE_COMPRESSED_9)
#all: debug

makebuild:
	-mkdir -p $(OBJDIR)

#convert a TGA BMP16 into a native 3D texture unit format (GL_COLOR 16bpp)
$(OBJDIR)/%.o : %.tga
	-$(GRITTOOL) ${<:.o=.c} -ftc -o $@ -gu32 -gb -gB16
	$(CC)	$(CC_ASFLAGS)	$(DIRLIBS_HDR)	$(DIRLIBS_SRC)	-o $@ -c ${@:.o=.c}
	
#rule for reading *.bin in /data folder, converts to .C source code, which then gets compiled as object so it can be linked later
$(OBJDIR)/%.o : %.bin
	-@echo 'trying to convert .BIN ($<) -> .C output: ${@:.o=.c} : arrayName: $(subst build/,,${@:.o=})'
	$(BIN2C)	$<	${@:.o=.c}	$(subst build/,,${@:.o=})
	$(CC)	$(CC_ASFLAGS)	$(DIRLIBS_HDR)	$(DIRLIBS_SRC)	$(DIRLIBS_PROHIBITED)	-o $@ -c ${@:.o=.c}

#pcx parsing (Blender 3D export -> native DS Callist + texture)
$(OBJDIR)/%.o : %.pcx	
	-@echo 'trying to convert .BIN ($<) -> .C output: ${@:.o=.c} : arrayName: $(subst build/,,${@:.o=})'
	$(BIN2C)	$<	${@:.o=.c}	$(subst build/,,${@:.o=})
	$(CC) $(CC_ASFLAGS)	$(DIRLIBS_HDR)	$(DIRLIBS_SRC)	-o $@ -c ${@:.o=.c}	

#Do C Compile now!
$(OBJDIR)/%.o : %.c
	-@echo 'C Compile start: $@'
	$(CC)	$(CC_ASFLAGS)	$(DIRLIBS_HDR)	$(DIRLIBS_SRC)	-o $@ -c $<
	-@echo 'C Compile End: $@'

#Do C++ (CXX) Compile now!
$(OBJDIR)/%.o : %.cpp
	-@echo 'C++ Compile start: $@'
	$(CPP)	$(CPP_FLAGS)	$(DIRLIBS_HDR)	$(DIRLIBS_SRC)	-o	$@	-c	$<
	-@echo 'C Compile End: $@'

#Coto: Special "exception" rule. Fixes tricky projects such as SnemulDS's opcodes2.s where GCC AS is used, while compatibility with .S is required for newer projects.
#.S is GCC CC/CPP for compiling assembler with cpp headers
#.s is GCC AS Assembler for compiling assembler the usual way

#Do s (GCC AS) Compile now!
$(OBJDIR)/%.o : %.s
	-@echo 's Compile start: $@'
	$(AS)	$(DIRLIBS_HDR)	$(DIRLIBS_SRC)	$(AS_ASFLAGS)	-o $@ -c $<
	-@echo 's Compile end: $@'

#Do S (GCC CC) Compile now!
$(OBJDIR)/%.o : %.S
	-@echo 'S Compile start: $@'
	$(CPP)	-x assembler-with-cpp	$(DIRLIBS_HDR)	$(DIRLIBS_SRC)	$(CC_ASFLAGS)	-o $@ -c $<
	-@echo 'S Compile end: $@'

#Special rule: compile crt0 for arm9 entrypoint
$(TARGET_LIBRARY_CRT0_FILE_COMPRESSED_9).o:	$(TARGET_LIBRARY_LINKER_FILE_COMPRESSED_9)
	-@echo 'S Compile start: crt0s'
	rm -fr $(BUILD)/$(TARGET_LIBRARY_CRT0_FILE_COMPRESSED_9).o
	$(CPP)	-x assembler-with-cpp	$(DIRLIBS_HDR)	$(DIRLIBS_SRC)	$(CC_ASFLAGS)	-o	$(OBJDIR)/$@	-c	$<
	-@echo 'S Compile end: crt0s'

#Linkage.
$(OBJPROG): $(addprefix $(OBJDIR)/, $(OBJECTS))
	-@echo 'Entering Linking phase'
	-$(CC) $^	$(LDFLAGS)	$(DIRLIBS_HDR)	$(DIRLIBS_SRC)	$(DIRLIBS_SHARED)	-L	$(TARGET_LIBRARY_PATH)/	-L	$(TOOLCHAIN_PATH)/		$(LIBRARIES_NTR)	-L	$(TARGET_LIBRARY_LINKER_PATH_LD)	-o $@
	-$(CC) $^	$(LDFLAGS_DSI)	$(DIRLIBS_HDR)	$(DIRLIBS_SRC)	$(DIRLIBS_SHARED)	-L	$(TARGET_LIBRARY_PATH)/	-L	$(TOOLCHAIN_PATH)/		$(LIBRARIES_TWL)	-L	$(TARGET_LIBRARY_LINKER_PATH_LD)	-o ${@:.elf=_dsi.elf}
	-@echo 'Leaving Linking phase. Output: $@'

#xxxx-nonstripped.elf -> xxxx.elf STRIP operation
$(ELF_ARM9): $(OBJPROG)
	-@echo '$@ Unstripped. STRIP Start!'
	-mv build/${@:.elf=-nonstripped_dsi.elf}	$(CURDIR)
	$(OBJSTRIP)	-s -R .comment -R .gnu.version	$<
	-mv	$<	$@
	
$(BINSTRIP_RULE_COMPRESSED_9): $(ELF_ARM9)
	-@echo 'STRIP End: Remove ELF header and build baremetal Binary executable: $<	->	$@ !'
	$(OBJCOPY) 	-S -O binary	$<	$@
	$(OBJCOPY) 	-S -O binary	${<:.elf=-nonstripped_dsi.elf}	${@:.bin=_twl.bin}
	-mv	$@ ../arm9/data
	-mv	${@:.bin=_twl.bin} ../arm9/data
	-rm	$<
	-rm	${<:.elf=-nonstripped_dsi.elf}
	
################################################################################################
	
clean:
	-@echo 'cleanup $(BINSTRIP_RULE_COMPRESSED_9)'
	-@rm -fr $(BINSTRIP_RULE_COMPRESSED_9)	$(BINSTRIP_RULE_COMPRESSED_9).map	$(BINSTRIP_RULE_COMPRESSED_9)_twl.map	$(BUILD_ARM9)	data/$(DIR_ARM9).bin	data/arm7bootldr.bin	data/arm7bootldr_twl.bin
	
debug:
	-@echo 'debug $(objs_arm9)'