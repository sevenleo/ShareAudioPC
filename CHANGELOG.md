# Changelog

All notable project changes should be recorded in this file.

The format follows a simple staged log. Keep new entries under `Unreleased` until a release tag is created.

## Unreleased

### Added

- Added the current project documentation inventory in `docs/CURRENT_STATUS.md`.
- Added this `CHANGELOG.md` to track recent changes by implementation stage.
- Added a current-state note to `docs/ideia.md` clarifying that it is historical input, not the current implementation contract.
- Added `docs/PLAN-GUI.md` with the GUI implementation checklist.
- Added a Qt-free shared `SessionController` for CLI and GUI runtime behavior.
- Added optional Qt Widgets GUI target `shareaudio_gui`.
- Added GUI dashboard source with Share, Listen, Local IPs, Devices, Diagnostics, and Help areas.
- Added GUI CMake presets for Windows/Linux debug/release builds.
- Added native `SAL1` stream session header support.
- Added receiver mode autodetection from the stream session header.
- Added PCM broadcast behavior that sends the stream header before audio bytes to every receiver.
- Added simplified user-facing CLI commands:
  - `shareaudio_cli share`
  - `shareaudio_cli share --mode ultrafast`
  - `shareaudio_cli listen <host>`
  - `shareaudio_cli devices`
  - `shareaudio_cli ips`
  - `shareaudio_cli help`
- Added protocol tests for stream header encoding, decoding, and invalid headers.
- Added broadcast server test coverage for header-before-audio ordering.
- Added CLI behavior tests for new commands and removed legacy flags.

### Changed

- `share` now defaults to Balanced Mode when `--mode` is omitted.
- `listen <host>` no longer accepts or requires an audio mode argument.
- `ConsoleUi` now uses the shared session controller instead of owning streaming internals directly.
- Receiver packet size now comes from the validated stream header.
- README usage examples now document only the simplified CLI.
- `docs/PLAN.md` now reflects the current CLI and protocol autodetection direction.

### Removed

- Removed legacy CLI command handling from the user-facing parser:
  - `--status`
  - `--start-transmitter`
  - `--connect`
  - `--transmit-pcm`
  - `--receive-pcm`
  - `--list-ips`
  - `--list-audio-devices`

### Known Incomplete Work

- Opus Quality Mode is still not usable.
- Browser listening is not supported by the native TCP protocol.
- Android/Web compatibility is not validated.
- Linux build/test and real cross-machine audio tests are still pending.
- GUI build verification is pending until Qt6 Widgets is installed or configured in `CMAKE_PREFIX_PATH`.

## Earlier MVP Work

### Added

- Added C++20/CMake project foundation.
- Added Windows/Linux CMake presets.
- Added miniaudio and standalone Asio vendored dependencies.
- Added project-wide config, logging, and result/error utilities.
- Added audio capture/playback abstractions and miniaudio backend.
- Added TCP networking primitives and local loopback self-test.
- Added PCM transmitter and receiver pipelines.
- Added fixed-size jitter buffer.
- Added recent devices storage.
- Added Opus wrapper shell and Opus packet length helpers.
- Added automated tests for config, protocol, jitter buffer, storage, local IP matching, audio fakes, TCP, and PCM pipelines.
