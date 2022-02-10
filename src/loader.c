#include <stdbool.h>
#include <printf.h>
#include <stddef.h>
#include <stdint.h>
#include <multiboot2.h>
#include <string.h>
#include <fb.h>
#include <elf.h>

#define IS_SET(flags, bit) ((flags) & (1 << (bit)))

#define KB (1 << 10)
#define MB (1 << 20)
#define GB (1 << 30)

struct rgb_framebuffer rgb_fb;

extern char _loaderstart;
extern char _loaderend;

void setup_boot_pgtables();
void load_gdt(void *entry, unsigned long addr);

int have_longmode();
int have_cpuid();

#define PAGE_OFFSET (0xffff800000000000)

/* load the 64 bit higher half kernel, assuming - 
 * higher half starts at PAGE_OFFSET 
 * kernel can be loaded within first 10 MB
 * kernel wants to be loaded after loader */

void loader_main(unsigned long magic, unsigned long addr) 
{
	struct multiboot_tag *tag;
	struct multiboot_tag_mmap *mmap_tag = NULL;
	struct multiboot_tag_module *mod = NULL;
	uint32_t mbi_size;
	uint32_t kernel_elf_size = 0;
	void *kernel_entry = NULL;
	void *kernel_begin = (void *)~0;
	void *kernel_end = NULL;
	void *kernel_module_space = NULL;
	multiboot_memory_map_t *mmap;
	multiboot_memory_map_t *kernel_mmap_region = NULL;
	multiboot_memory_map_t *loader_mmap_region = NULL;

	if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) 
		return;

	if (addr & 0x7)
		return;

	mbi_size = *(uint32_t *) addr;
	for (tag = (struct multiboot_tag *) (addr + 8); tag->type != MULTIBOOT_TAG_TYPE_END;
			tag = (struct multiboot_tag *)((void *)tag + ((tag->size + 7) & ~7))) {
		switch (tag->type) {
			case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
			{
				struct multiboot_tag_framebuffer *tagfb = 
					(struct multiboot_tag_framebuffer *) tag;
				switch( tagfb->common.framebuffer_type) {
					case MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED:
						return;
					case MULTIBOOT_FRAMEBUFFER_TYPE_RGB:
						if ((tagfb->common.framebuffer_addr & 0xFFFFFFFF00000000) != 0)
							return;
						rgb_fb.base = (void *)(unsigned long)tagfb->common.framebuffer_addr;
						rgb_fb.bpp = tagfb->common.framebuffer_bpp;
						rgb_fb.pitch = tagfb->common.framebuffer_pitch;
						rgb_fb.height = tagfb->common.framebuffer_height;
						rgb_fb.width = tagfb->common.framebuffer_width;
						rgb_fb.blue_pos = tagfb->framebuffer_blue_field_position;
						rgb_fb.blue_size = tagfb->framebuffer_blue_mask_size;
						rgb_fb.red_pos = tagfb->framebuffer_red_field_position;
						rgb_fb.red_size = tagfb->framebuffer_red_mask_size;
						rgb_fb.green_pos = tagfb->framebuffer_green_field_position;
						rgb_fb.green_size = tagfb->framebuffer_green_mask_size;
						break;
					case MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT:
						return;
					default:
						return;
				}

			}
				break;
			case MULTIBOOT_TAG_TYPE_MMAP:
				mmap_tag = (struct multiboot_tag_mmap *) tag;
				break;
			case MULTIBOOT_TAG_TYPE_MODULE:
				mod = (struct multiboot_tag_module *)tag;
				break;
		}
	}

	if ((unsigned long) rgb_fb.base == 0) { /*no framebuffer info from grub */
		return;
	}

	printk("loader loaded at: 0x%x\n", &_loaderstart);
	printk("loader ends at 0x%x\n", &_loaderend);
	printk("\ndimensions of framebuffer received: %dx%d\n", rgb_fb.width, rgb_fb.height);
	printk("framebuffer at 0x%x\n", rgb_fb.base);
	printk("\nmbi = 0x%x\n", addr);
	printk("mbi size = %d KB\n", mbi_size / KB);

	if (mod != NULL) {
		printk("\nmultiboot module received: start = 0x%x | end = 0x%x | cmdline = \"%s\"\n", mod->mod_start, mod->mod_end,
				mod->cmdline);
		kernel_elf_size = mod->mod_end - mod->mod_start;
	} else {
		printk("no kernel module received from grub\n");
		return;
	}

	if (mmap_tag != NULL) {
		char *mem_type[6] = { 0, "MULTIBOOT_MEMORY_AVAILABLE", "MULTIBOOT_MEMORY_RESERVED", "MULTIBOOT_MEMORY_ACPI_RECLAIMABLE",
			"MULTIBOOT_MEMORY_NVS", "MULTIBOOT_MEMORY_BADRAM" };
		uint64_t total_usable_mem = 0;

		printk("\nmemory map:\n");
		for (mmap = mmap_tag->entries;(void *) mmap < ((void *) mmap_tag + mmap_tag->size);
				mmap = (multiboot_memory_map_t *) ((void *)mmap + mmap_tag->entry_size)) {
			if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
				total_usable_mem += mmap->len;
			}

			printk("start addr: 0x%llx | length: 0x%llx | type: %s\n", mmap->addr, mmap->len, 
					mmap->type < 6 && mmap->type > 0 ? mem_type[mmap->type] : "(undefined)");	
		}

		printk("total usable memory = %d MB\n", total_usable_mem / MB);
	} else {
		printk("no memory map received from grub\n");
	}

	Elf64_Ehdr *hdr = (Elf64_Ehdr *) mod->mod_start;
	Elf64_Phdr *phdrs = (Elf64_Phdr *) (unsigned long)(mod->mod_start + hdr->e_phoff);
	size_t sz = hdr->e_phnum * hdr->e_phentsize;
	
	if (memcmp(&hdr->e_ident[EI_MAG0], ELFMAG, SELFMAG) != 0 || hdr->e_ident[EI_CLASS] != ELFCLASS64 || 
			hdr->e_ident[EI_DATA] != ELFDATA2LSB || hdr->e_type != ET_EXEC || hdr->e_machine != EM_X86_64 ||
			hdr->e_version != EV_CURRENT) {
		printk("invalid ELF64 file received\n");
		return;
	} else {
		printk("\nkernel elf received\n");
	}

	for (Elf64_Phdr *phdr = phdrs;(char *) phdr < (char *)phdrs + sz; 
			phdr = (Elf64_Phdr *)((void *)phdr + hdr->e_phentsize)) {
		if ((unsigned long) phdr->p_paddr < (unsigned long) kernel_begin)
			kernel_begin = (void *)(unsigned long) phdr->p_paddr;
		if ((unsigned long)(phdr->p_paddr + phdr->p_memsz) > (unsigned long) kernel_end)
			kernel_end = (void *)(unsigned long) (phdr->p_paddr + phdr->p_memsz);
	}

	/* unlikely - loader grew big enough to enter space where kernel wants to be loaded */
	if (((unsigned long) &_loaderend >= (unsigned long) kernel_begin && 
			(unsigned long) &_loaderend < (unsigned long) kernel_end) || 
			((unsigned long) &_loaderstart >= (unsigned long) kernel_begin &&
			(unsigned long) &_loaderstart < (unsigned long) kernel_end) ) {
		printk("\nloader: error: loader and kernel overlap\n");
		return;
	}

	/* make sure kernel doesn't want to be loaded inside loader */
	if (((unsigned long) kernel_begin >= (unsigned long) &_loaderstart && 
			(unsigned long) kernel_begin < (unsigned long) &_loaderend) || 
			((unsigned long) kernel_end >= (unsigned long) &_loaderstart &&
			(unsigned long) kernel_end < (unsigned long) &_loaderend) ) {
		printk("\nloader: error: loader and kernel overlap\n");
		return;
	}

	/* finally, make sure kernel fits within first 10 MB and wants to be loaded after loader */
	if ( (unsigned long) kernel_end >= (10 * MB) || (unsigned long)kernel_begin < (unsigned long) &_loaderstart) {
		printk("\nloader: error: kernel cannot fit within first 10MB\n");
		return;
	}

	for (mmap = mmap_tag->entries;(void *) mmap < ((void *) mmap_tag + mmap_tag->size);
			mmap = (multiboot_memory_map_t *) ((void *)mmap + mmap_tag->entry_size)) {
		if (((unsigned long)kernel_begin >= mmap->addr && (unsigned long)kernel_begin < mmap->addr + mmap->len && 
					(unsigned long)kernel_end > mmap->addr && 
					(unsigned long)kernel_end < mmap->addr + mmap->len )) {
			kernel_mmap_region = mmap;
		}
		if (((unsigned long)&_loaderstart >= mmap->addr && (unsigned long)&_loaderstart < mmap->addr + mmap->len && 
					(unsigned long)&_loaderend > mmap->addr && 
					(unsigned long)&_loaderend < mmap->addr + mmap->len )) {
			loader_mmap_region = mmap;
		}

	}
	printk("\nloader mmap region = 0x%x\n", loader_mmap_region->addr);

	/* ensure region where kernel wants to be loaded is normal memory */
	if (loader_mmap_region == NULL || kernel_mmap_region == NULL || kernel_mmap_region->type != MULTIBOOT_MEMORY_AVAILABLE) {
		printk("\nloader: region where kernel wants to be loaded does not lie in normal memory\n");
		return;
	}

	if ((mod->mod_start > (unsigned long) kernel_begin && mod->mod_start < (unsigned long) kernel_end) || 
			(mod->mod_end > (unsigned long) kernel_begin && mod->mod_end < (unsigned long) kernel_end) || 
			((unsigned long)kernel_begin > mod->mod_start && (unsigned long)kernel_end < mod->mod_end) ) {
		/* region where grub loaded kernel elf file overlaps with where kernel wants to be loaded
		 * in this case search for a region big enough to hold kernel elf file and move it there */
		
		/* first, check if it can fit right after kernel */
		if ((unsigned long) kernel_end + kernel_elf_size < kernel_mmap_region->addr + kernel_mmap_region->len)
			kernel_module_space = kernel_end;
		/* check if it can fit right after loader */
		else if ((unsigned long) &_loaderend + kernel_elf_size < loader_mmap_region->addr + loader_mmap_region->len &&
				(unsigned long) &_loaderend + kernel_elf_size < (unsigned long)kernel_begin)
			kernel_module_space = (void *)(unsigned long) &_loaderend;
		/* finally scan all regions */
		else {
			for (mmap = mmap_tag->entries;(void *) mmap < ((void *) mmap_tag + mmap_tag->size);
					mmap = (multiboot_memory_map_t *) ((void *)mmap + mmap_tag->entry_size)) {
				if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
					if (mmap->len > kernel_elf_size && kernel_module_space == NULL
							&& (mmap->addr & 0xffffffff00000000) == 0
							&& (mmap->addr > (unsigned long)&_loaderend || 
								(unsigned long) mmap->addr + kernel_elf_size < (unsigned long)&_loaderstart)
							&& (mmap->addr > (unsigned long) kernel_end || 
								(unsigned long) mmap->addr + kernel_elf_size < (unsigned long) kernel_begin))
						kernel_module_space = (void *) (unsigned long) mmap->addr;
				}
			}

		}

		/* no region found */
		if (kernel_module_space == NULL) {
			printk("\nloader: cannot find region to store kernel elf file\n");
			return;
		}
		
		memcpy(kernel_module_space, (void *) mod->mod_start, kernel_elf_size);
		hdr = (Elf64_Ehdr *) kernel_module_space;
		phdrs = (Elf64_Phdr *) (unsigned long)(kernel_module_space + hdr->e_phoff);
	}
	
	kernel_entry = (void *) (unsigned long) (hdr->e_entry - PAGE_OFFSET);
	for (Elf64_Phdr *phdr = phdrs;(char *) phdr < (char *)phdrs + sz; 
			phdr = (Elf64_Phdr *)((void *)phdr + hdr->e_phentsize)) {
		switch(phdr->p_type) {
			case PT_LOAD:
			{
				printk("PT_LOAD | p_addr = 0x%llx | v_addr = 0x%llx | offset = 0x%llx | mem size = 0x%llx | file sz = 0x%llx\n", 
						phdr->p_paddr, phdr->p_vaddr, phdr->p_offset, phdr->p_memsz, phdr->p_filesz);
				if (phdr->p_memsz == 0) 
					continue;
				
				memcpy((void *)(unsigned long)phdr->p_paddr, (void *)((char *)hdr + phdr->p_offset), phdr->p_filesz);

				if (phdr->p_filesz != phdr->p_memsz)
					memset((void *)(unsigned long)phdr->p_paddr + phdr->p_filesz, 0, phdr->p_memsz - phdr->p_filesz);
				
			}
				break;
		}
	}

	if (have_cpuid() != 0) {
		printk("\nloader: processor does not support cpuid\n");
		return;
	}

	switch (have_longmode()) {
		case 0:
			printk("\nloader: switching to long mode...\n");
			break;
		case 1:
			printk("\nloader: processor does not support long mode\n");
			return;
		case 2:
			printk("\nloader: processor does not support extended cpuid functions\n");
			return;
		default:
			return;
	}

	setup_boot_pgtables(); /* now in 32 bit compatibility submode */

	/* setup a 64 bit code segment and far jmp to it*/
	load_gdt(kernel_entry, addr);

}

