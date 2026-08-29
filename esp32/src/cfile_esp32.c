/* Read-only CFILE adapter for flash-linked DESCENT.HOG and DESCENT.PIG. */
#if !defined(DESCENT_MENU_ONLY) || !DESCENT_MENU_ONLY
#define DESCENT_ENGINE_BUILD 1
#include "esp32_compat.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "cfile.h"

extern const unsigned char _binary_descent_hog_start[];
extern const unsigned char _binary_descent_hog_end[];
extern const unsigned char _binary_descent_pig_start[];
extern const unsigned char _binary_descent_pig_end[];

static const char *base_name(const char *name)
{
	const char *slash = strrchr(name, '/');
	const char *backslash = strrchr(name, '\\');
	if (backslash != NULL && (slash == NULL || backslash > slash))
		slash = backslash;
	return slash == NULL ? name : slash + 1;
}

static unsigned int read_le32(const unsigned char *bytes)
{
	return (unsigned int)bytes[0] |
		((unsigned int)bytes[1] << 8) |
		((unsigned int)bytes[2] << 16) |
		((unsigned int)bytes[3] << 24);
}

static int find_hog_member(const char *filename,
	const unsigned char **data, unsigned int *size)
{
	const unsigned char *cursor = _binary_descent_hog_start;
	const unsigned char *end = _binary_descent_hog_end;
	const char *wanted = base_name(filename);

	if ((size_t)(end - cursor) < 3 || memcmp(cursor, "DHF", 3) != 0)
		return 0;
	cursor += 3;
	while ((size_t)(end - cursor) >= 17) {
		char member_name[14];
		unsigned int member_size;
		memcpy(member_name, cursor, 13);
		member_name[13] = '\0';
		member_size = read_le32(cursor + 13);
		cursor += 17;
		if (member_size > (unsigned int)(end - cursor))
			return 0;
		if (strcasecmp(member_name, wanted) == 0) {
			*data = cursor;
			*size = member_size;
			return 1;
		}
		cursor += member_size;
	}
	return 0;
}

static int find_embedded_file(const char *filename,
	const unsigned char **data, unsigned int *size)
{
	const char *name = base_name(filename);
	if (strcasecmp(name, "DESCENT.HOG") == 0) {
		*data = _binary_descent_hog_start;
		*size = (unsigned int)(_binary_descent_hog_end -
			_binary_descent_hog_start);
		return 1;
	}
	if (strcasecmp(name, "DESCENT.PIG") == 0) {
		*data = _binary_descent_pig_start;
		*size = (unsigned int)(_binary_descent_pig_end -
			_binary_descent_pig_start);
		return 1;
	}
	return find_hog_member(name, data, size);
}

static const unsigned char *cfile_data(const CFILE *fp)
{
	return (const unsigned char *)(uintptr_t)fp->file;
}

void cfile_use_alternate_hogdir(char *path) { (void)path; }
void cfile_use_alternate_hogfile(char *name) { (void)name; }

int cfexist(char *filename)
{
	const unsigned char *data;
	unsigned int size;
	if (!find_embedded_file(filename, &data, &size))
		return 0;
	return strcasecmp(base_name(filename), "DESCENT.PIG") == 0 ||
		strcasecmp(base_name(filename), "DESCENT.HOG") == 0 ? 1 : 2;
}

CFILE *cfopen(char *filename, char *mode)
{
	const unsigned char *data;
	unsigned int size;
	CFILE *fp;
	if (strcasecmp(mode, "rb") != 0 ||
		!find_embedded_file(filename, &data, &size))
		return NULL;
	fp = (CFILE *)malloc(sizeof(*fp));
	if (fp == NULL)
		return NULL;
	fp->file = (FILE *)(uintptr_t)data;
	fp->size = (int)size;
	fp->lib_offset = 0;
	fp->raw_position = 0;
	printf("CFILE flash open: %s, %u bytes at %p (alignment %u)\n",
		filename, size, data, (unsigned int)((uintptr_t)data & 3U));
	return fp;
}

int cfilelength(CFILE *fp) { return fp->size; }
int cftell(CFILE *fp) { return fp->raw_position; }

int cfgetc(CFILE *fp)
{
	if (fp->raw_position >= fp->size)
		return EOF;
	return cfile_data(fp)[fp->raw_position++];
}

char *cfgets(char *buffer, size_t count, CFILE *fp)
{
	char *start = buffer;
	int value;
	if (count == 0)
		return NULL;
	while (count > 1) {
		value = cfgetc(fp);
		if (value == EOF) {
			if (buffer == start)
				return NULL;
			break;
		}
		if (value == '\r')
			continue;
		*buffer++ = (char)value;
		count--;
		if (value == '\n')
			break;
	}
	*buffer = '\0';
	return start;
}

size_t cfread(void *buffer, size_t element_size, size_t count, CFILE *fp)
{
	size_t bytes;
	if (element_size != 0 && count > SIZE_MAX / element_size)
		return EOF;
	bytes = element_size * count;
	if (bytes > (size_t)(fp->size - fp->raw_position))
		return EOF;
	memcpy(buffer, cfile_data(fp) + fp->raw_position, bytes);
	fp->raw_position += (int)bytes;
	return count;
}

int cfseek(CFILE *fp, long offset, int origin)
{
	long position;
	switch (origin) {
	case SEEK_SET: position = offset; break;
	case SEEK_CUR: position = fp->raw_position + offset; break;
	case SEEK_END: position = fp->size + offset; break;
	default: return 1;
	}
	if (position < 0 || position > fp->size)
		return 1;
	fp->raw_position = (int)position;
	return 0;
}

void cfclose(CFILE *fp) { free(fp); }
#endif
