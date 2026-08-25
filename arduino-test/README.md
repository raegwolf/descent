# ESP32-S3 ILI9341 RGB fill test

This standalone PlatformIO Arduino project drives the corrected Next Dimension
Electronics 2.8-inch 240x320 SPI display. It fills the ILI9341 panel red,
green, and blue continuously with no intentional delay and prints each color
name at 115200 baud before drawing it.

## Wiring

| Display pin | ESP32-S3 DevKitC-1 | Purpose |
| --- | ---: | --- |
| `SDO (MISO)` | GPIO13 | SPI data from the display; optional for this test |
| `LED` | `3V3` | Backlight always on |
| `SCK` | GPIO12 | SPI clock |
| `SDI (MOSI)` | GPIO11 | SPI data to the display |
| `DC` | GPIO9 | Data/command selection |
| `RESET` | GPIO6 | Display reset |
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
serial monitor. The equivalent `./build_upload_monitor` spelling also works.

If PlatformIO cannot select the USB serial port automatically, pass it as the
first argument:

```sh
./build-upload-monitor.sh /dev/cu.usbmodem1101
```

Alternatively set `PORT`. Set `PIO_BIN` if PlatformIO is not on `PATH` or at
`~/.platformio/penv/bin/pio`.

Press Ctrl-C to close the monitor. Expected output repeats continuously:

```text
ILI9341 RGB fill test started
RED
GREEN
BLUE
```

The current diagnostic uses software SPI to bypass ESP32-S3 hardware-SPI and
driver-specific register handling. Once the wiring and ILI9341 are confirmed,
the project can return to faster hardware SPI.
