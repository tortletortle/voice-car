#!/usr/bin/env python3
"""Safe TCP smoke test: sends STOP, then checks that an unknown action is rejected."""
from __future__ import annotations

import argparse
import json
import socket
import sys


def read_json_line(stream, limit: int = 256) -> dict:
    raw = stream.readline(limit + 1)
    if not raw:
        raise RuntimeError("server closed the connection before replying")
    if len(raw) > limit or not raw.endswith(b"\\n"):
        raise RuntimeError("response was oversized or not newline-terminated")
    try:
        value = json.loads(raw.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise RuntimeError(f"invalid JSON response: {exc}") from exc
    if not isinstance(value, dict):
        raise RuntimeError("response must be a JSON object")
    return value


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default="192.168.4.1")
    parser.add_argument("--port", type=int, default=3333)
    parser.add_argument("--timeout", type=float, default=3.0)
    args = parser.parse_args()

    if not 1 <= args.port <= 65535 or args.timeout <= 0:
        parser.error("port must be 1..65535 and timeout must be positive")

    try:
        with socket.create_connection((args.host, args.port), timeout=args.timeout) as sock:
            sock.settimeout(args.timeout)
            stream = sock.makefile("rwb")
            stream.write(b'{"source":"manual","action":"stop"}\\n')
            stream.flush()
            stop_reply = read_json_line(stream)
            if not (
                stop_reply.get("ok") is True
                and stop_reply.get("action") == "stop"
                and stop_reply.get("stage") == "esp32_queued"
            ):
                raise RuntimeError(f"STOP response did not match contract: {stop_reply}")

            stream.write(b'{"source":"manual","action":"__invalid_probe__"}\\n')
            stream.flush()
            reject_reply = read_json_line(stream)
            if not (
                reject_reply.get("ok") is False
                and reject_reply.get("error") == "unknown_action"
            ):
                raise RuntimeError(f"unknown action was not rejected: {reject_reply}")

        print("TCP smoke test: PASS (STOP accepted; unknown action rejected)")
        return 0
    except (OSError, RuntimeError) as exc:
        print(f"TCP smoke test: FAIL: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
