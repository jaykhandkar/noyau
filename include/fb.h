#include <stdint.h>

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

