#!/usr/bin/env python3
"""SERVO1/SERVO2 脉宽可视化手调（经 ESP32 CH343 转发到 STM32）。

依赖：pip install pyserial
默认口：COM9 @ 115200

命令：
  S1 <0..19999> / S1 OFF
  S2 <0..19999> / S2 OFF
  ALL OFF
  PING

动作按钮（GUI 本地序列，非 App 协议；须 STM32 bringup 文本台）：
  点头 = S1：低头1000 → 抬头500 → 回中750（与 App CMD_GIMBAL_PITCH_NOD 一致）
  摇头 = S2：左1000 → 右2000 → 左1000 → 中1500

App 二进制验收请用：tools/gimbal_vc_test.py（发 VC NOD / VC SHAKE）
"""

from __future__ import annotations

import queue
import sys
import threading
import tkinter as tk
from tkinter import messagebox, ttk

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    print("缺少 pyserial：pip install pyserial")
    sys.exit(1)

DEFAULT_PORT = "COM9"
BAUD = 115200
PULSE_MIN = 0
PULSE_MAX = 19999  # TIM2 ARR，20 ms 周期内脉宽上限
MOTION_HOLD_MS = 900

# SERVO1 俯仰扫角（µs），结束后单独回中
NOD_SEQ = (1000, 500)
NOD_CENTER_US = 750
# SERVO2 偏航扫角（µs）：左→右→左，结束后单独回中 1500（回中不混在扫角里）
SHAKE_SEQ = (1000, 2000, 1000)
SHAKE_CENTER_US = 1500
FINAL_CENTER_HOLD_MS = 1200


