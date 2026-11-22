#!/usr/bin/env python3
"""本机表情面板：画出 OLED 表情包，点按钮经 COM9 发 VC 命令。

用法：
  python tools/face_panel.py
  python tools/face_panel.py --port COM9 --http 8765

浏览器打开 http://127.0.0.1:8765/
DTR/RTS 保持低，避免打开串口时复位 ESP32。
"""

from __future__ import annotations

import argparse
import json
import re
import threading
import time
from collections import deque
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlparse

import serial

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "assets" / "oled-expression-pack-v1.0" / "voice_car_oled_faces_rle_v1.h"
BITMAP_BYTES = 1024

# 与 voice_word_map / app_main 一致。asset 是表情包静态脸编号。
FACES = [
    ("HI", "打招呼", 1),
    ("HAPPY", "开心", 1),
    ("CUTE", "可爱", 2),
    ("ANGRY", "生气", 3),
    ("SLEEP", "睡觉", 4),
    ("LISTEN", "在听", 5),
    ("CONF", "疑惑", 6),
    ("SURP", "惊讶", 7),
    ("SAD", "难过", 8),
    ("WINK", "眨眼", 9),
    ("NO", "不行", 10),
]
ALLOWED = {name for name, _label, _asset in FACES}


def parse_c_u16_array(text: str, name: str) -> list[int]:
    m = re.search(name + r"\[[^\]]*\]\s*=\s*\{([^}]*)\}", text)
    if not m:
        raise RuntimeError(f"header missing {name}")
    return [int(x.rstrip("uU"), 0) for x in m.group(1).split(",") if x.strip()]


def parse_c_string_bytes(text: str) -> bytes:
    m = re.search(r"vc_oled_rle_data\[\]\s*=\s*(.*?);", text, re.S)
    if not m:
        raise RuntimeError("header missing vc_oled_rle_data")
    blob = m.group(1)
    out = bytearray()
    i = 0
    while i < len(blob):
        if blob[i] != '"':
            i += 1
            continue
        i += 1
        while i < len(blob) and blob[i] != '"':
            if blob[i] == "\\" and i + 1 < len(blob):
                n = blob[i + 1]
                if n in "01234567":
                    j = i + 1
                    while j < len(blob) and j < i + 4 and blob[j] in "01234567":
                        j += 1
                    out.append(int(blob[i + 1 : j], 8) & 0xFF)
                    i = j
                elif n == "x" and i + 3 < len(blob):
                    out.append(int(blob[i + 2 : i + 4], 16))
                    i += 4
                else:
                    mapping = {"n": 10, "r": 13, "t": 9, "\\": 92, '"': 34, "0": 0}
                    out.append(mapping.get(n, ord(n)))
                    i += 2
            else:
                out.append(ord(blob[i]) & 0xFF)
                i += 1
        i += 1
    return bytes(out)


def decode_rle(data: bytes, offset: int, length: int) -> bytes:
    src = offset
    end = offset + length
    dst = bytearray()
    while src < end and len(dst) < BITMAP_BYTES:
        control = data[src]
        src += 1
        count = (control & 0x7F) + 1
        if control & 0x80:
            if src >= end or len(dst) + count > BITMAP_BYTES:
                raise RuntimeError("rle repeat overrun")
            value = data[src]
            src += 1
            dst.extend([value] * count)
        else:
            if src + count > end or len(dst) + count > BITMAP_BYTES:
                raise RuntimeError("rle literal overrun")
            dst.extend(data[src : src + count])
            src += count
    if len(dst) != BITMAP_BYTES or src != end:
        raise RuntimeError(f"rle size {len(dst)} src {src} end {end}")
    return bytes(dst)


def load_bitmaps() -> dict[int, bytes]:
    text = HEADER.read_text(encoding="utf-8", errors="replace")
    offsets = parse_c_u16_array(text, "vc_oled_rle_offset")
    lengths = parse_c_u16_array(text, "vc_oled_rle_length")
    raw = parse_c_string_bytes(text)
    bitmaps: dict[int, bytes] = {}
    for asset in range(11):
        bitmaps[asset] = decode_rle(raw, offsets[asset], lengths[asset])
    return bitmaps


