#!/usr/bin/env python3
"""PC TTS speaks Yahboom wake/cmd; listen COM for AA 55 .. FB frames."""

from __future__ import annotations

import argparse
import subprocess
import threading
import time

import serial


def speak(text: str) -> None:
    # Escape for PowerShell single-quoted string
    safe = text.replace("'", "''")
    ps = (
        "Add-Type -AssemblyName System.Speech; "
        "$s=New-Object System.Speech.Synthesis.SpeechSynthesizer; "
        "$s.Rate=-2; $s.Volume=100; "
        f"$s.Speak('{safe}')"
    )
    subprocess.run(
        ["powershell", "-NoProfile", "-Command", ps],
        check=False,
        capture_output=True,
    )


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default="COM10")
    ap.add_argument("--baud", type=int, default=115200)
    args = ap.parse_args()

    ser = serial.Serial(port=args.port, baudrate=args.baud, timeout=0.2)
    ser.dtr = False
    ser.rts = False
    time.sleep(0.2)
    ser.reset_input_buffer()

    buf = bytearray()
    stop = False

    def reader() -> None:
        while not stop:
            n = ser.in_waiting
            if n:
                raw = ser.read(n)
                buf.extend(raw)
                hx = " ".join(f"{b:02X}" for b in raw[:48])
                print(f"RX {hx}", flush=True)
            else:
                time.sleep(0.03)

    print(f"=== TTS test on {args.port} @{args.baud} ===", flush=True)
    print("PC will speak; put speaker near module mic.", flush=True)
    threading.Thread(target=reader, daemon=True).start()
    time.sleep(0.4)

    phrases = ["Hi Yahboom", "Hi Yahboom", "Car Forward", "Car Stop"]
    for p in phrases:
        print(f"SPEAK: {p}", flush=True)
        speak(p)
        time.sleep(2.8)

    stop = True
    time.sleep(0.3)

    hits: list[tuple[int, int]] = []
    i = 0
    while i + 4 < len(buf):
        if buf[i] == 0xAA and buf[i + 1] == 0x55 and buf[i + 4] == 0xFB:
            hits.append((buf[i + 2], buf[i + 3]))
            i += 5
        else:
            i += 1

    print(
        f"--- bytes={len(buf)} yahboom={len(hits)} "
        f"frames={[f'{a:02X}:{b:02X}' for a, b in hits]} ---",
        flush=True,
    )
    if hits:
        print("RESULT: OK got Yahboom frames", flush=True)
        code = 0
    elif buf:
        print("RESULT: FAIL noise only (no AA55..FB)", flush=True)
        code = 2
    else:
        print("RESULT: FAIL silent", flush=True)
        code = 3

    ser.close()
    return code


if __name__ == "__main__":
    raise SystemExit(main())
