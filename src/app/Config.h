#pragma once

#include "app/Result.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace shareaudio {

enum class SampleFormat {
    Signed16Pcm
};

enum class AudioMode {
    Balanced,
    Fast,
    Efficient
};

enum class VolumeMode {
    Full,
    System
};

struct Defaults {
    static constexpr std::uint16_t tcp_port = 33777;
    static constexpr int sample_rate = 48000;
    static constexpr int channel_count = 2;
    static constexpr int bytes_per_sample = 2;
    static constexpr std::size_t balanced_packet_bytes = 2048;
    static constexpr std::size_t fast_packet_bytes = 1024;
    static constexpr int opus_bitrate_bps = 128000;
    static constexpr std::size_t opus_frame_size_samples = 960;
    static constexpr std::size_t opus_pcm_frame_bytes = opus_frame_size_samples * channel_count * bytes_per_sample;
    static constexpr std::size_t max_opus_frame_bytes = 4096;
};

struct AudioFormat {
    int sample_rate { Defaults::sample_rate };
    int channels { Defaults::channel_count };
    SampleFormat sample_format { SampleFormat::Signed16Pcm };

    [[nodiscard]] int bytes_per_frame() const;
};

struct NetworkConfig {
    std::string bind_address { "0.0.0.0" };
    std::uint16_t port { Defaults::tcp_port };
};

struct TransmitterConfig {
    AudioMode mode { AudioMode::Balanced };
    VolumeMode volume_mode { VolumeMode::Full };
    std::string capture_device_id;
    NetworkConfig network;
};

struct ReceiverConfig {
    std::string host;
    std::uint16_t port { Defaults::tcp_port };
    AudioMode mode { AudioMode::Balanced };
    std::string playback_device_id;
};

struct AppConfig {
    AudioFormat audio;
    TransmitterConfig transmitter;
    ReceiverConfig receiver;
};

std::string to_string(AudioMode mode);
std::optional<AudioMode> parse_audio_mode(std::string value);
std::string to_string(VolumeMode mode);
std::optional<VolumeMode> parse_volume_mode(std::string value);
std::string to_json(const AppConfig& config);
Result<AppConfig> load_config_file(const std::filesystem::path& path);
Result<void> save_config_file(const std::filesystem::path& path, const AppConfig& config);
std::filesystem::path default_config_path();
Result<void> validate(const AppConfig& config);
std::size_t packet_size_for_mode(AudioMode mode);

} // namespace shareaudio
