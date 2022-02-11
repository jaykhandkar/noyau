#include <stdint.h>

/* set up enough page tables to be able to jump to 64 bit kernel
 * identity map first 10MB and map first 10 MB of kernel virtual 
 * address space to first 10 MB of physical memory*/

#define MB (1 << 20)

uint64_t pml4t[512] __attribute__((aligned(0x1000)));
uint64_t pdpt[512] __attribute__((aligned(0x1000)));
uint64_t pdt[512] __attribute__((aligned(0x1000)));
uint64_t pgt[5][512] __attribute__((aligned(0x1000))); /* one table maps 2 MB */

void setup_boot_pgtables()
{
	/* setup identity mapping */
	for (int j = 0; j < 5; j++)
		for(int i = 0; i < 512; i++) 
			pgt[j][i] = ((2 * MB * j) + (i * 0x1000)) | 0x3; /* supervisor | write | present */
	for (int i = 0; i < 5; i++)
		pdt[i] = (uint64_t) (unsigned long)&pgt[i] | 0x03;
	pdpt[0] = (uint64_t) (unsigned long)&pdt[0] | 0x03;
	pml4t[0] = (uint64_t) (unsigned long)&pdpt[0] | 0x03;
		
	/* map first 10 MB of kernel vm space to first 10 MB of physical address space */
	pml4t[0x100] = (uint64_t) (unsigned long)&pdpt[0] | 0x03;

	/* clear any previous paging */
	asm volatile("mov %%cr0, %%eax\n"
		     "and $0x7fffffff, %%eax\n"
		     "mov %%eax, %%cr0\n"
		     :
		     :
		     : "eax" );

	/* load cr3 and enable PAE */
	asm volatile("mov %0, %%eax\n"
		     "mov %%eax, %%cr3\n"
		     "mov %%cr4, %%eax\n"
		     "or $0x20, %%eax\n"
		     "mov %%eax, %%cr4\n"
		     :
		     : "r"(&pml4t)
		     : "eax" );

	/* set long mode bit and enable paging */
	asm volatile("mov $0xC0000080, %%ecx\n"
		     "rdmsr\n"
		     "or $0x100, %%eax\n"
		     "wrmsr\n"
		     "mov %%cr0, %%eax\n"
		     "or $0x80000000, %%eax\n"
		     "mov %%eax, %%cr0\n"
		     :
		     :
		     : "eax", "ecx");

}

