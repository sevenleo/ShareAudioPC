#include "app/AppController.h"

#include "platform/LocalIp.h"

namespace shareaudio {

AppController::AppController(AppConfig config)
    : config_(std::move(config))
{
    status_.audio_mode = config_.transmitter.mode;
}

Result<void> AppController::set_audio_mode(AudioMode mode)
{
    if (status_.mode != AppMode::Idle) {
        return Result<void>::failure(make_error(ErrorCode::InvalidState, "Audio mode can only be changed while idle."));
    }
    config_.transmitter.mode = mode;
    config_.receiver.mode = mode;
    status_.audio_mode = mode;
    return Result<void>::success();
}

Result<void> AppController::set_capture_device(std::string device_id)
{
    if (status_.mode != AppMode::Idle) {
        return Result<void>::failure(make_error(ErrorCode::InvalidState, "Capture device can only be changed while idle."));
    }
    config_.transmitter.capture_device_id = std::move(device_id);
    return Result<void>::success();
}

Result<void> AppController::set_playback_device(std::string device_id)
{
    if (status_.mode != AppMode::Idle) {
        return Result<void>::failure(make_error(ErrorCode::InvalidState, "Playback device can only be changed while idle."));
    }
    config_.receiver.playback_device_id = std::move(device_id);
    return Result<void>::success();
}

Result<void> AppController::start_transmitter()
{
    if (status_.mode != AppMode::Idle) {
        return Result<void>::failure(make_error(ErrorCode::InvalidState, "Cannot start transmitter unless the app is idle."));
    }
    status_.mode = AppMode::Transmitter;
    return Result<void>::success();
}

Result<void> AppController::stop_transmitter()
{
    if (status_.mode != AppMode::Transmitter) {
        return Result<void>::failure(make_error(ErrorCode::InvalidState, "Transmitter is not running."));
    }
    status_.mode = AppMode::Idle;
    status_.connected_clients = 0;
    return Result<void>::success();
}

Result<void> AppController::connect_receiver(std::string host)
{
    if (status_.mode != AppMode::Idle) {
        return Result<void>::failure(make_error(ErrorCode::InvalidState, "Cannot connect receiver unless the app is idle."));
    }
    auto local = is_local_address(host);
    if (!local.ok()) {
        return Result<void>::failure(local.error());
    }
    if (local.value()) {
        return Result<void>::failure(make_error(ErrorCode::InvalidArgument, "Refusing to connect receiver to this same machine."));
    }

    config_.receiver.host = std::move(host);
    status_.mode = AppMode::Receiver;
    return Result<void>::success();
}

Result<void> AppController::disconnect_receiver()
{
    if (status_.mode != AppMode::Receiver) {
        return Result<void>::failure(make_error(ErrorCode::InvalidState, "Receiver is not connected."));
    }
    status_.mode = AppMode::Idle;
    status_.jitter_buffer_depth = 0;
    return Result<void>::success();
}

AppStatus AppController::status() const
{
    return status_;
}

const AppConfig& AppController::config() const
{
    return config_;
}

} // namespace shareaudio
