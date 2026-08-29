#include <Arduino.h>

#ifndef DESCENT_MENU_ONLY
#define DESCENT_MENU_ONLY 0
#endif

#if DESCENT_MENU_ONLY

#include <climits>
#include <TFT_eSPI.h>
#include <esp_heap_caps.h>

namespace {

constexpr uint32_t kMebibyte = 1024u * 1024u;
constexpr uint32_t kRequiredFlashBytes = 16u * kMebibyte;
constexpr uint32_t kRequiredPsramBytes = 8u * kMebibyte;
constexpr uint32_t kPsramReservedAllowance = 64u * 1024u;
constexpr int16_t kPanelWidth = 320;
constexpr int16_t kPanelHeight = 240;
constexpr int16_t kGameWidth = 320;
constexpr int16_t kGameHeight = 200;
constexpr int16_t kGameTop = 20;
constexpr int16_t kDmaStripHeight = 50;
constexpr uint32_t kStartupScreenHoldMs = 750;
constexpr size_t kMaxFontCharacters = 128;
constexpr size_t kMenuItemCount = 8;
constexpr int16_t kOriginalTitleFontHeight = 22;
constexpr uint8_t kFontColor = 1;
constexpr uint8_t kFontProportional = 2;
constexpr uint8_t kFontKerned = 4;
constexpr uint8_t kTransparentColor = 255;
constexpr uint8_t kBitmapTableXor = 0xd3;
constexpr size_t kIndexedImageBytes =
    static_cast<size_t>(kGameWidth) * kGameHeight;
constexpr size_t kDmaStripPixels =
    static_cast<size_t>(kGameWidth) * kDmaStripHeight;
constexpr size_t kDmaStripBytes = kDmaStripPixels * sizeof(uint16_t);

extern const uint8_t embeddedHogStart[] asm("_binary_descent_hog_start");
extern const uint8_t embeddedHogEnd[] asm("_binary_descent_hog_end");

class EmbeddedFile {
 public:
  EmbeddedFile(const uint8_t *start, const uint8_t *end)
      : data_(start), size_(end - start), position_(0) {}

  bool seek(size_t position) {
    if (position > size_) {
      return false;
    }
    position_ = position;
    return true;
  }

  size_t read(uint8_t *destination, size_t byteCount) {
    if (byteCount > size_ - position_) {
      byteCount = size_ - position_;
    }
    memcpy(destination, data_ + position_, byteCount);
    position_ += byteCount;
    return byteCount;
  }

  size_t position() const { return position_; }
  size_t size() const { return size_; }

  const uint8_t *dataAt(size_t position, size_t byteCount) const {
    if (position > size_ || byteCount > size_ - position) {
      return nullptr;
    }
    return data_ + position;
  }

