ROOT := ..
BUILD := build
STAGE := $(BUILD)/source
DEPS := .deps
APP := $(BUILD)/Descent.app
CONTENTS := $(APP)/Contents
EXECUTABLE := $(CONTENTS)/MacOS/descent
SDL_FRAMEWORK := $(DEPS)/SDL3.framework
SDL_STAMP := $(DEPS)/.sdl3-3.4.14
SOURCE_DIRS := INCLUDES CFILE BIOS 2D 3D FIX VECMAT MISC MEM IFF PSLIB TEXMAP MAIN
SOURCE_INPUTS := $(shell find $(addprefix $(ROOT)/,$(SOURCE_DIRS)) -type f 2>/dev/null)
STAGE_STAMP := $(STAGE)/.prepared

CC := clang
CPPFLAGS := -DMACOS=1 -DSHAREWARE=1 -DRELEASE=1 -DNDEBUG=1 -DNASM=1 \
	-D_far= -Dfar= -Dnear= -D__far= -D__near= -D__interrupt= -D_interrupt= \
	-Iinclude -Isrc -I$(STAGE)/INCLUDES -I$(STAGE)/CFILE -I$(STAGE)/BIOS \
	-I$(STAGE)/2D -I$(STAGE)/3D -I$(STAGE)/FIX -I$(STAGE)/VECMAT \
	-I$(STAGE)/MISC -I$(STAGE)/MEM -I$(STAGE)/IFF -I$(STAGE)/PSLIB \
	-I$(STAGE)/TEXMAP -I$(STAGE)/MAIN
CFLAGS := -std=gnu89 -funsigned-char -fwrapv -fcommon -fno-strict-aliasing -g \
	-Wall -Wno-nonportable-include-path -Wno-unknown-pragmas \
	-Wno-implicit-function-declaration -Wno-int-conversion \
	-Wno-incompatible-pointer-types -Wno-deprecated-non-prototype -Wno-return-mismatch
APP_OPTFLAGS ?= -O2
APP_FLAGS := $(CPPFLAGS) $(CFLAGS) $(APP_OPTFLAGS) -F$(DEPS)

ENGINE_SOURCES := \
	CFILE/CFILE.C \
	2D/PCX.C 2D/CANVAS.C 2D/BITMAP.C 2D/BITBLT.C \
	2D/PIXEL.C 2D/GPIXEL.C 2D/RECT.C 2D/LINE.C \
	2D/SCANLINE.C 2D/RLE.C 2D/FONT.C 2D/DISC.C \
	2D/SCALE.C 2D/IBITBLT.C 2D/LINEAR_C.C \
	MISC/ERROR.C MEM/MEM.C IFF/IFF.C \
	MAIN/TITLES.C MAIN/TEXT.C MAIN/MENU.C MAIN/CONFIG.C MAIN/NEWMENU.C \
	MAIN/KCONFIG.C MAIN/GAMEFONT.C MAIN/GAMESEQ.C MAIN/BM.C MAIN/PIGGY.C \
	MAIN/HASH.C MAIN/CNTRLCEN.C MAIN/FUELCEN.C MAIN/GAUGES.C MAIN/OBJECT.C \
	MAIN/AI.C MAIN/WALL.C MAIN/GAMESEG.C MAIN/LASER.C MAIN/PHYSICS.C \
	MAIN/COLLIDE.C MAIN/FIREBALL.C MAIN/POWERUP.C MAIN/WEAPON.C MAIN/GAME.C \
	MAIN/SLEW.C MAIN/AIPATH.C MAIN/LIGHTING.C MAIN/RENDER.C MAIN/AUTOMAP.C \
	MAIN/FVI.C MAIN/FVI_OFLOW_C.C MAIN/HOSTAGE.C MAIN/NEWDEMO.C \
	MAIN/NEWDEMO_SHAREWARE_C.C MAIN/SWITCH.C MAIN/ENDLEVEL.C \
	MAIN/TERRAIN.C MAIN/GAMEMINE.C MAIN/HUD.C MAIN/MGLOBAL.C MAIN/VCLIP.C \
	MAIN/EFFECTS.C MAIN/ROBOT.C MAIN/TEXMERGE.C MAIN/POLYOBJ.C MAIN/INFERNO.C \
	MAIN/ARGS.C MAIN/SCORES.C MAIN/CREDITS.C MAIN/MISSION.C \
	MAIN/PLAYSAVE.C MAIN/GAMESAVE.C MAIN/MORPH.C MAIN/PAGING.C MAIN/CONTROLS.C \
	MAIN/BMREAD.C MAIN/JOYDEFS.C MAIN/STATE.C MAIN/STATE_SHAREWARE_C.C \
	FIX/FIX_C.C FIX/TABLES_C.C VECMAT/VECMAT_C.C VECMAT/VM_ALIAS_C.C \
	TEXMAP/NTMAP.C TEXMAP/TMAPFLAT.C TEXMAP/SCANLINE.C \
	3D/CLIPPER_C.C 3D/DRAW_C.C 3D/GLOBVARS_C.C 3D/HORIZON_C.C \
	3D/INSTANCE_C.C 3D/INTERP_C.C 3D/MATRIX_C.C 3D/POINTS_C.C \
	3D/ROD_C.C 3D/SETUP_C.C

