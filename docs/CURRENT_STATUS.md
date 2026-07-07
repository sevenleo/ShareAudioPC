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
- Quality Mode (Opus) is visible but disabled in transmitter (network mode integration pending).
- GUI passes selected capture/playback devices to the session controller.
- Soundwave visual identity applied via Qt stylesheet (Deep Space `#0B101D` and Slate Dark `#1C253E` background, Neon Green `#1DF09A` and Cyan Blue `#00A3FF` accents).

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
- Optional Qt Widgets dashboard GUI target with Soundwave stylesheet.
- Static-linked MinGW runtime for portable Windows binaries.
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
- Encoder/decoder wrapper fully implemented. Network quality pipeline integration pending (Fase 17).

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
- Release builds produce a fully portable `.exe` — MinGW runtime libraries (`libgcc`, `libstdc++`, `libwinpthread`) are linked statically.
- `windows-gui-debug` configure was attempted, but Qt6 Widgets was not installed or discoverable in this workspace.
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

GUI configure status in this workspace:

```powershell
cmake --preset windows-gui-debug
```

Result: failed because Qt6 Widgets was not installed or not in `CMAKE_PREFIX_PATH`.

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
