# Third-Party Dependencies

This directory stores pinned third-party source dependencies.

Pinned dependencies:

- `miniaudio-src`: miniaudio `0.11.25`, commit `9634bedb5b5a2ca38c1ee7108a9358a4e233f14d`.
- `asio-src`: standalone Asio `1.38.1`, commit `bbecff21a23b97c34641f0f1f08b28c91b9c77cf`.

Optional/system dependency:

- `libopus` headers/libraries when `SHAREAUDIO_ENABLE_OPUS=ON`.

Current Opus status:

- CMake can try to discover and link libopus when `SHAREAUDIO_ENABLE_OPUS=ON`.
- The ShareAudioLite Opus encode/decode wrapper is still incomplete and returns `NotSupported` for actual encode/decode calls.
- The CLI intentionally rejects Quality Mode until the Opus network pipeline is implemented.

License notes:

- miniaudio is available under Public Domain/Unlicense or MIT-0.
- standalone Asio uses the Boost Software License 1.0.
- libopus uses its own BSD-style license when installed by the platform/package manager.
