#pragma once

#include "app/Config.h"
#include "app/Result.h"
#include "audio/AudioTypes.h"
#include "audio/AudioPipeline.h"
#include "network/PcmBroadcastServer.h"
#include "network/TcpSocket.h"
#include "storage/RecentDevices.h"

#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace shareaudio {

class IAudioCapture;
class IAudioPlayback;

enum class SessionMode {
    Idle,
    Sharing,
    Connecting,
    Listening,
    SharingConnecting,
    SharingListening
};

enum class SessionAudioBackend {
    Default,
    Fake
};

enum class SystemVolumeTrackingState {
    Disabled,
    Active,
    Fallback,
    Unsupported
};

struct SessionControllerOptions {
    SessionAudioBackend backend { SessionAudioBackend::Default };
    bool allow_self_connection { false };
    std::filesystem::path recent_devices_path;
};

struct SessionStatus {
    SessionMode mode { SessionMode::Idle };
    bool sharing_active {};
    bool receiver_connecting {};
    bool receiver_listening {};
    bool mute_local_audio_requested {};
    bool local_audio_muted {};
    AudioMode selected_mode { AudioMode::Balanced };
    VolumeMode volume_mode { VolumeMode::Full };
    float system_volume_gain { 1.0f };
    SystemVolumeTrackingState system_volume_tracking { SystemVolumeTrackingState::Disabled };
    AudioMode detected_mode { AudioMode::Balanced };
    bool has_detected_mode {};
    std::string host;
    std::uint16_t port { Defaults::tcp_port };
    std::size_t connected_clients {};
    std::size_t bytes_sent {};
    std::size_t bytes_received {};
    std::size_t bytes_played {};
    std::size_t packets_produced {};
    std::size_t dropped_packets {};
    std::size_t underruns {};
    std::size_t jitter_buffer_depth {};
    std::string last_error;
    std::vector<std::string> log_events;
};

class SessionController {
public:
    explicit SessionController(AppConfig config = {}, SessionControllerOptions options = {});
    ~SessionController();

    SessionController(const SessionController&) = delete;
    SessionController& operator=(const SessionController&) = delete;

    Result<void> start_sharing(
        AudioMode mode,
        std::string capture_device_id = {},
        VolumeMode volume_mode = VolumeMode::Full);
    Result<void> set_volume_mode(VolumeMode volume_mode);
    Result<void> set_local_audio_muted(bool muted);
    Result<void> stop_sharing();
    Result<void> start_listening(std::string host, std::string playback_device_id = {});
    Result<void> stop_listening();
    Result<void> stop();

    [[nodiscard]] std::vector<std::string> list_local_ips() const;
    [[nodiscard]] std::vector<AudioDevice> list_capture_devices() const;
    [[nodiscard]] std::vector<AudioDevice> list_playback_devices() const;
    [[nodiscard]] SessionStatus status_snapshot() const;
    [[nodiscard]] AppConfig config_snapshot() const;
    [[nodiscard]] std::vector<std::string> recent_devices() const;
    [[nodiscard]] bool wait_until_idle(std::chrono::milliseconds timeout);

private:
    std::unique_ptr<IAudioCapture> make_capture() const;
    std::unique_ptr<IAudioPlayback> make_playback() const;
    void set_error(std::string message);
    void add_log(std::string message);
    void add_log_locked(const std::string& message);
    void finish_listening();
    void start_volume_monitor(IAudioCapture* capture, PcmTransmitterPipeline* transmitter);
    void stop_volume_monitor();
    [[nodiscard]] SessionMode derived_mode_locked() const;

    mutable std::mutex mutex_;
    std::condition_variable state_changed_;
    AppConfig config_;
    SessionControllerOptions options_;
    RecentDevices recent_;
    bool sharing_active_ {};
    SessionMode receiver_mode_ { SessionMode::Idle };
    AudioMode detected_mode_ { AudioMode::Balanced };
    bool has_detected_mode_ {};
    std::string last_error_;
    std::vector<std::string> log_events_;
    SessionStatus last_completed_status_;

    std::unique_ptr<IAudioCapture> capture_;
    std::unique_ptr<IAudioPlayback> playback_;
    std::unique_ptr<PcmBroadcastServer> server_;
    std::unique_ptr<PcmTransmitterPipeline> transmitter_;
    std::unique_ptr<PcmReceiverPipeline> receiver_;
    std::shared_ptr<TcpSocket> receiver_socket_;
    std::thread transmitter_worker_;
    std::thread volume_monitor_worker_;
    std::thread listener_worker_;
    std::atomic_bool transmitter_running_ { false };
    std::atomic_bool volume_monitor_running_ { false };
    std::atomic_bool volume_monitor_ready_ { false };
    std::atomic<float> system_volume_gain_ { 1.0f };
    std::atomic<SystemVolumeTrackingState> system_volume_tracking_ { SystemVolumeTrackingState::Disabled };
    std::atomic_bool mute_local_audio_requested_ { false };
    std::atomic_bool local_audio_muted_ { false };
    std::atomic_bool listener_stop_requested_ { false };
};

const char* to_string(SessionMode mode);
const char* to_string(SystemVolumeTrackingState state);

} // namespace shareaudio
