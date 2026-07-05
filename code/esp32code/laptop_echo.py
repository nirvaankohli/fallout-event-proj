#!/usr/bin/env python3
"""Simple laptop client for the ESP32 USB serial or Wi-Fi echo server."""

from __future__ import annotations

import argparse
import glob
import socket
import sys
import time

def auto_detect_serial_port() -> str:
    candidates = sorted(
        glob.glob("/dev/cu.usbmodem*")
        + glob.glob("/dev/cu.SLAB_USBtoUART*")
        + glob.glob("/dev/cu.wchusbserial*")
    )
    if not candidates:
        raise SystemExit("No likely ESP32 serial device found. Pass --port explicitly.")
    return candidates[0]


def open_serial_client(port: str, baud: int):
    try:
        import serial
    except ImportError as exc:  # pragma: no cover - user environment dependent
        raise SystemExit(
            "pyserial is required for serial mode. Install it with: python3 -m pip install pyserial"
        ) from exc

    device = serial.Serial(port=port, baudrate=baud, timeout=1)
    time.sleep(2)
    device.reset_input_buffer()
    return device


def run_serial_mode(port: str | None, baud: int) -> int:
    resolved_port = port or auto_detect_serial_port()
    print(f"Opening serial connection on {resolved_port} at {baud} baud")
    device = open_serial_client(resolved_port, baud)
    print("Type text and press Enter. Ctrl+C to quit.")
    print("Try: ping")

    try:
        while True:
            outgoing = input("> ").strip()
            if not outgoing:
                continue

            device.write((outgoing + "\n").encode("utf-8"))
            device.flush()

            incoming = device.readline().decode("utf-8", errors="replace").strip()
            if incoming:
                print(f"ESP32: {incoming}")
            else:
                print("ESP32: no response before timeout")
    except KeyboardInterrupt:
        print("\nClosing serial connection")
        return 0
    finally:
        device.close()


def run_tcp_mode(host: str, port: int) -> int:
    print(f"Opening TCP connection to {host}:{port}")
    try:
        with socket.create_connection((host, port), timeout=5) as sock:
            reader = sock.makefile("r", encoding="utf-8", newline="\n")
            print("Type text and press Enter. Ctrl+C to quit.")
            try:
                while True:
                    outgoing = input("> ").strip()
                    if not outgoing:
                        continue

                    sock.sendall((outgoing + "\n").encode("utf-8"))
                    incoming = reader.readline().strip()
                    if incoming:
                        print(f"ESP32: {incoming}")
                    else:
                        print("ESP32: connection closed")
                        return 1
            except KeyboardInterrupt:
                print("\nClosing TCP connection")
                return 0
    except OSError as exc:
        if exc.errno == 51:
            print("Network is unreachable.")
            print("Join the ESP32 Wi-Fi network 'Fallout-ESP32-Echo' first, then retry.")
            print(f"After joining, connect again to {host}:{port}.")
            return 2

        raise


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--mode",
        choices=("serial", "tcp"),
        default="serial",
        help="Communication mode to use.",
    )
    parser.add_argument(
        "--port",
        help="Serial device path for --mode serial, like /dev/cu.usbmodem101.",
    )
    parser.add_argument(
        "--baud",
        type=int,
        default=115200,
        help="Serial baud rate.",
    )
    parser.add_argument(
        "--host",
        default="192.168.4.1",
        help="ESP32 IP address for --mode tcp.",
    )
    parser.add_argument(
        "--tcp-port",
        type=int,
        default=3333,
        help="ESP32 TCP port for --mode tcp.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    if args.mode == "serial":
        return run_serial_mode(args.port, args.baud)

    return run_tcp_mode(args.host, args.tcp_port)


if __name__ == "__main__":
    sys.exit(main())
