#include <Arduino.h>

#ifndef TFT_MOSI
#define TFT_MOSI 11
#endif
#ifndef TFT_SCLK
#define TFT_SCLK 12
#endif
#ifndef TFT_CS
#define TFT_CS 10
#endif
#ifndef TFT_DC
#define TFT_DC 9
#endif
#ifndef TFT_RST
#define TFT_RST 8
#endif

#include <Adafruit_GFX.h>
#include <TFT_eSPI.h>
#include <esp_heap_caps.h>
#include <esp_ota_ops.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#ifndef SERIAL_LOGGING_ENABLED
#define SERIAL_LOGGING_ENABLED 1
#endif

namespace {
constexpr uint32_t kMebibyte = 1024u * 1024u;
constexpr uint32_t kRequiredFlashBytes = 16u * kMebibyte;
constexpr uint32_t kRequiredPsramBytes = 8u * kMebibyte;
constexpr uint32_t kPsramReservedAllowance = 64u * 1024u;
constexpr uint32_t kFpsReportIntervalMs = 1000u;
constexpr int16_t kPanelWidth = 320;
constexpr int16_t kPanelHeight = 240;
constexpr int16_t kDmaStripHeight = 50;
constexpr size_t kFramebufferPixels =
    static_cast<size_t>(kPanelWidth) * kPanelHeight;
constexpr size_t kFramebufferBytes = kFramebufferPixels * sizeof(uint16_t);
constexpr size_t kDmaStripPixels =
    static_cast<size_t>(kPanelWidth) * kDmaStripHeight;
constexpr size_t kDmaStripBytes = kDmaStripPixels * sizeof(uint16_t);

TFT_eSPI display;

class PsramCanvas16 : public GFXcanvas16 {
 public:
  PsramCanvas16() : GFXcanvas16(kPanelWidth, kPanelHeight, false) {}

