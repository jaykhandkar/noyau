#include <fb.h>
#include <printf.h>
#include <multiboot2.h>

void setup_kernel_pgtables();
void clear(struct rgb_framebuffer *);

struct rgb_framebuffer rgb_fb;

void kernel_entry(unsigned long addr)
{
	struct multiboot_tag *tag;
	struct multiboot_tag_mmap *mmap_tag;

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
						rgb_fb.base = (void *)tagfb->common.framebuffer_addr + 0xffff800000000000;
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
	setup_kernel_pgtables();
	clear(&rgb_fb);
	printk("hello from long mode.\n");
}