PLATFORM_SOURCES := src/config.c src/main_macos.c src/sdl_shim.c src/gr_macos.c src/palette_macos.c \
	src/key_macos.c src/timer_macos.c src/platform_stubs.c src/dos_find_macos.c \
	src/songs_macos.c src/psrand.c
ENGINE_OBJECTS := $(addprefix $(BUILD)/obj/,$(ENGINE_SOURCES:.C=.o))
PLATFORM_OBJECTS := $(addprefix $(BUILD)/obj/macos/,$(notdir $(PLATFORM_SOURCES:.c=.o)))
APP_OBJECTS := $(ENGINE_OBJECTS) $(PLATFORM_OBJECTS)

.PHONY: all app test launch-test level-smoke model-smoke smoke run clean prepare
.SECONDARY:

all: test app
app: $(EXECUTABLE)

$(SDL_STAMP): scripts/fetch_sdl3.sh
	@mkdir -p $(DEPS)
	sh scripts/fetch_sdl3.sh $(DEPS)
	@touch $@

prepare: $(STAGE_STAMP)

$(STAGE_STAMP): $(SOURCE_INPUTS) Makefile
	@mkdir -p $(STAGE)
	@for directory in $(SOURCE_DIRS); do rsync -a $(ROOT)/$$directory/ $(STAGE)/$$directory/; done
	@find $(STAGE) -type f \( -name '*.C' -o -name '*.H' \) -exec perl -pi -e 's/\x1a//g; /^\s*#\s*include/ && s#\\\\#/#g' {} +
	@touch $@

$(STAGE)/%.C: | prepare
	@test -f $@

$(BUILD)/obj/%.o: $(STAGE)/%.C $(STAGE_STAMP)
	@mkdir -p $(dir $@)
	$(CC) -x c $(APP_FLAGS) -c $< -o $@

$(BUILD)/obj/macos/%.o: src/%.c $(SDL_STAMP) | prepare
	@mkdir -p $(dir $@)
	$(CC) -x c $(APP_FLAGS) -c $< -o $@

$(EXECUTABLE): $(APP_OBJECTS) Info.plist $(SDL_STAMP) | prepare
	@mkdir -p $(CONTENTS)/MacOS $(CONTENTS)/Frameworks $(CONTENTS)/Resources
	$(CC) $(APP_OBJECTS) -F$(DEPS) -framework SDL3 -lm \
		-Wl,-rpath,@executable_path/../Frameworks -o $(EXECUTABLE)
	cp Info.plist $(CONTENTS)/Info.plist
	ditto $(SDL_FRAMEWORK) $(CONTENTS)/Frameworks/SDL3.framework
	@if test -f $(ROOT)/resources/descent.hog; then cp $(ROOT)/resources/descent.hog $(CONTENTS)/Resources/DESCENT.HOG; fi
	@if test -f $(ROOT)/resources/descent.pig; then cp $(ROOT)/resources/descent.pig $(CONTENTS)/Resources/DESCENT.PIG; fi
	codesign --force --deep --sign - $(APP)

$(BUILD)/test_fix: tests/test_fix.c $(STAGE)/FIX/FIX_C.C $(STAGE)/FIX/TABLES_C.C | prepare
	@mkdir -p $(BUILD)
	$(CC) -x c $(CPPFLAGS) $(CFLAGS) tests/test_fix.c $(STAGE)/FIX/FIX_C.C $(STAGE)/FIX/TABLES_C.C -lm -o $@

$(BUILD)/test_linear: tests/test_linear.c $(STAGE)/2D/LINEAR_C.C | prepare
	@mkdir -p $(BUILD)
	$(CC) -x c $(CPPFLAGS) $(CFLAGS) tests/test_linear.c $(STAGE)/2D/LINEAR_C.C -o $@

