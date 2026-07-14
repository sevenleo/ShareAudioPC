# Changelog

All notable project changes should be recorded in this file.

The format follows a simple staged log.

## [Unreleased]

### Added

- Added a static Qt GUI theme separated from `MainWindow` behavior.

### Changed

- Restored the complete compact Simple/Advanced GUI structure from commit `8df3daec` while retaining current Sharing/Receiver, VolumeMode, autostart, and tray behavior.
- Restored the exact public AudioMode labels and a single source of truth for host, mode, capture device, playback device, and session controls.
- Updated advanced sizing to target the established desktop dimensions while clamping the window to the current screen's available area.
- Reworked the GUI into a light desktop workspace with a flat status strip, two equal Sharing/Receiver panels, semantic panel accents, advanced tabs below the primary controls, and persistent footer actions.
- Added responsive matrix layouts and a scrollable content workspace so Sharing/Receiver panels and advanced tab sections stack instead of overlapping when the window is reduced.

### Removed

- Removed the sidebar/stacked-page experiment, duplicate controls, viewport-scaled fonts, emoji navigation, simulated CPU/latency/packet-loss values, fixed codec badges, invented interface names, and conceptual engine actions.

## [0.4.0] - 2026-07-12

### Added
- **Dynamic Layout Matrix (Grid-based Rearrangement)**: Implemented adaptive card layouts in `MainWindow` (`rearrange_layouts`) for Simple, Advanced, and Hardware pages using `QGridLayout`. On windows wider than 800px (or 1000px for Advanced), cards arrange in multi-column matrices. On narrower widths, layouts stack vertically.
- **Dynamic Font & UI Element Scaling**: Added runtime recalculation of font sizes, padding, margins, and button heights (`scale_ui_elements`) based on the window proportions, resolving text cutoffs, large elements, and overlapping labels on resize.
- **Dynamic Resize Handling**: Hooked resize events (`resizeEvent`) to trigger the dynamic scaling and card layout matrices instantly during window resizing.

## [0.3.1] - 2026-07-12

### Fixed

- **GUI Memory Safety (Heap Corruption Fix)**: Resolved a critical access violation (`0xC0000005` / heap corruption) in the active nodes list widget update. Memory allocation and deallocation for `QListWidgetItem` objects are now delegated entirely to the Qt library binary boundary (using `addItem` and `item` queries) to avoid DLL/EXE allocator mismatches on Windows.
- **GUI Startup Null-Safety**: Fixed instant crashes on launch when auto-starting server or receiver modes. Added robust pointer validation checks for system volume checkboxes, playback combo boxes, and capture combo boxes.
- **Dynamic Element Scaling**: Improved layouts so that the horizontal segmented profile buttons, IP scan buttons, and diagnostics control buttons scale proportionally and expand to fill available horizontal width. Added vertical stretches to keep card layouts cleanly aligned.
- **Style Contrast Correction**: Explicitly styled all `QLabel` text color to light gray (`#E5E2E1`), resolving unreadable black text issues against the dark background theme on light Windows system configurations.

### Added

- Added transmitter `VolumeMode` with `full` and Windows-only `system` values across GUI, CLI, internal JSON configuration, and `shareaudio.cfg`.
- Added read-only Windows Core Audio endpoint-volume tracking, dB-to-linear PCM gain conversion, mute handling, 100 ms polling, reroute rebinding, and a 10 ms stereo gain ramp.
- Added GUI `Follow system volume` control and diagnostic fields for configured VolumeMode, applied gain, and tracking state.
- Added automated coverage for VolumeMode parsing/persistence, dB conversion, byte-preserving Full mode, PCM scaling/ramping/mute, Efficient pipeline compatibility, and fallback state.

### Changed

