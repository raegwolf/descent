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
 * $Source: f:/miner/source/fix/rcs/fix.h $
 * $Revision: 1.13 $
 * $Author: matt $
 * $Date: 1994/12/06 13:52:34 $
 *
 * FIX.H - prototypes and macros for fixed-point functions
 *
 * Copyright (c) 1993  Matt Toschlog & Mike Kulas
 *
 * $Log: fix.h $
 * Revision 1.13  1994/12/06  13:52:34  matt
 * Added f2ir(), which is fix-to-int with rounding
 * 
 * Revision 1.12  1994/05/18  21:45:16  matt
 * Added comments
 * 
 * Revision 1.11  1994/01/19  23:12:02  matt
 * Made fix_atan2() left-handed, like our coordinate system
 * 
 * Revision 1.10  1993/10/20  01:09:00  matt
 * Add fix_asin(), improved fix_atan2()
 * 
 * Revision 1.9  1993/10/19  23:53:46  matt
 * Added fix_atan2()
 * 
 * Revision 1.8  1993/10/19  22:22:40  matt
 * Added fix_acos()
 * 
 * Revision 1.7  1993/09/17  11:37:01  mike
 * Add capitalized versions of "some handy constants", eg:
 * #define F1_0 f1_0
 * 
 * Revision 1.6  1993/08/24  13:00:48  matt
 * Adopted new standard, and made assembly-callable routines not trash any regs
 * 
 * Revision 1.5  1993/08/12  13:12:45  matt
 * Changed fixmul() to use SHRD instead of shl,shr,or
 * 
 * Revision 1.4  1993/08/04  19:57:18  matt
 * Added parens in fix/float conversion macros
 * 
 * Revision 1.3  1993/08/04  11:41:45  matt
 * Fixed bogus constants
 * 
 * Revision 1.2  1993/08/04  09:30:11  matt
 * Added more constants
 * 
 * Revision 1.1  1993/08/03  17:45:53  matt
 * Initial revision
 * 
 *
 */

#ifndef _FIX_H
#define _FIX_H

#include "types.h"

#if defined(MACOS)
#include <stdint.h>
#endif

#if defined(MACOS)
typedef int32_t fix;            //16 bits int, 16 bits frac
typedef int16_t fixang;         //angles
#else
typedef long fix;				//16 bits int, 16 bits frac
typedef short fixang;		//angles
#endif

//Convert an int to a fix
#define i2f(i) ((i)<<16)

//Get the int part of a fix
#define f2i(f) ((f)>>16)

//Get the int part of a fix, with rounding
#define f2ir(f) (((f)+f0_5)>>16)

//Convert fix to float and float to fix
#define f2fl(f) (((float) (f)) / 65536.0)
#define fl2f(f) ((fix) ((f) * 65536))

//Some handy constants
#define f0_0	0
#define f1_0	0x10000
#define f2_0	0x20000
#define f3_0	0x30000
#define f10_0	0xa0000

#define f0_5 0x8000
#define f0_1 0x199a

#define F0_0	f0_0
#define F1_0	f1_0
#define F2_0	f2_0
#define F3_0	f3_0
#define F10_0	f10_0

#define F0_5 	f0_5
#define F0_1 	f0_1

#if defined(MACOS)
static __inline__ fix fixmul(fix a,fix b)
{
	return (fix)(((int64_t)a * (int64_t)b) >> 16);
}
#else
fix fixmul(fix a,fix b);
#endif
#ifdef __WATCOMC__
#pragma aux fixmul parm [eax] [edx] = \
	"imul	edx"				\
	"shrd	eax,edx,16";
#endif


#if defined(MACOS)
static __inline__ fix fixdiv(fix a,fix b)
{
	return (fix)(((int64_t)a * 65536) / b);
}
#else
fix fixdiv(fix a,fix b);
#endif
#ifdef __WATCOMC__
#pragma aux fixdiv parm [eax] [ebx] modify exact [eax edx] = \
	"mov	edx,eax"	\
	"sar	edx,16"	\
	"shl	eax,16"	\
	"idiv	ebx";
#endif

#if defined(MACOS)
static __inline__ fix fixmuldiv(fix a,fix b,fix c)
{
	return (fix)(((int64_t)a * (int64_t)b) / c);
}
#else
fix fixmuldiv(fix a,fix b,fix c);
#endif
#ifdef __WATCOMC__
#pragma aux fixmuldiv parm [eax] [edx] [ebx] modify exact [eax edx] = \
	"imul	edx"	\
	"idiv	ebx";
#endif

//computes the square root of a long, returning a short
#if defined(MACOS)
ushort long_sqrt(int32_t a);
#else
ushort long_sqrt(long a);
#endif

//computes the square root of a quad, returning a long
#if defined(MACOS)
int32_t quad_sqrt(uint32_t low,int32_t high);
#else
ulong quad_sqrt(long low,long high);
#endif

//computes the square root of a fix, returning a fix
fix fix_sqrt(fix a);

//compute sine and cosine of an angle, filling in the variables
//either of the pointers can be NULL
void fix_sincos(fix a,fix *s,fix *c);		//with interpolation
void fix_fastsincos(fix a,fix *s,fix *c);	//no interpolation

//compute inverse sine & cosine
fixang fix_asin(fix v); 
fixang fix_acos(fix v); 

//given cos & sin of an angle, return that angle.
//parms need not be normalized, that is, the ratio of the parms cos/sin must
//equal the ratio of the actual cos & sin for the result angle, but the parms 
//need not be the actual cos & sin.  
//NOTE: this is different from the standard C atan2, since it is left-handed.
fixang fix_atan2(fix cos,fix sin); 

#if defined(MACOS)
/* Portable form of the EDX:EAX accumulator used by FIX.ASM. */
typedef union quad {
	struct {
		uint32_t low;
		int32_t high;
	};
	int64_t large;
} quad;

static __inline__ void fixquadinit(quad *q) { q->large = 0; }
static __inline__ void fixquadnegate(quad *q) { q->large = -q->large; }
static __inline__ void fixmulaccum(quad *q,fix a,fix b)
{
	q->large += (int64_t)a * (int64_t)b;
}
static __inline__ fix fixquadadjust(quad *q) { return (fix)(q->large >> 16); }
long fixdivquadlong(ulong qlow,ulong qhigh,ulong d);
#endif

#ifdef __WATCOMC__
#pragma aux fix_fastsincos parm [eax] [esi] [edi] modify exact [eax ebx];
#pragma aux fix_sincos parm [eax] [esi] [edi] modify exact [eax ebx];

#pragma aux fix_acos "*" parm [eax] value [ax] modify exact [eax];
#pragma aux fix_atan2 "*" parm [eax] [ebx] value [ax] modify exact [eax ebx];

#pragma aux long_sqrt "*" parm [eax] value [ax] modify [];
#pragma aux fix_sqrt "*" parm [eax] value [eax] modify [];
#pragma aux quad_sqrt "*" parm [eax] [edx] value [eax] modify [];
#endif

#endif
