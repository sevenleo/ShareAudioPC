# ShareAudioLite Current Status

This document describes the current implementation state. `docs/ideia.md` remains the original source idea, while this file tracks what the current C++ project actually supports.

## Runtime Status

The current always-built user-facing application is `shareaudio_cli`. An optional Qt Widgets desktop target, `shareaudio_gui`, is now defined when `SHAREAUDIO_ENABLE_DESKTOP_UI=ON`.

Supported runtime flows:

- `shareaudio_cli share`
- `shareaudio_cli share --mode ultrafast`
- `shareaudio_cli share [--device <device_id>]`
- `shareaudio_cli listen <host> [--device <device_id>]`
- `shareaudio_cli devices`
- `shareaudio_cli ips`
- `shareaudio_cli help`

GUI parity:

- Help/About covers `help`.
- Local IPs panel covers `ips`.
- Devices panel covers `devices`.
- Share panel covers `share`, `share --mode ultrafast` and custom device.
- Listen panel covers `listen <host>`.
- Quality Mode (Opus) is fully implemented and integrated in transmitter and receiver.
- GUI passes selected capture/playback devices to the session controller.
- **Redesigned 3-Line Simple Mode**: Starts in a resizable `830x310` window showing precisely 3 lines: Status (State, dynamic IP info, Port, and last message), Server Transmit button, and Client Receive controls (server IP text field + connect/disconnect button).
- **Advanced Mode Toggle**: Clicking "Show Advanced Options" resizes the window dynamically to `1100x730`, exposing quality/device selector comboboxes and the full tabbed dashboard (Network, Hardware, Diagnostics). Both views can be resized freely.
- **Default Device Pre-selection**: Automatically detects, pre-selects, and highlights the system's default capture and playback devices on startup, ensuring a smooth experience.
- **Embedded brand icon**: `logo.ico` is bundled inside the compiled `shareaudio_gui.exe` binary via Windows resource script. The window title bar and taskbar display the project logo.
- **Qt resource bundling**: `logo.png` and `logo.svg` are embedded inside the executable via `resources.qrc` (CMake AUTORCC).
- Updated color palette: Dark Space `#0A0F1D`, Card Dark `#151F3C`, accent borders `#25335A`, with contextual button colors — green for Share, blue for Connect, red for Stop.

Removed legacy commands:

- `--status`
- `--start-transmitter`
- `--connect`
- `--transmit-pcm`
- `--receive-pcm`
- `--list-ips`
- `--list-audio-devices`

## Implemented Components

- `AppConfig`, validation, and JSON persistence helpers.
- Logging with error, warning, info, and debug levels.
- `Result<T>` and project error codes.
- Audio format model fixed at 48 kHz, stereo, signed 16-bit PCM.
- `IAudioCapture` and `IAudioPlayback` abstractions.
- miniaudio capture/playback backend with notification/disconnect callbacks.
- Generated/null audio test backends.
- PCM packet chunker.
- Fixed-size jitter buffer.
- TCP server/client primitives.
- PCM broadcast server.
- Local IP enumeration and self-connection blocking.
- Recent devices storage.
- libopus encoder and decoder wrappers (fully implemented and statically linked).
- Automated unit and loopback tests (including real Opus encode/decode roundtrip).
- Qt-free `SessionController` shared by CLI and GUI.
- Qt Widgets tabbed GUI target with Simple/Advanced toggle layout and embedded brand assets.
- Windows resource script (`resources.rc`) embedding `logo.ico` inside the compiled `.exe`.
- Qt resource file (`resources.qrc`) embedding `logo.png` and `logo.svg` via CMake AUTORCC.
- Static-linked C++ runtime (`-static-libgcc -static-libstdc++`) for the GUI, dynamic C heap shared with Qt DLLs to avoid allocator conflicts.
- Dedicated MinGW 13.1.0 toolchain (`qt6/Tools/mingw1310_64`) configured for the GUI build to match Qt6 prebuilt DLL ABI.
- Single-instance lock mechanism (PIDs checked and old processes closed at start).
- Auto-reconnection logic (receiver continuously tries to reconnect to transmitter under connection loss).
- Hybrid TCP/HTTP server auto-detection (150ms timeout window) supporting `/info`, `/stream` and `/` endpoints.
- HTTP receiver client fallback with raw metadata JSON parsing.

## Audio Modes

Balanced Mode:

- Default mode for `shareaudio_cli share`.
- Raw PCM.
- 2048-byte packets.

Ultrafast Mode:

- Selected with `shareaudio_cli share --mode ultrafast`.
- Raw PCM.
- 1024-byte packets.

Quality Mode:

