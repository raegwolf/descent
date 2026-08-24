/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX"). See ../README.TXT for the complete
non-commercial source license.
*/

/* Portable reference replacement for 2D/LINEAR.ASM.

   The original gr_linear_line_ receives x0/y0/x1/y1 in EAX/EDX/EBX/ECX and
   writes through these public globals:

       _gr_var_color   dd ?
       _gr_var_bitmap  dd ?
       _gr_var_bwidth  dd ?

   Its LineIsTopToBottom block swaps both endpoints when y0 > y1 so reversing
   endpoints draws the same pixels. It special-cases vertical, horizontal, and
   diagonal lines, then uses error-term stepping in XMajor or YMajor.

   This C implementation keeps that endpoint normalization and implements the
   same inclusive integer Bresenham contract without VGA or x86 instructions.
   The caller remains responsible for clipping, exactly as in LINEAR.ASM.
*/

#include "LINEAR_C.H"

#include <stddef.h>

unsigned int gr_var_color;
unsigned int gr_var_bwidth;
unsigned char *gr_var_bitmap;

static int absolute_int(int value)
{
    return value < 0 ? -value : value;
}

void gr_linear_line(int x0, int y0, int x1, int y1)
{
    int dx;
    int dy;
    int x_step;
    int error;

    if (gr_var_bitmap == NULL || gr_var_bwidth == 0)
        return;

    if (y0 > y1) {
        int temporary = x0;
        x0 = x1;
        x1 = temporary;
        temporary = y0;
        y0 = y1;
        y1 = temporary;
    }

    dx = absolute_int(x1 - x0);
    dy = y1 - y0;
    x_step = x0 <= x1 ? 1 : -1;
    error = dx - dy;

    for (;;) {
        int twice_error;

        gr_var_bitmap[(size_t)y0 * gr_var_bwidth + (size_t)x0] =
            (unsigned char)gr_var_color;
        if (x0 == x1 && y0 == y1)
            break;

        twice_error = 2 * error;
        if (twice_error > -dy) {
            error -= dy;
            x0 += x_step;
        }
        if (twice_error < dx) {
            error += dx;
            ++y0;
        }
    }
}
