#ifndef DESCENT_ARDUINO_PSSTRING_H
#define DESCENT_ARDUINO_PSSTRING_H
#include <stdio.h>
#include <string.h>
#include <strings.h>
#define stricmp(a,b) strcasecmp((a),(b))
#define strnicmp(a,b,n) strncasecmp((a),(b),(n))
char *descent_itoa(int value, char *buffer, int radix);
char *descent_strrev(char *value);
#define itoa descent_itoa
#define strrev descent_strrev
#endif
