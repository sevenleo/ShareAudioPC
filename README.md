# ShareAudioLite

ShareAudioLite is a cross-platform Windows/Linux LAN audio transmitter and receiver based on the original idea in `docs/ideia.md`.

The project is built as a native C++20 application with a console CLI (`shareaudio_cli`) and an optional Qt Widgets desktop GUI (`shareaudio_gui`). It can share and listen to low-latency raw PCM or Opus compressed audio over TCP on a local network.

---

## 🛠️ Technical Architecture & Pipeline

The system uses a producer-consumer architecture separated into two distinct pipelines (Transmitter and Receiver) decoupled by thread-safe queues and a network interface.

### System Diagram

```mermaid
graph TD
    subgraph Transmitter (Server)
        A[Miniaudio Capture/Loopback] -->|48kHz Stereo S16LE| B[PcmChunker]
        B -->|Fixed Chunks| C[PcmTransmitterPipeline Queue]
        C -->|Transmitter Worker Thread| D[PcmBroadcastServer]
        D -->|TCP Port 8080| E((LAN Network))
    end

    subgraph Receiver (Client)
        E -->|TCP Socket Connection| F[PcmReceiverPipeline]
        F -->|Read SAL1 Header & Parse| G[Receiver Worker Thread]
        G -->|Read Audio Packets| H[JitterBuffer]
        H -->|Audio Feed| I[IAudioPlayback - Miniaudio]
    end
```

---

## 📡 Transmitter (Server) Architecture & Flow

When you run `shareaudio_cli share` (or click "Start Sharing" in the GUI), the following pipeline is initiated:

1. **System Audio Capture**:
   - On Windows, a `MiniaudioCapture` loopback context is initialized. It captures whatever is playing on the default playback device (using WASAPI loopback).
   - On Linux, it captures from the default capture device (ALSA/PulseAudio/PipeWire).
   - Captured frames are standard **48 kHz, Stereo, Signed 16-bit PCM** (4 bytes per frame).

2. **Chunking & Packetization**:
   - The audio callback feeds raw captured bytes into a `PcmChunker`.
   - `PcmChunker` groups the arbitrary incoming frame sizes into fixed-size chunks:
     - **Balanced Mode**: exactly `2048` bytes (512 samples/channel).
     - **Ultrafast Mode**: exactly `1024` bytes (256 samples/channel).
     - **Quality Mode (Opus)**: 20ms chunks (`3840` bytes) which are passed to the `OpusEncoder` wrapper.

3. **Buffering & Queueing**:
   - Chunked packets are pushed to a thread-safe FIFO queue (`PcmTransmitterPipeline`) with a maximum capacity (default `256` packets).
   - If a client is slow and the queue fills up, older packets are dropped to prioritize low latency (backpressure prevention).

4. **Network Broadcasting**:
   - The `PcmBroadcastServer` listens on TCP port `8080`.
   - An asynchronous listener accepts incoming client sockets.
   - When a receiver connects, the server immediately sends a **16-byte `SAL1` Stream Session Header**.
   - A dedicated `transmitter_worker_` thread pops packets from the pipeline queue and broadcasts the raw bytes to all connected TCP clients.

---

## 🎧 Receiver (Client) Architecture & Flow

When you run `shareaudio_cli listen <host>` (or click "Connect" in the GUI):

1. **TCP Connection**:
   - The `TcpSocket` initiates a connection to the transmitter on port `8080` (with a 5-second timeout).
   - Once connected, the client reads exactly 16 bytes to retrieve the `SAL1` header.

2. **Protocol Negotiation**:
   - The client parses and validates the header (deriving codec, sample rate, packet size, and mode).
   - If the header is invalid or uses an unsupported codec, the connection is terminated.
   - It instantiates a `PcmReceiverPipeline` and initializes the local `IAudioPlayback` device (miniaudio playback) configured to match the stream sample rate and channel layout.

3. **The Read Loop & Auto-Reconnection**:
   - The dedicated `listener_worker_` thread loops, performing exact socket reads of size `packet_size`.
   - **Reconnection Logic**: If a socket read fails (due to a server crash, reboot, or network drop), the receiver does not exit. Instead:
     - It stops the local playback device and resets the jitter buffer to prevent looping static/garbage sound.
     - It enters the `SessionMode::Connecting` state.
     - It enters a loop trying to reconnect to the host every 1.5 seconds.
     - Once the server returns online, it reads the new `SAL1` header, re-initializes the playback device, and resumes playing audio.

