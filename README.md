# fallout-event-proj

This repo now contains a single bare-bones Arduino sketch with no ESP32, WiFi, or multi-board code.

## Behavior

- Starts `Serial` at `115200`
- Prints a short startup message
- Blinks the built-in LED once per second
- Echoes anything you type into the serial monitor
- If you send `blink`, it does one extra quick blink

## Build with PlatformIO

The default PlatformIO environment is `uno`:

```bash
.venv/bin/platformio run
.venv/bin/platformio device monitor -b 115200
```

If you want a different Arduino-compatible board, update `board` in [platformio.ini](/Users/nirvaank/Code/Hardware/fallout/event/project/fallout-event-proj/platformio.ini).
