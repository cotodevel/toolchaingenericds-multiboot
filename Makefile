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

#ToolchainGenericDS specific: Use Makefiles from either TGDS, or custom
export SOURCE_MAKEFILE7 = custom
export SOURCE_MAKEFILE9 = default

#Shared
ifeq ($(TGDS_ENV),windows)
	include $(DEFAULT_GCC_PATH)/Makefile.basenewlib
else
	export TGDS_ENV := linux
	export DEFAULT_GCC_PATH := /usr/arm-none-eabi/lib/newlib-nano-2.1-nds/6.2_2016q4/
	include $(DEFAULT_GCC_PATH)Makefile.basenewlib
endif

#Custom
# Project Specific
export TGDSPROJECTNAME = ToolchainGenericDS-multiboot
export EXECUTABLE_FNAME = $(TGDSPROJECTNAME).nds
export EXECUTABLE_VERSION_HEADER =	0.2
export EXECUTABLE_VERSION =	"$(EXECUTABLE_VERSION_HEADER)"
export TGDSPKG_TARGET_PATH := '//'
export TGDSREMOTEBOOTER_SERVER_IP_ADDR := '192.168.43.185'
export TGDSREMOTEBOOTER_SERVER_PORT := 1040
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

export BINSTRIP_RULE_9 :=	$(DIR_ARM9).bin

export BINSTRIP_RULE_COMPRESSED_9 :=	$(DECOMPRESSOR_BOOTCODE_9).bin
export BINSTRIP_RULE_COMPRESSED_9i :=	$(DECOMPRESSOR_BOOTCODE_9i).bin

export TARGET_LIBRARY_CRT0_FILE_7 = nds_arm_ld_crt0

#for 0x02380000 -> 0x06000000 arm7bootldr payload
export TARGET_LIBRARY_CRT0_FILE_7custom = nds_arm_ld_crt0custom

export TARGET_LIBRARY_CRT0_FILE_9 = nds_arm_ld_crt0
export TARGET_LIBRARY_CRT0_FILE_COMPRESSED_9 = nds_arm_ld_crt0

export TARGET_LIBRARY_LINKER_FILE_7i = $(TARGET_LIBRARY_CRT0_FILE_7).S
export TARGET_LIBRARY_LINKER_FILE_7 = ../$(TARGET_LIBRARY_CRT0_FILE_7).S
export TARGET_LIBRARY_LINKER_FILE_9 = ../$(TARGET_LIBRARY_CRT0_FILE_9).S
export TARGET_LIBRARY_LINKER_FILE_COMPRESSED_9 = ../$(DECOMPRESSOR_BOOTCODE_9)/$(TARGET_LIBRARY_CRT0_FILE_COMPRESSED_9).S

#for 0x02380000 -> 0x06000000 arm7bootldr payload
export TARGET_LIBRARY_LINKER_FILE_7custom = $(TARGET_LIBRARY_CRT0_FILE_7custom).S

export TARGET_LIBRARY_TGDS_NTR_7 = toolchaingen7
export TARGET_LIBRARY_TGDS_NTR_9 = toolchaingen9
export TARGET_LIBRARY_TGDS_TWL_7 = $(TARGET_LIBRARY_TGDS_NTR_7)i
export TARGET_LIBRARY_TGDS_TWL_9 = $(TARGET_LIBRARY_TGDS_NTR_9)i

#####################################################ARM7#####################################################

export DIRS_ARM7_SRC = source/	\
			source/petitfs-src/	\
			source/interrupts/	\
			../common/	\
			../common/templateCode/source/	\
			../common/templateCode/data/arm7/	
			
export DIRS_ARM7_HEADER = source/	\
			source/petitfs-src/	\
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
	
ifeq ($(SOURCE_MAKEFILE9),default)
	cp	-r	$(TARGET_LIBRARY_MAKEFILES_SRC9_NOFPIC)	$(CURDIR)/$(DIR_ARM9)
