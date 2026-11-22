#!/usr/bin/env python3
"""Stage-A helper: Yahboom Voice-ASR-TTS wake + UART hunt.

Does NOT hear the module speaker — print checklist for human ear.
Default wake: Hi Yahboom (not Hi M Five). Frame: AA 55 xx xx FB.
"""

from __future__ import annotations

import argparse
import subprocess
import threading
import time

import serial
from serial.tools import list_ports


def speak(text: str) -> None:
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
    ap.add_argument("--port", default="")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--no-tts", action="store_true")
    args = ap.parse_args()

    ports = list(list_ports.comports())
    print("=== Yahboom Voice-ASR-TTS / power ===", flush=True)
    if not ports:
        print("FAIL: no COM — plug module Type-C (or CH340) to PC USB", flush=True)
        return 1
    for p in ports:
        print(f"  {p.device}: {p.description} [{p.hwid}]", flush=True)

    port = args.port.strip() or ports[0].device
    print(f"Using {port} @{args.baud}", flush=True)
    print("CHECK: red LED on; slide switch = RUN (not flash).", flush=True)

    ser = serial.Serial(port=port, baudrate=args.baud, timeout=0.2)
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
                print(
                    "RX " + " ".join(f"{b:02X}" for b in raw[:40]),
                    flush=True,
                )
            else:
                time.sleep(0.03)

    threading.Thread(target=reader, daemon=True).start()
    time.sleep(0.3)

    if not args.no_tts:
        print("=== TTS near mic; LISTEN to MODULE speaker ===", flush=True)
        for phrase in ("Hi Yahboom", "Hi Yahboom", "Xiao Ya Xiao Ya"):
            print(f"SPEAK: {phrase}", flush=True)
            speak(phrase)
            time.sleep(3.0)
    else:
        print("=== 10s listen (say Hi Yahboom / 小亚小亚 yourself) ===", flush=True)
        time.sleep(10.0)

    stop = True
    time.sleep(0.2)

    # Yahboom: AA 55 mode cmd FB
    y_hits: list[tuple[int, int]] = []
    # M5 leftover: AA 55 ID 55 AA
    m5_hits: list[int] = []
    i = 0
    while i + 4 < len(buf):
        if buf[i] == 0xAA and buf[i + 1] == 0x55 and buf[i + 4] == 0xFB:
            y_hits.append((buf[i + 2], buf[i + 3]))
            i += 5
        elif (
            buf[i] == 0xAA
            and buf[i + 1] == 0x55
            and buf[i + 3] == 0x55
            and buf[i + 4] == 0xAA
        ):
            m5_hits.append(buf[i + 2])
            i += 5
        else:
            i += 1

    print(
        f"--- uart_bytes={len(buf)} yahboom={len(y_hits)} "
        f"frames={[f'{a:02X}:{b:02X}' for a, b in y_hits]} "
        f"m5_legacy={len(m5_hits)} ---",
        flush=True,
    )
    print(
        "HUMAN: Did MODULE speaker answer (I am here / 我在)? YES/NO",
        flush=True,
    )
    print(
        "If NO + red LED OK -> docs/voice-module/固件重烧备忘.md",
        flush=True,
    )
    ser.close()
    return 0 if y_hits else 2


if __name__ == "__main__":
    raise SystemExit(main())
