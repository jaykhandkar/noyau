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

void putchar(struct rgb_framebuffer *fb, uint8_t c, uint32_t cx, uint32_t cy, uint32_t fg, uint32_t bg)
{
	if (c >= 0x80)
		return;
	struct psf1_header *header = (struct psf1_header *) &_binary_meta_font_psf_start;
	uint8_t *glyphptr = (uint8_t *)&_binary_meta_font_psf_start + sizeof(struct psf1_header) + c * header->charsize;

	for (uint32_t y = 0; y < header->charsize; y++) {
		for (uint32_t x = 0; x < 8; x++) {
			if ((*glyphptr & (0x80 >> x)) > 0)
				set_pixel(fb, cx * 8 + x, cy * header->charsize + y, fg);
			else
				set_pixel(fb, cx * 8 + x, cy * header->charsize + y, bg);
		}
		glyphptr++;
	}
}

void _putchar(char c)
{
	uint32_t white = (((1 << rgb_fb.red_size) - 1) << rgb_fb.red_pos) | (((1 << rgb_fb.blue_size) - 1) << rgb_fb.blue_pos)
	       | (((1 << rgb_fb.green_size) - 1) << rgb_fb.green_pos);
	uint32_t black = 0;

	if (c == '\n') {
		cy += 1;
		cx = 0;
		return;
	}
	putchar(&rgb_fb, c, cx, cy, white, black);	
	cx += 1;
}