endif
	$(MAKE)	-R	-C	$(CURDIR)/$(DECOMPRESSOR_BOOTCODE_9)/
	$(MAKE)	-R	-C	$(DIR_ARM9)/
$(EXECUTABLE_FNAME)	:	compile
	-@echo 'ndstool begin'
	$(NDSTOOL)	-v	-c $@	-7  $(CURDIR)/arm7/$(BINSTRIP_RULE_7)	-e7  0x02380000	-9 $(CURDIR)/arm9/$(BINSTRIP_RULE_9) -e9  0x02000800	-r9 0x02000000	-b	icon.bmp "ToolchainGenericDS SDK;$(TGDSPROJECTNAME) NDS Binary; "
	-mv $(EXECUTABLE_FNAME)	release/arm7dldi-ntr
	-mv $(CURDIR)/arm9/data/tgds_multiboot_payload.bin $(CURDIR)/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin
	-mv $(CURDIR)/arm9/data/tgds_multiboot_payload_twl.bin $(CURDIR)/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin
	$(NDSTOOL)	-c 	${@:.nds=.srl} -7  $(CURDIR)/arm7/arm7_twl.bin	-e7  0x02380000	-9 $(CURDIR)/arm9/arm9_twl.bin -e9  0x02000800	-r9 0x02000000	\
	-g "TGDS" "NN" "NDS.TinyFB"	\
	-z 80040000 -u 00030004 -a 00000138 \
	-b icon.bmp "$(TGDSPROJECTNAME);$(TGDSPROJECTNAME) TWL Binary;" \
	-h "twlheader.bin"
	-mv ${@:.nds=.srl}	release/arm7dldi-twl
	-cp $(CURDIR)/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../armv4core/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin
	-cp $(CURDIR)/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds} $(CURDIR)/../armv4core/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-cp $(CURDIR)/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../cgmodels/targetClientRenderers/NintendoDS/Env_Mapping/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin
	-cp $(CURDIR)/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds} $(CURDIR)/../cgmodels/targetClientRenderers/NintendoDS/Env_Mapping/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-cp $(CURDIR)/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../cgmodels/targetClientRenderers/NintendoDS/TexturedBlenderModelNDSRender/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin
	-cp $(CURDIR)/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds} $(CURDIR)/../cgmodels/targetClientRenderers/NintendoDS/TexturedBlenderModelNDSRender/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-cp $(CURDIR)/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../gbaARMHook/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin
	-cp $(CURDIR)/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds} $(CURDIR)/../gbaARMHook/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-cp $(CURDIR)/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../ToolchainGenericDS/lib/Woopsi/woopsiExamples/demoWoopsi/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin
	-cp $(CURDIR)/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds} $(CURDIR)/../ToolchainGenericDS/lib/Woopsi/woopsiExamples/demoWoopsi/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-cp $(CURDIR)/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../ToolchainGenericDS-argvtest/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin
	-cp $(CURDIR)/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds} $(CURDIR)/../ToolchainGenericDS-argvtest/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-cp $(CURDIR)/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../ToolchainGenericDS-filedownload/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin
	-cp $(CURDIR)/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds} $(CURDIR)/../ToolchainGenericDS-filedownload/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-cp $(CURDIR)/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../ToolchainGenericDS-foobilliard/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin
	-cp $(CURDIR)/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds} $(CURDIR)/../ToolchainGenericDS-foobilliard/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-cp $(CURDIR)/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../ToolchainGenericDS-FTPServer/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin
	-cp $(CURDIR)/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds} $(CURDIR)/../ToolchainGenericDS-FTPServer/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-cp $(CURDIR)/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../ToolchainGenericDS-multimediaplayer/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin
	-cp $(CURDIR)/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds} $(CURDIR)/../ToolchainGenericDS-multimediaplayer/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-cp $(CURDIR)/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../ToolchainGenericDS-OnlineApp/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin
	-cp $(CURDIR)/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds} $(CURDIR)/../ToolchainGenericDS-OnlineApp/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-cp $(CURDIR)/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../ToolchainGenericDS-template/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin
	-cp $(CURDIR)/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds} $(CURDIR)/../ToolchainGenericDS-template/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-cp $(CURDIR)/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../ToolchainGenericDS-UnitTest/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin
	-cp $(CURDIR)/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds} $(CURDIR)/../ToolchainGenericDS-UnitTest/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-cp $(CURDIR)/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../ToolchainGenericDS-wmbhost/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin
	-cp $(CURDIR)/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds} $(CURDIR)/../ToolchainGenericDS-wmbhost/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-cp $(CURDIR)/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../ToolchainGenericDS-zlib-example/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin
	-cp $(CURDIR)/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds} $(CURDIR)/../ToolchainGenericDS-zlib-example/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-cp $(CURDIR)/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../snemulds/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin
	-cp $(CURDIR)/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds} $(CURDIR)/../snemulds/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-cp $(CURDIR)/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../tgdsproject3d/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin
	-cp $(CURDIR)/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds} $(CURDIR)/../tgdsproject3d/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-cp $(CURDIR)/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../snakegl/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin
	-cp $(CURDIR)/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds} $(CURDIR)/../snakegl/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-cp $(CURDIR)/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../aquariumgl/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin
	-cp $(CURDIR)/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds} $(CURDIR)/../aquariumgl/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-cp $(CURDIR)/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../batallionnds/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin
	-cp $(CURDIR)/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds} $(CURDIR)/../batallionnds/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-@echo '-'
	-cp $(CURDIR)/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../armv4core/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin
	-cp $(CURDIR)/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl} $(CURDIR)/../armv4core/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-cp $(CURDIR)/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../cgmodels/targetClientRenderers/NintendoDS/Env_Mapping/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin
	-cp $(CURDIR)/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl} $(CURDIR)/../cgmodels/targetClientRenderers/NintendoDS/Env_Mapping/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-cp $(CURDIR)/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../cgmodels/targetClientRenderers/NintendoDS/TexturedBlenderModelNDSRender/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin
	-cp $(CURDIR)/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl} $(CURDIR)/../cgmodels/targetClientRenderers/NintendoDS/TexturedBlenderModelNDSRender/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-cp $(CURDIR)/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../gbaARMHook/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin
	-cp $(CURDIR)/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl} $(CURDIR)/../gbaARMHook/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-cp $(CURDIR)/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../ToolchainGenericDS/lib/Woopsi/woopsiExamples/demoWoopsi/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin
	-cp $(CURDIR)/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl} $(CURDIR)/../ToolchainGenericDS/lib/Woopsi/woopsiExamples/demoWoopsi/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-cp $(CURDIR)/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../ToolchainGenericDS-argvtest/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin
	-cp $(CURDIR)/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl} $(CURDIR)/../ToolchainGenericDS-argvtest/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-cp $(CURDIR)/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../ToolchainGenericDS-filedownload/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin
	-cp $(CURDIR)/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl} $(CURDIR)/../ToolchainGenericDS-filedownload/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-cp $(CURDIR)/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../ToolchainGenericDS-foobilliard/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin
	-cp $(CURDIR)/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl} $(CURDIR)/../ToolchainGenericDS-foobilliard/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-cp $(CURDIR)/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../ToolchainGenericDS-FTPServer/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin
	-cp $(CURDIR)/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl} $(CURDIR)/../ToolchainGenericDS-FTPServer/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-cp $(CURDIR)/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../ToolchainGenericDS-multimediaplayer/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin
	-cp $(CURDIR)/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl} $(CURDIR)/../ToolchainGenericDS-multimediaplayer/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-cp $(CURDIR)/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../ToolchainGenericDS-OnlineApp/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin
	-cp $(CURDIR)/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl} $(CURDIR)/../ToolchainGenericDS-OnlineApp/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-cp $(CURDIR)/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../ToolchainGenericDS-template/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin
	-cp $(CURDIR)/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl} $(CURDIR)/../ToolchainGenericDS-template/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-cp $(CURDIR)/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../ToolchainGenericDS-UnitTest/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin
	-cp $(CURDIR)/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl} $(CURDIR)/../ToolchainGenericDS-UnitTest/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-cp $(CURDIR)/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../ToolchainGenericDS-wmbhost/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin
	-cp $(CURDIR)/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl} $(CURDIR)/../ToolchainGenericDS-wmbhost/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-cp $(CURDIR)/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../ToolchainGenericDS-zlib-example/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin
	-cp $(CURDIR)/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl} $(CURDIR)/../ToolchainGenericDS-zlib-example/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-cp $(CURDIR)/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../snemulds/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin
	-cp $(CURDIR)/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl} $(CURDIR)/../snemulds/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-cp $(CURDIR)/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../tgdsproject3d/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin
	-cp $(CURDIR)/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl} $(CURDIR)/../tgdsproject3d/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-cp $(CURDIR)/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../snakegl/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin
	-cp $(CURDIR)/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl} $(CURDIR)/../snakegl/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-cp $(CURDIR)/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../aquariumgl/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin
	-cp $(CURDIR)/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl} $(CURDIR)/../aquariumgl/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-cp $(CURDIR)/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../batallionnds/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin
	-cp $(CURDIR)/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl} $(CURDIR)/../batallionnds/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-@echo 'ndstool end: built: $@'
	
