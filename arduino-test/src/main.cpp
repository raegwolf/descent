#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

namespace {
constexpr int8_t kChipSelect = 10;
constexpr int8_t kDataCommand = 9;
constexpr int8_t kMosi = 11;
constexpr int8_t kClock = 12;
constexpr int8_t kReset = 6;
constexpr int8_t kMiso = 13;

// Passing explicit MOSI and clock pins selects software SPI. This diagnostic
// bypasses the ESP32-S3 hardware SPI peripheral and TFT_eSPI register access.
Adafruit_ILI9341 display(kChipSelect, kDataCommand, kMosi, kClock, kReset,
                         kMiso);

const char *flashModeName(FlashMode_t mode) {
  switch (mode) {
    case FM_QIO:
      return "QIO";
    case FM_QOUT:
      return "QOUT";
    case FM_DIO:
      return "DIO";
    case FM_DOUT:
      return "DOUT";
    case FM_FAST_READ:
      return "FAST_READ";
    case FM_SLOW_READ:
      return "SLOW_READ";
    default:
      return "UNKNOWN";
  }
}

void printHardwareInfo() {
  const uint32_t flashBytes = ESP.getFlashChipSize();
  const uint32_t psramBytes = ESP.getPsramSize();

  Serial.println("=== ESP32 hardware information ===");
  Serial.printf("Model: %s, revision %u, %u core(s)\n", ESP.getChipModel(),
                ESP.getChipRevision(), ESP.getChipCores());
  Serial.printf("CPU frequency: %lu MHz\n",
                static_cast<unsigned long>(ESP.getCpuFreqMHz()));
  Serial.printf("ESP-IDF version: %s\n", ESP.getSdkVersion());
  Serial.printf("Flash: %lu bytes (%lu MiB), %lu MHz, mode %s\n",
                static_cast<unsigned long>(flashBytes),
                static_cast<unsigned long>(flashBytes / (1024u * 1024u)),
                static_cast<unsigned long>(ESP.getFlashChipSpeed() / 1000000u),
                flashModeName(ESP.getFlashChipMode()));
  Serial.printf("Internal heap: %lu bytes total, %lu bytes free, "
                "%lu-byte largest block\n",
                static_cast<unsigned long>(ESP.getHeapSize()),
                static_cast<unsigned long>(ESP.getFreeHeap()),
                static_cast<unsigned long>(ESP.getMaxAllocHeap()));

  if (psramBytes != 0) {
    Serial.printf("PSRAM: %lu bytes (%lu MiB) total, %lu bytes free, "
                  "%lu-byte largest block\n",
                  static_cast<unsigned long>(psramBytes),
                  static_cast<unsigned long>(psramBytes / (1024u * 1024u)),
                  static_cast<unsigned long>(ESP.getFreePsram()),
                  static_cast<unsigned long>(ESP.getMaxAllocPsram()));
  } else {
    Serial.println("PSRAM: not detected or not enabled by the board build");
  }

  Serial.printf("Sketch: %lu bytes used, %lu bytes available\n",
                static_cast<unsigned long>(ESP.getSketchSize()),
                static_cast<unsigned long>(ESP.getFreeSketchSpace()));
  Serial.println("==================================");
}

void showColor(const char *name, uint16_t rgb565) {
  Serial.println(name);
  display.fillScreen(rgb565);
}
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1000);

  printHardwareInfo();

  display.begin();
  display.setRotation(0);

  Serial.println("ILI9341 software-SPI RGB fill test started");
  uint32_t displayId = 0;
  for (uint8_t index = 0; index < 4; ++index) {
    displayId =
        (displayId << 8) | display.readcommand8(ILI9341_RDID4, index);
  }
  Serial.printf("ILI9341 RDID4: 0x%08lX\n",
                static_cast<unsigned long>(displayId));
}

void loop() {
  showColor("RED", ILI9341_RED);
  delay(1000);
  showColor("GREEN", ILI9341_GREEN);
  delay(1000);
  showColor("BLUE", ILI9341_BLUE);
  delay(1000);
}
