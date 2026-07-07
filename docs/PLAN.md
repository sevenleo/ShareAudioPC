# ShareAudioLite Implementation Plan

## Summary
Build **ShareAudioLite** as a cross-platform Windows/Linux application based on the original specification in `docs/ideia.md`. The application will work as both an audio **Transmitter** and **Receiver** over a local TCP network, supporting raw PCM low-latency modes and Opus quality mode.

## Current Implementation Snapshot
The current MVP is a native C++20 console application. It supports raw PCM sharing/listening over TCP with the simplified CLI, `SAL1` stream headers, and receiver mode autodetection. Quality Mode/Opus, browser listening, Android/Web compatibility, Linux verification, and manual cross-machine test coverage remain pending.

The planned implementation stack is:

- **Language:** C++20
- **Build system:** CMake
- **Audio:** miniaudio
- **Networking:** standalone Asio
- **Codec:** libopus
- **Default TCP port:** `8080`
- **Audio format:** 48,000 Hz, stereo, signed 16-bit PCM

## Phase 1 - Project Foundation
- [x] Create the `docs/PLAN.md` file.
- [x] Create the root CMake project.
- [x] Define C++20 as the required language standard.
- [x] Create the initial source tree:
  - [x] `src/app`
  - [x] `src/audio`
  - [x] `src/network`
  - [x] `src/protocol`
  - [x] `src/codec`
  - [x] `src/storage`
  - [x] `src/platform`
  - [x] `src/ui`
  - [x] `tests`
  - [x] `third_party`
- [x] Add a minimal executable target.
- [x] Add a minimal test target.
- [x] Add build presets for Windows debug/release.
- [x] Add build presets for Linux debug/release.
- [x] Add a basic logging utility.
- [x] Add a project-wide error/result type.
- [x] Verify that the empty application builds on Windows.
- [ ] Verify that the empty application builds on Linux.

## Phase 2 - Third-Party Dependencies
- [x] Add `miniaudio` to `third_party`.
- [x] Add `standalone Asio` to `third_party` or configure it through CMake.
- [x] Add `libopus` integration switch.
- [x] Add compile-time feature flags:
  - [x] `SHAREAUDIO_ENABLE_OPUS`
  - [x] `SHAREAUDIO_ENABLE_TESTS`
  - [x] `SHAREAUDIO_ENABLE_CONSOLE_UI`
  - [x] `SHAREAUDIO_ENABLE_DESKTOP_UI`
- [x] Document required Linux packages.
- [x] Document required Windows compiler/toolchain.
- [x] Confirm dependency licenses are acceptable.
- [x] Ensure dependencies are pinned to known versions.

## Phase 3 - Core Configuration Model
- [x] Define `AudioFormat`.
- [x] Define `AudioMode`.
- [x] Define `NetworkConfig`.
- [x] Define `TransmitterConfig`.
- [x] Define `ReceiverConfig`.
- [x] Define `AppConfig`.
- [x] Define default constants:
  - [x] TCP port `8080`
  - [x] sample rate `48000`
  - [x] channel count `2`
  - [x] sample format `signed 16-bit PCM`
  - [x] balanced packet size `2048 bytes`
  - [x] ultrafast packet size `1024 bytes`
  - [x] Opus bitrate `128 kbps CBR`
- [x] Add validation for invalid config values.
- [x] Add config serialization to JSON.
- [x] Add config loading from JSON.
- [x] Add fallback to defaults when config is missing.

