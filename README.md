# Attrax SZ2026 H2O Round Display Firmware

Firmware for Seeed Studio XIAO ESP32S3 with the XIAO Round Display.

The current display path uses LVGL with the local Seeed Round Display driver in `roundsidplay/`.

## Flashing

See [FLASHING.md](FLASHING.md) for Arduino CLI compile and upload commands.

Quick path:

```sh
arduino-cli board list
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32S3 --build-path .arduino-build --libraries roundsidplay/Seeed_GFX-master --libraries roundsidplay/Seeed_Arduino_RoundDisplay-main .
arduino-cli upload -p /dev/cu.usbmodem2101 --fqbn esp32:esp32:XIAO_ESP32S3 --input-dir .arduino-build --verbose
```

Replace `/dev/cu.usbmodem2101` with the connected device port.
