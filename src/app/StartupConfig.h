#pragma once

#include "app/Config.h"
#include "app/Result.h"

#include <filesystem>
#include <optional>
#include <string>

namespace shareaudio {

struct StartupConfig {
    bool autostart { false };
    bool traymode { false };
    bool startintray { false };
    std::string mode;               // "server", "client", or GUI-only "both"
    std::string audio_mode;         // "balanced", "fast", or "efficient"
    std::string volume_mode;        // "full" or Windows-only "system"
    std::string device_id;          // capture device id
    std::string playback_device_id; // playback device id
    std::string server_ip;          // target server IP for client mode

    [[nodiscard]] bool has_mode() const { return !mode.empty(); }
    [[nodiscard]] bool has_audio_mode() const { return !audio_mode.empty(); }
    [[nodiscard]] bool has_volume_mode() const { return !volume_mode.empty(); }
    [[nodiscard]] bool has_device_id() const { return !device_id.empty(); }
    [[nodiscard]] bool has_playback_device_id() const { return !playback_device_id.empty(); }
    [[nodiscard]] bool has_server_ip() const { return !server_ip.empty(); }

    [[nodiscard]] bool is_server() const { return mode == "server"; }
    [[nodiscard]] bool is_client() const { return mode == "client"; }
    [[nodiscard]] bool is_both() const { return mode == "both"; }

    [[nodiscard]] std::optional<AudioMode> parsed_audio_mode() const;
    [[nodiscard]] std::optional<VolumeMode> parsed_volume_mode() const;
};

/// Load startup configuration from a KEY=VALUE text file.
/// Returns a default (empty) StartupConfig if the file does not exist.
/// Returns an error only if the file exists but cannot be read.
Result<StartupConfig> load_startup_config(const std::filesystem::path& cfg_path);

/// Resolve the directory containing the running executable.
/// Uses platform-specific APIs (GetModuleFileNameW on Windows, /proc/self/exe on Linux).
std::filesystem::path executable_directory();

/// Convenience: load from the default location (next to the executable).
Result<StartupConfig> load_default_startup_config();

} // namespace shareaudio
