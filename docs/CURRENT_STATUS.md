# ShareAudioLite Current Status

This document describes the current implementation state. `docs/ideia.md` remains the original source idea, while this file tracks what the current C++ project actually supports.

## Runtime Status

The current always-built user-facing application is `shareaudio_cli`. An optional Qt Widgets desktop target, `shareaudio_gui`, is now defined when `SHAREAUDIO_ENABLE_DESKTOP_UI=ON`.

Supported runtime flows:

- `shareaudio_cli share`
- `shareaudio_cli share --mode ultrafast`
- `shareaudio_cli listen <host>`
- `shareaudio_cli devices`
- `shareaudio_cli ips`
- `shareaudio_cli help`

GUI parity:

- Help/About covers `help`.
- Local IPs panel covers `ips`.
- Devices panel covers `devices`.
- Share panel covers `share` and `share --mode ultrafast`.
- Listen panel covers `listen <host>`.
- Quality Mode is visible but disabled.

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
- miniaudio capture/playback backend.
- Generated/null audio test backends.
- PCM packet chunker.
- Fixed-size jitter buffer.
- TCP server/client primitives.
- PCM broadcast server.
- Local IP enumeration and self-connection blocking.
- Recent devices storage.
- Opus wrapper shell and protocol framing helpers.
- Automated unit and loopback tests.
- Qt-free `SessionController` shared by CLI and GUI.
- Optional Qt Widgets dashboard GUI target.

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

- Planned Opus mode.
- Rejected by the CLI for now.
- The Opus wrapper can validate initialization requirements, but actual encode/decode is not implemented yet.

## Native Protocol

The current native TCP protocol starts each receiver connection with a 16-byte `SAL1` stream session header.

The receiver reads this header before audio bytes, validates it, then derives:

- stream mode
- codec
- channel count
- bytes per sample
- sample rate
- packet size

Legacy no-header streams are not supported by the simplified CLI.

## Platform Status

Windows:

- CMake configure/build/test has been verified with the `windows-debug` preset.
- `windows-gui-debug` configure was attempted, but Qt6 Widgets was not installed or discoverable in this workspace.
- Capture uses miniaudio loopback against playback devices.
- Playback uses miniaudio playback devices.

Linux:

- Linux presets exist.
- miniaudio capture/playback code is present.
- Linux build/test has not been verified in this workspace.
- System-audio capture may require selecting a PulseAudio/PipeWire monitor source manually.

Browser:

- Not supported by the native TCP app.
- A browser-compatible transport would require a WebSocket/WebRTC/HTTP bridge or separate protocol endpoint.

Android/Web:

- Compatibility has not been validated.
- The current native protocol can be adjusted later in the protocol module after reviewing the Android/Web implementation.

## Verification Status

Last verified commands in this workspace:

```powershell
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

Manual testing still needed:

- Windows transmitter to Windows receiver.
- Windows transmitter to Linux receiver.
- Linux transmitter to Windows receiver.
- Linux transmitter to Linux receiver.
- Long-running audio sessions.
- Real device disconnect/reconnect behavior.
- Measured latency and CPU use.
