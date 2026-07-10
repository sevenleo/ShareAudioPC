#include "network/PcmBroadcastServer.h"

#include "app/Logger.h"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <thread>

namespace shareaudio {
namespace {

constexpr std::size_t max_stream_client_queue = 8;

std::span<const std::uint8_t> bytes_view(const std::string& value)
{
    return std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(value.data()), value.size());
}

} // namespace

struct PcmBroadcastServer::StreamClient {
    explicit StreamClient(std::shared_ptr<TcpSocket> stream_socket)
        : socket(std::move(stream_socket))
    {
    }

    ~StreamClient()
    {
        stop();
    }

    StreamClient(const StreamClient&) = delete;
    StreamClient& operator=(const StreamClient&) = delete;

    void start()
    {
        worker = std::thread([this] {
            run();
        });
    }

    void stop()
    {
        {
            std::scoped_lock lock(mutex);
            stopping = true;
        }
        cv.notify_all();
        if (socket) {
            socket->close();
        }
        if (worker.joinable()) {
            worker.join();
        }
    }

    bool active() const
    {
        return running && socket && socket->valid();
    }

    bool enqueue(std::span<const std::uint8_t> packet)
    {
        if (!active()) {
            return false;
        }

        {
            std::scoped_lock lock(mutex);
            if (stopping || queue.size() >= max_stream_client_queue) {
                return false;
            }
            queue.emplace_back(packet.begin(), packet.end());
        }
        cv.notify_one();
        return true;
    }

    void run()
    {
        while (true) {
            std::vector<std::uint8_t> packet;
            {
                std::unique_lock lock(mutex);
                cv.wait(lock, [this] {
                    return stopping || !queue.empty();
                });
                if (stopping && queue.empty()) {
                    break;
                }
                packet = std::move(queue.front());
                queue.pop_front();
            }

            auto result = socket->send_all(packet);
            if (!result.ok()) {
                Logger::warning("Dropping PCM broadcast client: " + result.error().message);
                break;
            }
        }

        running = false;
        if (socket) {
            socket->close();
        }
    }

    std::shared_ptr<TcpSocket> socket;
    mutable std::mutex mutex;
    std::condition_variable cv;
    std::deque<std::vector<std::uint8_t>> queue;
    std::thread worker;
    bool stopping {};
    std::atomic_bool running { true };
};

