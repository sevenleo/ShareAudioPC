# ShareAudioLite

ShareAudioLite is a cross-platform Windows/Linux LAN audio transmitter and receiver based on the original idea in `docs/ideia.md`.

The project is currently a C++20 MVP with a console CLI and an optional Qt Widgets desktop GUI. It can share and listen to raw PCM audio over TCP on a local network, using miniaudio for capture/playback and standalone Asio for networking.

## Current State

Implemented:

- C++20/CMake project foundation.
- Windows and Linux build presets.
- Vendored miniaudio and standalone Asio source dependencies.
- Console CLI executable: `shareaudio_cli`.
- Optional Qt Widgets desktop executable target: `shareaudio_gui`.
- Raw PCM transmitter and receiver pipeline.
- Balanced Mode: 2048-byte PCM packets.
- Ultrafast Mode: 1024-byte PCM packets.
- Native `SAL1` stream session header.
- Receiver mode autodetection from the stream header.
- TCP broadcast server with multiple receiver support.
- TCP client with exact reads/writes and disconnect handling.
- Local IP detection and self-connection blocking.
- Audio capture/playback abstractions.
- miniaudio-backed capture/playback.
- Fixed-size jitter buffer.
- Recent device storage.
- Config JSON load/save helpers.
- CI-friendly automated tests.

Not complete yet:

- Opus Quality Mode is not usable yet. The wrapper validates configuration, but encode/decode still return `NotSupported`.
- Browser listening is not supported by the current native TCP protocol.
- Android/Web compatibility has not been validated.
- Linux build/test has not been verified in this workspace.
- Manual cross-machine audio validation is still pending.
- GUI build verification requires Qt6 Widgets; Qt6 was not available in this workspace.

## Build

Windows with MinGW:

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug
ctest --preset windows-debug
```

Linux:

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug
```

Release builds:

```bash
cmake --preset windows-release
cmake --build --preset windows-release

cmake --preset linux-release
cmake --build --preset linux-release
```

## Build Options

- `SHAREAUDIO_ENABLE_MINIAUDIO=ON`: enables real audio capture/playback. Default: `ON`.
- `SHAREAUDIO_ENABLE_OPUS=ON`: attempts to link libopus. The network Quality Mode pipeline is still incomplete.
- `SHAREAUDIO_ENABLE_TESTS=ON`: builds `shareaudio_tests`. Default in presets: `ON`.
- `SHAREAUDIO_ENABLE_CONSOLE_UI=ON`: builds `shareaudio_cli`. Default in presets: `ON`.
- `SHAREAUDIO_ENABLE_DESKTOP_UI=OFF`: skips the optional Qt GUI target. Default in normal presets: `OFF`.
- `SHAREAUDIO_ENABLE_DESKTOP_UI=ON`: builds `shareaudio_gui` and requires Qt6 Widgets.

Example:

```bash
cmake --preset linux-debug -DSHAREAUDIO_ENABLE_OPUS=ON
```

GUI presets require Qt6 Widgets to be installed and discoverable by CMake:

```bash
cmake --preset windows-gui-debug
cmake --build --preset windows-gui-debug

cmake --preset linux-gui-debug
cmake --build --preset linux-gui-debug
```

## Dependencies

Required:

- CMake 3.24+
- C++20 compiler
- Threads support from the platform toolchain

Vendored:

- `third_party/miniaudio-src`: miniaudio `0.11.25`
- `third_party/asio-src`: standalone Asio `1.38.1`

Optional/system:

- `libopus` development package when configuring with `SHAREAUDIO_ENABLE_OPUS=ON`

Linux audio note: system-audio capture usually requires selecting a monitor/source device exposed by PulseAudio or PipeWire. The current Linux backend lists capture devices, but it does not yet automatically prefer monitor sources.

## Console Usage

Show help:

```bash
shareaudio_cli help
```

Show local IP addresses:

```bash
shareaudio_cli ips
```

Show audio devices:

```bash
shareaudio_cli devices
```

Share audio on TCP port `8080`. If no mode is provided, `share` uses Balanced Mode:

```bash
shareaudio_cli share
```

Share audio using Ultrafast Mode:

```bash
shareaudio_cli share --mode ultrafast
```

Listen to another machine. The receiver reads the `SAL1` stream header and autodetects the mode and packet size:

```bash
shareaudio_cli listen 192.168.1.50
```

Removed legacy flags:

- `--status`
- `--start-transmitter`
- `--connect`
- `--transmit-pcm`
- `--receive-pcm`
- `--list-ips`
- `--list-audio-devices`

Quality Mode is intentionally rejected by the CLI until the Opus encode/decode and network pipeline are complete.

## Desktop GUI Usage

When built with `SHAREAUDIO_ENABLE_DESKTOP_UI=ON`, the project also produces:

```bash
shareaudio_gui
```

The GUI exposes the same current functions as the CLI:

- Help/About for `shareaudio_cli help`.
- Local IPs panel for `shareaudio_cli ips`.
- Devices panel for `shareaudio_cli devices`.
- Share panel for `shareaudio_cli share`.
- Mode selector for `shareaudio_cli share --mode ultrafast`.
- Listen panel for `shareaudio_cli listen <host>`.

The GUI uses the same `SAL1` receiver autodetection path as the CLI. Quality Mode is shown as unavailable until Opus is implemented.

The GUI persists the last selected mode, host, capture device, playback device, and recent devices using the existing local JSON storage.

## Testing Real Audio

Use two different machines on the same LAN. Self-connections are blocked by design.

On the transmitter:

```bash
shareaudio_cli ips
shareaudio_cli devices
shareaudio_cli share
```

On the receiver:

```bash
shareaudio_cli listen <transmitter-ip>
```

For lower packet size and lower buffering target:

```bash
shareaudio_cli share --mode ultrafast
```

The receiver does not need a mode argument. It derives the mode from the stream header.

## Protocol Summary

Each TCP receiver connection starts with a native 16-byte stream header:

| Offset | Size | Field |
| --- | ---: | --- |
| 0 | 4 | Magic: `SAL1` |
| 4 | 1 | Version: `1` |
| 5 | 1 | Mode: `1` balanced, `2` ultrafast, `3` future quality |
| 6 | 1 | Codec: `1` pcm_s16le, `2` future opus |
| 7 | 1 | Channels: `2` |
| 8 | 1 | Bytes per sample: `2` |
| 9 | 2 | Packet size, Big-Endian |
| 11 | 1 | Reserved, currently `0` |
| 12 | 4 | Sample rate, Big-Endian |

After the header:

- Balanced Mode sends raw PCM chunks of exactly `2048` bytes.
- Ultrafast Mode sends raw PCM chunks of exactly `1024` bytes.
- Future Quality Mode will send Opus frames with a 2-byte Big-Endian frame length.

All PCM is normalized to 48,000 Hz, stereo, signed 16-bit little-endian samples.

The simplified native CLI does not support legacy no-header streams.

## Browser And Android Status

A browser cannot currently connect directly and listen to the native app. The current implementation uses raw TCP sockets, while browsers generally require WebSocket/WebRTC/HTTP-based transports.

Android/Web compatibility is intentionally not claimed yet. The existing Android/Web protocol must be reviewed before the native protocol is adjusted or bridged.

## Documentation

- `docs/ideia.md`: original source idea, kept as historical product input.
- `docs/PLAN.md`: implementation checklist and remaining work.
- `docs/PLAN-GUI.md`: GUI checklist.
- `docs/CURRENT_STATUS.md`: current technical state and known gaps.
- `CHANGELOG.md`: recent changes by implementation stage.
- `third_party/README.md`: pinned dependency and license notes.
