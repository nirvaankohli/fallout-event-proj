# XIAO ESP32-C3 Motor Controller

This folder contains the firmware and browser controller for a four-motor tank-drive setup built from two DRV8833 drivers.

- `main.cpp`: XIAO ESP32-C3 firmware with Wi-Fi, WebSocket control, serial test commands, and safety timeout logic.
- `web_controller.html`: Browser UI with two vertical joysticks for left/right tank drive.
- `laptop_echo.py`: Legacy serial/TCP helper from the earlier echo phase. Serial mode still remains useful for quick manual commands, but the main UI is now `web_controller.html`.

## Pin mapping

This firmware fans each left/right drive command out to two motors per side using the bench-tested wiring from the temporary motor-spin sketch:

- `Left front motor IN1` -> `GPIO2`
- `Left front motor IN2` -> `GPIO3`
- `Right front motor IN1` -> `GPIO4`
- `Right front motor IN2` -> `GPIO5`
- `Left rear motor IN1` -> `GPIO20`
- `Left rear motor IN2` -> `GPIO8`
- `Right rear motor IN1` -> `GPIO9`
- `Right rear motor IN2` -> `GPIO10`

Each side is controlled independently:

- Left joystick/command drives both left motors together.
- Right joystick/command drives both right motors together.

DRV8833 drive behavior for each motor channel:

- Forward: `IN1=HIGH`, `IN2=LOW`
- Reverse: `IN1=LOW`, `IN2=HIGH`
- Stop/coast: both inputs `LOW`

Command magnitude still uses `-100..100`, but the current firmware treats any non-zero value as a full-speed direction command.

## Wi-Fi flow

Preferred mode:

1. Set `kWifiSsid` and `kWifiPassword` in [main.cpp](/Users/nirvaank/Code/Hardware/fallout/event/project/fallout-event-proj/code/esp32code/main.cpp).
2. Build and upload the firmware.
3. Read the XIAO IP from the serial monitor.
4. Open `web_controller.html` in a browser and point it at `ws://XIAO_IP:3333/`.

Fallback AP mode:

- SSID: `FalloutESP32`
- Password: none
- XIAO IP: `192.168.4.1`
- WebSocket URL: `ws://192.168.4.1:3333/`

If station Wi-Fi fails, the firmware automatically starts the fallback access point.

## WebSocket command format

Browser clients send JSON messages:

```json
{"type":"drive","left":42,"right":-30}
{"type":"stop"}
{"type":"ping"}
```

The XIAO replies with JSON status or error messages:

```json
{"type":"status","connected":true,"left":42,"right":-30,"reason":"drive"}
{"type":"error","message":"invalid json"}
```

`left` and `right` are normalized to `-100..100`.

## Serial test commands

USB serial still works at `115200` baud and is useful for bench tests:

```text
ping
wifi status
drive 100 100
drive 100 -100
stop
help
```

## Safety stop

- The firmware stops motors by driving all H-bridge inputs `LOW`.
- `SLEEP` is not MCU-controlled in firmware.
- Invalid WebSocket payloads force a safe stop.
- If joystick updates stop arriving for about `300 ms`, all four motors are stopped automatically.
- If the WebSocket client disconnects, all four motors are stopped automatically.

## Startup behavior

- On boot, all four motors spin forward together for about `0.5 s`.
- After that startup check, the firmware stops the motors and starts Wi-Fi/WebSocket control.

## Browser controller

Open [web_controller.html](/Users/nirvaank/Code/Hardware/fallout/event/project/fallout-event-proj/code/esp32code/web_controller.html) in a browser.

- Left pad controls forward and reverse drive.
- Right pad controls turning.
- Up is forward, down is reverse.
- Releasing a joystick returns that motor to zero.
- The page resends the held drive command about every `33 ms` to keep response feeling tight.
- The page shows connection state and the last status reason returned by the XIAO.

## Build and upload

From the repo root:

```bash
python3 -m platformio run
python3 -m platformio run --target upload --upload-port /dev/cu.usbmodem101
python3 -m platformio device monitor -b 115200
```
