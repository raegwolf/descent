#ifndef DESCENT_LINEAR_C_H
#define DESCENT_LINEAR_C_H

/* Portable declarations for the public state and routine exported by
   LINEAR_C.C. The historical GRDEF.H remains untouched for the DOS build. */
extern unsigned int gr_var_color;
extern unsigned int gr_var_bwidth;
extern unsigned char *gr_var_bitmap;

void gr_linear_line(int x0, int y0, int x1, int y1);

#endif

