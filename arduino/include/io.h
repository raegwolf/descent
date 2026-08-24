#ifndef DESCENT_ARDUINO_IO_H
#define DESCENT_ARDUINO_IO_H
#include <unistd.h>
int filelength(int fd);
void _makepath(char *path, const char *drive, const char *dir,
               const char *name, const char *ext);
void _splitpath(const char *path, char *drive, char *dir,
                char *name, char *ext);
#endif