class Link:
    def __init__(self, port: str) -> None:
        self.port = port
        self.lock = threading.Lock()
        self.lines: deque[str] = deque(maxlen=40)
        self.ser: serial.Serial | None = None
        self.error = ""
        self._buf = bytearray()
        self._stop = False
        self._thread = threading.Thread(target=self._reader, daemon=True)

    def start(self) -> None:
        try:
            ser = serial.Serial()
            ser.port = self.port
            ser.baudrate = 115200
            ser.timeout = 0.2
            ser.dtr = False
            ser.rts = False
            ser.open()
            ser.dtr = False
            ser.rts = False
            time.sleep(0.2)
            ser.reset_input_buffer()
            self.ser = ser
            self.note(f"opened {self.port} 115200")
        except Exception as exc:  # noqa: BLE001 — show the open error on the panel
            self.error = str(exc)
            self.note(f"OPEN FAIL {exc}")
        self._thread.start()

    def note(self, text: str) -> None:
        self.lines.append(text)

    def send(self, name: str) -> str:
        if name not in ALLOWED:
            return "unknown"
        line = f"VC {name}"
        with self.lock:
            if self.ser is None or not self.ser.is_open:
                self.note(f"not sent {line}: {self.error or 'port closed'}")
                return self.error or "port closed"
            self.ser.write((line + "\r\n").encode("ascii"))
        self.note(f">> {line}")
        return "sent"

    def snapshot(self) -> dict:
        return {
            "port": self.port,
            "ok": self.ser is not None and self.ser.is_open,
            "error": self.error,
            "lines": list(self.lines),
        }

    def close(self) -> None:
        self._stop = True
        with self.lock:
            if self.ser is not None:
                self.ser.close()

    def _reader(self) -> None:
        while not self._stop:
            ser = self.ser
            if ser is None or not ser.is_open:
                time.sleep(0.2)
                continue
            try:
                n = ser.in_waiting
                raw = ser.read(n or 1)
            except Exception as exc:  # noqa: BLE001
                self.error = str(exc)
                self.note(f"READ FAIL {exc}")
                time.sleep(0.3)
                continue
            if not raw:
                continue
            self._buf.extend(raw)
            while True:
                cut = None
                for sep in (b"\n", b"\r"):
                    i = self._buf.find(sep)
                    if i >= 0 and (cut is None or i < cut):
                        cut = i
                if cut is None:
                    break
                line = self._buf[:cut].decode("utf-8", errors="replace").strip()
                del self._buf[: cut + 1]
                if line:
                    self.note(line)


