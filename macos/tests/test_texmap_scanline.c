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
void c_tmap_scanline_per_nolight(void);

static void initialize_fade_table(void)
{
	int level;
	int color;
	for (level = 0; level < GR_FADE_LEVELS; ++level)
		for (color = 0; color < 256; ++color)
			gr_fade_table[level * 256 + color] = (ubyte)color;
}

static void test_short_lit_tail(void)
{
	ubyte framebuffer[32];
	ubyte texture[64 * 64];

	memset(framebuffer, 0xa5, sizeof(framebuffer));
	memset(texture, 0, sizeof(texture));
	texture[0] = 42;

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
}

static void test_block_and_tail(void)
{
	ubyte framebuffer[24];
	ubyte texture[64 * 64];
	int i;

	memset(framebuffer, 0xa5, sizeof(framebuffer));
	memset(texture, 0, sizeof(texture));
	for (i = 0; i < 18; ++i)
		texture[i] = (ubyte)(i + 1);

	dest_row_data = framebuffer + 3;
	loop_count = 16;
	fx_u = fx_v = 0;
	fx_z = F1_0;
	fx_du_dx = F1_0;
	fx_dv_dx = fx_dz_dx = 0;
	pixptr = texture;
	Transparency_on = 0;

	c_tmap_scanline_per_nolight();

	assert(framebuffer[2] == 0xa5);
	for (i = 0; i < 17; ++i)
		assert(framebuffer[3 + i] == (ubyte)(i + 1));
	assert(framebuffer[20] == 0xa5);
}

static void test_block_transparency(void)
{
	ubyte framebuffer[18];
	ubyte texture[64 * 64];
	int i;

	memset(framebuffer, 0x5a, sizeof(framebuffer));
	memset(texture, 7, sizeof(texture));
	texture[8] = 255;
	dest_row_data = framebuffer + 1;
	loop_count = 15;
	fx_u = fx_v = 0;
	fx_z = F1_0;
	fx_du_dx = F1_0;
	fx_dv_dx = fx_dz_dx = 0;
	pixptr = texture;
	Transparency_on = 1;

	c_tmap_scanline_per_nolight();

	for (i = 0; i < 16; ++i)
		assert(framebuffer[1 + i] == (i == 8 ? 0x5a : 7));
}

int main(void)
{
	initialize_fade_table();
	test_short_lit_tail();
	test_block_and_tail();
	test_block_transparency();
	return 0;
}
