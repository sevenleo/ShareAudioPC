# PLAN-FULL-SYNC.md - Desktop and Mobile Compatibility Plan

This document details the synchronization plan and technical specifications agreed upon between the **Desktop** (C++20) and **Mobile** (Android) development teams to achieve 100% bidirectional compatibility for **ShareAudioLite**.

---

## 🤝 1. The Agreement: Option C (Hybrid Auto-Detect Protocol)

Both teams have agreed to implement **Option C (Hybrid Protocol with Auto-Detection)**. 

### Why this option was chosen:
1. **Low-Latency Efficiency**: Desktop-to-Desktop and Mobile-to-Mobile native connections remain pure TCP with zero HTTP overhead and no JSON parsing inside the audio network loop, using the 16-byte `SAL1` header.
2. **Browser Compatibility**: Mobile and Desktop transmitters retain their HTTP `/stream`, `/info` and `/` endpoints, allowing any standard web browser to connect and listen directly via Web Audio API.
3. **Cross-Platform Transparency**: A Desktop receiver can connect to an Android transmitter, and an Android receiver can connect to a Desktop transmitter out-of-the-box.

---

## 🌐 2. Handshake Protocol & Flow Chart

```
                        New Client Socket Connection
                                     │
                        ┌────────────┴────────────┐
                        │ Read first 4 bytes with  │
                        │   150ms timeout window  │
                        └────────────┬────────────┘
                                     │
                     ┌───────────────┴───────────────┐
                     ▼                               ▼
             Bytes received?                 Timeout expired?
                     │                               │
         ┌───────────┴───────────┐                   │
         ▼                       ▼                   ▼
   Bytes == "GET "         Bytes != "GET "     Treat as NATIVE
   (HTTP Request)          (Custom client)     (SAL1 Receiver)
         │                       │                   │
         ▼                       ▼                   ▼
 ┌───────────────┐       ┌───────────────┐   ┌───────────────┐
 │ Process Route │       │ Send 16-Byte  │   │ Send 16-Byte  │
 │  HTTP Response│       │  SAL1 Header  │   │  SAL1 Header  │
 └───────────────┘       └───────────────┘   └───────────────┘
```

---

## 💻 3. Changes Required in the Desktop Application (C++)

### 3.1. Server Modification (`PcmBroadcastServer`)
- **First-byte Preview**: Upon accepting a new socket, do not write the `SAL1` header immediately. Start an asynchronous read operation of 4 bytes (`socket.async_read_some`) alongside a `steady_timer` set to 150ms.
- **HTTP Mode Handling**: If 4 bytes are received and match `"GET "` (hex `0x47 0x45 0x54 0x20`):
  - Read the rest of the HTTP request headers.
  - Parse the endpoint path:
    - **`/info`**: Send HTTP headers (`Content-Type: application/json`, `Connection: close`) and return the active session configuration parameters in JSON:
      ```json
      {
        "status": "streaming",
        "connectedClients": 1,
        "sampleRate": 48000,
        "channels": 2,
        "codec": "pcm", 
        "bitrate": 128000,
        "chunkSize": 2048
      }
      ```
    - **`/stream`**: Send HTTP headers (`Content-Type: application/octet-stream`, `Connection: keep-alive`, `Cache-Control: no-cache`) and directly stream raw audio bytes (omitting the 16-byte `SAL1` header).
    - **`/`**: Serve a basic inline HTML5 player page with a JavaScript script that decodes the `/stream` using the Web Audio API.
- **Native Mode Handling**: If the timer expires before receiving 4 bytes, or if the bytes received do not match `"GET "`:
  - Send the 16-byte binary `SAL1` stream header immediately.
  - Resume native raw binary transmission.

### 3.2. Receiver Modification (`SessionController` client thread)
- **Silent Handshake Connection**: Connect to the server socket and do not send any data. Wait silently to read exactly 16 bytes.
- **Autodetect HTTP Fallback**:
  - If the socket closes or receives data that is not a valid `SAL1` header (e.g. HTTP headers):
    - Close the socket.
    - Connect again to `<IP>:8080/info` to fetch the metadata JSON.
    - Parse the JSON to determine the active codec (`pcm` or `opus`).
    - Connect again to `<IP>:8080/stream` using an HTTP `GET /stream HTTP/1.1\r\n\r\n` request, discard the response headers (up to `\r\n\r\n`), and begin reading the audio stream.

---

## 📱 4. Changes Required in the Mobile Application (Android)

### 4.1. Server Modification (Android Server Socket)
- **Preview & Timeout**: Modify the server's client connection handling. Instead of reading lines immediately (which blocks waiting for `\r\n`), check the first 4 bytes of input.
- **Auto-Detect Trigger**:
  - If the input starts with `"GET "`, delegate the socket to the existing HTTP handler thread.
  - If the client stays silent (timeout of 150ms) or sends non-HTTP bytes:
    - Write the 16-byte binary `SAL1` stream header to the output stream immediately.
    - Begin streaming raw audio payload blocks directly.

### 4.2. Receiver Modification (Android Client Socket)
- **Native Probe**: When connecting to a desktop server:
  - Do not write any HTTP request.
  - Wait to read 16 bytes.
  - If the signature matches `"SAL1"`, parse the remaining 12 bytes to automatically configure the local AudioTrack / Opus decoder, and read the stream directly.
  - If it times out or fails, fallback to the existing HTTP pipeline (`GET /info` -> `GET /stream`).

---

## 🎵 5. Codec Synchronization Parameters

Both implementations must ensure the following codec parameters match exactly for Quality Mode:

- **Bitrate**: `128000` bps (128 kbps)
- **Rate Control**: Constant Bitrate (CBR, VBR=0)
- **Complexity**: `5`
- **Frame Duration**: `20` ms
- **Samples/Channel per frame**: `960`
- **PCM bytes per frame**: `3840` bytes (960 frames * 2 channels * 2 bytes/sample)
- **Framing**: Every compressed Opus packet must be prefixed by a **2-byte Big-Endian length header** on the network socket:
  ```
  [2 bytes Big-Endian payload length N] [N bytes of compressed Opus frame data]
  ```

---

## 🧪 6. Compatibility Verification Matrix

| Sender (Server) | Receiver (Client) | Stream Type | Expected Behavior |
| :--- | :--- | :---: | :--- |
| **C++ Desktop** | **C++ Desktop** | PCM / Opus | Pure native binary transmission (via `SAL1` handshake). |
| **Android Mobile**| **Android Mobile**| PCM / Opus | Pure native binary transmission (via `SAL1` handshake). |
| **C++ Desktop** | **Android Mobile**| PCM / Opus | Híbrida: Mobile client requests `/info` & `/stream`, Desktop replies via HTTP. |
| **Android Mobile**| **C++ Desktop** | PCM / Opus | Híbrida: Desktop client falls back to HTTP `/info` & `/stream` endpoints. |
| **C++ Desktop** | **Web Browser** | PCM | Browser connects to `http://IP:8080/` and plays audio via HTML5 player. |
| **Android Mobile**| **Web Browser** | PCM | Browser connects to `http://IP:8080/` and plays audio via HTML5 player. |
