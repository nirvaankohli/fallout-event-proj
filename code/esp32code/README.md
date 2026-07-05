# XIAO ESP32-C3 Motor Controller

This folder contains the firmware and browser controller for the DRV8833-driven dual-motor setup.

- `main.cpp`: XIAO ESP32-C3 firmware with Wi-Fi, WebSocket control, serial test commands, and safety timeout logic.
- `web_controller.html`: Browser UI with two vertical joysticks for left/right tank drive.
- `laptop_echo.py`: Legacy serial/TCP helper from the earlier echo phase. Serial mode still remains useful for quick manual commands, but the main UI is now `web_controller.html`.

## Pin mapping

The firmware follows the schematic labels exactly:

- `LeftDriveIn1` -> GPIO2 / D0
- `LeftDriveIn2` -> GPIO3 / D1
- `RightDriveIn1` -> GPIO4 / D2
- `RightDriveIn2` -> GPIO5 / D3
- `Activity LED` -> GPIO6 / D4
- `Sleep` -> GPIO10 / D10

DRV8833 drive behavior:

- Forward: `IN1=HIGH`, `IN2=LOW`
- Reverse: `IN1=LOW`, `IN2=HIGH`
- Stop/coast: both inputs `LOW`

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

- The DRV8833 `SLEEP` pin is driven high only while a motor command is active.
- GPIO6 is driven high at the same time, so an external LED can indicate active joystick motion.
- Invalid WebSocket payloads force a safe stop.
- If joystick updates stop arriving for about `300 ms`, both motors are stopped automatically.
- If the WebSocket client disconnects, both motors are stopped automatically.

## Browser controller

Open [web_controller.html](/Users/nirvaank/Code/Hardware/fallout/event/project/fallout-event-proj/code/esp32code/web_controller.html) in a browser.

- Left joystick drives only the left motor.
- Right joystick drives only the right motor.
- Up is forward, down is reverse.
- Releasing a joystick returns that motor to zero.
- The page shows connection state and the last status reason returned by the XIAO.

## Build and upload

From the repo root:

```bash
python3 -m platformio run
python3 -m platformio run --target upload --upload-port /dev/cu.usbmodem101
python3 -m platformio device monitor -b 115200
```