$(BUILD)/test_asm_contracts: tests/test_asm_contracts.c \
		$(STAGE)/MAIN/FVI_OFLOW_C.C $(STAGE)/VECMAT/VECMAT_C.C \
		$(STAGE)/VECMAT/VM_ALIAS_C.C $(STAGE)/FIX/FIX_C.C $(STAGE)/FIX/TABLES_C.C | prepare
	@mkdir -p $(BUILD)
	$(CC) -x c $(CPPFLAGS) $(CFLAGS) tests/test_asm_contracts.c \
		$(STAGE)/MAIN/FVI_OFLOW_C.C $(STAGE)/VECMAT/VECMAT_C.C \
		$(STAGE)/VECMAT/VM_ALIAS_C.C $(STAGE)/FIX/FIX_C.C \
		$(STAGE)/FIX/TABLES_C.C -lm -o $@

$(BUILD)/test_texmap_scanline: tests/test_texmap_scanline.c $(STAGE)/TEXMAP/SCANLINE.C | prepare
	@mkdir -p $(BUILD)
	$(CC) -x c $(CPPFLAGS) $(CFLAGS) tests/test_texmap_scanline.c \
		$(STAGE)/TEXMAP/SCANLINE.C -o $@

$(BUILD)/test_dos_find: tests/test_dos_find.c src/dos_find_macos.c
	@mkdir -p $(BUILD)
	$(CC) -x c -Iinclude $(CFLAGS) tests/test_dos_find.c src/dos_find_macos.c -o $@

$(BUILD)/test_sdl_quit: tests/test_sdl_quit.c src/sdl_shim.c $(SDL_STAMP)
	@mkdir -p $(BUILD)
	$(CC) -x c -Isrc $(CFLAGS) -F$(DEPS) tests/test_sdl_quit.c src/sdl_shim.c \
		-framework SDL3 -Wl,-rpath,$(abspath $(DEPS)) -o $@

test: $(BUILD)/test_fix $(BUILD)/test_linear $(BUILD)/test_asm_contracts \
		$(BUILD)/test_texmap_scanline $(BUILD)/test_dos_find $(BUILD)/test_sdl_quit
	$(BUILD)/test_fix
	$(BUILD)/test_linear
	$(BUILD)/test_asm_contracts
	$(BUILD)/test_texmap_scanline
	$(BUILD)/test_dos_find
	$(BUILD)/test_sdl_quit

launch-test: app
	@cd $(ROOT); \
		SDL_VIDEODRIVER=dummy DESCENT_USER_DIR=/tmp/descent-launch-test-data \
		macos/$(EXECUTABLE) -notitles -nosound \
		-nonetwork -nojoystick -nomouse > /tmp/descent-launch-test.log 2>&1 & \
		launch_pid=$$!; \
		sleep 2; \
		if ! kill -0 $$launch_pid 2>/dev/null; then \
			cat /tmp/descent-launch-test.log; \
			wait $$launch_pid; \
			exit 1; \
		fi; \
		kill -KILL $$launch_pid; \
		wait $$launch_pid 2>/dev/null || true

smoke: app
	cd $(ROOT) && SDL_VIDEODRIVER=dummy DESCENT_USER_DIR=/tmp/descent-smoke-data \
		DESCENT_FRAME_DUMP=/tmp/descent-startup.ppm \
		DESCENT_SMOKE_EXIT=1 \
		macos/$(EXECUTABLE) -macos-smoke -notitles -nosound -nonetwork \
		-nojoystick -nomouse
	test -s /tmp/descent-startup.ppm

model-smoke: app
	cd $(ROOT) && SDL_VIDEODRIVER=dummy DESCENT_USER_DIR=/tmp/descent-smoke-data \
		DESCENT_FRAME_DUMP=/tmp/descent-model.ppm \
		DESCENT_SMOKE_EXIT=1 \
		macos/$(EXECUTABLE) -macos-smoke -macos-model-smoke -notitles \
		-nosound -nonetwork -nojoystick -nomouse
	test -s /tmp/descent-model.ppm

level-smoke: app
	cd $(ROOT) && SDL_VIDEODRIVER=dummy DESCENT_USER_DIR=/tmp/descent-smoke-data \
		DESCENT_LEVEL_FRAME_DUMP=/tmp/descent-level.ppm DESCENT_SMOKE_EXIT=1 \
		macos/$(EXECUTABLE) -macos-smoke -macos-level-smoke -notitles \
		-nosound -nonetwork -nojoystick -nomouse
	test -s /tmp/descent-level.ppm

run: app
	open $(APP)

clean:
	rm -rf $(BUILD)
