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

#TGDS1.6 compatible Makefile

#ToolchainGenericDS specific: 
export SOURCE_MAKEFILE7 = custom
export SOURCE_MAKEFILE9 = custom

#Translate paths to windows with forward slashes
cpath := $(shell pwd)
ifeq ($(shell uname -o), Cygwin)
    CURDIR := '$(shell cygpath -w -p ${cpath})'
endif

#Shared
include $(DEFAULT_GCC_PATH_WIN)/Makefile.basenewlib

#Custom
# Project Specific
export TGDSPROJECTNAME = ToolchainGenericDS-multiboot
export EXECUTABLE_FNAME = $(TGDSPROJECTNAME).nds
export EXECUTABLE_VERSION_HEADER =	0.1
export EXECUTABLE_VERSION =	"$(EXECUTABLE_VERSION_HEADER)"
export TGDSPKG_TARGET_NAME = /
#The ndstool I use requires to have the elf section removed, so these rules create elf headerless- binaries.
export DIR_ARM7 = arm7
export BUILD_ARM7	=	build
export DIR_ARM9 = arm9
export BUILD_ARM9	=	build
export ELF_ARM7 = arm7.elf
export ELF_ARM9 = arm9.elf
export NONSTRIPELF_ARM7 = arm7-nonstripped.elf
export NONSTRIPELF_ARM9 = arm9-nonstripped.elf

export DECOMPRESSOR_BOOTCODE_9 = tgds_multiboot_payload
export DECOMPRESSOR_BOOTCODE_9i = tgds_multiboot_payload_twl

export BINSTRIP_RULE_7 :=	$(DIR_ARM7).bin
export BINSTRIP_RULE_arm7bootldr =	arm7bootldr.bin
export BINSTRIP_RULE_arm7ibootldr =	arm7ibootldr.bin

export BINSTRIP_RULE_9 :=	$(DIR_ARM9).bin

export BINSTRIP_RULE_COMPRESSED_9 :=	$(DECOMPRESSOR_BOOTCODE_9).bin
export BINSTRIP_RULE_COMPRESSED_9i :=	$(DECOMPRESSOR_BOOTCODE_9i).bin

export TARGET_LIBRARY_CRT0_FILE_7 = nds_arm_ld_crt0
export TARGET_LIBRARY_CRT0_FILE_9 = nds_arm_ld_crt0
export TARGET_LIBRARY_CRT0_FILE_COMPRESSED_9 = nds_arm_ld_crt0

export TARGET_LIBRARY_LINKER_FILE_7 = $(TARGET_LIBRARY_PATH)$(TARGET_LIBRARY_LINKER_SRC)/$(TARGET_LIBRARY_CRT0_FILE_7).S
export TARGET_LIBRARY_LINKER_FILE_9 = $(TARGET_LIBRARY_PATH)$(TARGET_LIBRARY_LINKER_SRC)/$(TARGET_LIBRARY_CRT0_FILE_9).S
export TARGET_LIBRARY_LINKER_FILE_COMPRESSED_9 = ../$(DECOMPRESSOR_BOOTCODE_9)/$(TARGET_LIBRARY_CRT0_FILE_COMPRESSED_9).S

export TARGET_LIBRARY_TGDS_NTR_7 = toolchaingen7
export TARGET_LIBRARY_TGDS_NTR_9 = toolchaingen9
export TARGET_LIBRARY_TGDS_TWL_7 = $(TARGET_LIBRARY_TGDS_NTR_7)i
export TARGET_LIBRARY_TGDS_TWL_9 = $(TARGET_LIBRARY_TGDS_NTR_9)i

#####################################################ARM7#####################################################

export DIRS_ARM7_SRC = source/	\
			source/interrupts/	\
			../common/	\
			../common/templateCode/source/	\
			../common/templateCode/data/arm7/	
			
