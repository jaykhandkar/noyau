#include <stdint.h>
#include <stddef.h>
#include <printf.h>
#include <cpuid.h>

/* map 2TB of physical memory into kernel virtual address space using 1GB pages */

uint64_t kernel_pml4e[512] __attribute__((aligned(0x1000)));
uint64_t kernel_pdpts[4][512] __attribute((aligned(0x1000)));

#define MB (1 << 20)
#define GB (1 << 30)
void kernel_entry(unsigned long);

void setup_kernel_pgtables()
{
	unsigned long kernel_pml4e_phyaddr = (unsigned long) &kernel_pml4e - 0xffff800000000000;
	unsigned int eax = 0;
	unsigned int edx = 0;
	unsigned int unused = 0;

	__get_cpuid(0x80000001, &eax, &unused, &unused, &edx);
	if ((edx & (1 << 26)) == 0) {
		/* processor does not support 1GB pages
		 * TODO: use 2MB pages instead */
		return;
	}

	for (size_t i = 0; i < 4; i++)
		for (size_t j = 0; j < 512; j++)
			kernel_pdpts[i][j] = ((i * 512 * GB) + (j * GB)) | 0x80 | 0x3; /* 1GB page | supervisor | RW | present */	
	for (size_t i = 0; i < 4; i++)
		kernel_pml4e[0x100 + i] = ((uint64_t) &kernel_pdpts[i] - 0xffff800000000000)| 0x3;

	asm volatile("mov %0, %%rax\n"
		     "mov %%rax, %%cr3\n"
		     :
		     : "r"(kernel_pml4e_phyaddr)
		     : "rax");
}
