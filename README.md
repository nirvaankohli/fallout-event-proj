# fallout-event-proj

This repo now contains a two-board Arduino setup for ESP32-C3 devices:

- `code/laptop-xiao-esp32c3`
  The XIAO ESP32-C3 connected to the laptop over USB. It reads a line from the computer on `Serial` and broadcasts it over ESP-NOW.
- `code/onboard-xiao-esp32c3`
  The onboard Seeed XIAO ESP32-C3. It receives ESP-NOW messages and turns an LED on for 1 second.

## Hardware topology

The communication path is:

`Computer -> USB Serial -> laptop-connected XIAO ESP32-C3 -> ESP-NOW -> onboard XIAO ESP32-C3`

### Real hardware wiring

- No data wire is needed between the two XIAO boards for ESP-NOW.
- Give both boards power.
- Connect an LED and resistor to the onboard XIAO:
  - `D10 -> resistor -> LED anode`
  - `LED cathode -> GND`

If both boards are powered by USB, that is enough for this example.

## Commands

Open the serial monitor for the laptop-connected XIAO at `115200` baud and send one line at a time:

- `hello`
- `open door`
- `blink now`
- any other short line

Every line becomes one ESP-NOW packet. The onboard XIAO receives it and turns the LED on for `1000ms`.

## Build with PlatformIO

This repo uses one root `platformio.ini` with two environments:

- `laptop_xiao_esp32c3`
- `onboard_xiao_esp32c3`

Build them with:

```bash
.venv/bin/platformio run -e laptop_xiao_esp32c3
.venv/bin/platformio run -e onboard_xiao_esp32c3
```

## Wokwi simulation

The included [diagram.json](/Users/nirvaank/Code/Hardware/fallout/event/project/fallout-event-proj/diagram.json) simulates both boards together:

- XIAO ESP32-C3 as the laptop sender
- XIAO ESP32-C3 as the onboard receiver
- a green LED on onboard XIAO `D10` to show the 1-second blink

Files used by the simulation:

- [diagram.json](/Users/nirvaank/Code/Hardware/fallout/event/project/fallout-event-proj/diagram.json)
- [wokwi-laptop-xiao.ino](/Users/nirvaank/Code/Hardware/fallout/event/project/fallout-event-proj/wokwi-laptop-xiao.ino)
- [wokwi-xiao-onboard.ino](/Users/nirvaank/Code/Hardware/fallout/event/project/fallout-event-proj/wokwi-xiao-onboard.ino)

`wokwi.toml` is included for local Wokwi/VS Code setups that expect a root configuration file. The multi-board `diagram.json` itself uses per-board code assignments so each board runs its own sketch.
