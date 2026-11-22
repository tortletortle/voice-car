#!/usr/bin/env sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
OUT="${TMPDIR:-/tmp}/voice-car-remote-actions-test"
cc -std=c99 -Wall -Wextra -Werror -pedantic \
  -I"$ROOT/firmware/common" \
  -I"$ROOT/firmware/esp32/components/remote_net" \
  "$ROOT/firmware/esp32/components/remote_net/remote_actions.c" \
  "$ROOT/tests/test_remote_actions.c" \
  -o "$OUT"
"$OUT"
