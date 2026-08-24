/* POSIX implementation of the DOS filename enumeration API used by menus. */
#include "dos.h"

#include <glob.h>
#include <string.h>

static glob_t find_matches;
static size_t find_index;
static int find_active;

static int copy_current_name(struct find_t *result)
{
	const char *name;
	const char *slash;
	if (!find_active || find_index >= find_matches.gl_pathc)
		return 1;
	name = find_matches.gl_pathv[find_index];
	slash = strrchr(name, '/');
	if (slash != NULL)
		name = slash + 1;
	strncpy(result->name, name, sizeof(result->name) - 1);
	result->name[sizeof(result->name) - 1] = '\0';
	return 0;
}

int _dos_findfirst(const char *pattern, int attributes, struct find_t *result)
{
	int glob_result;
	(void)attributes;
	if (find_active) {
		globfree(&find_matches);
		find_active = 0;
	}
	memset(&find_matches, 0, sizeof(find_matches));
	find_index = 0;
	glob_result = glob(pattern, GLOB_NOSORT, NULL, &find_matches);
	if (glob_result != 0 || find_matches.gl_pathc == 0) {
		if (glob_result == 0)
			globfree(&find_matches);
		return 1;
	}
	find_active = 1;
	return copy_current_name(result);
}

int _dos_findnext(struct find_t *result)
{
	if (!find_active)
		return 1;
	find_index++;
	if (find_index >= find_matches.gl_pathc) {
		globfree(&find_matches);
		find_active = 0;
		return 1;
	}
	return copy_current_name(result);
}
