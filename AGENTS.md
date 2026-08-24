# Descent Port

This repository contains the released source code for Descent 1 version 1.5,
the 1990s game by Parallax Software. The original code targets 32-bit DOS,
Watcom C/C++, MASM, VGA hardware, and DOS interrupt-driven input and timing.
Read `README.TXT` before changing code: the source is licensed only for
non-commercial, royalty-free use.

## Porting order

1. Port Descent to macOS. Use SDL as the platform boundary and preserve the
   original 320x200 indexed-color rendering model. The first milestone is a
   native `.app` that reaches a running, keyboard-controlled main menu.
2. After the macOS port is working, port the portable core to Arduino ESP32.
   Keep platform-independent code free of Cocoa, Objective-C, and direct SDL
   dependencies so that the SDL boundary can later be replaced by an ESP32
   display/input boundary.

## macOS boundary

- Put macOS-only code, build files, downloaded dependencies, and the SDL shim
  under the top-level `macos/` directory.
- Code outside `macos/` must not include SDL headers. The shim owns window,
  event, timing, and framebuffer presentation calls.
- Prefer adding adapters and portable sibling implementations over editing the
  DOS sources. Preserve the Watcom build whenever a small `#ifdef` is enough.
- Define `MACOS=1` in the macOS build. When existing behavior needs a platform
  adaptation, keep it in the existing source behind `#if defined(MACOS)` and
  leave the historical implementation in the `#else` branch.
- Do not silently recreate original game assets. Descent data files are not in
  this source release; use clearly identified fallback presentation only for a
  bring-up milestone, then load user-supplied game data through the original
  resource paths.

## Assembly replacement policy

- Portable builds must explicitly list C sources and must never glob or compile
  `.ASM` files.
- Replace x86 assembly with a sibling C file in the same subsystem, using the
  `_C.C` suffix (for example `FIX/FIX.ASM` -> `FIX/FIX_C.C`).
- Keep the original assembly file unchanged. At the function it replaces, add
  a comment block containing the relevant original instructions or algorithm
  labels, the register/input contract, and the C equivalence being implemented.
  The goal is to let another person validate the replacement independently.
- Preserve fixed-width DOS semantics. In particular, do not assume that C
  `long` is 32 bits on macOS. Avoid undefined signed overflow and document any
  intentional wrap, saturation, rounding, or divide-by-zero behavior.
- Prefer readable reference C over clever compiler intrinsics. The same code
  should be suitable for ESP32 unless measurement later proves otherwise.

## Change discipline

- Change as little original code as possible. Do not reformat historical files
  or clean up unrelated warnings while porting a subsystem.
- Keep each milestone buildable. Add focused tests for assembly replacements,
  menu models, indexed framebuffer operations, parsers, and math routines where
  practical.
- Test both normal and boundary values, especially negative fixed-point values,
  reversed drawing endpoints, overflows, and null optional outputs.
- Build and test the macOS milestone with `make -C macos test app`. Use
  `make -C macos smoke` for a headless SDL initialization/render check.
