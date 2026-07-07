#include "ui/ConsoleUi.h"

#include "platform/LocalIp.h"

#include <chrono>
#include <iostream>
#include <thread>

namespace shareaudio {

ConsoleUi::ConsoleUi(AppController& controller)
    : controller_(controller)
    , session_(controller.config())
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

        auto start = session_.start_sharing(mode);
        if (!start.ok()) {
            std::cerr << start.error().message << '\n';
            return 2;
        }

        std::cout << "Sharing on TCP port " << Defaults::tcp_port << " in " << to_string(mode) << " mode. Press Enter to stop.\n";
        std::string line;
        std::getline(std::cin, line);

        session_.stop_sharing();
        auto stats = session_.status_snapshot();
        std::cout << "Stopped. packets=" << stats.packets_produced
                  << " dropped_packets=" << stats.dropped_packets
                  << " bytes_sent=" << stats.bytes_sent << '\n';
        return 0;
    }
    if (args[0] == "listen") {
        if (args.size() != 2) {
            std::cerr << "Usage: shareaudio_cli listen <host>\n";
            return 2;
        }

        auto start = session_.start_listening(args[1]);
        if (!start.ok()) {
            std::cerr << start.error().message << '\n';
            return 2;
        }

        bool announced = false;
        while (true) {
            auto status = session_.status_snapshot();
            if (!announced && status.mode == SessionMode::Listening) {
                std::cout << "Listening to " << args[1] << ':' << Defaults::tcp_port << " in " << to_string(status.detected_mode)
                          << " mode. Stop with Ctrl+C or transmitter disconnect.\n";
                announced = true;
            }
            if (status.mode == SessionMode::Idle) {
                if (!status.last_error.empty()) {
                    std::cerr << status.last_error << '\n';
                }
                std::cout << "Stopped. received_bytes=" << status.bytes_received
                          << " played_bytes=" << status.bytes_played
                          << " underruns=" << status.underruns << '\n';
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
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
    for (const auto& device : session_.list_capture_devices()) {
        std::cout << "  " << device.id << " - " << device.name << (device.is_default ? " (default)" : "") << '\n';
    }

    std::cout << "Playback devices:\n";
    for (const auto& device : session_.list_playback_devices()) {
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
