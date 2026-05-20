# Repository Guidelines

## Project Structure & Module Organization

This workspace is a reference bundle under `refrence/` containing two Arduino libraries. `refrence/Seeed_Arduino_RoundDisplay-main/` provides the XIAO Round Display LVGL integration; its public header lives in `src/`, board examples live in `examples/`, and library metadata is in `library.properties`. `refrence/Seeed_GFX-master/` contains the display graphics library; core code is in `TFT_eSPI.cpp` and `TFT_eSPI.h`, with platform backends in `Processors/`, panel drivers in `TFT_Drivers/`, touch drivers in `Touch_Drivers/`, fonts in `Fonts/`, and sketches in `examples/`.

## Build, Test, and Development Commands

Use Arduino tooling from the relevant library directory. Common checks:

```sh
arduino-cli lib install "lvgl" "TFT_eSPI"
arduino-cli compile --fqbn Seeeduino:samd:seeed_XIAO_m0 examples/HardwareTest
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32S3 examples/TFT_eSPI_Clock
```

Run `arduino-cli board listall | rg XIAO` to confirm installed board package names before compiling. For PlatformIO consumers, validate `Seeed_GFX-master` with `pio run` after adding a local `platformio.ini`; its `library.json` declares Arduino framework compatibility.

## Coding Style & Naming Conventions

Follow the existing C/C++ Arduino style. Use two to four spaces consistently with nearby code, braces on their current local pattern, uppercase names for macros such as `SCREEN_WIDTH`, and lower snake case for free functions such as `xiao_disp_init`. Keep hardware pin and display constants near the top of headers. Avoid broad refactors in vendor-derived files unless needed for the change.

## Testing Guidelines

There is no standalone unit test suite. Treat example sketches as compile and hardware smoke tests. Prefer adding or updating focused sketches under `examples/` when changing display, touch, font, or board-specific behavior. At minimum, compile the affected example and one unrelated diagnostic sketch before submitting changes.

## Commit & Pull Request Guidelines

This checkout has no local Git history, so use clear imperative commits such as `Fix LVGL 9 display flush` or `Add XIAO ESP32S3 setup example`. Pull requests should include the affected library path, tested board/FQBN, Arduino IDE or `arduino-cli` version, linked issue if available, and photos or short video for visible display or touch changes.

## Security & Configuration Tips

Do not commit machine-specific Arduino package paths, generated build output, or private Wi-Fi/API credentials in sketches. Keep board-specific setup changes isolated in `User_Setup.h`, `User_Setup_Select.h`, or a dedicated example setup file.
