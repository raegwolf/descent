#ifndef DESCENT_ARDUINO_COMPAT_H
#define DESCENT_ARDUINO_COMPAT_H

/*
 * The macOS work established portable C paths that replace DOS assembly and
 * 16-bit compiler behavior. ARDUINO selects those paths while the ESP32 shim
 * supplies its own display, clock, filesystem, and no-device implementations.
 */
#if defined(ARDUINO) && defined(DESCENT_ENGINE_BUILD) && !defined(MACOS)
#define MACOS 1
#endif

#if defined(DESCENT_ENGINE_BUILD)
#define _far
#define far
#define near
#define __far
#define __near
#define __interrupt
#define interrupt
#endif

#endif