## Phase 4 - Protocol Definition
- [x] Create a dedicated protocol module.
- [x] Define protocol responsibilities separately from networking.
- [x] Implement raw PCM packet handling.
- [x] Implement Balanced Mode packet size rules.
- [x] Implement Ultrafast Mode packet size rules.
- [x] Implement Opus frame header handling.
- [x] Encode Opus packet length as 2-byte Big-Endian.
- [x] Decode Opus packet length as 2-byte Big-Endian.
- [x] Implement native stream session header with `SAL1` magic.
- [x] Encode stream mode, codec, channel count, bytes per sample, sample rate, and packet size in the session header.
- [x] Decode and validate the native stream session header.
- [x] Reject invalid stream header magic, version, mode, codec, sample format, and packet size.
- [x] Add strict bounds checking for packet sizes.
- [x] Add protection against malformed packet length values.
- [x] Add protocol unit tests for PCM packet sizes.
- [x] Add protocol unit tests for Opus length prefix encoding.
- [x] Add protocol unit tests for native stream header encoding and decoding.
- [x] Add protocol unit tests for invalid native stream headers.
- [x] Add protocol unit tests for short reads and malformed packets.
- [x] Keep the protocol isolated so Android/Web compatibility can be adjusted without rewriting audio or UI code.

## Phase 5 - TCP Networking Foundation
- [x] Implement TCP server startup on port `8080`.
- [x] Implement TCP server shutdown.
- [x] Implement async client accept loop.
- [x] Support multiple connected receiver clients.
- [x] Send the native stream session header to every accepted receiver before audio bytes.
- [x] Implement client connection lifecycle tracking.
- [x] Implement TCP client connection by IP address.
- [x] Implement TCP client disconnect.
- [x] Implement exact byte reads.
- [x] Implement exact byte writes.
- [x] Add timeout handling for connect operations.
- [x] Add disconnect detection.
- [x] Add network error reporting.
- [x] Add graceful shutdown on application exit.
- [x] Add tests using localhost sockets where possible.

## Phase 6 - Local IP And Loop Prevention
- [x] Enumerate local IPv4 addresses.
- [x] Enumerate local IPv6 addresses.
- [x] Detect `localhost`.
- [x] Detect `127.0.0.1`.
- [x] Detect `::1`.
- [x] Detect any IP assigned to local network adapters.
- [x] Prevent receiver connection to the same machine.
- [x] Return a user-facing error for blocked self-connections.
- [x] Add unit tests for local IP matching.
- [ ] Add integration test for rejected local connection.

## Phase 7 - Audio Capture Abstraction
- [x] Define `IAudioCapture`.
- [x] Define capture lifecycle:
  - [x] initialize
  - [x] start
  - [x] stop
  - [x] shutdown
- [x] Define capture callback format.
- [x] Ensure captured data is normalized to 48 kHz stereo signed 16-bit PCM.
- [x] Add capture device enumeration API.
- [x] Add default capture device selection.
- [x] Add selected capture device support.
- [x] Add error handling for missing devices.
- [ ] Add error handling for device disconnect.
- [x] Add logging for capture start/stop/device errors.

## Phase 8 - Windows Audio Capture
- [x] Implement Windows capture backend using miniaudio with WASAPI where possible.
- [x] Implement system-audio loopback capture on Windows.
- [x] Use the default playback device as the default loopback source.
- [x] Support selecting a specific playback device for loopback capture.
- [x] Convert captured audio to the project PCM format when needed.
- [x] Handle device format mismatch.
- [ ] Handle capture device reset/disconnect.
- [ ] Verify capture works with common Windows output devices.
- [ ] Verify capture works when system audio is silent.
- [ ] Verify capture recovers cleanly after stop/start.

## Phase 9 - Linux Audio Capture
- [x] Implement Linux capture backend using miniaudio.
- [x] Support selecting an available capture device.
- [x] Document that Linux system-audio capture may require choosing a monitor/source device.
- [ ] Prefer PulseAudio/PipeWire monitor sources when available.
- [x] Support ALSA fallback where possible.
- [x] Convert captured audio to the project PCM format when needed.
- [x] Handle missing monitor source.
- [x] Handle device busy errors.
- [ ] Handle device disconnect.
- [ ] Verify capture on a PipeWire-based desktop.
- [ ] Verify capture on a PulseAudio-based desktop if available.

## Phase 10 - Audio Playback Abstraction
- [x] Define `IAudioPlayback`.
- [x] Define playback lifecycle:
  - [x] initialize
  - [x] start
  - [x] submit PCM frames
  - [x] stop
  - [x] shutdown