export DIRS_ARM7_HEADER = source/	\
			source/interrupts/	\
			include/	\
			../common/	\
			../common/templateCode/source/	\
			../common/templateCode/data/arm7/	\
			build/	\
			../$(PosIndCodeDIR_FILENAME)/$(DIR_ARM7)/include/
#####################################################ARM9#####################################################

export DIRS_ARM9_SRC = data/	\
			source/	\
			source/interrupts/	\
			source/gui/	\
			source/TGDSMemoryAllocator/	\
			../common/	\
			../common/templateCode/source/	\
			../common/templateCode/data/arm9/	
			
export DIRS_ARM9_HEADER = data/	\
			build/	\
			include/	\
			source/gui/	\
			source/TGDSMemoryAllocator/	\
			../common/	\
			../common/templateCode/source/	\
			../common/templateCode/data/arm9/	\
			../$(PosIndCodeDIR_FILENAME)/$(DIR_ARM9)/include/
# Build Target(s)	(both processors here)
all: $(EXECUTABLE_FNAME)
#all:	debug

#ignore building this
.PHONY: $(ELF_ARM7)	$(ELF_ARM9)

#Make
compile	:
	-cp	-r	$(TARGET_LIBRARY_PATH)$(TARGET_LIBRARY_MAKEFILES_SRC)/templateCode/	$(CURDIR)/common/
	-cp	-r	$(TARGET_LIBRARY_MAKEFILES_SRC7_FPIC)	$(CURDIR)/$(PosIndCodeDIR_FILENAME)/$(DIR_ARM7)
	-$(MAKE)	-R	-C	$(PosIndCodeDIR_FILENAME)/$(DIR_ARM7)/
	-cp	-r	$(TARGET_LIBRARY_MAKEFILES_SRC9_FPIC)	$(CURDIR)/$(PosIndCodeDIR_FILENAME)/$(DIR_ARM9)
	-$(MAKE)	-R	-C	$(PosIndCodeDIR_FILENAME)/$(DIR_ARM9)/
ifeq ($(SOURCE_MAKEFILE7),default)
	cp	-r	$(TARGET_LIBRARY_MAKEFILES_SRC7_NOFPIC)	$(CURDIR)/$(DIR_ARM7)
endif
	$(MAKE)	-R	-C	$(DIR_ARM7)/
	$(MAKE)	-R	-C	arm7bootldr/
	$(MAKE)	-R	-C	arm7ibootldr/
	-mv arm7bootldr/arm7bootldr.bin	tgds_multiboot_payload/data
	-mv arm7ibootldr/arm7ibootldr.bin	tgds_multiboot_payload_twl/data
	
ifeq ($(SOURCE_MAKEFILE9),default)
	cp	-r	$(TARGET_LIBRARY_MAKEFILES_SRC9_NOFPIC)	$(CURDIR)/$(DIR_ARM9)
endif
	$(MAKE)	-R	-C	$(CURDIR)/stage2_9/
	$(MAKE)	-R	-C	$(CURDIR)/$(DECOMPRESSOR_BOOTCODE_9)/
	$(MAKE)	-R	-C	$(CURDIR)/$(DECOMPRESSOR_BOOTCODE_9i)/
	$(MAKE)	-R	-C	$(DIR_ARM9)/
