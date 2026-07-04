# fallout-event-proj

This repo contains a two-board wireless link for Seeed XIAO ESP32-C3 devices:

- `code/laptop-xiao-esp32c3`
  The XIAO connected to the laptop over USB. It starts a private Wi-Fi access point, reads lines from `Serial`, and sends each line to the onboard XIAO over UDP.
- `code/onboard-xiao-esp32c3`
  The onboard XIAO. It joins the laptop XIAO's private Wi-Fi network, receives UDP messages, and sends an ACK line back for each one.

## Hardware topology

The working communication path is:

`Computer -> USB Serial -> laptop-connected XIAO -> private Wi-Fi link -> onboard XIAO -> ACK back over Wi-Fi`

No data wire is needed between the two boards.

## Network details

- SSID: `fallout-xiao-link`
- Password: `fallout123`
- Laptop XIAO AP IP: `192.168.4.1`
- Onboard XIAO STA IP: `192.168.4.2`
- Command port: `4210`
- ACK port: `4211`

## Commands

Open the serial monitor for the laptop-connected XIAO at `115200` baud and send one line at a time:

- `hello`
- `status`
- `open door`

Expected result:

- laptop XIAO prints `[SEND #...]`
- onboard XIAO prints `[RECV #...]`
- laptop XIAO prints `[ACK] message #... received: ...`

## Build with PlatformIO

This repo uses one root `platformio.ini` with two environments:

- `laptop_xiao_esp32c3`
- `onboard_xiao_esp32c3`

Build them with:

```bash
.venv/bin/platformio run -e laptop_xiao_esp32c3
.venv/bin/platformio run -e onboard_xiao_esp32c3
```
