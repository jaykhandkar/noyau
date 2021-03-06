#define ASM_FILE 1
#include <multiboot2.h>
 
/* multiboot header */
.section .multiboot
.code32
.align 8
multiboot_header:
.long MULTIBOOT2_HEADER_MAGIC
.long MULTIBOOT_ARCHITECTURE_I386
.long multiboot_header_end - multiboot_header
.long   -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT_ARCHITECTURE_I386 + (multiboot_header_end - multiboot_header))
.align 8
framebuffer_tag_start:
.short MULTIBOOT_HEADER_TAG_FRAMEBUFFER
.short MULTIBOOT_HEADER_TAG_OPTIONAL
.long framebuffer_tag_end - framebuffer_tag_start
.long 1920
.long 1080
.long 32
framebuffer_tag_end:
.align 8
.short MULTIBOOT_HEADER_TAG_END
.short 0
.long 8
multiboot_header_end:
 
.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:
 
.section .text
.code32
.global _start
.type _start, @function
_start:
	mov $stack_top, %esp
 
	pushl $0
	popf

	pushl %ebx
	pushl %eax
	
	call loader_main
 
	cli
1:	hlt
	jmp 1b
 
