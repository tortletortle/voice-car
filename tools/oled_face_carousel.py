#!/usr/bin/env python3
"""OLED 表情包轮播调试器（PC 预览 + 串口控制 STM32 bringup）。

依赖：pip install pyserial
资源：assets/oled-expression-pack-v1.0/
默认口：COM9 @ 115200（经 ESP32 转发 USART3）

串口命令（需 bringup 已合入 FACE 支持并重编烧录）：
  FACE <0..22>
  NEXT / PREV
  AUTO <ms>     # 0=停轮播
  ANIM <0|1|2>  # 0=blink 1=look_around 2=sleep_breathe
  LIST
  PING
"""

from __future__ import annotations

import json
import queue
import sys
import threading
import tkinter as tk
from pathlib import Path
from tkinter import messagebox, ttk

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    print("缺少 pyserial：pip install pyserial")
    sys.exit(1)

ROOT = Path(__file__).resolve().parents[1]
PACK = ROOT / "assets" / "oled-expression-pack-v1.0"
MANIFEST = PACK / "manifest.json"
PNG_DIR = PACK / "png"

DEFAULT_PORT = "COM9"
BAUD = 115200

# 与 voice_car_oled_faces_rle_v1.h / manifest 对齐
STATIC_KEYS = [
    "neutral",
    "happy",
    "cute",
    "angry",
    "sleep",
    "listening",
    "confused",
    "surprised",
    "sad",
    "wink",
    "reject",
]
ANIMS = {
    0: ("blink", ["blink_00", "blink_01", "blink_02", "blink_03"]),
    1: ("look_around", ["look_around_00", "look_around_01", "look_around_02", "look_around_03"]),
    2: ("sleep_breathe", ["sleep_breathe_00", "sleep_breathe_01", "sleep_breathe_02", "sleep_breathe_03"]),
}


def asset_list(manifest: dict) -> list[tuple[int, str, str]]:
    """(id, key, label) for all 23 frames."""
    out: list[tuple[int, str, str]] = []
    for face in manifest["static_faces"]:
        out.append((int(face["id"]), face["key"], face["label"]))
    idx = 11
    for anim in manifest["animations"]:
        for frame in anim["frames"]:
            key = Path(frame).stem
            out.append((idx, key, f'{anim["key"]} / {key}'))
            idx += 1
    return out


