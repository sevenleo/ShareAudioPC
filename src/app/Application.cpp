#include "app/Application.h"

#include "app/AppController.h"
#include "app/StartupConfig.h"
#include "ui/ConsoleUi.h"

#include <iostream>
#include <string>
#include <vector>

namespace shareaudio {

int Application::run(int argc, char** argv)
{
    std::vector<std::string> args;
    args.reserve(argc > 0 ? static_cast<std::size_t>(argc - 1) : 0);
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    // If no CLI arguments were provided, try loading shareaudio.cfg
    if (args.empty()) {
        auto cfg_result = load_default_startup_config();
        if (cfg_result.ok()) {
            const auto& cfg = cfg_result.value();
            if (cfg.autostart) {
                if (cfg.is_server()) {
                    args.push_back("share");
                    if (cfg.has_audio_mode()) {
                        args.push_back("--audio-mode");
                        args.push_back(cfg.audio_mode);
                    }
                    if (cfg.has_device_id()) {
                        args.push_back("--device");
                        args.push_back(cfg.device_id);
                    }
                    std::cout << "[cfg] Auto-starting server from shareaudio.cfg\n";
                } else if (cfg.is_client()) {
                    if (!cfg.has_server_ip()) {
                        std::cerr << "[cfg] AUTOSTART=true and MODE=client, but SERVER_IP is missing in shareaudio.cfg.\n";
                        return 2;
                    }
                    args.push_back("listen");
                    args.push_back(cfg.server_ip);
                    if (cfg.has_playback_device_id()) {
                        args.push_back("--device");
                        args.push_back(cfg.playback_device_id);
                    }
                    std::cout << "[cfg] Auto-connecting to " << cfg.server_ip << " from shareaudio.cfg\n";
                } else {
                    std::cerr << "[cfg] AUTOSTART=true but MODE is missing or invalid in shareaudio.cfg. Use MODE=server or MODE=client.\n";
                    return 2;
                }
            }
        }
    }

    AppController controller;
    ConsoleUi ui(controller);
    return ui.run(args);
}

} // namespace shareaudio