- [x] Add playback device enumeration API.
- [x] Add default playback device selection.
- [x] Add selected playback device support.
- [x] Add playback buffer management.
- [x] Add underrun detection.
- [x] Add overrun handling.
- [x] Add error handling for missing output device.
- [ ] Add error handling for output device disconnect.

## Phase 11 - Windows Audio Playback
- [x] Implement Windows playback backend using miniaudio/WASAPI.
- [x] Play 48 kHz stereo signed 16-bit PCM.
- [x] Support default playback device.
- [x] Support selected playback device.
- [x] Handle playback device format conversion.
- [x] Handle playback stop/start.
- [ ] Handle device disconnect.
- [ ] Verify playback with generated test tone.
- [ ] Verify playback with received network PCM.

## Phase 12 - Linux Audio Playback
- [x] Implement Linux playback backend using miniaudio.
- [x] Support PulseAudio/PipeWire/ALSA backends as available.
- [x] Play 48 kHz stereo signed 16-bit PCM.
- [x] Support default playback device.
- [x] Support selected playback device.
- [x] Handle playback device format conversion.
- [x] Handle playback stop/start.
- [ ] Handle device disconnect.
- [ ] Verify playback with generated test tone.
- [ ] Verify playback with received network PCM.

## Phase 13 - PCM Transmitter Pipeline
- [x] Connect audio capture to TCP server.
- [x] Implement a producer/consumer queue between capture and network.
- [x] Implement Balanced Mode packetization with 2048-byte chunks.
- [x] Implement Ultrafast Mode packetization with 1024-byte chunks.
- [x] Broadcast PCM packets to all connected clients.
- [x] Drop or backpressure slow clients without blocking capture.
- [x] Remove disconnected clients safely.
- [x] Track transmitted byte count.
- [x] Track connected client count.
- [x] Add logs for transmission start/stop.
- [ ] Verify stable transmission for at least 10 minutes.

## Phase 14 - PCM Receiver Pipeline
- [x] Connect TCP client to audio playback.
- [x] Implement read loop for raw PCM mode.
- [x] Read and validate the native stream session header before PCM packets.
- [x] Autodetect receiver stream mode and packet size from the session header.
- [x] Feed received PCM into jitter buffer.
- [x] Feed jitter buffer output into playback.
- [x] Handle short reads.
- [x] Handle server disconnect.
- [x] Handle playback underrun.
- [x] Add reconnect-safe shutdown.
- [x] Track received byte count.
- [x] Track current buffer depth.
- [ ] Verify receiver plays Windows transmitter audio.
- [ ] Verify receiver plays Linux transmitter audio.

## Phase 15 - Jitter Buffer
- [x] Implement fixed-size jitter buffer for first version.
- [x] Store PCM frames in a thread-safe buffer.
- [x] Define target buffer duration.
- [x] Define minimum buffer duration.
- [x] Define maximum buffer duration.
- [x] Add underrun behavior.
- [x] Add overflow behavior.
- [x] Add buffer reset on disconnect.
- [x] Add buffer metrics.
- [x] Add tests for enqueue/dequeue.
- [x] Add tests for underrun.
- [x] Add tests for overflow.
- [x] Add tests for reset.
- [ ] Later evaluate adaptive jitter behavior.

## Phase 16 - Opus Codec Integration
- [x] Add Opus encoder wrapper.
- [x] Add Opus decoder wrapper.
- [x] Configure encoder for 48 kHz stereo.
- [x] Configure encoder for 128 kbps CBR.
- [x] Define Opus frame size.
- [ ] Convert PCM input into Opus encoder frames.
- [ ] Convert Opus decoder output into PCM frames.
- [x] Handle encoder errors.
- [x] Handle decoder errors.
- [ ] Add tests with generated PCM input.
- [ ] Add tests for encode/decode roundtrip.
- [x] Add tests for malformed Opus frames.