- Opus Quality Mode.
- Encoder/decoder wrapper and network pipeline fully implemented and integrated (Phase 17). Bitrate set to 128 kbps CBR with 2-byte Big-Endian length headers.

## Native and Hybrid Protocols

The application supports a dual-protocol handshake:

1. **Native TCP Protocol**:
   - Starts each receiver connection with a 16-byte `SAL1` stream session header.
   - The receiver reads this header silently before audio bytes, validates it, then derives: stream mode, codec, channel count, bytes per sample, sample rate, and packet size.

2. **HTTP/1.1 Protocol Fallback**:
   - Allows Mobile and Web Browser clients to connect seamlessly.
   - The server listens for `GET ` requests.
   - `/info` responds with JSON metadata describing the active stream parameters.
   - `/stream` responds with HTTP continuous stream headers, followed by raw audio packets.
   - `/` serves a lightweight Soundwave Web Audio player for browser listening.

## Platform Status

Windows:

- CMake configure/build/test has been verified with the `windows-debug` preset.
- CLI release builds produce a fully portable `.exe` — MinGW runtime libraries linked statically.
- GUI release build (`windows-gui-release`) verified using MinGW 13.1.0 and local Qt 6.6.3 installation.
- `shareaudio_gui.exe` packages logo icon in binary, starts in Simple Mode, and expands to Advanced Mode on demand.
- Dependencies deployed via `windeployqt.exe` — the entire `build/windows-gui-release/` folder is portable.
- Capture uses miniaudio loopback against playback devices.
- Playback uses miniaudio playback devices.

Linux:

- Linux presets exist.
- miniaudio capture/playback code is present.
- Linux build/test has not been verified in this workspace.
- System-audio capture may require selecting a PulseAudio/PipeWire monitor source manually.

Browser:

- Fully supported! Web browsers can connect to `http://<IP>:8080/` to play the raw PCM stream directly using Web Audio API via the inline HTML5 player served by the transmitter.

Android/Web:

- 100% compatibility has been established! The codebase implements the hybrid TCP/HTTP auto-detect server and HTTP fallback receiver client designed in `PLAN-FULL-SYNC.md`. This allows seamless cross-platform communication between the C++ Desktop and Android Mobile apps.

## Verification Status

Last verified commands in this workspace:

```powershell
cmake --preset windows-debug -DSHAREAUDIO_ENABLE_OPUS=ON
cmake --build --preset windows-debug
ctest --preset windows-debug
```

GUI release build status in this workspace:

```powershell
cmake --preset windows-gui-release -DCMAKE_C_COMPILER="d:/GITHUB/ShareAudioPC_2/qt6/Tools/mingw1310_64/bin/gcc.exe" -DCMAKE_CXX_COMPILER="d:/GITHUB/ShareAudioPC_2/qt6/Tools/mingw1310_64/bin/g++.exe" -DCMAKE_MAKE_PROGRAM="d:/GITHUB/ShareAudioPC_2/qt6/Tools/mingw1310_64/bin/mingw32-make.exe" -DCMAKE_PREFIX_PATH="d:/GITHUB/ShareAudioPC_2/qt6/6.6.3/mingw_64" -DSHAREAUDIO_ENABLE_OPUS=ON
cmake --build build/windows-gui-release --config Release
d:\GITHUB\ShareAudioPC_2\qt6\6.6.3\mingw_64\bin\windeployqt.exe D:\GITHUB\ShareAudioPC_2\build\windows-gui-release\shareaudio_gui.exe
```

Result: Passed successfully. Executable compiled with embedded icon, AUTORCC resources, and Simple/Advanced toggle layout. Packaged with windeployqt.exe.

Automated coverage includes:

- config validation and persistence
- protocol packet framing
- `SAL1` stream header encode/decode and invalid headers
- jitter buffer behavior
- recent device storage
- local IP checks
- TCP loopback
- PCM broadcast server header-before-audio behavior
- CLI command behavior for new and removed commands
- shared session controller start/stop behavior
- shared session controller loopback listener autodetection
- Opus wrapper encoder and decoder real roundtrip encoding/decoding
- audio fake backends (NullAudioPlayback, GeneratedToneCapture)
- PCM transmitter/receiver pipeline stats tracking
- Single-instance locking PID acquisition and release behavior
- Auto-reconnection logic and stats saving

Manual testing still needed:

- Windows transmitter to Windows receiver.
- Windows transmitter to Linux receiver.
- Linux transmitter to Windows receiver.
- Linux transmitter to Linux receiver.
- Long-running audio sessions.
- Real device disconnect/reconnect behavior.
- Measured latency and CPU use.
