#ifndef _PGTABLE_H
#define _PGTABLE_H

#define PAGE_OFFSET (0xffff800000000000)

inline void *phys_to_virt(void *addr)
{
	return (void *) (unsigned long) addr + PAGE_OFFSET;
}

#endif
