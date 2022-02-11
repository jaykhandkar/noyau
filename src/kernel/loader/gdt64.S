.set PRESENT, 0x80
.set NOT_SYS, 0x10
.set EXEC, 0x8
.set RW, 0x2
.set DC, 0x4
.set ACCESSED, 0x1

.set GRAN_4K, 0x80
.set SZ_32, 0x40 
.set LONG_MODE, 0x20

.section .data
.global gdt64
.global gdt64.pointer
.global gdt64.code
.global gdt64.data
gdt64:
	.equ gdt64.null, . - gdt64	
	.quad 0
	.equ gdt64.code, . - gdt64
	.long 0xffff
	.byte 0
	.byte PRESENT | NOT_SYS | EXEC | RW
	.byte GRAN_4K | LONG_MODE | 0xf
	.byte 0
        .equ gdt64.data, . - gdt64
        .long 0xFFFF
        .byte 0
        .byte PRESENT | NOT_SYS | RW
        .byte GRAN_4K | SZ_32 | 0xF
        .byte 0
gdt64.pointer:
	.equ gdt64.sz, . - gdt64 - 1
	.word . - gdt64 - 1 
	.long gdt64
	.long 0

string:
	.asciz "arg = 0x%lx\n"

.section .text
.global load_gdt
.type load_gdt, @function
load_gdt:
	push %ebp
	mov %esp, %ebp
	lgdt gdt64.pointer
	mov $gdt64.data, %ax
	mov %ax, %ds
	mov %ax, %ss
	mov 12(%ebp), %edi
        pushw $gdt64.code
	pushl 8(%ebp) 
	retf
