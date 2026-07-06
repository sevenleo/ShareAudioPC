# Third-Party Dependencies

This directory is reserved for pinned third-party source dependencies.

Planned dependencies:

- `miniaudio` for cross-platform audio capture/playback.
- `standalone Asio` for asynchronous TCP networking.
- `libopus` headers/libraries when `SHAREAUDIO_ENABLE_OPUS=ON`.

The current implementation keeps the core build dependency-free so protocol, config, storage, TCP primitives, and tests can compile before vendoring external packages.
