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
 * $Source: f:/miner/source/texmap/rcs/ntmap.c $
 * $Revision: 1.52 $
 * $Author: john $
 * $Date: 1995/03/14 15:13:06 $
 * 
 * Start of conversion to new texture mapper.
 * 
 * $Log: ntmap.c $
 * Revision 1.52  1995/03/14  15:13:06  john
 * Increased MAX_Y_Pointers to 480.
 * 
 * Revision 1.51  1995/02/23  14:25:09  john
 * Added editor tmap.
 * 
 * Revision 1.50  1995/02/20  18:22:58  john
 * Put all the externs in the assembly modules into tmap_inc.asm.
 * Also, moved all the C versions of the inner loops into a new module, 
 * scanline.c.
 * 
 * Revision 1.49  1995/02/20  17:09:11  john
 * Added code so that you can build the tmapper with no assembly!
 * 
 * Revision 1.48  1995/01/06  11:11:30  mike
 * even when not in editor, have 400 lines in texture map scanline table.
 * 
 * Revision 1.47  1994/12/15  16:43:25  matt
 * Took out code only needed by editor
 * 
 * Revision 1.46  1994/12/09  22:35:37  mike
 * fix bug in before call to asm_tmap_scanline_per causing write of pixel onto past right border onto left.
 * 
 * Revision 1.45  1994/12/06  16:31:06  mike
 * fix bug in asm_tmap_scanline_matt interface.
 * 
 * Revision 1.44  1994/12/04  20:37:18  mike
 * *** empty log message ***
 * 
 * Revision 1.43  1994/12/02  23:30:04  mike
 * optimizations.
 * 
 * Revision 1.42  1994/11/30  00:57:43  mike
 * optimizations.
 * 
 * Revision 1.41  1994/11/28  13:34:27  mike
 * optimizations.
 * 
 * Revision 1.40  1994/11/28  01:30:01  mike
 * kill warning.
 * 
 * Revision 1.39  1994/11/28  01:28:59  mike
 * optimizations.
 * 
 * Revision 1.38  1994/11/21  14:08:07  john
 * Took out all multiple instead of divide code.
 * 
 * Revision 1.37  1994/11/19  15:21:52  mike
 * rip out unused code.
 * 
 * Revision 1.36  1994/11/14  11:42:51  mike
 * optimization.
 * 
 * Revision 1.35  1994/11/12  16:41:36  mike
 * *** empty log message ***
 * 
 * Revision 1.34  1994/11/10  21:28:41  mike
 * remove call to init_interface_vars_to_assembler.
 * 
 * Revision 1.33  1994/11/10  11:08:59  mike
 * detail level stuff.
 * 
 * Revision 1.32  1994/11/09  22:55:52  matt
 * Added variable Current_seg_depth for detail level optimization
 * 
 * Revision 1.31  1994/11/09  19:57:31  john
 * Added texture rle caching.
 * 
 * Revision 1.30  1994/11/09  19:54:48  mike
 * Call flat shader if Tmap_flat_flag set.
 * 
 * Revision 1.29  1994/11/02  21:33:31  john
 * Added Burger Bill's optimization, ie.. 2 muls per 8 pixels.
 * 
 * Revision 1.28  1994/11/02  11:32:16  john
 * Added code for c callable inner loop and code to 
 * test dividing out z0.
 * 
 * Revision 1.27  1994/10/28  20:54:32  matt
 * Added error checking
 * 
 * Revision 1.26  1994/10/25  11:20:20  mike
 * fix bug in lighting overflow checking for one scanline tall linear texture maps.
 * 
 * Revision 1.25  1994/08/03  15:40:33  mike
 * Prevent divide overflows, decrease occurrence of precision-caused glitches.
 * 
 * Revision 1.24  1994/07/27  09:31:16  mike
 * Fix concave texture map problem, decrease occurrence of unimportant int 3.
 * 
 * Revision 1.23  1994/06/17  12:23:31  mike
 * Support non-lighted texture maps.
 * 
 * Revision 1.22  1994/06/11  08:10:24  mike
 * Fix mysterious hang bug, lighting value was out of range.
 * 
 * Revision 1.21  1994/06/09  16:10:16  mike
 * Change SC2000 from constant to variable.
 * 
 */


#if defined(MACOS)
//#pragma off (unreferenced)
//static char rcsid[] = "$Id: ntmap.c 1.66 1996/12/04 19:27:55 matt Exp $";
//#pragma on (unreferenced)
#else
#pragma off (unreferenced)
static char rcsid[] = "$Id: ntmap.c 1.52 1995/03/14 15:13:06 john Exp $";
#pragma on (unreferenced)
#endif

#define VESA 0
#define NUM_TMAPS 16

#define HEADLIGHT_LIGHTING 0

#define WIREFRAME 0
#define PERSPECTIVE 1

#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>

#if defined(MACOS)
#include "pa_enabl.h"                   //$$POLY_ACC
#else
#endif
#include "mono.h"
#include "fix.h"
#include "3d.h"
#include "gr.h"
#include "error.h"
#include "key.h"

#include "texmap.h"
#include "texmapl.h"
#include "rle.h"
#include "scanline.h"

#if defined(MACOS)
#if defined(POLY_ACC)
#include "poly_acc.h"
#endif
#else
#include "..\main\textures.h"
#endif

#define	EDITOR_TMAP	1		//if in, include extra stuff

#define F15_5 (F1_0*15 + F0_5)

// 1 means enable special sc2000 code, else enable only for Descent
#define	SC2000K	0
int	SC2000 = SC2000K;

// Temporary texture map, interface from Matt's 3d system to Mike's texture mapper.
g3ds_tmap Tmap1;

grs_bitmap Texmap_ptrs[NUM_TMAPS];
grs_bitmap Texmap4_ptrs[NUM_TMAPS];

fix Range_max=0; // debug, kill me

int	Interpolation_method=0;	// 0 = choose best method
int	Lighting_on;				// initialize to no lighting
int	Tmap_flat_flag = 0;		//	1 = render texture maps as flat shaded polygons.
int	Current_seg_depth;		// HACK INTERFACE: how far away the current segment (& thus texture) is
int	Max_perspective_depth;
int	Max_linear_depth;
int	Max_flat_depth;

#if defined(MACOS)
//variables for clipping the texture-mapper to screen region
// (render.c)
#else
#endif
extern int Window_clip_left, Window_clip_bot, Window_clip_right, Window_clip_top;

// These variables are the interface to assembler.  They get set for each texture map, which is a real waste of time.
//	They should be set only when they change, which is generally when the window bounds change.  And, even still, it's
//	a pretty bad interface.
int	bytes_per_row=-1;
#if defined(MACOS)
/*
 * TMAP_*.ASM treated framebuffer addresses as 32-bit integers on DOS.
 * The portable renderer must retain the full native pointer width.
 */
ubyte	*write_buffer;
#else
int	write_buffer;
#endif
int  	window_left;
int	window_right;
int	window_top;
int	window_bottom;
#if defined(MACOS)
int	window_width;
int	window_height;
ubyte	*dest_row_data;
int	loop_count;

#if defined(MACOS)
/* Build-time guard against reintroducing the DOS 32-bit pointer ABI. */
typedef char macos_write_buffer_is_pointer[
	(sizeof(write_buffer) == sizeof(void *)) ? 1 : -1];
typedef char macos_dest_row_data_is_pointer[
	(sizeof(dest_row_data) == sizeof(void *)) ? 1 : -1];
#endif

#define	MAX_Y_POINTERS	1024
#else
int  	window_width;
int  	window_height;
int	write_buffer;
#ifdef EDITOR_TMAP
#define	MAX_Y_POINTERS	480
#else
#define	MAX_Y_POINTERS	480
#endif
#endif
int	y_pointers[MAX_Y_POINTERS];

#if defined(MACOS)
#ifdef USE_SELECTORS
short	pixel_data_selector;		// selector for current pixel data for texture mapper
#endif
#else
short	pixel_data_selector;		// selector for current pixel data for texture mapper
#endif

fix fix_recip[FIX_RECIP_TABLE_SIZE];

#if defined(MACOS)
#else
int	Lighting_enabled;
#endif
int	Fix_recip_table_computed=0;

