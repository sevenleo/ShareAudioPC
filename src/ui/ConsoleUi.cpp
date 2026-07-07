#include "ui/ConsoleUi.h"

#include "platform/LocalIp.h"
#include "app/SingleInstance.h"

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
        std::string capture_device_id;
        for (std::size_t i = 1; i < args.size(); i += 2) {
            if (i + 1 >= args.size()) {
                std::cerr << "Missing value for argument: " << args[i] << "\n";
                return 2;
            }
            if (args[i] == "--mode") {
                auto parsed = parse_audio_mode(args[i + 1]);
                if (!parsed || *parsed == AudioMode::Quality) {
                    std::cerr << "share supports --mode balanced or --mode ultrafast. Quality mode requires Opus implementation.\n";
                    return 2;
                }
                mode = *parsed;
            } else if (args[i] == "--device" || args[i] == "-d") {
                capture_device_id = args[i + 1];
            } else {
                std::cerr << "Unknown argument: " << args[i] << "\n";
                std::cerr << "Usage: shareaudio_cli share [--mode balanced|ultrafast] [--device <device_id>]\n";
                return 2;
            }
        }

        // Enforce single instance
        (void)enforce_single_instance();

        // Print available local IPs
        std::cout << "Available local IP addresses for connection:\n";
        print_local_ips();

        auto start = session_.start_sharing(mode, capture_device_id);
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
        if (args.size() < 2) {
            std::cerr << "Usage: shareaudio_cli listen <host> [--device <device_id>]\n";
            return 2;
        }
        std::string host = args[1];
        std::string playback_device_id;
        for (std::size_t i = 2; i < args.size(); i += 2) {
            if (i + 1 >= args.size()) {
                std::cerr << "Missing value for argument: " << args[i] << "\n";
                return 2;
            }
            if (args[i] == "--device" || args[i] == "-d") {
                playback_device_id = args[i + 1];
            } else {
                std::cerr << "Unknown argument: " << args[i] << "\n";
                std::cerr << "Usage: shareaudio_cli listen <host> [--device <device_id>]\n";
                return 2;
            }
        }

        // Enforce single instance
        (void)enforce_single_instance();

        auto start = session_.start_listening(host, playback_device_id);
        if (!start.ok()) {
            std::cerr << start.error().message << '\n';
            return 2;
        }

        bool announced = false;
        while (true) {
            auto status = session_.status_snapshot();
            if (!announced && status.mode == SessionMode::Listening) {
                std::cout << "Listening to " << host << ':' << Defaults::tcp_port << " in " << to_string(status.detected_mode)
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
        << "  shareaudio_cli share [--mode balanced|ultrafast] [--device <device_id>]\n"
        << "  shareaudio_cli listen <host> [--device <device_id>]\n"
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
