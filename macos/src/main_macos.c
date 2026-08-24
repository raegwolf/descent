#include "key.h"
#include "gameseq.h"
#include "polyobj.h"
#include "timer.h"
#include "platform.h"

#include <limits.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#include <mach-o/dyld.h>

int inferno_init(int argc, char **argv);
void function_loop(void);
int inferno_done(void);
extern unsigned macos_tmap_scanline_count;

static int has_argument(int argc, char **argv, const char *wanted)
{
	int index;
	for (index = 1; index < argc; ++index)
		if (strcmp(argv[index], wanted) == 0)
			return 1;
	return 0;
}

static int render_model_smoke(void)
{
	vms_angvec angles = { 0, 0, 0 };
	int frame;
	int model_num;
	if (N_robot_types < 1)
		return 1;
	model_num = Robot_info[0].model_num;
	if (model_num < 0 || model_num >= N_polygon_models)
		return 1;
	macos_tmap_scanline_count = 0;
	draw_model_picture(model_num, &angles);
	if (macos_tmap_scanline_count == 0)
		return 1;
	for (frame = 0; frame < 32; ++frame)
		macos_present_frame();
	return 0;
}

static void run_level_smoke(void)
{
	const char *dump_path;
	StartNewGame(1);
	dump_path = getenv("DESCENT_LEVEL_FRAME_DUMP");
	if (dump_path != NULL)
		setenv("DESCENT_FRAME_DUMP", dump_path, 1);
	function_loop();
}

static int bundle_resources_path(char *path, size_t capacity)
{
	uint32_t size = (uint32_t)capacity;
	char *marker;
	if (_NSGetExecutablePath(path, &size) != 0) return 0;
	marker = strstr(path, "/Contents/MacOS/");
	if (marker == NULL) return 0;
	strcpy(marker, "/Contents/Resources");
	return 1;
}

static int ensure_directory(const char *path)
{
	if (mkdir(path, 0755) == 0 || errno == EEXIST)
		return 1;
	return 0;
}

static int has_suffix(const char *name, const char *suffix)
{
	size_t name_length = strlen(name);
	size_t suffix_length = strlen(suffix);
	return name_length >= suffix_length &&
	       strcasecmp(name + name_length - suffix_length, suffix) == 0;
}

static int is_user_file(const char *name)
{
	const char *extension = strrchr(name, '.');
	if (strcasecmp(name, "descent.cfg") == 0 ||
	    strcasecmp(name, "descent.hi") == 0 ||
	    has_suffix(name, ".plr") || has_suffix(name, ".plx") ||
	    has_suffix(name, ".dem"))
		return 1;
	return extension != NULL &&
	       (strncasecmp(extension, ".sg", 3) == 0 ||
	        strncasecmp(extension, ".mg", 3) == 0);
}

static void copy_if_missing(const char *source, const char *destination)
{
	FILE *input;
	FILE *output;
	char buffer[8192];
	size_t count;
	if (access(destination, F_OK) == 0)
		return;
	input = fopen(source, "rb");
	if (input == NULL)
		return;
	output = fopen(destination, "wb");
	if (output == NULL) {
		fclose(input);
		return;
	}
	while ((count = fread(buffer, 1, sizeof(buffer), input)) != 0)
		fwrite(buffer, 1, count, output);
	fclose(output);
	fclose(input);
}

static void migrate_bundle_user_files(const char *resources,
	                                  const char *user_directory)
{
	DIR *directory = opendir(resources);
	struct dirent *entry;
	char source[PATH_MAX];
	char destination[PATH_MAX];
	if (directory == NULL)
		return;
	while ((entry = readdir(directory)) != NULL) {
		if (!is_user_file(entry->d_name))
			continue;
		if (snprintf(source, sizeof(source), "%s/%s", resources,
		             entry->d_name) >= (int)sizeof(source) ||
		    snprintf(destination, sizeof(destination), "%s/%s",
		             user_directory, entry->d_name) >= (int)sizeof(destination))
			continue;
		copy_if_missing(source, destination);
	}
	closedir(directory);
}

static void link_bundle_resource(const char *resources,
	                             const char *user_directory,
	                             const char *name)
{
	char source[PATH_MAX];
	char destination[PATH_MAX];
	if (snprintf(source, sizeof(source), "%s/%s", resources, name) >=
	        (int)sizeof(source) ||
	    snprintf(destination, sizeof(destination), "%s/%s", user_directory,
	             name) >= (int)sizeof(destination))
		return;
	if (access(destination, F_OK) == 0)
		return;
	(void)symlink(source, destination);
}

static void use_macos_working_directory(void)
{
	char resources[PATH_MAX];
	char support[PATH_MAX];
	char user_directory[PATH_MAX];
	const char *override = getenv("DESCENT_USER_DIR");
	const char *user_home;
	if (!bundle_resources_path(resources, sizeof(resources)))
		return;
	if (override != NULL && override[0] != '\0') {
		if (snprintf(user_directory, sizeof(user_directory), "%s", override) >=
		    (int)sizeof(user_directory))
			return;
	} else {
		user_home = getenv("HOME");
		if (user_home == NULL)
			return;
		if (snprintf(support, sizeof(support), "%s/Library/Application Support",
		             user_home) >= (int)sizeof(support) ||
		    snprintf(user_directory, sizeof(user_directory), "%s/Descent",
		             support) >= (int)sizeof(user_directory))
			return;
		if (!ensure_directory(support))
			return;
	}
	if (!ensure_directory(user_directory))
		return;
	if (override == NULL || override[0] == '\0')
		migrate_bundle_user_files(resources, user_directory);
	link_bundle_resource(resources, user_directory, "DESCENT.HOG");
	link_bundle_resource(resources, user_directory, "DESCENT.PIG");
	(void)chdir(user_directory);
}

static void enable_finder_launch_log(void)
{
	char path[PATH_MAX];
	const char *user_home;
	FILE *log_file;
	if (getppid() != 1)
		return;
	user_home = getenv("HOME");
	if (user_home == NULL)
		return;
	if (snprintf(path, sizeof(path), "%s/Library/Logs/Descent.log",
	             user_home) >= (int)sizeof(path))
		return;
	log_file = fopen(path, "a");
	if (log_file == NULL)
		return;
	dup2(fileno(log_file), STDOUT_FILENO);
	dup2(fileno(log_file), STDERR_FILENO);
	fclose(log_file);
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	fprintf(stderr, "\n--- Descent launch ---\n");
}

int main(int argc, char **argv)
{
	int result;
	enable_finder_launch_log();
	use_macos_working_directory();
	timer_init();
	key_init();
	result = inferno_init(argc, argv);
	if (result == 0) {
		if (has_argument(argc, argv, "-macos-model-smoke"))
			result = render_model_smoke();
		else if (has_argument(argc, argv, "-macos-level-smoke"))
			run_level_smoke();
		else
			function_loop();
		if (result == 0)
			result = inferno_done();
	}
	key_close();
	timer_close();
	return result;
}
