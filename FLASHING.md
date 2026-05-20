# Round Display Firmware Flashing

This project is an Arduino CLI compatible firmware for Seeed Studio XIAO ESP32S3 with the XIAO Round Display.

## Requirements

- Arduino CLI installed.
- ESP32 board package installed in Arduino CLI.
- Required Arduino libraries installed:
  - `lvgl`
  - `Seeed_GFX`
  - `Seeed Arduino Round display`
  - `NimBLE-Arduino`
  - `U8g2`

Install common libraries:

```sh
arduino-cli lib install "lvgl" "NimBLE-Arduino" "U8g2"
```

Install the ESP32 board package if needed:

```sh
arduino-cli core update-index
arduino-cli core install esp32:esp32
```

## Check Connected Device

Connect the XIAO ESP32S3 over USB, then list boards:

```sh
arduino-cli board list
```

Look for a USB serial port like:

```text
/dev/cu.usbmodem2101
```

On macOS the port usually starts with `/dev/cu.usbmodem`. On Linux it is often `/dev/ttyACM0` or `/dev/ttyUSB0`.

## Compile

From the repository root:

```sh
arduino-cli compile \
  --fqbn esp32:esp32:XIAO_ESP32S3 \
  --build-path .arduino-build \
  --libraries roundsidplay/Seeed_GFX-master \
  --libraries roundsidplay/Seeed_Arduino_RoundDisplay-main \
  .
```

Expected current firmware size is about:

```text
Sketch uses 1402659 bytes (41%) of program storage space.
Global variables use 116508 bytes (35%) of dynamic memory.
```

## Upload

Replace `/dev/cu.usbmodem2101` with the port shown by `arduino-cli board list`:

```sh
arduino-cli upload \
  -p /dev/cu.usbmodem2101 \
  --fqbn esp32:esp32:XIAO_ESP32S3 \
  --input-dir .arduino-build \
  --verbose
```

The upload is successful when the output ends with:

```text
Hash of data verified.
Hard resetting via RTS pin...
```

## Notes

- `Round.ino` is the Arduino CLI entry file because Arduino sketches require the main `.ino` file name to match the directory name.
- `Attrax-SZ2026-H2O.ino` is kept as the original migrated firmware entry reference.
- `lv_conf.h` in the repository root pins LVGL to the local Seeed Round Display configuration.
- `build_opt.h` defines `ATTRAX_ROUND_DISPLAY_PORT` and `LV_CONF_INCLUDE_SIMPLE` for Arduino CLI builds.
- Build output in `.arduino-build/` is ignored and should not be committed.
