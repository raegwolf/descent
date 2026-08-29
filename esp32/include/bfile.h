#ifndef DESCENT_ESP32_BFILE_H
#define DESCENT_ESP32_BFILE_H
#include <stdio.h>
typedef FILE BFILE;
#define bfopen fopen
#define bfclose fclose
#define bfread fread
#define bfwrite fwrite
#define bfseek fseek
#define bfeof feof
#define bfputs fputs
#define bfgets fgets
#define bftell ftell
static int bfilelength(BFILE *file)
{
	long here = ftell(file), length;
	fseek(file, 0, SEEK_END);
	length = ftell(file);
	fseek(file, here, SEEK_SET);
	return (int)length;
}
#endif
