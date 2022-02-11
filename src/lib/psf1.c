#include <stdint.h>
#include <fb.h>
#include <psf1.h>

/* a simple ASCII text renderer using PSF1 fonts */

extern char _binary_meta_font_psf_start;
extern char _binary_meta_font_psf_end;

extern struct rgb_framebuffer rgb_fb;

static uint32_t cx;
static uint32_t cy;

void set_pixel(struct rgb_framebuffer *, uint32_t, uint32_t, uint32_t);

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

void clear(struct rgb_framebuffer *fb)
{
	for (uint32_t i = 0; i < fb->height; i++)
		for (uint32_t j = 0; j < fb->width; j++)
			set_pixel(fb, j, i, 0);
}


void putchar(struct rgb_framebuffer *fb, uint8_t c, uint32_t cx, uint32_t cy, uint32_t fg, uint32_t bg)
{
	if (c >= 0x80)
		return;
	struct psf1_header *header = (struct psf1_header *) &_binary_meta_font_psf_start;
	uint8_t charsize = header->charsize;
	uint8_t *glyphptr = (uint8_t *)&_binary_meta_font_psf_start + sizeof(struct psf1_header) + c * charsize;

	for (uint32_t y = 0; y < charsize; y++) {
		for (uint32_t x = 0; x < 8; x++) {
			if ((*glyphptr & (0x80 >> x)) > 0)
				set_pixel(fb, cx * 8 + x, cy * charsize + y, fg);
			else
				set_pixel(fb, cx * 8 + x, cy * charsize + y, bg);
		}
		glyphptr++;
	}
}

void _putchar(char c)
{
	uint32_t white = (((1 << rgb_fb.red_size) - 1) << rgb_fb.red_pos) | (((1 << rgb_fb.blue_size) - 1) << rgb_fb.blue_pos)
	       | (((1 << rgb_fb.green_size) - 1) << rgb_fb.green_pos);
	uint32_t black = 0;
	struct psf1_header *header = (struct psf1_header *) &_binary_meta_font_psf_start;
	uint8_t charsize = header->charsize;

	if (cx > (rgb_fb.width / charsize)) {
		cy += 1;
		cx = 0;
	}

	if (c == '\n') {
		cy += 1;
		cx = 0;
		return;
	}
	putchar(&rgb_fb, c, cx, cy, white, black);	
	cx += 1;
}
