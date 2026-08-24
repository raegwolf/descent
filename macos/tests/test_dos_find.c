#include "dos.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
	char directory[] = "/tmp/descent-find-XXXXXX";
	char old_directory[1024];
	struct find_t found;
	FILE *file;
	int saw_alpha = 0;
	int saw_beta = 0;

	assert(getcwd(old_directory, sizeof(old_directory)) != NULL);
	assert(mkdtemp(directory) != NULL);
	assert(chdir(directory) == 0);
	file = fopen("alpha.plr", "wb"); assert(file != NULL); fclose(file);
	file = fopen("beta.plr", "wb"); assert(file != NULL); fclose(file);
	file = fopen("ignore.dem", "wb"); assert(file != NULL); fclose(file);

	assert(_dos_findfirst("*.plr", 0, &found) == 0);
	do {
		if (strcmp(found.name, "alpha.plr") == 0) saw_alpha = 1;
		if (strcmp(found.name, "beta.plr") == 0) saw_beta = 1;
	} while (_dos_findnext(&found) == 0);
	assert(saw_alpha && saw_beta);

	assert(chdir(old_directory) == 0);
	{
		char path[1200];
		snprintf(path, sizeof(path), "%s/alpha.plr", directory); unlink(path);
		snprintf(path, sizeof(path), "%s/beta.plr", directory); unlink(path);
		snprintf(path, sizeof(path), "%s/ignore.dem", directory); unlink(path);
	}
	assert(rmdir(directory) == 0);
	return 0;
}
