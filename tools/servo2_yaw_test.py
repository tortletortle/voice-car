#!/usr/bin/env python3
"""SERVO2 定位/摇头最短实机测（bringup 文本命令，经 COM9）。

序列：PING → 左1000 → 中1500 → 右2000 → 摇头(L→R→L→C) → ALL OFF
"""

from __future__ import annotations

import sys
import time

import serial

PORT = "COM9"
BAUD = 115200
HOLD_S = 0.45


def main() -> int:
    ser = serial.Serial()
    ser.port = PORT
    ser.baudrate = BAUD
    ser.timeout = 0.2
    ser.dtr = False
    ser.rts = False
    ser.open()
    ser.dtr = False
    ser.rts = False
    time.sleep(0.3)
    ser.reset_input_buffer()

    def send(cmd: str) -> None:
        print(f">> {cmd}")
        ser.write((cmd + "\r\n").encode("ascii"))
        time.sleep(0.05)
        raw = ser.read(ser.in_waiting or 1)
        if raw:
            print(raw.decode("utf-8", errors="replace").strip())

    try:
        send("PING")
        time.sleep(0.3)
        print("-- 左 1000")
        send("S2 1000")
        time.sleep(HOLD_S)
        print("-- 中 1500")
        send("S2 1500")
        time.sleep(HOLD_S)
        print("-- 右 2000")
        send("S2 2000")
        time.sleep(HOLD_S)
        print("-- 摇头 L→R→L→C")
        for us in (1000, 2000, 1000, 1500):
            send(f"S2 {us}")
            time.sleep(HOLD_S)
        send("ALL OFF")
        print("DONE")
        return 0
    finally:
        try:
            ser.write(b"ALL OFF\r\n")
        except serial.SerialException:
            pass
        ser.close()


if __name__ == "__main__":
    sys.exit(main())
