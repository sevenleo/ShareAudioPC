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

## 3. Add focused regression coverage for the GUI/CLI contract

- **Priority:** P3 — prevent documentation drift.
- **Locations:** `tests/main.cpp` and relevant controller/parser test seams.
- **Problem:** Behavior is spread across `MainWindow`, `ConsoleUi`, `Application`, and `SessionController`; source review cannot exercise every state transition.
- **Requested implementation:** Test CLI mode/volume parsing and usage errors, simultaneous sharing/listening state precedence, `MUTE_LOCAL_AUDIO` versus `VOLUME_MODE=system`, and the diagnostics fields copied by the GUI.
- **Acceptance criteria:** Tests fail if command syntax, state precedence, startup exclusivity, or diagnostic names drift. Keep coverage focused; add no framework or broad fixture layer.

## Audit notes

- No implementation file was changed during this review.
- Existing pending work in `docs/PLAN.md` was preserved.
- The former fixed `All good` contradiction is resolved; older changelog entries remain historical records.
