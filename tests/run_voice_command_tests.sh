#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
CC=${CC:-cc}
OUT=${TMPDIR:-/tmp}/voice-car-command-test.$$
trap 'rm -f "$OUT"' EXIT HUP INT TERM
"$CC" -std=c99 -Wall -Wextra -Werror -pedantic \
  -I"$ROOT/firmware/common" \
  "$ROOT/tests/test_voice_command_router.c" \
  "$ROOT/firmware/common/cmd_router_core.c" \
  -o "$OUT"
"$OUT"