## Phase 17 - Opus Network Mode
- [ ] Implement Quality Mode in transmitter.
- [ ] Encode captured PCM to Opus.
- [x] Prefix every Opus frame with 2-byte Big-Endian length.
- [ ] Send prefixed Opus frames over TCP.
- [ ] Implement Quality Mode in receiver.
- [x] Read 2-byte Big-Endian length.
- [ ] Read exact Opus frame bytes.
- [ ] Decode Opus frame into PCM.
- [ ] Send decoded PCM into jitter buffer.
- [x] Handle invalid frame length.
- [ ] Handle decoder failure without crashing.
- [ ] Verify Quality Mode Windows to Windows.
- [ ] Verify Quality Mode Linux to Linux.
- [ ] Verify Quality Mode Windows to Linux.
- [ ] Verify Quality Mode Linux to Windows.

## Phase 18 - Application Modes
- [x] Define app mode: idle.
- [x] Define app mode: transmitter.
- [x] Define app mode: receiver.
- [x] Prevent transmitter and receiver from conflicting when needed.
- [x] Implement start transmitter command.
- [x] Implement stop transmitter command.
- [x] Implement connect receiver command.
- [x] Implement disconnect receiver command.
- [x] Implement selected audio mode command.
- [x] Implement selected capture device command.
- [x] Implement selected playback device command.
- [x] Return clear errors for invalid state transitions.

## Phase 19 - Recent Devices Storage
- [x] Store recent transmitter IPs in local JSON.
- [x] Define storage path per platform.
- [x] Add recent IP after successful connection.
- [x] Avoid duplicate recent IP entries.
- [x] Limit recent IP list length.
- [x] Add clear history command.
- [x] Handle corrupted history file.
- [x] Handle missing history file.
- [x] Add tests for loading history.
- [x] Add tests for saving history.
- [x] Add tests for clearing history.

## Phase 20 - Console UI MVP
- [x] Add a simple console UI for early testing.
- [x] Replace implementation-oriented flags with simple user-facing subcommands:
  - [x] `share`
  - [x] `share --mode ultrafast`
  - [x] `listen <host>`
  - [x] `devices`
  - [x] `ips`
  - [x] `help`
- [x] Default `share` to Balanced Mode when no mode is provided.
- [x] Autodetect receiver mode from the native stream session header.
- [x] Remove legacy CLI flags from help and parser:
  - [x] `--status`
  - [x] `--start-transmitter`
  - [x] `--connect`
  - [x] `--transmit-pcm`
  - [x] `--receive-pcm`
  - [x] `--list-ips`
  - [x] `--list-audio-devices`
- [x] Show current local IP addresses.
- [x] Allow starting transmitter.
- [x] Allow selecting supported PCM modes:
  - [x] Balanced
  - [x] Ultrafast
- [x] Reject Quality Mode with a clear error until Opus network mode is implemented.
- [x] Allow connecting receiver by IP.
- [x] Allow disconnecting receiver.
- [x] Show connected clients.
- [x] Show current audio device names.
- [x] Show transmission status.
- [x] Show receiver buffer status.
- [x] Show errors in readable form.
- [x] Use console UI as the first cross-platform control surface.

## Phase 21 - Desktop UI Planning
- [x] Keep desktop UI separate from core logic.
- [ ] Choose desktop UI only after console MVP is stable.
- [x] Preserve three main sections:
  - [x] Transmitter
  - [x] Receiver
  - [x] Settings
- [x] Expose the same commands used by console UI.
- [ ] Show transmitter status.
- [ ] Show local IP with copy action.
- [ ] Show audio mode selection.
- [ ] Show receiver connection field.
- [ ] Show recent devices.
- [ ] Show selected capture/playback devices.
- [x] Apply Soundwave visual identity:
  - [x] Neon Green `#1DF09A`
  - [x] Cyan Blue `#00A3FF`
  - [x] Dark Slate `#1C253E`
  - [x] Deep Space `#0B101D`

