#include "ui/ConsoleUi.h"

#include "audio/AudioAbstractions.h"
#include "audio/AudioPipeline.h"
#include "audio/MiniaudioBackend.h"
#include "network/PcmBroadcastServer.h"
#include "network/TcpSocket.h"
#include "platform/LocalIp.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

namespace shareaudio {

ConsoleUi::ConsoleUi(AppController& controller)
    : controller_(controller)
{
}

int ConsoleUi::run(const std::vector<std::string>& args)
{
    if (args.empty() || args[0] == "help" || args[0] == "--help" || args[0] == "-h") {
        print_help();
        return 0;
    }
    if (args[0] == "ips") {
        print_local_ips();
        return 0;
    }
    if (args[0] == "devices") {
        print_audio_devices();
        return 0;
    }
    if (args[0] == "share") {
        AudioMode mode = AudioMode::Balanced;
        if (args.size() == 3 && args[1] == "--mode") {
            auto parsed = parse_audio_mode(args[2]);
            if (!parsed || *parsed == AudioMode::Quality) {
                std::cerr << "share supports --mode balanced or --mode ultrafast. Quality mode requires Opus implementation.\n";
                return 2;
            }
            mode = *parsed;
        } else if (args.size() != 1) {
            std::cerr << "Usage: shareaudio_cli share [--mode balanced|ultrafast]\n";
            return 2;
        }

#if SHAREAUDIO_ENABLE_MINIAUDIO
        MiniaudioCapture capture;
#else
        GeneratedToneCapture capture;
#endif
        AudioFormat format;
        auto init = capture.initialize(format, {});
        if (!init.ok()) {
            std::cerr << init.error().message << '\n';
            return 2;
        }

        PcmBroadcastServer server;
        auto start_server = server.start(Defaults::tcp_port, mode);
        if (!start_server.ok()) {
            std::cerr << start_server.error().message << '\n';
            return 2;
        }

        PcmTransmitterPipeline pipeline(mode);
        std::atomic_bool running { true };
        std::thread network_worker([&] {
            while (running) {
                std::vector<std::uint8_t> packet;
                if (pipeline.try_pop_packet(packet)) {
                    server.broadcast(packet);
                } else {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        });

        auto start_capture = capture.start([&](std::span<const std::uint8_t> pcm) {
            pipeline.on_captured_pcm(pcm);
        });
        if (!start_capture.ok()) {
            running = false;
            if (network_worker.joinable()) {
                network_worker.join();
            }
            server.stop();
            std::cerr << start_capture.error().message << '\n';
            return 2;
        }

        std::cout << "Sharing on TCP port " << Defaults::tcp_port << " in " << to_string(mode) << " mode. Press Enter to stop.\n";
        std::string line;
        std::getline(std::cin, line);

        capture.stop();
        running = false;
        if (network_worker.joinable()) {
            network_worker.join();
        }
        server.stop();
        auto stats = pipeline.stats();
        auto broadcast = server.stats();
        std::cout << "Stopped. captured_bytes=" << stats.bytes_captured
                  << " packets=" << stats.packets_produced
                  << " dropped_packets=" << stats.dropped_packets
                  << " bytes_sent=" << broadcast.bytes_sent << '\n';
        return 0;
    }
    if (args[0] == "listen") {
        if (args.size() != 2) {
            std::cerr << "Usage: shareaudio_cli listen <host>\n";
            return 2;
        }

        auto local = is_local_address(args[1]);
        if (!local.ok()) {
            std::cerr << local.error().message << '\n';
            return 2;
        }
        if (local.value()) {
            std::cerr << "Refusing to connect receiver to this same machine.\n";
            return 2;
        }

#if SHAREAUDIO_ENABLE_MINIAUDIO
        MiniaudioPlayback playback;
#else
        NullAudioPlayback playback;
#endif
        AudioFormat format;
        auto init = playback.initialize(format, {});
        if (!init.ok()) {
            std::cerr << init.error().message << '\n';
            return 2;
        }

        auto connected = TcpSocket::connect_to(args[1], Defaults::tcp_port);
        if (!connected.ok()) {
            std::cerr << connected.error().message << '\n';
            return 2;
        }

        auto header_bytes = connected.value().receive_exact(ProtocolWriter::stream_header_size);
        if (!header_bytes.ok()) {
            std::cerr << header_bytes.error().message << '\n';
            return 2;
        }
        auto header = ProtocolReader::parse_stream_header(header_bytes.value());
        if (!header.ok()) {
            std::cerr << header.error().message << '\n';
            return 2;
        }
        if (header.value().codec != StreamCodec::PcmS16Le || header.value().mode == AudioMode::Quality) {
            std::cerr << "This build can only listen to balanced or ultrafast PCM streams. Quality mode requires Opus implementation.\n";
            return 2;
        }

        PcmReceiverPipeline receiver(playback, header.value().packet_size * 8);
        auto start_playback = receiver.start();
        if (!start_playback.ok()) {
            std::cerr << start_playback.error().message << '\n';
            return 2;
        }

        std::cout << "Listening to " << args[1] << ':' << Defaults::tcp_port << " in " << to_string(header.value().mode) << " mode. Stop with Ctrl+C or transmitter disconnect.\n";
        const auto packet_size = static_cast<std::size_t>(header.value().packet_size);
        while (true) {
            auto packet = connected.value().receive_exact(packet_size);
            if (!packet.ok()) {
                std::cerr << packet.error().message << '\n';
                break;
            }
            receiver.receive_pcm(packet.value());
            auto pumped = receiver.pump_playback(packet_size);
            if (!pumped.ok()) {
                std::cerr << pumped.error().message << '\n';
                break;
            }
        }
        playback.stop();
        auto stats = receiver.stats();
        std::cout << "Stopped. received_bytes=" << stats.bytes_received
                  << " played_bytes=" << stats.bytes_played
                  << " underruns=" << stats.buffer.underruns << '\n';
        return 0;
    }

    std::cerr << "Unknown command. Use 'shareaudio_cli help'.\n";
    return 2;
}

void ConsoleUi::print_help() const
{
    std::cout
        << "ShareAudioLite " << SHAREAUDIO_VERSION << "\n\n"
        << "Usage:\n"
        << "  shareaudio_cli share [--mode balanced|ultrafast]\n"
        << "  shareaudio_cli listen <host>\n"
        << "  shareaudio_cli devices\n"
        << "  shareaudio_cli ips\n"
        << "  shareaudio_cli help\n\n"
        << "Defaults:\n"
        << "  share uses balanced mode when --mode is omitted.\n"
        << "  listen detects the stream mode from the sender.\n"
        << "  quality mode is hidden until Opus encode/decode is implemented.\n";
}

void ConsoleUi::print_audio_devices() const
{
    std::cout << "Capture devices:\n";
#if SHAREAUDIO_ENABLE_MINIAUDIO
    MiniaudioCapture capture;
#else
    GeneratedToneCapture capture;
#endif
    for (const auto& device : capture.devices()) {
        std::cout << "  " << device.id << " - " << device.name << (device.is_default ? " (default)" : "") << '\n';
    }

    std::cout << "Playback devices:\n";
#if SHAREAUDIO_ENABLE_MINIAUDIO
    MiniaudioPlayback playback;
#else
    NullAudioPlayback playback;
#endif
    for (const auto& device : playback.devices()) {
        std::cout << "  " << device.id << " - " << device.name << (device.is_default ? " (default)" : "") << '\n';
    }
}

void ConsoleUi::print_local_ips() const
{
    auto addresses = list_local_ip_addresses();
    if (!addresses.ok()) {
        std::cerr << addresses.error().message << '\n';
        return;
    }
    for (const auto& address : addresses.value()) {
        std::cout << address << '\n';
    }
}

} // namespace shareaudio
