#include <stdbool.h>
#include <printf.h>
#include <stddef.h>
#include <stdint.h>
#include <multiboot2.h>
#include <fb.h>

#define IS_SET(flags, bit) ((flags) & (1 << (bit)))

#define KB (1 << 10)
#define MB (1 << 20)
#define GB (1 << 30)

struct rgb_framebuffer rgb_fb;

extern char _kernelstart;
extern char _kernelend;

void set_pixel(struct rgb_framebuffer *fb, uint32_t x, uint32_t y, uint32_t color)
{
	switch (fb->bpp) {
		case 8:
		{
			uint8_t *pixel = (uint8_t *)(fb->base + fb->pitch * y + x);
			*pixel = color;
		}
			break;
		case 15:
		case 16:
		{
			uint16_t *pixel = (uint16_t *)(fb->base + fb->pitch * y + x * 2);
			*pixel = color;
		}
			break;
		case 24:
		{
			uint32_t *pixel = (uint32_t *)(fb->base + fb->pitch * y + x * 3);
			*pixel = (color & 0xffffff) | (*pixel & 0xff000000);
		}
			break;
		case 32:
		{
			uint32_t *pixel = (uint32_t *)(fb->base + fb->pitch * y + x * 4);
			*pixel = color;
		}
			break;	
	}
}

void kernel_main(unsigned long magic, unsigned long addr) 
{
	struct multiboot_tag *tag;
	struct multiboot_tag_mmap *mmap_tag = NULL;
	uint32_t mbi_size;

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
		}
	}
	printk("hello, kernel\n");

	if (mmap_tag != NULL) {
		char *mem_type[6] = { 0, "MULTIBOOT_MEMORY_AVAILABLE", "MULTIBOOT_MEMORY_RESERVED", "MULTIBOOT_MEMORY_ACPI_RECLAIMABLE",
			"MULTIBOOT_MEMORY_NVS", "MULTIBOOT_MEMORY_BADRAM" };
		multiboot_memory_map_t *mmap;
		uint64_t total_usable_mem = 0;

		for (mmap = mmap_tag->entries;(void *) mmap < ((void *) mmap_tag + mmap_tag->size);
				mmap = (multiboot_memory_map_t *) ((void *)mmap + mmap_tag->entry_size)) {
			if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE)
				total_usable_mem += mmap->len;

			printk("start addr: 0x%llx | length: 0x%llx | type: %s\n", mmap->addr, mmap->len, 
					mmap->type < 6 && mmap->type > 0 ? mem_type[mmap->type] : "(undefined)");	
		}

		printk("total usable memory = %d MB\n", total_usable_mem / MB);
	} else {
		printk("no memory map received from grub\n");
	}

	printk("kernel loaded at: 0x%x\n", &_kernelstart);
	printk("kernel ends at 0x%x\n", &_kernelend);
	printk("dimensions of framebuffer received: %dx%d\n", rgb_fb.width, rgb_fb.height);
	printk("mbi = 0x%x\n", addr);
	printk("mbi size = %d KB\n", mbi_size / KB);
	printk("framebuffer at 0x%x\n", rgb_fb.base);
}