## Phase 22 - Compatibility With Android/Web
- [ ] Obtain the existing Android/Web protocol implementation.
- [ ] Identify the real mode selection behavior.
- [ ] Identify whether protocol negotiation exists.
- [ ] Confirm PCM signedness.
- [ ] Confirm PCM endianness.
- [ ] Confirm Opus frame duration.
- [ ] Confirm packet framing for each mode.
- [ ] Confirm whether Web client supports raw PCM, Opus, or both.
- [ ] Add compatibility notes to documentation.
- [ ] Adjust protocol module only, avoiding audio/network rewrites.
- [ ] Test Windows transmitter to Android receiver.
- [ ] Test Android transmitter to Windows receiver.
- [ ] Test Linux transmitter to Android/Web receiver where applicable.
- [ ] Test Android/Web transmitter to Linux receiver where applicable.

## Phase 23 - Diagnostics And Observability
- [x] Add log levels:
  - [x] error
  - [x] warning
  - [x] info
  - [x] debug
- [x] Log audio backend selection.
- [x] Log selected audio devices.
- [x] Log network connections.
- [x] Log network disconnections.
- [x] Log protocol errors.
- [x] Log codec errors.
- [x] Log buffer underruns.
- [x] Log buffer overruns.
- [x] Add optional runtime stats:
  - [x] connected clients
  - [x] bytes sent
  - [x] bytes received
  - [x] current jitter buffer depth
  - [x] dropped packets/frames
  - [ ] average send rate
  - [ ] average receive rate

## Phase 24 - Error Handling
- [x] Define user-facing error messages.
- [x] Define internal error codes.
- [x] Handle port already in use.
- [x] Handle firewall/network binding failure.
- [x] Handle unreachable host.
- [x] Handle connection refused.
- [x] Handle connection reset.
- [x] Handle audio device unavailable.
- [x] Handle unsupported audio format.
- [x] Handle Opus initialization failure.
- [x] Handle malformed packets.
- [x] Handle corrupted config files.
- [x] Ensure failures do not crash the application.
- [x] Ensure stop/shutdown paths are idempotent.

## Phase 25 - Automated Tests
- [x] Add unit tests for protocol packet framing.
- [x] Add unit tests for config validation.
- [x] Add unit tests for recent device storage.
- [x] Add unit tests for local IP detection.
- [x] Add unit tests for jitter buffer behavior.
- [x] Add unit tests for Opus wrapper when enabled.
- [x] Add network integration tests.
- [x] Add loopback client/server integration test.
- [x] Add generated PCM test source.
- [x] Add playback-free receiver test using a fake audio sink.
- [x] Add transmitter test using a fake audio source.
- [x] Add CI-friendly tests that do not require real audio hardware.

## Phase 26 - Manual Test Matrix
- [ ] Windows transmitter to Windows receiver.
- [ ] Windows transmitter to Linux receiver.
- [ ] Linux transmitter to Windows receiver.
- [ ] Linux transmitter to Linux receiver.
- [ ] Balanced Mode over wired LAN.
- [ ] Balanced Mode over Wi-Fi.
- [ ] Ultrafast Mode over wired LAN.
- [ ] Ultrafast Mode over Wi-Fi.
- [ ] Quality Mode over wired LAN.
- [ ] Quality Mode over Wi-Fi.
- [ ] Multiple receiver clients.
- [ ] Receiver disconnect during playback.
- [ ] Transmitter shutdown while receiver is connected.
- [ ] Network drop during streaming.
- [ ] Audio device disconnect during capture.
- [ ] Audio device disconnect during playback.
- [ ] Silent system audio.
- [ ] High-volume system audio.
- [ ] Long-running session of at least 1 hour.

## Phase 27 - Performance And Latency
- [ ] Measure approximate end-to-end latency in Balanced Mode.
- [ ] Measure approximate end-to-end latency in Ultrafast Mode.
- [ ] Measure approximate end-to-end latency in Quality Mode.
- [ ] Measure CPU usage while transmitting PCM.
- [ ] Measure CPU usage while transmitting Opus.
- [ ] Measure memory usage during long sessions.
- [ ] Tune packet queue sizes.
- [ ] Tune jitter buffer target size.
- [ ] Tune audio callback buffer size.
- [x] Avoid allocations inside audio callbacks.
- [x] Avoid blocking network writes inside audio callbacks.
- [x] Document known latency trade-offs.

