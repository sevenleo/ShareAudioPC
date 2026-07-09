#include "app/StartupConfig.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <unistd.h>
#include <climits>
#endif

namespace shareaudio {

namespace {

std::string trim(const std::string& s)
{
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return {};
    }
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string to_lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

} // namespace

std::optional<AudioMode> StartupConfig::parsed_audio_mode() const
{
    if (audio_mode.empty()) {
        return std::nullopt;
    }
    return parse_audio_mode(audio_mode);
}

Result<StartupConfig> load_startup_config(const std::filesystem::path& cfg_path)
{
    StartupConfig config;

    if (!std::filesystem::exists(cfg_path)) {
        // File not found is not an error — just return defaults.
        return Result<StartupConfig>::success(config);
    }

    std::ifstream file(cfg_path);
    if (!file.is_open()) {
        return Result<StartupConfig>::failure(
            make_error(ErrorCode::IoError, "Cannot open startup config: " + cfg_path.string()));
    }

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);

        // Skip blank lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Split on first '='
        auto eq_pos = line.find('=');
        if (eq_pos == std::string::npos) {
            continue; // Malformed line — skip silently
        }

        auto key = to_lower(trim(line.substr(0, eq_pos)));
        auto value = trim(line.substr(eq_pos + 1));

        if (key == "autostart") {
            auto lv = to_lower(value);
            config.autostart = (lv == "true" || lv == "1" || lv == "yes" || lv == "on");
        } else if (key == "mode") {
            config.mode = to_lower(value);
        } else if (key == "audio_mode") {
            config.audio_mode = to_lower(value);
        } else if (key == "device_id") {
            config.device_id = value; // Device IDs are case-sensitive
        } else if (key == "playback_device_id") {
            config.playback_device_id = value;
        } else if (key == "server_ip") {
            config.server_ip = value;
        }
        // Unknown keys are silently ignored for forward compatibility
    }

    return Result<StartupConfig>::success(config);
}

std::filesystem::path executable_directory()
{
#ifdef _WIN32
    wchar_t buffer[MAX_PATH] = {};
    DWORD len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (len > 0 && len < MAX_PATH) {
        return std::filesystem::path(buffer).parent_path();
    }
#else
    char buffer[PATH_MAX] = {};
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len > 0) {
        buffer[len] = '\0';
        return std::filesystem::path(buffer).parent_path();
    }
#endif
    // Fallback: current working directory
    return std::filesystem::current_path();
}

Result<StartupConfig> load_default_startup_config()
{
    return load_startup_config(executable_directory() / "shareaudio.cfg");
}

} // namespace shareaudio
