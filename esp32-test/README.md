# ESP32-S3 ILI9341 asynchronous DMA test

This standalone PlatformIO ESP32-S3 test project uses the Arduino framework and
targets an ESP32-S3 N16R8 module
(16 MB QIO flash and 8 MB OPI PSRAM) and drives the corrected Next Dimension
Electronics 2.8-inch 240x320 SPI display in 320x240 landscape mode. It validates the configured flash,
PSRAM, panel dimensions, two full 153,600-byte RGB565 framebuffers in PSRAM,
and two 32,000-byte DMA strip buffers in internal SRAM. A dedicated FreeRTOS
task transfers one frame while `loop()` renders the next. The panel displays
live memory and completed-transfer FPS figures, while serial diagnostics are
printed at 115200 baud.

## Why the sketch is `main.cpp`, not `.ino`

This is an Arduino-framework project even though its entry point is
`src/main.cpp`: it includes `Arduino.h` and supplies the normal `setup()` and
`loop()` functions. PlatformIO conventionally uses an explicit C++ source file.
An Arduino IDE `.ino` sketch is also C++, but the IDE preprocesses it by adding
headers and generating function declarations. Avoiding that hidden preprocessing
makes this test consistent with the larger Descent port, which mixes C and C++
translation units.

## Wiring

| Display pin | ESP32-S3 DevKitC-1 | Purpose |
| --- | ---: | --- |
| `SDO (MISO)` | Not connected | GPIO13 is reserved in software; TFT reads are unused |
| `LED` | `3V3` | Backlight always on |
| `SCK` | GPIO12 | SPI clock |
| `SDI (MOSI)` | GPIO11 | SPI data to the display |
| `DC` | GPIO9 | Data/command selection |
| `RESET` | GPIO8 | Display reset |
| `CS` | GPIO10 | Display chip select |
| `GND` | `GND` | Common ground |
| `VCC` | `5V` | Module power |

The ESP32-S3 GPIO signals are 3.3 V. Never connect 5 V to a GPIO. The display
listing says its onboard regulator and level shifting accept either 5 V or
3.3 V module power and logic; this setup powers `VCC` from 5 V and connects
`LED` to 3.3 V. The SD card and touchscreen are not used by this test.

## Build, upload, and monitor

From this directory:

```sh
./build-upload-monitor.sh
```

The script builds first, uploads only after a successful build, then opens the
serial monitor.

If PlatformIO cannot select the USB serial port automatically, pass it as the
first argument:

```sh
./build-upload-monitor.sh /dev/cu.usbmodem1101
```

Alternatively set `PORT`. Set `PIO_BIN` if PlatformIO is not on `PATH` or at
`~/.platformio/penv/bin/pio`.

This test keeps `Serial` on ESP32-S3 UART0 for boards whose USB connector uses
a WCH USB-to-UART bridge (for example VID:PID `1A86:55D3`). It does not redirect
logging to the ESP32-S3 native USB CDC/JTAG peripheral.

Press Ctrl-C to close the monitor. Expected output repeats continuously:

```text
Flash chip: 16777216 bytes (16 MiB), ...
PSRAM: ... bytes (~8 MiB) usable, ...
Running app slot: 6553600 bytes capacity, ... bytes used, ... bytes free
PSRAM framebuffers: 2 x 153600 bytes OK
Internal DMA strips: 2 x 32000 bytes OK
ILI9341 320x240 asynchronous DMA display started
setup() complete; entering loop()
millis(): ..., DMA FPS: ... (... frames in ... ms)
```

The diagnostic uses TFT_eSPI and 40 MHz hardware SPI without changing the
wiring. Each frame is split into four 320x50 DMA transfers and one 320x40
transfer. The display task alternates the two internal DMA buffers while the
main task renders into whichever complete PSRAM framebuffer is free. The FPS
counter measures frames whose final DMA transfer has completed; serial logging
is limited to one report per second so it does not become the bottleneck.

`USE_FSPI_PORT` is explicitly enabled in `platformio.ini`. This is required for
TFT_eSPI 2.5.x on ESP32-S3 so its direct-register writes target the SPI2/FSPI
peripheral rather than the invalid address `0x10`; it does not change the GPIO
wiring above.

The project pins TFT_eSPI 2.5.43 and applies a small build-time ESP32-S3 patch.
Its DMA completion callback otherwise mixes the ESP-IDF logical host number
with a raw register index and writes to invalid address `0x30`. Static flash and
partition sizes are cached before DMA begins because querying the sketch size
performs an expensive app-image SHA verification.

All UART output goes through the `serialBegin`, `serialPrintln`, and
`serialPrintf` wrappers. For a performance run with serial initialization,
formatting, and writes compiled out, change this build flag in `platformio.ini`:

```ini
-D SERIAL_LOGGING_ENABLED=0
```

The on-screen FPS counter continues to update when serial logging is disabled.

The display also shows a per-frame timing profile:

- `Frame`: total display-task time for a complete frame.
- `DMA wait`: time blocked waiting for SPI DMA completion.
- `Copy/swap`: time spent copying RGB565 pixels from PSRAM into the internal
  DMA strips and changing byte order.
- `Submit`: address-window setup and DMA transaction submission time.
- `Main wait`: time the main task waited for a free complete framebuffer.

Copy/swap is intentionally performed before waiting for the previous strip, so
that CPU work remains overlapped with the active SPI transfer. Profiling adds a
small number of `micros()` calls per frame.