#---------------------------------------------------------------------------------
# Clean
each_obj = $(foreach dirres,$(dir_read_arm9_files),$(dirres).)
	
clean:
	$(MAKE)	clean	-C	$(DIR_ARM7)/
	$(MAKE) clean	-C	$(PosIndCodeDIR_FILENAME)/$(DIR_ARM7)/
	$(MAKE) clean	-C	arm7bootldr/
ifeq ($(SOURCE_MAKEFILE7),default)
	-@rm -rf $(CURDIR)/$(DIR_ARM7)/Makefile
endif
#--------------------------------------------------------------------	
	$(MAKE) clean	-C	$(CURDIR)/$(DECOMPRESSOR_BOOTCODE_9)/
	$(MAKE)	clean	-C	$(DIR_ARM9)/
	$(MAKE) clean	-C	$(PosIndCodeDIR_FILENAME)/$(DIR_ARM9)/
ifeq ($(SOURCE_MAKEFILE9),default)
	-@rm -rf $(CURDIR)/$(DIR_ARM9)/Makefile
endif
	-@rm -rf $(CURDIR)/$(PosIndCodeDIR_FILENAME)/$(DIR_ARM7)/Makefile
	-@rm -rf $(CURDIR)/$(PosIndCodeDIR_FILENAME)/$(DIR_ARM9)/Makefile
	-@rm -rf $(CURDIR)/../armv4core/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../armv4core/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-@rm -rf $(CURDIR)/../cgmodels/targetClientRenderers/NintendoDS/Env_Mapping/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../cgmodels/targetClientRenderers/NintendoDS/Env_Mapping/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-@rm -rf $(CURDIR)/../cgmodels/targetClientRenderers/NintendoDS/TexturedBlenderModelNDSRender/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../cgmodels/targetClientRenderers/NintendoDS/TexturedBlenderModelNDSRender/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-@rm -rf $(CURDIR)/../gbaARMHook/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../gbaARMHook/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-@rm -rf $(CURDIR)/../ToolchainGenericDS/lib/Woopsi/woopsiExamples/demoWoopsi/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../ToolchainGenericDS/lib/Woopsi/woopsiExamples/demoWoopsi/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-@rm -rf $(CURDIR)/../ToolchainGenericDS-argvtest/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../ToolchainGenericDS-argvtest/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-@rm -rf $(CURDIR)/../ToolchainGenericDS-filedownload/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../ToolchainGenericDS-filedownload/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-@rm -rf $(CURDIR)/../ToolchainGenericDS-foobilliard/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../ToolchainGenericDS-foobilliard/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-@rm -rf $(CURDIR)/../ToolchainGenericDS-FTPServer/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../ToolchainGenericDS-FTPServer/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-@rm -rf $(CURDIR)/../ToolchainGenericDS-multimediaplayer/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../ToolchainGenericDS-multimediaplayer/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-@rm -rf $(CURDIR)/../ToolchainGenericDS-OnlineApp/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../ToolchainGenericDS-OnlineApp/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-@rm -rf $(CURDIR)/../ToolchainGenericDS-template/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../ToolchainGenericDS-template/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-@rm -rf $(CURDIR)/../ToolchainGenericDS-UnitTest/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../ToolchainGenericDS-UnitTest/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-@rm -rf $(CURDIR)/../ToolchainGenericDS-wmbhost/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../ToolchainGenericDS-wmbhost/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-@rm -rf $(CURDIR)/../ToolchainGenericDS-zlib-example/release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin $(CURDIR)/../ToolchainGenericDS-zlib-example/release/arm7dldi-ntr/${EXECUTABLE_FNAME:.nds=.nds}
	-@echo '-'
	-@rm -rf $(CURDIR)/../armv4core/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../armv4core/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-@rm -rf $(CURDIR)/../cgmodels/targetClientRenderers/NintendoDS/Env_Mapping/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../cgmodels/targetClientRenderers/NintendoDS/Env_Mapping/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-@rm -rf $(CURDIR)/../cgmodels/targetClientRenderers/NintendoDS/TexturedBlenderModelNDSRender/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../cgmodels/targetClientRenderers/NintendoDS/TexturedBlenderModelNDSRender/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-@rm -rf $(CURDIR)/../gbaARMHook/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../gbaARMHook/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-@rm -rf $(CURDIR)/../ToolchainGenericDS/lib/Woopsi/woopsiExamples/demoWoopsi/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../ToolchainGenericDS/lib/Woopsi/woopsiExamples/demoWoopsi/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-@rm -rf $(CURDIR)/../ToolchainGenericDS-argvtest/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../ToolchainGenericDS-argvtest/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-@rm -rf $(CURDIR)/../ToolchainGenericDS-filedownload/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../ToolchainGenericDS-filedownload/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-@rm -rf $(CURDIR)/../ToolchainGenericDS-foobilliard/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../ToolchainGenericDS-foobilliard/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-@rm -rf $(CURDIR)/../ToolchainGenericDS-FTPServer/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../ToolchainGenericDS-FTPServer/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-@rm -rf $(CURDIR)/../ToolchainGenericDS-multimediaplayer/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../ToolchainGenericDS-multimediaplayer/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-@rm -rf $(CURDIR)/../ToolchainGenericDS-OnlineApp/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../ToolchainGenericDS-OnlineApp/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-@rm -rf $(CURDIR)/../ToolchainGenericDS-template/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../ToolchainGenericDS-template/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-@rm -rf $(CURDIR)/../ToolchainGenericDS-UnitTest/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../ToolchainGenericDS-UnitTest/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-@rm -rf $(CURDIR)/../ToolchainGenericDS-wmbhost/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../ToolchainGenericDS-wmbhost/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	-@rm -rf $(CURDIR)/../ToolchainGenericDS-zlib-example/release/arm7dldi-twl/tgds_multiboot_payload_twl.bin $(CURDIR)/../ToolchainGenericDS-zlib-example/release/arm7dldi-twl/${EXECUTABLE_FNAME:.nds=.srl}
	

	






	-@rm -fr $(EXECUTABLE_FNAME)	$(TGDSPROJECTNAME).srl		$(CURDIR)/common/templateCode/	tgds_multiboot_payload/data/arm7bootldr.bin	$(CURDIR)/$(DECOMPRESSOR_BOOTCODE_9)/$(BINSTRIP_RULE_COMPRESSED_9)	release/arm7dldi-ntr/$(EXECUTABLE_FNAME)	release/arm7dldi-ntr/tgds_multiboot_payload_ntr.bin	release/arm7dldi-twl/tgds_multiboot_payload_twl.bin	release/arm7dldi-twl/ToolchainGenericDS-multiboot.srl