fix fx_l, fx_u, fx_v, fx_z, fx_du_dx, fx_dv_dx, fx_dz_dx, fx_dl_dx;
#if defined(MACOS)
int fx_xleft, fx_xright, fx_y, fx_u_right, fx_v_right, fx_z_right;
#else
int fx_xleft, fx_xright, fx_y;
#endif
unsigned char * pixptr;
#if defined(MACOS)
#else
int per2_flag = 0;
#endif
int Transparency_on = 0;
int dither_intensity_lighting = 0;

ubyte * tmap_flat_cthru_table;
ubyte tmap_flat_color;
ubyte tmap_flat_shade_value;
#if defined(MACOS)

// F1_0/Z LOOKUP TABLE:
// Sig bits... 10 looks fuzzy, but only uses 4K
//             11 breaks up when close, but quite acceptable, Uses 8K
//             12	looks good,except when very close. Can be used for game.  Uses 16K. Recommended by John.

#define DIVIDE_SIG_BITS		17
#define Z_SHIFTER 			(30-DIVIDE_SIG_BITS)

#ifndef NASM
#define DIVIDE_TABLE_SIZE	(1<<DIVIDE_SIG_BITS)
ubyte divide_table_filled = 0;
fix divide_table[1<<DIVIDE_SIG_BITS];		// Can use 16 bits if we divide by the greator of Z0 or Z1, instead of always Z0.

void fill_divide_table()
{
	int i;

	divide_table_filled = 1;

	divide_table[0]=f1_0;
	divide_table[1]=f1_0;
	for (i=2; i<DIVIDE_TABLE_SIZE; i++ )	{
		// entry[i] = 1.0 / i+0.5;
		divide_table[i] = fixdiv( F1_0*2, 2*i+1 );
	}
}
#endif
#else
#endif



// -------------------------------------------------------------------------------------
void init_fix_recip_table(void)
{
	int	i;

	fix_recip[0] = F1_0;

	for (i=1; i<FIX_RECIP_TABLE_SIZE; i++)
		fix_recip[i] = F1_0/i;

	Fix_recip_table_computed = 1;
}

// -------------------------------------------------------------------------------------
//	Initialize interface variables to assembler.
//	These things used to be constants.  This routine is now (10/6/93) getting called for
//	every texture map.  It should get called whenever the window changes, or, preferably,
//	not at all.  I'm pretty sure these variables are only being used for range checking.
void init_interface_vars_to_assembler(void)
{
	grs_bitmap	*bp;

	bp = &grd_curcanv->cv_bitmap;

	Assert(bp!=NULL);
	Assert(bp->bm_data!=NULL);
	Assert(bp->bm_h <= MAX_Y_POINTERS);

	//	If bytes_per_row has changed, create new table of pointers.
	if (bytes_per_row != (int) bp->bm_rowsize) {
		int	y_val, i;

		bytes_per_row = (int) bp->bm_rowsize;

		y_val = 0;
		for (i=0; i<MAX_Y_POINTERS; i++) {
			y_pointers[i] = y_val;
			y_val += bytes_per_row;
		}
	}

#if defined(MACOS)
	write_buffer = bp->bm_data;
#else
	write_buffer = (int) bp->bm_data;
#endif

	window_left = 0;
	window_right = (int) bp->bm_w-1;
	window_top = 0;
	window_bottom = (int) bp->bm_h-1;

	Window_clip_left = window_left;
	Window_clip_right = window_right;
	Window_clip_top = window_top;
	Window_clip_bot = window_bottom;

	window_width = bp->bm_w;
	window_height = bp->bm_h;

	if (!Fix_recip_table_computed)
		init_fix_recip_table();
#if defined(MACOS)

#if defined(POLY_ACC)
    pa_set_3d_offset(bp->bm_x, bp->bm_y);
#endif

#ifndef NASM
	if ( !divide_table_filled ) fill_divide_table();
#endif
#else
#endif
}

#if defined(MACOS)
#else
int tmap_set_selector(int selector, void *buffer, unsigned int size);
#endif

// -------------------------------------------------------------------------------------
//                             VARIABLES
#if defined(MACOS)
#else
extern short	pixel_data_selector;		// selector for current pixel data for texture mapper
#endif
extern g3ds_tmap Tmap1;

// -------------------------------------------------------------------------------------
//	Returns number preceding val modulo modulus.
//	prevmod(3,4) = 2
//	prevmod(0,4) = 3
int prevmod(int val,int modulus)
{
	if (val > 0)
		return val-1;
	else
		return modulus-1;
//	return (val + modulus - 1) % modulus;
}


//	Returns number succeeding val modulo modulus.
//	succmod(3,4) = 0
//	succmod(0,4) = 1
int succmod(int val,int modulus)
{
	if (val < modulus-1)
		return val+1;
	else
		return 0;

//	return (val + 1) % modulus;
}

// -------------------------------------------------------------------------------------
//	Select topmost vertex (minimum y coordinate) and bottommost (maximum y coordinate) in
//	texture map.  If either is part of a horizontal edge, then select leftmost vertex for
//	top, rightmost vertex for bottom.
//	Important: Vertex is selected with integer precision.  So, if there are vertices at
//	(0.0,0.7) and (0.5,0.3), the first vertex is selected, because they y coordinates are
//	considered the same, so the smaller x is favored.
//	Parameters:
//		nv		number of vertices
//		v3d	pointer to 3d vertices containing u,v,x2d,y2d coordinates
//	Results in:
//		*min_y_ind
//		*max_y_ind
// -------------------------------------------------------------------------------------
void compute_y_bounds(g3ds_tmap *t, int *vlt, int *vlb, int *vrt, int *vrb,int *bottom_y_ind)
{
	int	i;
	int	min_y,max_y;
	int	min_y_ind;
	int	original_vrt;
	fix	min_x;

	// Scan all vertices, set min_y_ind to vertex with smallest y coordinate.
	min_y = f2i(t->verts[0].y2d);
	max_y = min_y;
	min_y_ind = 0;
	min_x = f2i(t->verts[0].x2d);
	*bottom_y_ind = 0;

	for (i=1; i<t->nv; i++) {
		if (f2i(t->verts[i].y2d) < min_y) {
			min_y = f2i(t->verts[i].y2d);
			min_y_ind = i;
			min_x = f2i(t->verts[i].x2d);
		} else if (f2i(t->verts[i].y2d) == min_y) {
			if (f2i(t->verts[i].x2d) < min_x) {
				min_y_ind = i;
				min_x = f2i(t->verts[i].x2d);
			}
		}
		if (f2i(t->verts[i].y2d) > max_y) {
			max_y = f2i(t->verts[i].y2d);
			*bottom_y_ind = i;
		}
	}

//--removed mk, 11/27/94--	//	Check for a non-upright-hourglass polygon and fix, if necessary, by bashing a y coordinate.
//--removed mk, 11/27/94--	//	min_y_ind = index of minimum y coordinate, *bottom_y_ind = index of maximum y coordinate
//--removed mk, 11/27/94--{
//--removed mk, 11/27/94--	int	max_temp, min_temp;
//--removed mk, 11/27/94--
//--removed mk, 11/27/94--	max_temp = *bottom_y_ind;
//--removed mk, 11/27/94--	if (*bottom_y_ind < min_y_ind)
//--removed mk, 11/27/94--		max_temp += t->nv;
//--removed mk, 11/27/94--
//--removed mk, 11/27/94--	for (i=min_y_ind; i<max_temp; i++) {
//--removed mk, 11/27/94--		if (f2i(t->verts[i%t->nv].y2d) > f2i(t->verts[(i+1)%t->nv].y2d)) {
//--removed mk, 11/27/94--			Int3();
//--removed mk, 11/27/94--			t->verts[(i+1)%t->nv].y2d = t->verts[i%t->nv].y2d;
//--removed mk, 11/27/94--		}
//--removed mk, 11/27/94--	}
//--removed mk, 11/27/94--
//--removed mk, 11/27/94--	min_temp = min_y_ind;
//--removed mk, 11/27/94--	if (min_y_ind < *bottom_y_ind)
//--removed mk, 11/27/94--		min_temp += t->nv;
//--removed mk, 11/27/94--
//--removed mk, 11/27/94--	for (i=*bottom_y_ind; i<min_temp; i++) {
//--removed mk, 11/27/94--		if (f2i(t->verts[i%t->nv].y2d) < f2i(t->verts[(i+1)%t->nv].y2d)) {
//--removed mk, 11/27/94--			Int3();
//--removed mk, 11/27/94--			t->verts[(i+1)%t->nv].y2d = t->verts[i%t->nv].y2d;
//--removed mk, 11/27/94--		}
//--removed mk, 11/27/94--	}
//--removed mk, 11/27/94--}

	// Set "vertex left top", etc. based on vertex with topmost y coordinate
	*vlt = min_y_ind;
	*vrt = *vlt;
	*vlb = prevmod(*vlt,t->nv);
	*vrb = succmod(*vrt,t->nv);

	// If right edge is horizontal, then advance along polygon bound until it no longer is or until all
	// vertices have been examined.
	// (Left edge cannot be horizontal, because *vlt is set to leftmost point with highest y coordinate.)

	original_vrt = *vrt;

	while (f2i(t->verts[*vrt].y2d) == f2i(t->verts[*vrb].y2d)) {
		if (succmod(*vrt,t->nv) == original_vrt) {
			break;
		}
		*vrt = succmod(*vrt,t->nv);
		*vrb = succmod(*vrt,t->nv);
	}
}

