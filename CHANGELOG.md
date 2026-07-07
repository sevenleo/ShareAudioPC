# Changelog

All notable project changes should be recorded in this file.

The format follows a simple staged log.

## [0.5.0] - 2026-07-07

### Added

- Added Full Opus Quality Network Mode Implementation: Completed Phase 17 of the implementation plan. The transmitter now encodes captured audio to Opus frames and prefixes them with 2-byte Big-Endian length headers. The receiver parses length headers, fetches exact frame sizes, decodes them back to PCM, and feeds the output into the jitter buffer, enabling high-quality, low-bandwidth LAN streaming.
- Added quality mode support to CLI and GUI. Removed the mock errors/limitations and enabled the combobox option in the GUI.

## [0.4.0] - 2026-07-07

### Added

- Added Redesigned 3-Line Simple Layout: The compact GUI now lists precisely 3 lines in simple mode:
  - Line 1: Real-time Status details (App state, dynamic server/client IP, port, last message).
  - Line 2: Server button (dynamic start/stop button to transmit audio).
  - Line 3: Client input field (to enter server IP) and dynamic connect/disconnect button.
- Added Automatic Default Device Pre-selection: On startup, the default system capture and playback devices are pre-selected in the comboboxes and highlighted in the lists, preventing users from selecting incorrect sound settings.

## [0.3.0] - 2026-07-07

### Added

- Added Windows Executable Icon: bundled `icon/logo.ico` resource into the GUI executable so it has the custom brand icon in Windows Explorer and the taskbar.
- Added Qt Resource Bundling: configured `resources.qrc` and CMake AUTORCC to embed png/svg assets directly inside the compiled binary.
- Added Simple vs Advanced UI Toggle: introduced a `"Show Advanced Options"` dashboard toggle that collapses the interface into a sleek, basic-only layout or expands it into a comprehensive tabbed panel for power users.

### Fixed

- Fixed GUI Heap Allocator and Startup Mismatch: resolved standard library runtime entry-point issues (`_M_replace_cold`) and heap allocator conflicts (`RtlFreeHeap`/`0xc0000139` crashes) by installing/configuring a local MinGW GCC 13.1.0 toolchain and compiling the GUI with `-static-libgcc -static-libstdc++`.

## [0.2.0] - 2026-07-07

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
- Added `--device` (or `-d`) options to CLI `share` and `listen` commands to allow selecting audio capture and playback devices.
- Added single-instance enforcement lock system checking for `shareaudio.pid` and closing older processes.
- Added auto-reconnection loop in receiver thread to dynamically reconnect to transmitter under connection loss.
- Added printout of local IP addresses right when starting a sharing session in the CLI.
- Added dynamic FetchContent download and compilation configuration for `libopus` v1.4.
- Added full `libopus` implementation to `OpusEncoder` and `OpusDecoder` wrappers supporting CBR, float/fixed calculations, and stereo frame compression/decompression.
- Added Soundwave visual identity CSS stylesheet to Qt GUI `MainWindow` (Deep Space backgrounds, Neon Green/Cyan highlights).
- Added protocol tests for stream header encoding, decoding, and invalid headers.
- Added broadcast server test coverage for header-before-audio ordering.
- Added CLI behavior tests for new commands and removed legacy flags.
- Added real Opus codec PCM -> Opus -> PCM roundtrip automated unit test coverage.
- Added single-instance unit test coverage.
- Added hybrid HTTP/TCP server connection auto-detection with 150ms timeout window.
- Added server handlers for HTTP endpoints: `/info` JSON metadata, `/stream` keep-alive chunk streaming, and `/` inline HTML5 browser player.
- Added HTTP client fallback and metadata JSON parsing inside receiver client thread.
- Added automated integration tests for hybrid HTTP/TCP auto-detection, info route JSON parsing, and fallback streaming.

### Fixed

- Fixed Windows binary portability: MinGW runtime libraries (`libgcc`, `libstdc++`, `libwinpthread`) are now linked statically so `shareaudio_cli.exe` runs on any Windows PC without requiring MinGW DLLs.
- Fixed `sin` compile error in unit tests by including `<cmath>`.
- Fixed redefinition warning of `NOMINMAX` in `SingleInstance.cpp`.

### Changed

- `share` now defaults to Balanced Mode when `--mode` is omitted.
- `listen <host>` no longer accepts or requires an audio mode argument.
- `ConsoleUi` now uses the shared session controller instead of owning streaming internals directly.
- Receiver packet size now comes from the validated stream header.
- README usage examples now document only the simplified CLI.
- `docs/PLAN.md` now reflects the current CLI and protocol autodetection direction.
- Updated all project documentation (`PLAN.md`, `PLAN-GUI.md`, `CURRENT_STATUS.md`, `README.md`) to accurately reflect current code state after thorough audit.

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

- Quality Mode (Opus) network pipeline integration is pending (Fase 17).
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