rebase:
	git reset --hard HEAD
	git clean -f -d
	git pull

commitChanges:
	-@git commit -a	-m '$(COMMITMSG)'
	-@git push origin HEAD

#---------------------------------------------------------------------------------

switchStable:
	-@git checkout -f	'TGDS1.65'

#---------------------------------------------------------------------------------

switchMaster:
	-@git checkout -f	'master'

#---------------------------------------------------------------------------------

#ToolchainGenericDS Package deploy format required by ToolchainGenericDS-OnlineApp.
BuildTGDSPKG:
	-@echo 'Build TGDS Package. '
	-$(TGDSPKGBUILDER) $(TGDSPROJECTNAME) $(TGDSPROJECTNAME).srl	$(TGDSPKG_TARGET_PATH) $(LIBPATH) /release/arm7dldi-ntr/

#---------------------------------------------------------------------------------
remotebootTWL:
	-mv $(TGDSPROJECTNAME).srl	$(CURDIR)/release/arm7dldi-twl
	-chmod 777 -R $(CURDIR)/release/arm7dldi-twl
	-$(TGDSREMOTEBOOTER) /release/arm7dldi-twl $(TGDSREMOTEBOOTER_SERVER_IP_ADDR) twl_mode $(TGDSPROJECTNAME) $(TGDSPKG_TARGET_PATH) $(LIBPATH) remotepackage	nogdb	$(TGDSREMOTEBOOTER_SERVER_PORT)
	-rm -rf remotepackage.zip

