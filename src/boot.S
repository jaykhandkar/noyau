/* multiboot constants */
.set ALIGN,    1<<0             
.set MEMINFO,  1<<1             
.set VIDEO,    1<<2
.set FLAGS,    ALIGN | MEMINFO | VIDEO
.set MAGIC,    0x1BADB002       
.set CHECKSUM, -(MAGIC + FLAGS) 
 
/* multiboot header */
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM
.long 0
.long 0
.long 0
.long 0
.long 0
.long 0
.long 0
.long 0
.long 0
 
.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:
 
.section .text
.global _start
.type _start, @function
_start:
	mov $stack_top, %esp
 
	pushl $0
	popf

	pushl %ebx
	pushl %eax
	
	call kernel_main
 
	cli
1:	hlt
	jmp 1b
 
