# IDleaf Smart Plant Round Display Solution

## Product Goal

IDleaf is a 240 x 240 round-screen smart plant companion for a desktop plant. It
keeps a friendly idle sprite on the screen, lets users choose the plant type they
are caring for, evaluates the current growing environment, records recent values
and supports shared care moments from two phones.

## Screen Structure

1. Idle sprite
   - Default screen after boot and after inactivity.
   - The sprite fills most of the circular screen.
   - Touch anywhere to enter the plant application.
   - The expression is driven by sensor state, so the idle screen still feels alive.

2. Status
   - Shows selected plant name and type.
   - Shows current environment score.
   - Shows compact soil, humidity and temperature values.
   - Provides `CHANGE PLANT` entry for the five supported plant types.

3. Plant selection
   - Monstera, Mint, Succulent, Peace Lily and Pothos.
   - Each plant has its own ideal temperature, humidity and soil moisture range.
   - Selecting a plant switches the assessment model immediately.

4. Trend
   - Stores recent readings in an in-memory circular buffer.
   - Presents recent temperature, humidity and soil values.
   - Current implementation keeps history until reboot.

5. Care together
   - Shows care history.
   - Shows connected phone count from the device Wi-Fi AP.
   - A phone can send a heart from `http://192.168.4.1`.
   - A phone can draw a heart or simple picture on the web canvas and send it as
     an 8 x 8 sketch.
   - Received hearts or sketches show a tappable prompt on the round display.
   - Tapping the prompt pops the heart or sketch onto the screen and triggers the
     backlight heartbeat effect.

6. Judge demo
   - Runs through plant selection, status changes, history, care together and
     incoming heart response.
   - Supports interaction during the demo: fake good data, fake dry data, fake
     heart and fake sketch actions are available from the phone page and selected
     screen controls.
   - Can be started from the display care page or the phone web page.

## Sprite States

- `CALM`: default normal state.
- `HAPPY`: environment score is excellent.
- `NEEDS CARE`: environment score is too low.
- `CURIOUS`: no sensor is ready yet, or the idle sprite has been waiting for a while.
- `LOVED`: a phone sends a heart.

The current firmware draws the sprite with LVGL primitive objects instead of a
bitmap, which keeps the asset small and lets expressions switch quickly.

## Sensor Assessment

The firmware calculates a score from available readings:

- Air temperature
- Air humidity
- Soil moisture

Unavailable values are ignored. If no valid sensor values exist, the screen shows
`WAIT SENSOR` behavior and the idle sprite becomes curious. On the current round
display build, `A0`, `D6` and `D7` conflict with display battery/backlight/touch
functions, so soil and ultrasonic sensing are disabled until moved to free pins.

## Firmware Notes

- Target board: Seeed Studio XIAO ESP32S3.
- Display: Seeed XIAO Round Display, GC9A01, 240 x 240.
- UI framework: LVGL with local Seeed round display driver.
- Access point: `XIAO-CAM-TEST-3`.
- Phone control URL: `http://192.168.4.1`.
- The loop task stack is set to 16 KB because LVGL label rendering on this build
  exceeded the Arduino ESP32 default loop stack during refresh.

## Validation

Latest local validation:

- `arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32S3 ...`
- `arduino-cli upload -p /dev/cu.usbmodem2101 ...`
- Serial monitor after upload showed no `Guru Meditation` or reboot loop during
  the observation window.
- Remaining expected warning: `DHT20 not found on I2C` when the DHT20 module is
  not connected or not detected.

## Design Asset

The confirmed flow draft is stored at:

`assets/design/smart_plant_round_flow_v3.svg`
