#include <stdint.h>
#define MB (1 << 20)

uint64_t page_dir_ptr_tab[4] __attribute__((aligned(0x20)));
uint64_t page_dir[512] __attribute__((aligned(0x1000)));

void setup_pae_paging()
{
	page_dir_ptr_tab[0] = (uint64_t)&page_dir | 1; 
	page_dir[0] = 0b10000011;
	asm volatile ("movl %%cr4, %%eax; bts $5, %%eax; movl %%eax, %%cr4" ::: "eax"); // set bit5 in CR4 to enable PAE		 
        asm volatile ("movl %0, %%cr3" :: "r" (&page_dir_ptr_tab)); // load PDPT into CR3
	asm volatile ("movl %%cr0, %%eax; orl $0x80000000, %%eax; movl %%eax, %%cr0;" ::: "eax");
}

void *pae_map(uint64_t phys_addr)
{
	uint64_t phys_pfn = phys_addr / (2 * MB);
	page_dir[1] = phys_pfn | 0b10000011;
	page_dir[2] = (phys_pfn + 2 *MB) | 0b10000011;
	page_dir[3] = (phys_pfn + 4 * MB) | 0b10000011;
	page_dir[4] = (phys_pfn + 6 * MB) | 0b10000011;
	return (void *)(2 * MB) + (phys_addr - phys_pfn);
}