  void attach(uint16_t *storage) { buffer = storage; }
};

struct FrameBuffer {
  PsramCanvas16 canvas;
  uint16_t *pixels = nullptr;
};

struct StaticMemoryInfo {
  uint32_t sramTotal;
  uint32_t psramTotal;
  uint32_t flashChip;
  uint32_t appUsed;
  uint32_t appCapacity;
};

struct TimingProfile {
  uint32_t displayFrameUs;
  uint32_t dmaWaitUs;
  uint32_t copySwapUs;
  uint32_t submitUs;
  uint32_t framebufferWaitUs;
};

FrameBuffer frameBuffers[2];
StaticMemoryInfo staticMemory;
uint16_t *dmaStrips[2];
QueueHandle_t freeFrameQueue;
QueueHandle_t readyFrameQueue;
TaskHandle_t displayTaskHandle;
uint32_t completedFrames;
uint32_t fpsWindowStartMs;
uint32_t completedFramesAtWindowStart;
uint32_t measuredFpsTenths;
uint32_t lastDisplayFrameUs;
uint32_t lastDmaWaitUs;
uint32_t lastCopySwapUs;
uint32_t lastSubmitUs;
uint32_t lastFramebufferWaitUs;

void serialBegin(uint32_t baud) {
#if SERIAL_LOGGING_ENABLED
  Serial.begin(baud);
#else
  (void)baud;
#endif
}

void serialPrintln(const char *message) {
#if SERIAL_LOGGING_ENABLED
  Serial.println(message);
#else
  (void)message;
#endif
}

template <typename... Args>
void serialPrintf(const char *format, Args... args) {
#if SERIAL_LOGGING_ENABLED
  Serial.printf(format, args...);
#else
  (void)format;
  (void)sizeof...(args);
#endif
}

[[noreturn]] void stopWithMessage(const char *message) {
  serialPrintf("ERROR: %s\n", message);
  display.fillScreen(TFT_RED);
  display.setTextColor(TFT_WHITE, TFT_RED);
  display.setTextSize(2);
  display.setCursor(8, 12);
  display.println("Hardware error:");
  display.println(message);
  for (;;) {
    delay(1000);
  }
}

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

void captureStaticMemoryInfo() {
  const esp_partition_t *appPartition = esp_ota_get_running_partition();
  staticMemory.sramTotal = ESP.getHeapSize();
  staticMemory.psramTotal = ESP.getPsramSize();
  staticMemory.flashChip = ESP.getFlashChipSize();
  // getSketchSize() verifies the app image with SHA hardware, so call it only
  // before display DMA starts rather than once per rendered frame.
  staticMemory.appUsed = ESP.getSketchSize();
  staticMemory.appCapacity = appPartition != nullptr ? appPartition->size : 0u;
}

void printHardwareInfo() {
  serialPrintln("=== ESP32 hardware information ===");
  serialPrintf("Model: %s, revision %u, %u core(s)\n", ESP.getChipModel(),
               ESP.getChipRevision(), ESP.getChipCores());
  serialPrintf("CPU frequency: %lu MHz\n",
               static_cast<unsigned long>(ESP.getCpuFreqMHz()));
  serialPrintf("ESP-IDF version: %s\n", ESP.getSdkVersion());
  serialPrintf("Flash chip: %lu bytes (%lu MiB), %lu MHz, mode %s\n",
               static_cast<unsigned long>(staticMemory.flashChip),
               static_cast<unsigned long>(staticMemory.flashChip / kMebibyte),
               static_cast<unsigned long>(ESP.getFlashChipSpeed() / 1000000u),
               flashModeName(ESP.getFlashChipMode()));
  serialPrintf("Internal heap: %lu bytes total, %lu bytes free, "
               "%lu-byte largest block\n",
               static_cast<unsigned long>(staticMemory.sramTotal),
               static_cast<unsigned long>(ESP.getFreeHeap()),
               static_cast<unsigned long>(ESP.getMaxAllocHeap()));

  if (staticMemory.psramTotal != 0) {
    serialPrintf("PSRAM: %lu bytes (~%lu MiB) usable, %lu bytes free, "
                 "%lu-byte largest block\n",
                 static_cast<unsigned long>(staticMemory.psramTotal),
                 static_cast<unsigned long>(
                     (staticMemory.psramTotal + kMebibyte / 2u) / kMebibyte),
                 static_cast<unsigned long>(ESP.getFreePsram()),
                 static_cast<unsigned long>(ESP.getMaxAllocPsram()));
  } else {
    serialPrintln("PSRAM: not detected or not enabled by the board build");
  }

  if (staticMemory.appCapacity != 0) {
    serialPrintf("Running app slot: %lu bytes capacity, %lu bytes used, "
                 "%lu bytes free\n",
                 static_cast<unsigned long>(staticMemory.appCapacity),
                 static_cast<unsigned long>(staticMemory.appUsed),
                 static_cast<unsigned long>(staticMemory.appCapacity -
                                            staticMemory.appUsed));
  }
  serialPrintln("==================================");
}

void verifyHardwareConfiguration() {
  if (staticMemory.flashChip < kRequiredFlashBytes) {
    stopWithMessage("16 MB flash is required");
  }
  // ESP-IDF exposes usable heap capacity, which can be slightly below the
  // physical 8 MiB after allocator and PSRAM bookkeeping reservations.
  if (!psramFound() ||
      staticMemory.psramTotal <
          kRequiredPsramBytes - kPsramReservedAllowance) {
    stopWithMessage("8 MB OPI PSRAM is required");
  }
  if (display.width() != kPanelWidth || display.height() != kPanelHeight) {
    stopWithMessage("Expected 320x240 landscape mode");
  }
}

void initializeFramePipeline() {
  for (FrameBuffer &frame : frameBuffers) {
    frame.pixels = static_cast<uint16_t *>(heap_caps_malloc(
        kFramebufferBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (frame.pixels == nullptr) {
      stopWithMessage("Could not allocate two PSRAM framebuffers");
    }
    frame.canvas.attach(frame.pixels);
    frame.canvas.fillScreen(TFT_BLACK);
  }

  for (uint16_t *&strip : dmaStrips) {
    strip = static_cast<uint16_t *>(heap_caps_malloc(
        kDmaStripBytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
    if (strip == nullptr) {
      stopWithMessage("Could not allocate two DMA strip buffers");
    }
  }

  freeFrameQueue = xQueueCreate(2, sizeof(FrameBuffer *));
  readyFrameQueue = xQueueCreate(2, sizeof(FrameBuffer *));
  if (freeFrameQueue == nullptr || readyFrameQueue == nullptr) {
    stopWithMessage("Could not create framebuffer queues");
  }

  for (FrameBuffer &frame : frameBuffers) {
    FrameBuffer *framePointer = &frame;
    xQueueSend(freeFrameQueue, &framePointer, portMAX_DELAY);
  }

  serialPrintf("PSRAM framebuffers: 2 x %u bytes OK\n",
               static_cast<unsigned int>(kFramebufferBytes));
  serialPrintf("Internal DMA strips: 2 x %u bytes OK\n",
               static_cast<unsigned int>(kDmaStripBytes));
}

TimingProfile readTimingProfile() {
  return {
      __atomic_load_n(&lastDisplayFrameUs, __ATOMIC_RELAXED),
      __atomic_load_n(&lastDmaWaitUs, __ATOMIC_RELAXED),
      __atomic_load_n(&lastCopySwapUs, __ATOMIC_RELAXED),
      __atomic_load_n(&lastSubmitUs, __ATOMIC_RELAXED),
      __atomic_load_n(&lastFramebufferWaitUs, __ATOMIC_RELAXED),
  };
}

void printMilliseconds(PsramCanvas16 &canvas, const char *label,
                       uint32_t microseconds) {
  canvas.printf("%-9s %3lu.%02lu\n", label,
                static_cast<unsigned long>(microseconds / 1000u),
                static_cast<unsigned long>((microseconds % 1000u) / 10u));
}

void renderMemoryStatus(FrameBuffer &frame) {
  const uint32_t sramAvailable = ESP.getFreeHeap();
  const uint32_t psramAvailable = ESP.getFreePsram();
  const TimingProfile timing = readTimingProfile();
  PsramCanvas16 &canvas = frame.canvas;

  canvas.fillScreen(TFT_BLACK);
  canvas.setCursor(8, 8);
  canvas.setTextSize(2);
  canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
  canvas.println("ESP32-S3 MEMORY");
  canvas.setCursor(216, 8);
  canvas.setTextColor(TFT_GREEN, TFT_BLACK);
  canvas.printf("FPS %lu.%lu",
                static_cast<unsigned long>(measuredFpsTenths / 10u),
                static_cast<unsigned long>(measuredFpsTenths % 10u));
  canvas.drawFastHLine(8, 30, kPanelWidth - 16, TFT_DARKCYAN);
  canvas.setCursor(8, 40);
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.printf("SRAM  %4lu/%4lu KiB\n",
                static_cast<unsigned long>(
                    (staticMemory.sramTotal - sramAvailable) / 1024u),
                static_cast<unsigned long>(sramAvailable / 1024u));
  canvas.printf("PSRAM %4lu/%4lu KiB\n",
                static_cast<unsigned long>(
                    (staticMemory.psramTotal - psramAvailable) / 1024u),
                static_cast<unsigned long>(psramAvailable / 1024u));
  canvas.printf("APP   %4lu/%4lu KiB\n",
                static_cast<unsigned long>(staticMemory.appUsed / 1024u),
                static_cast<unsigned long>(staticMemory.appCapacity / 1024u));
  canvas.printf("FLASH %7lu KiB\n",
                static_cast<unsigned long>(staticMemory.flashChip / 1024u));

  canvas.drawFastHLine(8, 108, kPanelWidth - 16, TFT_DARKCYAN);
  canvas.setCursor(8, 116);
  canvas.setTextColor(TFT_CYAN, TFT_BLACK);
  canvas.println("PROFILE (ms)");
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  printMilliseconds(canvas, "Frame", timing.displayFrameUs);
  printMilliseconds(canvas, "DMA wait", timing.dmaWaitUs);
  printMilliseconds(canvas, "Copy/swap", timing.copySwapUs);
  printMilliseconds(canvas, "Submit", timing.submitUs);
  printMilliseconds(canvas, "Main wait", timing.framebufferWaitUs);
}

void copySwapPixels(uint16_t *destination, const uint16_t *source,
                    size_t pixelCount) {
  for (size_t index = 0; index < pixelCount; ++index) {
    const uint16_t pixel = source[index];
    destination[index] = static_cast<uint16_t>((pixel << 8) | (pixel >> 8));
  }
}

void displayTask(void *) {
  FrameBuffer *frame = nullptr;
  for (;;) {
    if (xQueueReceive(readyFrameQueue, &frame, portMAX_DELAY) != pdTRUE) {
      continue;
    }

    const uint32_t frameStartUs = micros();
    uint32_t dmaWaitUs = 0;
    uint32_t copySwapUs = 0;
    uint32_t submitUs = 0;
    display.startWrite();
    int stripIndex = 0;
    for (int16_t y = 0; y < kPanelHeight; y += kDmaStripHeight) {
      const int16_t height =
          min<int16_t>(kDmaStripHeight, kPanelHeight - y);
      uint16_t *source = frame->pixels + static_cast<size_t>(y) * kPanelWidth;
      uint16_t *strip = dmaStrips[stripIndex];
      const size_t pixelCount = static_cast<size_t>(kPanelWidth) * height;

      uint32_t phaseStartUs = micros();
      copySwapPixels(strip, source, pixelCount);
      copySwapUs += micros() - phaseStartUs;

      phaseStartUs = micros();
      display.dmaWait();
      dmaWaitUs += micros() - phaseStartUs;

      phaseStartUs = micros();
      display.pushImageDMA(0, y, kPanelWidth, height,
                           static_cast<const uint16_t *>(strip));
      submitUs += micros() - phaseStartUs;
      stripIndex ^= 1;
    }

    const uint32_t finalWaitStartUs = micros();
    display.dmaWait();
    dmaWaitUs += micros() - finalWaitStartUs;
    display.endWrite();

    __atomic_store_n(&lastDisplayFrameUs, micros() - frameStartUs,
                     __ATOMIC_RELAXED);
    __atomic_store_n(&lastDmaWaitUs, dmaWaitUs, __ATOMIC_RELAXED);
    __atomic_store_n(&lastCopySwapUs, copySwapUs, __ATOMIC_RELAXED);
    __atomic_store_n(&lastSubmitUs, submitUs, __ATOMIC_RELAXED);

    __atomic_add_fetch(&completedFrames, 1u, __ATOMIC_RELAXED);
    xQueueSend(freeFrameQueue, &frame, portMAX_DELAY);
  }
}

void updateFpsReport() {
  const uint32_t nowMs = millis();
  const uint32_t elapsedMs = nowMs - fpsWindowStartMs;
  if (elapsedMs < kFpsReportIntervalMs) {
    return;
  }

  const uint32_t completed =
      __atomic_load_n(&completedFrames, __ATOMIC_RELAXED);
  const uint32_t frames = completed - completedFramesAtWindowStart;
  measuredFpsTenths = static_cast<uint32_t>(
      (static_cast<uint64_t>(frames) * 10000u + elapsedMs / 2u) / elapsedMs);
  const TimingProfile timing = readTimingProfile();
  serialPrintf("millis(): %lu, DMA FPS: %lu.%lu (%lu frames in %lu ms)\n",
               static_cast<unsigned long>(nowMs),
               static_cast<unsigned long>(measuredFpsTenths / 10u),
               static_cast<unsigned long>(measuredFpsTenths % 10u),
               static_cast<unsigned long>(frames),
               static_cast<unsigned long>(elapsedMs));
  serialPrintf("profile: frame=%lu us, DMA wait=%lu us, copy/swap=%lu us, "
               "submit=%lu us, main wait=%lu us\n",
               static_cast<unsigned long>(timing.displayFrameUs),
               static_cast<unsigned long>(timing.dmaWaitUs),
               static_cast<unsigned long>(timing.copySwapUs),
               static_cast<unsigned long>(timing.submitUs),
               static_cast<unsigned long>(timing.framebufferWaitUs));
  completedFramesAtWindowStart = completed;
  fpsWindowStartMs = nowMs;
}
}  // namespace

void setup() {
  serialBegin(115200);
  delay(1000);

  captureStaticMemoryInfo();
  printHardwareInfo();

  serialPrintln("Initializing ILI9341 on FSPI...");
  display.init();
  serialPrintln("ILI9341 initialization complete");
  display.setRotation(1);
  display.setSwapBytes(true);
  display.fillScreen(TFT_BLACK);
  verifyHardwareConfiguration();

  if (!display.initDMA()) {
    stopWithMessage("Could not initialize TFT SPI DMA");
  }
  initializeFramePipeline();

  const BaseType_t created = xTaskCreatePinnedToCore(
      displayTask, "tft-dma", 4096, nullptr, 2, &displayTaskHandle, 0);
  if (created != pdPASS) {
    stopWithMessage("Could not create TFT DMA task");
  }

  serialPrintln("ILI9341 320x240 asynchronous DMA display started");
  serialPrintln("setup() complete; entering loop()");
  fpsWindowStartMs = millis();
}

void loop() {
  FrameBuffer *frame = nullptr;
  const uint32_t framebufferWaitStartUs = micros();
  if (xQueueReceive(freeFrameQueue, &frame, portMAX_DELAY) == pdTRUE) {
    __atomic_store_n(&lastFramebufferWaitUs,
                     micros() - framebufferWaitStartUs, __ATOMIC_RELAXED);
    renderMemoryStatus(*frame);
    xQueueSend(readyFrameQueue, &frame, portMAX_DELAY);
  }
  updateFpsReport();
}