4. **Jitter Buffering & Playback**:
   - Received packets are pushed into a thread-safe `JitterBuffer` (stores up to a predefined capacity in bytes).
   - The playback backend reads from the `JitterBuffer` in the audio output thread.
   - If the network experiences delay, the buffer pads with silent bytes (underrun handling) to keep the audio stream continuous.

---

## 💾 Binary Network Protocol Specification (`SAL1`)

The protocol is designed to run over raw TCP. Each connection starts with a **16-byte fixed-size metadata header**:

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                      Magic: 'S' 'A' 'L' '1'                   |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|    Version    |      Mode     |     Codec     |   Channels    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|BytesPerSample |          Packet Size          |   Reserved    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                          Sample Rate                          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### Header Fields Detail

| Offset (Bytes) | Size (Bytes) | Type | Field Description |
| :--- | :---: | :---: | :--- |
| `0 - 3` | 4 | `char[4]` | Protocol Magic: Must be `"SAL1"` |
| `4` | 1 | `uint8_t` | Version: Must be `1` |
| `5` | 1 | `uint8_t` | Stream Mode: `1` = Balanced, `2` = Ultrafast, `3` = Quality |
| `6` | 1 | `uint8_t` | Codec ID: `1` = Raw PCM (s16le), `2` = Opus |
| `7` | 1 | `uint8_t` | Channels: Fixed at `2` (Stereo) |
| `8` | 1 | `uint8_t` | Bytes Per Sample: Fixed at `2` (16-bit) |
| `9 - 10` | 2 | `uint16_t` | Packet Size (**Big-Endian**): `2048` (Balanced) or `1024` (Ultrafast). Set to `0` for Opus. |
| `11` | 1 | `uint8_t` | Reserved: `0` |
| `12 - 15` | 4 | `uint32_t` | Sample Rate (**Big-Endian**): Fixed at `48000` |

### Audio Data Transmission Payload

Following the 16-byte header, data is transmitted continuously as follows:

* **Raw PCM Modes (Balanced / Ultrafast)**:
  - Consists of a continuous stream of raw PCM bytes.
  - The client reads exactly `Packet Size` bytes on each iteration.
  - Byte order of sample values is **Little-Endian** signed 16-bit integers (`s16le`).

* **Quality Mode (Opus)**:
  - Consists of successive compressed Opus packets.
  - Each Opus packet is preceded by a **2-byte Big-Endian length header**:
    ```
    +----------------------------------+------------------------------+
    | 2-Byte Big-Endian Payload Length | Compressed Opus Packet Data  |
    +----------------------------------+------------------------------+
    ```
  - The client first reads 2 bytes, decodes the length $N$, then reads exactly $N$ bytes of compressed data to pass to the decoder.

---

## 🔐 Concurrency & Thread-Safety Contracts

- **No Allocations in Audio Callbacks**: Audio capture and playback threads operate on preallocated memory and circular buffers to prevent latency spikes caused by garbage collection or heap allocation.
- **Mutex Isolation**: Lock contention is minimized in the `SessionController` by using separate locks for state transitions, logging, and statistics.
- **Socket Decoupling**: Sockets run on dedicated threads (`transmitter_worker_` and `listener_worker_`) which communicate with the audio thread only through thread-safe pipeline queues, preventing network blockages from choking the audio system.
- **Single-Instance Execution**: Enforced at startup using cross-platform PID tracking. Writes current PID to `shareaudio.pid`. When a new instance runs, it sends a kill signal/command to terminate the old process.

---

## 🚀 Running the Project

### Building

Ensure CMake 3.24+ is installed.

#### Debug build (with Opus enabled):
```powershell
cmake --preset windows-debug -DSHAREAUDIO_ENABLE_OPUS=ON
cmake --build --preset windows-debug
ctest --preset windows-debug
```

#### Release build (with Opus enabled & statically linked for distribution):
```powershell
cmake --preset windows-release -DSHAREAUDIO_ENABLE_OPUS=ON
cmake --build --preset windows-release
```