// -------------------------------------------------------------------------------------
//	Returns dx/dy given two vertices.
//	If dy == 0, returns 0.0
// -------------------------------------------------------------------------------------
//--fix compute_dx_dy_lin(g3ds_tmap *t, int top_vertex,int bottom_vertex)
//--{
//--	int	dy;
//--
//--	// compute delta x with respect to y for any edge
//--	dy = f2i(t->verts[bottom_vertex].y2d - t->verts[top_vertex].y2d) + 1;
//--	if (dy)
//--		return (t->verts[bottom_vertex].x2d - t->verts[top_vertex].x2d) / dy;
//--	else
//--		return 0;
//--
//--}

fix compute_du_dy_lin(g3ds_tmap *t, int top_vertex,int bottom_vertex, fix recip_dy)
{
	return fixmul(t->verts[bottom_vertex].u - t->verts[top_vertex].u, recip_dy);
}


fix compute_dv_dy_lin(g3ds_tmap *t, int top_vertex,int bottom_vertex, fix recip_dy)
{
	return fixmul(t->verts[bottom_vertex].v - t->verts[top_vertex].v, recip_dy);
}

fix compute_dl_dy_lin(g3ds_tmap *t, int top_vertex,int bottom_vertex, fix recip_dy)
{
	return fixmul(t->verts[bottom_vertex].l - t->verts[top_vertex].l, recip_dy);

}

fix compute_dx_dy(g3ds_tmap *t, int top_vertex,int bottom_vertex, fix recip_dy)
{
	return fixmul(t->verts[bottom_vertex].x2d - t->verts[top_vertex].x2d, recip_dy);
}

fix compute_du_dy(g3ds_tmap *t, int top_vertex,int bottom_vertex, fix recip_dy)
{
	return fixmul(fixmul(t->verts[bottom_vertex].u,t->verts[bottom_vertex].z) - fixmul(t->verts[top_vertex].u,t->verts[top_vertex].z), recip_dy);
}

#if defined(MACOS)
#else

#endif
fix compute_dv_dy(g3ds_tmap *t, int top_vertex,int bottom_vertex, fix recip_dy)
{
	return fixmul(fixmul(t->verts[bottom_vertex].v,t->verts[bottom_vertex].z) - fixmul(t->verts[top_vertex].v,t->verts[top_vertex].z), recip_dy);

}

fix compute_dz_dy(g3ds_tmap *t, int top_vertex,int bottom_vertex, fix recip_dy)
{
	return fixmul(t->verts[bottom_vertex].z - t->verts[top_vertex].z, recip_dy);

}
int Skip_short_flag=0;

// -------------------------------------------------------------------------------------
//	Texture map current scanline in perspective.
// -------------------------------------------------------------------------------------
#if defined(MACOS)
#else
void ntmap_scanline_lighted(grs_bitmap *srcb, int y, fix xleft, fix xright, fix uleft, fix uright, fix vleft, fix vright, fix zleft, fix zright, fix lleft, fix lright)
{
	fix	u,v,l;
	fix	dx,recip_dx;

	fix	du_dx,dv_dx,dz_dx,z;

	u = uleft;
	v = vleft;
	l = lleft;

	fx_xright = f2i(xright);
	fx_xleft = f2i(xleft);

	dx = fx_xright - fx_xleft;
	if ((dx < 0) || (xright < 0) || (xleft > xright))		// the (xleft > xright) term is not redundant with (dx < 0) because dx is computed using integers
		return;

	// setup to call assembler scanline renderer
	if (dx < FIX_RECIP_TABLE_SIZE)
		recip_dx = fix_recip[dx];
	else
		recip_dx = F1_0/dx;

	du_dx = fixmul(uright - uleft,recip_dx);
	dv_dx = fixmul(vright - vleft,recip_dx);
	dz_dx = fixmul(zright - zleft,recip_dx);

	z = zleft;

	fx_u = uleft;
	fx_v = vleft;
	fx_z = zleft;
	fx_du_dx = du_dx;
	fx_dv_dx = dv_dx;
	fx_dz_dx = dz_dx;
	fx_y = y;
	pixptr = srcb->bm_data;

	switch (Lighting_enabled) {
		case 0:
			if (fx_xleft > Window_clip_right)
				return;
			if (fx_xright < Window_clip_left)
				return;
			if (fx_xright > Window_clip_right)
				fx_xright = Window_clip_right;
			
			#ifdef NASM
				c_tmap_scanline_per_nolight();
			#else
				asm_tmap_scanline_per();
			#endif
			break;
		case 1: {
			fix	mul_thing;

			if (lleft < 0) lleft = 0;
			if (lright < 0) lright = 0;
			if (lleft > (NUM_LIGHTING_LEVELS*F1_0-F1_0/2)) lleft = (NUM_LIGHTING_LEVELS*F1_0-F1_0/2);
			if (lright > (NUM_LIGHTING_LEVELS*F1_0-F1_0/2)) lright = (NUM_LIGHTING_LEVELS*F1_0-F1_0/2);

			fx_l = lleft;
			fx_dl_dx = fixmul(lright - lleft,recip_dx);

			//	This is a pretty ugly hack to prevent lighting overflows.
			mul_thing = dx * fx_dl_dx;
			if (lleft + mul_thing < 0)
				fx_dl_dx += 12;
			else if (lleft + mul_thing > (NUM_LIGHTING_LEVELS*F1_0-F1_0/2))
				fx_dl_dx -= 12;

			if (fx_xleft > Window_clip_right)
				return;
			if (fx_xright < Window_clip_left)
				return;
			if (fx_xright > Window_clip_right)
				fx_xright = Window_clip_right;

			#ifdef NASM
				c_tmap_scanline_per();
			#else
				asm_tmap_scanline_per();
			#endif
			break;
		}
		case 2:
#ifdef EDITOR_TMAP
			fx_xright = f2i(xright);
			fx_xleft = f2i(xleft);

			asm_tmap_scanline_matt();
#else
			Int3();	//	Illegal, called an editor only routine!
#endif
			break;
	}

}
#endif

int Do_vertical_scan=0;

int	Break_on_flat=0;

