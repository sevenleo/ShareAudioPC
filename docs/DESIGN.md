# ShareAudioPC GUI Concept Design Prompt

Create a modern, elegant, desktop-first concept design for **ShareAudioPC**, a native Windows/Linux LAN audio sharing application. This is an operational tool, not a marketing website. The first viewport must be the usable application workspace.

The visual design must make audio transmission and reception understandable at a glance: who is sending, who is receiving, whether each side is active, which device is used, and whether there is an error. The app must feel calm, reliable, technical, and quick to operate during repeated use.

## Product Context

ShareAudioPC can run two independent local-network audio sessions at the same time:

- **Sharing / Transmitter:** captures local audio and sends it to one or more clients over the LAN.
- **Receiver:** connects to another transmitter and plays its received audio locally.

Both functions may be active simultaneously. This is not a relay application and no echo cancellation exists. The design should make the independent states of Sharing and Receiver visually unambiguous.

The default TCP port is **33777**. The application uses English labels and controls.

## Workspace Modes And Window Structure

The GUI is one desktop workspace with two presentation modes. **Simple** and **Advanced** are not separate applications and must not create separate navigation flows. Advanced mode reveals additional controls inside the same Sharing and Receiver areas and reveals the diagnostic tabs below them.

The common window structure is:

1. **Session Status** at the top.
2. **Sharing (Transmitter)** panel.
3. **Receiver** panel.
4. Advanced-only tab area.
5. Persistent footer with application version and **Minimize to tray**.

The Sharing and Receiver panels are independent. Both action buttons must remain available at the same time, including when both sessions are active. Starting or stopping one service must not hide, disable, or replace the controls of the other service.

### Simple Mode

Simple Mode is the default compact workspace for frequent everyday operation. It must allow a user to start transmitting or connect to a transmitter without opening another screen or understanding advanced audio settings.

Target dimensions are approximately `830x350`, with a minimum of `800x340` when the display permits. The layout must remain readable at laptop resolutions such as `1366x768`.

Simple Mode must show:

- **Session Status** with the current state, IP/host context, TCP port `33777`, and the Advanced mode toggle.
- A separate inline error row only when a real error or connection message exists; errors must not compress the state, host, and port into an unreadable single line.
- **Sharing (Transmitter)** with the primary button `Start Sharing`, changing to `Stop Sharing` while active.
- On Windows, the checkbox `Follow system volume` in the Sharing panel.
- **Receiver** with the `Transmitter IP` field and the primary button `Connect Receiver`, changing to `Disconnect` while connecting or listening.
- The footer checkbox `Minimize to tray`, always visible regardless of the current mode.
- The button `Show Advanced Options`, changing to `Hide Advanced Options` in Advanced Mode.

Simple Mode must not display the Network & Hardware or Diagnostics & Help tabs. It must not duplicate AudioMode, device selectors, host fields, or session buttons in a second location.

### Advanced Mode

Advanced Mode is the full operational workspace for setup, device selection, troubleshooting, and monitoring. It preserves the complete Simple Mode action surface at the top and adds detailed controls without replacing the primary buttons.

Target dimensions are approximately `1100x760`, with a normal minimum of `1000x640`, clamped to the available display area. The layout must remain usable at `1366x768` and `1920x1080`.

Advanced Mode must show the same **Session Status**, **Sharing (Transmitter)**, **Receiver**, and footer elements as Simple Mode. The additional controls are:

- Sharing `AudioMode` selector with exactly:
  - `Balanced (Recommended)`;
  - `Fast (Low Latency)`;
  - `Efficient (Low Data)`.
- Sharing `Audio Device` selector for the capture/loopback source.
- Sharing `Clients` value showing the connected receiver count.
- Receiver `Audio Speaker` selector for the playback output.
- Receiver `Stream Mode` value showing the detected sender mode, or `-` before metadata is available.
- The `Network & Hardware` tab.
- The `Diagnostics & Help` tab.

Controls must be disabled only according to the session they affect:

