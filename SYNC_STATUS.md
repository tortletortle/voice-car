# V0.3 source import status

Partial source import: shared command protocol, frame parser, and router core. The full local V0.3 Git baseline contains 79 tracked files, including the existing target-specific sources, tests and documentation. Do not treat this remote snapshot as a complete buildable release. The user's separately tested physical hardware drivers have not yet been imported.

## Voice command contract update — 2026-11-21

- Added shared command IDs for nod, shake, repeated nod/shake, and neutral head action without changing existing chassis command IDs or the six-byte frame format.
- Added the Chinese voice phrase/action table in `docs/VOICE_COMMANDS_V1.md`.
- Added independent chassis/gimbal pending slots to the common router. STOP remains highest priority, clears stale chassis motion, and does not discard a queued head action.
- Added a standalone host regression test; `tests/run_voice_command_tests.sh` passes with a C99 compiler.
- Integration is not end-to-end yet: the current remote tree does not contain the CI1302 word-ID map or target-specific ESP32/STM32 command handlers. Do not invent ASR word IDs or claim physical servo validation. Import these common-layer changes into the complete 79-file local baseline before whole-vehicle integration, preserving the separately tested hardware drivers.