class ServoTuneApp:
    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.root.title("Voice Car · SERVO1/2 手调")
        self.root.geometry("560x520")
        self.root.minsize(520, 480)

        self.ser: serial.Serial | None = None
        self._stop = threading.Event()
        self._reader: threading.Thread | None = None
        self.pulse = tk.IntVar(value=1500)
        self.ch = tk.StringVar(value="1")
        self.port_var = tk.StringVar(value=DEFAULT_PORT)
        self.status_var = tk.StringVar(value="未连接")
        self.log_q: queue.Queue[str] = queue.Queue()
        self._motion_token = 0
        self._motion_after: str | None = None

        self._build()
        self.root.protocol("WM_DELETE_WINDOW", self._on_close)
        self.root.after(100, self._drain_log)
        self._refresh_ports()
        self.root.after(400, self._connect)

    def _build(self) -> None:
        top = ttk.Frame(self.root, padding=10)
        top.pack(fill=tk.X)

        ttk.Label(top, text="串口").grid(row=0, column=0, sticky=tk.W)
        self.port_box = ttk.Combobox(top, textvariable=self.port_var, width=12)
        self.port_box.grid(row=0, column=1, padx=4)
        ttk.Button(top, text="刷新", command=self._refresh_ports).grid(row=0, column=2)
        ttk.Button(top, text="连接", command=self._connect).grid(row=0, column=3, padx=4)
        ttk.Button(top, text="断开", command=self._disconnect).grid(row=0, column=4)
        ttk.Label(top, textvariable=self.status_var).grid(row=0, column=5, padx=8)

        mid = ttk.LabelFrame(self.root, text="脉宽手调（µs）", padding=12)
        mid.pack(fill=tk.X, padx=10, pady=8)

        chrow = ttk.Frame(mid)
        chrow.pack(fill=tk.X, pady=(0, 6))
        ttk.Label(chrow, text="通道").pack(side=tk.LEFT)
        ttk.Radiobutton(chrow, text="舵机1 PA0", value="1", variable=self.ch).pack(
            side=tk.LEFT, padx=8
        )
        ttk.Radiobutton(chrow, text="舵机2 PA1", value="2", variable=self.ch).pack(
            side=tk.LEFT, padx=8
        )

        self.value_lbl = ttk.Label(mid, text="S1 1500 µs", font=("Segoe UI", 18))
        self.value_lbl.pack()

        self.scale = ttk.Scale(
            mid,
            from_=PULSE_MIN,
            to=PULSE_MAX,
            orient=tk.HORIZONTAL,
            command=self._on_slide,
        )
        self.scale.set(1500)
        self.scale.pack(fill=tk.X, pady=8)
        self.scale.bind("<ButtonRelease-1>", lambda _e: self._apply())

        row = ttk.Frame(mid)
        row.pack(fill=tk.X)
        for label, us in (
            ("抬头500", 500),
            ("中750", 750),
            ("低头1000", 1000),
            ("1500", 1500),
            ("0", 0),
            ("max", PULSE_MAX),
        ):
            ttk.Button(row, text=label, width=8, command=lambda v=us: self._set_pulse(v)).pack(
                side=tk.LEFT, padx=2
            )

        btns = ttk.Frame(mid)
        btns.pack(fill=tk.X, pady=8)
        ttk.Button(btns, text="应用当前通道", command=self._apply).pack(side=tk.LEFT, padx=3)
        ttk.Button(btns, text="关当前通道", command=self._off).pack(side=tk.LEFT, padx=3)
        ttk.Button(btns, text="两路全关", command=self._all_off).pack(side=tk.LEFT, padx=3)
        ttk.Button(btns, text="PING", command=self._ping).pack(side=tk.LEFT, padx=3)
        ttk.Button(btns, text="-10", command=lambda: self._nudge(-10)).pack(side=tk.LEFT, padx=3)
        ttk.Button(btns, text="+10", command=lambda: self._nudge(10)).pack(side=tk.LEFT, padx=3)
        ttk.Button(btns, text="-50", command=lambda: self._nudge(-50)).pack(side=tk.LEFT, padx=3)
        ttk.Button(btns, text="+50", command=lambda: self._nudge(50)).pack(side=tk.LEFT, padx=3)

        act = ttk.LabelFrame(self.root, text="动作（点头=S1 俯仰 / 摇头=S2 偏航）", padding=10)
        act.pack(fill=tk.X, padx=10, pady=(0, 8))
        act_row = ttk.Frame(act)
        act_row.pack(fill=tk.X)
        ttk.Button(act_row, text="点头", width=10, command=self._nod).pack(side=tk.LEFT, padx=4)
        ttk.Button(act_row, text="摇头", width=10, command=self._shake).pack(side=tk.LEFT, padx=4)
        ttk.Button(act_row, text="停止动作", width=10, command=self._stop_motion).pack(
            side=tk.LEFT, padx=4
        )
        ttk.Label(
            act,
            text="点头：1000→500→回中750；摇头：1000→2000→1000→回中1500；每步约900ms。堵转立刻「两路全关」。",
            wraplength=520,
        ).pack(anchor=tk.W, pady=(6, 0))

        tip = ttk.Label(
            mid,
            text="范围 0～19999 µs（定时器上限）。两路可同时开；大脉宽可能堵转，调完点「两路全关」。",
            wraplength=500,
        )
        tip.pack(anchor=tk.W, pady=(4, 0))

        logf = ttk.LabelFrame(self.root, text="串口回显", padding=8)
        logf.pack(fill=tk.BOTH, expand=True, padx=10, pady=(0, 10))
        self.log = tk.Text(logf, height=10, wrap=tk.WORD)
        self.log.pack(fill=tk.BOTH, expand=True)
        self.log.configure(state=tk.DISABLED)

    def _ch_tag(self) -> str:
        return f"S{self.ch.get()}"

    def _refresh_ports(self) -> None:
        ports = [p.device for p in list_ports.comports()]
        if DEFAULT_PORT not in ports:
            ports = [DEFAULT_PORT] + ports
        self.port_box["values"] = ports
        if self.port_var.get() not in ports and ports:
            self.port_var.set(ports[0])

    def _append(self, line: str) -> None:
        self.log.configure(state=tk.NORMAL)
        self.log.insert(tk.END, line + "\n")
        self.log.see(tk.END)
        self.log.configure(state=tk.DISABLED)

    def _drain_log(self) -> None:
        while True:
            try:
                line = self.log_q.get_nowait()
            except queue.Empty:
                break
            self._append(line)
            if line.startswith("stm32:") and "OK" in line:
                self.status_var.set("已连接 " + self.port_var.get())
        self.root.after(100, self._drain_log)

    def _on_slide(self, _v: str) -> None:
        us = int(round(float(self.scale.get())))
        us = max(PULSE_MIN, min(PULSE_MAX, us))
        self.pulse.set(us)
        self.value_lbl.configure(text=f"{self._ch_tag()} {us} µs")

    def _set_pulse(self, us: int) -> None:
        self.scale.set(us)
        self._on_slide("")
        self._apply()

    def _nudge(self, delta: int) -> None:
        self._set_pulse(max(PULSE_MIN, min(PULSE_MAX, self.pulse.get() + delta)))

    def _send(self, cmd: str) -> None:
        if not self.ser or not self.ser.is_open:
            messagebox.showwarning("串口", "先连接 COM 口")
            return
        data = (cmd.strip() + "\r\n").encode("ascii", errors="ignore")
        try:
            self.ser.write(data)
            self._append(f">> {cmd}")
        except serial.SerialException as e:
            messagebox.showerror("串口", str(e))
            self._disconnect()

    def _apply(self) -> None:
        self._on_slide("")
        self._send(f"{self._ch_tag()} {self.pulse.get()}")

    def _off(self) -> None:
        self._send(f"{self._ch_tag()} OFF")

    def _all_off(self) -> None:
        self._stop_motion()
        self._send("ALL OFF")

    def _ping(self) -> None:
        self._send("PING")

    def _cancel_motion_timer(self) -> None:
        if self._motion_after is not None:
            try:
                self.root.after_cancel(self._motion_after)
            except tk.TclError:
                pass
            self._motion_after = None

    def _stop_motion(self) -> None:
        self._motion_token += 1
        self._cancel_motion_timer()
        self._append("-- 停止动作")

    def _pulse_ch(self, ch: str, us: int) -> None:
        self.ch.set(ch)
        self.scale.set(us)
        self._on_slide("")
        self._send(f"S{ch} {us}")

    def _run_seq(
        self,
        ch: str,
        steps: tuple[int, ...],
        label: str,
        center_us: int,
    ) -> None:
        if not self.ser or not self.ser.is_open:
            messagebox.showwarning("串口", "先连接 COM 口")
            return
        self._motion_token += 1
        token = self._motion_token
        self._cancel_motion_timer()
        self.ch.set(ch)
        self._append(f"-- {label} 开始（末步回中 {center_us}µs）")

        def finish_center() -> None:
            if token != self._motion_token:
                return
            self._pulse_ch(ch, center_us)
            self._append(f"-- {label} 已回中 {center_us}µs")

            def re_center() -> None:
                if token != self._motion_token:
                    return
                self._pulse_ch(ch, center_us)
                self._motion_after = None

            # 200ms 后再发一次中位，防止桥接丢末帧停在左侧
            self._motion_after = self.root.after(200, re_center)

        def step(i: int) -> None:
            if token != self._motion_token:
                return
            if i >= len(steps):
                self._append(f"-- {label} 扫角结束 → 回中")
                finish_center()
                return
            us = steps[i]
            self._pulse_ch(ch, us)
            self._append(f"-- {label} 步{i + 1}/{len(steps)} → {us}µs")
            self._motion_after = self.root.after(
                MOTION_HOLD_MS, lambda i=i: step(i + 1)
            )

        step(0)

    def _nod(self) -> None:
        """点头：SERVO1 俯仰扫角，结束回中 750。"""
        self._run_seq("1", NOD_SEQ, "点头", NOD_CENTER_US)

    def _shake(self) -> None:
        """摇头：SERVO2 偏航扫角，结束必须回中 1500。"""
        self._run_seq("2", SHAKE_SEQ, "摇头", SHAKE_CENTER_US)

    def _reader_loop(self) -> None:
        buf = ""
        while not self._stop.is_set() and self.ser and self.ser.is_open:
            try:
                raw = self.ser.read(self.ser.in_waiting or 1)
            except serial.SerialException:
                self.log_q.put("!! 串口断开")
                break
            if not raw:
                continue
            buf += raw.decode("utf-8", errors="replace")
            while "\n" in buf:
                line, buf = buf.split("\n", 1)
                line = line.strip("\r")
                if line:
                    self.log_q.put(line)

    def _connect(self) -> None:
        self._disconnect()
        port = self.port_var.get().strip()
        try:
            self.ser = serial.Serial()
            self.ser.port = port
            self.ser.baudrate = BAUD
            self.ser.timeout = 0.05
            # 先禁止 DTR/RTS，避免 CH343 反复复位 ESP32
            self.ser.dtr = False
            self.ser.rts = False
            self.ser.open()
            self.ser.dtr = False
            self.ser.rts = False
        except serial.SerialException as e:
            messagebox.showerror("串口", f"打开 {port} 失败：{e}")
            return
        self._stop.clear()
        self._reader = threading.Thread(target=self._reader_loop, daemon=True)
        self._reader.start()
        self.status_var.set(f"等待 {port}")
        self._append(f"-- 打开 {port} @ {BAUD}")
        self.root.after(1500, self._ping)

    def _disconnect(self) -> None:
        self._stop.set()
        if self._reader and self._reader.is_alive():
            self._reader.join(timeout=0.5)
        self._reader = None
        if self.ser:
            try:
                if self.ser.is_open:
                    self.ser.close()
            except serial.SerialException:
                pass
            self.ser = None
        self.status_var.set("未连接")

    def _on_close(self) -> None:
        self._stop_motion()
        try:
            if self.ser and self.ser.is_open:
                self.ser.write(b"ALL OFF\r\n")
        except serial.SerialException:
            pass
        self._disconnect()
        self.root.destroy()


def main() -> None:
    root = tk.Tk()
    try:
        root.call("tk", "scaling", 1.25)
    except tk.TclError:
        pass
    ServoTuneApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