- While Sharing is active, disable AudioMode, capture device, and Follow system volume controls; Receiver controls remain usable.
- While Receiver is connecting or listening, disable the transmitter host field and playback device selector; Sharing controls remain usable.
- Start/Stop Sharing and Connect/Disconnect remain independent toggle actions.

## Expected Screens, Buttons, And Options

The following inventory is the minimum functional surface expected in the design. Visual concepts may improve grouping and hierarchy, but must not remove or invent these controls.

| Area or screen | Required controls and information |
| --- | --- |
| Session Status | State, IP/Host, Port, current error/message, `Show Advanced Options` / `Hide Advanced Options` |
| Sharing (Transmitter) | `Start Sharing` / `Stop Sharing`, Windows-only `Follow system volume`, `AudioMode`, `Audio Device`, `Clients` |
| Receiver | `Transmitter IP`, `Connect Receiver` / `Disconnect`, `Audio Speaker`, `Stream Mode` |
| Network & Hardware | Local IP list, `Refresh`, `Copy Selected IP`, recent hosts, capture source list, playback output list, `Refresh Devices List` |
| Diagnostics & Help | Bytes Sent, Packets Produced, Packets Dropped, Bytes Received, Bytes Played, Jitter Buffer Bytes, Playback Underruns, event log, `Copy Diagnostics to Clipboard`, `Help Guide` |
| Footer | Application version and `Minimize to tray` |
| System tray | `Show Window` / `Hide Window`, checkable `Minimize to tray`, `Exit` |

The design must include visual states for every toggle action:

- `Start Sharing` and `Connect Receiver` when idle;
- `Stop Sharing` and `Disconnect` when active;
- connecting state for Receiver;
- disabled device and AudioMode controls while their session is active;
- system tray hidden state and restored window state;
- unavailable device, invalid configuration, refused connection, timeout, and self-connection error.

## GUI State Labels

The overall status value uses these stable English labels:

- `Idle`: neither service is active;
- `Sharing`: transmitter active and Receiver idle;
- `Connecting`: Receiver is attempting to connect and Sharing is idle;
- `Listening`: Receiver is connected and playing while Sharing is idle;
- `Sharing + Connecting`: Sharing active while Receiver is connecting;
- `Sharing + Listening`: both Sharing and Receiver active.

Sharing and Receiver should also have independent visual indicators so a combined state never looks like one ambiguous session. The overall state text is informational; the panel-level action and state are the authoritative interaction points.

## Required Screens And States

Design the following as one coherent desktop application:

1. **Simple workspace**
   - Compact default view for everyday use.
   - A top status strip showing overall state, local/transmitter IP context, receiver host when connected, TCP port, and the most recent error or message.
   - A clearly separated **Sharing** area with a Start Sharing / Stop Sharing control.
   - A clearly separated **Receiver** area with a transmitter IP input and Connect Receiver / Disconnect control.
   - Both areas must remain usable simultaneously. Starting Sharing must not visually disable Receiver, and vice versa.
   - On Windows, include the optional checkbox **Follow system volume** in the Sharing area.
   - A persistent footer control: **Minimize to tray**.
   - A compact control to switch to and from Advanced mode.

2. **Advanced workspace**
   - Keep the same Sharing and Receiver controls visible at the top; do not make core session actions disappear inside tabs.
   - Add advanced controls for Sharing:
     - **AudioMode** selector with exactly these options:
       - Balanced (Recommended)
       - Fast (Low Latency)
       - Efficient (Low Data)
     - Capture/loopback audio device selector.
     - Connected-client count.
     - Follow system volume checkbox on Windows only.
   - Add advanced controls for Receiver:
     - Transmitter host/IP field.
     - Playback audio device selector.
     - Detected stream AudioMode.
   - Show that device and AudioMode controls become unavailable only for the active session they affect, while the other independent session remains controllable.

3. **Network & Hardware view**
   - Local IP address list with refresh and copy-selected-IP actions.
   - Recent transmitter hosts; selecting one should make it easy to reuse in the Receiver host field.
   - Capture source list and playback output list.
   - Refresh devices action.
   - Present device lists for scanning, not as decorative cards.

