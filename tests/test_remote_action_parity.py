#!/usr/bin/env python3
"""Keep the Android and ESP32 remote-action allow-lists identical."""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
kotlin_path = ROOT / "android-app/app/src/main/java/com/voicecar/remote/AllowedActions.kt"
c_path = ROOT / "firmware/esp32/components/remote_net/remote_actions.c"

kotlin = kotlin_path.read_text(encoding="utf-8")
c_source = c_path.read_text(encoding="utf-8")

block_match = re.search(r"val values\s*=\s*setOf\((.*?)\)", kotlin, re.S)
if not block_match:
    print("Could not find AllowedActions.values setOf block", file=sys.stderr)
    sys.exit(2)

android_actions = set(re.findall(r'"([a-z_]+)"', block_match.group(1)))
esp32_actions = set(re.findall(r'\{"([a-z_]+)"\s*,\s*CMD_', c_source))

if android_actions != esp32_actions:
    print("Android-only actions:", sorted(android_actions - esp32_actions))
    print("ESP32-only actions:", sorted(esp32_actions - android_actions))
    sys.exit(1)

print(f"Android/ESP32 action parity: PASS ({len(android_actions)} actions)")
