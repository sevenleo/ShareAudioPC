# ShareAudioPC Code Fix Tasks

This file contains code-only findings from the documentation audit. The documentation changes in this review do not implement any item below.

## 1. Replace the static GUI health status — resolved

- **Priority:** P2 — misleading status information.
- **Location:** `src/gui/MainWindow.cpp`, `build_ui()`, around `new QLabel("All good", status_panel_)`.
- **Problem:** The status strip always renders `All good`; it is never updated from `SessionStatus`, `last_error`, counters, device state, or connection state.
- **Impact:** Users can read a static presentation label as a live health assessment while the receiver is connecting, an error exists, a device is unavailable, or packets are dropped.
- **Resolution:** The restored compact status row removes the static label and continues to render controller state and `last_error` separately.
- **Acceptance criteria:** Met: the permanently positive label is absent.

## 2. Handle console interruption gracefully

- **Priority:** P2 — reliability and cleanup.
- **Location:** `src/ui/ConsoleUi.cpp`, `ConsoleUi::run()`, the `listen` polling loop and `share` wait path.
- **Problem:** CLI output says `Stop with Ctrl+C`, but no SIGINT/SIGTERM or Windows console-control handler converts Ctrl+C into `SessionController::stop_listening()`/`stop()`.
- **Impact:** Ctrl+C may terminate directly instead of running normal worker, socket, PID-lock, log, and device cleanup. The `shareaudio_cli stop` command exists, but the Ctrl+C message promises a graceful path not explicitly implemented.
- **Requested implementation:** Add a small platform-appropriate interruption path that sets an existing stop request; let the main loop perform the actual stop. The handler must not do blocking or non-signal-safe work.
- **Acceptance criteria:** Ctrl+C uses the normal cleanup path, returns a deterministic exit code, and does not leave a stale lock or active audio device.

## 3. Add focused GUI widget-contract regression coverage

- **Priority:** P3 — prevent documentation drift.
- **Locations:** `tests/main.cpp` and a minimal Qt-enabled test seam if GUI construction is made testable.
- **Problem:** Existing tests already cover CLI AudioMode/VolumeMode parsing and usage errors, simultaneous controller state precedence, startup-config parsing, and local-mute/System-VolumeMode exclusivity. They do not construct `MainWindow`, verify widget text/visibility/alignment, or cover the field names emitted by `diagnostics_text()`.
- **Requested implementation:** Add one focused GUI contract check for the Simple/Advanced visibility switch, exact Start/Stop/Connect/Disconnect labels, primary button object names/icons, unique advanced form labels, and diagnostics field names. Avoid screenshot frameworks and broad GUI fixtures.
- **Acceptance criteria:** The check fails if the GUI reintroduces duplicate labels, truncation-prone action sizing, incorrect action states, or diagnostic-key drift.

## Audit notes

- This audit updates documentation only; runtime implementation changes are tracked separately in the changelog.
- Existing pending work in `docs/PLAN.md` was preserved.
- The former fixed `All good` contradiction is resolved; older changelog entries remain historical records.
- Current automated tests may fail their three single-instance expectations when another `shareaudio_gui` or `shareaudio_cli` process owns the normal `%APPDATA%\ShareAudioLite` state. The documented build flow isolates `APPDATA` under the build directory to avoid that environmental collision.
