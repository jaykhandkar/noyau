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
entry64:
	movabs $stack_top, %rsp
	/* this is where we would call our C code */
	mov $(10 * (1 << 20) - 1), %rax
	movb $0x3, (%rax)
	cli
1:	hlt
	jmp 1b
