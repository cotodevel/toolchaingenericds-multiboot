
/*

			Copyright (C) 2017  Coto
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
USA

*/

.arch	armv5te
.cpu	arm946e-s
.arm

/****** Actual Payload decompressor ******/
.section ".init"
.global _start
_start:
@---------------------------------------------------------------------------------
	@Info from: http://problemkaputt.de/gbatek.htm
	@DS BIOS RAM Usage
	@Below memory at 27FFxxxh is mirrored to 23FFxxxh (on retail consoles with 4MB RAM), however, it should be accessed via address 27FFxxxh 
	@(for compatibility with debug consoles with 8MB RAM). Accessing it via mirrors at 2FFFxxxh is also valid (this is done by DSi enhanced games; 
	@even when running in non-DSi mode; this allows DSi games to use the same memory addresses in NDS and DSi mode).
	
	@Coto: vectors @ 0xffff0000 is bios area. Accidental undefined,dataabort,SWI,etc during MPU setup is expected. 
	@Thus, BIOS exceptions must be handled from such DS BIOS RAM stacks.
	
	@Also, MPU exceptions are triggered in USR mode (0x10&psr), SYS mode exceptions are ignored. (0x1f&psr)
	
	@disable exceptions when MPU setup. Enter SYS mode.
	ldr r0,=0x04000204
	mov r1,#0
	str r1,[r0]
	
	@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	@ 1) Give VRAM_D to ARM7 @0x06000000
	ldr	r3, =0x04000200
	ldr	r2, =0x82	@ u8 vrambits = (VRAM_D_0x06000000_ARM7 | VRAM_ENABLE);
	strb	r2, [r3, #0x43]
	@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

	mov	r0, #0xdf		@ sys
	msr	cpsr, r0
	ldr 	sp, =(0x027FF000 + 0x1000)
	
	push {r0-r3,lr}
	ldr	r3, =set0xFFFF0000FastMPUSettings
	blx	r3
	pop {r0-r3,lr}
	
	mov	r0, #0x12		@ irq
	msr	cpsr, r0
	ldr	sp, =sp_IRQ

	mov	r0, #0x13		@ svc dtcm stacks, irq enable
	msr	cpsr, r0
	ldr	sp, =sp_SVC		
	
	@ The debug handler is called: 
	@on undefined instruction exceptions, 
	@on data/prefetch aborts (caused by the protection unit), 
	@on FIQ (possibly caused by hardware debuggers). 
	@It is also called by accidental software-jumps to the reset vector, and by unused SWI numbers within range 0..1Fh.
	
	mov	r0, #0x1F		@ usr/sys dtcm stacks, irq enable (ignore all exceptions usermode must be excluded here)
	msr	cpsr, r0
	ldr	sp, =sp_SYS
	
	/* copy the VMA sections to each LMA sections for hardware RAM not initialized at self boot crt0 stage*/
	ldr                 r1,=__itcm_vma_start                 /* point our source register to the start of the .itcm section (VMA)*/
	ldr                 r2,=_itcm_start                 /* point our destination register to the I-TCM dedicated to our writable data (LMA)*/
	ldr                 r3,=_itcm_end                  /* point r3 after the last byte we will be writing (LMA)*/
	bl                  copy  
	
	ldr                 r1,=__dtcm_vma_start                /* point our source register to the start of the .dtcm section (VMA)*/
	ldr                 r2,=_dtcm_start                 /* point our destination register to the D-TCM dedicated to our writable data (LMA)*/
	ldr                 r3,=_dtcm_end                  /* point r3 after the last byte we will be writing (LMA)*/
	bl                  copy
		
	@TGDS-MB intra argv save-restore
	ldr                 r1,=0x02FFFE70                /* point our source register to the start (VMA)*/
	ldr                 r2,=argvIntraTGDSMB                 /* point our destination register to our writable data (LMA)*/
	add                 r3,r2,#(256)                  /* point r3 after the last byte we will be writing (LMA)*/
	add                 r3,r3,#(144)                  /* point r3 after the last byte we will be writing (LMA)*/
	bl                  copy
	
	mov r0, #0	@do not care about fw version anymore
	bl IRQInit /*initHardware destroys ARM7 VRAM_D, thus crashes ARM7 procesor when running from VRAM_D */
	
	ldr                 r1,=argvIntraTGDSMB                /* point our source register to the start (VMA)*/
	ldr                 r2,=0x02FFFE70                 /* point our destination register to our writable data (LMA)*/
	add                 r3,r2,#(256)                  /* point r3 after the last byte we will be writing (LMA)*/
	add                 r3,r3,#(144)                  /* point r3 after the last byte we will be writing (LMA)*/
	bl                  copy
	
	blx handleARGV
	bl getGlobalArgv
	mov r1,r0
	push {r1}
	bl getGlobalArgc
	pop {r1}
	bl	main			@entrypoint
	b exception_sysexit

.ltorg

@format:	r0 = src_vma, r1 = dest_vma 
clear:
	mov	r2, #0
clearlop:	cmp	r0, r1
	strcc	r2, [r0],#4
	bcc	clearlop
	bx lr

@format:	r0 = src_vma_section, r1 = dest_lma_start, r2 = dest_lma_end	(where both lma are a whole physical memory region from start to end range address)
copy:               
	cmp                 r2,r3                       /* check if we've reached the end */
	ldrlo               r0,[r1],#4                  /* if end not reached, get word and advance source pointer */
	strlo               r0,[r2],#4                  /* if end not reached, store word and advance destination pointer */
	blo                 copy                        /* if end not reached, branch back to loop */
	bx                  lr                          /* return to caller */

.global argvIntraTGDSMB
argvIntraTGDSMB:
	.skip 0x200
	
.pool
.align
.end