 private:
  const uint8_t *data_;
  size_t size_;
  size_t position_;
};

struct HogEntry {
  uint32_t offset;
  uint32_t length;
};

struct DescentFont {
  uint16_t width;
  uint16_t height;
  uint8_t flags;
  uint8_t minimumCharacter;
  uint8_t maximumCharacter;
  size_t characterCount;
  const uint8_t *glyphs[kMaxFontCharacters];
  uint16_t glyphWidths[kMaxFontCharacters];
  const uint8_t *kerning;
  const uint8_t *dataEnd;
  uint8_t colorMap[256];
  uint8_t monochromeColor;
};

TFT_eSPI display;
uint8_t *indexedImage;
uint16_t *dmaStrips[2];
uint8_t palette[256 * 3];
DescentFont normalMenuFont{};
DescentFont selectedMenuFont{};
DescentFont copyrightFont{};
char menuLabels[kMenuItemCount][32];
char copyrightText[80];

[[noreturn]] void stopWithMessage(const char *message) {
  Serial.printf("ERROR: %s\n", message);
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

bool readExact(EmbeddedFile &file, void *destination, size_t byteCount) {
  return file.read(static_cast<uint8_t *>(destination), byteCount) == byteCount;
}

uint32_t readLittleEndian32(const uint8_t bytes[4]) {
  return static_cast<uint32_t>(bytes[0]) |
         (static_cast<uint32_t>(bytes[1]) << 8) |
         (static_cast<uint32_t>(bytes[2]) << 16) |
         (static_cast<uint32_t>(bytes[3]) << 24);
}

uint16_t readLittleEndian16(const uint8_t *bytes) {
  return static_cast<uint16_t>(bytes[0]) |
         static_cast<uint16_t>(bytes[1] << 8);
}

char asciiLower(char character) {
  if (character >= 'A' && character <= 'Z') {
    return static_cast<char>(character - 'A' + 'a');
  }
  return character;
}

bool hogNameEquals(const char storedName[13], const char *wantedName) {
  for (size_t index = 0; index < 13; ++index) {
    const char stored = storedName[index];
    const char wanted = *wantedName;
    if (asciiLower(stored) != asciiLower(wanted)) {
      return false;
    }
    if (wanted == '\0') {
      return true;
    }
    ++wantedName;
  }
  return *wantedName == '\0';
}

bool findHogEntry(EmbeddedFile &hog, const char *wantedName, HogEntry &entry) {
  char magic[3];
  if (!hog.seek(0) || !readExact(hog, magic, sizeof(magic)) ||
      memcmp(magic, "DHF", sizeof(magic)) != 0) {
    return false;
  }

  const uint32_t hogSize = hog.size();
  while (static_cast<uint32_t>(hog.position()) + 17u <= hogSize) {
    char storedName[13];
    uint8_t encodedLength[4];
    if (!readExact(hog, storedName, sizeof(storedName)) ||
        !readExact(hog, encodedLength, sizeof(encodedLength))) {
      return false;
    }

    const uint32_t dataOffset = hog.position();
    const uint32_t dataLength = readLittleEndian32(encodedLength);
    if (dataLength > hogSize - dataOffset) {
      return false;
    }
    if (hogNameEquals(storedName, wantedName)) {
      entry = {dataOffset, dataLength};
      return true;
    }
    if (!hog.seek(dataOffset + dataLength)) {
      return false;
    }
  }
  return false;
}

bool decodePcx(EmbeddedFile &hog, const HogEntry &entry) {
  constexpr size_t kPcxHeaderBytes = 128;
  constexpr size_t kPcxPaletteBytes = 769;
  uint8_t header[kPcxHeaderBytes];
  if (entry.length < kPcxHeaderBytes + kPcxPaletteBytes ||
      !hog.seek(entry.offset) || !readExact(hog, header, sizeof(header))) {
    return false;
  }

  const uint16_t width =
      readLittleEndian16(&header[8]) - readLittleEndian16(&header[4]) + 1u;
  const uint16_t height =
      readLittleEndian16(&header[10]) - readLittleEndian16(&header[6]) + 1u;
  const uint16_t bytesPerLine = readLittleEndian16(&header[66]);
  if (header[0] != 10 || header[2] != 1 || header[3] != 8 ||
      header[65] != 1 || width != kGameWidth || height != kGameHeight ||
      bytesPerLine < width) {
    return false;
  }

  const uint32_t imageEnd = entry.offset + entry.length - kPcxPaletteBytes;
  for (uint16_t row = 0; row < height; ++row) {
    uint16_t column = 0;
    while (column < bytesPerLine) {
      uint8_t encoded;
      if (static_cast<uint32_t>(hog.position()) >= imageEnd ||
          !readExact(hog, &encoded, 1)) {
        return false;
      }

      uint8_t count = 1;
      uint8_t value = encoded;
      if ((encoded & 0xc0u) == 0xc0u) {
        count = encoded & 0x3fu;
        if (count == 0 || static_cast<uint32_t>(hog.position()) >= imageEnd ||
            !readExact(hog, &value, 1)) {
          return false;
        }
      }
      if (count > bytesPerLine - column) {
        return false;
      }
      for (uint8_t repeated = 0; repeated < count; ++repeated, ++column) {
        if (column < width) {
          indexedImage[static_cast<size_t>(row) * width + column] = value;
        }
      }
    }
  }

  uint8_t paletteMarker;
  if (!hog.seek(imageEnd) || !readExact(hog, &paletteMarker, 1) ||
      paletteMarker != 12 || !readExact(hog, palette, sizeof(palette))) {
    return false;
  }
  return true;
}

bool loadPcx(EmbeddedFile &hog, const char *name) {
  HogEntry entry{};
  if (!findHogEntry(hog, name, entry)) {
    Serial.printf("ERROR: %s not found in embedded HOG\n", name);
    return false;
  }
  Serial.printf("Loading %s from embedded DESCENT.HOG (%lu bytes)\n", name,
                static_cast<unsigned long>(entry.length));
  return decodePcx(hog, entry);
}

uint8_t rotateLeft(uint8_t value) {
  return static_cast<uint8_t>((value << 1) | (value >> 7));
}

uint8_t decodeTextByte(uint8_t value) {
  value = rotateLeft(value);
  value ^= kBitmapTableXor;
  return rotateLeft(value);
}

bool loadTextLine(EmbeddedFile &hog, const HogEntry &textEntry,
                  size_t wantedLine, char *destination,
                  size_t destinationSize) {
  const uint8_t *source = hog.dataAt(textEntry.offset, textEntry.length);
  if (source == nullptr || destinationSize == 0) {
    return false;
  }

  size_t line = 0;
  size_t sourceOffset = 0;
  while (line < wantedLine && sourceOffset < textEntry.length) {
    if (source[sourceOffset++] == '\n') {
      ++line;
    }
  }
  if (line != wantedLine || sourceOffset >= textEntry.length) {
    return false;
  }

  size_t outputOffset = 0;
  while (sourceOffset < textEntry.length && source[sourceOffset] != '\n') {
    if (source[sourceOffset] != '\r') {
      if (outputOffset + 1 >= destinationSize) {
        return false;
      }
      destination[outputOffset++] =
          static_cast<char>(decodeTextByte(source[sourceOffset]));
    }
    ++sourceOffset;
  }
  destination[outputOffset] = '\0';
  return outputOffset != 0;
}

int squaredDifference(int left, int right) {
  const int difference = left - right;
  return difference * difference;
}

uint8_t findClosestMenuColor(int red, int green, int blue) {
  int bestDistance = INT_MAX;
  uint8_t bestIndex = 0;
  // Match the original palette mapper: indices 254 and 255 are reserved.
  for (int index = 0; index < 254; ++index) {
    const size_t entry = static_cast<size_t>(index) * 3u;
    const int distance =
        squaredDifference(red, palette[entry] >> 2) +
        squaredDifference(green, palette[entry + 1u] >> 2) +
        squaredDifference(blue, palette[entry + 2u] >> 2);
    if (distance < bestDistance) {
      bestDistance = distance;
      bestIndex = static_cast<uint8_t>(index);
      if (distance == 0) {
        break;
      }
    }
  }
  return bestIndex;
}

bool loadFont(EmbeddedFile &hog, const char *name, DescentFont &font) {
  constexpr size_t kFileHeaderBytes = 8;
  constexpr size_t kDiskFontHeaderBytes = 28;
  constexpr size_t kColorPaletteBytes = 768;
  HogEntry entry{};
  if (!findHogEntry(hog, name, entry)) {
    return false;
  }

  const uint8_t *file = hog.dataAt(entry.offset, entry.length);
  if (file == nullptr || entry.length < kFileHeaderBytes + kDiskFontHeaderBytes ||
      memcmp(file, "PSFN", 4) != 0) {
    return false;
  }

  const uint32_t dataSize = readLittleEndian32(file + 4);
  if (dataSize < kDiskFontHeaderBytes ||
      dataSize > entry.length - kFileHeaderBytes) {
    return false;
  }
  const uint8_t *diskFont = file + kFileHeaderBytes;
  font.width = readLittleEndian16(diskFont);
  font.height = readLittleEndian16(diskFont + 2);
  font.flags = static_cast<uint8_t>(readLittleEndian16(diskFont + 4));
  font.minimumCharacter = diskFont[8];
  font.maximumCharacter = diskFont[9];
  if (font.maximumCharacter < font.minimumCharacter) {
    return false;
  }
  font.characterCount =
      static_cast<size_t>(font.maximumCharacter - font.minimumCharacter) + 1u;
  if (font.characterCount > kMaxFontCharacters || font.height == 0) {
    return false;
  }

  const uint32_t glyphDataOffset = readLittleEndian32(diskFont + 12);
  const uint32_t widthsOffset = readLittleEndian32(diskFont + 20);
  const uint32_t kerningOffset = readLittleEndian32(diskFont + 24);
  if (glyphDataOffset >= dataSize || widthsOffset >= dataSize ||
      font.characterCount * sizeof(uint16_t) > dataSize - widthsOffset) {
    return false;
  }

  const uint8_t *glyph = diskFont + glyphDataOffset;
  const uint8_t *fontDataEnd = diskFont + dataSize;
  for (size_t index = 0; index < font.characterCount; ++index) {
    font.glyphWidths[index] =
        readLittleEndian16(diskFont + widthsOffset + index * 2u);
    const size_t rowBytes = (font.flags & kFontColor)
                                ? font.glyphWidths[index]
                                : (font.glyphWidths[index] + 7u) / 8u;
    const size_t glyphBytes = rowBytes * font.height;
    if (glyph > fontDataEnd || glyphBytes > static_cast<size_t>(fontDataEnd - glyph)) {
      return false;
    }
    font.glyphs[index] = glyph;
    glyph += glyphBytes;
  }

  font.kerning = nullptr;
  font.dataEnd = fontDataEnd;
  if (font.flags & kFontKerned) {
    if (kerningOffset >= dataSize) {
      return false;
    }
    font.kerning = diskFont + kerningOffset;
  }

  if (font.flags & kFontColor) {
    if (entry.length - kFileHeaderBytes - dataSize < kColorPaletteBytes) {
      return false;
    }
    const uint8_t *fontPalette = diskFont + dataSize;
    for (size_t index = 0; index < 255; ++index) {
      const size_t color = index * 3u;
      font.colorMap[index] = findClosestMenuColor(
          fontPalette[color], fontPalette[color + 1u],
          fontPalette[color + 2u]);
    }
    font.colorMap[kTransparentColor] = kTransparentColor;
  }
  return true;
}

int fontCharacterIndex(const DescentFont &font, unsigned char character) {
  if (character < font.minimumCharacter ||
      character > font.maximumCharacter) {
    return -1;
  }
  return character - font.minimumCharacter;
}

int characterSpacing(const DescentFont &font, unsigned char character,
                     unsigned char nextCharacter) {
  const int characterIndex = fontCharacterIndex(font, character);
  if (characterIndex < 0) {
    return font.width / 2;
  }

  int spacing = font.glyphWidths[characterIndex];
  const int nextIndex = fontCharacterIndex(font, nextCharacter);
  if (font.kerning != nullptr && nextIndex >= 0) {
    const uint8_t *entry = font.kerning;
    while (entry < font.dataEnd && *entry != 255) {
      if (font.dataEnd - entry < 3) {
        break;
      }
      if (entry[0] == characterIndex && entry[1] == nextIndex) {
        spacing = entry[2];
        break;
      }
      entry += 3;
    }
  }
  return spacing;
}

int textWidth(const DescentFont &font, const char *text) {
  int width = 0;
  while (*text != '\0' && *text != '\n') {
    const unsigned char character = static_cast<unsigned char>(*text);
    const unsigned char nextCharacter =
        static_cast<unsigned char>(text[1]);
    width += characterSpacing(font, character, nextCharacter);
    ++text;
  }
  return width;
}

void drawText(const DescentFont &font, int startX, int startY,
              const char *text) {
  int x = startX;
  while (*text != '\0') {
    const unsigned char character = static_cast<unsigned char>(*text);
    const unsigned char nextCharacter =
        static_cast<unsigned char>(text[1]);
    const int characterIndex = fontCharacterIndex(font, character);
    const int spacing = characterSpacing(font, character, nextCharacter);
    if (characterIndex >= 0) {
      const uint16_t glyphWidth = font.glyphWidths[characterIndex];
      const uint8_t *glyph = font.glyphs[characterIndex];
      if (font.flags & kFontColor) {
        for (uint16_t row = 0; row < font.height; ++row) {
          for (uint16_t column = 0; column < glyphWidth; ++column) {
            const uint8_t sourceColor = glyph[row * glyphWidth + column];
            const int destinationX = x + column;
            const int destinationY = startY + row;
            if (sourceColor != kTransparentColor && destinationX >= 0 &&
                destinationX < kGameWidth && destinationY >= 0 &&
                destinationY < kGameHeight) {
              indexedImage[destinationY * kGameWidth + destinationX] =
                  font.colorMap[sourceColor];
            }
          }
        }
      } else {
        const size_t rowBytes = (glyphWidth + 7u) / 8u;
        for (uint16_t row = 0; row < font.height; ++row) {
          for (uint16_t column = 0; column < glyphWidth; ++column) {
            const uint8_t bits = glyph[row * rowBytes + column / 8u];
            const int destinationX = x + column;
            const int destinationY = startY + row;
            if ((bits & (0x80u >> (column & 7u))) != 0 &&
                destinationX >= 0 && destinationX < kGameWidth &&
                destinationY >= 0 && destinationY < kGameHeight) {
              indexedImage[destinationY * kGameWidth + destinationX] =
                  font.monochromeColor;
            }
          }
        }
      }
    }
    x += spacing;
    ++text;
  }
}

bool buildMainMenu(EmbeddedFile &hog) {
  static constexpr size_t kMenuTextLines[kMenuItemCount] = {
      0,    // New game
      325,  // Options...
      326,  // Change Pilots...
      327,  // View Demo...
      1,    // High scores
      329,  // Ordering Info
      328,  // Credits
      2,    // Quit
  };
  HogEntry textEntry{};
  if (!findHogEntry(hog, "descent.txb", textEntry)) {
    return false;
  }
  for (size_t index = 0; index < kMenuItemCount; ++index) {
    if (!loadTextLine(hog, textEntry, kMenuTextLines[index],
                      menuLabels[index], sizeof(menuLabels[index]))) {
      return false;
    }
  }
  if (!loadTextLine(hog, textEntry, 11, copyrightText,
                    sizeof(copyrightText)) ||
      !loadFont(hog, "font2-1.fnt", normalMenuFont) ||
      !loadFont(hog, "font2-2.fnt", selectedMenuFont) ||
      !loadFont(hog, "font3-1.fnt", copyrightFont)) {
    return false;
  }
  copyrightFont.monochromeColor = findClosestMenuColor(6, 6, 6);

  int widestItem = 0;
  for (const char *label : menuLabels) {
    widestItem = max(widestItem, textWidth(normalMenuFont, label));
  }
  const int menuWidth = widestItem + 30;
  const int menuHeight = kOriginalTitleFontHeight + 8 +
                         kMenuItemCount * (normalMenuFont.height + 1) + 30;
  const int textX = (kGameWidth - menuWidth) / 2 + 15;
  int textY = (kGameHeight - menuHeight) / 2 +
              kOriginalTitleFontHeight + 8 + 15;

  for (size_t index = 0; index < kMenuItemCount; ++index) {
    drawText(index == 0 ? selectedMenuFont : normalMenuFont, textX, textY,
             menuLabels[index]);
    textY += normalMenuFont.height + 1;
  }
  const int copyrightX =
      (kGameWidth - textWidth(copyrightFont, copyrightText)) / 2;
  drawText(copyrightFont, copyrightX,
           kGameHeight - copyrightFont.height - 2, copyrightText);
  return true;
}

uint16_t paletteColor(uint8_t paletteIndex) {
  const size_t entry = static_cast<size_t>(paletteIndex) * 3u;
  const uint16_t red = palette[entry];
  const uint16_t green = palette[entry + 1u];
  const uint16_t blue = palette[entry + 2u];
  return static_cast<uint16_t>(((red & 0xf8u) << 8) |
                               ((green & 0xfcu) << 3) | (blue >> 3));
}

uint16_t swapBytes(uint16_t value) {
  return static_cast<uint16_t>((value << 8) | (value >> 8));
}

void presentFrame() {
  display.fillScreen(TFT_BLACK);
  display.startWrite();
  int stripIndex = 0;
  for (int16_t sourceY = 0; sourceY < kGameHeight;
       sourceY += kDmaStripHeight) {
    const int16_t height =
        min<int16_t>(kDmaStripHeight, kGameHeight - sourceY);
    uint16_t *strip = dmaStrips[stripIndex];
    const size_t pixelCount = static_cast<size_t>(kGameWidth) * height;
    const uint8_t *source =
        indexedImage + static_cast<size_t>(sourceY) * kGameWidth;

    // Match esp32-test: prepare the next byte-swapped internal-SRAM strip
    // while the previous FSPI DMA transaction is still in flight.
    for (size_t pixel = 0; pixel < pixelCount; ++pixel) {
      strip[pixel] = swapBytes(paletteColor(source[pixel]));
    }
    display.dmaWait();
    display.pushImageDMA(0, kGameTop + sourceY, kGameWidth, height,
                         static_cast<const uint16_t *>(strip));
    stripIndex ^= 1;
  }
  display.dmaWait();
  display.endWrite();
}

void verifyHardwareConfiguration() {
  if (ESP.getFlashChipSize() < kRequiredFlashBytes) {
    stopWithMessage("16 MB flash is required");
  }
  if (!psramFound() ||
      ESP.getPsramSize() < kRequiredPsramBytes - kPsramReservedAllowance) {
    stopWithMessage("8 MB OPI PSRAM is required");
  }
  if (display.width() != kPanelWidth || display.height() != kPanelHeight) {
    stopWithMessage("Expected 320x240 landscape mode");
  }
}

void allocateBuffers() {
  indexedImage = static_cast<uint8_t *>(heap_caps_malloc(
      kIndexedImageBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (indexedImage == nullptr) {
    stopWithMessage("Could not allocate indexed framebuffer");
  }
  for (uint16_t *&strip : dmaStrips) {
    strip = static_cast<uint16_t *>(heap_caps_malloc(
        kDmaStripBytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
    if (strip == nullptr) {
      stopWithMessage("Could not allocate two DMA strips");
    }
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Descent ESP32-S3 menu bring-up starting");

  Serial.println("Initializing ILI9341 on FSPI...");
  display.init();
  display.setRotation(1);
  display.setSwapBytes(true);
  display.fillScreen(TFT_BLACK);
  verifyHardwareConfiguration();
  allocateBuffers();

  EmbeddedFile hog(embeddedHogStart, embeddedHogEnd);
  if (!display.initDMA()) {
    stopWithMessage("Could not initialize TFT SPI DMA");
  }

  static constexpr const char *kStartupScreens[] = {
      "iplogo1.pcx", "logo.pcx", "descent.pcx"};
  for (const char *screen : kStartupScreens) {
    if (!loadPcx(hog, screen)) {
      stopWithMessage("Could not load startup PCX");
    }
    presentFrame();
    delay(kStartupScreenHoldMs);
  }

  if (!loadPcx(hog, "menu.pcx") || !buildMainMenu(hog)) {
    stopWithMessage("Could not build main menu");
  }
  presentFrame();
  Serial.println("Descent main menu displayed at y=20..219");
}

void loop() {
  delay(1000);
}

#else

#include <SPI.h>
#include <TFT_eSPI.h>
#include <esp_heap_caps.h>

#include "esp32_bridge.h"
#include "joystick_input.h"

namespace {

constexpr uint32_t kMebibyte = 1024u * 1024u;
constexpr uint32_t kRequiredFlashBytes = 16u * kMebibyte;
constexpr uint32_t kRequiredPsramBytes = 8u * kMebibyte;
constexpr uint32_t kPsramReservedAllowance = 64u * 1024u;
constexpr int kTftWidth = 320;
constexpr int kTftHeight = 240;
constexpr int kGameWidth = 320;
constexpr int kGameHeight = 200;
constexpr int kGameTop = 20;
constexpr int kDmaStripHeight = 50;
constexpr size_t kDmaStripPixels =
    static_cast<size_t>(kGameWidth) * kDmaStripHeight;
constexpr size_t kDmaStripBytes = kDmaStripPixels * sizeof(uint16_t);
constexpr int kLeftJoystickXPin = 4;
constexpr int kLeftJoystickYPin = 5;
constexpr int kLeftJoystickButtonPin = 15;
constexpr int kRightJoystickXPin = 6;
constexpr int kRightJoystickYPin = 7;
constexpr int kRightJoystickButtonPin = 16;
constexpr unsigned int kJoystickCenterSamples = 32;

TFT_eSPI display;
uint16_t *dmaStrips[2];

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

uint16_t indexedColor(const unsigned char *palette, unsigned char index)
{
    const unsigned int entry = static_cast<unsigned int>(index) * 3U;
    const uint16_t red = palette[entry];
    const uint16_t green = palette[entry + 1];
    const uint16_t blue = palette[entry + 2];
    const uint16_t color = static_cast<uint16_t>(
        ((red & 0x3eU) << 10) | ((green & 0x3fU) << 5) | (blue >> 1));
    return static_cast<uint16_t>((color << 8) | (color >> 8));
}

void verifyHardwareConfiguration()
{
    if (ESP.getFlashChipSize() < kRequiredFlashBytes)
        stopWithMessage("16 MB flash is required");
    if (!psramFound() ||
        ESP.getPsramSize() < kRequiredPsramBytes - kPsramReservedAllowance)
        stopWithMessage("8 MB OPI PSRAM is required");
    if (display.width() != kTftWidth || display.height() != kTftHeight)
        stopWithMessage("Expected 320x240 landscape mode");
}

void allocateDmaStrips()
{
    for (uint16_t *&strip : dmaStrips) {
        strip = static_cast<uint16_t *>(heap_caps_malloc(
            kDmaStripBytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
        if (strip == nullptr)
            stopWithMessage("Could not allocate two DMA strips");
    }
}

void initializeJoysticks()
{
    static constexpr int kAxisPins[DESCENT_JOYSTICK_AXIS_COUNT] = {
        kLeftJoystickXPin, kLeftJoystickYPin,
        kRightJoystickXPin, kRightJoystickYPin
    };
    uint32_t totals[DESCENT_JOYSTICK_AXIS_COUNT] = {};
    uint16_t centers[DESCENT_JOYSTICK_AXIS_COUNT];

    pinMode(kLeftJoystickButtonPin, INPUT_PULLUP);
    pinMode(kRightJoystickButtonPin, INPUT_PULLUP);
    analogReadResolution(12);
    for (int pin : kAxisPins)
        analogSetPinAttenuation(pin, ADC_11db);

    Serial.println("Centering joysticks; leave both sticks released...");
    for (unsigned int sample = 0; sample < kJoystickCenterSamples; ++sample) {
        for (unsigned int axis = 0; axis < DESCENT_JOYSTICK_AXIS_COUNT;
             ++axis)
            totals[axis] += analogRead(kAxisPins[axis]);
        delay(2);
    }
    for (unsigned int axis = 0; axis < DESCENT_JOYSTICK_AXIS_COUNT; ++axis)
        centers[axis] = static_cast<uint16_t>(
            totals[axis] / kJoystickCenterSamples);
    descent_joystick_calibrate(millis(), centers);
    Serial.printf("Joystick neutral readings: J1=%u,%u J2=%u,%u\n",
                  centers[0], centers[1], centers[2], centers[3]);
    Serial.printf("Joystick offsets: J1=%d,%d J2=%d,%d\n",
                  descent_joystick_axis_offset(0),
                  descent_joystick_axis_offset(1),
                  descent_joystick_axis_offset(2),
                  descent_joystick_axis_offset(3));
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

    Serial.printf("Free internal RAM before engine: %u bytes; PSRAM: %u bytes\n",
                  heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                  heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    timer_init();
    key_init();
    if (!esp32_init_world_storage() || !esp32_init_ai_storage() ||
        !esp32_init_object_storage() || !esp32_init_polygon_storage() ||
        !esp32_init_automap_storage() || !esp32_init_morph_storage() ||
        !esp32_init_piggy_storage() || !esp32_init_bitmap_storage() ||
        !esp32_init_lighting_storage() || !esp32_init_robot_storage() ||
        !esp32_init_effect_storage())
        stopWithMessage("Could not allocate engine tables in PSRAM");
    Serial.println("Loading the Descent engine...");
    if (inferno_init(6, arguments) != 0)
        stopWithMessage("Engine initialization failed");

    Serial.printf("Free internal RAM after engine: %u bytes; PSRAM: %u bytes\n",
                  heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                  heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    Serial.println("Starting Level 1 (dual joystick input enabled; sound and networking disabled)...");
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

extern "C" void *esp32_alloc_psram(unsigned int size)
{
    return heap_caps_calloc(1, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

extern "C" void esp32_free_psram(void *buffer)
{
    heap_caps_free(buffer);
}

extern "C" void esp32_present_indexed(const unsigned char *pixels,
                                          const unsigned char *palette,
                                          int width, int height)
{
    if (pixels == nullptr || palette == nullptr || dmaStrips[0] == nullptr ||
        width != kGameWidth || height < kGameHeight)
        return;

    display.startWrite();
    int stripIndex = 0;
    for (int sourceY = 0; sourceY < kGameHeight;
         sourceY += kDmaStripHeight) {
        const int stripHeight = min(kDmaStripHeight, kGameHeight - sourceY);
        const size_t pixelCount =
            static_cast<size_t>(kGameWidth) * stripHeight;
        const unsigned char *source =
            pixels + static_cast<size_t>(sourceY) * kGameWidth;
        uint16_t *strip = dmaStrips[stripIndex];
        for (size_t pixel = 0; pixel < pixelCount; ++pixel)
            strip[pixel] = indexedColor(palette, source[pixel]);
        display.dmaWait();
        /* indexedColor() has already put every RGB565 pixel into the byte
         * order required by SPI DMA.  Preserve the esp32-test contract by
         * selecting TFT_eSPI's const overload; the mutable overload observes
         * setSwapBytes(true) and would swap this strip a second time. */
        display.pushImageDMA(0, kGameTop + sourceY, kGameWidth,
                             stripHeight,
                             static_cast<const uint16_t *>(strip));
        stripIndex ^= 1;
    }
    display.dmaWait();
    display.endWrite();
    yield();
}

extern "C" uint32_t esp32_milliseconds(void)
{
    return millis();
}

extern "C" void esp32_poll_joysticks(void)
{
    static constexpr int kAxisPins[DESCENT_JOYSTICK_AXIS_COUNT] = {
        kLeftJoystickXPin, kLeftJoystickYPin,
        kRightJoystickXPin, kRightJoystickYPin
    };
    uint16_t raw[DESCENT_JOYSTICK_AXIS_COUNT];
    uint8_t buttons = 0;

    for (unsigned int axis = 0; axis < DESCENT_JOYSTICK_AXIS_COUNT; ++axis)
        raw[axis] = static_cast<uint16_t>(analogRead(kAxisPins[axis]));
    if (digitalRead(kLeftJoystickButtonPin) == LOW)
        buttons |= 1U << DESCENT_JOYSTICK_LEFT_BUTTON;
    if (digitalRead(kRightJoystickButtonPin) == LOW)
        buttons |= 1U << DESCENT_JOYSTICK_RIGHT_BUTTON;
    descent_joystick_update(millis(), raw, buttons);
}

extern "C" void esp32_delay_ms(unsigned int milliseconds)
{
    delay(milliseconds);
}

extern "C" int esp32_check_heap_integrity(void)
{
    return heap_caps_check_integrity_all(true) ? 1 : 0;
}

void setup()
{
    Serial.begin(115200);
    delay(250);
    Serial.println("Descent ESP32-S3 starting");

    initializeJoysticks();

    display.init();
    display.setRotation(1);
    display.setSwapBytes(true);
    display.fillScreen(TFT_BLACK);
    verifyHardwareConfiguration();
    allocateDmaStrips();
    if (!display.initDMA())
        stopWithMessage("Could not initialize TFT SPI DMA");
    Serial.println("DESCENT.HOG and DESCENT.PIG are embedded in flash");
    Serial.println("Dual joystick input enabled on GPIO 4/5/15 and 6/7/16");

    BaseType_t created = xTaskCreatePinnedToCore(
        gameTask, "descent", 49152, nullptr, 1, nullptr, 1);
    if (created != pdPASS)
        stopWithMessage("Could not create game task");
}

void loop()
{
    delay(1000);
}

#endif
