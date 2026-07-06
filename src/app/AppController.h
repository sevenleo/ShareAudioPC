#pragma once

#include "app/Config.h"
#include "app/Result.h"

#include <string>

namespace shareaudio {

enum class AppMode {
    Idle,
    Transmitter,
    Receiver
};

struct AppStatus {
    AppMode mode { AppMode::Idle };
    AudioMode audio_mode { AudioMode::Balanced };
    std::size_t connected_clients {};
    std::size_t jitter_buffer_depth {};
};

class AppController {
public:
    explicit AppController(AppConfig config = {});

    Result<void> set_audio_mode(AudioMode mode);
    Result<void> set_capture_device(std::string device_id);
    Result<void> set_playback_device(std::string device_id);
    Result<void> start_transmitter();
    Result<void> stop_transmitter();
    Result<void> connect_receiver(std::string host);
    Result<void> disconnect_receiver();

    [[nodiscard]] AppStatus status() const;
    [[nodiscard]] const AppConfig& config() const;

private:
    AppConfig config_;
    AppStatus status_;
};

} // namespace shareaudio
