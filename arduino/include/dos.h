#ifndef DESCENT_ARDUINO_DOS_H
#define DESCENT_ARDUINO_DOS_H

#define _A_NORMAL 0
#define _A_SUBDIR 0x10
#define EZERO 0

struct find_t { char name[256]; };
int _dos_findfirst(const char *pattern, int attributes, struct find_t *result);
int _dos_findnext(struct find_t *result);

#endif
