# ShareAudioLite

ShareAudioLite is a cross-platform Windows/Linux LAN audio transmitter and receiver based on the original idea in `docs/ideia.md`.

The current codebase is an initial C++20 foundation: configuration, protocol framing, TCP primitives, local IP loop prevention, fake audio abstractions, jitter buffering, recent device storage, a console entrypoint, and CI-friendly tests. Real hardware audio capture/playback through miniaudio and full libopus encoding/decoding are the next implementation steps.

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

## Dependencies

Required now:

- CMake 3.24+
- C++20 compiler

Planned runtime/audio dependencies:

- `miniaudio` vendored in `third_party`
- standalone Asio vendored or provided by the system
- optional `libopus` for Quality Mode

Linux audio note: system-audio capture usually requires selecting a monitor/source device exposed by PulseAudio or PipeWire.

## Console Usage

```bash
shareaudio_cli --help
shareaudio_cli --list-ips
shareaudio_cli --status
```

## Protocol Summary

- Balanced Mode: raw PCM chunks of exactly `2048` bytes.
- Ultrafast Mode: raw PCM chunks of exactly `1024` bytes.
- Quality Mode: each Opus frame is prefixed by a 2-byte Big-Endian frame length.

All PCM is normalized to 48,000 Hz, stereo, signed 16-bit little-endian samples unless Android/Web compatibility validation later requires a protocol adjustment.