$(EXECUTABLE_FNAME)	:	compile
	-@echo 'ndstool begin'
	$(NDSTOOL)	-v	-c $@	-7  $(CURDIR)/arm7/$(BINSTRIP_RULE_7)	-e7  0x037f8000	-9 $(CURDIR)/ARM9/$(BINSTRIP_RULE_9) -e9  0x02000000	-b	icon.bmp "ToolchainGenericDS SDK;$(TGDSPROJECTNAME) NDS Binary; "
	-mv $(EXECUTABLE_FNAME)	release/arm7dldi-ntr
	-mv arm9/build/tgds_multiboot_payload.h	release/arm7dldi-ntr
	-mv arm9/build/tgds_multiboot_payload.c	release/arm7dldi-ntr
	-mv arm9/build/tgds_multiboot_payload_twl.h	release/arm7dldi-ntr
	-mv arm9/build/tgds_multiboot_payload_twl.c	release/arm7dldi-ntr
	-mv arm9/data/tgds_multiboot_payload.bin release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin
	-mv arm9/data/tgds_multiboot_payload_twl.bin release/arm7dldi-ntr
	-$(NDSTOOL)	-c 	${@:.nds=.srl} -g "TGDS" "NN" "NDS.TinyFB" -b	icon.bmp "ToolchainGenericDS SDK;$(TGDSPROJECTNAME) TWL Binary;" -7 arm7/arm7-nonstripped_dsi.elf -9 arm9/arm9-nonstripped_dsi.bin	-e9  0x02000000
	-mv ${@:.nds=.srl}	release/arm7dldi-ntr
	-mv release/arm7dldi-ntr/${@:.nds=.srl}	E:/
	-mv release/arm7dldi-ntr/tgds_multiboot_payload_twl.bin	E:/
	-@echo 'ndstool end: built: $@'
	
#---------------------------------------------------------------------------------
# Clean
each_obj = $(foreach dirres,$(dir_read_arm9_files),$(dirres).)
	
clean:
	$(MAKE)	clean	-C	$(DIR_ARM7)/
	$(MAKE) clean	-C	$(PosIndCodeDIR_FILENAME)/$(DIR_ARM7)/
	$(MAKE) clean	-C	arm7bootldr/
	$(MAKE) clean	-C	arm7ibootldr/
ifeq ($(SOURCE_MAKEFILE7),default)
	-@rm -rf $(CURDIR)/$(DIR_ARM7)/Makefile
endif
#--------------------------------------------------------------------	
	$(MAKE) clean	-C	$(CURDIR)/stage2_9/
	$(MAKE) clean	-C	$(CURDIR)/$(DECOMPRESSOR_BOOTCODE_9)/
	$(MAKE) clean	-C	$(CURDIR)/$(DECOMPRESSOR_BOOTCODE_9i)/
	$(MAKE)	clean	-C	$(DIR_ARM9)/
	$(MAKE) clean	-C	$(PosIndCodeDIR_FILENAME)/$(DIR_ARM9)/
ifeq ($(SOURCE_MAKEFILE9),default)
	-@rm -rf $(CURDIR)/$(DIR_ARM9)/Makefile
endif
	-@rm -rf $(CURDIR)/$(PosIndCodeDIR_FILENAME)/$(DIR_ARM7)/Makefile
	-@rm -rf $(CURDIR)/$(PosIndCodeDIR_FILENAME)/$(DIR_ARM9)/Makefile
	-@rm -fr $(EXECUTABLE_FNAME)	$(TGDSPROJECTNAME).srl		$(CURDIR)/common/templateCode/	arm9/data/arm7bootldr.bin	arm9/data/arm7ibootldr.bin	$(CURDIR)/$(DECOMPRESSOR_BOOTCODE_9)/$(BINSTRIP_RULE_COMPRESSED_9)	release/arm7dldi-ntr/$(EXECUTABLE_FNAME)	release/arm7dldi-ntr/tgds_multiboot_payload.h	release/arm7dldi-ntr/tgds_multiboot_payload.c	release/arm7dldi-ntr/tgds_multiboot_payload_twl.h	release/arm7dldi-ntr/tgds_multiboot_payload_twl.c	release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin	release/arm7dldi-ntr/tgds_multiboot_payload_twl.bin

rebase:
	git reset --hard HEAD
	git clean -f -d
	git pull

commitChanges:
	-@git commit -a	-m '$(COMMITMSG)'
	-@git push origin HEAD

#ToolchainGenericDS Package deploy format required by ToolchainGenericDS-OnlineApp.
BuildTGDSPKG:
	-@echo 'Build TGDS Package. '
	-$(TGDSPKGBUILDER) $(TGDSPROJECTNAME) $(TGDSPKG_TARGET_NAME) $(LIBPATH) /release/arm7dldi-ntr/