- Rebuilt the Qt GUI as a clean, stacked-page dark-charcoal console matching the "Obsidian Industrial" concept, implementing dedicated Workspace layout states (Simple/Advanced), horizontal segmented profile selectors, and dynamic metrics.
- Rebranded visible GUI titles, tray tooltips, dialogs, and copied diagnostics as `ShareAudioPC`.
- Consolidated project documentation under `docs`: `README.md`, `CHANGELOG.md`, `PLAN.md`, and the command-only `BUILD.md`.
- Merged the relevant content from the previous status, protocol, sync, GUI plan, and original idea documents into `docs/README.md`.
- Rewrote `docs/PLAN.md` as a pending-work checklist only.
- Expanded `docs/README.md` into the primary technical reference for protocol, audio modes, pipelines, build, packaging, runtime behavior, and platform compatibility.
- Audited `docs/README.md` against the current source and corrected overstated claims around Efficient/Opus end-to-end readiness, HTTP Opus fallback, browser player mode support, Linux presets, socket options, callback allocation behavior, and automated coverage.
- Moved pending work and improvement notes out of `docs/README.md` into `docs/PLAN.md`, keeping the README focused on current application behavior.
- Restored and improved portable Windows packaging: `cmake --install` now produces the root `release` folder, runs Qt deployment, and excludes Opus development install artifacts from the portable folder.
- Fixed HTTP Opus fallback so `codec="opus"` metadata initializes the receiver as Efficient/Opus instead of Balanced PCM.
- Fixed Efficient AudioMode transmitter chunking so desktop transmitters feed exact 20ms PCM frames into the Opus encoder and emit length-prefixed Opus packets.
- Renamed the public selector to `AudioMode`, changed the CLI flag to `--audio-mode`, changed startup config to `AUDIO_MODE`, and standardized values to `balanced`, `fast`, and `efficient`.
- Changed the default communication port from the legacy value to TCP `33777` for native `SAL1` and HTTP fallback routes.
- Added GUI system tray support with `TRAYMODE` and `STARTINTRAY` startup config keys.
- Moved the GUI `Minimize to tray` control to a larger fixed footer visible in both simple and advanced modes.
- Added GUI simultaneous sharing/listening support and GUI-only `shareaudio.cfg` `MODE=both` autostart.
- Replaced the root `/` browser page with a Web Receiver that reads `/info`, supports Fast/Balanced PCM, rejects Efficient/Opus, and schedules playback through Web Audio with adaptive drop thresholds.
- Updated the root Web Receiver page branding to `ShareAudioPC`, refined the embedded page styling, and added a red `Disconnect` button state that aborts the active stream.
- Reworked Web Receiver jitter handling to use adaptive prebuffering, larger render blocks, stable browser-oriented latency targets, and no timeline reset when dropping over-buffered render blocks.
- Added bounded per-client stream send queues so slow HTTP/native receivers do not write synchronously inside the broadcast loop.
- Updated CLI/GUI help text and project documentation to match the current Efficient/Opus, browser route, Android/Web, and `docs/BUILD.md` behavior.

### Removed

- Removed obsolete consolidated documentation files from `docs`.
- Removed root-level `README.md` and `CHANGELOG.md`; project documentation now lives under `docs`.

## [0.6.0] - 2026-07-07

### Added

- **Portable Startup Configuration (`shareaudio.cfg`)**: Place a `shareaudio.cfg` file next to the executable to auto-configure and auto-start the application. Supports variables: `AUTOSTART` (master switch), `MODE` (server/client), `AUDIO_MODE`, `DEVICE_ID`, `PLAYBACK_DEVICE_ID`, and `SERVER_IP`.
- **CLI Zero-Argument Fallback**: Running `shareaudio_cli` with no arguments now loads `shareaudio.cfg` if present and auto-starts as server or client based on the config. Explicit CLI arguments always override the config file entirely.
- **GUI Pre-Fill from Config**: The GUI pre-fills the mode combobox, device selections, and server IP field from `shareaudio.cfg` on startup. If `AUTOSTART=true` and the config is valid, the session starts automatically after the window opens.
- **Graceful Validation**: Invalid or incomplete configs, such as `MODE=client` without `SERVER_IP`, are silently ignored in the GUI, which opens normally without auto-starting.
- Comprehensive unit tests for config file parsing, covering missing files, full and partial configs, comments, case-insensitivity, and unknown keys.

## [0.5.1] - 2026-07-07

### Fixed

- Optimized GUI window sizing and resizing: increased the default and minimum window sizes by 30% for both Simple and Advanced modes. Simple Mode is now `830x310` with minimum `800x300`; Advanced Mode is now `1100x730` with minimum `1000x600`.
- Removed fixed maximum sizing constraints so users can resize the GUI window freely in all modes.

## [0.5.0] - 2026-07-07

### Added

- Added full Opus Efficient AudioMode network implementation. The transmitter encodes captured audio to Opus frames and prefixes them with 2-byte Big-Endian length headers. The receiver parses length headers, reads exact frame sizes, decodes them back to PCM, and feeds the output into the jitter buffer.
- Added Efficient AudioMode support to CLI and GUI.
- Removed the mock Efficient AudioMode errors and enabled the GUI combobox option.

## [0.4.0] - 2026-07-07