#if defined(MACOS)
#else
// -------------------------------------------------------------------------------------
//	Render a texture map with lighting using perspective interpolation in inner and outer loops.
// -------------------------------------------------------------------------------------
void ntexture_map_lighted(grs_bitmap *srcb, g3ds_tmap *t)
{
	int	vlt,vrt,vlb,vrb;	// vertex left top, vertex right top, vertex left bottom, vertex right bottom
	int	topy,boty,y, dy;
	fix	dx_dy_left,dx_dy_right;
	fix	du_dy_left,du_dy_right;
	fix	dv_dy_left,dv_dy_right;
	fix	dz_dy_left,dz_dy_right;
	fix	dl_dy_left,dl_dy_right;
	fix	recip_dyl, recip_dyr;
	int	max_y_vertex;
	fix	xleft,xright,uleft,vleft,uright,vright,zleft,zright,lleft,lright;
	int	next_break_left, next_break_right;

	g3ds_vertex *v3d;

	v3d = t->verts;

#if SC2000K
if (Do_vertical_scan) {
	texture_map_hyp_lin_v(srcb, t);
	return;
}
#endif

	// Determine top and bottom y coords.
	compute_y_bounds(t,&vlt,&vlb,&vrt,&vrb,&max_y_vertex);

	// Set top and bottom (of entire texture map) y coordinates.
	topy = f2i(v3d[vlt].y2d);
	boty = f2i(v3d[max_y_vertex].y2d);
	if (topy > Window_clip_bot)
		return;
	if (boty > Window_clip_bot)
		boty = Window_clip_bot;

	// Set amount to change x coordinate for each advance to next scanline.
	dy = f2i(t->verts[vlb].y2d) - f2i(t->verts[vlt].y2d);
	if (dy < FIX_RECIP_TABLE_SIZE)
		recip_dyl = fix_recip[dy];
	else
		recip_dyl = F1_0/dy;

	dx_dy_left = compute_dx_dy(t,vlt,vlb, recip_dyl);
	du_dy_left = compute_du_dy(t,vlt,vlb, recip_dyl);
	dv_dy_left = compute_dv_dy(t,vlt,vlb, recip_dyl);
	dz_dy_left = compute_dz_dy(t,vlt,vlb, recip_dyl);

	dy = f2i(t->verts[vrb].y2d) - f2i(t->verts[vrt].y2d);
	if (dy < FIX_RECIP_TABLE_SIZE)
		recip_dyr = fix_recip[dy];
	else
		recip_dyr = F1_0/dy;

	du_dy_right = compute_du_dy(t,vrt,vrb, recip_dyr);
	dx_dy_right = compute_dx_dy(t,vrt,vrb, recip_dyr);
	dv_dy_right = compute_dv_dy(t,vrt,vrb, recip_dyr);
	dz_dy_right = compute_dz_dy(t,vrt,vrb, recip_dyr);

	if (Lighting_enabled) {
		dl_dy_left = compute_dl_dy_lin(t,vlt,vlb, recip_dyl);
		dl_dy_right = compute_dl_dy_lin(t,vrt,vrb, recip_dyr);

		lleft = v3d[vlt].l;
		lright = v3d[vrt].l;
	}

 	// Set initial values for x, u, v
	xleft = v3d[vlt].x2d;
	xright = v3d[vrt].x2d;

	zleft = v3d[vlt].z;
	zright = v3d[vrt].z;

	uleft = fixmul(v3d[vlt].u,zleft);
	uright = fixmul(v3d[vrt].u,zright);
	vleft = fixmul(v3d[vlt].v,zleft);
	vright = fixmul(v3d[vrt].v,zright);

	// scan all rows in texture map from top through first break.
	next_break_left = f2i(v3d[vlb].y2d);
	next_break_right = f2i(v3d[vrb].y2d);

	for (y = topy; y < boty; y++) {

		// See if we have reached the end of the current left edge, and if so, set
		// new values for dx_dy and x,u,v
		if (y == next_break_left) {
			fix	recip_dy;

			// Handle problem of double points.  Search until y coord is different.  Cannot get
			// hung in an infinite loop because we know there is a vertex with a lower y coordinate
			// because in the for loop, we don't scan all spanlines.
			while (y == f2i(v3d[vlb].y2d)) {
				vlt = vlb;
				vlb = prevmod(vlb,t->nv);
			}
			next_break_left = f2i(v3d[vlb].y2d);

			dy = f2i(t->verts[vlb].y2d) - f2i(t->verts[vlt].y2d);
			if (dy < FIX_RECIP_TABLE_SIZE)
				recip_dy = fix_recip[dy];
			else
				recip_dy = F1_0/dy;

			dx_dy_left = compute_dx_dy(t,vlt,vlb, recip_dy);

			xleft = v3d[vlt].x2d;
			zleft = v3d[vlt].z;
			uleft = fixmul(v3d[vlt].u,zleft);
			vleft = fixmul(v3d[vlt].v,zleft);
			lleft = v3d[vlt].l;

			du_dy_left = compute_du_dy(t,vlt,vlb, recip_dy);
			dv_dy_left = compute_dv_dy(t,vlt,vlb, recip_dy);
			dz_dy_left = compute_dz_dy(t,vlt,vlb, recip_dy);

			if (Lighting_enabled) {
				dl_dy_left = compute_dl_dy_lin(t,vlt,vlb, recip_dy);
				lleft = v3d[vlt].l;
			}
		}

		// See if we have reached the end of the current left edge, and if so, set
		// new values for dx_dy and x.  Not necessary to set new values for u,v.
		if (y == next_break_right) {
			fix	recip_dy;

			while (y == f2i(v3d[vrb].y2d)) {
				vrt = vrb;
				vrb = succmod(vrb,t->nv);
			}

			next_break_right = f2i(v3d[vrb].y2d);

			dy = f2i(t->verts[vrb].y2d) - f2i(t->verts[vrt].y2d);
			if (dy < FIX_RECIP_TABLE_SIZE)
				recip_dy = fix_recip[dy];
			else
				recip_dy = F1_0/dy;

			dx_dy_right = compute_dx_dy(t,vrt,vrb, recip_dy);

			xright = v3d[vrt].x2d;
			zright = v3d[vrt].z;
			uright = fixmul(v3d[vrt].u,zright);
			vright = fixmul(v3d[vrt].v,zright);

			du_dy_right = compute_du_dy(t,vrt,vrb, recip_dy);
			dv_dy_right = compute_dv_dy(t,vrt,vrb, recip_dy);
			dz_dy_right = compute_dz_dy(t,vrt,vrb, recip_dy);

			if (Lighting_enabled) {
				dl_dy_right = compute_dl_dy_lin(t,vrt,vrb, recip_dy);
				lright = v3d[vrt].l;
			}
		}

		if (Lighting_enabled) {
			if (y >= Window_clip_top)
				ntmap_scanline_lighted(srcb,y,xleft,xright,uleft,uright,vleft,vright,zleft,zright,lleft,lright);
			lleft += dl_dy_left;
			lright += dl_dy_right;
		} else
			if (y >= Window_clip_top)
				ntmap_scanline_lighted(srcb,y,xleft,xright,uleft,uright,vleft,vright,zleft,zright,lleft,lright);

		uleft += du_dy_left;
		vleft += dv_dy_left;

		uright += du_dy_right;
		vright += dv_dy_right;

		xleft += dx_dy_left;
		xright += dx_dy_right;

		zleft += dz_dy_left;
		zright += dz_dy_right;

	}

	// We can get lleft or lright out of bounds here because we compute dl_dy using fixed point values,
	//	but we plot an integer number of scanlines, therefore doing an integer number of additions of the delta.

//if (Break_on_flat)
//	mprintf(0, "[%i %i %i] ", y, f2i(xleft), f2i(xright));

	ntmap_scanline_lighted(srcb,y,xleft,xright,uleft,uright,vleft,vright,zleft,zright,lleft,lright);
}


// -------------------------------------------------------------------------------------
//	Texture map current scanline using linear interpolation.
// -------------------------------------------------------------------------------------
void ntmap_scanline_lighted_linear(grs_bitmap *srcb, int y, fix xleft, fix xright, fix uleft, fix uright, fix vleft, fix vright, fix lleft, fix lright)
{
	fix	u,v,l;
	fix	dx,recip_dx;

	fix	du_dx,dv_dx,dl_dx;

	u = uleft;
	v = vleft;
	l = lleft;

	dx = f2i(xright) - f2i(xleft);
	if ((dx < 0) || (xright < 0) || (xleft > xright))		// the (xleft > xright) term is not redundant with (dx < 0) because dx is computed using integers
		return;

		// setup to call assembler scanline renderer
		if (dx < FIX_RECIP_TABLE_SIZE)
			recip_dx = fix_recip[dx];
		else
			recip_dx = F1_0/dx;

		du_dx = fixmul(uright - uleft,recip_dx);
		dv_dx = fixmul(vright - vleft,recip_dx);

		fx_u = uleft;
		fx_v = vleft;
		fx_du_dx = du_dx;
		fx_dv_dx = dv_dx;
		fx_y = y;
		fx_xright = f2i(xright);
		fx_xleft = f2i(xleft);
		pixptr = srcb->bm_data;

		switch (Lighting_enabled) {
			case 0:
				#ifdef NASM
					c_tmap_scanline_lin_nolight();
				#else
					asm_tmap_scanline_lin();
				#endif
				break;
			case 1:
				if (lleft < F1_0/2)
					lleft = F1_0/2;
				if (lright < F1_0/2)
					lright = F1_0/2;

				if (lleft > MAX_LIGHTING_VALUE*NUM_LIGHTING_LEVELS)
					lleft = MAX_LIGHTING_VALUE*NUM_LIGHTING_LEVELS;
				if (lright > MAX_LIGHTING_VALUE*NUM_LIGHTING_LEVELS)
					lright = MAX_LIGHTING_VALUE*NUM_LIGHTING_LEVELS;

				fx_l = lleft;
				dl_dx = fixmul(lright - lleft,recip_dx);
				fx_dl_dx = dl_dx;
				#ifdef NASM
					c_tmap_scanline_lin();
				#else
					asm_tmap_scanline_lin_lighted();
				#endif
				break;
			case 2:
#ifdef EDITOR_TMAP
				fx_xright = f2i(xright);
				fx_xleft = f2i(xleft);
				asm_tmap_scanline_matt();
#else
				Int3();	//	Illegal, called an editor only routine!
#endif
				break;
		}
}
#endif

