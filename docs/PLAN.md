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

- [ ] Verify Windows transmitter to Windows receiver.
- [ ] Verify Windows transmitter to Linux receiver.
- [ ] Verify Linux transmitter to Windows receiver.
- [ ] Verify Linux transmitter to Linux receiver.
- [ ] Verify Balanced Mode over wired LAN.
- [ ] Verify Balanced Mode over Wi-Fi.
- [ ] Verify Fast AudioMode over wired LAN.
- [ ] Verify Fast AudioMode over Wi-Fi.
- [ ] Verify Efficient AudioMode over wired LAN.
- [ ] Verify Efficient AudioMode over Wi-Fi.
- [ ] Verify Efficient AudioMode Windows to Windows.
- [ ] Verify Efficient AudioMode Linux to Linux.
- [ ] Verify Efficient AudioMode Windows to Linux.
- [ ] Verify Efficient AudioMode Linux to Windows.
- [ ] Verify multiple receiver clients.
- [ ] Verify Windows playback with a generated test tone.

## Android/Web Compatibility Tests

- [ ] Test Windows transmitter to Android receiver.
- [ ] Test Android transmitter to Windows receiver.
- [ ] Test Linux transmitter to Android/Web receiver where applicable.
- [ ] Test Android/Web transmitter to Linux receiver where applicable.
- [ ] Test browser playback through `http://<IP>:33777/` with Fast AudioMode.
- [ ] Test browser playback through `http://<IP>:33777/` with Balanced AudioMode.
- [ ] Test browser playback through `http://<IP>:33777/` with Efficient AudioMode and confirm the unsupported-mode message is shown.
- [ ] Test `/info` metadata from a browser or HTTP client during an active stream.
- [ ] Test `/stream` playback/consumption during an active stream.
- [ ] Test one browser receiver and one native receiver connected to the same transmitter at the same time.

## GUI And CLI Manual Tests

- [ ] Run manual GUI-to-CLI streaming test.
- [ ] Run manual CLI-to-GUI streaming test.
- [ ] Run manual GUI-to-GUI streaming test.
- [ ] Verify GUI default capture device selection on a clean Windows profile.
- [ ] Verify GUI default playback device selection on a clean Windows profile.
- [ ] Verify CLI `shareaudio.cfg` zero-argument server autostart in a release folder.
- [ ] Verify CLI `shareaudio.cfg` zero-argument client autostart in a release folder.
- [ ] Verify GUI `shareaudio.cfg` field prefill and autostart in a release folder.
- [ ] Verify GUI simultaneous sharing and receiver operation.
- [ ] Verify GUI `shareaudio.cfg` `AUTOSTART=true` with `MODE=both` and `SERVER_IP=<ip>`.
- [ ] Verify GUI `TRAYMODE=true` minimizes to the system tray.
- [ ] Verify GUI `TRAYMODE=true` close button hides to the system tray without stopping the session.
- [ ] Verify GUI tray menu restores the window and exits the app.
- [ ] Verify GUI `STARTINTRAY=true` starts hidden in the system tray.

## Reliability And Long-Running Behavior

- [ ] Validate or intentionally ignore non-zero `SAL1` reserved byte with an explicit protocol decision and test coverage.
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
- [ ] Evaluate adding a per-client write timeout for permanently stuck sockets.
- [ ] Reduce allocations and lock contention in the capture-callback-triggered transmitter path.
- [ ] Measure approximate end-to-end latency in Balanced Mode.
- [ ] Measure approximate end-to-end latency in Fast AudioMode.
- [ ] Measure approximate end-to-end latency in Efficient AudioMode.
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
