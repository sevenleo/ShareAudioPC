# ShareAudioLite Pending Plan

This checklist contains only remaining project work. Completed implementation history is tracked in `docs/CHANGELOG.md`, and current behavior is documented in `docs/README.md`.

## Linux Verification

- [ ] Add or restore Linux CMake presets.
- [ ] Confirm Linux configure step passes with Linux presets or documented manual CMake flags.
- [ ] Confirm Linux build passes.
- [ ] Confirm Linux automated tests pass.
- [ ] Produce a Linux executable artifact.
- [ ] Verify Linux capture on a PipeWire-based desktop.
- [ ] Verify Linux capture on a PulseAudio-based desktop if available.
- [ ] Verify Linux playback with a generated test tone.
- [ ] Verify Linux playback with received network PCM.
- [ ] Prefer PulseAudio/PipeWire monitor sources automatically when available.

## Manual Windows/Linux Streaming Tests

- [ ] Fix Quality Mode transmitter chunking so 20ms PCM frames are emitted to the Opus encoder.
- [ ] Add automated test proving Quality Mode transmitter produces Opus-framed packets from captured PCM.
- [ ] Fix or verify HTTP Opus fallback mode inference so `codec=opus` initializes the receiver as Quality/Opus.
- [ ] Verify Windows transmitter to Windows receiver.
- [ ] Verify Windows transmitter to Linux receiver.
- [ ] Verify Linux transmitter to Windows receiver.
- [ ] Verify Linux transmitter to Linux receiver.
- [ ] Verify Balanced Mode over wired LAN.
- [ ] Verify Balanced Mode over Wi-Fi.
- [ ] Verify Ultrafast Mode over wired LAN.
- [ ] Verify Ultrafast Mode over Wi-Fi.
- [ ] Verify Quality Mode over wired LAN.
- [ ] Verify Quality Mode over Wi-Fi.
- [ ] Verify Quality Mode Windows to Windows.
- [ ] Verify Quality Mode Linux to Linux.
- [ ] Verify Quality Mode Windows to Linux.
- [ ] Verify Quality Mode Linux to Windows.
- [ ] Verify multiple receiver clients.
- [ ] Verify Windows playback with a generated test tone.

## Android/Web Compatibility Tests

- [ ] Update the embedded browser player to read `/info` and adapt packet size/codec, or clearly restrict it in UI to Balanced PCM.
- [ ] Test Windows transmitter to Android receiver.
- [ ] Test Android transmitter to Windows receiver.
- [ ] Test Linux transmitter to Android/Web receiver where applicable.
- [ ] Test Android/Web transmitter to Linux receiver where applicable.
- [ ] Test browser playback through `http://<IP>:8080/`.
- [ ] Test `/info` metadata from a browser or HTTP client during an active stream.
- [ ] Test `/stream` playback/consumption during an active stream.

## GUI And CLI Manual Tests

- [ ] Update GUI Help/About text so it matches the current code paths for Quality/Opus, browser route, and Android/Web compatibility.
- [ ] Run manual GUI-to-CLI streaming test.
- [ ] Run manual CLI-to-GUI streaming test.
- [ ] Run manual GUI-to-GUI streaming test.
- [ ] Verify GUI default capture device selection on a clean Windows profile.
- [ ] Verify GUI default playback device selection on a clean Windows profile.
- [ ] Verify CLI `shareaudio.cfg` zero-argument server autostart in a release folder.
- [ ] Verify CLI `shareaudio.cfg` zero-argument client autostart in a release folder.
- [ ] Verify GUI `shareaudio.cfg` field prefill and autostart in a release folder.

## Reliability And Long-Running Behavior

- [ ] Validate or intentionally ignore non-zero `SAL1` reserved byte with an explicit protocol decision and test coverage.
- [ ] Add a dedicated automated test for receiver-side HTTP fallback (`/info` then `/stream`).
- [ ] Verify stable transmission for at least 10 minutes.
- [ ] Verify a long-running session of at least 1 hour.
- [ ] Verify receiver disconnect during playback.
- [ ] Verify transmitter shutdown while receiver is connected.
- [ ] Verify network drop during streaming.
- [ ] Verify receiver keeps reconnecting while the transmitter is offline.
- [ ] Verify receiver resumes playback when the transmitter returns.
- [ ] Verify audio device disconnect during capture.
- [ ] Verify audio device disconnect during playback.
- [ ] Verify capture recovers cleanly after stop/start.
- [ ] Verify capture works with common Windows output devices.
- [ ] Verify capture works when system audio is silent.
- [ ] Verify playback with received network PCM.
- [ ] Add integration test for rejected local self-connection.

## Performance And Latency

- [ ] Evaluate enabling `TCP_NODELAY` on sockets.
- [ ] Evaluate explicit socket send/receive buffer sizes such as 64 KB.
- [ ] Add per-client send timeout or asynchronous write queue if slow receivers can block broadcast too long.
- [ ] Reduce allocations and lock contention in the capture-callback-triggered transmitter path.
- [ ] Measure approximate end-to-end latency in Balanced Mode.
- [ ] Measure approximate end-to-end latency in Ultrafast Mode.
- [ ] Measure approximate end-to-end latency in Quality Mode.
- [ ] Measure CPU usage while transmitting PCM.
- [ ] Measure CPU usage while transmitting Opus.
- [ ] Measure memory usage during long sessions.
- [ ] Measure average send rate.
- [ ] Measure average receive rate.
- [ ] Tune packet queue sizes.
- [ ] Tune jitter buffer target size.
- [ ] Tune audio callback buffer size.
- [ ] Evaluate adaptive jitter behavior.

## Packaging And Release

- [ ] Confirm logs are useful for troubleshooting real user issues.
- [ ] Confirm manual test matrix is complete enough for first release.
- [ ] Confirm PCM modes are stable on real machines.
- [ ] Confirm Linux runtime package/dependency notes are complete.
- [ ] Confirm Windows release folder works on a clean machine without developer tools.
- [ ] Tag first MVP release.