remotegdbTWL:
	-mv $(TGDSPROJECTNAME).srl	$(CURDIR)/release/arm7dldi-twl
	-chmod 777 -R $(CURDIR)/release/arm7dldi-twl
	-$(TGDSREMOTEBOOTER) /release/arm7dldi-twl $(TGDSREMOTEBOOTER_SERVER_IP_ADDR) twl_mode $(TGDSPROJECTNAME) $(TGDSPKG_TARGET_PATH) $(LIBPATH) remotepackage	gdbenable	$(TGDSREMOTEBOOTER_SERVER_PORT)
	-rm -rf remotepackage.zip

remotebootNTR:
	-mv $(TGDSPROJECTNAME).nds	$(CURDIR)/release/arm7dldi-ntr
	-chmod 777 -R $(CURDIR)/release/arm7dldi-ntr
	-$(TGDSREMOTEBOOTER) /release/arm7dldi-ntr $(TGDSREMOTEBOOTER_SERVER_IP_ADDR) ntr_mode $(TGDSPROJECTNAME) $(TGDSPKG_TARGET_PATH) $(LIBPATH) remotepackage	nogdb	$(TGDSREMOTEBOOTER_SERVER_PORT)
	-rm -rf remotepackage.zip

remotegdbNTR:
	-mv $(TGDSPROJECTNAME).nds	$(CURDIR)/release/arm7dldi-ntr
	-chmod 777 -R $(CURDIR)/release/arm7dldi-ntr
	-$(TGDSREMOTEBOOTER) /release/arm7dldi-ntr $(TGDSREMOTEBOOTER_SERVER_IP_ADDR) ntr_mode $(TGDSPROJECTNAME) $(TGDSPKG_TARGET_PATH) $(LIBPATH) remotepackage	gdbenable	$(TGDSREMOTEBOOTER_SERVER_PORT)
	-rm -rf remotepackage.zip