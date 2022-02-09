ENTRY (entry64)

SECTIONS
{
	KERNEL_VMA = 0xffff800000000000;
	/* The kernel will live at 0xffff800000000000 + 2MB in the virtual address space, */
	/* which will be mapped to 2MB in the physical address space. 
	 * 0xffff800000000000 is the start of the higher half of the 48 bit canonical address
	 * space. */
	. = KERNEL_VMA + 2M;

	_kernel_start = .;
	.text ALIGN (4K) : AT (ADDR (.text) - KERNEL_VMA)
	{
		*(.text)
	}
	.rodata ALIGN (4K) : AT (ADDR (.rodata) - KERNEL_VMA)
	{
		*(.rodata)
	} 
	.data ALIGN (4K) : AT (ADDR (.data) - KERNEL_VMA)
	{
		*(.data)
	}
	.bss ALIGN (4K) : AT (ADDR (.bss) - KERNEL_VMA)
	{
		*(COMMON)
		*(.bss)
		*(.bootstrap_stack)
	}
	_kernel_end = .;
}
