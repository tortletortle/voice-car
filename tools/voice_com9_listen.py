#!/usr/bin/env python3
"""Listen COM9 for voice: lines from ESP (after firmware with PC mirror).

Usage:
  python tools/voice_com9_listen.py
  python tools/voice_com9_listen.py --seconds 20
"""

from __future__ import annotations

import argparse
import sys
import time

import serial

DEFAULT_PORT = "COM9"
BAUD = 115200


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default=DEFAULT_PORT)
    ap.add_argument("--seconds", type=float, default=15.0)
    args = ap.parse_args()

    ser = serial.Serial()
    ser.port = args.port
    ser.baudrate = BAUD
    ser.timeout = 0.2
    ser.dtr = False
    ser.rts = False
    ser.open()
    ser.dtr = False
    ser.rts = False
    time.sleep(0.2)
    ser.reset_input_buffer()

    print(
        f"Listening {args.port} {args.seconds:.0f}s — speak wake+cmd to mic",
        flush=True,
    )
    print("Expect lines starting with 'voice:' if module wired + new ESP firmware", flush=True)

    t0 = time.time()
    got_voice = False
    total = 0
    while time.time() - t0 < args.seconds:
        n = ser.in_waiting
        if n:
            raw = ser.read(n)
            total += len(raw)
            text = raw.decode("utf-8", errors="replace")
            sys.stdout.write(text)
            sys.stdout.flush()
            if "voice:" in text:
                got_voice = True
        else:
            time.sleep(0.05)

    print(f"\n--- done bytes={total} voice_tag={'YES' if got_voice else 'NO'} ---")
    ser.close()
    return 0 if got_voice else 2


if __name__ == "__main__":
    sys.exit(main())
