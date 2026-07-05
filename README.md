# fallout-event-proj

This repo now targets a `seeed_xiao_esp32c3` motor-control build for the Fallout event hardware.

## Current behavior

- Connects the XIAO ESP32-C3 to Wi‑Fi or starts a fallback access point
- Exposes a WebSocket motor-control endpoint on port `3333`
- Drives a DRV8833 dual H-bridge using the schematic pin mapping
- Supports tank-drive commands where left and right values control the left and right motors independently
- Includes a browser controller page with two virtual joysticks
- Keeps a serial command interface for quick testing and diagnostics
- Auto-stops the motors on disconnect, invalid payload, or command timeout

## Main files

- Firmware: [code/esp32code/main.cpp](/Users/nirvaank/Code/Hardware/fallout/event/project/fallout-event-proj/code/esp32code/main.cpp)
- Browser controller: [code/esp32code/web_controller.html](/Users/nirvaank/Code/Hardware/fallout/event/project/fallout-event-proj/code/esp32code/web_controller.html)
- Firmware docs: [code/esp32code/README.md](/Users/nirvaank/Code/Hardware/fallout/event/project/fallout-event-proj/code/esp32code/README.md)

## Build with PlatformIO

```bash
python3 -m platformio run
python3 -m platformio run --target upload --upload-port /dev/cu.usbmodem101
python3 -m platformio device monitor -b 115200
```

Board and libraries are defined in [platformio.ini](/Users/nirvaank/Code/Hardware/fallout/event/project/fallout-event-proj/platformio.ini).
