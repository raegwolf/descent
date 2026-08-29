# Descent ESP32-S3 direct Level 1 bring-up

This PlatformIO target boots the shareware game directly into Level 1. It
skips the title screens, briefing, pilot UI, and main menu, using the same
direct-start sequence as the macOS port: initialize the engine, create the
automatic `pilot` state in memory, suppress briefings, call `StartNewGame(1)`,
and enter the game loop.

Both `resources/descent.hog` and `resources/descent.pig` are linked into the
firmware and read directly from memory-mapped flash. No SD card or writable
player/config filesystem is required. The source and data remain subject to
the non-commercial licence in the repository `README.TXT`.

## Display and hardware

The target is configured for the tested ESP32-S3 N16R8 board: 16 MB flash,
8 MB OPI PSRAM, and a 2.8-inch ILI9341 SPI display. The original indexed
320x200 game framebuffer is shown at native resolution on the rotated 320x240
panel. Rows 0-19 and 220-239 remain black, giving 20 pixels above and below.

The display configuration retains the validated `esp32-test` path:

- TFT_eSPI 2.5.43 with ILI9341 and FSPI
- MOSI 11, SCLK 12, MISO 13, CS 10, DC 9, reset 8
- 40 MHz SPI and `USE_FSPI_PORT=1`
- the ESP32-S3 DMA callback fix in `tools/patch_tft_espi.py`
- alternating byte-swapped 320x50 DMA strips in internal SRAM

Sound, networking, joystick, mouse, and keyboard input are disabled for this
first-level display test.

## Flash layout

The HOG is 2,339,773 bytes and the PIG is 5,092,871 bytes. Together they are
7,432,644 bytes, before code. `partitions.csv` therefore uses one 15 MiB
factory application slot instead of two OTA slots. OTA updating is not
available in this bring-up layout; serial flashing remains available.

The verified linked application is 8,055,521 bytes, 51.2% of the 15 MiB slot.
Changing either resource automatically rebuilds its linked object.

Each embedded archive is independently aligned to a 4-byte flash boundary.
This is required by the ESP32-S3 for DOS-era data paths that assume aligned
32-bit access. The partition table also reserves a 64 KiB core-dump area for
hardware bring-up diagnostics.

## Fast profiles

The default `custom_menu_only = no` stages the explicit engine source list and
builds direct-Level-1 firmware. Set it to `yes` only for the earlier static
menu/display diagnostic: that conditional omits the gameplay translation
units and PIG object so it compiles quickly.

## Build, upload, and monitor

From `esp32/`:

```sh
./build-upload-monitor.sh
```

Pass a serial device if automatic selection is ambiguous:

```sh
./build-upload-monitor.sh /dev/cu.usbserial-XXXX
```

Build without uploading:

```sh
~/.platformio/penv/bin/pio run
```

At 115200 baud, startup reports hardware checks, embedded resource use, free
internal/PSRAM before and after engine initialization, and the Level 1 start.
Fatal bring-up errors are printed to serial and displayed on the TFT.