#### Building the Desktop GUI (with Brand Logo and Assets):
To compile the portable GUI and avoid C-runtime/allocator heap conflicts, configure the build using the Qt MinGW toolchain:
```powershell
# Configure release build using the matching Qt compiler
cmake --preset windows-gui-release -DCMAKE_C_COMPILER="d:/GITHUB/ShareAudioPC_2/qt6/Tools/mingw1310_64/bin/gcc.exe" -DCMAKE_CXX_COMPILER="d:/GITHUB/ShareAudioPC_2/qt6/Tools/mingw1310_64/bin/g++.exe" -DCMAKE_MAKE_PROGRAM="d:/GITHUB/ShareAudioPC_2/qt6/Tools/mingw1310_64/bin/mingw32-make.exe" -DCMAKE_PREFIX_PATH="d:/GITHUB/ShareAudioPC_2/qt6/6.6.3/mingw_64" -DSHAREAUDIO_ENABLE_OPUS=ON

# Compile the release binaries
cmake --build build/windows-gui-release --config Release

# Deploy Qt DLL dependencies using windeployqt
d:\GITHUB\ShareAudioPC_2\qt6\6.6.3\mingw_64\bin\windeployqt.exe D:\GITHUB\ShareAudioPC_2\build\windows-gui-release\shareaudio_gui.exe
```

---

## 🖥️ Desktop GUI Features
The GUI target (`shareaudio_gui.exe`) has been personalized and styled to look modern, clean, and professional:
- **Redesigned 3-Line Simple Mode**: To keep things extremely simple, the app opens in a resizable `830x310` window with precisely three clean lines:
  - **Line 1 (Status)**: Displays the current application state, server/client IP details dynamically depending on the mode, TCP Port, and the last error message or status event.
  - **Line 2 (Server)**: A single green/red button to start or stop sharing the system sound.
  - **Line 3 (Client)**: A clean line edit to enter the transmitter's IP address and a single blue/red connect/disconnect button to listen to the broadcast.
- **Default Device Auto-Selection**: On launch, the app automatically pre-selects the system's default capture (microphone/loopback) and playback (speaker) audio devices, highlighting them in the settings, so users don't have to worry about selecting the wrong sound card.
- **Interactive Toggle**: Clicking **"Show Advanced Options"** resizes the window to `1100x730` and exposes:
  - **Quality mode selection** (Balanced / Ultrafast).
  - **Selected capture and playback audio devices** (via combobox dropdowns).
  - **Full Network & Devices tabs** (including local IPs list and connection history).
  - **System Logs and Stats** (under the Diagnostics & Help tab).
- **Embedded Brand Identity**: The project icon (`logo.ico`) is embedded directly into the Windows executable binary, and the UI features dynamic neon-themed buttons and dark mode styling.

---

### CLI Command Options

* **Start Audio Server (Transmitter)**:
  ```bash
  # Shares system audio on port 8080 (defaults to Balanced Mode)
  shareaudio_cli share
  
  # Shares using Ultrafast mode for minimal latency
  shareaudio_cli share --mode ultrafast
  
  # Shares using a specific capture device
  shareaudio_cli share --device "playback:1"
  ```
  *When started, it will list all available local IP addresses automatically.*

* **Start Audio Client (Receiver)**:
  ```bash
  # Connects and starts listening to the host (autodetects mode from SAL1 header)
  shareaudio_cli listen 192.168.1.150
  
  # Listens using a specific playback device
  shareaudio_cli listen 192.168.1.150 --device "playback:2"
  ```

* **Inspect Devices**:
  ```bash
  shareaudio_cli devices
  ```

---

## ⚙️ Portable Startup Configuration (`shareaudio.cfg`)

You can place a configuration file named `shareaudio.cfg` in the same directory as the executable (`shareaudio_cli.exe` or `shareaudio_gui.exe`) to configure and auto-start sessions without user intervention.

### Example `shareaudio.cfg`
```ini
# ShareAudioLite Startup Configuration
# Place this next to the executable (CLI or GUI)

# Master switch (must be true to enable autostart)
AUTOSTART=true

# Mode: 'server' (transmitting) or 'client' (receiving)
MODE=server

# Audio quality mode: 'balanced', 'ultrafast', or 'quality'
SHARE_QUALITY=balanced

# Optional hardware devices IDs
# DEVICE_ID=
# PLAYBACK_DEVICE_ID=

# Server IP/Host (Required only when MODE=client)
SERVER_IP=192.168.1.100
```

### Features & Behavior:
- **AUTOSTART Switch**: If `AUTOSTART` is `false` or missing, the file is loaded but no session is automatically started.
- **GUI Autostart**: If `AUTOSTART=true` and settings are valid, the GUI automatically opens and initiates the session (sharing or connecting) after a brief initialization window.
- **GUI Pre-Fill**: Regardless of the `AUTOSTART` setting, the fields in the GUI (Host/IP, capture device, playback device, quality mode) are pre-filled with the values defined in the file.
- **CLI Behavior**: Running `shareaudio_cli` with no arguments loads `shareaudio.cfg` and initiates the connection or broadcast. If any explicit command line arguments are provided (e.g. `shareaudio_cli share`), the configuration file is **completely ignored**.

