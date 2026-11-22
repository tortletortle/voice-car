#!/usr/bin/env python3
"""经 ESP32 CH343 注入 VC 命令，验收 App 舵机动画（不依赖 ASR）。

前提：
  - STM32 烧的是 App（解析 AA 55 帧），不是 bringup 文本台
  - ESP32 已烧含 VC 注入的固件，COM9 桥正常
  - VC_SERVO_OUTPUT_ENABLE=1；异常立刻 ALL OFF 无效时请断电

用法：
  python tools/gimbal_vc_test.py
  python tools/gimbal_vc_test.py --port COM9 --only nod
"""

from __future__ import annotations

import argparse
import sys
import time

import serial

DEFAULT_PORT = "COM9"
BAUD = 115200
HOLD_S = 1.6  # 点头/摇头整段约 3×450ms，留余量


def send(ser: serial.Serial, line: str) -> None:
    print(f">> {line}")
    ser.write((line + "\r\n").encode("ascii"))
    time.sleep(0.08)
    raw = ser.read(ser.in_waiting or 1)
    if raw:
        print(raw.decode("utf-8", errors="replace").strip())


def main() -> int:
    ap = argparse.ArgumentParser(description="VC gimbal inject test")
    ap.add_argument("--port", default=DEFAULT_PORT)
    ap.add_argument(
        "--only",
        choices=("all", "up", "down", "nod", "shake"),
        default="all",
    )
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
    time.sleep(0.3)
    ser.reset_input_buffer()

    seq: list[tuple[str, str, float]] = []
    if args.only in ("all", "up"):
        seq.append(("抬头", "VC UP", 0.8))
    if args.only in ("all", "down"):
        seq.append(("低头", "VC DOWN", 0.8))
    if args.only in ("all", "nod"):
        seq.append(("点头 低头→抬头→回中", "VC NOD", HOLD_S))
    if args.only in ("all", "shake"):
        seq.append(("摇头 左→右→左→中", "VC SHAKE", HOLD_S))

    try:
        for title, cmd, wait_s in seq:
            print(f"-- {title}")
            send(ser, cmd)
            time.sleep(wait_s)
        send(ser, "VC PCEN")
        send(ser, "VC YCTR")
        print("DONE")
        return 0
    finally:
        try:
            send(ser, "VC PCEN")
            send(ser, "VC YCTR")
        except serial.SerialException:
            pass
        ser.close()


if __name__ == "__main__":
    sys.exit(main())
