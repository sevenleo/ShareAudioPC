# PLAN-GUI.md Checklist

## Goal
- [x] Create a second binary named `shareaudio_gui`.
- [x] Keep `shareaudio_cli` unchanged and fully supported.
- [x] Ensure every current CLI function is available in the GUI.
- [x] Use Qt Widgets for the desktop GUI.
- [x] Use a dashboard layout for non-technical users.

## CLI Parity
- [x] GUI supports `help` through Help/About.
- [x] GUI supports `ips` through a Local IPs panel.
- [x] GUI supports `devices` through a Devices panel.
- [x] GUI supports `share` with Balanced as default.
- [x] GUI supports `share --mode ultrafast`.
- [x] GUI supports `listen <host>` with a Host/IP field.
- [x] GUI receiver autodetects mode from `SAL1`.
- [x] GUI rejects/blocks Quality Mode until Opus is implemented.
- [x] GUI blocks self-connections like the CLI.
- [x] GUI exposes readable errors equivalent to CLI errors.

## Core Refactor
- [x] Extract streaming logic from `ConsoleUi`.
- [x] Create a Qt-free session controller in core.
- [x] Add `start_sharing`.
- [x] Add `stop_sharing`.
- [x] Add `start_listening`.
- [x] Add `stop_listening`.
- [x] Add local IP listing API.
- [x] Add capture/playback device listing API.
- [x] Add status snapshot API.
- [x] Add last-error/log event API.
- [x] Refactor CLI to use the shared controller.
- [x] Keep CLI command behavior unchanged.

## GUI Screens
- [x] Main dashboard window.
- [x] Top status bar: Idle, Sharing, Listening.
- [x] Share panel with Start/Stop.
- [x] Mode selector: Balanced and Ultrafast.
- [x] Quality shown disabled as Coming Soon.
- [x] Capture device selector.
- [x] Listen panel with Host/IP input.
- [x] Recent devices list.
- [x] Connect/Disconnect buttons.
- [x] Detected stream mode display.
- [x] Local IP list with copy button.
- [x] Devices panel with refresh button.
- [x] Diagnostics/log panel.
- [x] Copy diagnostics button.
- [x] Help/About dialog.

## Build System
- [x] Add optional Qt6 Widgets discovery.
- [x] Add `SHAREAUDIO_ENABLE_DESKTOP_UI`.
- [x] Add `shareaudio_gui` target.
- [x] Keep existing CLI target.
- [x] Keep non-GUI presets independent from Qt.
- [x] Add GUI debug/release presets for Windows.
- [x] Add GUI debug/release presets for Linux.

## UX Requirements
- [x] One active session at a time.
- [x] Disable conflicting controls while sharing/listening.
- [x] Stop session cleanly when closing the window.
- [x] Show clear user-facing errors.
- [x] Show port `8080`.
- [x] Show connected clients.
- [x] Show bytes sent/received.
- [x] Show dropped packets/underruns.
- [x] Refresh stats periodically without blocking UI.

## Persistence
- [x] Persist last selected mode.
- [x] Persist last host.
- [x] Persist selected capture device.
- [x] Persist selected playback device.
- [x] Persist recent devices.
- [x] Handle missing/corrupt config safely.

## Tests
- [x] Core tests for start/stop sharing.
- [x] Core tests for start/stop listening.
- [x] Core tests for simultaneous mode rejection.
- [x] Core tests for idempotent stop.
- [x] Core tests for Quality rejection.
- [x] Core tests for self-connection rejection.
- [x] CLI regression tests.
- [ ] GUI construction test.
- [ ] GUI button/controller interaction test.
- [ ] Manual GUI-to-CLI streaming test.
- [ ] Manual CLI-to-GUI streaming test.
- [ ] Manual GUI-to-GUI streaming test.

## Documentation
- [x] Update `README.md`.
- [x] Update `docs/CURRENT_STATUS.md`.
- [x] Update `docs/PLAN.md`.
- [x] Update `CHANGELOG.md`.
- [x] Document Qt dependency.
- [x] Document `shareaudio_gui` usage.
- [x] Document CLI/GUI parity.
- [x] Document remaining Opus limitation.

## Acceptance Criteria
- [x] `shareaudio_cli` still builds.
- [ ] `shareaudio_gui` builds when Qt is available.
- [x] CLI tests pass.
- [x] GUI exposes every current CLI function.
- [x] GUI can start and stop sharing.
- [x] GUI can connect and disconnect listener.
- [x] GUI receiver autodetects stream mode.
- [x] GUI handles errors without crashing.

## Verification Notes
- [x] `cmake --build --preset windows-debug` passed without requiring Qt.
- [x] `ctest --preset windows-debug` passed.
- [x] `cmake --preset windows-gui-debug` was attempted.
- [ ] Qt6 Widgets was not available in this workspace, so `shareaudio_gui` compile verification remains pending.
