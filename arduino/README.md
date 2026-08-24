# Arduino Uno display bring-up

This PlatformIO project initializes the 3.5-inch SPI ILI9488 display, fills it
red, and writes `Hello, world!` in white.

## Wiring

| Display | Arduino Uno R3 |
| --- | --- |
| VCC | 5V |
| GND | GND |
| CS | D10 |
| DC / RS | D9 |
| RESET / RST | D8 |
| SDI / MOSI | D11 |
| SDO / MISO | D12 |
| SCK / CLK | D13 |

The sketch reserves D4 for the SD-card chip select and holds it high. Leave the
SD card out while testing the display.

## Build and upload

Install the PlatformIO IDE extension in VS Code, or install PlatformIO Core so
that the `pio` command is available. From this directory, run:

```sh
pio run
pio run --target upload
pio device monitor --baud 9600
```

When PlatformIO was installed through the VS Code extension on macOS but `pio`
is not on the normal shell path, invoke its executable directly:

```sh
~/.platformio/penv/bin/pio run
~/.platformio/penv/bin/pio run --target upload
~/.platformio/penv/bin/pio device monitor --baud 9600
```

The Uno starts the program automatically after upload. The serial monitor
should print:

```text
ILI9488 red-screen hello world is running.
```

Use `Ctrl-C` to exit the serial monitor. If PlatformIO cannot select the port,
list available devices and pass the appropriate port explicitly:

```sh
pio device list
pio run --target upload --upload-port /dev/cu.usbmodemXXXX
```
