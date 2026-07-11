#include "app/Config.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <regex>
#include <sstream>

namespace shareaudio {
namespace {

constexpr int legacy_default_tcp_port = 8080;

std::string lower_copy(std::string value)
{
    std::ranges::transform(value, value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::optional<int> parse_int_field(const std::string& json, const std::string& key)
{
    const std::regex pattern("\"" + key + "\"\\s*:\\s*(\\d+)");
    std::smatch match;
    if (std::regex_search(json, match, pattern)) {
        return std::stoi(match[1].str());
    }
    return std::nullopt;
}

std::optional<std::string> parse_string_field(const std::string& json, const std::string& key)
{
    const std::regex pattern("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch match;
    if (std::regex_search(json, match, pattern)) {
        return match[1].str();
    }
    return std::nullopt;
}

} // namespace

int AudioFormat::bytes_per_frame() const
{
    return channels * Defaults::bytes_per_sample;
}

std::string to_string(AudioMode mode)
{
    switch (mode) {
    case AudioMode::Balanced:
        return "balanced";
    case AudioMode::Fast:
        return "fast";
    case AudioMode::Efficient:
        return "efficient";
    }
    return "balanced";
}

std::optional<AudioMode> parse_audio_mode(std::string value)
{
    value = lower_copy(std::move(value));
    if (value == "balanced") {
        return AudioMode::Balanced;
    }
    if (value == "fast") {
        return AudioMode::Fast;
    }
    if (value == "efficient") {
        return AudioMode::Efficient;
    }
    return std::nullopt;
}

std::string to_string(VolumeMode mode)
{
    switch (mode) {
    case VolumeMode::Full:
        return "full";
    case VolumeMode::System:
        return "system";
    }
    return "full";
}

std::optional<VolumeMode> parse_volume_mode(std::string value)
{
    value = lower_copy(std::move(value));
    if (value == "full") {
        return VolumeMode::Full;
    }
    if (value == "system") {
        return VolumeMode::System;
    }
    return std::nullopt;
}

std::size_t packet_size_for_mode(AudioMode mode)
{
    switch (mode) {
    case AudioMode::Balanced:
        return Defaults::balanced_packet_bytes;
    case AudioMode::Fast:
        return Defaults::fast_packet_bytes;
    case AudioMode::Efficient:
        return 0;
    }
    return 0;
}

Result<void> validate(const AppConfig& config)
{
    if (config.audio.sample_rate <= 0) {
        return Result<void>::failure(make_error(ErrorCode::ConfigError, "Audio sample rate must be positive."));
    }
    if (config.audio.channels != Defaults::channel_count) {
        return Result<void>::failure(make_error(ErrorCode::ConfigError, "Only stereo audio is supported by the protocol."));
    }
    if (config.transmitter.network.port == 0 || config.receiver.port == 0) {
        return Result<void>::failure(make_error(ErrorCode::ConfigError, "TCP port must be between 1 and 65535."));
    }
    return Result<void>::success();
}

std::string to_json(const AppConfig& config)
{
    std::ostringstream out;
    out << "{\n";
    out << "  \"audio\": {\n";
    out << "    \"sample_rate\": " << config.audio.sample_rate << ",\n";
    out << "    \"channels\": " << config.audio.channels << ",\n";
    out << "    \"sample_format\": \"signed_16_pcm\"\n";
    out << "  },\n";
    out << "  \"transmitter\": {\n";
    out << "    \"mode\": \"" << to_string(config.transmitter.mode) << "\",\n";
    out << "    \"volume_mode\": \"" << to_string(config.transmitter.volume_mode) << "\",\n";
    out << "    \"capture_device_id\": \"" << config.transmitter.capture_device_id << "\",\n";
    out << "    \"bind_address\": \"" << config.transmitter.network.bind_address << "\",\n";
    out << "    \"port\": " << config.transmitter.network.port << "\n";
    out << "  },\n";
    out << "  \"receiver\": {\n";
    out << "    \"host\": \"" << config.receiver.host << "\",\n";
    out << "    \"port\": " << config.receiver.port << ",\n";
    out << "    \"mode\": \"" << to_string(config.receiver.mode) << "\",\n";
    out << "    \"playback_device_id\": \"" << config.receiver.playback_device_id << "\"\n";
    out << "  }\n";
    out << "}\n";
    return out.str();
}

Result<AppConfig> load_config_file(const std::filesystem::path& path)
{
    AppConfig config;
    if (!std::filesystem::exists(path)) {
        return Result<AppConfig>::success(config);
    }

    std::ifstream file(path);
    if (!file) {
        return Result<AppConfig>::failure(make_error(ErrorCode::IoError, "Unable to open config file: " + path.string()));
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string json = buffer.str();

    if (auto sample_rate = parse_int_field(json, "sample_rate")) {
        config.audio.sample_rate = *sample_rate;
    }
    if (auto channels = parse_int_field(json, "channels")) {
        config.audio.channels = *channels;
    }
    if (auto bind = parse_string_field(json, "bind_address")) {
        config.transmitter.network.bind_address = *bind;
    }
    if (auto transmitter_mode = parse_string_field(json, "mode")) {
        if (auto mode = parse_audio_mode(*transmitter_mode)) {
            config.transmitter.mode = *mode;
            config.receiver.mode = *mode;
        }
    }
    if (auto volume_mode = parse_string_field(json, "volume_mode")) {
        if (auto mode = parse_volume_mode(*volume_mode)) {
            config.transmitter.volume_mode = *mode;
        }
    }
    if (auto capture = parse_string_field(json, "capture_device_id")) {
        config.transmitter.capture_device_id = *capture;
    }
    if (auto host = parse_string_field(json, "host")) {
        config.receiver.host = *host;
    }
    if (auto playback = parse_string_field(json, "playback_device_id")) {
        config.receiver.playback_device_id = *playback;
    }
    if (auto port = parse_int_field(json, "port")) {
        if (*port > 0 && *port <= 65535) {
            const auto migrated_port = *port == legacy_default_tcp_port ? Defaults::tcp_port : static_cast<std::uint16_t>(*port);
            config.transmitter.network.port = migrated_port;
            config.receiver.port = migrated_port;
        }
    }

    auto validation = validate(config);
    if (!validation.ok()) {
        return Result<AppConfig>::failure(validation.error());
    }
    return Result<AppConfig>::success(config);
}

Result<void> save_config_file(const std::filesystem::path& path, const AppConfig& config)
{
    auto validation = validate(config);
    if (!validation.ok()) {
        return validation;
    }

    try {
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }
    } catch (const std::filesystem::filesystem_error& error) {
        return Result<void>::failure(make_error(ErrorCode::IoError, "Unable to create config directory: " + std::string(error.what())));
    }

    std::ofstream file(path);
    if (!file) {
        return Result<void>::failure(make_error(ErrorCode::IoError, "Unable to write config file: " + path.string()));
    }
    file << to_json(config);
    return Result<void>::success();
}

std::filesystem::path default_config_path()
{
#ifdef _WIN32
    const char* appdata = std::getenv("APPDATA");
    if (appdata != nullptr) {
        return std::filesystem::path(appdata) / "ShareAudioLite" / "config.json";
    }
#else
    const char* home = std::getenv("HOME");
    if (home != nullptr) {
        return std::filesystem::path(home) / ".config" / "shareaudiolite" / "config.json";
    }
#endif
    return std::filesystem::temp_directory_path() / "shareaudiolite-config.json";
}

} // namespace shareaudio
