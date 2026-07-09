#include "network/PcmBroadcastServer.h"

#include "app/Logger.h"

#include <algorithm>

namespace shareaudio {

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
                    clients_count = clients_.size();
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

                (void)client->send_all(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(response.data()), response.size()));
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

                auto sent_headers = client->send_all(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(response.data()), response.size()));
                if (!sent_headers.ok()) {
                    client->close();
                    return;
                }
            }
            else {
                std::string html = "<!DOCTYPE html>\n"
                    "<html>\n"
                    "<head>\n"
                    "  <meta charset=\"utf-8\">\n"
                    "  <title>ShareAudioLite Player</title>\n"
                    "  <style>\n"
                    "    body { background-color: #0B101D; color: #FFFFFF; font-family: sans-serif; display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100vh; margin: 0; }\n"
                    "    .card { background-color: #1C253E; padding: 30px; border-radius: 12px; border: 1px solid #2A3656; text-align: center; box-shadow: 0 4px 20px rgba(0,0,0,0.5); }\n"
                    "    h1 { color: #00A3FF; margin-bottom: 20px; font-size: 24px; }\n"
                    "    button { background: linear-gradient(90deg, #1DF09A, #00A3FF); color: #0B101D; border: none; border-radius: 6px; padding: 12px 24px; font-weight: bold; font-size: 16px; cursor: pointer; }\n"
                    "    button:hover { opacity: 0.9; }\n"
                    "  </style>\n"
                    "</head>\n"
                    "<body>\n"
                    "  <div class=\"card\">\n"
                    "    <h1>ShareAudioLite Browser Player</h1>\n"
                    "    <button id=\"playBtn\">Connect & Listen</button>\n"
                    "  </div>\n"
                    "  <script>\n"
                    "    const playBtn = document.getElementById('playBtn');\n"
                    "    playBtn.addEventListener('click', async () => {\n"
                    "      playBtn.disabled = true;\n"
                    "      playBtn.innerText = 'Connecting...';\n"
                    "      try {\n"
                    "        const audioCtx = new (window.AudioContext || window.webkitAudioContext)({ sampleRate: 48000 });\n"
                    "        const response = await fetch('/stream');\n"
                    "        const reader = response.body.getReader();\n"
                    "        playBtn.innerText = 'Streaming Active';\n"
                    "        \n"
                    "        const bytesPerSample = 2;\n"
                    "        const channels = 2;\n"
                    "        const frameBytes = bytesPerSample * channels;\n"
                    "        let buffer = new Uint8Array(0);\n"
                    "        let nextPlayTime = audioCtx.currentTime + 0.1;\n"
                    "        \n"
                    "        while (true) {\n"
                    "          const { done, value } = await reader.read();\n"
                    "          if (done) break;\n"
                    "          \n"
                    "          let newBuf = new Uint8Array(buffer.length + value.length);\n"
                    "          newBuf.set(buffer);\n"
                    "          newBuf.set(value, buffer.length);\n"
                    "          buffer = newBuf;\n"
                    "          \n"
                    "          const packetSize = 2048;\n"
                    "          while (buffer.length >= packetSize) {\n"
                    "            const packet = buffer.slice(0, packetSize);\n"
                    "            buffer = buffer.slice(packetSize);\n"
                    "            \n"
                    "            const sampleCount = packetSize / frameBytes;\n"
                    "            const audioBuf = audioCtx.createBuffer(channels, sampleCount, 48000);\n"
                    "            const chL = audioBuf.getChannelData(0);\n"
                    "            const chR = audioBuf.getChannelData(1);\n"
                    "            const dataView = new DataView(packet.buffer, packet.byteOffset, packet.byteLength);\n"
                    "            \n"
                    "            for (let i = 0; i < sampleCount; ++i) {\n"
                    "              chL[i] = dataView.getInt16(i * 4, true) / 32768.0;\n"
                    "              chR[i] = dataView.getInt16(i * 4 + 2, true) / 32768.0;\n"
                    "            }\n"
                    "            \n"
                    "            const source = audioCtx.createBufferSource();\n"
                    "            source.buffer = audioBuf;\n"
                    "            source.connect(audioCtx.destination);\n"
                    "            \n"
                    "            if (nextPlayTime < audioCtx.currentTime) {\n"
                    "              nextPlayTime = audioCtx.currentTime + 0.05;\n"
                    "            }\n"
                    "            source.start(nextPlayTime);\n"
                    "            nextPlayTime += audioBuf.duration;\n"
                    "          }\n"
                    "        }\n"
                    "      } catch (err) {\n"
                    "        alert('Connection error: ' + err.message);\n"
                    "        playBtn.disabled = false;\n"
                    "        playBtn.innerText = 'Connect & Listen';\n"
                    "      }\n"
                    "    });\n"
                    "  </script>\n"
                    "</body>\n"
                    "</html>\n";

                std::string response = "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html\r\n"
                    "Connection: close\r\n"
                    "Content-Length: " + std::to_string(html.size()) + "\r\n"
                    "\r\n" + html;

                (void)client->send_all(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(response.data()), response.size()));
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

        std::scoped_lock lock(mutex_);
        clients_.push_back(std::move(client));
        stats_.connected_clients = clients_.size();
        Logger::info("PCM broadcast client connected.");
    });
}

void PcmBroadcastServer::stop()
{
    server_.stop();
    std::scoped_lock lock(mutex_);
    for (auto& client : clients_) {
        if (client) {
            client->close();
        }
    }
    clients_.clear();
    stats_.connected_clients = 0;
}

void PcmBroadcastServer::broadcast(std::span<const std::uint8_t> packet)
{
    std::scoped_lock lock(mutex_);
    auto remove_from = std::remove_if(clients_.begin(), clients_.end(), [&](const std::shared_ptr<TcpSocket>& client) {
        if (!client || !client->valid()) {
            ++stats_.dropped_clients;
            return true;
        }

        auto result = client->send_all(packet);
        if (!result.ok()) {
            client->close();
            ++stats_.dropped_clients;
            Logger::warning("Dropping PCM broadcast client: " + result.error().message);
            return true;
        }

        stats_.bytes_sent += packet.size();
        return false;
    });

    clients_.erase(remove_from, clients_.end());
    stats_.connected_clients = clients_.size();
}

BroadcastStats PcmBroadcastServer::stats() const
{
    std::scoped_lock lock(mutex_);
    BroadcastStats result = stats_;
    result.connected_clients = clients_.size();
    return result;
}

bool PcmBroadcastServer::running() const
{
    return server_.running();
}

} // namespace shareaudio
