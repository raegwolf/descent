/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.  
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/
/*
 * $Source: f:/miner/source/texmap/rcs/scanline.c $
 * $Revision: 1.2 $
 * $Author: john $
 * $Date: 1995/02/20 18:23:39 $
 * 
 * Routines to draw the texture mapped scanlines.
 * 
 * $Log: scanline.c $
 * Revision 1.2  1995/02/20  18:23:39  john
 * Added new module for C versions of inner loops.
 * 
 * Revision 1.1  1995/02/20  17:42:27  john
 * Initial revision
 * 
 * 
 */


#if defined(MACOS)
//#pragma off (unreferenced)
//static char rcsid[] = "$Id: scanline.c 1.3 1996/01/24 16:38:16 champaign Exp $";
//#pragma on (unreferenced)
#else
#pragma off (unreferenced)
static char rcsid[] = "$Id: scanline.c 1.2 1995/02/20 18:23:39 john Exp $";
#pragma on (unreferenced)
#endif

#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>

#include "fix.h"
#include "mono.h"
#include "gr.h"
#include "grdef.h"
#include "texmap.h"
#include "texmapl.h"
#include "scanline.h"

#if defined(MACOS)
extern ubyte * dest_row_data;
extern int loop_count;

/* Used by the macOS model smoke test to prove this renderer was exercised. */
unsigned macos_tmap_scanline_count;

/*
 * Portable replacement for TMAP_PER.ASM's tmap_loop_fast.
 *
 * The DOS loop receives U/Z, V/Z, Z, their x deltas, the destination in EDI,
 * the texture selector, lighting state, transparency state, and loop_count
 * through the texture-mapper globals/register contract.  Its NBITS=4 path
 * projects the endpoints of each 16-pixel group with IDIV, interpolates U/V
 * inside that group, then sends the short tail through the per-pixel loop.
 * The C path below performs the same grouping with fixed-width arithmetic;
 * the ungrouped tail retains the reference C division behavior.
 */
#define PERSPECTIVE_BLOCK_PIXELS 16

static fix macos_project_texture_coordinate(fix numerator, fix denominator)
{
	return (fix)(((int64_t)numerator * (int64_t)F1_0) / denominator);
}

static int macos_projected_texture_index(fix projected_u, fix projected_v)
{
	return ((projected_v >> 16) & (64 * 63)) +
	       ((projected_u >> 16) & 63);
}

