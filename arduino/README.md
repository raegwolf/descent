# Descent for Arduino ESP32-S3

This PlatformIO project builds the released Descent 1.5 gameplay engine as an
Arduino application for an ESP32-S3 with 16 MB flash and 8 MB octal PSRAM
(N16R8). It boots directly into Level 1 and presents the original indexed
320x200 framebuffer, centred at native resolution on a 480x320 ILI9488 SPI
display.

The current milestone intentionally has no keyboard, joystick, mouse, sound,
music, serial multiplayer, or networking. With no input mapping the game runs
but cannot be controlled.

An Arduino Uno cannot run this project: it does not have enough RAM, flash, or
CPU capacity. The red-screen Uno example that was previously in this directory
has been replaced by the ESP32-S3 game port.

## Required hardware and data

- ESP32-S3 N16R8 development board (16 MB flash, 8 MB OPI PSRAM)
- 3.5-inch 480x320 SPI display using the ILI9488 controller
- FAT-formatted SD card connected through the display module
- User-supplied shareware `DESCENT.HOG` and `DESCENT.PIG` in the SD-card root

The game data is not compiled into this repository or firmware. The source and
data remain subject to the non-commercial licence described in the repository
`README.TXT` and the shareware documentation.

## Wiring

The TFT and SD card share MOSI, MISO, and SCLK. They have separate chip-select
pins.

| Display module | ESP32-S3 GPIO |
| --- | ---: |
| VCC | 5V |
| GND | GND |
| TFT CS | 10 |
| TFT DC / RS | 9 |
| TFT RESET / RST | 8 |
| SDI / MOSI | 11 |
| SDO / MISO | 13 |
| SCK / CLK | 12 |
| SD CS | 4 |

ESP32-S3 GPIO uses 3.3 V logic. Do not apply 5 V to a GPIO. The listed display
breakout accepts 5 V power, but its interface must be compatible with 3.3 V
logic.

## Build and deploy

From the repository root:

```sh
cd arduino
~/.platformio/penv/bin/pio run
~/.platformio/penv/bin/pio run --target upload
~/.platformio/penv/bin/pio device monitor --baud 115200
```

If `pio` is already on `PATH`, use `pio` instead of the full executable path.
If PlatformIO cannot choose a USB port:

```sh
pio device list
pio run --target upload --upload-port /dev/cu.usbmodemXXXX
pio device monitor --port /dev/cu.usbmodemXXXX --baud 115200
```

The board resets and runs the firmware automatically after upload. Expected
serial startup includes:

```text
Descent ESP32-S3 starting
Loading the Descent engine...
Starting Level 1 (input, sound and networking disabled)...
```

Hardware or data errors are also printed over serial and displayed on the TFT.

## Port structure

`src/main.cpp` is the Arduino boundary: TFT presentation, SD mounting, PSRAM
allocation, and the FreeRTOS game task. `src/platform_arduino.c` supplies the
no-input/no-sound Arduino platform services. `tools/stage_engine.py` builds an
explicit list of the original C engine modules and portable replacements; DOS
assembly and networking modules are never compiled.

The engine still renders 8-bit indexed pixels. At presentation time the shim
converts a frame to RGB565 in PSRAM and transfers it over 40 MHz SPI. Large
historical global tables are also allocated from PSRAM under `ARDUINO`, leaving
internal RAM available for the runtime and game task.