4. **Diagnostics & Help view**
   - Live transmission and reception metrics:
     - bytes sent and received;
     - packets produced and dropped;
     - bytes played;
     - jitter-buffer depth;
     - playback underruns.
   - Read-only event log with readable severity treatment.
   - Copy diagnostics action.
   - Help action.
   - When System VolumeMode is enabled, diagnostics must accommodate:
     - VolumeMode: full or system;
     - current applied system-volume gain;
     - tracking state: active, fallback, unsupported, or disabled.

5. **System tray**
   - Tray icon state for an active or idle app.
   - Tray menu with:
     - Show Window / Hide Window;
     - checkable Minimize to tray;
     - Exit.
   - When tray mode is enabled, minimizing or closing the window hides it without stopping active audio sessions.
   - Double-clicking the tray icon restores the window.

6. **Startup and error states**
   - App may start hidden in the tray.
   - Show clear states for idle, sharing, connecting, listening, sharing + connecting, and sharing + listening.
   - Design clear but non-alarming inline error treatment for connection failures, unavailable audio devices, and configuration issues.
   - Include empty states for no recent hosts, no detected devices, and no active session.

## Functional Rules The Design Must Respect

- Sharing and Receiver can run simultaneously and stop independently.
- Receiver cannot connect to the same computer.
- The app auto-detects the sender AudioMode.
- Efficient uses Opus; Balanced and Fast use PCM. Do not imply that Efficient is “highest sound quality.”
- Windows-only **Follow system volume** applies the selected output endpoint master volume to audio sent by Sharing. It is a runtime option and must be easy to notice without becoming the primary action.
- On Linux, this system-volume option is unavailable.
- Tray mode is runtime-only. It does not edit the portable configuration file.
- The app supports portable startup configuration and may automatically start Sharing, Receiver, or both. The configuration editor itself is not currently part of the product and must not be invented as a required screen.
- Do not introduce account systems, cloud sync, playlists, recording, chat, user profiles, payments, social features, or relay-server controls.

## Visual And Interaction Direction

- Design for desktop Windows first, while remaining credible on Linux.
- Prefer a dense, quiet operational layout over a dashboard full of floating cards.
- Use a restrained neutral foundation with semantic accents:
  - green only for active sharing/success;
  - blue or cyan for receiver/connectivity;
  - red for stop, disconnect, and important failures;
  - amber only for warnings/fallback states.
- Avoid a screen dominated by dark blue, purple gradients, oversized typography, decorative glow, or marketing-style hero composition.
- Use real hierarchy: the session state and start/stop/connect/disconnect actions are primary; device selection, mode selection, metrics, and logs are secondary.
- Use familiar icons for refresh, copy, device selection, diagnostics, help, tray, and close/exit actions. Include tooltips for icon-only controls.
- Buttons must have stable widths and not shift when their labels change between Start/Stop or Connect/Disconnect.
- Use clear toggle/checkbox semantics for binary options, menus/selectors for option sets, and buttons only for commands.
- Keep corners modest, around 4–8 px. Do not nest cards inside cards.
- Support a comfortable compact window and a wider advanced window. The design must remain usable at typical 1366x768 laptop resolution and at 1920x1080.
- Ensure visible keyboard focus, readable contrast, and text that never truncates essential status or control names.

## Requested Deliverables

Produce a conceptual design package containing:

1. A primary Simple workspace screen.
2. A primary Advanced workspace screen with Network & Hardware selected.
3. A Diagnostics & Help screen.
4. A tray-menu / hidden-to-tray state.
5. Key state variations for idle, sharing, connecting, listening, sharing + listening, and connection error.
6. A component inventory for status indicators, session controls, AudioMode selector, device selectors, metric rows, log rows, checkboxes, and tray controls.
7. Color, typography, spacing, iconography, hover, disabled, selected, warning, error, and active-state guidance.

Keep all visible application text in English. Use the exact product name **ShareAudioPC** in the window title and primary brand treatment.