class FaceCarouselApp:
    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.root.title("Voice Car · OLED 表情轮播调试")
        self.root.geometry("720x620")
        self.root.minsize(640, 560)

        if not MANIFEST.is_file():
            messagebox.showerror("资源", f"找不到 {MANIFEST}")
            root.destroy()
            return

        self.manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
        self.assets = asset_list(self.manifest)
        self.labels = {
            i: f"{i:02d}  {key}  —  {label}" for i, key, label in self.assets
        }

        self.ser: serial.Serial | None = None
        self._stop = threading.Event()
        self._reader: threading.Thread | None = None
        self.log_q: queue.Queue[str] = queue.Queue()

        self.port_var = tk.StringVar(value=DEFAULT_PORT)
        self.status_var = tk.StringVar(value="未连接")
        self.idx_var = tk.IntVar(value=0)
        self.auto_var = tk.BooleanVar(value=False)
        self.interval_var = tk.IntVar(value=800)
        self.send_uart_var = tk.BooleanVar(value=True)
        self.anim_var = tk.IntVar(value=0)
        self._photo: tk.PhotoImage | None = None
        self._auto_job: str | None = None

        self._build()
        self.root.protocol("WM_DELETE_WINDOW", self._on_close)
        self.root.after(100, self._drain_log)
        self._refresh_ports()
        self._show_local(0)
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
        ttk.Checkbutton(top, text="同步下发 UART", variable=self.send_uart_var).grid(
            row=0, column=6, padx=6
        )

        mid = ttk.LabelFrame(self.root, text="表情 / 动画帧", padding=12)
        mid.pack(fill=tk.BOTH, expand=True, padx=10, pady=8)

        left = ttk.Frame(mid)
        left.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        self.preview = tk.Label(left, background="#000000", width=64, height=16)
        self.preview.pack(pady=6)

        self.name_lbl = ttk.Label(left, text="", font=("Segoe UI", 12))
        self.name_lbl.pack()

        self.face_box = ttk.Combobox(
            left,
            values=[self.labels[i] for i, _, _ in self.assets],
            state="readonly",
            width=48,
        )
        self.face_box.current(0)
        self.face_box.pack(pady=6)
        self.face_box.bind("<<ComboboxSelected>>", self._on_pick)

        nav = ttk.Frame(left)
        nav.pack(pady=4)
        ttk.Button(nav, text="◀ PREV", command=self._prev).pack(side=tk.LEFT, padx=3)
        ttk.Button(nav, text="NEXT ▶", command=self._next).pack(side=tk.LEFT, padx=3)
        ttk.Button(nav, text="下发 FACE", command=self._send_face).pack(side=tk.LEFT, padx=3)
        ttk.Button(nav, text="PING", command=self._ping).pack(side=tk.LEFT, padx=3)
        ttk.Button(nav, text="LIST", command=lambda: self._send("LIST")).pack(
            side=tk.LEFT, padx=3
        )

        auto = ttk.LabelFrame(mid, text="轮播", padding=8)
        auto.pack(side=tk.RIGHT, fill=tk.Y, padx=(12, 0))
        ttk.Checkbutton(
            auto, text="AUTO 轮播", variable=self.auto_var, command=self._toggle_auto
        ).pack(anchor=tk.W)
        row = ttk.Frame(auto)
        row.pack(fill=tk.X, pady=4)
        ttk.Label(row, text="间隔 ms").pack(side=tk.LEFT)
        ttk.Spinbox(row, from_=100, to=5000, increment=100, textvariable=self.interval_var, width=8).pack(
            side=tk.LEFT, padx=4
        )
        ttk.Button(auto, text="应用间隔到板端 AUTO", command=self._send_auto).pack(
            anchor=tk.W, pady=4
        )
        ttk.Button(auto, text="板端 AUTO 0（停）", command=lambda: self._send("AUTO 0")).pack(
            anchor=tk.W
        )

        ttk.Separator(auto, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=8)
        ttk.Label(auto, text="动画序列").pack(anchor=tk.W)
        for i, (name, _) in ANIMS.items():
            ttk.Radiobutton(auto, text=name, value=i, variable=self.anim_var).pack(anchor=tk.W)
        ttk.Button(auto, text="播动画 (ANIM)", command=self._send_anim).pack(
            anchor=tk.W, pady=6
        )

        tip = ttk.Label(
            self.root,
            text="先 PC 预览确认表情；要真机 OLED 轮播须 bringup 含 FACE 并烧录。未勾「同步下发」只改本地预览。",
            wraplength=680,
            padding=(10, 0),
        )
        tip.pack(anchor=tk.W)

        logf = ttk.LabelFrame(self.root, text="串口回显", padding=8)
        logf.pack(fill=tk.BOTH, expand=True, padx=10, pady=(4, 10))
        self.log = tk.Text(logf, height=8, wrap=tk.WORD)
        self.log.pack(fill=tk.BOTH, expand=True)
        self.log.configure(state=tk.DISABLED)

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
            if "READY" in line or line.startswith("OK FACE") or line == "PONG":
                self.status_var.set("已连接 " + self.port_var.get())
        self.root.after(100, self._drain_log)

    def _png_path(self, key: str) -> Path:
        return PNG_DIR / f"{key}.png"

    def _show_local(self, idx: int) -> None:
        idx = max(0, min(len(self.assets) - 1, idx))
        self.idx_var.set(idx)
        asset_id, key, label = self.assets[idx]
        self.face_box.current(idx)
        self.name_lbl.configure(text=f"#{asset_id}  {key}  ({label})")
        path = self._png_path(key)
        if not path.is_file():
            self.preview.configure(image="", text=f"缺文件 {path.name}")
            self._photo = None
            return
        try:
            # 放大预览（原图 128×64）
            img = tk.PhotoImage(file=str(path))
            img = img.zoom(3, 3)
            self._photo = img
            self.preview.configure(image=self._photo, text="")
        except tk.TclError as e:
            self.preview.configure(image="", text=str(e))
            self._photo = None

    def _on_pick(self, _e=None) -> None:
        self._goto(self.face_box.current(), send=True)

    def _goto(self, idx: int, send: bool) -> None:
        self._show_local(idx)
        if send and self.send_uart_var.get():
            self._send_face()

    def _next(self) -> None:
        self._goto((self.idx_var.get() + 1) % len(self.assets), send=True)

    def _prev(self) -> None:
        self._goto((self.idx_var.get() - 1) % len(self.assets), send=True)

    def _toggle_auto(self) -> None:
        if self.auto_var.get():
            self._schedule_auto()
            if self.send_uart_var.get():
                self._send_auto()
        else:
            if self._auto_job is not None:
                self.root.after_cancel(self._auto_job)
                self._auto_job = None
            if self.send_uart_var.get():
                self._send("AUTO 0")

    def _schedule_auto(self) -> None:
        if self._auto_job is not None:
            self.root.after_cancel(self._auto_job)
            self._auto_job = None
        if not self.auto_var.get():
            return
        ms = max(100, int(self.interval_var.get()))
        self._auto_job = self.root.after(ms, self._auto_tick)

    def _auto_tick(self) -> None:
        self._auto_job = None
        if not self.auto_var.get():
            return
        self._next()
        self._schedule_auto()

    def _send(self, cmd: str) -> None:
        if not self.ser or not self.ser.is_open:
            messagebox.showwarning("串口", "先连接 COM 口（或只做本地预览关掉「同步下发」）")
            return
        data = (cmd.strip() + "\r\n").encode("ascii", errors="ignore")
        try:
            self.ser.write(data)
            self._append(f">> {cmd}")
        except serial.SerialException as e:
            messagebox.showerror("串口", str(e))
            self._disconnect()

    def _send_face(self) -> None:
        asset_id = self.assets[self.idx_var.get()][0]
        self._send(f"FACE {asset_id}")

    def _send_auto(self) -> None:
        ms = max(0, int(self.interval_var.get()))
        self._send(f"AUTO {ms}")

    def _send_anim(self) -> None:
        self._send(f"ANIM {self.anim_var.get()}")

    def _ping(self) -> None:
        self._send("PING")

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
        self.auto_var.set(False)
        if self._auto_job is not None:
            self.root.after_cancel(self._auto_job)
        try:
            if self.ser and self.ser.is_open:
                self.ser.write(b"AUTO 0\r\n")
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
    FaceCarouselApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
