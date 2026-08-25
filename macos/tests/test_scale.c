#undef NDEBUG
#include "gr.h"

#include <assert.h>
#include <string.h>

grs_canvas *grd_curcanv;

void gr_rle_decode(ubyte *src, ubyte *dest)
{
	(void)src;
	(void)dest;
}

void scale_bitmap_c(grs_bitmap *source_bmp, grs_bitmap *dest_bmp,
			    int x0, int y0, int x1, int y1,
			    fix u0, fix v0, fix u1, fix v1);

int main(void)
{
	ubyte source_pixels[] = {
		1, 255, 2,
		3, 4, 255
	};
	ubyte destination_pixels[] = {
		9, 9, 9,
		9, 9, 9
	};
	grs_bitmap source;
	grs_bitmap destination;

	memset(&source, 0, sizeof(source));
	memset(&destination, 0, sizeof(destination));
	source.bm_w = destination.bm_w = 3;
	source.bm_h = destination.bm_h = 2;
	source.bm_rowsize = destination.bm_rowsize = 3;
	source.bm_type = destination.bm_type = BM_LINEAR;
	source.bm_data = source_pixels;
	destination.bm_data = destination_pixels;

	scale_bitmap_c(&source, &destination, 0, 0, 2, 1,
			 i2f(0), i2f(0), i2f(2), i2f(1));

	assert(destination_pixels[0] == 1);
	assert(destination_pixels[1] == 9);
	assert(destination_pixels[2] == 2);
	assert(destination_pixels[3] == 3);
	assert(destination_pixels[4] == 4);
	assert(destination_pixels[5] == 9);
	return 0;
}
