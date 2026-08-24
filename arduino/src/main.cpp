#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <esp_heap_caps.h>
#include <unistd.h>

#include "arduino_bridge.h"

namespace {

constexpr int kTftWidth = 480;
constexpr int kTftHeight = 320;
constexpr int kGameWidth = 320;
constexpr int kGameHeight = 200;
constexpr int kGameX = (kTftWidth - kGameWidth) / 2;
constexpr int kGameY = (kTftHeight - kGameHeight) / 2;
constexpr int kSdChipSelectPin = 4;
constexpr int kSpiClockPin = 12;
constexpr int kSpiMisoPin = 13;
constexpr int kSpiMosiPin = 11;

TFT_eSPI display;
uint16_t *rgbFrame;

[[noreturn]] void stopWithMessage(const char *message)
{
    Serial.println(message);
    display.fillScreen(TFT_BLACK);
    display.setTextColor(TFT_RED, TFT_BLACK);
    display.setTextSize(2);
    display.setCursor(8, 12);
    display.println("Descent stopped:");
    display.setTextColor(TFT_WHITE, TFT_BLACK);
    display.println(message);
    for (;;) {
        delay(1000);
    }
}

bool resourceExists(const char *upper, const char *lower)
{
    return SD.exists(upper) || SD.exists(lower);
}

void gameTask(void *)
{
    char program[] = "descent";
    char notitles[] = "-notitles";
    char nosound[] = "-nosound";
    char nonetwork[] = "-nonetwork";
    char nojoystick[] = "-nojoystick";
    char nomouse[] = "-nomouse";
    char *arguments[] = {
        program, notitles, nosound, nonetwork, nojoystick, nomouse, nullptr
    };

    timer_init();
    key_init();
    if (!arduino_init_world_storage() || !arduino_init_ai_storage() ||
        !arduino_init_object_storage() || !arduino_init_polygon_storage() ||
        !arduino_init_automap_storage() || !arduino_init_morph_storage() ||
        !arduino_init_piggy_storage() || !arduino_init_bitmap_storage() ||
        !arduino_init_lighting_storage() || !arduino_init_robot_storage() ||
        !arduino_init_effect_storage())
        stopWithMessage("Could not allocate engine tables in PSRAM");
    Serial.println("Loading the Descent engine...");
    if (inferno_init(6, arguments) != 0)
        stopWithMessage("Engine initialization failed");

    Serial.println("Starting Level 1 (input, sound and networking disabled)...");
    const int previousSkipBriefings = Skip_briefing_screens;
    Skip_briefing_screens = 1;
    StartNewGame(1);
    Skip_briefing_screens = previousSkipBriefings;
    function_loop();

    inferno_done();
    key_close();
    timer_close();
    stopWithMessage("The game loop returned");
}

}  // namespace

extern "C" void *arduino_alloc_psram(unsigned int size)
{
    return heap_caps_calloc(1, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

extern "C" void arduino_free_psram(void *buffer)
{
    heap_caps_free(buffer);
}

extern "C" void arduino_present_indexed(const unsigned char *pixels,
                                          const unsigned char *palette,
                                          int width, int height)
{
    if (pixels == nullptr || palette == nullptr || rgbFrame == nullptr ||
        width != kGameWidth || height < kGameHeight)
        return;

    for (int pixel = 0; pixel < kGameWidth * kGameHeight; ++pixel) {
        const unsigned int entry = static_cast<unsigned int>(pixels[pixel]) * 3U;
        const uint16_t red = static_cast<uint16_t>(palette[entry] >> 1);
        const uint16_t green = static_cast<uint16_t>(palette[entry + 1]);
        const uint16_t blue = static_cast<uint16_t>(palette[entry + 2] >> 1);
        rgbFrame[pixel] = static_cast<uint16_t>((red << 11) | (green << 5) | blue);
    }

    display.pushImage(kGameX, kGameY, kGameWidth, kGameHeight, rgbFrame);
    yield();
}

extern "C" uint32_t arduino_milliseconds(void)
{
    return millis();
}

extern "C" void arduino_delay_ms(unsigned int milliseconds)
{
    delay(milliseconds);
}

void setup()
{
    Serial.begin(115200);
    delay(250);
    Serial.println("Descent ESP32-S3 starting");

    pinMode(kSdChipSelectPin, OUTPUT);
    digitalWrite(kSdChipSelectPin, HIGH);
    display.init();
    display.setRotation(1);
    display.setSwapBytes(true);
    display.fillScreen(TFT_BLACK);

    if (!psramFound())
        stopWithMessage("8 MB PSRAM is required");

    rgbFrame = static_cast<uint16_t *>(heap_caps_malloc(
        kGameWidth * kGameHeight * sizeof(uint16_t),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (rgbFrame == nullptr)
        stopWithMessage("Could not allocate RGB framebuffer");

    SPI.begin(kSpiClockPin, kSpiMisoPin, kSpiMosiPin, kSdChipSelectPin);
    if (!SD.begin(kSdChipSelectPin, SPI, 20000000U, "/sd", 8, false))
        stopWithMessage("SD card mount failed");
    if (!resourceExists("/DESCENT.HOG", "/descent.hog") ||
        !resourceExists("/DESCENT.PIG", "/descent.pig"))
        stopWithMessage("Copy DESCENT.HOG and DESCENT.PIG to SD root");
    if (chdir("/sd") != 0)
        stopWithMessage("Could not select SD root");

    BaseType_t created = xTaskCreatePinnedToCore(
        gameTask, "descent", 32768, nullptr, 1, nullptr, 1);
    if (created != pdPASS)
        stopWithMessage("Could not create game task");
}

void loop()
{
    delay(1000);
}
