#include "ui/ConsoleUi.h"

#include "audio/AudioAbstractions.h"
#include "platform/LocalIp.h"

#include <iostream>

namespace shareaudio {

ConsoleUi::ConsoleUi(AppController& controller)
    : controller_(controller)
{
}

int ConsoleUi::run(const std::vector<std::string>& args)
{
    if (args.empty() || args[0] == "--help" || args[0] == "-h") {
        print_help();
        return 0;
    }
    if (args[0] == "--list-ips") {
        print_local_ips();
        return 0;
    }
    if (args[0] == "--status") {
        print_status();
        return 0;
    }
    if (args[0] == "--mode" && args.size() >= 2) {
        auto mode = parse_audio_mode(args[1]);
        if (!mode) {
            std::cerr << "Unknown mode: " << args[1] << '\n';
            return 2;
        }
        auto result = controller_.set_audio_mode(*mode);
        if (!result.ok()) {
            std::cerr << result.error().message << '\n';
            return 2;
        }
        print_status();
        return 0;
    }
    if (args[0] == "--start-transmitter") {
        auto result = controller_.start_transmitter();
        if (!result.ok()) {
            std::cerr << result.error().message << '\n';
            return 2;
        }
        print_status();
        return 0;
    }
    if (args[0] == "--connect" && args.size() >= 2) {
        auto result = controller_.connect_receiver(args[1]);
        if (!result.ok()) {
            std::cerr << result.error().message << '\n';
            return 2;
        }
        print_status();
        return 0;
    }

    std::cerr << "Unknown command. Use --help.\n";
    return 2;
}

void ConsoleUi::print_help() const
{
    std::cout
        << "ShareAudioLite " << SHAREAUDIO_VERSION << "\n\n"
        << "Commands:\n"
        << "  --help                 Show this help text\n"
        << "  --list-ips             Show local IP addresses\n"
        << "  --status               Show app status\n"
        << "  --mode <mode>          Select quality, balanced, or ultrafast\n"
        << "  --start-transmitter    Enter transmitter mode (foundation stub)\n"
        << "  --connect <ip>         Enter receiver mode unless IP is local\n\n"
        << "Audio devices exposed by the current test backends:\n";

    GeneratedToneCapture capture;
    NullAudioPlayback playback;
    for (const auto& device : capture.devices()) {
        std::cout << "  capture: " << device.id << " - " << device.name << '\n';
    }
    for (const auto& device : playback.devices()) {
        std::cout << "  playback: " << device.id << " - " << device.name << '\n';
    }
}

void ConsoleUi::print_status() const
{
    const auto status = controller_.status();
    const char* mode = "idle";
    if (status.mode == AppMode::Transmitter) {
        mode = "transmitter";
    } else if (status.mode == AppMode::Receiver) {
        mode = "receiver";
    }

    std::cout
        << "mode=" << mode
        << " audio_mode=" << to_string(status.audio_mode)
        << " connected_clients=" << status.connected_clients
        << " jitter_buffer_depth=" << status.jitter_buffer_depth
        << '\n';
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