// -------------------------------------------------------------------------------------
//	Render a texture map with lighting using perspective interpolation in inner and outer loops.
// -------------------------------------------------------------------------------------
#if defined(MACOS)
#else
void ntexture_map_lighted_linear(grs_bitmap *srcb, g3ds_tmap *t)
{
	int	vlt,vrt,vlb,vrb;	// vertex left top, vertex right top, vertex left bottom, vertex right bottom
	int	topy,boty,y, dy;
	fix	dx_dy_left,dx_dy_right;
	fix	du_dy_left,du_dy_right;
	fix	dv_dy_left,dv_dy_right;
	fix	dl_dy_left,dl_dy_right;
	int	max_y_vertex;
	fix	xleft,xright,uleft,vleft,uright,vright,lleft,lright;
	int	next_break_left, next_break_right;
	fix	recip_dyl, recip_dyr;
#endif

#if defined(MACOS)
void ntmap_outerloop_correct_lighted( g3ds_tmap *t )
{
	#define LINEAR	0						// 0=Perspective, 1=Linear
	#define LIGHTING 1					// 0=No lighting, 1=Use lighting
#ifdef NASM
	#define INNER_LOOP c_tmap_scanline_per	// Function to be called when ready to draw.
#else
	#define INNER_LOOP asm_tmap_scanline_per	// Function to be called when ready to draw.
#endif
	#define CHECK_WINDOW	1				// Set to 1 to check window clip
	#define USE_UVS 1						// Set to 1 to enable U,V interpolation
	#include "ntmapout_c.h"
#else
	g3ds_vertex *v3d;

	v3d = t->verts;

	// Determine top and bottom y coords.
	compute_y_bounds(t,&vlt,&vlb,&vrt,&vrb,&max_y_vertex);

	// Set top and bottom (of entire texture map) y coordinates.
	topy = f2i(v3d[vlt].y2d);
	boty = f2i(v3d[max_y_vertex].y2d);

	if (topy > Window_clip_bot)
		return;
	if (boty > Window_clip_bot)
		boty = Window_clip_bot;

	dy = f2i(t->verts[vlb].y2d) - f2i(t->verts[vlt].y2d);
	if (dy < FIX_RECIP_TABLE_SIZE)
		recip_dyl = fix_recip[dy];
	else
		recip_dyl = F1_0/dy;

	dy = f2i(t->verts[vrb].y2d) - f2i(t->verts[vrt].y2d);
	if (dy < FIX_RECIP_TABLE_SIZE)
		recip_dyr = fix_recip[dy];
	else
		recip_dyr = F1_0/dy;

	// Set amount to change x coordinate for each advance to next scanline.
	dx_dy_left = compute_dx_dy(t,vlt,vlb, recip_dyl);
	dx_dy_right = compute_dx_dy(t,vrt,vrb, recip_dyr);

	du_dy_left = compute_du_dy_lin(t,vlt,vlb, recip_dyl);
	du_dy_right = compute_du_dy_lin(t,vrt,vrb, recip_dyr);

	dv_dy_left = compute_dv_dy_lin(t,vlt,vlb, recip_dyl);
	dv_dy_right = compute_dv_dy_lin(t,vrt,vrb, recip_dyr);

	if (Lighting_enabled) {
		dl_dy_left = compute_dl_dy_lin(t,vlt,vlb, recip_dyl);
		dl_dy_right = compute_dl_dy_lin(t,vrt,vrb, recip_dyr);

		lleft = v3d[vlt].l;
		lright = v3d[vrt].l;
	}

 	// Set initial values for x, u, v
	xleft = v3d[vlt].x2d;
	xright = v3d[vrt].x2d;

	uleft = v3d[vlt].u;
	uright = v3d[vrt].u;
	vleft = v3d[vlt].v;
	vright = v3d[vrt].v;

	// scan all rows in texture map from top through first break.
	next_break_left = f2i(v3d[vlb].y2d);
	next_break_right = f2i(v3d[vrb].y2d);

	for (y = topy; y < boty; y++) {

		// See if we have reached the end of the current left edge, and if so, set
		// new values for dx_dy and x,u,v
		if (y == next_break_left) {
			fix	recip_dy;

			// Handle problem of double points.  Search until y coord is different.  Cannot get
			// hung in an infinite loop because we know there is a vertex with a lower y coordinate
			// because in the for loop, we don't scan all spanlines.
			while (y == f2i(v3d[vlb].y2d)) {
				vlt = vlb;
				vlb = prevmod(vlb,t->nv);
			}
			next_break_left = f2i(v3d[vlb].y2d);

			dy = f2i(t->verts[vlb].y2d) - f2i(t->verts[vlt].y2d);
			if (dy < FIX_RECIP_TABLE_SIZE)
				recip_dy = fix_recip[dy];
			else
				recip_dy = F1_0/dy;

			dx_dy_left = compute_dx_dy(t,vlt,vlb, recip_dy);

			xleft = v3d[vlt].x2d;
			uleft = v3d[vlt].u;
			vleft = v3d[vlt].v;
			lleft = v3d[vlt].l;

			du_dy_left = compute_du_dy_lin(t,vlt,vlb, recip_dy);
			dv_dy_left = compute_dv_dy_lin(t,vlt,vlb, recip_dy);

			if (Lighting_enabled) {
				dl_dy_left = compute_dl_dy_lin(t,vlt,vlb, recip_dy);
				lleft = v3d[vlt].l;
			}
		}

		// See if we have reached the end of the current left edge, and if so, set
		// new values for dx_dy and x.  Not necessary to set new values for u,v.
		if (y == next_break_right) {
			fix	recip_dy;

			while (y == f2i(v3d[vrb].y2d)) {
				vrt = vrb;
				vrb = succmod(vrb,t->nv);
			}

			dy = f2i(t->verts[vrb].y2d) - f2i(t->verts[vrt].y2d);
			if (dy < FIX_RECIP_TABLE_SIZE)
				recip_dy = fix_recip[dy];
			else
				recip_dy = F1_0/dy;

			next_break_right = f2i(v3d[vrb].y2d);
			dx_dy_right = compute_dx_dy(t,vrt,vrb, recip_dy);

			xright = v3d[vrt].x2d;
			uright = v3d[vrt].u;
			vright = v3d[vrt].v;

			du_dy_right = compute_du_dy_lin(t,vrt,vrb, recip_dy);
			dv_dy_right = compute_dv_dy_lin(t,vrt,vrb, recip_dy);

			if (Lighting_enabled) {
				dl_dy_right = compute_dl_dy_lin(t,vrt,vrb, recip_dy);
				lright = v3d[vrt].l;
			}
		}

		if (Lighting_enabled) {
			ntmap_scanline_lighted_linear(srcb,y,xleft,xright,uleft,uright,vleft,vright,lleft,lright);
			lleft += dl_dy_left;
			lright += dl_dy_right;
		} else
			ntmap_scanline_lighted_linear(srcb,y,xleft,xright,uleft,uright,vleft,vright,lleft,lright);

		uleft += du_dy_left;
		vleft += dv_dy_left;

		uright += du_dy_right;
		vright += dv_dy_right;

		xleft += dx_dy_left;
		xright += dx_dy_right;

	}

	// We can get lleft or lright out of bounds here because we compute dl_dy using fixed point values,
	//	but we plot an integer number of scanlines, therefore doing an integer number of additions of the delta.

	ntmap_scanline_lighted_linear(srcb,y,xleft,xright,uleft,uright,vleft,vright,lleft,lright);
#endif
}
#if defined(MACOS)

void ntmap_outerloop_correct_nolight( g3ds_tmap *t )
{
	#define LINEAR	0						// 0=Perspective, 1=Linear
	#define LIGHTING 0					// 0=No lighting, 1=Use lighting
#ifdef NASM
	#define INNER_LOOP c_tmap_scanline_per_nolight	// Function to be called when ready to draw.
#else
	#define INNER_LOOP asm_tmap_scanline_per	// Function to be called when ready to draw.
#endif
	#define CHECK_WINDOW	1				// Set to 1 to check window clip
	#define USE_UVS 1						// Set to 1 to enable U,V interpolation
	#include "ntmapout_c.h"
}


extern void asm_tmap_scanline_llt();

void ntmap_outerloop_lin_lighted_trans( g3ds_tmap *t )
{
	#define LINEAR	1						// 0=Perspective, 1=Linear
	#define LIGHTING 1					// 0=No lighting, 1=Use lighting
#ifdef NASM
	#define INNER_LOOP c_tmap_scanline_lin	// Function to be called when ready to draw.
#else
	#define INNER_LOOP asm_tmap_scanline_llt	// Function to be called when ready to draw.
#endif
	#define CHECK_WINDOW	1				// Set to 1 to check window clip
	#define USE_UVS 1						// Set to 1 to enable U,V interpolation
	#include "ntmapout_c.h"
}

extern void asm_tmap_scanline_lln();

void ntmap_outerloop_lin_lighted_notrans( g3ds_tmap *t )
{
	#define LINEAR	1						// 0=Perspective, 1=Linear
	#define LIGHTING 1					// 0=No lighting, 1=Use lighting
#ifdef NASM
	#define INNER_LOOP c_tmap_scanline_lin	// Function to be called when ready to draw.
#else
	#define INNER_LOOP asm_tmap_scanline_lln	// Function to be called when ready to draw.
#endif
	#define CHECK_WINDOW	1				// Set to 1 to check window clip
	#define USE_UVS 1						// Set to 1 to enable U,V interpolation
	#include "ntmapout_c.h"
}


extern void asm_tmap_scanline_plt();

void ntmap_outerloop_per_lighted_trans( g3ds_tmap *t )
{
	#define LINEAR	0						// 0=Perspective, 1=Linear
	#define LIGHTING 1					// 0=No lighting, 1=Use lighting
#ifdef NASM
	#define INNER_LOOP c_tmap_scanline_per	// Function to be called when ready to draw.
#else
	#define INNER_LOOP asm_tmap_scanline_plt	// Function to be called when ready to draw.
#endif
	#define CHECK_WINDOW	1				// Set to 1 to check window clip
	#define USE_UVS 1						// Set to 1 to enable U,V interpolation
	#include "ntmapout_c.h"
}

extern void asm_tmap_scanline_pln();

void ntmap_outerloop_per_lighted_notrans( g3ds_tmap *t )
{
	#define LINEAR	0						// 0=Perspective, 1=Linear
	#define LIGHTING 1					// 0=No lighting, 1=Use lighting
#ifdef NASM
	#define INNER_LOOP c_tmap_scanline_per	// Function to be called when ready to draw.
#else
	#define INNER_LOOP asm_tmap_scanline_pln	// Function to be called when ready to draw.
#endif
	#define CHECK_WINDOW	1				// Set to 1 to check window clip
	#define USE_UVS 1						// Set to 1 to enable U,V interpolation
	#include "ntmapout_c.h"
}

extern void asm_tmap_scanline_lnt();

void ntmap_outerloop_lin_nolight_trans( g3ds_tmap *t )
{
	#define LINEAR	1						// 0=Perspective, 1=Linear
	#define LIGHTING 0					// 0=No lighting, 1=Use lighting
#ifdef NASM
	#define INNER_LOOP c_tmap_scanline_lin_nolight	// Function to be called when ready to draw.
#else
	#define INNER_LOOP asm_tmap_scanline_lnt	// Function to be called when ready to draw.
#endif
	#define CHECK_WINDOW	1				// Set to 1 to check window clip
	#define USE_UVS 1						// Set to 1 to enable U,V interpolation
	#include "ntmapout_c.h"
}

extern void asm_tmap_scanline_lnn();

void ntmap_outerloop_lin_nolight_notrans( g3ds_tmap *t )
{
	#define LINEAR	1						// 0=Perspective, 1=Linear
	#define LIGHTING 0					// 0=No lighting, 1=Use lighting
#ifdef NASM
	#define INNER_LOOP c_tmap_scanline_lin_nolight	// Function to be called when ready to draw.
#else
	#define INNER_LOOP asm_tmap_scanline_lnn	// Function to be called when ready to draw.
#endif
	#define CHECK_WINDOW	1				// Set to 1 to check window clip
	#define USE_UVS 1						// Set to 1 to enable U,V interpolation
	#include "ntmapout_c.h"
}


extern void asm_tmap_scanline_pnt();

void ntmap_outerloop_per_nolight_trans( g3ds_tmap *t )
{
	#define LINEAR	0						// 0=Perspective, 1=Linear
	#define LIGHTING 0					// 0=No lighting, 1=Use lighting
#ifdef NASM
	#define INNER_LOOP c_tmap_scanline_per_nolight	// Function to be called when ready to draw.
#else
	#define INNER_LOOP asm_tmap_scanline_pnt	// Function to be called when ready to draw.
#endif
	#define CHECK_WINDOW	1				// Set to 1 to check window clip
	#define USE_UVS 1						// Set to 1 to enable U,V interpolation
	#include "ntmapout_c.h"
}


extern void asm_tmap_scanline_pnn();

void ntmap_outerloop_per_nolight_notrans( g3ds_tmap *t )
{
	#define LINEAR	0						// 0=Perspective, 1=Linear
	#define LIGHTING 0					// 0=No lighting, 1=Use lighting
#ifdef NASM
	#define INNER_LOOP c_tmap_scanline_per_nolight	// Function to be called when ready to draw.
#else
	#define INNER_LOOP asm_tmap_scanline_pnn	// Function to be called when ready to draw.
#endif
	#define CHECK_WINDOW	1				// Set to 1 to check window clip
	#define USE_UVS 1						// Set to 1 to enable U,V interpolation
	#include "ntmapout_c.h"
}


extern void asm_tmap_scanline_editor();
extern void c_tmap_scanline_editor();

void ntmap_outerloop_editor( g3ds_tmap *t )
{
	#define LINEAR	0						// 0=Perspective, 1=Linear
	#define LIGHTING 0					// 0=No lighting, 1=Use lighting
#ifdef NASM
	#define INNER_LOOP c_tmap_scanline_editor	// Function to be called when ready to draw.
#else
	#define INNER_LOOP asm_tmap_scanline_editor	// Function to be called when ready to draw.
#endif
	#define CHECK_WINDOW	0				// Set to 1 to check window clip
	#define USE_UVS 1						// Set to 1 to enable U,V interpolation
	#include "ntmapout_c.h"
}

// -------------------------------------------------------------------------------------
//	Render a texture map.
// Linear in outer loop, linear in inner loop.
// -------------------------------------------------------------------------------------
void texture_map_flat(g3ds_tmap *t)
{
	#define LINEAR	1						// 0=Perspective, 1=Linear
	#define LIGHTING 0					// 0=No lighting, 1=Use lighting
#ifdef NASM
	#define INNER_LOOP c_tmap_scanline_flat	// Function to be called when ready to draw.
#else
	#define INNER_LOOP asm_tmap_scanline_flat	// Function to be called when ready to draw.
#endif
	#define CHECK_WINDOW	1				// Set to 1 to check window clip
	#define USE_UVS 0						// Set to 1 to enable U,V interpolation
	#include "ntmapout_c.h"
}

extern void asm_tmap_scanline_shaded();	// In tmapfade.asm

void texture_map_flat_faded(g3ds_tmap *t)
{
	#define LINEAR	1						// 0=Perspective, 1=Linear
	#define LIGHTING 0					// 0=No lighting, 1=Use lighting
#ifdef NASM
	#define INNER_LOOP c_tmap_scanline_shaded	// Function to be called when ready to draw.
#else
	#define INNER_LOOP asm_tmap_scanline_shaded	// Function to be called when ready to draw.
#endif
	#define CHECK_WINDOW	1				// Set to 1 to check window clip
	#define USE_UVS 0						// Set to 1 to enable U,V interpolation
	#include "ntmapout_c.h"
}

#else
#endif

// fix	DivNum = F1_0*12;

extern void draw_tmap_flat(grs_bitmap *bp,int nverts,g3s_point **vertbuf);

// -------------------------------------------------------------------------------------
// Interface from Matt's data structures to Mike's texture mapper.
// -------------------------------------------------------------------------------------
void draw_tmap(grs_bitmap *bp,int nverts,g3s_point **vertbuf)
{
	int	i;
#if defined(MACOS)
	int	lighting_mode = Lighting_on;
	int	render_method;
	fix 	min_z;
   #ifdef _3DFX
   int   bm_index = bp->bm_handle;
   #endif
#else

	//	These variables are used in system which renders texture maps which lie on one scanline as a line.
#if SC2000K
	int	flat_flag;							// Set to 1 and remains 1 so long as all y coords are the same (in integer portion).
	int	last_y;								// Y coordinate of previous vertex.
	fix	min_x = 0xfff0000, max_x = 0;	// Minimum and maximum bounds of line to render in place of flat texture map.
#endif
	// fix	div_numerator;
	int	lighting_on_save = Lighting_on;
#endif

	Assert(nverts <= MAX_TMAP_VERTS);
#if defined(MACOS)
	Assert(bp->bm_w == 64);
	Assert(Lighting_on < 3);
#else


#ifdef USE_MULT_CODE
	if ( !divide_table_filled ) fill_divide_table();
#endif

	// -- now called from g3_start_frame -- init_interface_vars_to_assembler();
#endif

	//	If no transparency and seg depth is large, render as flat shaded.
	if ((Current_seg_depth > Max_linear_depth) && ((bp->bm_flags & 3) == 0)) {
		draw_tmap_flat(bp, nverts, vertbuf);
		return;
	}

	if ( bp->bm_flags & BM_FLAG_RLE )
#if defined(MACOS)
   {
#else
#endif
		bp = rle_expand_texture( bp );		// Expand if rle'd
#if defined(MACOS)
      #ifdef _3DFX
      bp->bm_handle = bm_index;
      #endif
   }
#else
#endif

	Transparency_on = bp->bm_flags & BM_FLAG_TRANSPARENT;
	if (bp->bm_flags & BM_FLAG_NO_LIGHTING)
#if defined(MACOS)
		lighting_mode = 0;
#else
		Lighting_on = 0;
#endif

#if defined(MACOS)
	pixptr = bp->bm_data;

	render_method = Interpolation_method;
	if (render_method==0) {
		if (Current_seg_depth > Max_perspective_depth)
			render_method = 1;		// use linear
		else
			render_method = 2;		// use perspective
#else
	//	Set selector for current texture map.
	if ( bp->bm_selector == 0 )	{
		if (gr_bitmap_assign_selector( bp ) )
			Error( "Couldn't assign selector in ntmap.c!\n" );
#endif
	}
#if defined(MACOS)
#else
	pixel_data_selector = bp->bm_selector;
#endif

	// Setup texture map in Tmap1
	Tmap1.nv = nverts;						// Initialize number of vertices
#if defined(MACOS)
#else

#if SC2000K
	last_y = f2i(vertbuf[0]->p3_sy);
	flat_flag = 1;								// Says so far, this texture map exists all on one scan line
#endif
// 	div_numerator = DivNum;	//f1_0*3;
#endif

	for (i=0; i<nverts; i++) {
		g3ds_vertex	*tvp = &Tmap1.verts[i];
		g3s_point	*vp = vertbuf[i];

		tvp->x2d = vp->p3_sx;
		tvp->y2d = vp->p3_sy;
#if defined(MACOS)
#else

#if SC2000K
		//	If y coordinates are not the same (in integer portion), then this texture map is not all on one scan line
		if (f2i(tvp->y2d) != last_y)
			flat_flag = 0;

		// If everything is flat so far, set minimum x and y for rendering a scanline
		if (flat_flag) {
			if (tvp->x2d < min_x)
				min_x = tvp->x2d;

			if (tvp->x2d > max_x)
				max_x = tvp->x2d;
		}
#endif
		//	Check for overflow on fixdiv.  Will overflow on vp->z <= something small.  Allow only as low as 256.
		if (vp->z < 256) {
			vp->z = 256;
			// Int3();		// we would overflow if we divided!
		}

		tvp->z = fixdiv(F1_0*12, vp->z);
#endif
 		tvp->u = vp->p3_u << 6; //* bp->bm_w;
		tvp->v = vp->p3_v << 6; //* bp->bm_h;

#if defined(MACOS)
		//	Perspective stuff:
		if ( render_method > 1 )
      {
			//Check for overflow on fixdiv.  Will overflow on vp->z <= something small.  Allow only as low as 256.
			if (vp->p3_z < 256) {
				vp->p3_z = 256;
				// Int3();		// we would overflow if we divided!
			}
#else
		Assert(Lighting_on < 3);
#endif

#if defined(MACOS)
			tvp->z = vp->p3_z;	//fixdiv(F1_0*12, vp->p3_z);
			if ( (i==0) || (tvp->z < min_z) )
				min_z = tvp->z;
		}

      #ifdef _3DFX
      tvp->z = vp->p3_z;
      #endif

		#if defined(POLY_ACC)
		 #ifdef PA_3DFX_VOODOO
	      tvp->z = vp->p3_z;
		 #endif
		#endif

		// Lighting stuff:
		if (lighting_mode)
#else
		if (Lighting_on)
#endif
			tvp->l = vp->p3_l * NUM_LIGHTING_LEVELS;
	}

#if defined(MACOS)
#if defined(POLY_ACC)
 #ifdef PA_3DFX_VOODOO
    switch (render_method)
    {
        case 1:                             // linear interpolation
            pa_draw_tmap(bp, &Tmap1, Transparency_on, lighting_mode, 0, min_z);  // linear
            break;
        case 2:
        case 3:                             // perspective every pixel interpolation
            pa_draw_tmap(bp, &Tmap1, Transparency_on, lighting_mode, 1, min_z);  // persp.
            break;
        default:
            Assert(0);              // Illegal value for Interpolation_method, must be 0,1,2,3
			
    }
	return;
 #endif
#endif
#else
#if SC2000K
	// Render a horizontal line instead of a texture map if flat_flag still set (all vertices on one scanline).
	if (SC2000 && flat_flag) {
		fix y,cvw;

		Int3();
		y = Tmap1.verts[0].y2d;
		cvw = i2f(grd_curcanv->cv_bitmap.bm_w-1);

		if (y<0 || y>i2f(grd_curcanv->cv_bitmap.bm_h-1)) { Lighting_on = lighting_on_save; return; }

		//if (min_x < 0) min_x = 0;
		//if (max_x > cvw) max_x = cvw;

		gr_setcolor(*(bp->bm_data+32*32+32));				// Read center pixel of 64x64 bitmap, use as line color.
		gr_line(min_x, Tmap1.verts[0].y2d, max_x, Tmap1.verts[0].y2d);

		Lighting_on = lighting_on_save;
		return;
	}
#endif
#endif

#if defined(MACOS)
#ifdef _3DFX

   if ( _3dfx_available )
   {
      if ( _3dfx_rendering_poly_obj )
      {
         _3dfx_DownloadAndUseTexture( bp->bm_handle );
      }
      if ( Lighting_on )
      {
         guColorCombineFunction( GR_COLORCOMBINE_TEXTURE_TIMES_ITRGB );

         _3dfxDrawTmap( bp, &Tmap1 );
      }
      else
      {
         guColorCombineFunction( GR_COLORCOMBINE_DECAL_TEXTURE );

         _3dfxDrawTmap( bp, &Tmap1 );
      }
   }
   _3dfx_triangles_rendered_post_clip += Tmap1.nv - 2;

   if ( _3dfx_skip_ddraw )
      goto done;
#endif

	// Calculate 1/Z for perspective
	if ( render_method > 1 )	{
		g3ds_vertex	*tvp = Tmap1.verts;
		for (i=0; i<nverts; i++, tvp++) {
			tvp->z = fixdiv( min_z, tvp->z );
			// Z is now in the range 0-F1_0 (17 bits)
			// So, in the next line, we need to 
			// scale Z down so that it fits in the available
			// number of significant bits (12 bits now).  But, at the
			// same time, we're scaling it up by Z_SHIFTER amount so 
			// that it use 30 bits for Z interpolation, which keeps the	
			// dz_dx values from becoming so small that they go to zero.
			tvp->z <<= Z_SHIFTER-(17-DIVIDE_SIG_BITS);					
#else
	Lighting_enabled = Lighting_on;

	// Now, call my texture mapper.
	if (Lighting_on) {
		switch (Interpolation_method) {	// 0 = choose, 1 = linear, 2 = /8 perspective, 3 = full perspective
			case 0:								// choose best interpolation
				per2_flag = 1;
				if (Current_seg_depth > Max_perspective_depth)
					ntexture_map_lighted_linear(bp, &Tmap1);
				else
					ntexture_map_lighted(bp, &Tmap1);
				break;
			case 1:								// linear interpolation
				per2_flag = 1;
				ntexture_map_lighted_linear(bp, &Tmap1);
				break;
			case 2:								// perspective every 8th pixel interpolation
				per2_flag = 1;
				ntexture_map_lighted(bp, &Tmap1);
				break;
			case 3:								// perspective every pixel interpolation
				per2_flag = 0;					// this hack means do divide every pixel
				ntexture_map_lighted(bp, &Tmap1);
				break;
			default:
				Assert(0);				// Illegal value for Interpolation_method, must be 0,1,2,3
		}
	} else {
		switch (Interpolation_method) {	// 0 = choose, 1 = linear, 2 = /8 perspective, 3 = full perspective
			case 0:								// choose best interpolation
				per2_flag = 1;
				if (Current_seg_depth > Max_perspective_depth)
					ntexture_map_lighted_linear(bp, &Tmap1);
				else
					ntexture_map_lighted(bp, &Tmap1);
				break;
			case 1:								// linear interpolation
				per2_flag = 1;
				ntexture_map_lighted_linear(bp, &Tmap1);
				break;
			case 2:								// perspective every 8th pixel interpolation
				per2_flag = 1;
				ntexture_map_lighted(bp, &Tmap1);
				break;
			case 3:								// perspective every pixel interpolation
				per2_flag = 0;					// this hack means do divide every pixel
				ntexture_map_lighted(bp, &Tmap1);
				break;
			default:
				Assert(0);				// Illegal value for Interpolation_method, must be 0,1,2,3
#endif
		}
	}

#if defined(MACOS)
	// render_method:
	//   1 = linear, 2 = subdivided perspective, 3 = full perspective
	// lighting_mode:
	//	  0 = no lighting, 1=lighting, 2=editor

#if defined(POLY_ACC)
    switch (render_method)
    {
/*  obsolete 3/5/96.
        case 0:                             // choose best interpolation
            if (Current_seg_depth > Max_perspective_depth)
                pa_draw_tmap(bp, &Tmap1, Transparency_on, lighting_mode, 0);  // linear
            else
                pa_draw_tmap(bp, &Tmap1, Transparency_on, lighting_mode, 1);  // persp.
            break;
*/
        case 1:                             // linear interpolation
            pa_draw_tmap(bp, &Tmap1, Transparency_on, lighting_mode, 0, min_z);  // linear
            break;
        case 2:
        case 3:                             // perspective every pixel interpolation
            pa_draw_tmap(bp, &Tmap1, Transparency_on, lighting_mode, 1, min_z);  // persp.
            break;
        default:
            Assert(0);              // Illegal value for Interpolation_method, must be 0,1,2,3
    }

#else
	switch( lighting_mode )	{
	case 0:
		switch (render_method) {	
		case 1:								// linear interpolation
			if (Transparency_on)
				ntmap_outerloop_lin_nolight_trans(&Tmap1);
			else
				ntmap_outerloop_lin_nolight_notrans(&Tmap1);
			break;
		case 2:								// subdivided perspective
			if (Transparency_on)
				ntmap_outerloop_per_nolight_trans( &Tmap1 );
			else
				ntmap_outerloop_per_nolight_notrans( &Tmap1 );
			break;
		case 3:								// perspective every pixel interpolation
			ntmap_outerloop_correct_nolight( &Tmap1 );
			break;
		default:
			Assert(0);				// Illegal value for Interpolation_method, must be 0,1,2,3
		}
		break;
	case 1:
		switch (render_method) {	
		case 1:								// linear interpolation
			if (Transparency_on)
				ntmap_outerloop_lin_lighted_trans(&Tmap1);
			else
				ntmap_outerloop_lin_lighted_notrans(&Tmap1);
			break;
		case 2:								// subdivided perspective
			if (Transparency_on)
				ntmap_outerloop_per_lighted_trans( &Tmap1 );
			else
				ntmap_outerloop_per_lighted_notrans( &Tmap1 );
			break;
		case 3:								// perspective every pixel interpolation
			ntmap_outerloop_correct_lighted( &Tmap1 );
			break;
		default:
			Assert(0);				// Illegal value for Interpolation_method, must be 0,1,2,3
		}
		break;
#ifdef EDITOR_TMAP
	case 2:
		ntmap_outerloop_editor( &Tmap1 );
		break;
#endif
	}
#endif  //POLY_ACC

#ifdef _3DFX
done:
   return;
#endif
#else
	Lighting_on = lighting_on_save;
#endif

}
#if defined(MACOS)

#ifdef _3DFX

#define SNAP( a ) ( ( fix ) ( (a) & ( 0xFFFFF000L ) ) )

void _3dfxDrawTmap( grs_bitmap *bp, g3ds_tmap *Tmap1 )
{
   GrVertex a, b, c;
   int      i;

   if ( _3dfx_no_texture )
      guColorCombineFunction( GR_COLORCOMBINE_ITRGB );

   a.r = a.g = a.b = Tmap1->verts[0].l * fix_to_rgb;

   a.tmuvtx[0].oow = a.oow = 1.0 / ( Tmap1->verts[0].z * fix_to_float );
   a.tmuvtx[0].sow = Tmap1->verts[0].u * fix_to_st * a.oow;
   a.tmuvtx[0].tow = Tmap1->verts[0].v * fix_to_st * a.oow;

   a.x = SNAP( Tmap1->verts[0].x2d ) * fix_to_float;
   a.y = SNAP( Tmap1->verts[0].y2d ) * fix_to_float;

   for ( i = 1; i < Tmap1->nv - 1; i++ )
   {
      b.r = b.g = b.b = Tmap1->verts[i].l * fix_to_rgb;
      b.tmuvtx[0].oow = b.oow = 1.0 / ( Tmap1->verts[i].z * fix_to_float );
      b.tmuvtx[0].sow = Tmap1->verts[i].u * fix_to_st * b.oow;
      b.tmuvtx[0].tow = Tmap1->verts[i].v * fix_to_st * b.oow;
      b.x   = SNAP( Tmap1->verts[i].x2d ) * fix_to_float;
      b.y   = SNAP( Tmap1->verts[i].y2d ) * fix_to_float;

      c.r = c.g = c.b = Tmap1->verts[i+1].l * fix_to_rgb;
      c.tmuvtx[0].oow = c.oow = 1.0 / ( Tmap1->verts[i+1].z * fix_to_float );
      c.tmuvtx[0].sow = Tmap1->verts[i+1].u * fix_to_st * c.oow;
      c.tmuvtx[0].tow = Tmap1->verts[i+1].v * fix_to_st * c.oow;
      c.x   = SNAP( Tmap1->verts[i+1].x2d ) * fix_to_float;
      c.y   = SNAP( Tmap1->verts[i+1].y2d ) * fix_to_float;

      grDrawTriangle( &a, &b, &c );
   }
}

#endif
#else
#endif

