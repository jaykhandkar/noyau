.section .bss
.align 16
stack_bottom:
.skip 16384
stack_top:

.section .data
test:
.byte 32

.section .text
.global entry64
.code64
entry64:
	movabs $stack_top, %rsp

	movabs $fix_rip, %rax
	jmp *%rax
fix_rip:
	call kernel_entry 

	cli
2:	hlt
	jmp 2b
