# Attrax SZ2026 H2O Round Display Firmware

Firmware for Seeed Studio XIAO ESP32S3 with the XIAO Round Display.

The current display path uses LVGL with the local Seeed Round Display driver in `roundsidplay/`.

## Smart Plant Application

The 240 x 240 touch display now starts in an idle sprite mode. The plant companion
fills the round screen and changes expression from sensor state:

- `CURIOUS` when no valid sensor reading is available or when it idles for a while.
- `HAPPY` when the selected plant's environment score is excellent.
- `CALM` for normal conditions.
- `NEEDS CARE` when the environment score drops too low.
- `LOVED` when a paired phone sends a heart.

Touch the idle sprite to enter the application. The app uses a lighter three-tab
layout to avoid text overlap on the round screen:

- `STATUS` shows the selected plant, sprite, suitability score and compact live readings.
- `TREND` shows the latest sampled temperature, humidity and soil readings collected since boot.
- `CARE` shows watering/heart history and how many phones are connected to the device Wi-Fi access point.

Use `CHANGE PLANT` on the status screen to select one of five companions:
Monstera, Mint, Succulent, Peace Lily or Pothos.

The current Round Display wiring occupies `A0`, so soil moisture is displayed as `--`
until the moisture sensor is moved to a free ADC pin and enabled in `pollSensors()`.
To send a heart, connect a phone to the device access point, open `http://192.168.4.1`,
and press `Send heart <3` in the `Care Together` card. The same page also includes
a small drawing pad: users can draw a heart or simple 8 x 8 sketch and send it as
a care gift. The round display shows a tappable notification first; tapping the
screen reveals the heart or drawing as a pop-up and runs the heartbeat backlight
animation.

For judging presentations, press `DEMO` on the display `CARE` page or `Judge demo`
in the phone control page. It demonstrates the five plant choices, sprite
expressions, growth history, shared care history and incoming heart response.
During demo mode, the screen and phone page can also inject fake good/dry sensor
data, fake hearts and fake sketches so judges can interact instead of only watching
an automatic sequence.

See [docs/SMART_PLANT_SOLUTION.md](docs/SMART_PLANT_SOLUTION.md) for the complete
interaction plan and [assets/design/smart_plant_round_flow_v3.svg](assets/design/smart_plant_round_flow_v3.svg)
for the round-screen UI draft.

The history buffer currently lasts until reboot. Authenticated two-account cloud sync
and persistent history storage require a companion service or local flash data model;
they are not implied by two phones joining Wi-Fi.

## Flashing

See [FLASHING.md](FLASHING.md) for Arduino CLI compile and upload commands.

Quick path:

```sh
arduino-cli board list
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32S3 --build-path .arduino-build --libraries roundsidplay/Seeed_GFX-master --libraries roundsidplay/Seeed_Arduino_RoundDisplay-main .
arduino-cli upload -p /dev/cu.usbmodem2101 --fqbn esp32:esp32:XIAO_ESP32S3 --input-dir .arduino-build --verbose
```

Replace `/dev/cu.usbmodem2101` with the connected device port.
