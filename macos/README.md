# Descent macOS port

This directory contains the macOS SDL3 platform boundary for the released
Descent 1 source. It builds the original engine as a signed arm64
`Descent.app`, loads the original HOG/PIG resources, and presents Descent's
320x200 indexed framebuffer with nearest-neighbor scaling. The current
milestone reaches and runs the original main menu; it does not contain a
replacement menu or reimplemented game flow.

Sound playback and networking are deliberately excluded for this first
graphics pass. Their original call sites remain intact, with sound functions
implemented as no-ops at the macOS boundary and `NETWORK` disabled for the
`MACOS` build.

## Build and run

```sh
./build-run-macos.sh
```

Equivalent direct make commands:

```sh
make -C macos test app
make -C macos run
```

The first build downloads the official SDL 3.4.14 macOS framework, verifies
its SHA-256, and stores it under ignored `macos/.deps/`. The framework and any
available `resources/descent.hog` and `resources/descent.pig` files are copied
into the app bundle. A normal launch ensures that `pilot.plr` exists, loads
that pilot without showing pilot-selection or creation menus, skips the title
and Level 1 briefing, then starts a new game on Level 1. Set
`start_to_new_game` to `0` in `macos/src/config.c` to restore the original
title, briefing, and main-menu startup flow. The same file's `show_fps` setting
controls the in-game FPS counter.

For a headless graphics/asset smoke test:

```sh
make -C macos launch-test smoke model-smoke
```

`launch-test` verifies that a normal startup remains alive instead of exiting
during initialization. `smoke` starts the real engine with SDL's dummy video
driver, loads the shareware data, follows the direct-to-Level-1 startup path,
writes a rendered frame to `/tmp/descent-startup.ppm`, and exits successfully.
`model-smoke` renders an original textured robot preview through the
briefing/polygon-model path and writes `/tmp/descent-model.ppm`.

To exercise Level 1 initialization with its dedicated frame dump:

```sh
make -C macos level-smoke
```

Fatal messages from a Finder or `open` launch are retained in
`~/Library/Logs/Descent.log`.

Pilots, configuration, saves, and demos are stored in
`~/Library/Application Support/Descent`. On the first launch after this
change, user files created by earlier builds inside the app bundle are copied
there automatically.

## Portable engine implementations

The Makefile explicitly lists C translation units and does not compile or link
any `.ASM` file. Adjacent portable implementations cover:

- fixed-point and table routines in `FIX/FIX_C.C` and `FIX/TABLES_C.C`;
- vector/matrix routines in `VECMAT/VECMAT_C.C` and alias entry points in
  `VECMAT/VM_ALIAS_C.C`;
- macOS branches in the original 2D bitmap, canvas, line, scaling, font, RLE,
  and blit sources under `2D/`;
- macOS branches in the original software texture mapper sources under
  `TEXMAP/`;
- the complete software 3D pipeline in `3D/*_C.C`;
- FVI's Watcom overflow check in `MAIN/FVI_OFLOW_C.C`.

Assembly replacement files contain contract comments identifying the preserved
assembly behavior. Tests cover fixed-point math, integer square roots, linear
drawing in both directions, overflow behavior, and vector alias/cross/dot
contracts.