def page() -> bytes:
    cards = []
    for name, label, asset in FACES:
        cards.append(
            f'<button class="card" data-cmd="{name}" data-asset="{asset}">'
            f'<canvas data-asset="{asset}" width="128" height="64"></canvas>'
            f"<b>{label}</b><span>VC {name} · FACE {asset}</span></button>"
        )
    html = """<!DOCTYPE html>
<html lang="zh-CN"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>VoiceCar 表情</title>
<style>
  body{margin:0;background:#111;color:#eee;font-family:system-ui,sans-serif}
  header{padding:16px 20px 8px}
  h1{margin:0;font-size:1.25rem}
  p{margin:6px 0 0;color:#9aa}
  #st{min-height:1.2em;color:#7dcea0}
  .grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(180px,1fr));gap:12px;padding:12px 20px 24px}
  button.card{appearance:none;border:1px solid #333;background:#1b1b1b;color:#eee;border-radius:14px;padding:10px;text-align:left;cursor:pointer}
  button.card:active{transform:scale(.98)}
  canvas{width:100%;height:auto;background:#0a0a0a;border-radius:8px;image-rendering:pixelated}
  b{display:block;margin-top:8px}
  span{color:#888;font-size:.8rem}
  pre{margin:0 20px 24px;background:#0c0c0c;border-radius:10px;padding:12px;max-height:180px;overflow:auto;color:#cde}
</style></head><body>
<header>
  <h1>VoiceCar 表情</h1>
  <p id="st">连接中…</p>
</header>
<div class="grid">
""" + "\n".join(cards) + """
</div>
<pre id="log"></pre>
<script>
const cache = {};
async function bitmap(id){
  if (cache[id]) return cache[id];
  const r = await fetch('/bitmap/' + id);
  const buf = new Uint8Array(await r.arrayBuffer());
  const c = document.createElement('canvas');
  c.width = 128; c.height = 64;
  const ctx = c.getContext('2d');
  const img = ctx.createImageData(128, 64);
  for (let y = 0; y < 64; y++){
    for (let x = 0; x < 128; x++){
      const on = (buf[(y >> 3) * 128 + x] >> (y & 7)) & 1;
      const i = (y * 128 + x) * 4;
      const v = on ? 236 : 10;
      img.data[i] = img.data[i+1] = img.data[i+2] = v;
      img.data[i+3] = 255;
    }
  }
  ctx.putImageData(img, 0, 0);
  cache[id] = c;
  return c;
}
async function paint(){
  for (const el of document.querySelectorAll('canvas[data-asset]')){
    const src = await bitmap(el.dataset.asset);
    el.getContext('2d').drawImage(src, 0, 0);
  }
}
async function refresh(){
  const r = await fetch('/api/status');
  const j = await r.json();
  document.getElementById('st').textContent = j.ok
    ? ('已接 ' + j.port + ' · 点脸发给车')
    : ('串口未打开 ' + j.port + ' · ' + (j.error || ''));
  document.getElementById('log').textContent = (j.lines || []).join('\\n');
}
document.querySelectorAll('button.card').forEach(btn => {
  btn.addEventListener('click', async () => {
    const c = btn.dataset.cmd;
    document.getElementById('st').textContent = '发送 VC ' + c;
    const r = await fetch('/api/cmd?c=' + encodeURIComponent(c));
    const j = await r.json();
    document.getElementById('st').textContent = j.ok ? ('已发 VC ' + c) : ('失败 ' + (j.error || ''));
    refresh();
  });
});
paint();
refresh();
setInterval(refresh, 800);
</script>
</body></html>
"""
    return html.encode("utf-8")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default="COM9")
    ap.add_argument("--http", type=int, default=8765)
    args = ap.parse_args()

    bitmaps = load_bitmaps()
    link = Link(args.port)
    link.start()

    class Handler(BaseHTTPRequestHandler):
        def log_message(self, fmt: str, *a) -> None:  # quieter console
            return

        def _send(self, code: int, body: bytes, content: str) -> None:
            self.send_response(code)
            self.send_header("Content-Type", content)
            self.send_header("Content-Length", str(len(body)))
            self.send_header("Cache-Control", "no-store")
            self.end_headers()
            self.wfile.write(body)

        def do_GET(self) -> None:  # noqa: N802
            url = urlparse(self.path)
            if url.path in ("/", "/index.html"):
                self._send(200, page(), "text/html; charset=utf-8")
                return
            if url.path == "/api/status":
                body = json.dumps(link.snapshot(), ensure_ascii=False).encode("utf-8")
                self._send(200, body, "application/json; charset=utf-8")
                return
            if url.path == "/api/cmd":
                name = (parse_qs(url.query).get("c") or [""])[0].upper()
                err = link.send(name)
                ok = err == "sent"
                body = json.dumps(
                    {"ok": ok, "cmd": name, "error": "" if ok else err},
                    ensure_ascii=False,
                ).encode("utf-8")
                self._send(200 if ok else 400, body, "application/json; charset=utf-8")
                return
            if url.path.startswith("/bitmap/"):
                try:
                    asset = int(url.path.rsplit("/", 1)[-1])
                    blob = bitmaps[asset]
                except (ValueError, KeyError):
                    self._send(404, b"missing", "text/plain")
                    return
                self._send(200, blob, "application/octet-stream")
                return
            self._send(404, b"missing", "text/plain")

    server = ThreadingHTTPServer(("127.0.0.1", args.http), Handler)
    print(f"panel http://127.0.0.1:{args.http}/  serial {args.port}", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        link.close()
        server.server_close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
