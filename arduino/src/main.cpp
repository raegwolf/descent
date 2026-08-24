#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <ILI9488.h>

namespace {

constexpr uint8_t kTftChipSelectPin = 10;
constexpr uint8_t kTftDataCommandPin = 9;
constexpr uint8_t kTftResetPin = 8;
constexpr uint8_t kSdChipSelectPin = 4;

ILI9488 display(kTftChipSelectPin, kTftDataCommandPin, kTftResetPin);

}  // namespace

void setup()
{
    Serial.begin(9600);

    // Keep the display module's SD card off the shared SPI bus for bring-up.
    pinMode(kSdChipSelectPin, OUTPUT);
    digitalWrite(kSdChipSelectPin, HIGH);

    // Hardware SPI on an Uno uses D11 (MOSI), D12 (MISO), and D13 (SCK).
    display.begin();
    display.setRotation(1);
    display.fillScreen(ILI9488_RED);
    display.setCursor(20, 20);
    display.setTextColor(ILI9488_WHITE);
    display.setTextSize(3);
    display.println(F("Hello, world!"));

    Serial.println(F("ILI9488 red-screen hello world is running."));
}

void loop()
{
}
