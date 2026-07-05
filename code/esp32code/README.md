# ESP32 Echo Link

This folder contains:

- `main.cpp`: Arduino sketch for the ESP32-C3 that echoes lines back over USB serial and Wi-Fi.
- `laptop_echo.py`: Laptop-side client for USB serial or Wi-Fi TCP.

## Recommended wireless mode

The easiest setup is to put the ESP32 on the same Wi-Fi network as your laptop.

Set these near the top of `main.cpp`:

- `kWifiSsid`
- `kWifiPassword`

Then rebuild and upload. On boot, the ESP32 will connect to that Wi-Fi network and print its IP address over USB serial.

After that, run:

```bash
python3 code/esp32code/laptop_echo.py --mode tcp --host ESP32_IP_HERE --tcp-port 3333
```

## Fallback access point mode

If `kWifiSsid` is left blank or the Wi-Fi connection fails, the ESP32 creates its own Wi-Fi network:

- SSID: `FalloutESP32`
- Password: none
- ESP32 IP: `192.168.4.1`
- TCP port: `3333`

Connect your laptop to that Wi-Fi network, then run:

```bash
python3 code/esp32code/laptop_echo.py --mode tcp --host 192.168.4.1 --tcp-port 3333
```

Type `ping` or any message and the ESP32 will echo it back.

## USB serial mode

With the ESP32 plugged in over USB, serial echo still works at `115200` baud.

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
python3 -m platformio run --target upload --upload-port /dev/cu.usbmodem101
python3 -m platformio device monitor -b 115200
```