#else
#endif
void c_tmap_scanline_flat()
{
	ubyte *dest;
	int x;

#if defined(MACOS)
	dest = dest_row_data;
#else
	dest = (ubyte *)(write_buffer + fx_xleft + (bytes_per_row * fx_y )  );
#endif

#if defined(MACOS)
	for (x=loop_count; x >= 0; x-- ) {
#else
	for (x= fx_xright-fx_xleft+1 ; x > 0; --x ) {
#endif
		*dest++ = tmap_flat_color;
	}
}

void c_tmap_scanline_shaded()
{
	int fade;
	ubyte *dest;
	int x;

#if defined(MACOS)
	dest = dest_row_data;
#else
	dest = (ubyte *)(write_buffer + fx_xleft + (bytes_per_row * fx_y)  );
#endif

	fade = tmap_flat_shade_value<<8;
#if defined(MACOS)
	for (x=loop_count; x >= 0; x-- ) {
		ubyte c = gr_fade_table[ fade |(*dest)];
		*dest++ = c;
#else
	for (x= fx_xright-fx_xleft+1 ; x > 0; --x ) {
		*dest++ = gr_fade_table[ fade |(*dest)];
#endif
	}
}

void c_tmap_scanline_lin_nolight()
{
	ubyte *dest;
	uint c;
	int x;
	fix u,v,dudx, dvdx;

	u = fx_u;
	v = fx_v*64;
	dudx = fx_du_dx; 
	dvdx = fx_dv_dx*64; 

#if defined(MACOS)
	dest = dest_row_data;
#else
	dest = (ubyte *)(write_buffer + fx_xleft + (bytes_per_row * fx_y)  );
#endif

	if (!Transparency_on)	{
#if defined(MACOS)
		for (x=loop_count; x >= 0; x-- ) {
#else
		for (x= fx_xright-fx_xleft+1 ; x > 0; --x ) {
#endif
			*dest++ = (uint)pixptr[ (f2i(v)&(64*63)) + (f2i(u)&63) ];
			u += dudx;
			v += dvdx;
		}
	} else {
#if defined(MACOS)
		for (x=loop_count; x >= 0; x-- ) {
#else
		for (x= fx_xright-fx_xleft+1 ; x > 0; --x ) {
#endif
			c = (uint)pixptr[ (f2i(v)&(64*63)) + (f2i(u)&63) ];
			if ( c!=255)
				*dest = c;
			dest++;
			u += dudx;
			v += dvdx;
		}
	}
}


void c_tmap_scanline_lin()
{
	ubyte *dest;
	uint c;
	int x;
	fix u,v,l,dudx, dvdx, dldx;

	u = fx_u;
	v = fx_v*64;
	dudx = fx_du_dx; 
	dvdx = fx_dv_dx*64; 

#if defined(MACOS)
	l = fx_l;
	dldx = fx_dl_dx;
	dest = dest_row_data;
#else
	l = fx_l>>8;
	dldx = fx_dl_dx>>8;
	dest = (ubyte *)(write_buffer + fx_xleft + (bytes_per_row * fx_y)  );
#endif

	if (!Transparency_on)	{
#if defined(MACOS)
		for (x=loop_count; x >= 0; x-- ) {
#else
		for (x= fx_xright-fx_xleft+1 ; x > 0; --x ) {
#endif
			*dest++ = gr_fade_table[ (l&(0xff00)) + (uint)pixptr[ (f2i(v)&(64*63)) + (f2i(u)&63) ] ];
			l += dldx;
			u += dudx;
			v += dvdx;
		}
	} else {
#if defined(MACOS)
		for (x=loop_count; x >= 0; x-- ) {
#else
		for (x= fx_xright-fx_xleft+1 ; x > 0; --x ) {
#endif
			c = (uint)pixptr[ (f2i(v)&(64*63)) + (f2i(u)&63) ];
			if ( c!=255)
				*dest = gr_fade_table[ (l&(0xff00)) + c ];
			dest++;
			l += dldx;
			u += dudx;
			v += dvdx;
		}
	}
}


void c_tmap_scanline_per_nolight()
{
	ubyte *dest;
	uint c;
	int x;
	fix u,v,z,dudx, dvdx, dzdx;
#if defined(MACOS)
	int remaining;
	fix projected_u, projected_v;
#endif

	u = fx_u;
	v = fx_v*64;
	z = fx_z;
	dudx = fx_du_dx; 
	dvdx = fx_dv_dx*64; 
	dzdx = fx_dz_dx;

#if defined(MACOS)
	dest = dest_row_data;
#else
	dest = (ubyte *)(write_buffer + fx_xleft + (bytes_per_row * fx_y)  );
#endif

#if defined(MACOS)
	remaining = loop_count + 1;
	if (remaining >= PERSPECTIVE_BLOCK_PIXELS) {
		projected_u = macos_project_texture_coordinate(u, z);
		projected_v = macos_project_texture_coordinate(v, z);
		while (remaining >= PERSPECTIVE_BLOCK_PIXELS) {
			fix next_u = u + dudx * PERSPECTIVE_BLOCK_PIXELS;
			fix next_v = v + dvdx * PERSPECTIVE_BLOCK_PIXELS;
			fix next_z = z + dzdx * PERSPECTIVE_BLOCK_PIXELS;
			fix next_projected_u = macos_project_texture_coordinate(next_u, next_z);
			fix next_projected_v = macos_project_texture_coordinate(next_v, next_z);
			fix projected_du = (fix)(((int64_t)next_projected_u - projected_u) >> 4);
			fix projected_dv = (fix)(((int64_t)next_projected_v - projected_v) >> 4);

			for (x = 0; x < PERSPECTIVE_BLOCK_PIXELS; ++x) {
				c = (uint)pixptr[macos_projected_texture_index(projected_u,
				                                                   projected_v)];
				if (!Transparency_on || c != 255)
					*dest = (ubyte)c;
				dest++;
				projected_u += projected_du;
				projected_v += projected_dv;
			}

			u = next_u;
			v = next_v;
			z = next_z;
			projected_u = next_projected_u;
			projected_v = next_projected_v;
			remaining -= PERSPECTIVE_BLOCK_PIXELS;
		}
	}

	for (x = remaining; x > 0; --x) {
		c = (uint)pixptr[((v / z) & (64 * 63)) + ((u / z) & 63)];
		if (!Transparency_on || c != 255)
			*dest = (ubyte)c;
		dest++;
		u += dudx;
		v += dvdx;
		z += dzdx;
	}
#else
	if (!Transparency_on)	{
		for (x= fx_xright-fx_xleft+1 ; x > 0; --x ) {
			*dest++ = (uint)pixptr[ ( (v/z)&(64*63) ) + ((u/z)&63) ];
			u += dudx;
			v += dvdx;
			z += dzdx;
		}
	} else {
		for (x= fx_xright-fx_xleft+1 ; x > 0; --x ) {
			c = (uint)pixptr[ ( (v/z)&(64*63) ) + ((u/z)&63) ];
			if ( c!=255)
				*dest = c;
			dest++;
			u += dudx;
			v += dvdx;
			z += dzdx;
		}
	}
#endif
}

void c_tmap_scanline_per()
{
	ubyte *dest;
	uint c;
	int x;
	fix u,v,z,l,dudx, dvdx, dzdx, dldx;
#if defined(MACOS)
	int remaining;
	fix projected_u, projected_v;
#endif

#if defined(MACOS)
	macos_tmap_scanline_count++;
#else
#endif
	u = fx_u;
	v = fx_v*64;
	z = fx_z;
	dudx = fx_du_dx; 
	dvdx = fx_dv_dx*64; 
	dzdx = fx_dz_dx;

#if defined(MACOS)
	l = fx_l;
	dldx = fx_dl_dx;
	dest = dest_row_data;
#else
	l = fx_l>>8;
	dldx = fx_dl_dx>>8;
	dest = (ubyte *)(write_buffer + fx_xleft + (bytes_per_row * fx_y)  );
#endif

#if defined(MACOS)
	remaining = loop_count + 1;
	if (remaining >= PERSPECTIVE_BLOCK_PIXELS) {
		projected_u = macos_project_texture_coordinate(u, z);
		projected_v = macos_project_texture_coordinate(v, z);
		while (remaining >= PERSPECTIVE_BLOCK_PIXELS) {
			fix next_u = u + dudx * PERSPECTIVE_BLOCK_PIXELS;
			fix next_v = v + dvdx * PERSPECTIVE_BLOCK_PIXELS;
			fix next_z = z + dzdx * PERSPECTIVE_BLOCK_PIXELS;
			fix next_projected_u = macos_project_texture_coordinate(next_u, next_z);
			fix next_projected_v = macos_project_texture_coordinate(next_v, next_z);
			fix projected_du = (fix)(((int64_t)next_projected_u - projected_u) >> 4);
			fix projected_dv = (fix)(((int64_t)next_projected_v - projected_v) >> 4);

			for (x = 0; x < PERSPECTIVE_BLOCK_PIXELS; ++x) {
				c = (uint)pixptr[macos_projected_texture_index(projected_u,
				                                                   projected_v)];
				if (!Transparency_on || c != 255)
					*dest = gr_fade_table[(l & 0xff00) + c];
				dest++;
				l += dldx;
				projected_u += projected_du;
				projected_v += projected_dv;
			}

			u = next_u;
			v = next_v;
			z = next_z;
			projected_u = next_projected_u;
			projected_v = next_projected_v;
			remaining -= PERSPECTIVE_BLOCK_PIXELS;
		}
	}

	for (x = remaining; x > 0; --x) {
		c = (uint)pixptr[((v / z) & (64 * 63)) + ((u / z) & 63)];
		if (!Transparency_on || c != 255)
			*dest = gr_fade_table[(l & 0xff00) + c];
		dest++;
		l += dldx;
		u += dudx;
		v += dvdx;
		z += dzdx;
	}
#else
	if (!Transparency_on)	{
		for (x= fx_xright-fx_xleft+1 ; x > 0; --x ) {
			*dest++ = gr_fade_table[ (l&(0xff00)) + (uint)pixptr[ ( (v/z)&(64*63) ) + ((u/z)&63) ] ];
			l += dldx;
			u += dudx;
			v += dvdx;
			z += dzdx;
		}
	} else {
		for (x= fx_xright-fx_xleft+1 ; x > 0; --x ) {
			c = (uint)pixptr[ ( (v/z)&(64*63) ) + ((u/z)&63) ];
			if ( c!=255)
				*dest = gr_fade_table[ (l&(0xff00)) + c ];
			dest++;
			l += dldx;
			u += dudx;
			v += dvdx;
			z += dzdx;
		}
	}
#endif
}
#if defined(MACOS)

#define zonk 1

void c_tmap_scanline_editor()
{
	ubyte *dest;
	uint c;
	int x;
	fix u,v,z,dudx, dvdx, dzdx;

	u = fx_u;
	v = fx_v*64;
	z = fx_z;
	dudx = fx_du_dx; 
	dvdx = fx_dv_dx*64; 
	dzdx = fx_dz_dx;

	dest = dest_row_data;

	if (!Transparency_on)	{
		for (x=loop_count; x >= 0; x-- ) {
			*dest++ = zonk;
			//(uint)pixptr[ ( (v/z)&(64*63) ) + ((u/z)&63) ];
			u += dudx;
			v += dvdx;
			z += dzdx;
		}
	} else {
		for (x=loop_count; x >= 0; x-- ) {
			c = (uint)pixptr[ ( (v/z)&(64*63) ) + ((u/z)&63) ];
			if ( c!=255)
				*dest = zonk;
			dest++;
			u += dudx;
			v += dvdx;
			z += dzdx;
		}
	}
}
#else
#endif