std::string web_receiver_html()
{
    return R"HTML(<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ShareAudioPC Web receiver</title>
  <style>
    :root { color-scheme: dark; }
    body {
      margin: 0;
      min-height: 100vh;
      display: grid;
      place-items: center;
      background: linear-gradient(180deg, #101826 0%, #080C14 100%);
      color: #FFFFFF;
      font-family: "Segoe UI", Arial, sans-serif;
    }
    main {
      position: relative;
      width: min(460px, calc(100vw - 32px));
      overflow: hidden;
      background: #111827;
      border: 1px solid #243148;
      border-radius: 8px;
      padding: 30px;
      box-sizing: border-box;
      box-shadow: 0 24px 70px rgba(0, 0, 0, 0.35);
    }
    main::before {
      content: "";
      position: absolute;
      inset: 0 0 auto;
      height: 3px;
      background: linear-gradient(90deg, #1DF09A, #00A3FF);
    }
    h1 {
      margin: 0 0 18px;
      color: #9FB0C8;
      font-size: 17px;
      font-weight: 600;
      line-height: 1.12;
    }
    h1::first-line {
      color: #FFFFFF;
      font-size: 34px;
      font-weight: 800;
    }
    p {
      line-height: 1.45;
      margin: 0;
    }
    .status {
      display: flex;
      align-items: center;
      gap: 9px;
      min-height: 22px;
      color: #BAC7DE;
      font-size: 14px;
    }
    .status::before {
      content: "";
      width: 8px;
      height: 8px;
      border-radius: 50%;
      background: #1DF09A;
      box-shadow: 0 0 16px rgba(29, 240, 154, 0.55);
    }
    .status.error::before {
      background: #FF6B6B;
      box-shadow: 0 0 16px rgba(255, 107, 107, 0.45);
    }
    button {
      margin-top: 22px;
      width: 100%;
      border: 0;
      border-radius: 6px;
      padding: 13px 16px;
      font-weight: 700;
      font-size: 16px;
      color: #0B101D;
      background: linear-gradient(90deg, #1DF09A, #00A3FF);
      cursor: pointer;
    }
    button:disabled {
      cursor: default;
      opacity: 0.65;
    }
    button.disconnect {
      color: #FFFFFF;
      background: #D83B3B;
    }
    dl {
      display: grid;
      grid-template-columns: max-content 1fr;
      gap: 8px 14px;
      margin: 18px 0 0;
      padding: 14px;
      background: #0C1220;
      border: 1px solid #1E293B;
      border-radius: 8px;
      color: #BAC7DE;
    }
    dt { color: #7F8EA8; }
    dd { margin: 0; color: #FFFFFF; }
    .error { color: #FF6B6B; }
  </style>
</head>
<body>
  <main>
    <h1>ShareAudioPC<br>Web receiver</h1>
    <p id="status" class="status">Ready to connect.</p>
    <button id="connectButton">Connect Audio</button>
    <dl>
      <dt>Mode</dt><dd id="mode">-</dd>
      <dt>Chunk</dt><dd id="chunk">-</dd>
      <dt>Dropped</dt><dd id="dropped">0</dd>
    </dl>
  </main>
  <script>
    const connectButton = document.getElementById('connectButton');
    const statusText = document.getElementById('status');
    const modeText = document.getElementById('mode');
    const chunkText = document.getElementById('chunk');
    const droppedText = document.getElementById('dropped');

    let audioContext = null;
    let nextPlayTime = 0;
    let latencyTarget = 0.010;
    let dropThreshold = 0.045;
    let packetSize = 2048;
    let droppedPackets = 0;
    let abortController = null;
    let isConnected = false;
    let isConnecting = false;
    let disconnectRequested = false;

    function setStatus(message, isError = false) {
      statusText.textContent = message;
      statusText.className = isError ? 'status error' : 'status';
    }

    function setConnected(connected) {
      isConnected = connected;
      connectButton.disabled = false;
      connectButton.textContent = connected ? 'Disconnect' : 'Connect Audio';
      connectButton.classList.toggle('disconnect', connected);
    }

    function disconnectAudio() {
      disconnectRequested = true;
      if (abortController) {
        abortController.abort();
        abortController = null;
      }
      if (audioContext) {
        audioContext.close().catch(() => {});
        audioContext = null;
      }
      nextPlayTime = 0;
      setStatus('Disconnected.');
      setConnected(false);
    }

    function appendBytes(left, right) {
      const merged = new Uint8Array(left.length + right.length);
      merged.set(left, 0);
      merged.set(right, left.length);
      return merged;
    }

    function configureMode(info) {
      if (info.codec !== 'pcm') {
        throw new Error('This web receiver supports only Fast and Balanced PCM. Select Fast or Balanced on the transmitter.');
      }
      if (info.sampleRate !== 48000 || info.channels !== 2) {
        throw new Error('Unsupported PCM format. Expected 48 kHz stereo.');
      }

      packetSize = Number(info.chunkSize);
      if (packetSize === 1024) {
        latencyTarget = 0.003;
        dropThreshold = 0.020;
        modeText.textContent = 'fast';
      } else if (packetSize === 2048) {
        latencyTarget = 0.010;
        dropThreshold = 0.045;
        modeText.textContent = 'balanced';
      } else {
        throw new Error('Unsupported PCM chunk size: ' + packetSize);
      }
      chunkText.textContent = packetSize + ' bytes';
    }

    function schedulePacket(packet) {
      const now = audioContext.currentTime;
      if (nextPlayTime < now) {
        nextPlayTime = now + latencyTarget;
      }

      if (nextPlayTime - now > dropThreshold) {
        droppedPackets += 1;
        droppedText.textContent = String(droppedPackets);
        nextPlayTime = now + latencyTarget;
        return;
      }

      const frameBytes = 4;
      const frameCount = packet.length / frameBytes;
      const audioBuffer = audioContext.createBuffer(2, frameCount, 48000);
      const left = audioBuffer.getChannelData(0);
      const right = audioBuffer.getChannelData(1);
      const view = new DataView(packet.buffer, packet.byteOffset, packet.byteLength);

      for (let i = 0; i < frameCount; ++i) {
        left[i] = view.getInt16(i * 4, true) / 32768.0;
        right[i] = view.getInt16(i * 4 + 2, true) / 32768.0;
      }

      const source = audioContext.createBufferSource();
      source.buffer = audioBuffer;
      source.connect(audioContext.destination);
      source.start(nextPlayTime);
      nextPlayTime += audioBuffer.duration;
    }

    async function connectAudio() {
      if (isConnected || isConnecting) {
        return;
      }

      isConnecting = true;
      disconnectRequested = false;
      abortController = new AbortController();
      connectButton.disabled = true;
      connectButton.textContent = 'Connecting...';
      connectButton.classList.remove('disconnect');
      droppedPackets = 0;
      droppedText.textContent = '0';
      setStatus('Loading stream metadata...');

      audioContext = new (window.AudioContext || window.webkitAudioContext)({ sampleRate: 48000 });
      await audioContext.resume();

      const infoResponse = await fetch('/info', { cache: 'no-store', signal: abortController.signal });
      if (!infoResponse.ok) {
        throw new Error('Unable to load /info: HTTP ' + infoResponse.status);
      }
      const info = await infoResponse.json();
      configureMode(info);

      setStatus('Opening PCM stream...');
      const streamResponse = await fetch('/stream', { cache: 'no-store', signal: abortController.signal });
      if (!streamResponse.ok || !streamResponse.body) {
        throw new Error('Unable to open /stream: HTTP ' + streamResponse.status);
      }

      const reader = streamResponse.body.getReader();
      let buffer = new Uint8Array(0);
      nextPlayTime = audioContext.currentTime + latencyTarget;
      setStatus('Streaming PCM audio.');
      isConnecting = false;
      setConnected(true);

      while (true) {
        const result = await reader.read();
        if (result.done) {
          throw new Error('Stream closed.');
        }
        buffer = appendBytes(buffer, result.value);
        while (buffer.length >= packetSize) {
          const packet = buffer.slice(0, packetSize);
          buffer = buffer.slice(packetSize);
          schedulePacket(packet);
        }
      }
    }

    connectButton.addEventListener('click', async () => {
      if (isConnected) {
        disconnectAudio();
        return;
      }

      try {
        await connectAudio();
      } catch (error) {
        if (disconnectRequested || error.name === 'AbortError') {
          setStatus('Disconnected.');
        } else {
          setStatus(error.message, true);
        }
      } finally {
        isConnecting = false;
        abortController = null;
        if (audioContext) {
          audioContext.close().catch(() => {});
          audioContext = null;
        }
        setConnected(false);
      }
    });
  </script>
</body>
</html>)HTML";
}

Result<void> PcmBroadcastServer::start(std::uint16_t port, AudioMode mode)
{
    StreamHeader stream;
    stream.mode = mode;
    stream.codec = mode == AudioMode::Efficient ? StreamCodec::Opus : StreamCodec::PcmS16Le;
    stream.packet_size = mode == AudioMode::Efficient ? Defaults::max_opus_frame_bytes : static_cast<std::uint32_t>(packet_size_for_mode(mode));
    return start(port, stream);
}

Result<void> PcmBroadcastServer::start(std::uint16_t port, const StreamHeader& header)
{
    active_header_ = header;
    auto encoded_header = ProtocolWriter::make_stream_header(header);
    if (!encoded_header.ok()) {
        return Result<void>::failure(encoded_header.error());
    }
    stream_header_ = encoded_header.value();

    return server_.start(port, [this](std::shared_ptr<TcpSocket> client) {
        auto preview = client->receive_with_timeout(4, 150);
        bool is_http = false;

        if (preview.ok()) {
            const auto& bytes = preview.value();
            if (bytes.size() == 4 && bytes[0] == 'G' && bytes[1] == 'E' && bytes[2] == 'T' && bytes[3] == ' ') {
                is_http = true;
            }
        }

        if (is_http) {
            std::string request_headers = "GET ";
            bool found_separator = false;
            while (client->valid() && request_headers.size() < 4096) {
                auto chunk = client->receive_with_timeout(1, 1000);
                if (!chunk.ok() || chunk.value().empty()) {
                    break;
                }
                request_headers.push_back(static_cast<char>(chunk.value()[0]));
                if (request_headers.size() >= 4 && request_headers.substr(request_headers.size() - 4) == "\r\n\r\n") {
                    found_separator = true;
                    break;
                }
            }

            if (!found_separator) {
                client->close();
                return;
            }

            std::string path = "/";
            auto space1 = request_headers.find(' ');
            if (space1 != std::string::npos) {
                auto space2 = request_headers.find(' ', space1 + 1);
                if (space2 != std::string::npos) {
                    path = request_headers.substr(space1 + 1, space2 - (space1 + 1));
                }
            }

            if (path == "/info") {
                std::string codec_str = (active_header_.codec == StreamCodec::Opus) ? "opus" : "pcm";
                std::size_t clients_count = 0;
                {
                    std::scoped_lock lock(mutex_);
                    clients_count = active_client_count_locked();
                }

                std::string json = "{\n"
                    "  \"status\": \"streaming\",\n"
                    "  \"connectedClients\": " + std::to_string(clients_count) + ",\n"
                    "  \"sampleRate\": " + std::to_string(active_header_.sample_rate) + ",\n"
                    "  \"channels\": " + std::to_string(active_header_.channels) + ",\n"
                    "  \"codec\": \"" + codec_str + "\",\n"
                    "  \"bitrate\": 128000,\n"
                    "  \"chunkSize\": " + std::to_string(active_header_.packet_size) + "\n"
                    "}\n";

                std::string response = "HTTP/1.1 200 OK\r\n"
                    "Content-Type: application/json\r\n"
                    "Connection: close\r\n"
                    "Content-Length: " + std::to_string(json.size()) + "\r\n"
                    "\r\n" + json;

                (void)client->send_all(bytes_view(response));
                client->close();
                return;
            }
            else if (path == "/stream") {
                std::string response = "HTTP/1.1 200 OK\r\n"
                    "Content-Type: application/octet-stream\r\n"
                    "Connection: keep-alive\r\n"
                    "Cache-Control: no-cache, no-store, must-revalidate\r\n"
                    "Pragma: no-cache\r\n"
                    "\r\n";

                auto sent_headers = client->send_all(bytes_view(response));
                if (!sent_headers.ok()) {
                    client->close();
                    return;
                }
            }
            else {
                const std::string html = web_receiver_html();

                std::string response = "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html\r\n"
                    "Connection: close\r\n"
                    "Content-Length: " + std::to_string(html.size()) + "\r\n"
                    "\r\n" + html;

                (void)client->send_all(bytes_view(response));
                client->close();
                return;
            }
        }
        else {
            auto sent_header = client->send_all(stream_header_);
            if (!sent_header.ok()) {
                Logger::warning("Dropping PCM broadcast client before registration: " + sent_header.error().message);
                client->close();
                std::scoped_lock lock(mutex_);
                ++stats_.dropped_clients;
                return;
            }
        }

        register_stream_client(std::move(client));
        Logger::info("PCM broadcast client connected.");
    });
}

void PcmBroadcastServer::stop()
{
    server_.stop();

    std::vector<std::shared_ptr<StreamClient>> clients;
    {
        std::scoped_lock lock(mutex_);
        clients = std::move(clients_);
        clients_.clear();
        stats_.connected_clients = 0;
    }

    for (auto& client : clients) {
        if (client) {
            client->stop();
        }
    }
}

void PcmBroadcastServer::broadcast(std::span<const std::uint8_t> packet)
{
    std::scoped_lock lock(mutex_);
    auto remove_from = std::remove_if(clients_.begin(), clients_.end(), [&](const std::shared_ptr<StreamClient>& client) {
        if (!client || !client->active()) {
            ++stats_.dropped_clients;
            return true;
        }

        if (client->enqueue(packet)) {
            stats_.bytes_sent += packet.size();
        }
        return false;
    });

    clients_.erase(remove_from, clients_.end());
    stats_.connected_clients = active_client_count_locked();
}

BroadcastStats PcmBroadcastServer::stats() const
{
    std::scoped_lock lock(mutex_);
    BroadcastStats result = stats_;
    result.connected_clients = active_client_count_locked();
    return result;
}

bool PcmBroadcastServer::running() const
{
    return server_.running();
}

void PcmBroadcastServer::register_stream_client(std::shared_ptr<TcpSocket> client)
{
    auto stream_client = std::make_shared<StreamClient>(std::move(client));
    stream_client->start();

    std::scoped_lock lock(mutex_);
    clients_.push_back(std::move(stream_client));
    stats_.connected_clients = active_client_count_locked();
}

std::size_t PcmBroadcastServer::active_client_count_locked() const
{
    return static_cast<std::size_t>(std::count_if(clients_.begin(), clients_.end(), [](const auto& client) {
        return client && client->active();
    }));
}

} // namespace shareaudio
