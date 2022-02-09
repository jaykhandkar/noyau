.section .text
.global have_longmode
.type have_longmode, @function
have_longmode:
	push %ebx
	mov $0x80000000, %eax
	cpuid
	cmp $0x80000001, %eax 
	jb .noxtend

	mov $0x80000001, %eax
	cpuid
	test $0x20000000, %edx
	jz .nolongmode
 	
	mov $0x0, %eax
	pop %ebx
	ret
.nolongmode:
	mov $0x1, %eax
	pop %ebx
	ret
.noxtend:
	mov $0x2, %eax
        pop %ebx
	ret

.global have_cpuid
.type have_cpuid, @function
have_cpuid:
        pushfl
        pop %eax

        mov %eax, %ecx

        xor $0x00200000, %eax

        push %eax
        popfl

        pushfl
        pop %eax

	push %ecx
	popfl

	cmp %eax, %ecx
	je .no_cpuid
	mov $0x0, %eax
	ret
.no_cpuid:
	mov $0x1, %eax
	ret
