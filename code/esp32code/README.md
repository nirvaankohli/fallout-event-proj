# ESP32 Echo Link

This folder contains:

- `main.cpp`: Arduino sketch for the ESP32-C3 that echoes lines back over USB serial.
- `laptop_echo.py`: Laptop-side client for USB serial now, plus optional Wi-Fi TCP later.

## What works immediately

With the ESP32 plugged in over USB, the fastest path is serial echo at `115200` baud.

## Optional Wi-Fi mode

If you want wireless echo, set `kWifiSsid` and `kWifiPassword` near the top of `main.cpp`, rebuild, and upload. The ESP32 will start a TCP echo server on port `3333` and print its IP address to serial on boot.

## Laptop setup

```bash
python3 -m pip install pyserial
python3 code/esp32code/laptop_echo.py --mode serial --port /dev/cu.usbmodem101
```

If you omit `--port`, the script will try to auto-detect the ESP32 serial device on macOS.

## Build and upload

From the repo root:

```bash
python3 -m platformio run
python3 -m platformio run --target upload
python3 -m platformio device monitor -b 115200
```