### Added

- Added redesigned 3-line Simple Layout:
  - Line 1: real-time status details, app state, dynamic server/client IP, port, and last message.
  - Line 2: server start/stop button to transmit audio.
  - Line 3: client input field for server IP and dynamic connect/disconnect button.
- Added automatic default device pre-selection. On startup, the default system capture and playback devices are pre-selected in comboboxes and highlighted in device lists.

## [0.3.0] - 2026-07-07

### Added

- Added Windows executable icon by bundling `icon/logo.ico` into the GUI executable.
- Added Qt resource bundling through `resources.qrc` and CMake AUTORCC for png/svg assets.
- Added Simple vs Advanced UI toggle, allowing the dashboard to collapse into a basic layout or expand into a full tabbed panel.

### Fixed

- Fixed GUI heap allocator and startup mismatch. Standard library runtime entry-point issues and heap allocator crashes were resolved by using a local MinGW GCC 13.1.0 toolchain and compiling the GUI with `-static-libgcc -static-libstdc++`.

## [0.2.0] - 2026-07-07

### Added

- Added project documentation inventory and current status documentation.
- Added changelog tracking by implementation stage.
- Added a current-state note to the original project idea document.
- Added GUI implementation checklist.
- Added a Qt-free shared `SessionController` for CLI and GUI runtime behavior.
- Added optional Qt Widgets GUI target `shareaudio_gui`.
- Added GUI dashboard source with Share, Listen, Local IPs, Devices, Diagnostics, and Help areas.
- Added GUI CMake presets for Windows/Linux debug/release builds.
- Added native `SAL1` stream session header support.
- Added receiver mode autodetection from the stream session header.
- Added PCM broadcast behavior that sends the stream header before audio bytes to every receiver.
- Added simplified user-facing CLI commands:
  - `shareaudio_cli share`
  - `shareaudio_cli share --audio-mode fast`
  - `shareaudio_cli listen <host>`
  - `shareaudio_cli devices`
  - `shareaudio_cli ips`
  - `shareaudio_cli help`
- Added `--device` and `-d` options to CLI `share` and `listen` commands.
- Added single-instance enforcement lock using `shareaudio.pid`.
- Added auto-reconnection loop in receiver thread to reconnect when the transmitter returns.
- Added printout of local IP addresses when starting a sharing session in the CLI.
- Added dynamic FetchContent download and compilation configuration for `libopus` v1.4.
- Added full `libopus` implementation to `OpusEncoder` and `OpusDecoder` wrappers.
- Added Soundwave visual identity stylesheet to Qt GUI.
- Added protocol tests for stream header encoding, decoding, and invalid headers.
- Added broadcast server test coverage for header-before-audio ordering.
- Added CLI behavior tests for new commands and removed legacy flags.
- Added real Opus PCM -> Opus -> PCM roundtrip automated unit test coverage.
- Added single-instance unit test coverage.
- Added hybrid HTTP/TCP server connection auto-detection with 150ms timeout window.
- Added server handlers for HTTP endpoints: `/info` JSON metadata, `/stream` continuous keep-alive binary streaming, and `/` inline HTML5 browser player.
- Added HTTP client fallback and metadata JSON parsing inside receiver client thread.

### Fixed

- Fixed Windows binary portability by linking MinGW runtime libraries statically.
- Fixed `sin` compile error in unit tests by including `<cmath>`.
- Fixed redefinition warning of `NOMINMAX` in `SingleInstance.cpp`.

### Changed

- `share` defaults to Balanced AudioMode when `--audio-mode` is omitted.
- `listen <host>` no longer accepts or requires an audio mode argument.
- `ConsoleUi` now uses the shared session controller instead of owning streaming internals directly.
- Receiver packet size now comes from the validated stream header.
- README usage examples were updated to the simplified CLI.
- Implementation plan was updated to reflect the current CLI and protocol autodetection direction.

### Removed

- Removed legacy CLI command handling from the user-facing parser:
  - `--status`
  - `--start-transmitter`
  - `--connect`
  - `--transmit-pcm`
  - `--receive-pcm`
  - `--list-ips`
  - `--list-audio-devices`

### Known Incomplete Work At That Stage

- Efficient AudioMode network pipeline integration was still pending.
- Browser listening was not yet supported by the native TCP protocol.
- Android/Web compatibility was not yet validated.
- Linux build/test and real cross-machine audio tests were still pending.
- GUI build verification was pending until Qt6 Widgets was installed or configured.

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
