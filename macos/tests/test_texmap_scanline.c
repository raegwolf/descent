#undef NDEBUG
#include "fix.h"
#include "gr.h"

#include <assert.h>
#include <string.h>

int Transparency_on;
ubyte *dest_row_data;
int loop_count;
fix fx_u, fx_v, fx_z, fx_du_dx, fx_dv_dx, fx_dz_dx;
fix fx_l, fx_dl_dx;
unsigned char *pixptr;
ubyte tmap_flat_color;
ubyte tmap_flat_shade_value;
ubyte gr_fade_table[256 * GR_FADE_LEVELS];

void c_tmap_scanline_per(void);

int main(void)
{
	ubyte framebuffer[32];
	ubyte texture[64 * 64];
	int i;

	memset(framebuffer, 0xa5, sizeof(framebuffer));
	memset(texture, 0, sizeof(texture));
	texture[0] = 42;
	for (i = 0; i < 256; ++i)
		gr_fade_table[i] = (ubyte)i;

	dest_row_data = framebuffer + 8;
	loop_count = 3;
	fx_u = fx_v = 0;
	fx_z = F1_0;
	fx_du_dx = fx_dv_dx = fx_dz_dx = 0;
	fx_l = fx_dl_dx = 0;
	pixptr = texture;
	Transparency_on = 0;

	c_tmap_scanline_per();

	assert(framebuffer[7] == 0xa5);
	assert(framebuffer[8] == 42);
	assert(framebuffer[9] == 42);
	assert(framebuffer[10] == 42);
	assert(framebuffer[11] == 42);
	assert(framebuffer[12] == 0xa5);
	return 0;
}
