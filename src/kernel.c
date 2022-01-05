#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <multiboot.h>

#define IS_SET(flags, bit) ((flags) & (1 << (bit)))

struct rgb_framebuffer {
	void    *base;
	uint32_t pitch;
	uint32_t width;
	uint32_t height;
	uint8_t  bpp;
	uint8_t  red_pos;
	uint8_t  red_size;
	uint8_t  green_pos;
	uint8_t  green_size;
	uint8_t  blue_pos;
	uint8_t  blue_size;
};

struct rgb_framebuffer rgb_fb;

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
	multiboot_info_t *mbi;

	if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
		return;

	mbi = (multiboot_info_t *) addr;

	if (IS_SET(mbi->flags, 12)) {
		switch (mbi->framebuffer_type) {
			case MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED: /* not implemented */
				return;
			case MULTIBOOT_FRAMEBUFFER_TYPE_RGB:
				rgb_fb.base = (void *) (unsigned long)mbi->framebuffer_addr;
				rgb_fb.pitch = mbi->framebuffer_pitch;
				rgb_fb.width = mbi->framebuffer_width;
				rgb_fb.height = mbi->framebuffer_height;
				rgb_fb.bpp = mbi->framebuffer_bpp;
				rgb_fb.red_pos = mbi->framebuffer_red_field_position;
				rgb_fb.red_size = mbi->framebuffer_red_mask_size;
				rgb_fb.blue_pos = mbi->framebuffer_blue_field_position;
				rgb_fb.blue_size = mbi->framebuffer_blue_mask_size;
				rgb_fb.green_pos = mbi->framebuffer_green_field_position;
				rgb_fb.green_size = mbi->framebuffer_green_mask_size;
				break;
			case MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT: /* shouldn't happen, we requested multiboot to change video mode */
				return;
		}
	}

	for (unsigned x = 0; x < rgb_fb.width / 2; x++) {
		set_pixel(&rgb_fb, x, 50, ((1 << rgb_fb.blue_size) - 1) << rgb_fb.blue_pos);
	}
}
