ENTRY (entry64)

PHDRS
{
     text PT_LOAD;
     data PT_LOAD ;
}

SECTIONS
{
	/* The kernel will live at 3GB + 3MB in the virtual address space, */
	/* which will be mapped to 3MB in the physical address space. */
	/* Note that we page-align the sections. */
	. = 0xffff800000300000;

	_kernel_start = .;
	/* Add a symbol that indicates the start address of the kernel. */
	.text ALIGN (4K) : AT (ADDR (.text) - 0xffff800000000000)
	{
		*(.text)
	} :text
	.rodata ALIGN (4K) : AT (ADDR (.rodata) - 0xffff800000000000)
	{
		*(.rodata)
	} 
	.data ALIGN (4K) : AT (ADDR (.data) - 0xffff800000000000)
	{
		*(.data)
	} :data
	.bss ALIGN (4K) : AT (ADDR (.bss) - 0xffff800000000000)
	{
		*(COMMON)
		*(.bss)
		*(.bootstrap_stack)
	} :data
	/* Add a symbol that indicates the end address of the kernel. */
	_kernel_end = .;
}