## Phase 28 - Packaging
- [x] Add Windows release build instructions.
- [x] Add Linux release build instructions.
- [x] Produce Windows executable artifact.
- [ ] Produce Linux executable artifact.
- [x] Document required runtime files.
- [x] Document optional config file location.
- [x] Document firewall notes for TCP port `8080`.
- [x] Document Linux audio source selection notes.
- [x] Add version number to application.
- [x] Add build metadata to logs.

## Phase 29 - Documentation
- [x] Write `README.md`.
- [x] Write `CHANGELOG.md`.
- [x] Write `docs/CURRENT_STATUS.md`.
- [x] Document project goals.
- [x] Document supported platforms.
- [x] Document build steps.
- [x] Document run steps.
- [x] Document Transmitter usage.
- [x] Document Receiver usage.
- [x] Document audio modes.
- [x] Document known Linux audio limitations.
- [x] Document Android/Web compatibility status.
- [x] Document troubleshooting.
- [x] Document protocol summary.
- [x] Document simplified CLI commands.
- [x] Document `SAL1` stream session header behavior.
- [x] Document current browser/Android/Web status.
- [x] Document current Opus/Quality Mode limitation.
- [x] Keep `docs/ideia.md` as the original source idea with a current-state note.
- [x] Keep `docs/PLAN.md` as the implementation checklist.

## Phase 30 - Release Readiness
- [x] Confirm Windows build passes.
- [ ] Confirm Linux build passes.
- [x] Confirm automated tests pass.
- [ ] Confirm manual test matrix is complete enough for first release.
- [ ] Confirm PCM modes are stable.
- [x] Confirm Opus mode is stable or clearly marked experimental.
- [ ] Confirm logs are useful for troubleshooting.
- [x] Confirm configuration survives restart.
- [x] Confirm recent devices survive restart.
- [x] Confirm no known crash on normal disconnect paths.
- [ ] Tag first MVP release.

## Public Interfaces And Internal Contracts
- [x] `IAudioCapture` provides normalized PCM frames.
- [x] `IAudioPlayback` consumes normalized PCM frames.
- [x] `StreamHeader` describes the native stream mode, codec, format, and packet size.
- [x] `ProtocolWriter` serializes PCM and Opus packets.
- [x] `ProtocolReader` deserializes PCM and Opus packets.
- [x] `ProtocolWriter` serializes the native stream session header.
- [x] `ProtocolReader` deserializes and validates the native stream session header.
- [x] `TcpTransmitterServer` manages receiver clients.
- [x] `TcpReceiverClient` manages one transmitter connection.
- [x] `OpusEncoder` accepts PCM and returns Opus frames.
- [x] `OpusDecoder` accepts Opus frames and returns PCM.
- [x] `JitterBuffer` decouples network timing from playback timing.
- [x] UI layers call application commands and do not directly own audio/network internals.

## Acceptance Criteria
- [x] The project builds from source on Windows.
- [ ] The project builds from source on Linux.
- [ ] A Windows transmitter can stream PCM to a Windows receiver.
- [ ] A Linux transmitter can stream PCM to a Linux receiver.
- [ ] Cross-platform Windows/Linux PCM streaming works.
- [x] Opus Quality Mode works or is explicitly marked incomplete.
- [x] Receiver autodetects stream mode from the native stream session header.
- [x] Receiver blocks self-connections.
- [x] Recent devices are persisted.
- [x] Network disconnects do not crash the app.
- [x] Audio device errors do not crash the app.
- [x] The implementation remains compatible with the protocol direction from `docs/ideia.md`.

## Assumptions
- The first implementation target is a console-controlled MVP, with desktop UI added after core streaming is stable.
- Raw PCM modes should be implemented before Opus.
- Android/Web source code will be reviewed before final compatibility is claimed.
- Linux system-audio capture may require selecting a monitor/source device depending on the user's audio stack.
- The native Windows/Linux CLI protocol now starts with a `SAL1` stream session header and does not support legacy no-header streams.
- The protocol may need minor adjustments after validating the existing Android/Web implementation.
